#include <sctk_debug.h>
#include "sctk_route.h"
#include "sctk_pmi.h"
#include "sctk_shm_mapper.h"
#include "sctk_handler_pmi.h"
#include "sctk_alloc.h"

#include <sctk_net_tools.h>
#include <sctk_route.h>
#include "sctk_shm_raw_queues.h"
#include "sctk_shm.h"

static int sctk_shm_proc_local_rank_on_node = -1;
static volatile int sctk_shm_driver_initialized = 0;
static char* sctk_qemu_shm_process_filename = NULL;
static size_t sctk_qemu_shm_process_size = 0;
static void* sctk_qemu_shm_shmem_base = NULL;

// FROM Henry S. Warren, Jr.'s "Hacker's Delight."
static long sctk_shm_roundup_powerof2(unsigned long n)
{
    assume( n < ( 1 << 31));
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n+1;
}

char *sctk_get_qemu_shm_process_filename( void )
{
    return sctk_qemu_shm_process_filename;
}

void sctk_set_qemu_shm_process_filename( char *filename )
{
    sctk_qemu_shm_process_filename = filename;
}

size_t sctk_get_qemu_shm_process_size( void )
{
    return sctk_qemu_shm_process_size;
}

void sctk_set_qemu_shm_process_size( size_t size )
{
    sctk_qemu_shm_process_size = size;
}

void *sctk_get_shm_host_pmi_infos(void)
{
    return sctk_qemu_shm_shmem_base;
}

static void 
sctk_network_send_message_endpoint_shm ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
    vcli_cell_t * __cell = NULL;
    while(!__cell)
        __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, endpoint->data.shm.dest);

    /* get free cells from dest queue */
	__cell->size = SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t );
    assume_m(__cell->size < VCLI_CELLS_SIZE, "ERROR: msg too big");

    memcpy( __cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       

    if( SCTK_MSG_SIZE ( msg ) > 0)
        sctk_net_copy_in_buffer(msg, (char*) __cell->data + sizeof(sctk_thread_ptp_message_body_t));          
        
    vcli_raw_push_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell);       
    sctk_complete_and_free_message( msg ); 
}

static void 
sctk_network_notify_matching_message_shm ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}

static void 
sctk_network_notify_perform_message_shm ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
}

static int 
sctk_send_message_from_network_shm ( sctk_thread_ptp_message_t *msg )
{
	if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
	{
		/* No reordering */
		sctk_send_message_try_check ( msg, 1 );
	}
	return 1;
}

static void 
sctk_network_notify_recv_message_shm ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}

static void 
sctk_network_notify_idle_message_shm ( void )
{
    size_t size;
    vcli_cell_t * __cell;
    sctk_thread_ptp_message_t *msg; 
    void *inb, *body;

    if(!sctk_shm_driver_initialized)
        return;

    __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID,sctk_shm_proc_local_rank_on_node); 

    if( __cell == NULL)
        return;
    
    assume_m( __cell->size >= sizeof ( sctk_thread_ptp_message_body_t ), "Incorrect Msg\n");

    size = __cell->size - sizeof ( sctk_thread_ptp_message_body_t ) + sizeof ( sctk_thread_ptp_message_t );
    msg = sctk_malloc ( size );
    assume(msg != NULL);

    /* copy header */ 
    inb = __cell->data;
    memcpy( (char *) msg, __cell->data, sizeof ( sctk_thread_ptp_message_body_t ));       
    if( size > sizeof ( sctk_thread_ptp_message_t ))
    {
        /* copy msg */
        body = ( char * ) msg + sizeof ( sctk_thread_ptp_message_t );
        inb += sizeof ( sctk_thread_ptp_message_body_t );
        size = size -  sizeof ( sctk_thread_ptp_message_t );
        memcpy(body, inb, size);       
    }
    
    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
	msg->tail.message_type = SCTK_MESSAGE_NETWORK;
	
    if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
	    return;
    
    vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
  
	sctk_rebuild_header ( msg );
	sctk_reinit_header ( msg, sctk_free, sctk_net_message_copy );
    
    sctk_send_message_from_network_shm ( msg );
}

static void 
sctk_network_notify_any_source_message_shm ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

}

/********************************************************************/
/* SHM Init                                                         */
/********************************************************************/

void sctk_shm_host_pmi_init( void )
{
	int sctk_shm_vmhost_node_rank;
	int sctk_shm_vmhost_node_number;
	int sctk_vmhost_shm_local_process_rank;
	int sctk_vmhost_shm_local_process_number;
    sctk_shm_guest_pmi_infos_t *shm_host_pmi_infos;

    shm_host_pmi_infos = (sctk_shm_guest_pmi_infos_t*) sctk_get_shm_host_pmi_infos();
    
	sctk_pmi_get_node_rank ( &sctk_shm_vmhost_node_rank );
	sctk_pmi_get_node_number ( &sctk_shm_vmhost_node_number );
	sctk_pmi_get_process_on_node_rank ( &sctk_vmhost_shm_local_process_rank );
	sctk_pmi_get_process_on_node_number ( &sctk_vmhost_shm_local_process_number );
    
    /* Set pmi infos for SHM infos */ 
    
    //shm_host_pmi_infos->sctk_pmi_procid = ;
    //shm_host_pmi_infos->sctk_pmi_nprocs = ; 
}

static void *
sctk_shm_add_region_slave(sctk_size_t size,sctk_alloc_mapper_handler_t * handler) 
{
	//vars
	char *filename;
	int fd;
	void *ptr;

	//check args
	assert(handler != NULL);
	assert(handler->recv_handler != NULL);
	assert(handler->send_handler != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//get filename
	filename = handler->recv_handler(handler->option);
	assume_m(filename != NULL,"Failed to get the SHM filename.");

	//firt try to map
	fd = sctk_shm_mapper_slave_open(filename);
	ptr = sctk_shm_mapper_mmap(NULL,fd,size);

    sctk_pmi_barrier();

	close(fd);
	free(filename);
	return ptr;
}

static void *
sctk_shm_add_region_master(sctk_size_t size,sctk_alloc_mapper_handler_t * handler)
{
	//vars
	char *filename;
	int fd;
	void *ptr;
    bool status;
	
	//check args
	assert(handler != NULL);
	assert(handler->recv_handler != NULL);
	assert(handler->send_handler != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//get filename
	filename = handler->gen_filename(handler->option);

	//create file and map it
	fd = sctk_shm_mapper_create_shm_file(filename,size);
	ptr = sctk_shm_mapper_mmap(NULL,fd,size);

	//sync filename
	status = handler->send_handler(filename,handler->option);
	assume_m(status,"Fail to send the SHM filename to other participants.");

    sctk_pmi_barrier();

	close(fd);
	sctk_shm_mapper_unlink(filename);

	//free temp memory and return
	free(filename);
	return ptr;
}

static void* sctk_shm_add_region(sctk_size_t size,sctk_shm_mapper_role_t role,
            sctk_alloc_mapper_handler_t * handler)
{
	//errors
	assume_m(handler != NULL,"You must provide a valid handler to exchange the SHM filename.");
	if(size == 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE != 0){
        printf("Size must be non NULL and multiple of page size (%lu bytes), get %lu\n",SCTK_SHM_MAPPER_PAGE_SIZE,size);
        abort();
    }
	//select the method
	switch(role)
	{
		case SCTK_SHM_MAPPER_ROLE_MASTER:
		    return sctk_shm_add_region_master(size,handler);
		case SCTK_SHM_MAPPER_ROLE_SLAVE:
			return sctk_shm_add_region_slave(size,handler);
		default:
			assume_m(0,"Invalid role on sctk_shm_mapper.");
	}
}

static void sctk_shm_add_route(int dest,int shm_dest,sctk_rail_info_t *rail )
{
	sctk_endpoint_t *new_route;

    /* Allocate a new route */
    new_route = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
    assume ( new_route != NULL );

    sctk_endpoint_init ( new_route, dest, rail, ROUTE_ORIGIN_STATIC );

    new_route->data.shm.dest = shm_dest;

    /* Add the new route */
    sctk_rail_add_static_route ( rail, dest, new_route );

    /* set the route as connected */
    sctk_endpoint_set_state ( new_route, STATE_CONNECTED );
    
    return;
}

static void sctk_shm_init_raw_queue(size_t sctk_shmem_size, int sctk_shmem_cells_num, int rank)
{
    char buffer[16];
    void *shm_base = NULL;
    sctk_shm_mapper_role_t shm_role;
    struct sctk_alloc_mapper_handler_s *pmi_handler = NULL;

    if( sctk_mpc_is_vmguest() )
    {
        printf("MPC is vmguest process\n");
        shm_base = sctk_mpc_get_vmguest_shmem_base();
        sctk_qemu_shm_shmem_base = shm_base;
        shm_base += sizeof( sctk_shm_guest_pmi_infos_t );
        sctk_vcli_raw_infos_add( shm_base, sctk_shmem_size, sctk_shmem_cells_num, rank );
    }
    else
    {
        sprintf(buffer, "%d", rank);
        pmi_handler = sctk_shm_pmi_handler_init(buffer);
        shm_role = (sctk_shm_proc_local_rank_on_node == rank) ? SCTK_SHM_MAPPER_ROLE_MASTER : SCTK_SHM_MAPPER_ROLE_SLAVE; 
        shm_base = sctk_shm_add_region(sctk_shmem_size,shm_role,pmi_handler);
        if( shm_role == SCTK_SHM_MAPPER_ROLE_MASTER && sctk_mpc_is_vmhost() )
        {
            printf("MPC is vmhost process\n");
            sctk_qemu_shm_shmem_base = shm_base;
            shm_base += sizeof( sctk_shm_guest_pmi_infos_t );
        }
        sctk_vcli_raw_infos_add( shm_base, sctk_shmem_size, sctk_shmem_cells_num, rank );
        if( shm_role == SCTK_SHM_MAPPER_ROLE_MASTER )
        {
            vcli_raw_reset_infos_reset(rank);
            sctk_set_qemu_shm_process_filename(pmi_handler->gen_filename(pmi_handler->option));
            sctk_set_qemu_shm_process_size(sctk_shmem_size);
        }
        sctk_shm_pmi_handler_free(pmi_handler);
    }
}

void sctk_shm_check_raw_queue(int local_process_number)
{
    int i;
#ifdef SCTK_SHM_CHECK_RAW_QUEUE
    for( i=0; i < local_process_number; i++)
    {
        assume_m( vcli_raw_empty_queue(SCTK_QEMU_SHM_SEND_QUEUE_ID,i),"Queue must be empty")
        assume_m( vcli_raw_empty_queue(SCTK_QEMU_SHM_RECV_QUEUE_ID,i),"Queue must be empty")
        assume_m( vcli_raw_empty_queue(SCTK_QEMU_SHM_CMPL_QUEUE_ID,i),"Queue must be empty")
        assume_m(!vcli_raw_empty_queue(SCTK_QEMU_SHM_FREE_QUEUE_ID,i),"Queue must be full")
    }
    sctk_pmi_barrier();
#endif /* SCTK_SHM_CHECK_RAW_QUEUE */
}

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

void sctk_network_init_shm ( sctk_rail_info_t *rail )
{
    int i, local_process_rank, local_process_number, first_proc_on_node;
    size_t sctk_shmem_size;
    int sctk_shmem_cells_num;

	/* Register Hooks in rail */
	rail->send_message_endpoint = sctk_network_send_message_endpoint_shm;
	rail->notify_recv_message = sctk_network_notify_recv_message_shm;
	rail->notify_matching_message = sctk_network_notify_matching_message_shm;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_shm;
	rail->notify_perform_message = sctk_network_notify_perform_message_shm;
	rail->notify_idle_message = sctk_network_notify_idle_message_shm;
	rail->send_message_from_network = sctk_send_message_from_network_shm;

    rail->network_name = "SHM";
	sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, NULL );

	sctk_shmem_cells_num = rail->runtime_config_driver_config->driver.value.shm.cells_num;
    sctk_shmem_size = sctk_shm_get_raw_queues_size(sctk_shmem_cells_num);

    if( sctk_mpc_is_vmhost() || sctk_mpc_is_vmguest() )
        sctk_shmem_size = sctk_shm_roundup_powerof2(sctk_shmem_size);

    sctk_pmi_get_process_on_node_rank(&local_process_rank);
    sctk_pmi_get_process_on_node_number(&local_process_number);
    sctk_shm_proc_local_rank_on_node = local_process_rank;
    first_proc_on_node = sctk_get_process_rank() - local_process_rank;
    
    if( sctk_mpc_is_vmguest())
    {
        sctk_vcli_raw_infos_init( 1 );
        sctk_shm_init_raw_queue(sctk_shmem_size, sctk_shmem_cells_num,0);
        int process_number = sctk_get_process_rank();
        for( i=0; i<process_number; i++)
            sctk_shm_add_route(i,0,rail);
    }
    else
    {
        sctk_vcli_raw_infos_init(local_process_number);
        for( i=0; i<local_process_number; i++)
        {
            sctk_shm_init_raw_queue(sctk_shmem_size, sctk_shmem_cells_num, i);
            sctk_shm_add_route(first_proc_on_node+i,i,rail);
        }
    }

    sctk_shm_driver_initialized = 1;
}

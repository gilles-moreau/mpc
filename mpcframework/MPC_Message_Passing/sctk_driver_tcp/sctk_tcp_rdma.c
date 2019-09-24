/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_debug.h>
#include <sctk_net_tools.h>
#include <sctk_tcp_toolkit.h>

/**
 * Define TCP RDMA values
 */
typedef enum
{
    SCTK_RDMA_MESSAGE_HEADER, /**< It is an RDMA header */
    SCTK_RDMA_READ,           /**< RDMA read */
    SCTK_RDMA_WRITE           /**< RDMA write */
} sctk_tcp_rdma_type_t;

/**
 * Called when a net message matched with a local RECV.
 * \param[in] tmp the request object (local-recv,remote-send)
 */
static void sctk_tcp_rdma_message_copy ( sctk_message_to_copy_t *tmp )
{
	sctk_endpoint_t *route;
	int fd;

	route = tmp->msg_send->tail.route_table;

	fd = route->data.tcp.fd;
	sctk_spinlock_lock ( & ( route->data.tcp.lock ) );
	{
		sctk_tcp_rdma_type_t op_type;
		op_type = SCTK_RDMA_READ;
		mpc_common_io_safe_write ( fd, &op_type, sizeof ( sctk_tcp_rdma_type_t ) );
		mpc_common_io_safe_write ( fd, & ( tmp->msg_send->tail.rdma_src ), sizeof ( void * ) );
		mpc_common_io_safe_write ( fd, & ( tmp ), sizeof ( void * ) );
	}
	sctk_spinlock_unlock ( & ( route->data.tcp.lock ) );
}

/************************************************************************/
/* Network Hooks                                                        */
/************************************************************************/

/**
 * Polling function created for each route.
 * \param[in] tmp the endpoint to progress.
 */
static void *sctk_tcp_rdma_thread ( sctk_endpoint_t *tmp )
{
	int fd;
	fd = tmp->data.tcp.fd;

	sctk_nodebug ( "Rail %d from %d launched", tmp->rail->rail_number,
	               tmp->key.destination );

	while ( 1 )
	{
		sctk_thread_ptp_message_t *msg;
		size_t size;
		sctk_tcp_rdma_type_t op_type;
		ssize_t res;

		res = mpc_common_io_safe_read ( fd, ( char * ) &op_type, sizeof ( sctk_tcp_rdma_type_t ) );

		if ( res < (ssize_t)sizeof ( sctk_tcp_rdma_type_t ) )
		{
			return NULL;
		}

		switch ( op_type )
		{
			case SCTK_RDMA_MESSAGE_HEADER:
			{
				size = sizeof ( sctk_thread_ptp_message_t );
				msg = sctk_malloc ( size );

				/* Recv header*/
				sctk_nodebug ( "Read %d", sizeof ( sctk_thread_ptp_message_body_t ) );
				mpc_common_io_safe_read ( fd, ( char * ) msg, sizeof ( sctk_thread_ptp_message_body_t ) );
				mpc_common_io_safe_read ( fd, & ( msg->tail.rdma_src ), sizeof ( void * ) );
				msg->tail.route_table = tmp;

				SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
				msg->tail.message_type = SCTK_MESSAGE_NETWORK;

				sctk_rebuild_header ( msg );
				sctk_reinit_header ( msg, sctk_free, sctk_tcp_rdma_message_copy );

				sctk_nodebug ( "MSG RECV|%s|", ( char * ) body );

				sctk_nodebug ( "Msg recved" );
				tmp->rail->send_message_from_network ( msg );
				break;
			}

			case SCTK_RDMA_READ :
			{
				sctk_message_to_copy_t *copy_ptr;
				mpc_common_io_safe_read ( fd, ( char * ) &msg, sizeof ( void * ) );
				mpc_common_io_safe_read ( fd, ( char * ) &copy_ptr, sizeof ( void * ) );

				sctk_spinlock_lock ( & ( tmp->data.tcp.lock ) );
				op_type = SCTK_RDMA_WRITE;
				mpc_common_io_safe_write ( fd, &op_type, sizeof ( sctk_tcp_rdma_type_t ) );
				mpc_common_io_safe_write ( fd, & ( copy_ptr ), sizeof ( void * ) );
				sctk_net_write_in_fd ( msg, fd );
				sctk_spinlock_unlock ( & ( tmp->data.tcp.lock ) );

				sctk_complete_and_free_message ( msg );
				break;
			}

			case SCTK_RDMA_WRITE :
			{
				sctk_message_to_copy_t *copy_ptr;
				sctk_thread_ptp_message_t *send = NULL;
				sctk_thread_ptp_message_t *recv = NULL;

				mpc_common_io_safe_read ( fd, ( char * ) &copy_ptr, sizeof ( void * ) );
				sctk_net_read_in_fd ( copy_ptr->msg_recv, fd );

				send = copy_ptr->msg_send;
				recv = copy_ptr->msg_recv;
				sctk_message_completion_and_free ( send, recv );
				break;
			}

			default:
				not_reachable();
		}

	}

	return NULL;
}

/**
 * Send an TCP RDMA message.
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
static void sctk_network_send_message_tcp_rdma_endpoint ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
	int fd;

	sctk_nodebug ( "send message through rail %d", rail->rail_number );

	sctk_spinlock_lock ( & ( endpoint->data.tcp.lock ) );

	fd = endpoint->data.tcp.fd;

	sctk_tcp_rdma_type_t op_type = SCTK_RDMA_MESSAGE_HEADER;

	mpc_common_io_safe_write ( fd, &op_type, sizeof ( sctk_tcp_rdma_type_t ) );
	mpc_common_io_safe_write ( fd, ( char * ) msg, sizeof ( sctk_thread_ptp_message_body_t ) );
	mpc_common_io_safe_write ( fd, &msg, sizeof ( void * ) );

	sctk_spinlock_unlock ( & ( endpoint->data.tcp.lock ) );

}

/**
 * Not used for this network.
 * \param[in] msg not used
 * \param[in] rail not used
 */
static void sctk_network_notify_recv_message_tcp_rdma ( __UNUSED__ sctk_thread_ptp_message_t *msg, __UNUSED__ sctk_rail_info_t *rail ) {}

/**
 * Not used for this network.
 * \param[in] msg not used
 * \param[in] rail not used
 */
static void sctk_network_notify_matching_message_tcp_rdma ( __UNUSED__ sctk_thread_ptp_message_t *msg, __UNUSED__ sctk_rail_info_t *rail ) {}

/**
 * Not used for this network.
 * \param[in] remote not used
 * \param[in] remote_task_id not used
 * \param[in] polling_task_id not used
 * \param[in] blocking not used
 * \param[in] rail not used
 */
static void sctk_network_notify_perform_message_tcp_rdma ( __UNUSED__ int remote, __UNUSED__ int remote_task_id,__UNUSED__ int polling_task_id, __UNUSED__  int blocking, __UNUSED__ sctk_rail_info_t *rail ) {}

/**
 * Not used for this network.
 * \param[in] rail not used
 */
static void sctk_network_notify_idle_message_tcp_rdma ( __UNUSED__ sctk_rail_info_t *rail ) {}

/**
 * Not used for this network.
 * \param[in] msg not used
 * \param[in] rail not used
 */
static void sctk_network_notify_any_source_message_tcp_rdma ( __UNUSED__ int polling_task_id, __UNUSED__ int blocking,__UNUSED__  sctk_rail_info_t *rail ) {}

/************************************************************************/
/* TCP RDMA Init                                                        */
/************************************************************************/
/**
 * not used for this network.
 * \param[in] rail not used
 */
void sctk_network_finalize_tcp_rdma(__UNUSED__ sctk_rail_info_t* rail)
{
}

/**
 * Procedure to init an TCP RDMA rail.
 * \param[in,out] rail the rail to init.
 */
void sctk_network_init_tcp_rdma ( sctk_rail_info_t *rail )
{
	/* Init RDMA specific infos */
	rail->send_message_endpoint     = sctk_network_send_message_tcp_rdma_endpoint;
	rail->notify_recv_message       = sctk_network_notify_recv_message_tcp_rdma;
	rail->notify_matching_message   = sctk_network_notify_matching_message_tcp_rdma;
	rail->notify_perform_message    = sctk_network_notify_perform_message_tcp_rdma;
	rail->notify_idle_message       = sctk_network_notify_idle_message_tcp_rdma;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_tcp_rdma;
	rail->driver_finalize           = sctk_network_finalize_tcp_rdma;

	int sctk_use_tcp_o_ib = rail->runtime_config_driver_config->driver.value.tcp.tcpoib;

	/* Handle the IPoIB case */
	if ( sctk_use_tcp_o_ib == 0 )
	{
		rail->network_name = "TCP RDMA";
	}
	else
	{
		rail->network_name = "TCP_O_IB RDMA";
	}

	/* Actually Init the TCP layer */
	sctk_network_init_tcp_all ( rail, sctk_use_tcp_o_ib, sctk_tcp_rdma_thread );
}

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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_multirail.h"
#include "sctk_low_level_comm.h"

#include <stdlib.h>

/************************************************************************/
/* Rail Gates                                                           */
/************************************************************************/

/* HERE ARE DEFAULT GATES */

int sctk_rail_gate_boolean( sctk_rail_info_t * rail, sctk_thread_ptp_message_t * message , void * gate_config )
{
	struct sctk_runtime_config_struct_gate_boolean * conf = (struct sctk_runtime_config_struct_gate_boolean *)gate_config;
	return conf->value;
}

int sctk_rail_gate_probabilistic( sctk_rail_info_t * rail, sctk_thread_ptp_message_t * message , void * gate_config )
{
	struct sctk_runtime_config_struct_gate_probabilistic * conf = (struct sctk_runtime_config_struct_gate_probabilistic *)gate_config;
	
	int num = ( rand() % 100 );

	return ( num < conf->probability );
}

int sctk_rail_gate_minsize( sctk_rail_info_t * rail, sctk_thread_ptp_message_t * message , void * gate_config )
{
	struct sctk_runtime_config_struct_gate_min_size * conf = (struct sctk_runtime_config_struct_gate_min_size *)gate_config;
	
	size_t message_size = SCTK_MSG_SIZE( message );
	
	return ( conf->value < message_size );
}

int sctk_rail_gate_maxsize( sctk_rail_info_t * rail, sctk_thread_ptp_message_t * message , void * gate_config )
{
	struct sctk_runtime_config_struct_gate_max_size * conf = (struct sctk_runtime_config_struct_gate_max_size *)gate_config;
	
	size_t message_size = SCTK_MSG_SIZE( message );
	
	return ( message_size < conf->value );
}

struct sctk_gate_context
{
	int (*func)( sctk_rail_info_t * rail, sctk_thread_ptp_message_t * message , void * gate_config );
	void * params;
};


static inline void sctk_gate_get_context( struct sctk_runtime_config_struct_net_gate * gate , struct sctk_gate_context * ctx )
{
	ctx->func = NULL;
	ctx->params = NULL;

	if( ! gate )
		return;

	switch( gate->type )
	{
		case SCTK_RTCFG_net_gate_boolean:
			ctx->func = (int (*)( sctk_rail_info_t *, sctk_thread_ptp_message_t *, void *))gate->value.boolean.gatefunc.value;
			ctx->params = (void *)&gate->value.boolean;
		break;
		case SCTK_RTCFG_net_gate_probabilistic:
			ctx->func = (int (*)( sctk_rail_info_t *, sctk_thread_ptp_message_t *, void *))gate->value.probabilistic.gatefunc.value;
			ctx->params = (void *)&gate->value.probabilistic;
		break;
		case SCTK_RTCFG_net_gate_minsize:
			ctx->func = (int (*)( sctk_rail_info_t *, sctk_thread_ptp_message_t *, void *))gate->value.minsize.gatefunc.value;
			ctx->params = (void *)&gate->value.minsize;		
		break;
		case SCTK_RTCFG_net_gate_maxsize:
			ctx->func = (int (*)( sctk_rail_info_t *, sctk_thread_ptp_message_t *, void *))gate->value.maxsize.gatefunc.value;
			ctx->params = (void *)&gate->value.maxsize;			
		break;
		case SCTK_RTCFG_net_gate_user:
			ctx->func = (int (*)( sctk_rail_info_t *, sctk_thread_ptp_message_t *, void *))gate->value.user.gatefunc.value;
			ctx->params = (void *)&gate->value.user;		
		break;
		
		case SCTK_RTCFG_net_gate_NONE:
		default:
			sctk_fatal("No such gate type");
	}
}

/************************************************************************/
/* sctk_endpoint_list                                                   */
/************************************************************************/

int sctk_endpoint_list_init_entry( sctk_endpoint_list_t * entry, sctk_endpoint_t * endpoint )
{
	if( !endpoint )
	{
		sctk_error("No endpoint provided.. ignoring");
		return 1;
	}
	
	/* TO CHANGE */
	entry->priority = endpoint->rail->runtime_config_rail->priority;
	
	if( !endpoint->rail )
	{
		sctk_error("No rail found in endpoint.. ignoring");
		return 1;	
	}
	
	entry->gates = endpoint->rail->runtime_config_rail->gates;
	entry->gate_count = endpoint->rail->runtime_config_rail->gates_size;
	
	entry->endpoint = endpoint;

	entry->next = NULL;
	entry->prev = NULL;
	
	return 0;
}


sctk_endpoint_list_t * sctk_endpoint_list_new_entry( sctk_endpoint_t * endpoint )
{
	sctk_endpoint_list_t *  ret = sctk_malloc( sizeof( sctk_endpoint_list_t ) );
	
	assume( ret != NULL );
	
	if( sctk_endpoint_list_init_entry( ret, endpoint ) )
	{
		sctk_free( ret );
		return NULL;
	}	
	
	return ret;
}


sctk_endpoint_list_t * sctk_endpoint_list_push( sctk_endpoint_list_t * list, sctk_endpoint_t * endpoint )
{
	sctk_endpoint_list_t * table_start = list;
	sctk_endpoint_list_t * new = sctk_endpoint_list_new_entry( endpoint );

	if( !new )
	{
		sctk_error("Failed to insert a route.. ignoring");
		return list;
	}
	
	sctk_endpoint_list_t * insertion_point = table_start;
	
	/* If list already contains elements */
	if( insertion_point )
	{
	
		/* Go down the list according to priorities */
		while( 1 )
		{
			if( !insertion_point->next )
				break;
			
			if( insertion_point->next->priority < new->priority )
				break;
			
			insertion_point = insertion_point->next;
		}
		
		/* Here we know where we want to be inserted (after insertion_point)  */

		if( new->priority < insertion_point->priority )
		{
			/* Insert after */
			new->next = insertion_point->next;
			
			if( new->next )
			{
				new->next->prev = new;
			}
			
			new->prev = insertion_point;
			insertion_point->next = new;
		}
		else
		{
			/* Insert before (when there is a single entry) */
			new->prev = insertion_point->prev;
			new->next = insertion_point;
			
			if( new->prev )
			{
				new->prev->next = new;
			}
			else
			{
				/* We are list head, update accordingly */
				table_start = new;
			}
			
			insertion_point->prev = new;
			
		}
	}
	else
	{
		/* List is empty, become table start */
		table_start = new;
	}
	
	return table_start;
}

sctk_endpoint_list_t * sctk_endpoint_list_pop( sctk_endpoint_list_t * list, sctk_endpoint_list_t * topop )
{
	sctk_endpoint_list_t * table_start = list;
	
	if( topop->prev )
	{
		topop->prev->next = topop->next;
	}
	else
	{
		/* We were the first element (update table start) */
		table_start = topop->next;
	}
	
	if( topop->next )
	{
		topop->next->prev = topop->prev;
	}
	
	sctk_endpoint_list_free_entry( topop );
	
	return table_start;
}

sctk_endpoint_list_t * sctk_endpoint_list_pop_endpoint( sctk_endpoint_list_t * list, sctk_endpoint_t * topop )
{
	sctk_endpoint_list_t * topop_list_entry = NULL;
	
	while( list )
	{
		if( list->endpoint == topop )
		{
			topop_list_entry = list;
			break;
		}
	}
	
	if( !topop_list_entry )
	{
		sctk_warning("Could not find this entry in table");
		return list;
	}
	
	return sctk_endpoint_list_pop( list, topop_list_entry );
}

void sctk_endpoint_list_free_entry( sctk_endpoint_list_t * entry )
{
	if( ! entry )
		return;
	
	memset( entry, 0, sizeof( sctk_endpoint_list_t) );
	/* Release the entry */
	sctk_free( entry );
}

void sctk_endpoint_list_release( sctk_endpoint_list_t ** list )
{
	sctk_endpoint_list_t * tofree = NULL;
	sctk_endpoint_list_t * head = *list;

	while( head )
	{
		tofree = head;
		head = head->next;
		sctk_endpoint_list_free_entry( tofree );
	};

	*list = NULL;
}


/************************************************************************/
/* sctk_multirail_destination_table_entry                               */
/************************************************************************/

void sctk_multirail_destination_table_entry_init( sctk_multirail_destination_table_entry_t * entry, int destination )
{
	entry->endpoints = NULL;
	sctk_spin_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	entry->endpoints_lock = lckinit;
	entry->destination = destination;
}

void sctk_multirail_destination_table_entry_release( sctk_multirail_destination_table_entry_t * entry )
{
	if( ! entry )
		return;
	
	/* Make sure that we are not acquired */
	sctk_spinlock_write_lock( &entry->endpoints_lock );
	
	sctk_endpoint_list_release( &entry->endpoints );
	entry->destination = -1;
}

void sctk_multirail_destination_table_entry_free( sctk_multirail_destination_table_entry_t * entry )
{
	sctk_multirail_destination_table_entry_release( entry );
	sctk_free( entry );
}

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_entry_new(int destination)
{
	sctk_multirail_destination_table_entry_t * ret = sctk_malloc( sizeof( sctk_multirail_destination_table_entry_t) );
	
	assume( ret != NULL );
	
	sctk_multirail_destination_table_entry_init( ret, destination );
	
	return ret;
}

void sctk_multirail_destination_table_entry_push_endpoint( sctk_multirail_destination_table_entry_t * entry, sctk_endpoint_t * endpoint )
{
	sctk_spinlock_write_lock( &entry->endpoints_lock );
	entry->endpoints =  sctk_endpoint_list_push( entry->endpoints, endpoint );
	sctk_spinlock_write_unlock( &entry->endpoints_lock );
}

void sctk_multirail_destination_table_entry_pop_endpoint( sctk_multirail_destination_table_entry_t * entry, sctk_endpoint_t * endpoint )
{
	sctk_spinlock_write_lock( &entry->endpoints_lock );
	entry->endpoints =  sctk_endpoint_list_pop_endpoint( entry->endpoints, endpoint );
	sctk_spinlock_write_unlock( &entry->endpoints_lock );
}



/************************************************************************/
/* sctk_multirail_destination_table                                     */
/************************************************************************/


static struct sctk_multirail_destination_table ___sctk_multirail_table;

static inline struct sctk_multirail_destination_table * sctk_multirail_destination_table_get()
{
	return &___sctk_multirail_table;
}


/************************************************************************/
/* Endpoint Election Logic                                              */
/************************************************************************/

sctk_endpoint_t * sctk_multirail_ellect_endpoint( sctk_thread_ptp_message_t *msg, int destination_process, int is_process_specific, int is_for_on_demand, sctk_multirail_destination_table_entry_t ** ext_routes )
{
	
	sctk_multirail_destination_table_entry_t * routes = sctk_multirail_destination_table_acquire_routes( destination_process );
	*ext_routes = routes;

	sctk_endpoint_list_t * cur = NULL;
	
	if( routes )
	{
		cur = routes->endpoints;
	}
	
	/* Note that in the case of process specific messages
	 * we return the first matching route which is the
	 * one of Highest priority */
	while( cur && !is_process_specific )
	{
		int this_rail_has_been_elected = 1;
		
		int cur_gate;
		
		
		/* First we want only to check on-demand rails */
		if( !cur->endpoint->rail->connect_on_demand && is_for_on_demand )
		{
			/* No on-demand in this rail */
			this_rail_has_been_elected = 0;
		}
		else
		{
			/* Note that no gate is equivalent to being elected */
			for( cur_gate = 0 ; cur_gate < cur->gate_count ; cur_gate++ )
			{
				struct sctk_gate_context gate_ctx;
				sctk_gate_get_context( &cur->gates[ cur_gate ] , &gate_ctx );
				
				if( ( gate_ctx.func )( cur->endpoint->rail, msg, gate_ctx.params ) == 0 )
				{
					this_rail_has_been_elected = 0;
					break;
				}
				
			}
		}
		
		if( this_rail_has_been_elected )
		{
			break;
		}
		
		cur = cur->next;
	}
	
	if( !cur )
		return NULL;
	
	
	return cur->endpoint;
}



sctk_spinlock_t on_demand_connection_lock = SCTK_SPINLOCK_INITIALIZER;




void sctk_multirail_on_demand_connection( sctk_thread_ptp_message_t *msg )
{
	
	int count = sctk_rail_count();
	int i;
	
	int tab[64];
	assume( count < 64 ); 
	
	memset( tab, 0, 64 * sizeof( int ) );
	
	/* Lets now test each rail to find the one accepting this 
	 * message while being able to setup on demand-connections */
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		
		/* First we want only to check on-demand rails */
		if( !rail->connect_on_demand )
		{
			/* No on-demand in this rail */
			continue;
		}
		
		/* ###############################################################################
		 * Now we test gate functions to detect if this message is elligible on this rail
		 * ############################################################################### */
		
		/* Retrieve data from CTX */
		struct sctk_runtime_config_struct_net_gate * gates = rail->runtime_config_rail->gates;
		int gate_count = rail->runtime_config_rail->gates_size;
		
		int priority = rail->runtime_config_rail->priority;
		
		int this_rail_has_been_elected = 1;
		
		/* Test all gates on this rail */
		int j;
		for( j = 0 ; j < gate_count; j++ )
		{
			struct sctk_gate_context gate_ctx;
			sctk_gate_get_context( &gates[ j ], &gate_ctx );

			if( ( gate_ctx.func )( rail, msg, gate_ctx.params ) == 0 )
			{
				/* This gate does not pass */
				this_rail_has_been_elected = 0;
				break;
			}
		
		}
		
		/* ############################################################################### */
		
		/* If we are here the rail is ellected */
		
		if( this_rail_has_been_elected )
		{
			/* This rail passed the test save its priority 
			 * to prepare the next phase of prority selection */
			tab[i] = priority;
		}
	}
	
	/* Lets now connect to the rails passing the test with the highest priority */
	int current_max = 0;
	int max_offset = -1;

	for( i = 0 ; i < count ; i++ )
	{
		if( ! tab[i] )
			continue;
		
		if( current_max <= tab[i] )
		{
			/* This is the new BEST */
			current_max = tab[i]; 
			max_offset = i;
		}
		
	}


	if( max_offset < 0 )
	{
		/* No rail found */
		sctk_fatal("No route to host == Make sure you have at least one on-demand rail able to satify any type of message");
	}

	/* Enter the critical section to guanrantee the uniqueness of the 
	 * newly created rail by first checking if it is not already present */
	sctk_spinlock_lock( & on_demand_connection_lock );

	int dest_process = SCTK_MSG_DEST_PROCESS ( msg );

	/* First retry to acquire a route for on-demand
	 * in order to avoid double connections */
	sctk_multirail_destination_table_entry_t * routes = NULL;
	sctk_endpoint_t * previous_endpoint = sctk_multirail_ellect_endpoint( msg, dest_process, 0 /* Not Process Specific */, 1 /* For On-Demand */ , &routes );

	/* Check if no previous */
	if( ! previous_endpoint )
	{
		/* If we are here we have elected the on-demand rail with the highest priority and matching this message
		 * initiate the on-demand connection */
		sctk_rail_info_t * elected_rail = sctk_rail_get_by_id ( max_offset );
		(elected_rail->connect_on_demand)( elected_rail,  dest_process );
	}
	
	sctk_multirail_destination_table_relax_routes( routes );
	
	sctk_spinlock_unlock( & on_demand_connection_lock );
}


/************************************************************************/
/* sctk_multirail_hooks                                                 */
/************************************************************************/


void sctk_multirail_send_message( sctk_thread_ptp_message_t *msg )
{
	int retry;
	int destination_process;
	
	int is_process_specific = sctk_is_process_specific_message ( SCTK_MSG_HEADER ( msg ) );
	
	/* If the message is based on signalization we directly rely on routing */
	if( is_process_specific )
	{
		sctk_multirail_destination_table_route_to_process( SCTK_MSG_DEST_PROCESS ( msg ), &destination_process );
	}
	else
	{
		destination_process = SCTK_MSG_DEST_PROCESS ( msg );
	}

	int no_existing_route_matched = 0;

	do
	{
		retry = 0;
		
		if( no_existing_route_matched )
		{
			/* We have to do an on-demand connection */
			sctk_multirail_on_demand_connection( msg );
		}
		
		/* We need to retrieve the route entry in order to be
		 * able to relax it after use */
		sctk_multirail_destination_table_entry_t * routes = NULL;
		
		sctk_endpoint_t * endpoint = sctk_multirail_ellect_endpoint( msg, destination_process, is_process_specific, 0 /* Not for On-Demand */ , &routes );
		
		if( endpoint )
		{
			//sctk_error("RAIL %d", cur->endpoint->rail->rail_number );
			
			/* Prepare reordering */
			sctk_prepare_send_message_to_network_reorder ( msg );
			/* Set rail number in message */
			SCTK_MSG_SET_RAIL_ID( msg, endpoint->rail->rail_number );
			/* Send the message */
			(endpoint->rail->send_message_endpoint)( msg, endpoint );
		}
		else
		{
			/* Here we found a route to this process
			 * however, the gate function did not allow us in
			 * therefore a new on-demand connection is needed */
			no_existing_route_matched = 1;
			retry = 1;
		}

		/* Relax Routes */
		sctk_multirail_destination_table_relax_routes( routes );

	}while( retry );
}


void sctk_multirail_notify_receive( sctk_thread_ptp_message_t * msg )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_recv_message )
		{
			(rail->notify_recv_message)( msg, rail );
		}
	}
}

void sctk_multirail_notify_matching( sctk_thread_ptp_message_t * msg )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_matching_message )
		{
			(rail->notify_matching_message)( msg, rail );
		}
	}
}

void sctk_multirail_notify_perform( int remote, int remote_task_id, int polling_task_id, int blocking  )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_perform_message )
		{
			(rail->notify_perform_message)( remote, remote_task_id, polling_task_id, blocking, rail );
		}
	}
}

void sctk_multirail_notify_idle()
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_idle_message )
		{
			(rail->notify_idle_message)( rail );
		}
	}
}

void sctk_multirail_notify_anysource( int polling_task_id, int blocking )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_any_source_message )
		{
			(rail->notify_any_source_message)(polling_task_id, blocking, rail );
		}
	}
}




/************************************************************************/
/* sctk_multirail_destination_table                                     */
/************************************************************************/


void sctk_multirail_destination_table_init()
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	table->destinations = NULL;
	sctk_spin_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	table->table_lock = lckinit;
	
	/* Register Probing Calls */
	sctk_network_send_message_set ( sctk_multirail_send_message );
	
	sctk_network_notify_recv_message_set ( sctk_multirail_notify_receive );
	sctk_network_notify_matching_message_set ( sctk_multirail_notify_matching );
	sctk_network_notify_perform_message_set ( sctk_multirail_notify_perform );
	sctk_network_notify_idle_message_set ( sctk_multirail_notify_idle );
	sctk_network_notify_any_source_message_set ( sctk_multirail_notify_anysource );
	
	
}

void sctk_multirail_destination_table_release()
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * entry;
	sctk_multirail_destination_table_entry_t * to_free;

	sctk_spinlock_write_lock( &table->table_lock );


	HASH_ITER(hh, table->destinations, to_free, entry)
	{
		HASH_DEL( table->destinations, to_free);
		sctk_multirail_destination_table_entry_free( to_free );
	}
	
}


sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_acquire_routes(int destination )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	sctk_spinlock_read_lock( &table->table_lock );
	
	//sctk_warning("GET endpoint to %d", destination );
	
	HASH_FIND_INT(table->destinations, &destination, dest_entry);
	
	if( dest_entry )
	{
		/* Acquire entries in read */
		sctk_spinlock_read_lock( &dest_entry->endpoints_lock );
	}
	
	sctk_spinlock_read_unlock( &table->table_lock );
	
	return dest_entry;
}

void sctk_multirail_destination_table_relax_routes( sctk_multirail_destination_table_entry_t * entry )
{
	if(!entry)
		return;
	
	/* Just relax the read lock inside a previously acquired entry */
	sctk_spinlock_read_unlock( &entry->endpoints_lock );
}

void sctk_multirail_destination_table_push_endpoint(sctk_endpoint_t * endpoint )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;

	sctk_spinlock_write_lock( &table->table_lock );
	
	HASH_FIND_INT(table->destinations, &endpoint->dest, dest_entry);
	
	if( !dest_entry )
	{
		/* There is no previous destination_table_entry */
		dest_entry = sctk_multirail_destination_table_entry_new(endpoint->dest);
		HASH_ADD_INT( table->destinations, destination, dest_entry );
	}
	else
	{
		sctk_multirail_destination_table_entry_push_endpoint( dest_entry, endpoint );
	}
	
	sctk_multirail_destination_table_entry_push_endpoint( dest_entry, endpoint );
	
	sctk_spinlock_write_unlock( &table->table_lock );
	
}

void sctk_multirail_destination_table_pop_endpoint( sctk_endpoint_t * topop )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	sctk_spinlock_write_lock( &table->table_lock );
	
	HASH_FIND_INT(table->destinations, &topop->dest, dest_entry);
	
	if( dest_entry )
	{
		sctk_multirail_destination_table_entry_pop_endpoint( dest_entry, topop );
	}
	
	sctk_spinlock_write_unlock( &table->table_lock );	
}



void sctk_multirail_destination_table_route_to_process( int destination, int * new_destination )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * tmp;
	sctk_multirail_destination_table_entry_t * entry;

	int distance = -1;

	sctk_spinlock_read_lock( &table->table_lock );


	HASH_ITER(hh, table->destinations, entry, tmp)
	{
		//sctk_warning( " %d --- %d" , destination, entry->destination  );
		if( destination == entry->destination )
		{
			distance = 0;
			*new_destination = destination;
			break;
		}
		
		int cdistance = entry->destination - destination;
		
		if( cdistance < 0 )
			cdistance = -cdistance;
		
		if( ( distance < 0 ) || (cdistance < distance ) )
		{
			distance = cdistance;
			*new_destination = entry->destination;
		}
	}
	
	sctk_spinlock_read_unlock( &table->table_lock );
	
	if( distance == -1 )
	{
		sctk_warning("No route to host %d ", destination);
	}
	
}

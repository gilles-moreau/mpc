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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */

#include <comm.h>

#include <mpc_arch.h>
#include <sctk_low_level_comm.h>
#include <sctk_communicator.h>
#include <mpc_common_helper.h>
#include <sctk_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <sctk_checksum.h>
#include <sctk_reorder.h>
#include <sctk_control_messages.h>
#include <sctk_rail.h>
#include <mpc_launch.h>
#include <sctk_window.h>
#include <mpc_common_profiler.h>
#include <sctk_alloc.h>
#include <coll.h>

#ifdef SCTK_LIB_MODE
	#include "sctk_handle.h"
#endif /* SCTK_LIB_MODE */


#ifdef MPC_USE_INFINIBAND
#include <sctk_ib_device.h>
#endif

/********************************************************************/
/*Structures                                                        */
/********************************************************************/

typedef struct
{
	mpc_lowcomm_communicator_t comm;
	int rank;
} mpc_comm_dest_key_t;

static inline void _mpc_comm_dest_key_init( mpc_comm_dest_key_t *key, mpc_lowcomm_communicator_t comm, int rank )
{
	key->rank = rank;
	key->comm = comm;
}

typedef struct
{
	mpc_common_spinlock_t lock;
	volatile mpc_lowcomm_msg_list_t *list;
} mpc_comm_ptp_list_incomming_t;

typedef struct
{
	volatile mpc_lowcomm_msg_list_t *list;
} mpc_comm_ptp_list_pending_t;

typedef struct
{
	/* Messages are posted to the 'incoming' lists before being merged to the pending list. */
	mpc_comm_ptp_list_incomming_t incomming_send;
	mpc_comm_ptp_list_incomming_t incomming_recv;

	mpc_common_spinlock_t pending_lock;
	/* Messages in the 'pending' lists are waiting to be matched */
	mpc_comm_ptp_list_pending_t pending_send;
	mpc_comm_ptp_list_pending_t pending_recv;
	/* Flag to indicate that new messages have been merged into the 'pending' list */
	char changed;

} mpc_comm_ptp_message_lists_t;

typedef struct mpc_comm_ptp_s
{
	mpc_comm_dest_key_t key;

	mpc_comm_ptp_message_lists_t lists;

	/* Number of pending messages for the MPI task */
	OPA_int_t pending_nb;

	UT_hash_handle hh;

	/* Reorder table */
	sctk_reorder_list_t reorder;
} mpc_comm_ptp_t;


static inline void __mpc_comm_request_init( mpc_lowcomm_request_t *request,
        mpc_lowcomm_communicator_t comm,
        int request_type )
{
	static mpc_lowcomm_request_t the_initial_request =
	{
		.completion_flag = MPC_LOWCOMM_MESSAGE_DONE,
		.header.source = SCTK_PROC_NULL,
		.header.destination = SCTK_PROC_NULL,
		.header.source_task = SCTK_PROC_NULL,
		.header.destination_task = SCTK_PROC_NULL,
		.header.message_tag = SCTK_ANY_TAG,
		.header.communicator = SCTK_COMM_NULL,
		.header.msg_size = 0,
		.source_type = 0,
		.dest_type = 0,
		.request_type = 0,
		.is_null = 0,
		.truncated = 0,
		.msg = NULL,
		.query_fn = NULL,
		.cancel_fn = NULL,
		.wait_fn = NULL,
		.poll_fn = NULL,
		.free_fn = NULL,
		.extra_state = NULL,
		.pointer_to_source_request = NULL,
		.pointer_to_shadow_request = NULL,
		.ptr_to_pin_ctx = NULL
	};

	if ( request != NULL )
	{
		*request = the_initial_request;
		request->request_type = request_type;
		request->header.communicator = comm;
	}
}


/********************************************************************/
/*Functions                                                         */
/********************************************************************/

/*
 * Initialize the 'incoming' list.
 */

static inline void __mpc_comm_ptp_list_incoming_init( mpc_comm_ptp_list_incomming_t *list )
{
	list->list = NULL;
	mpc_common_spinlock_init(&list->lock, 0);
}

/*
 * Initialize the 'pending' list
 */
static inline void __mpc_comm_ptp_list_pending_init( mpc_comm_ptp_list_pending_t *list )
{
	list->list = NULL;
}

/*
 * Initialize a PTP the 'incoming' and 'pending' message lists.
 */
static inline void __mpc_comm_ptp_message_list_init( mpc_comm_ptp_message_lists_t *lists )
{
	__mpc_comm_ptp_list_incoming_init( &( lists->incomming_send ) );
	__mpc_comm_ptp_list_incoming_init( &( lists->incomming_recv ) );
	mpc_common_spinlock_init(&lists->pending_lock, 0);
	__mpc_comm_ptp_list_pending_init( &( lists->pending_send ) );
	__mpc_comm_ptp_list_pending_init( &( lists->pending_recv ) );
}

/*
 * Merge 'incoming' send and receive lists to 'pending' send and receive lists
 */
static inline void __mpc_comm_ptp_message_list_merge_pending( mpc_comm_ptp_message_lists_t *lists )
{
	if ( lists->incomming_send.list != NULL )
	{
		if ( mpc_common_spinlock_trylock( &( lists->incomming_send.lock ) ) == 0 )
		{
			DL_CONCAT( lists->pending_send.list,
			           ( mpc_lowcomm_msg_list_t * ) lists->incomming_send.list );
			lists->incomming_send.list = NULL;
			mpc_common_spinlock_unlock( &( lists->incomming_send.lock ) );
		}
	}

	if ( lists->incomming_recv.list != NULL )
	{
		if ( mpc_common_spinlock_trylock( &( lists->incomming_recv.lock ) ) == 0 )
		{
			DL_CONCAT( lists->pending_recv.list,
			           ( mpc_lowcomm_msg_list_t * ) lists->incomming_recv.list );
			lists->incomming_recv.list = NULL;
			mpc_common_spinlock_unlock( &( lists->incomming_recv.lock ) );
		}
	}

	/* Change the flag. New messages have been inserted to the 'pending' lists */
	lists->changed = 1;
}

static inline void __mpc_comm_ptp_message_list_lock_pending( mpc_comm_ptp_message_lists_t *lists )
{
	mpc_common_spinlock_lock_yield( &( lists->pending_lock ) );
}

static inline int __mpc_comm_ptp_message_list_trylock_pending( mpc_comm_ptp_message_lists_t *lists )
{
	return mpc_common_spinlock_trylock( &( lists->pending_lock ) );
}

static inline void __mpc_comm_ptp_message_list_unlock_pending( mpc_comm_ptp_message_lists_t *lists )
{
	mpc_common_spinlock_unlock( &( lists->pending_lock ) );
}

/*
 * Add message into the 'pending' recv list
 */
static inline void __mpc_comm_ptp_message_list_add_pending( mpc_comm_ptp_t *tmp,
        mpc_lowcomm_ptp_message_t *msg )
{
	msg->tail.internal_ptp = tmp;
	assert( tmp != NULL );
	OPA_incr_int( &tmp->pending_nb );
}

/*
 * Add message into the 'incoming' recv list
 */
static inline void __mpc_comm_ptp_message_list_add_incoming_recv( mpc_comm_ptp_t *tmp,
        mpc_lowcomm_ptp_message_t *msg )
{
	msg->tail.distant_list.msg = msg;
	mpc_common_spinlock_lock( &( tmp->lists.incomming_recv.lock ) );
	DL_APPEND( tmp->lists.incomming_recv.list, &( msg->tail.distant_list ) );
	mpc_common_spinlock_unlock( &( tmp->lists.incomming_recv.lock ) );
}
/*
 * Add message into the 'incoming' send list
 */
static inline void __mpc_comm_ptp_message_list_add_incoming_send( mpc_comm_ptp_t *tmp,
        mpc_lowcomm_ptp_message_t *msg )
{
	msg->tail.distant_list.msg = msg;
	mpc_common_spinlock_lock( &( tmp->lists.incomming_send.lock ) );
	DL_APPEND( tmp->lists.incomming_send.list, &( msg->tail.distant_list ) );
	mpc_common_spinlock_unlock( &( tmp->lists.incomming_send.lock ) );
}

/************************************************************************/
/*Data structure accessors                                              */
/************************************************************************/

static mpc_comm_ptp_t ** *sctk_ptp_array = NULL;
static int sctk_ptp_array_start = 0;
static int sctk_ptp_array_end = 0;

/*
 * Find an internal ptp according the a key (this key represents the task
 * number)
 */

static inline struct mpc_comm_ptp_s *__mpc_comm_ptp_array_get_key( mpc_comm_dest_key_t *key )
{
	int __dest__id = key->rank - sctk_ptp_array_start;

	if ( ( sctk_ptp_array != NULL ) && ( __dest__id >= 0 ) &&
	     ( __dest__id <= sctk_ptp_array_end - sctk_ptp_array_start ) )
	{
		return sctk_ptp_array[__dest__id][key->comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER];
	}

	return NULL;
}

static inline struct mpc_comm_ptp_s *__mpc_comm_ptp_array_get( mpc_lowcomm_communicator_t comm, int rank )
{
	mpc_comm_dest_key_t key;
	_mpc_comm_dest_key_init( &key, comm, rank );
	return __mpc_comm_ptp_array_get_key( &key );
}

struct mpc_comm_ptp_s *_mpc_comm_ptp_array_get( mpc_lowcomm_communicator_t comm, int rank )
{
	return __mpc_comm_ptp_array_get( comm, rank );
}

sctk_reorder_list_t *_mpc_comm_ptp_array_get_reorder( mpc_lowcomm_communicator_t communicator, int rank )
{
	struct mpc_comm_ptp_s *internal_ptp = __mpc_comm_ptp_array_get( communicator, rank );
	sctk_assert( internal_ptp );
	return &( internal_ptp->reorder );
}

/*
 * Insert a new entry to the PTP table. The function checks if the entry is
 * already prensent
 * and fail in this case
 */
static inline void __mpc_comm_ptp_array_insert( mpc_comm_ptp_t *tmp )
{
	static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
	static volatile int done = 0;
	mpc_common_spinlock_lock( &lock );

	/* Only one task allocates the structures */
	if ( done == 0 )
	{
		sctk_ptp_array_start = sctk_get_first_task_local( SCTK_COMM_WORLD );
		sctk_ptp_array_end = sctk_get_last_task_local( SCTK_COMM_WORLD );
		sctk_ptp_array = sctk_malloc( ( sctk_ptp_array_end - sctk_ptp_array_start + 1 ) * sizeof( mpc_comm_ptp_t ** ) );
		memset( sctk_ptp_array, 0, ( sctk_ptp_array_end - sctk_ptp_array_start + 1 ) * sizeof( mpc_comm_ptp_t ** ) );
		done = 1;
	}

	assume( tmp->key.rank >= sctk_ptp_array_start );
	assume( tmp->key.rank <= sctk_ptp_array_end );

	if ( sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] == NULL )
	{
		sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] = sctk_malloc( SCTK_PARALLEL_COMM_QUEUES_NUMBER * sizeof( mpc_comm_ptp_t * ) );
		memset( sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start], 0, SCTK_PARALLEL_COMM_QUEUES_NUMBER * sizeof( mpc_comm_ptp_t * ) );
	}

	assume( sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start][tmp->key.comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER] == NULL );
	sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] [tmp->key.comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER] = tmp;
	mpc_common_spinlock_unlock( &lock );
}

/*
 * Return if the task is on the node or not
 */

struct is_net_previous_answer
{
	int ask;
	int answer;
};

static __thread struct is_net_previous_answer __sctk_is_net = {-1, 0};

static inline int _mpc_comm_is_remote_rank( int dest )
{
#ifdef MPC_IN_PROCESS_MODE

	if ( dest != mpc_common_get_task_rank() )
	{
		return 1;
	}

	return 0;
#endif

	if ( __sctk_is_net.ask == dest )
	{
		return __sctk_is_net.answer;
	}

	mpc_comm_ptp_t *tmp = __mpc_comm_ptp_array_get( SCTK_COMM_WORLD, dest );
	__sctk_is_net.ask = dest;
	__sctk_is_net.answer = ( tmp == NULL );
	return ( tmp == NULL );
}

/* This is the exported version */
int mpc_lowcomm_is_remote_rank( int dest )
{
	return _mpc_comm_is_remote_rank( dest );
}

/********************************************************************/
/*Task engine                                                       */
/********************************************************************/

/* Messages in the '__mpc_ptp_task_list' have already been
 * matched and are wainting to be copied */
mpc_lowcomm_ptp_message_content_to_copy_t **__mpc_ptp_task_list = NULL;
mpc_common_spinlock_t *__mpc_ptp_task_lock = 0;
int sctk_ptp_tasks_count = 0;

static short __mpc_comm_ptp_task_init_done = 0;
mpc_common_spinlock_t __mpc_comm_ptp_task_init_lock = SCTK_SPINLOCK_INITIALIZER;

#define PTP_MAX_TASK_LISTS 100

static inline void __mpc_comm_ptp_task_init()
{
	mpc_common_spinlock_lock( &__mpc_comm_ptp_task_init_lock );

	if ( __mpc_comm_ptp_task_init_done )
	{
		mpc_common_spinlock_unlock( &__mpc_comm_ptp_task_init_lock );
		return;
	}

	if ( mpc_common_get_task_count() < PTP_MAX_TASK_LISTS )
	{
		sctk_ptp_tasks_count = mpc_common_get_task_count();
	}
	else
	{
		sctk_ptp_tasks_count = PTP_MAX_TASK_LISTS;
	}

	__mpc_ptp_task_list = sctk_malloc( sizeof( mpc_lowcomm_ptp_message_content_to_copy_t * ) * sctk_ptp_tasks_count );
	assume( __mpc_ptp_task_list );
	__mpc_ptp_task_lock = sctk_malloc( sizeof( mpc_common_spinlock_t * ) * sctk_ptp_tasks_count );
	assume( __mpc_ptp_task_lock );
	int i;

	for ( i = 0; i < sctk_ptp_tasks_count; i++ )
	{
		__mpc_ptp_task_list[i] = NULL;
		mpc_common_spinlock_init(&__mpc_ptp_task_lock[i], 0 );
	}

	__mpc_comm_ptp_task_init_done = 1;
	mpc_common_spinlock_unlock( &__mpc_comm_ptp_task_init_lock );
}

/*
 * Function which loop on t'he ptp_task_list' in order to copy
 * pending messages which have been already matched.
 * Only called if the task engine is enabled
 */
static inline int ___mpc_comm_ptp_task_perform( int key, int depth )
{
	int nb_messages_copied = 0; /* Number of messages processed */
	int target_list = key % sctk_ptp_tasks_count;

	/* Each element of this list has already been matched */
	while ( __mpc_ptp_task_list[target_list] != NULL )
	{
		mpc_lowcomm_ptp_message_content_to_copy_t *tmp = NULL;

		if ( mpc_common_spinlock_trylock( &( __mpc_ptp_task_lock[target_list] ) ) == 0 )
		{
			tmp = __mpc_ptp_task_list[target_list];

			if ( tmp != NULL )
			{
				/* Message found, we remove it from the list */
				DL_DELETE( __mpc_ptp_task_list[target_list], tmp );
			}

			mpc_common_spinlock_unlock( &( __mpc_ptp_task_lock[target_list] ) );
		}

		if ( tmp != NULL )
		{
			assume( tmp->msg_send->tail.message_copy );
			/* Call the copy function to copy the message from the network buffer to
			* the matching user buffer */
			tmp->msg_send->tail.message_copy( tmp );
			nb_messages_copied++;
		}
		else
		{
			if ( depth )
			{
				return nb_messages_copied;
			}
		}
	}

	if ( ( nb_messages_copied == 0 ) && ( depth < 3 ) )
	{
		___mpc_comm_ptp_task_perform( key + 1, depth + 1 );
	}

	return nb_messages_copied;
}

static inline int _mpc_comm_ptp_task_perform( int key )
{
	return ___mpc_comm_ptp_task_perform( key, 0 );
}

/*
 * Insert a message to copy. The message to insert has already been matched.
 */
static inline void _mpc_comm_ptp_task_insert( mpc_lowcomm_ptp_message_content_to_copy_t *tmp,
        mpc_comm_ptp_t *pair )
{
	int key = pair->key.rank % PTP_MAX_TASK_LISTS;
	mpc_common_spinlock_lock( &( __mpc_ptp_task_lock[key] ) );
	DL_APPEND( __mpc_ptp_task_list[key], tmp );
	mpc_common_spinlock_unlock( &( __mpc_ptp_task_lock[key] ) );
}

/*
 * Insert the message to copy into the pending list
 */
static inline void _mpc_comm_ptp_copy_task_insert( mpc_lowcomm_msg_list_t *ptr_recv,
        mpc_lowcomm_msg_list_t *ptr_send,
        mpc_comm_ptp_t *pair )
{
	mpc_lowcomm_ptp_message_content_to_copy_t *tmp;
	SCTK_PROFIL_START( MPC_Copy_message );
	tmp = &( ptr_recv->msg->tail.copy_list );
	tmp->msg_send = ptr_send->msg;
	tmp->msg_recv = ptr_recv->msg;

	/* If the message is small just copy */
	if ( tmp->msg_send->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS )
	{
		if ( tmp->msg_send->tail.message.contiguous.size <= 64 )
		{
			assume( tmp->msg_send->tail.message_copy );
			tmp->msg_send->tail.message_copy( tmp );
		}
		else
		{
			_mpc_comm_ptp_task_insert( tmp, pair );
		}
	}
	else
	{
		_mpc_comm_ptp_task_insert( tmp, pair );
	}

	SCTK_PROFIL_END( MPC_Copy_message );
}

/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

/*
 * Mark a message received as DONE and call the 'free' function associated
 * to the message
 */
int ___mpi_windows_in_use = 0;

static inline int __MPC_MPI_windows_in_use()
{
        return ___mpi_windows_in_use;
}

void mpc_lowcomm_rdma_MPI_windows_in_use(void)
{
        ___mpi_windows_in_use |= 1;
}

static int (*___notify_request_completion_trampoline)( mpc_lowcomm_request_t *req ) = NULL;

void mpc_lowcomm_set_request_completion_trampoline( int trampoline( mpc_lowcomm_request_t *req) )
{
        ___notify_request_completion_trampoline = trampoline;
}

int mpc_lowcomm_notify_request_completion(mpc_lowcomm_request_t *req)
{
        if( ___notify_request_completion_trampoline )
        {
                return (___notify_request_completion_trampoline)(req);
        }

        return 0;
}

void mpc_lowcomm_ptp_message_complete_and_free( mpc_lowcomm_ptp_message_t *msg )
{
	mpc_lowcomm_request_t *req = SCTK_MSG_REQUEST( msg );

	if ( req )
	{
#ifdef MPC_MPI

		if ( __MPC_MPI_windows_in_use() )
		{
			mpc_lowcomm_notify_request_completion( req );
		}

#endif

		if ( req->ptr_to_pin_ctx )
		{
			sctk_rail_pin_ctx_release( ( sctk_rail_pin_ctx_t * ) req->ptr_to_pin_ctx );
			sctk_free( req->ptr_to_pin_ctx );
			req->ptr_to_pin_ctx = NULL;
		}
	}

	void ( *free_memory )( void * );
	free_memory = msg->tail.free_memory;
	assume( free_memory );

	if ( msg->tail.internal_ptp )
	{
		assume( msg->tail.internal_ptp );
		OPA_decr_int( &msg->tail.internal_ptp->pending_nb );
	}

	if ( msg->tail.buffer_async )
	{
		mpc_buffered_msg_t *buffer_async = msg->tail.buffer_async;
		buffer_async->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
	}

	if ( SCTK_MSG_COMPLETION_FLAG( msg ) )
	{
		*( SCTK_MSG_COMPLETION_FLAG( msg ) ) = MPC_LOWCOMM_MESSAGE_DONE;
	}

	( free_memory )( msg );
}

void _mpc_comm_ptp_message_commit_request( mpc_lowcomm_ptp_message_t *send,
        mpc_lowcomm_ptp_message_t *recv )
{
	/* If a recv request is available */
	if ( recv->tail.request )
	{
		/* Update the request with the source, message tag and message size */
		/* Make sure to do the communicator translation here
		   as messages are sent with their comm_world rank this is required
		   when matching ANY_SOURCE messages in order to fill accordingly
		   the MPI_Status object from the request data */
		recv->tail.request->header.source_task = mpc_lowcomm_communicator_rank( SCTK_MSG_COMMUNICATOR( send ), SCTK_MSG_SRC_TASK( send ) );
		recv->tail.request->header.source = SCTK_MSG_SRC_PROCESS( send );
		recv->tail.request->header.message_tag = SCTK_MSG_TAG( send );
		recv->tail.request->header.msg_size = SCTK_MSG_SIZE( send );
		recv->tail.request->msg = NULL;
		recv->tail.request->truncated = ( SCTK_MSG_SIZE( send ) > SCTK_MSG_SIZE( recv ) );
	}

	/* If a send request is available */
	if ( send->tail.request )
	{
		send->tail.request->msg = NULL;
	}

#ifdef SCTK_USE_CHECKSUM
	/* Verify the checksum of the received message */
	sctk_checksum_verify( send, recv );
#endif
	/* Complete messages: mark messages as done and mark them as DONE */
	mpc_lowcomm_ptp_message_complete_and_free( send );
	mpc_lowcomm_ptp_message_complete_and_free( recv );
}

inline void mpc_lowcomm_ptp_message_copy( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;
	send = tmp->msg_send;
	recv = tmp->msg_recv;
	assume( send->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS );

	switch ( recv->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_CONTIGUOUS - MPC_LOWCOMM_MESSAGE_CONTIGUOUS" );
			size_t size;
			size = mpc_common_min( send->tail.message.contiguous.size,
			                       recv->tail.message.contiguous.size );
			memcpy( recv->tail.message.contiguous.addr,
			        send->tail.message.contiguous.addr, size );
			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_CONTIGUOUS - MPC_LOWCOMM_MESSAGE_PACK size %d",
			              send->tail.message.contiguous.size );
			size_t i;
			ssize_t j;
			size_t size;
			size_t total = 0;
			size_t recv_size = 0;

			if ( send->tail.message.contiguous.size > 0 )
			{
				for ( i = 0; i < recv->tail.message.pack.count; i++ )
				{
					for ( j = 0; j < recv->tail.message.pack.list.std[i].count; j++ )
					{
						size = ( recv->tail.message.pack.list.std[i].ends[j] -
						         recv->tail.message.pack.list.std[i].begins[j] + 1 ) *
						       recv->tail.message.pack.list.std[i].elem_size;
						recv_size += size;
					}
				}

				/* MPI 1.3 : The length of the received message must be less than or equal
				* to the length of the receive buffer */
				assume( send->tail.message.contiguous.size <= recv_size );
				sctk_nodebug( "contiguous size : %d, PACK SIZE : %d",
				              send->tail.message.contiguous.size, recv_size );
				char skip = 0;

				for ( i = 0; ( ( i < recv->tail.message.pack.count ) && !skip ); i++ )
				{
					for ( j = 0; ( ( j < recv->tail.message.pack.list.std[i].count ) && !skip );
					      j++ )
					{
						size = ( recv->tail.message.pack.list.std[i].ends[j] -
						         recv->tail.message.pack.list.std[i].begins[j] + 1 ) *
						       recv->tail.message.pack.list.std[i].elem_size;

						if ( total + size > send->tail.message.contiguous.size )
						{
							skip = 1;
							size = send->tail.message.contiguous.size - total;
						}

						memcpy( ( recv->tail.message.pack.list.std[i].addr ) +
						        recv->tail.message.pack.list.std[i].begins[j] *
						        recv->tail.message.pack.list.std[i].elem_size,
						        send->tail.message.contiguous.addr, size );
						total += size;
						send->tail.message.contiguous.addr += size;
						assume( total <= send->tail.message.contiguous.size );
					}
				}
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_CONTIGUOUS - MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE size %d",
			              send->tail.message.contiguous.size );
			size_t i;
			ssize_t j;
			size_t size;
			size_t total = 0;
			size_t recv_size = 0;

			if ( send->tail.message.contiguous.size > 0 )
			{
				for ( i = 0; i < recv->tail.message.pack.count; i++ )
				{
					for ( j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++ )
					{
						size = ( recv->tail.message.pack.list.absolute[i].ends[j] -
						         recv->tail.message.pack.list.absolute[i].begins[j] + 1 ) *
						       recv->tail.message.pack.list.absolute[i].elem_size;
						recv_size += size;
					}
				}

				/* MPI 1.3 : The length of the received message must be less than or equal
				   to the length of the receive buffer */
				assume( send->tail.message.contiguous.size <= recv_size );
				sctk_nodebug( "contiguous size : %d, ABSOLUTE SIZE : %d",
				              send->tail.message.contiguous.size, recv_size );
				char skip = 0;

				for ( i = 0; ( ( i < recv->tail.message.pack.count ) && !skip ); i++ )
				{
					for ( j = 0;
					      ( ( j < recv->tail.message.pack.list.absolute[i].count ) && !skip );
					      j++ )
					{
						size = ( recv->tail.message.pack.list.absolute[i].ends[j] -
						         recv->tail.message.pack.list.absolute[i].begins[j] + 1 ) *
						       recv->tail.message.pack.list.absolute[i].elem_size;

						if ( total + size > send->tail.message.contiguous.size )
						{
							skip = 1;
							size = send->tail.message.contiguous.size - total;
						}

						memcpy( ( recv->tail.message.pack.list.absolute[i].addr ) +
						        recv->tail.message.pack.list.absolute[i].begins[j] *
						        recv->tail.message.pack.list.absolute[i].elem_size,
						        send->tail.message.contiguous.addr, size );
						total += size;
						send->tail.message.contiguous.addr += size;
						assume( total <= send->tail.message.contiguous.size );
					}
				}
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		default:
			not_reachable();
	}
}

static inline void __mpc_comm_copy_buffer_pack_pack( unsigned long *restrict in_begins,
        unsigned long *restrict in_ends, size_t in_sizes,
        void *restrict in_adress, size_t in_elem_size,
        unsigned long *restrict out_begins,
        unsigned long *restrict out_ends,
        size_t out_sizes, void *restrict out_adress,
        size_t out_elem_size )
{
	unsigned long tmp_begin[1];
	unsigned long tmp_end[1];

	if ( ( in_begins == NULL ) && ( out_begins == NULL ) )
	{
		sctk_nodebug( "__mpc_comm_copy_buffer_pack_pack no mpc_pack" );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
		memcpy( out_adress, in_adress, in_sizes );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		sctk_nodebug( "__mpc_comm_copy_buffer_pack_pack mpc_pack" );

		if ( in_begins == NULL )
		{
			in_begins = tmp_begin;
			in_begins[0] = 0;
			in_ends = tmp_end;
			in_ends[0] = in_sizes - 1;
			in_elem_size = 1;
			in_sizes = 1;
		}

		if ( out_begins == NULL )
		{
			out_begins = tmp_begin;
			out_begins[0] = 0;
			out_ends = tmp_end;
			out_ends[0] = out_sizes - 1;
			out_elem_size = 1;
			out_sizes = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for ( i = 0; i < out_sizes; i++ )
		{
			for ( j = out_begins[i] * out_elem_size;
			      j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if ( in_j > in_ends[in_i] * in_elem_size )
				{
					in_i++;

					if ( in_i >= in_sizes )
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				    mpc_common_min( ( out_ends[i] * out_elem_size - j + out_elem_size ),
				                    ( in_ends[in_i] * in_elem_size - in_j + in_elem_size ) );
				memcpy( &( ( ( char * ) out_adress )[j] ), &( ( ( char * ) in_adress )[in_j] ),
				        max_length );
				sctk_nodebug( "Copy out[%d-%d]%s == in[%d-%d]%s", j, j + max_length,
				              &( ( ( char * ) out_adress )[j] ), in_j, in_j + max_length,
				              &( ( ( char * ) in_adress )[in_j] ) );
				j += max_length;
				in_j += max_length;
			}
		}
	}
}


static inline void __mpc_comm_copy_buffer_absolute_pack( long *restrict in_begins,
        long *restrict in_ends, size_t in_sizes,
        void *restrict in_adress, size_t in_elem_size,
        unsigned long *restrict out_begins,
        unsigned long *restrict out_ends, size_t out_sizes,
        void *restrict out_adress, size_t out_elem_size )
{
	unsigned long tmp_begin[1];
	unsigned long tmp_end[1];
	long tmp_begin_abs[1];
	long tmp_end_abs[1];

	if ( ( in_begins == NULL ) && ( out_begins == NULL ) )
	{
		sctk_nodebug( "__mpc_comm_copy_buffer_pack_pack no mpc_pack" );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
		memcpy( out_adress, in_adress, in_sizes );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		sctk_nodebug( "__mpc_comm_copy_buffer_pack_pack mpc_pack" );

		if ( in_begins == NULL )
		{
			in_begins = tmp_begin_abs;
			in_begins[0] = 0;
			in_ends = tmp_end_abs;
			in_ends[0] = in_sizes - 1;
			in_elem_size = 1;
			in_sizes = 1;
		}

		if ( out_begins == NULL )
		{
			out_begins = tmp_begin;
			out_begins[0] = 0;
			out_ends = tmp_end;
			out_ends[0] = out_sizes - 1;
			out_elem_size = 1;
			out_sizes = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for ( i = 0; i < out_sizes; i++ )
		{
			for ( j = out_begins[i] * out_elem_size;
			      j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if ( in_j > in_ends[in_i] * in_elem_size )
				{
					in_i++;

					if ( in_i >= in_sizes )
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				    mpc_common_min( ( out_ends[i] * out_elem_size - j + out_elem_size ),
				                    ( in_ends[in_i] * in_elem_size - in_j + in_elem_size ) );
				memcpy( &( ( ( char * ) out_adress )[j] ), &( ( ( char * ) in_adress )[in_j] ),
				        max_length );
				sctk_nodebug( "Copy out[%d-%d]%s == in[%d-%d]%s", j, j + max_length,
				              &( ( ( char * ) out_adress )[j] ), in_j, in_j + max_length,
				              &( ( ( char * ) in_adress )[in_j] ) );
				j += max_length;
				in_j += max_length;
			}
		}
	}
}

/*
 * Function without description
 */
static inline void __mpc_comm_copy_buffer_pack_absolute(
    unsigned long *restrict in_begins,
    unsigned long *restrict in_ends, size_t in_sizes,
    void *restrict in_adress, size_t in_elem_size,
    long *restrict out_begins,
    long *restrict out_ends, size_t out_sizes,
    void *restrict out_adress, size_t out_elem_size )
{
	unsigned long tmp_begin[1];
	unsigned long tmp_end[1];
	long tmp_begin_abs[1];
	long tmp_end_abs[1];

	if ( ( in_begins == NULL ) && ( out_begins == NULL ) )
	{
		sctk_nodebug( "__mpc_comm_copy_buffer_absolute_absolute no mpc_pack" );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
		memcpy( out_adress, in_adress, in_sizes );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		sctk_nodebug( "__mpc_comm_copy_buffer_absolute_absolute mpc_pack" );

		if ( in_begins == NULL )
		{
			in_begins = tmp_begin;
			in_begins[0] = 0;
			in_ends = tmp_end;
			in_ends[0] = in_sizes - 1;
			in_elem_size = 1;
			in_sizes = 1;
		}

		if ( out_begins == NULL )
		{
			out_begins = tmp_begin_abs;
			out_begins[0] = 0;
			out_ends = tmp_end_abs;
			out_ends[0] = out_sizes - 1;
			out_elem_size = 1;
			out_sizes = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for ( i = 0; i < out_sizes; i++ )
		{
			for ( j = out_begins[i] * out_elem_size;
			      j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if ( in_j > in_ends[in_i] * in_elem_size )
				{
					in_i++;

					if ( in_i >= in_sizes )
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				    mpc_common_min( ( out_ends[i] * out_elem_size - j + out_elem_size ),
				                    ( in_ends[in_i] * in_elem_size - in_j + in_elem_size ) );
				sctk_nodebug( "Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j, j + max_length,
				              &( ( ( char * ) out_adress )[j] ), in_j, in_j + max_length,
				              &( ( ( char * ) in_adress )[in_j] ) );
				memcpy( &( ( ( char * ) out_adress )[j] ), &( ( ( char * ) in_adress )[in_j] ),
				        max_length );
				sctk_nodebug( "Copy out[%d-%d]%d == in[%d-%d]%d", j, j + max_length,
				              ( ( ( char * ) out_adress )[j] ), in_j, in_j + max_length,
				              ( ( ( char * ) in_adress )[in_j] ) );
				j += max_length;
				in_j += max_length;
			}
		}
	}
}

/*
 * Function without description
 */
static inline void __mpc_comm_copy_buffer_absolute_absolute(
    long *restrict in_begins,
    long *restrict in_ends, size_t in_sizes,
    void *restrict in_adress, size_t in_elem_size,
    long *restrict out_begins,
    long *restrict out_ends, size_t out_sizes,
    void *restrict out_adress, size_t out_elem_size )
{
	long tmp_begin[1];
	long tmp_end[1];

	if ( ( in_begins == NULL ) && ( out_begins == NULL ) )
	{
		sctk_nodebug( "__mpc_comm_copy_buffer_absolute_absolute no mpc_pack" );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
		memcpy( out_adress, in_adress, in_sizes );
		sctk_nodebug( "%s == %s", out_adress, in_adress );
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		sctk_nodebug( "__mpc_comm_copy_buffer_absolute_absolute mpc_pack %p", in_begins );

		/* Empty message */
		if ( !in_sizes )
		{
			return;
		}

		if ( in_begins == NULL )
		{
			in_begins = tmp_begin;
			in_begins[0] = 0;
			in_ends = tmp_end;
			in_ends[0] = in_sizes - 1;
			in_elem_size = 1;
			in_sizes = 1;
		}

		if ( out_begins == NULL )
		{
			out_begins = tmp_begin;
			out_begins[0] = 0;
			out_ends = tmp_end;
			out_ends[0] = out_sizes - 1;
			out_elem_size = 1;
			out_sizes = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for ( i = 0; i < out_sizes; i++ )
		{
			for ( j = out_begins[i] * out_elem_size;
			      j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if ( in_j > in_ends[in_i] * in_elem_size )
				{
					in_i++;

					if ( in_i >= in_sizes )
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				    mpc_common_min( ( out_ends[i] * out_elem_size - j + out_elem_size ),
				                    ( in_ends[in_i] * in_elem_size - in_j + in_elem_size ) );
				sctk_nodebug( "Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j, j + max_length,
				              &( ( ( char * ) out_adress )[j] ), in_j, in_j + max_length,
				              &( ( ( char * ) in_adress )[in_j] ) );
				memcpy( &( ( ( char * ) out_adress )[j] ), &( ( ( char * ) in_adress )[in_j] ),
				        max_length );
				sctk_nodebug( "Copy out[%d-%d]%d == in[%d-%d]%d", j, j + max_length,
				              ( ( ( char * ) out_adress )[j] ), in_j, in_j + max_length,
				              ( ( ( char * ) in_adress )[in_j] ) );
				j += max_length;
				in_j += max_length;
			}
		}
	}
}

/*
 * Function without description
 */
inline void mpc_lowcomm_ptp_message_copy_pack( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;
	send = tmp->msg_send;
	recv = tmp->msg_recv;
	assume( send->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK );

	switch ( recv->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_PACK - MPC_LOWCOMM_MESSAGE_PACK" );
			size_t i;

			for ( i = 0; i < send->tail.message.pack.count; i++ )
			{
				__mpc_comm_copy_buffer_pack_pack( send->tail.message.pack.list.std[i].begins,
				                                  send->tail.message.pack.list.std[i].ends,
				                                  send->tail.message.pack.list.std[i].count,
				                                  send->tail.message.pack.list.std[i].addr,
				                                  send->tail.message.pack.list.std[i].elem_size,
				                                  recv->tail.message.pack.list.std[i].begins,
				                                  recv->tail.message.pack.list.std[i].ends,
				                                  recv->tail.message.pack.list.std[i].count,
				                                  recv->tail.message.pack.list.std[i].addr,
				                                  recv->tail.message.pack.list.std[i].elem_size );
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_PACK - MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE" );
			size_t i;

			for ( i = 0; i < send->tail.message.pack.count; i++ )
			{
				__mpc_comm_copy_buffer_pack_absolute(
				    send->tail.message.pack.list.std[i].begins,
				    send->tail.message.pack.list.std[i].ends,
				    send->tail.message.pack.list.std[i].count,
				    send->tail.message.pack.list.std[i].addr,
				    send->tail.message.pack.list.std[i].elem_size,
				    recv->tail.message.pack.list.absolute[i].begins,
				    recv->tail.message.pack.list.absolute[i].ends,
				    recv->tail.message.pack.list.absolute[i].count,
				    recv->tail.message.pack.list.absolute[i].addr,
				    recv->tail.message.pack.list.absolute[i].elem_size );
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_PACK - MPC_LOWCOMM_MESSAGE_CONTIGUOUS" );
			size_t i;
			ssize_t j;
			size_t size;
			char *body;
			body = recv->tail.message.contiguous.addr;
			sctk_nodebug( "COUNT %lu", send->tail.message.pack.count );

			for ( i = 0; i < send->tail.message.pack.count; i++ )
			{
				for ( j = 0; j < send->tail.message.pack.list.std[i].count; j++ )
				{
					size = ( send->tail.message.pack.list.std[i].ends[j] -
					         send->tail.message.pack.list.std[i].begins[j] + 1 ) *
					       send->tail.message.pack.list.std[i].elem_size;
					memcpy( body, ( ( char * ) ( send->tail.message.pack.list.std[i].addr ) ) +
					        send->tail.message.pack.list.std[i].begins[j] *
					        send->tail.message.pack.list.std[i].elem_size,
					        size );
					body += size;
				}
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		default:
			not_reachable();
	}
}

/*
 * Function without description
 */
inline void mpc_lowcomm_ptp_message_copy_pack_absolute( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;
	send = tmp->msg_send;
	recv = tmp->msg_recv;
	assume( send->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE );

	switch ( recv->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE - MPC_LOWCOMM_MESSAGE_PACK" );
			size_t i;

			for ( i = 0; i < send->tail.message.pack.count; i++ )
			{
				__mpc_comm_copy_buffer_absolute_pack(
				    send->tail.message.pack.list.absolute[i].begins,
				    send->tail.message.pack.list.absolute[i].ends,
				    send->tail.message.pack.list.absolute[i].count,
				    send->tail.message.pack.list.absolute[i].addr,
				    send->tail.message.pack.list.absolute[i].elem_size,
				    recv->tail.message.pack.list.std[i].begins,
				    recv->tail.message.pack.list.std[i].ends,
				    recv->tail.message.pack.list.std[i].count,
				    recv->tail.message.pack.list.std[i].addr,
				    recv->tail.message.pack.list.std[i].elem_size );
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			sctk_nodebug(
			    "MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE - MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE count == %d",
			    send->tail.message.pack.count );
			size_t i;

			for ( i = 0; i < send->tail.message.pack.count; i++ )
			{
				__mpc_comm_copy_buffer_absolute_absolute(
				    send->tail.message.pack.list.absolute[i].begins,
				    send->tail.message.pack.list.absolute[i].ends,
				    send->tail.message.pack.list.absolute[i].count,
				    send->tail.message.pack.list.absolute[i].addr,
				    send->tail.message.pack.list.absolute[i].elem_size,
				    recv->tail.message.pack.list.absolute[i].begins,
				    recv->tail.message.pack.list.absolute[i].ends,
				    recv->tail.message.pack.list.absolute[i].count,
				    recv->tail.message.pack.list.absolute[i].addr,
				    recv->tail.message.pack.list.absolute[i].elem_size );
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			sctk_nodebug( "MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE - MPC_LOWCOMM_MESSAGE_CONTIGUOUS" );
			size_t i;
			ssize_t j;
			size_t size;
			size_t send_size = 0;
			char *body;
			body = recv->tail.message.contiguous.addr;
			sctk_nodebug( "COUNT %lu", send->tail.message.pack.count );

			for ( i = 0; i < send->tail.message.pack.count; i++ )
			{
				for ( j = 0; j < send->tail.message.pack.list.absolute[i].count; j++ )
				{
					size = ( send->tail.message.pack.list.absolute[i].ends[j] -
					         send->tail.message.pack.list.absolute[i].begins[j] + 1 ) *
					       send->tail.message.pack.list.absolute[i].elem_size;
					send_size += size;
				}
			}

			sctk_nodebug( "msg_size = %d, send_size = %d, recv_size = %d",
			              SCTK_MSG_SIZE( send ), send_size,
			              recv->tail.message.contiguous.size );
			/* MPI 1.3 : The length of the received message must be less than or equal
			* to the length of the receive buffer */
			assume( send_size <= recv->tail.message.contiguous.size );

			for ( i = 0; ( i < send->tail.message.pack.count ); i++ )
			{
				for ( j = 0; ( j < send->tail.message.pack.list.absolute[i].count ); j++ )
				{
					size = ( send->tail.message.pack.list.absolute[i].ends[j] -
					         send->tail.message.pack.list.absolute[i].begins[j] + 1 ) *
					       send->tail.message.pack.list.absolute[i].elem_size;
					memcpy( body, ( ( char * ) ( send->tail.message.pack.list.absolute[i].addr ) ) +
					        send->tail.message.pack.list.absolute[i].begins[j] *
					        send->tail.message.pack.list.absolute[i].elem_size,
					        size );
					body += size;
				}
			}

			_mpc_comm_ptp_message_commit_request( send, recv );
			break;
		}

		default:
			not_reachable();
	}
}

/****************
 * BUFFERED PTP *
 ****************/

/* For message creation: a set of buffered ptp_message entries is allocated during init */
#define BUFFERED_PTP_MESSAGE_NUMBER 4096

__thread mpc_lowcomm_ptp_message_t *buffered_ptp_message = NULL;

static void __mpc_comm_buffered_ptp_init( void )
{
	/* Initialize the buffered_ptp_message list for the VP */
	if ( buffered_ptp_message == NULL )
	{
		mpc_lowcomm_ptp_message_t *tmp_message;
		tmp_message = sctk_malloc( sizeof( mpc_lowcomm_ptp_message_t ) *
		                           BUFFERED_PTP_MESSAGE_NUMBER );
		assume( tmp_message );
		int j;

		/* Loop on all buffers and create a list */
		for ( j = 0; j < BUFFERED_PTP_MESSAGE_NUMBER; ++j )
		{
			mpc_lowcomm_ptp_message_t *entry = &tmp_message[j];
			entry->from_buffered = 1;
			/* Add it to the list */
			LL_PREPEND( buffered_ptp_message, entry );
		}
	}
}

/*
 * Free the header. If the header is a from the buffered list, re-add it to
 * the list. Else, free the header.
 */
void __mpc_comm_free_header( void *tmp )
{
	mpc_lowcomm_ptp_message_t *header = ( mpc_lowcomm_ptp_message_t * )tmp;
	sctk_nodebug( "Free buffer %p buffered?%d", header, header->from_buffered );

	/* Header is from the buffered list */
	if ( header->from_buffered )
	{
		LL_PREPEND( buffered_ptp_message, header );
	}
	else
	{
		sctk_free( tmp );
	}
}

/*
 * Allocate a free header. Take it from the buffered list is an entry is free.
 * Else,
 * allocate a new header.
 */
static inline mpc_lowcomm_ptp_message_t *__mpc_comm_alloc_header()
{
	mpc_lowcomm_ptp_message_t *tmp = NULL;

	if ( buffered_ptp_message != NULL )
	{
		tmp = buffered_ptp_message;
		assume( tmp->from_buffered );
		LL_DELETE( buffered_ptp_message, buffered_ptp_message );
	}
	else
	{
		tmp = sctk_malloc( sizeof( mpc_lowcomm_ptp_message_t ) );
		/* Header must be freed after use */
		tmp->from_buffered = 0;
	}

	return tmp;
}

/********************************************************************/
/*INIT                                                              */
/********************************************************************/

/*
 * Init data structures used for task i. Called only once for each task
 */
void mpc_lowcomm_init_per_task( int rank )
{
	mpc_comm_ptp_t *tmp;
	int comm_id;
	__mpc_comm_ptp_task_init();
	__mpc_comm_buffered_ptp_init();

	for ( comm_id = 0; comm_id < SCTK_PARALLEL_COMM_QUEUES_NUMBER; comm_id++ )
	{
		tmp = sctk_malloc( sizeof( mpc_comm_ptp_t ) );
		memset( tmp, 0, sizeof( mpc_comm_ptp_t ) );
		_mpc_comm_dest_key_init( &tmp->key, comm_id, rank );
		/* Initialize reordering for the list */
		sctk_reorder_list_init( &tmp->reorder );
		/* Initialize the internal ptp lists */
		__mpc_comm_ptp_message_list_init( &( tmp->lists ) );
		/* And insert them */
		__mpc_comm_ptp_array_insert( tmp );
	}
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/
void __mpc_comm_ptp_message_pack_free( void * );

void mpc_lowcomm_ptp_message_header_clear( mpc_lowcomm_ptp_message_t *tmp,
        mpc_lowcomm_ptp_message_type_t msg_type, void ( *free_memory )( void * ),
        void ( *message_copy )( mpc_lowcomm_ptp_message_content_to_copy_t * ) )
{
	/* This memset costs 0.03 usecs in pingpong */
	memset( &tmp->tail, 0, sizeof( mpc_lowcomm_ptp_message_tail_t ) );
	tmp->tail.message_type = msg_type;

	switch ( tmp->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
			_mpc_comm_ptp_message_set_copy_and_free( tmp, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack );
			break;

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
			_mpc_comm_ptp_message_set_copy_and_free( tmp, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack_absolute );
			break;

		default:
			_mpc_comm_ptp_message_set_copy_and_free( tmp, free_memory, message_copy );
			break;
	}
}

mpc_lowcomm_ptp_message_t *mpc_lowcomm_ptp_message_header_create( mpc_lowcomm_ptp_message_type_t msg_type )
{
	mpc_lowcomm_ptp_message_t *tmp;
	tmp = __mpc_comm_alloc_header();
	/* Store if the buffer has been buffered */
	const char from_buffered = tmp->from_buffered;
	/* The copy and free functions will be set after */
	mpc_lowcomm_ptp_message_header_clear( tmp, msg_type, __mpc_comm_free_header, mpc_lowcomm_ptp_message_copy );
	/* Restore it */
	tmp->from_buffered = from_buffered;
	return tmp;
}


void mpc_lowcomm_ptp_message_set_contiguous_addr( mpc_lowcomm_ptp_message_t *restrict msg,
        const void *restrict addr, const size_t size )
{
	assert( msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS );
	msg->tail.message.contiguous.size = size;
	msg->tail.message.contiguous.addr = (void *)addr;
}

static inline void __mpc_comm_fill_request( mpc_lowcomm_request_t *request,
        int completion,
        mpc_lowcomm_ptp_message_t *msg,
        int request_type )
{
	assume( msg != NULL );
	__mpc_comm_request_init( request, SCTK_MSG_COMMUNICATOR( msg ), request_type );
	request->msg = msg;
	request->header.source = SCTK_MSG_SRC_PROCESS( msg );
	request->header.destination = SCTK_MSG_DEST_PROCESS( msg );
	request->header.destination_task = SCTK_MSG_DEST_TASK( msg );
	request->header.source_task = SCTK_MSG_SRC_TASK( msg );
	request->header.message_tag = SCTK_MSG_TAG( msg );
	request->header.msg_size = SCTK_MSG_SIZE( msg );
	request->is_null = 0;
	request->completion_flag = completion;
	request->status_error = SCTK_SUCCESS;
	request->ptr_to_pin_ctx = NULL;
}

void mpc_lowcomm_ptp_message_header_init( mpc_lowcomm_ptp_message_t *msg,
        const int message_tag,
        const mpc_lowcomm_communicator_t communicator,
        const int source, const int destination,
        mpc_lowcomm_request_t *request, const size_t count,
        mpc_lowcomm_ptp_message_class_t message_class,
        mpc_lowcomm_datatype_t datatype,
        mpc_lowcomm_request_type_t request_type )
{
	msg->tail.request = request;
	/* Save message class */
	SCTK_MSG_SPECIFIC_CLASS_SET( msg, message_class );

	/* PROCESS SPECIFIC MESSAGES */
	if ( _mpc_comm_ptp_message_is_for_process( message_class ) )
	{
		/* Fill in Source and Dest Process Informations (NO conversion needed)
		*/
		SCTK_MSG_SRC_PROCESS_SET( msg, source );
		SCTK_MSG_DEST_PROCESS_SET( msg, destination );
		/* Message does not come from a task source */
		SCTK_MSG_SRC_TASK_SET( msg, mpc_common_get_task_rank() );
		/* In all cases such message goes to a process */
		SCTK_MSG_DEST_TASK_SET( msg, -1 );
	}
	else
	{
		int source_task = -1;
		int dest_task = -1;
		/* Fill in Source and Dest Process Informations (convert from task) */

		if ( ( communicator != SCTK_COMM_WORLD ) && sctk_is_inter_comm( communicator ) )
		{
			if ( request_type == REQUEST_RECV )
			{
				/* If this is a RECV make sure the translation is done on the source according to remote */
				source_task = sctk_get_remote_comm_world_rank( communicator, source );
				dest_task = sctk_get_comm_world_rank( communicator, destination );
			}
			else if ( request_type == REQUEST_SEND )
			{
				/* If this is a SEND make sure the translation is done on the dest according to remote */
				source_task = sctk_get_comm_world_rank( communicator, source );
				dest_task = sctk_get_remote_comm_world_rank( communicator, destination );
			}
		}
		else
		{
			/* If we are not using an inter-comm just translate to COMM_WORLD */
			if ( source != SCTK_ANY_SOURCE )
			{
				source_task = sctk_get_comm_world_rank( communicator, source );
			}
			else
			{
				source_task = SCTK_ANY_SOURCE;
			}

			dest_task = sctk_get_comm_world_rank( communicator, destination );
		}

		SCTK_MSG_SRC_TASK_SET( msg, source_task );

		if ( source_task != SCTK_ANY_SOURCE )
		{
			SCTK_MSG_SRC_PROCESS_SET( msg, sctk_get_process_rank_from_task_rank( SCTK_MSG_SRC_TASK( msg ) ) );
		}
		else
		{
			SCTK_MSG_SRC_PROCESS_SET( msg, SCTK_ANY_SOURCE );
		}

		SCTK_MSG_DEST_TASK_SET( msg, dest_task );
		SCTK_MSG_DEST_PROCESS_SET( msg, sctk_get_process_rank_from_task_rank( SCTK_MSG_DEST_TASK( msg ) ) );


		mpc_common_debug("[%d -> %d P(%d -> %d)]", source_task, dest_task, source, destination);

#if 0

		/* If source matters */
		if ( source != SCTK_ANY_SOURCE )
		{
			/* If the communicator used is the COMM_SELF */
			if ( communicator == SCTK_COMM_SELF )
			{
				/* The world destination is actually ourself :) */
				int world_src = mpc_common_get_task_rank();
				SCTK_MSG_SRC_TASK_SET(
				    msg, sctk_get_comm_world_rank( communicator, world_src ) );
			}
			else
			{
				/* Get value from previous resolution according to intercomm */
				SCTK_MSG_SRC_TASK_SET( msg, source_task );
			}
		}
		else
		{
			/* Otherwise source does not matter */
			SCTK_MSG_SRC_TASK_SET( msg, SCTK_ANY_SOURCE );
		}

		/* Fill in dest */

		/* If the communicator used is the COMM_SELF */
		if ( communicator == SCTK_COMM_SELF )
		{
			/* The world destination is actually ourself :) */
			int world_dest = mpc_common_get_task_rank();
			SCTK_MSG_DEST_TASK_SET(
			    msg, sctk_get_comm_world_rank( communicator, world_dest ) );
		}
		else
		{
			/* Get value from previous resolution according to intercomm */
			SCTK_MSG_DEST_TASK_SET( msg, dest_task );
		}

#endif
	}

	/* Fill in Message meta-data */
	SCTK_MSG_COMMUNICATOR_SET( msg, communicator );
	SCTK_MSG_DATATYPE_SET( msg, datatype );
	SCTK_MSG_TAG_SET( msg, message_tag );
	SCTK_MSG_SIZE_SET( msg, count );
	SCTK_MSG_USE_MESSAGE_NUMBERING_SET( msg, 1 );
	SCTK_MSG_MATCH_SET( msg, sctk_m_probe_matching_get() );

	/* A message can be sent with a NULL request (see the MPI standard) */
	if ( request )
	{
		__mpc_comm_fill_request( request, MPC_LOWCOMM_MESSAGE_PENDING, msg, request_type );
		SCTK_MSG_COMPLETION_FLAG_SET( msg, &( request->completion_flag ) );
	}
}

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/

#define SCTK_PACK_REALLOC_STEP 10

void __mpc_comm_ptp_message_pack_free( void *tmp )
{
	mpc_lowcomm_ptp_message_t *msg;
	msg = tmp;

	if ( msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE )
	{
		sctk_free( msg->tail.message.pack.list.absolute );
	}
	else
	{
		sctk_free( msg->tail.message.pack.list.std );
	}

	__mpc_comm_free_header( tmp );
}

void mpc_lowcomm_ptp_message_add_pack( mpc_lowcomm_ptp_message_t *msg, void *adr,
                                       const unsigned int nb_items,
                                       const size_t elem_size,
                                       unsigned long *begins,
                                       unsigned long *ends )
{
	int step;

	if ( msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_UNDEFINED )
	{
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_PACK;
		_mpc_comm_ptp_message_set_copy_and_free( msg, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack );
	}

	assume( msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK );

	if ( msg->tail.message.pack.count >= msg->tail.message.pack.max_count )
	{
		msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
		msg->tail.message.pack.list.std =
		    sctk_realloc( msg->tail.message.pack.list.std,
		                  msg->tail.message.pack.max_count *
		                  sizeof( mpc_lowcomm_ptp_message_pack_std_list_t ) );
	}

	step = msg->tail.message.pack.count;
	msg->tail.message.pack.list.std[step].count = nb_items;
	msg->tail.message.pack.list.std[step].begins = begins;
	msg->tail.message.pack.list.std[step].ends = ends;
	msg->tail.message.pack.list.std[step].addr = adr;
	msg->tail.message.pack.list.std[step].elem_size = elem_size;
	msg->tail.message.pack.count++;
}
void mpc_lowcomm_ptp_message_add_pack_absolute( mpc_lowcomm_ptp_message_t *msg,
        const void *adr, const unsigned int nb_items,
        const size_t elem_size,
        long *begins,
        long *ends )
{
	int step;

	if ( msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_UNDEFINED )
	{
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE;
		_mpc_comm_ptp_message_set_copy_and_free( msg, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack_absolute );
	}

	assume( msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE );

	if ( msg->tail.message.pack.count >= msg->tail.message.pack.max_count )
	{
		msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
		msg->tail.message.pack.list.absolute =
		    sctk_realloc( msg->tail.message.pack.list.absolute,
		                  msg->tail.message.pack.max_count *
		                  sizeof( mpc_lowcomm_ptp_message_pack_absolute_list_t ) );
	}

	step = msg->tail.message.pack.count;
	msg->tail.message.pack.list.absolute[step].count = nb_items;
	msg->tail.message.pack.list.absolute[step].begins = begins;
	msg->tail.message.pack.list.absolute[step].ends = ends;
	msg->tail.message.pack.list.absolute[step].addr = adr;
	msg->tail.message.pack.list.absolute[step].elem_size = elem_size;
	msg->tail.message.pack.count++;
}

/********************************************************************/
/* Searching functions                                              */
/********************************************************************/

static __thread OPA_int_t m_probe_id;
static __thread OPA_int_t m_probe_id_task;

void sctk_m_probe_matching_init()
{
	OPA_store_int( &m_probe_id, 0 );
	OPA_store_int( &m_probe_id_task, -1 );
}

void sctk_m_probe_matching_set( int value )
{
	while ( OPA_cas_int( &m_probe_id, 0, value ) != 0 )
	{
		sctk_nodebug( "CAS %d", OPA_load_int( &m_probe_id ) );
		mpc_thread_yield();
	}

	int thread_id = mpc_common_get_thread_id();

	sctk_nodebug( "THREAD ID %d", thread_id );
	OPA_store_int( &m_probe_id_task, thread_id + 1 );
}

void sctk_m_probe_matching_reset()
{
	OPA_store_int( &m_probe_id, 0 );
	OPA_store_int( &m_probe_id_task, -1 );
}

int sctk_m_probe_matching_get()
{
	int thread_id = mpc_common_get_thread_id();

	if ( OPA_load_int( &m_probe_id_task ) != ( thread_id + 1 ) )
	{
		return -1;
	}

	return OPA_load_int( &m_probe_id );
}

__thread volatile int already_processsing_a_control_message = 0;

static inline mpc_lowcomm_msg_list_t *__mpc_comm_pending_msg_list_search_matching( mpc_comm_ptp_list_pending_t *pending_list,
        mpc_lowcomm_ptp_message_header_t *header )
{
	mpc_lowcomm_msg_list_t *ptr_found;
	mpc_lowcomm_msg_list_t *tmp;
	/* Loop on all  pending messages */
	DL_FOREACH_SAFE( pending_list->list, ptr_found, tmp )
	{
		mpc_lowcomm_ptp_message_header_t *header_found;
		sctk_assert( ptr_found->msg != NULL );
		header_found = &( ptr_found->msg->body.header );

		/* Control Message Handling */
		if ( header_found->message_type.type == MPC_LOWCOMM_CONTROL_MESSAGE_TASK )
		{
			if ( !already_processsing_a_control_message )
			{
				DL_DELETE( pending_list->list, ptr_found );
				/* Here we have a pending control message in the list we must take it out in order to avoid deadlocks */
				already_processsing_a_control_message = 1;
				sctk_control_messages_perform( ptr_found->msg, 0 );
				already_processsing_a_control_message = 0;
			}
		}

		/* Match the communicator, the tag, the source and the specific message tag */
		//sctk_error("Looking for C %d S %d SS %d T %d HAVE C %d S %d SS %d T %d", header->communicator, header->source_task, header->source, header->message_tag,
		//													header_found->communicator, header_found->source_task, header_found->source, header_found->message_tag);
		if ( /* Match Communicator */
		    ( header->communicator == header_found->communicator ) &&
		    /* Match message type */
		    ( header->message_type.type == header_found->message_type.type ) &&
		    /* Match source Process */
		    ( ( header->source == header_found->source ) || ( header->source == SCTK_ANY_SOURCE ) ) &&
		    /* Match source task appart for process specific messages which are not matched at task level */
		    ( ( header->source_task == header_found->source_task ) ||
		      ( header->source_task == SCTK_ANY_SOURCE ) ||
		      _mpc_comm_ptp_message_is_for_process( header->message_type.type ) ) &&
		    /* Match Message Tag while making sure that tags less than 0 are ignored (used for intercomm) */
		    ( ( header->message_tag == header_found->message_tag ) ||
		      ( ( header->message_tag == SCTK_ANY_TAG ) && ( header_found->message_tag >= 0 ) ) ) )
		{
			/* Message found. We delete it  */
			DL_DELETE( pending_list->list, ptr_found );

			/* Save matching datatypes inside the request */
			if ( ptr_found->msg->tail.request )
			{
				ptr_found->msg->tail.request->source_type = header->datatype;
				ptr_found->msg->tail.request->dest_type = header_found->datatype;
			}

			/* We return the pointer to the request */
			return ptr_found;
		}

		/* Check for canceled send messages*/
		if ( header_found->message_type.type == MPC_LOWCOMM_CANCELLED_SEND )
		{
			/* Message found. We delete it  */
			DL_DELETE( pending_list->list, ptr_found );
		}
	}
	/* No message found */
	return NULL;
}

/*
 * Probe for a matching message
 */
static inline int __mpc_comm_ptp_probe( mpc_comm_ptp_t *pair,
                                        mpc_lowcomm_ptp_message_header_t *header )
{
	mpc_lowcomm_msg_list_t *ptr_send;
	mpc_lowcomm_msg_list_t *tmp;
	__mpc_comm_ptp_message_list_merge_pending( &( pair->lists ) );
	DL_FOREACH_SAFE( pair->lists.pending_send.list, ptr_send, tmp )
	{
		sctk_assert( ptr_send->msg != NULL );
		mpc_lowcomm_ptp_message_header_t *header_send = &( ptr_send->msg->body.header );
		mpc_lowcomm_ptp_message_tail_t *tail_send = &( ptr_send->msg->tail );
		int send_message_matching_id =
		    OPA_load_int( &tail_send->matching_id );

		if ( /* Ignore Process Specific */
		    ( _mpc_comm_ptp_message_is_for_process( header->message_type.type ) == 0 ) &&
		    /* Has either no or the same matching ID */
		    ( ( send_message_matching_id == -1 ) ||
		      ( send_message_matching_id == OPA_load_int( &tail_send->matching_id ) ) ) &&
		    /* Match Communicator */
		    ( ( header->communicator == header_send->communicator ) ||
		      ( header->communicator == SCTK_ANY_COMM ) ) &&
		    /* Match message-type */
		    ( header->message_type.type == header_send->message_type.type ) &&
		    /* Match source task (note that we ignore source process
		       here as probe only come from the MPI layer == Only tasks */
		    ( ( header->source_task == header_send->source_task ) ||
		      ( header->source_task == SCTK_ANY_SOURCE ) ) &&
		    /* Match tag while making sure that tags less than 0 are
		       ignored (used for intercomm) */
		    ( ( header->message_tag == header_send->message_tag ) ||
		      ( ( header->message_tag == SCTK_ANY_TAG ) &&
		        ( header_send->message_tag >= 0 ) ) ) )
		{
			int matching_token = sctk_m_probe_matching_get();

			if ( matching_token != 0 )
			{
				OPA_store_int( &tail_send->matching_id, matching_token );
			}

			memcpy( header, &( ptr_send->msg->body.header ),
			        sizeof( mpc_lowcomm_ptp_message_header_t ) );
			return 1;
		}
	}
	return 0;
}

static inline int __mpc_comm_pending_msg_list_search_matching_from_recv( mpc_comm_ptp_t *pair, mpc_lowcomm_ptp_message_t *msg )
{
	sctk_assert( msg != NULL );
	mpc_lowcomm_msg_list_t *ptr_recv = &msg->tail.distant_list;
	mpc_lowcomm_msg_list_t *ptr_send = __mpc_comm_pending_msg_list_search_matching( &pair->lists.pending_send, &( msg->body.header ) );

	if ( SCTK_MSG_SPECIFIC_CLASS( msg ) == MPC_LOWCOMM_CANCELLED_RECV )
	{
		DL_DELETE( pair->lists.pending_recv.list, ptr_recv );
		assume( ptr_send == NULL );
	}

	/* We found a send request which corresponds to the recv request 'ptr_recv' */
	if ( ptr_send != NULL )
	{
		/* Recopy error if present */
		if ( ptr_send->msg->tail.request != NULL )
		{
			if ( ptr_send->msg->tail.request->status_error != SCTK_SUCCESS )
			{
				ptr_recv->msg->tail.request->status_error = ptr_send->msg->tail.request->status_error;
			}
		}

		/* Recv has matched a send, remove from list */
		DL_DELETE( pair->lists.pending_recv.list, ptr_recv );

		/* If the remote source is on a another node, we call the
		   notify matching hook in the inter-process layer. We do it
		   before copying the message to the receive buffer */
		if ( msg->tail.remote_source )
		{
			sctk_network_notify_matching_message( msg );
		}

		/*Insert the matching message to the list of messages that needs to be copied.*/
		_mpc_comm_ptp_copy_task_insert( ptr_recv, ptr_send, pair );
		return 1;
	}

	return 0;
}

/*
 * Function with loop on the recv 'pending' messages and try to match all
 * messages in this
 * list with the send 'pending' messages. This function must be called protected
 * by locks.
 * The function returns the number of messages copied (implies that this
 * messages where
 * matched).
 */
static inline int __mpc_comm_ptp_perform_msg_pair_locked( mpc_comm_ptp_t *pair )
{
	mpc_lowcomm_msg_list_t *ptr_recv;
	mpc_lowcomm_msg_list_t *tmp;
	int nb_messages_copied = 0;
	__mpc_comm_ptp_message_list_merge_pending( &( pair->lists ) );
	DL_FOREACH_SAFE( pair->lists.pending_recv.list, ptr_recv, tmp )
	{
		nb_messages_copied = __mpc_comm_pending_msg_list_search_matching_from_recv( pair, ptr_recv->msg );
	}
	return nb_messages_copied;
}

static inline int __mpc_comm_ptp_perform_msg_pair( mpc_comm_ptp_t *pair )
{
	if ( ( ( pair->lists.pending_send.list != NULL ) || ( pair->lists.incomming_send.list != NULL ) ) &&
	     ( ( pair->lists.pending_recv.list != NULL ) || ( pair->lists.incomming_recv.list != NULL ) ) )
	{
		__mpc_comm_ptp_message_list_lock_pending( &( pair->lists ) );
		__mpc_comm_ptp_perform_msg_pair_locked( pair );
		pair->lists.changed = 0;
		__mpc_comm_ptp_message_list_unlock_pending( &( pair->lists ) );
	}

	/* Call the task engine */
	return _mpc_comm_ptp_task_perform( pair->key.rank );
}

/*
 * Try to match all received messages with send requests from a internal ptp
 * list
 *
 * Returns:
 *  -1 Lock already taken
 *  >=0 Number of messages matched
 */
static inline int __mpc_comm_ptp_perform_msg_pair_trylock( mpc_comm_ptp_t *pair )
{
	int ret = -1;

	if ( !pair )
	{
		return ret;
	}

	SCTK_PROFIL_START( MPC_Test_message_pair_try );

	/* If the lock has not been taken, we continue */
	if ( OPA_load_int(&pair->lists.pending_lock) == 0 )
	{
		return __mpc_comm_ptp_perform_msg_pair( pair );
	}

	SCTK_PROFIL_END( MPC_Test_message_pair_try );
	return ret;
}

void mpc_lowcomm_ptp_msg_wait_init( struct mpc_lowcomm_ptp_msg_progress_s *wait,
                                    mpc_lowcomm_request_t *request, int blocking )
{
	wait->request = request;
	/* If we are in a MPI_Wait function or similar */
	wait->blocking = blocking;
	mpc_comm_ptp_t **recv_ptp = &wait->recv_ptp;
	mpc_comm_ptp_t **send_ptp = &wait->send_ptp;
	/* Searching for the pending list corresponding to the dest task */
	*recv_ptp = __mpc_comm_ptp_array_get( request->header.communicator, request->header.destination_task );
	*send_ptp = __mpc_comm_ptp_array_get( request->header.communicator, request->header.source_task );

	if ( request->request_type == REQUEST_SEND )
	{
		wait->polling_task_id = request->header.source_task;
	}
	else if ( request->request_type == REQUEST_RECV )
	{
		wait->polling_task_id = request->header.destination_task;
	}

	if ( request->request_type == REQUEST_GENERALIZED )
	{
		wait->polling_task_id = request->header.source_task;
	}

	/* Compute the rank of the remote process */
	if ( request->header.source_task != SCTK_ANY_SOURCE &&
	     request->header.communicator != SCTK_COMM_NULL &&
	     request->header.source_task != SCTK_PROC_NULL )
	{
		/* Convert task rank to process rank */
		wait->source_task_id = mpc_lowcomm_communicator_rank( request->header.communicator,
		                       request->header.source_task );
		wait->remote_process = sctk_get_process_rank_from_task_rank( wait->source_task_id );
	}
	else
	{
		wait->remote_process = -1;
		wait->source_task_id = -1;
	}
}

/*
 *  Function called when the message to receive is already completed
 */
static inline void __mpc_comm_ptp_msg_done( struct mpc_lowcomm_ptp_msg_progress_s *wait )
{
	const mpc_lowcomm_request_t *request = wait->request;

	/* The message is marked as done.
	   However, we need to poll if it is a inter-process message
	   and if we are waiting for a SEND request. If we do not do this,
	   we might overflow the number of send buffers waiting to be released */
	if ( request->header.source_task == SCTK_ANY_SOURCE )
	{
		sctk_network_notify_any_source_message( request->header.source_task, 0 );
	}
}

static inline void __mpc_comm_ptp_msg_wait( struct mpc_lowcomm_ptp_msg_progress_s *wait );

static void __mpc_comm_perform_msg_wfv( void *a )
{
	struct mpc_lowcomm_ptp_msg_progress_s *_wait = ( struct mpc_lowcomm_ptp_msg_progress_s * ) a;
	__mpc_comm_ptp_msg_wait( _wait );

	if ( ( volatile int ) _wait->request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE )
	{
		sctk_network_notify_idle_message();
	}
}

void mpc_lowcomm_perform_idle( volatile int *data, int value,
                               void ( *func )( void * ), void *arg )
{
	if ( *data == value )
	{
		return;
	}

	mpc_thread_wait_for_value_and_poll( data, value, func, arg );
}


static void (*__egreq_poll_trampoline)(void) = NULL;

void mpc_lowcomm_egreq_poll_set_trampoline(  void (*trampoline)(void) )
{
        __egreq_poll_trampoline = trampoline;
}

static inline void __egreq_poll()
{
        if(__egreq_poll_trampoline)
        {
                (__egreq_poll_trampoline)();
        }
}


void mpc_lowcomm_request_wait( mpc_lowcomm_request_t *request )
{
	if ( mpc_lowcomm_request_is_null( request ) )
	{
		return;
	}

	struct mpc_lowcomm_ptp_msg_progress_s _wait;

	if ( request->completion_flag == MPC_LOWCOMM_MESSAGE_CANCELED )
	{
		return;
	}

	if ( request->request_type == REQUEST_NULL )
	{
		return;
	}

#ifdef MPC_MPI
	if ( request->request_type == REQUEST_GENERALIZED )
	{
		mpc_lowcomm_perform_idle( ( int * ) & ( request->completion_flag ),
		                          MPC_LOWCOMM_MESSAGE_DONE, __egreq_poll, NULL );
        }
	else
#endif
	{
		if ( request->completion_flag == MPC_LOWCOMM_MESSAGE_DONE )
		{
			return;
		}

		/* Find the PTPs lists */
		mpc_lowcomm_ptp_msg_wait_init( &_wait, request, 1 );
		/* Fastpath try a few times directly before polling */
		int trials = 0;

		do
		{
			__mpc_comm_ptp_msg_wait( &_wait );
			trials++;
		}
		while ( ( request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE )  && ( trials < 32 ) );

		if ( request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE )
		{
			mpc_lowcomm_perform_idle(
			    ( int * ) & ( _wait.request->completion_flag ), MPC_LOWCOMM_MESSAGE_DONE,
			    ( void ( * )( void * ) ) __mpc_comm_perform_msg_wfv,
			    &_wait );
		}
		else
		{
			__mpc_comm_ptp_msg_done( &_wait );
		}
	}

	/* Propagate finish to parent request if present */
	if ( request->pointer_to_source_request )
	{
		( ( mpc_lowcomm_request_t * ) request->pointer_to_source_request )->completion_flag =
		    MPC_LOWCOMM_MESSAGE_DONE;
	}

	/* Free the shadow request bound if present (wait over the source ) */
	if ( request->pointer_to_shadow_request )
	{
		( ( mpc_lowcomm_request_t * ) request->pointer_to_shadow_request )
		->pointer_to_source_request = NULL;
	}
}

/*
 * Function called for performing message from a specific request.
 *
 * Firstly, we check if the message to receive is an intra-node message.
 * If it is, we walk on the pending list of the receive task in order
 * to find a message to match.
 *
 * If the message is an inter-node message, we call the inter-node
 * polling function according to the source of the message to
 * receive (SCTK_ANY_SOURCE or not).
 *
 */

static inline void __mpc_comm_ptp_msg_wait( struct mpc_lowcomm_ptp_msg_progress_s *wait )
{
	const mpc_lowcomm_request_t *request = wait->request;
	mpc_comm_ptp_t *recv_ptp = wait->recv_ptp;
	mpc_comm_ptp_t *send_ptp = wait->send_ptp;
	const int remote_process = wait->remote_process;
	const int source_task_id = wait->source_task_id;
	const int polling_task_id = wait->polling_task_id;
	const int blocking = wait->blocking;

	if ( request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE )
	{
		/* Check the source of the request. We try to poll the
			   source in order to retreive messages from the network */

		/* We try to poll for finding a message with a SCTK_ANY_SOURCE source
		   also in case we are blocked we punctually poll any-source */
		if ( request->header.source_task == SCTK_ANY_SOURCE )
		{
			/* We try to poll for finding a message with a SCTK_ANY_SOURCE source */
			sctk_network_notify_any_source_message( polling_task_id, blocking );
		}
		else if ( ( request->request_type == REQUEST_SEND && !recv_ptp ) ||
		          ( request->request_type == REQUEST_RECV && !send_ptp ) ||
		          ( request->header.source_task == request->header.destination_task ) )
		{
			/* We poll the network only if we need it (empty queues) */
			/* If the src and the dest tasks are same, we poll the network.
			   INFO: it is usefull for the overlap benchmark from the
				   MPC_THREAD_MULTIPLE Test Suite.
				   An additional thread is created and waiting ob MPI_Recv with src = dest */
			sctk_network_notify_perform_message( remote_process, source_task_id, polling_task_id, blocking );
		}

		if ( ( request->request_type == REQUEST_SEND ) && send_ptp )
		{
			__mpc_comm_ptp_perform_msg_pair_trylock( send_ptp );
		}
		else if ( ( request->request_type == REQUEST_RECV ) && recv_ptp )
		{
			__mpc_comm_ptp_perform_msg_pair_trylock( recv_ptp );
		}
		else
		{
			return;
		}

                if (  request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE )
                {
		        	sctk_network_notify_idle_message();
                }
	}
	else
	{
		__mpc_comm_ptp_msg_done( wait );
	}
}

/* This is the exported version */
void mpc_lowcomm_ptp_msg_progress( struct mpc_lowcomm_ptp_msg_progress_s *wait )
{
	return __mpc_comm_ptp_msg_wait( wait );
}

/*
 * Wait for all message according to a communicator and a task id
 */
void mpc_lowcomm_request_wait_all_msgs( const int task, const mpc_lowcomm_communicator_t com )
{
	mpc_comm_ptp_t *pair;
	int i;
	/* Get the pending list associated to the task */
	pair = __mpc_comm_ptp_array_get( com, task );
	sctk_assert( pair );

	do
	{
		i = OPA_load_int( &pair->pending_nb );
		sctk_nodebug( "pending = %d", pair->pending_nb );

		if ( i != 0 )
		{
			/* WARNING: The inter-process module *MUST* implement
				   the 'notify_any_source_message function'. If not,
				   the polling will never occur. */
			sctk_network_notify_any_source_message( task, 0 );
			__mpc_comm_ptp_perform_msg_pair_trylock( pair );
			/* Second test to avoid a thread_yield if the message has been handled */
			i = OPA_load_int( &pair->pending_nb );

			if ( i != 0 )
			{
				mpc_thread_yield();
			}
		}
	}
	while ( i != 0 );
}


/********************************************************************/
/* Send messages                                                    */
/********************************************************************/

/*
 * Function called when sending a message. The message can be forwarded to
 * another process
 * using the 'sctk_network_send_message' function. If the message
 * matches, we add it to the corresponding pending list
 * */
void _mpc_comm_ptp_message_send_check( mpc_lowcomm_ptp_message_t *msg, int poll_receiver_end )
{
	assert( mpc_common_get_process_rank() >= 0 );
	assert( mpc_common_get_process_count() >= SCTK_MSG_DEST_PROCESS( msg  ) );

	/* Flag msg's request as a send one */
	if ( msg->tail.request != NULL )
	{
		msg->tail.request->request_type = REQUEST_SEND;
	}

	/*  Message has not reached its destination */
	if ( SCTK_MSG_DEST_PROCESS( msg ) != mpc_common_get_process_rank() )
	{
		/* We forward the message */
		sctk_network_send_message( msg );
		return;
	}

	if ( _mpc_comm_ptp_message_is_for_control( SCTK_MSG_SPECIFIC_CLASS( msg ) ) )
	{
		/* If we are on the right process with a control message */
		/* Message is for local process call the handler (such message are never pending)
		       no need to perform a matching with a receive. However, the order is guaranteed
			   by the reorder prior to calling this function */
		sctk_control_messages_incoming( msg );
		/* Done */
		return;
	}

	assert( SCTK_MSG_COMMUNICATOR( msg ) >= 0 );

	if ( SCTK_MSG_COMPLETION_FLAG( msg ) != NULL )
	{
		*( SCTK_MSG_COMPLETION_FLAG( msg ) ) = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	/* Flag the Message as all local */
	msg->tail.remote_source = 0;
	/* We are searching for the corresponding pending list. If we do not find any entry, we forward the message */
	mpc_comm_ptp_t *src_pair = __mpc_comm_ptp_array_get( SCTK_MSG_COMMUNICATOR( msg ), SCTK_MSG_SRC_TASK( msg ) );
	mpc_comm_ptp_t *dest_pair = __mpc_comm_ptp_array_get( SCTK_MSG_COMMUNICATOR( msg ), SCTK_MSG_DEST_TASK( msg ) );

	if ( src_pair )
	{
		/* Message source is local */
		__mpc_comm_ptp_message_list_add_pending( src_pair, msg );
	}
	else
	{
		/* Message comes from network */
		msg->tail.internal_ptp = NULL;
	}

	assert( dest_pair != NULL );
	__mpc_comm_ptp_message_list_add_incoming_send( dest_pair, msg );

	/* If we ask for a checking, we check it */
	if ( poll_receiver_end )
	{
		__UNUSED__ int matched_nb;
		matched_nb = __mpc_comm_ptp_perform_msg_pair_trylock( dest_pair );
#ifdef MPC_Profiler

		switch ( matched_nb )
		{
			case -1:
				SCTK_COUNTER_INC( matching_locked, 1 );
				break;

			case 0:
				SCTK_COUNTER_INC( matching_not_found, 1 );
				break;

			default:
				SCTK_COUNTER_INC( matching_found, 1 );
				break;
		}

#endif
	}
}

/*
 * Function called for sending a message (i.e: MPI_Send).
 * Mostly used by the file mpc.c
 * */
void mpc_lowcomm_ptp_message_send( mpc_lowcomm_ptp_message_t *msg )
{
	int need_check = !_mpc_comm_is_remote_rank( SCTK_MSG_DEST_TASK( msg ) );
	_mpc_comm_ptp_message_send_check( msg, need_check );
}

/********************************************************************/
/* Recv messages                                                    */
/********************************************************************/

/*
 * Function called when receiving a message.
 * */
void _mpc_comm_ptp_message_recv_check( mpc_lowcomm_ptp_message_t *msg,
                                       int perform_check )
{
	if ( SCTK_MSG_COMPLETION_FLAG( msg ) != NULL )
	{
		*( SCTK_MSG_COMPLETION_FLAG( msg ) ) = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	if ( msg->tail.request != NULL )
	{
		msg->tail.request->request_type = REQUEST_RECV;
	}

	/* We are searching for the list corresponding to the task which receives the message */
	mpc_comm_ptp_t *send_ptp = __mpc_comm_ptp_array_get( SCTK_MSG_COMMUNICATOR( msg ), SCTK_MSG_SRC_TASK( msg ) );
	mpc_comm_ptp_t *recv_ptp = __mpc_comm_ptp_array_get( SCTK_MSG_COMMUNICATOR( msg ), SCTK_MSG_DEST_TASK( msg ) );
	/* We assume that the entry is found. If not, the message received is for a task which is not registered on the node. Possible issue. */
	assume( recv_ptp );

	if ( send_ptp == NULL )
	{
		/* This receive expects a message from a remote process */
		msg->tail.remote_source = 1;
		sctk_network_notify_recv_message( msg );
	}
	else
	{
		msg->tail.remote_source = 0;
	}

	/* We add the message to the pending list */
	__mpc_comm_ptp_message_list_add_pending( recv_ptp, msg );
	__mpc_comm_ptp_message_list_add_incoming_recv( recv_ptp, msg );

	/* Iw we ask for a matching, we run it */
	if ( perform_check )
	{
		__mpc_comm_ptp_perform_msg_pair_trylock( recv_ptp );
	}
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void mpc_lowcomm_ptp_message_recv( mpc_lowcomm_ptp_message_t *msg )
{
	int need_check = !_mpc_comm_is_remote_rank( SCTK_MSG_DEST_TASK( msg ) );
	_mpc_comm_ptp_message_recv_check( msg, need_check );
}

/********************************************************************/
/*Probe                                                             */
/********************************************************************/

static inline void
__mpc_comm_probe_source_tag_class_func( int destination, int source, int tag,
                                        mpc_lowcomm_ptp_message_class_t class,
                                        const mpc_lowcomm_communicator_t comm,
                                        int *status,
                                        mpc_lowcomm_ptp_message_header_t *msg )
{
	int world_source = source;
	int world_destination = destination;
	msg->source_task = world_source;
	msg->destination_task = world_destination;
	msg->message_tag = tag;
	msg->communicator = comm;
	msg->message_type.type = class;
	mpc_comm_ptp_t *dest_ptp = __mpc_comm_ptp_array_get( comm, world_destination );

	if ( source != SCTK_ANY_SOURCE )
	{
		mpc_comm_ptp_t *src_ptp = __mpc_comm_ptp_array_get( comm, world_source );

		if ( src_ptp == NULL )
		{
			int src_process = sctk_get_process_rank_from_task_rank( world_source );
			sctk_network_notify_perform_message( src_process, world_source, world_destination, 0 );
		}
	}
	else
	{
		sctk_network_notify_any_source_message( world_destination, 0 );
	}

	assume( dest_ptp != NULL );
	__mpc_comm_ptp_message_list_lock_pending( &( dest_ptp->lists ) );
	*status = __mpc_comm_ptp_probe( dest_ptp, msg );
	__mpc_comm_ptp_message_list_unlock_pending( &( dest_ptp->lists ) );
}

void mpc_lowcomm_message_probe_any_tag( int destination, int source,
                                        const mpc_lowcomm_communicator_t comm, int *status,
                                        mpc_lowcomm_ptp_message_header_t *msg )
{
	__mpc_comm_probe_source_tag_class_func( destination, source, SCTK_ANY_TAG,
	                                        MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg );
}

void mpc_lowcomm_message_probe_any_source( int destination, const mpc_lowcomm_communicator_t comm,
        int *status, mpc_lowcomm_ptp_message_header_t *msg )
{
	__mpc_comm_probe_source_tag_class_func( destination, SCTK_ANY_SOURCE,
	                                        msg->message_tag, MPC_LOWCOMM_P2P_MESSAGE, comm,
	                                        status, msg );
}

void mpc_lowcomm_message_probe_any_source_any_tag( int destination,
        const mpc_lowcomm_communicator_t comm, int *status,
        mpc_lowcomm_ptp_message_header_t *msg )
{
	__mpc_comm_probe_source_tag_class_func( destination, SCTK_ANY_SOURCE, SCTK_ANY_TAG,
	                                        MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg );
}

void mpc_lowcomm_message_probe( int destination, int source,
                                const mpc_lowcomm_communicator_t comm, int *status,
                                mpc_lowcomm_ptp_message_header_t *msg )
{
	__mpc_comm_probe_source_tag_class_func( destination, source, msg->message_tag,
	                                        MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg );
}



void mpc_lowcomm_message_probe_any_source_class_comm( int destination, int tag,
        mpc_lowcomm_ptp_message_class_t class,
        const mpc_lowcomm_communicator_t comm,
        int *status,
        mpc_lowcomm_ptp_message_header_t *msg )
{
	__mpc_comm_probe_source_tag_class_func( destination, SCTK_ANY_SOURCE, tag, class,
	                                        comm, status, msg );
}

/********************************************************************/
/*Message Interface                                                 */
/********************************************************************/


void mpc_lowcomm_request_init( mpc_lowcomm_request_t *request, mpc_lowcomm_communicator_t comm, int request_type )
{
	__mpc_comm_request_init( request, comm, request_type );
}

int mpc_lowcomm_request_free( mpc_lowcomm_request_t *request )
{
	int ret = SCTK_SUCCESS;
	/* Firstly wait the message before freeing */
	mpc_lowcomm_request_wait( request );

	if ( request->request_type == REQUEST_GENERALIZED )
	{
		ret = ( request->free_fn )( request->extra_state );
	}

	mpc_lowcomm_request_init( request, SCTK_COMM_NULL, REQUEST_NULL );
	request->is_null = 1;
	return ret;
}


int mpc_lowcomm_request_cancel( mpc_lowcomm_request_t *msg )
{
	int ret = SCTK_SUCCESS;

	switch ( msg->request_type )
	{
		case REQUEST_GENERALIZED:
			/* Call the cancel handler with a flag telling if the request was completed
			* which is our case is the same as not pending anymore */
			ret = ( msg->cancel_fn )( msg->extra_state,
			                          ( msg->completion_flag != MPC_LOWCOMM_MESSAGE_PENDING ) );
			break;

		case REQUEST_RECV:
			if ( msg->msg == NULL )
			{
				return ret;
			}

			SCTK_MSG_SPECIFIC_CLASS_SET( msg->msg, MPC_LOWCOMM_CANCELLED_RECV );
			break;

		case REQUEST_SEND:
			if ( msg->msg == NULL )
			{
				return ret;
			}

			/* NOTE: cancelling a Send is deprecated */
			if ( _mpc_comm_is_remote_rank( SCTK_MSG_DEST_PROCESS( msg->msg ) ) )
			{
				sctk_error( "Try to cancel a network message for %d from UNIX process %d",
				            SCTK_MSG_DEST_PROCESS( msg->msg ), mpc_common_get_process_rank() );
				not_implemented();
			}

			SCTK_MSG_SPECIFIC_CLASS_SET( msg->msg, MPC_LOWCOMM_CANCELLED_SEND );
			break;

		default:
			not_reachable();
			break;
	}

	msg->completion_flag = MPC_LOWCOMM_MESSAGE_CANCELED;
	return ret;
}

void mpc_lowcomm_isend_class_src( int src, int dest, const void *data, size_t size,
                                  int tag, mpc_lowcomm_communicator_t comm,
                                  mpc_lowcomm_ptp_message_class_t class,
                                  mpc_lowcomm_request_t *req )
{
	if ( dest == SCTK_PROC_NULL )
	{
		mpc_lowcomm_request_init( req, comm, REQUEST_SEND );
		return;
	}

	mpc_lowcomm_ptp_message_t *msg =
	    mpc_lowcomm_ptp_message_header_create( MPC_LOWCOMM_MESSAGE_CONTIGUOUS );
	mpc_lowcomm_ptp_message_set_contiguous_addr( msg, data, size );
	mpc_lowcomm_ptp_message_header_init( msg, tag, comm, src, dest, req, size, class,
	                                     SCTK_DATATYPE_IGNORE, REQUEST_SEND );
	mpc_lowcomm_ptp_message_send( msg );
}

void mpc_lowcomm_isend_class( int dest, const void *data, size_t size, int tag,
                              mpc_lowcomm_communicator_t comm,
                              mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req )
{
	mpc_lowcomm_isend_class_src( mpc_lowcomm_communicator_rank( comm, mpc_common_get_task_rank() ),
	                             dest,
	                             data,
	                             size,
	                             tag,
	                             comm,
	                             class,
	                             req );
}

void mpc_lowcomm_irecv_class_dest( int src, int dest, void *buffer, size_t size,
                                   int tag, mpc_lowcomm_communicator_t comm,
                                   mpc_lowcomm_ptp_message_class_t class,
                                   mpc_lowcomm_request_t *req )
{
	if ( src == SCTK_PROC_NULL )
	{
		mpc_lowcomm_request_init( req, comm, REQUEST_RECV );
		return;
	}

	mpc_lowcomm_ptp_message_t *msg =
	    mpc_lowcomm_ptp_message_header_create( MPC_LOWCOMM_MESSAGE_CONTIGUOUS );
	mpc_lowcomm_ptp_message_set_contiguous_addr( msg, buffer, size );
	mpc_lowcomm_ptp_message_header_init( msg, tag, comm, src, dest, req, size, class,
	                                     SCTK_DATATYPE_IGNORE, REQUEST_RECV );
	mpc_lowcomm_ptp_message_recv( msg );
}

void mpc_lowcomm_irecv_class(  int src, void *buffer, size_t size, int tag,
                               mpc_lowcomm_communicator_t comm,
                               mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req )
{
	mpc_lowcomm_irecv_class_dest( src, mpc_lowcomm_communicator_rank( comm, mpc_common_get_task_rank() ),
	                              buffer, size, tag, comm, class, req );
}


/*************************
 * PUBLIC COMM INTERFACE *
 *************************/


/*  ###############
 *  Rank Querry
 *  ############### */

int mpc_lowcomm_get_rank()
{
	return mpc_common_get_task_rank();
}

int mpc_lowcomm_get_size()
{
	return mpc_common_get_task_count();
}


int mpc_lowcomm_get_comm_rank( const mpc_lowcomm_communicator_t communicator )
{
	return mpc_lowcomm_communicator_rank ( communicator, mpc_lowcomm_get_rank() );
}

int mpc_lowcomm_get_comm_size(const mpc_lowcomm_communicator_t communicator)
{
	mpc_lowcomm_communicator_size( communicator );
}

int mpc_lowcomm_get_process_count()
{
	return mpc_common_get_process_count();
}

int mpc_lowcomm_get_process_rank()
{
	return mpc_common_get_process_rank();
}

/*  ###############
 *  Communicators
 *  ############### */

mpc_lowcomm_communicator_t mpc_lowcomm_create_comm( const mpc_lowcomm_communicator_t origin_communicator,
        const int nb_task_involved,
        const int *task_list )
{
	return sctk_create_communicator ( origin_communicator, nb_task_involved, task_list );
}

mpc_lowcomm_communicator_t mpc_lowcomm_delete_comm( const mpc_lowcomm_communicator_t comm )
{
	return sctk_delete_communicator ( comm );
}

/*  ###############
 *  Messages
 *  ############### */


void mpc_lowcomm_isend( int dest, const void *data, size_t size, int tag,
                        mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req )
{
	mpc_lowcomm_isend_class( dest, data, size, tag, comm, MPC_LOWCOMM_P2P_MESSAGE, req );
}

void mpc_lowcomm_irecv( int src, void *data, size_t size, int tag,
                        mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req )
{
	mpc_lowcomm_irecv_class( src, data, size, tag, comm, MPC_LOWCOMM_P2P_MESSAGE, req );
}

void mpc_lowcomm_request_wait( mpc_lowcomm_request_t *request );


void mpc_lowcomm_sendrecv( void *sendbuf, size_t size, int dest, int tag, void *recvbuf,
                           int src, int comm )
{
	mpc_lowcomm_request_t sendreq, recvreq;
	mpc_lowcomm_isend( dest, sendbuf, size, tag, comm, &sendreq );
	mpc_lowcomm_irecv( src, recvbuf, size, tag, comm, &recvreq );
	mpc_lowcomm_request_wait( &sendreq );
	mpc_lowcomm_request_wait( &recvreq );
}


void mpc_lowcomm_wait( mpc_lowcomm_request_t *request )
{
	mpc_lowcomm_request_wait( request );
}

void mpc_lowcomm_send( int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm )
{
	mpc_lowcomm_request_t req;
	mpc_lowcomm_isend( dest, data, size, tag, comm, &req );
	mpc_lowcomm_wait( &req );
}

void mpc_lowcomm_recv( int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm )
{
	mpc_lowcomm_request_t req;
	mpc_lowcomm_irecv( src, buffer, size, tag, comm, &req );
	mpc_lowcomm_wait( &req );
}


/*  ###############
 *  Setup & Teardown
 *  ############### */

void mpc_launch_init_runtime();

void mpc_lowcomm_libmode_init()
{
	mpc_launch_init_runtime();
	mpc_lowcomm_barrier( SCTK_COMM_WORLD );
}

void mpc_lowcomm_libmode_release()
{
	mpc_launch_release_runtime();
	mpc_lowcomm_barrier( SCTK_COMM_WORLD );
}

static void __lowcomm_init_per_task()
{
        int task_rank = mpc_common_get_task_rank();

	/* We call for all threads as some
	   progress threads may need buffered headers */
	if ( task_rank >= 0 )
	{

		mpc_lowcomm_terminaison_barrier();

		mpc_lowcomm_init_per_task( task_rank );

		mpc_lowcomm_terminaison_barrier();
	}
}

static void __lowcomm_release()
{
        int task_rank = mpc_common_get_task_rank();

	if ( task_rank >= 0 )
	{
		sctk_nodebug( "mpc_lowcomm_terminaison_barrier" );
		mpc_lowcomm_terminaison_barrier();
		mpc_lowcomm_terminaison_barrier();
		sctk_nodebug( "mpc_lowcomm_terminaison_barrier done" );
		sctk_net_finalize_task_level( task_rank, mpc_topology_get_current_cpu() );
	}
	else
	{
		not_reachable();
	}
}

static void __initialize_drivers()
{
	if ( mpc_common_get_process_count() > 1 )
	{
#ifdef MPC_USE_INFINIBAND
		if ( !mpc_common_get_flags()->network_driver_name && !sctk_ib_device_found() )
		{
			if ( !mpc_common_get_process_rank() )
			{
				mpc_common_debug( "Could not locate an IB device, fallback to TCP" );
			}

			mpc_common_get_flags()->network_driver_name = "tcp";
		}
#endif
		sctk_net_init_driver(mpc_common_get_flags()->network_driver_name);
	}

	mpc_lowcomm_rdma_window_init_ht();

	mpc_lowcomm_coll_init_hook = *( void ** )( &sctk_runtime_config_get()->modules.inter_thread_comm.collectives_init_hook.value );
	sctk_communicator_world_init ( mpc_common_get_flags()->task_number );
	sctk_communicator_self_init ();


#ifdef SCTK_LIB_MODE
	sctk_net_init_task_level( my_rank, 0 );
#endif
}

static void __finalize_driver()
{
	mpc_lowcomm_rdma_window_release_ht();
}

#ifdef MPC_USE_DMTCP
static void __initialize_ft(void)
{
	sctk_ft_init();
}
#endif


void mpc_lowcomm_registration() __attribute__((constructor));

void mpc_lowcomm_registration()
{
        MPC_INIT_CALL_ONLY_ONCE

        mpc_common_init_callback_register("Base Runtime Init Done", "LowComm Init", __initialize_drivers, 16);

        mpc_common_init_callback_register("Base Runtime Finalize", "Release LowComm", __finalize_driver, 16);

        mpc_common_init_callback_register("After Ending VPs", "Block sigpipe", mpc_common_helper_ignore_sigpipe, 0);

        mpc_common_init_callback_register("VP Thread Start", "MPC Message Passing Init per Task", __lowcomm_init_per_task, 0);

        mpc_common_init_callback_register("VP Thread End", "MPC Message Passing Release", __lowcomm_release, 0);

#ifdef MPC_USE_DMTCP
        mpc_common_init_callback_register("Base Runtime Init with Config", "Initialize Fault-Tolerance", __initialize_ft, 77);
#endif

}
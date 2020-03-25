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

#ifndef MPC_COMMON_H
#define MPC_COMMON_H

#include <uthash.h>
#include "mpcmp.h"
#include "mpc_info.h"
#include "mpc_datatypes.h"
#include "mpc_extended_request.h"
#include "sctk_debug.h"
#include "progress_engine.h"
#include "sctk_communicator.h"

/************************************************************************/
/* Per communicator context                                             */
/************************************************************************/

struct mpc_mpi_per_communicator_s;

typedef struct {
  sctk_communicator_t key;

  sctk_spinlock_t err_handler_lock;
  MPC_Handler_function*  err_handler;

  struct mpc_mpi_per_communicator_s* mpc_mpi_per_communicator;
  void (*mpc_mpi_per_communicator_copy)(struct mpc_mpi_per_communicator_s**,struct mpc_mpi_per_communicator_s*);
  void (*mpc_mpi_per_communicator_copy_dup)(struct mpc_mpi_per_communicator_s**,struct mpc_mpi_per_communicator_s*);

  UT_hash_handle hh;
}mpc_per_communicator_t;

/************************************************************************/
/* Per task context                                                     */
/************************************************************************/

/**
 *  \brief This structure describes an atexit handler for a task
 * 
 *  \warning atexit is rewitten in mpc_main.h generated by the configure
 *            this is just in case you were wondering.
 */
struct sctk_task_specific_atexit_s
{
	void (*func)(); /**< Function to be called when task exits */
	struct sctk_task_specific_atexit_s * next; /**< Following function to call */
};


/**
 *  \brief Describes the context of an MPI task
 *  
 *  This data structure is initialised by \ref __MPC_init_task_specific_t and
 * 	released by \ref __MPC_release_task_specific_t. Initial setup is done
 *  in \ref __MPC_setup_task_specific called in \ref sctk_user_main.
 * 
 */
typedef struct sctk_task_specific_s
{
	/* TODO */
	struct mpc_mpi_data_s* mpc_mpi_data;
        struct sctk_internal_ptp_s **my_ptp_internal;

	/* Communicator handling */
	mpc_per_communicator_t*per_communicator;
	sctk_spinlock_t per_communicator_lock;

	/* ID */
	int task_id; /**< MPI comm rank of the task */

	/* Status */
	int init_done;  /**< =1 if the task has called MPI_Init() 2
			     =2 if the task has called MPI_Finalize */
	int thread_level;

	/* Data-types */
	struct Datatype_Array *datatype_array;

	/* Extended Request Class handling */
	struct GRequest_context grequest_context;

        /* MPI_Info handling */
        struct MPC_Info_factory
            info_fact; /**< This structure is used to store the association
                                      between MPI_Infos structs and their ID */

        /* At EXIT */
        struct sctk_task_specific_atexit_s
            *exit_handlers; /**< These functions are called when tasks leaves
                                               (atexit) */

        /* For disguisement */
        struct sctk_thread_data_s * thread_data;

        /* Progresss List */
        struct sctk_progress_list * progress_list;
} sctk_task_specific_t;

struct sctk_task_specific_s *__MPC_get_task_specific();

/** \brief Unlock the datatype array
 */
static inline void sctk_datatype_lock( sctk_task_specific_t *task_specific )
{
	sctk_assert( task_specific != NULL );
	Datatype_Array_lock();
}

/** \brief Lock the datatype array
 */
static inline void sctk_datatype_unlock( sctk_task_specific_t *task_specific )
{
	sctk_assert( task_specific != NULL );
	Datatype_Array_unlock();
}

sctk_contiguous_datatype_t * sctk_task_specific_get_contiguous_datatype( sctk_task_specific_t *task_specific, MPC_Datatype datatype );
sctk_contiguous_datatype_t *sctk_get_contiguous_datatype(MPC_Datatype datatype);

sctk_derived_datatype_t * sctk_task_specific_get_derived_datatype(  sctk_task_specific_t *task_specific, MPC_Datatype datatype );
sctk_derived_datatype_t *sctk_get_derived_datatype(MPC_Datatype datatype);

/** \brief Retrieves a given per communicator context from task CTX
 */
mpc_per_communicator_t* sctk_thread_getspecific_mpc_per_comm(struct sctk_task_specific_s* task_specific,sctk_communicator_t comm);

int __INTERNAL__PMPC_Type_hcontiguous (MPC_Datatype * datatype, size_t count, MPC_Datatype *data_in, struct Datatype_External_context * ctx);

int MPC_Is_derived_datatype (MPC_Datatype datatype, int *res, sctk_derived_datatype_t *output_datatype );

int MPC_Datatype_set_context( MPC_Datatype datatype,  struct Datatype_External_context * dctx );

int PMPC_Derived_datatype_on_slot ( int id,
				    mpc_pack_absolute_indexes_t * begins,
				    mpc_pack_absolute_indexes_t * ends,
				    MPC_Datatype * types,
			     	    unsigned long count,
				    mpc_pack_absolute_indexes_t lb, int is_lb,
				    mpc_pack_absolute_indexes_t ub, int is_ub);

int PMPC_Type_set_size(MPC_Datatype datatype, size_t size );
/************************************************************************/
/* Per thread context message buffers                                   */
/************************************************************************/

#define MAX_MPC_BUFFERED_MSG 32

typedef struct 
{
	mpc_buffered_msg_t buffer[MAX_MPC_BUFFERED_MSG];
	volatile int buffer_rank;
	sctk_spinlock_t lock;
} sctk_buffer_t;

typedef struct sctk_thread_buffer_pool_s
{
	sctk_buffer_t sync;
	sctk_buffer_t async;
}sctk_thread_buffer_pool_t;


/************************************************************************/
/* Per thread context	                                          */
/************************************************************************/

typedef struct sctk_thread_specific_s
{
	sctk_thread_buffer_pool_t buffer_pool;
}sctk_thread_specific_t;

sctk_thread_specific_t * sctk_get_thread_specific();
void sctk_set_thread_specific( sctk_thread_specific_t * pointer );

void MPC_Init_thread_specific();
void MPC_Release_thread_specific();

/************************************************************************/
/* Non Generic MPI interface function                                   */
/************************************************************************/

int __MPC_Waitallp (mpc_msg_count count,
		   MPC_Request * parray_of_requests[],
		   MPC_Status array_of_statuses[]);


#endif /* MPC_COMMON_H */
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
#include "messaging.h"
#include "mpc_info.h"
#include "mpc_datatypes.h"
#include "egreq_classes.h"
#include "sctk_debug.h"
#include "egreq_progress.h"
#include "sctk_communicator.h"
#include "sctk_inter_thread_comm.h"

/************************************************************************/
/* Per communicator context                                             */
/************************************************************************/

struct mpc_mpi_per_communicator_s;

typedef struct {
  mpc_mp_communicator_t key;

  mpc_common_spinlock_t err_handler_lock;
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
struct mpc_mpi_m_per_mpi_process_ctx_atexit_s
{
	void (*func)(); /**< Function to be called when task exits */
	struct mpc_mpi_m_per_mpi_process_ctx_atexit_s * next; /**< Following function to call */
};

/**
 *  \brief Describes the context of an MPI task
 *  
 *  This data structure is initialised by \ref __mpc_m_per_mpi_process_ctx_init and
 * 	released by \ref __mpc_m_per_mpi_process_ctx_release. Initial setup is done
 *  in \ref __mpc_m_per_mpi_process_ctx_init called in \ref mpc_mpi_m_mpi_process_main.
 * 
 */
typedef struct mpc_mpi_m_per_mpi_process_ctx_s
{
	/* TODO */
	struct mpc_mpi_data_s *mpc_mpi_data;

	/* Communicator handling */
	mpc_per_communicator_t *per_communicator;
	mpc_common_spinlock_t per_communicator_lock;

	/* ID */
	int task_id; /**< MPI comm rank of the task */

	/* Status */
	int init_done; /**< =1 if the task has called MPI_Init() 2
			     =2 if the task has called MPI_Finalize */
	int thread_level;

	/* Data-types */
	struct Datatype_Array *datatype_array;

	/* Extended Request Class handling */
	struct _mpc_egreq_classes_storage grequest_context;

	/* MPI_Info handling */
	struct MPC_Info_factory info_fact; /**< This structure is used to store the association
                                                between MPI_Infos structs and their ID */

	/* At EXIT */
	struct mpc_mpi_m_per_mpi_process_ctx_atexit_s  *exit_handlers; /**< These functions are called when tasks leaves (atexit) */

	/* For disguisement */
	struct sctk_thread_data_s *thread_data;

	/* Progresss List */
	struct _mpc_egreq_progress_list *progress_list;
} mpc_mpi_m_per_mpi_process_ctx_t;

struct mpc_mpi_m_per_mpi_process_ctx_s *_mpc_m_per_mpi_process_ctx_get();

/** \brief Unlock the datatype array
 */
static inline void sctk_datatype_lock( mpc_mpi_m_per_mpi_process_ctx_t *task_specific )
{
	sctk_assert( task_specific != NULL );
	Datatype_Array_lock();
}

/** \brief Lock the datatype array
 */
static inline void sctk_datatype_unlock( mpc_mpi_m_per_mpi_process_ctx_t *task_specific )
{
	sctk_assert( task_specific != NULL );
	Datatype_Array_unlock();
}

sctk_contiguous_datatype_t * _mpc_m_per_mpi_process_ctx_contiguous_datatype_ts_get( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
									            mpc_mp_datatype_t datatype );
sctk_contiguous_datatype_t *_mpc_m_per_mpi_process_ctx_contiguous_datatype_get(mpc_mp_datatype_t datatype);

sctk_derived_datatype_t * _mpc_m_per_mpi_process_ctx_derived_datatype_ts_get(  mpc_mpi_m_per_mpi_process_ctx_t *task_specific, mpc_mp_datatype_t datatype );
sctk_derived_datatype_t *_mpc_m_per_mpi_process_ctx_derived_datatype_get(mpc_mp_datatype_t datatype);

/** \brief Retrieves a given per communicator context from task CTX
 */
mpc_per_communicator_t* _mpc_m_per_communicator_get(struct mpc_mpi_m_per_mpi_process_ctx_s* task_specific,mpc_mp_communicator_t comm);

int _mpc_m_type_hcontiguous_ctx (mpc_mp_datatype_t * datatype, size_t count, mpc_mp_datatype_t *data_in, struct Datatype_External_context * ctx);

int _mpc_m_derived_datatype_try_get_info (mpc_mp_datatype_t datatype, int *res, sctk_derived_datatype_t *output_datatype );

int _mpc_m_type_ctx_set( mpc_mp_datatype_t datatype,  struct Datatype_External_context * dctx );

int _mpc_m_derived_datatype_on_slot ( int id,
				    mpc_pack_absolute_indexes_t * begins,
				    mpc_pack_absolute_indexes_t * ends,
				    mpc_mp_datatype_t * types,
			     	    unsigned long count,
				    mpc_pack_absolute_indexes_t lb, int is_lb,
				    mpc_pack_absolute_indexes_t ub, int is_ub);

int _mpc_m_type_set_size(mpc_mp_datatype_t datatype, size_t size );

/************************************************************************/
/* Per Communicating Thread context	                                          */
/************************************************************************/

struct mpc_mpi_m_per_thread_ctx_s;

void mpc_mpi_m_per_thread_ctx_init();
void mpc_mpi_m_per_thread_ctx_release();

/************************************************************************/
/* Non Generic MPI interface function                                   */
/************************************************************************/

int _mpc_m_waitallp (mpc_mp_msg_count_t count,
		   mpc_mp_request_t * parray_of_requests[],
		   mpc_mp_status_t array_of_statuses[]);


#endif /* MPC_COMMON_H */

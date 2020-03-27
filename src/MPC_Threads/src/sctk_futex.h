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
#ifndef SCTK_FUTEX_H
#define SCTK_FUTEX_H

#include "mpc_common_datastructure.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_asm.h"
#include "sctk_futex_def.h"

/************************************************************************/
/* Futex Cell                                                         */
/************************************************************************/

struct futex_cell
{
	int *do_wait;
	int  freed;
	int  bitmask;
	int  skip;
	int  orig_op;
};

struct futex_cell *futex_cell_new(int bitmask, int orig_op);
int  futex_cell_match(struct futex_cell *cell, int bitmask);
void futex_cell_detach(struct futex_cell *cell);


struct futex_bitset_iterator_desc
{
	int bitmask;
	int to_process;
};


int futex_cell_apply_bitmask(void *elem /* struct futex_cell */,
                             void *arg /* struct futex_bitset_iterator_desc */);

/************************************************************************/
/* Futex Queues                                                         */
/************************************************************************/


struct futex_queue
{
	mpc_common_spinlock_t  queue_is_wake_tainted;
	struct mpc_common_fifo wait_list;
	int *                  futex_key;
};

struct futex_queue *futex_queue_new(int *futex_key);
int futex_queue_release(struct futex_queue *fq);
int *futex_queue_push(struct futex_queue *fq, int bitmask, int orig_op);
int futex_queue_wake(struct futex_queue *fq, int bitmask, int use_mask, int count, int op);

/************************************************************************/
/* Futex HT                                                             */
/************************************************************************/

struct futex_queue_HT
{
	OPA_int_t                   queue_table_is_being_manipulated;

	struct mpc_common_hashtable queue_hash_table;
	unsigned int                queue_count;
	unsigned int                queue_cleanup_ratio;
};

int futex_queue_HT_init(struct futex_queue_HT *ht);
int futex_queue_HT_release(struct futex_queue_HT *ht);
int *futex_queue_HT_register_thread(struct futex_queue_HT *ht, int *futex_key, int bitmask, int orig_op);
int futex_queue_HT_wake_threads(struct futex_queue_HT *ht, int *futex_key, int bitmask, int use_mask, int count, int op);


/************************************************************************/
/* Futex Context                                                        */
/************************************************************************/

void sctk_futex_context_init();
void sctk_futex_context_release();


#endif /* SCTK_FUTEX_H */
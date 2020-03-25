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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_barrier.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_openmp_tls.h"

// missing forward declaration
void mpcomp_taskwait( void );

/** Barrier for all threads of the same team */
void __mpcomp_internal_full_barrier(mpcomp_mvp_t *mvp) {
  long b, b_done;
  mpcomp_node_t *c, *new_root;

  sctk_assert(mvp);

  if( mvp->threads->info.num_threads == 1)
  {
    return;
  }

  sctk_assert(mvp->father);
  sctk_assert(mvp->father->instance);
  sctk_assert(mvp->father->instance->team);

  mpcomp_thread_t* thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
  c = mvp->father;
  new_root = thread->instance->root;

#if MPCOMP_TASK
  mpcomp_taskwait();
#endif /* MPCOMP_TASK */

  /* Step 1: Climb in the tree */
  b_done = c->barrier_done; /* Move out of sync region? */
  b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;

  while (b == c->barrier_num_threads && c != new_root) {
    sctk_atomics_store_int(&(c->barrier), 0);
    c = c->father;
    b_done = c->barrier_done;
    b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;
  }

  /* Step 2 - Wait for the barrier to be done */
  if (c != new_root || (c == new_root && b != c->barrier_num_threads)) {
#if OMPT_SUPPORT
    if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
      ompt_callback_sync_region_t callback;
      callback = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region_wait];

      if( callback ) {
         mpcomp_thread_t *t = mvp->threads;
         ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
         ompt_data_t* task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
         const void* code_ra = __builtin_return_address( 1 );

         callback( ompt_sync_region_barrier, ompt_scope_begin, parallel_data, task_data, code_ra );
      }
    }
#endif /* OMPT_SUPPORT */

    /* Wait for c->barrier == c->barrier_num_threads */
    while (b_done == c->barrier_done) {
      sctk_thread_yield();
#if MPCOMP_TASK
      mpcomp_task_schedule(1);
#endif /* MPCOMP_TASK */
    }

#if OMPT_SUPPORT
    if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
      ompt_callback_sync_region_t callback;
      callback = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region_wait];

      if( callback ) {
         mpcomp_thread_t *t = mvp->threads;
         ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
         ompt_data_t* task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
         const void* code_ra = __builtin_return_address( 1 );

         callback( ompt_sync_region_barrier, ompt_scope_end, parallel_data, task_data, code_ra );
      }
    }
#endif /* OMPT_SUPPORT */
  } else {
    sctk_atomics_store_int(&(c->barrier), 0);

#if MPCOMP_COHERENCY_CHECKING
    mpcomp_for_dyn_coherency_end_barrier();
    mpcomp_single_coherency_end_barrier();
#endif

    c->barrier_done++ ; /* No need to lock I think... */
  }

  /* Step 3 - Go down */
  while ( c->child_type != MPCOMP_CHILDREN_LEAF ) {
    c = c->children.node[mvp->tree_rank[c->depth]];
    c->barrier_done++; /* No need to lock I think... */
  }
#if MPCOMP_TASK
#if MPCOMP_COHERENCY_CHECKING
// mpcomp_task_coherency_barrier();
#endif
#endif /* MPCOMP_TASK */
}

/* Half barrier for the end of a parallel region */
void __mpcomp_internal_half_barrier_start( mpcomp_mvp_t *mvp ) 
{
  mpcomp_node_t *c, *new_root;

  sctk_assert(mvp);
  sctk_assert(mvp->father);
  sctk_assert(mvp->father->instance);
  sctk_assert(mvp->father->instance->team);

  new_root = mvp->instance->root;
  c = mvp->father;
   
  sctk_assert( c );
  sctk_assert(new_root != NULL);

#if 0 //MPCOMP_TASK
  (void)mpcomp_thread_tls_store(mvp->threads[0]);
  __mpcomp_internal_full_barrier(mvp);
  (void)mpcomp_thread_tls_store_father(); // To check...
#endif /* MPCOMP_TASK */

  /* Step 1: Climb in the tree */
  long b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;
  while (b == c->barrier_num_threads && c != new_root) {
    sctk_atomics_store_int(&(c->barrier), 0);
    c = c->father;
    b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;
  }
}

void __mpcomp_internal_half_barrier_end( mpcomp_mvp_t *mvp )
{
    mpcomp_thread_t* cur_thread;
    mpcomp_node_t* root;
    assert( mvp->threads );

    cur_thread = mvp->threads;

    /* End barrier for master thread */
    if( cur_thread->rank ) return;
    
    root = cur_thread->instance->root; 
    const int expected_num_threads = root->barrier_num_threads;
    while (sctk_atomics_load_int(&(root->barrier)) != expected_num_threads) 
        sctk_thread_yield();        
    sctk_atomics_store_int( &( root->barrier ), 0); 
}

/*
   OpenMP barrier.
   All threads of the same team must meet.
   This barrier uses some optimizations for threads inside the same microVP.
 */
void __mpcomp_barrier(void) {

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  sctk_nodebug("[%d] %s: Entering w/ %d thread(s)\n", t->rank, __func__,
               t->info.num_threads);

#if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
    ompt_data_t* parallel_data;
    ompt_data_t* task_data;

    if( t->info.in_single ) {
      t->info.in_single = 0;
      ompt_work_t wstype = (t->rank == t->instance->team->info.doing_single) ? ompt_work_single_executor : ompt_work_single_other;
      if( wstype == ompt_work_single_executor ) t->instance->team->info.doing_single = -1;

      ompt_callback_work_t callback_work;
      callback_work = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];

      if( callback_work ) {
        parallel_data = &(t->instance->team->info.ompt_region_data );
        task_data = &( t->task_infos.current_task->ompt_task_data );
        const void* code_ra = __builtin_return_address( 0 );
        callback_work( wstype, ompt_scope_end, parallel_data, task_data, 1, code_ra );
      }
    }

    ompt_callback_sync_region_t callback_sync;
    callback_sync = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region];

    if( callback_sync ) {
      parallel_data = &( t->instance->team->info.ompt_region_data );
      task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
      const void* code_ra = __builtin_return_address( 1 );

      callback_sync( ompt_sync_region_barrier, ompt_scope_begin, parallel_data, task_data, code_ra );
    }
  //__mpcomp_ompt_barrier_begin( false );
  }
#endif /* OMPT_SUPPORT */

  if (t->info.num_threads != 1)
  {
    /* Get the corresponding microVP */
    mpcomp_mvp_t *mvp = t->mvp;
    sctk_assert(mvp != NULL);
    sctk_nodebug("[%d] %s: t->mvp = %p", t->rank, __func__, t->mvp);
 
    /* Call the real barrier */
    sctk_assert( t->info.num_threads == t->mvp->threads->info.num_threads );
    __mpcomp_internal_full_barrier(mvp);
  }

#if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
    ompt_callback_sync_region_t callback;
    callback = (ompt_callback_sync_region_t) OMPT_Callbacks[ompt_callback_sync_region];

    if( callback ) {
       ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
       ompt_data_t* task_data = &(MPCOMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data );
       const void* code_ra = __builtin_return_address( 1 );

       callback( ompt_sync_region_barrier, ompt_scope_end, parallel_data, task_data, code_ra );
    }
  }
  //__mpcomp_ompt_barrier_end( true );
#endif /* OMPT_SUPPORT */
}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
__asm__(".symver __mpcomp_barrier, GOMP_barrier@@GOMP_1.0");
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
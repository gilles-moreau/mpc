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
#ifndef __SCTK_ETHREAD_INTERNALS_H_
#define __SCTK_ETHREAD_INTERNALS_H_

#include <mpc_config.h>

#ifdef __MIC__
	#include <immintrin.h>
#endif
	
#include <errno.h>
#include "sctk_debug.h"
#include "sctk_ethread.h"
#include "sctk_alloc.h"
#include "sctk_kernel_thread.h"
#ifdef MPC_Thread_db
#include "sctk_thread_dbg.h"
#endif
#include "sched.h"
#include <mpc_common_asm.h>
#include <mpc_common_helper.h>
#include <mpc_common_flags.h>
#include <mpc_runtime_config.h>

#ifdef __cplusplus
extern "C"
{
#endif


  void* sctk_align_ptr_to_page(void* ptr);
  size_t sctk_align_size_to_page(size_t size);



#define SCTK_ACTIVITY_DOWN(vp) vp->idle_activity++
#define SCTK_ACTIVITY_UP(vp) vp->activity++


  static inline
    int __sctk_ethread_poll_vp (sctk_ethread_virtual_processor_t * vp,
				sctk_ethread_per_thread_t * cur)
  {
    sctk_ethread_status_t stat_sav;
      stat_sav = cur->status;
      sctk_assert (cur->status != ethread_inside_polling);
      cur->status = ethread_inside_polling;

/*   if (sctk_net_ptp_poll != NULL) */
/*     sctk_net_ptp_poll (NULL); */

    if (expect_false (vp->poll_list != NULL))
      {
	sctk_ethread_polling_t *new_poll_list = NULL;
	sctk_ethread_polling_t *poll_list;
	sctk_ethread_polling_t *last = NULL;

	  poll_list = (sctk_ethread_polling_t *) vp->poll_list;
	  vp->poll_list = NULL;
	while (poll_list != NULL)
	  {
	    sctk_ethread_polling_t *tmp;
	    void *arg;
	    void (*func) (void *);

	      sctk_assert (vp->current == cur);
	      sctk_assert (vp == poll_list->my_self->vp);

	      arg = poll_list->arg;
	      func = poll_list->func;

	    if ((*(poll_list->data) != poll_list->value) && (func != NULL))
	      {
		/*Warning we can be yielded during func call */
		func (arg);
	      }

	    sctk_assert (vp->current == cur);
	    tmp = poll_list->next;
	    if (*(poll_list->data) == poll_list->value)
	      {
		sctk_assert_func
		  (if (poll_list->my_self->status != ethread_polling)
		   {
		   sctk_ethread_print_task (poll_list->my_self);
		   sctk_error ("Cur vp %p", vp);
		   sctk_ethread_print_task (cur);
		   sctk_assert (poll_list->my_self->status ==
				ethread_polling);}
		);
		sctk_nodebug ("Wake %p from polling %p vp %p",
			      poll_list->my_self, poll_list->data, vp);
		poll_list->my_self->status = ethread_ready;
		sctk_ethread_enqueue_task (vp, poll_list->my_self);
	      }
	    else
	      {
		poll_list->next = new_poll_list;
		new_poll_list = poll_list;
		if (last == NULL)
		  {
		    last = poll_list;
		  }
	      }

	    poll_list = tmp;
	  }
	if (last != NULL)
	  {
	    last->next = (sctk_ethread_polling_t *) vp->poll_list;
	    vp->poll_list = new_poll_list;
	  }
      }
    cur->status = stat_sav;
    return 0;
  }

#ifdef SCTK_KERNEL_THREAD_USE_TLS
  extern __thread void *sctk_ethread_key;
#else
  extern kthread_key_t sctk_ethread_key;
#endif

#ifdef SCTK_SCHED_CHECK
  static inline void
    __sctk_ethread_sched_yield_vp_head (sctk_ethread_virtual_processor_t
					* vp, sctk_ethread_per_thread_t * cur)
  {
#ifndef NO_INTERNAL_ASSERT
    sctk_ethread_per_thread_t *f_task = NULL;
    sctk_ethread_status_t start_status;
#endif

    sctk_nodebug ("thread %p yield %d", cur, cur->status);

    sctk_assert_func (start_status = cur->status;
      );

    sctk_assert (cur->status != ethread_system);
    sctk_assert_func (if (cur->vp != vp)
		      {
		      sctk_error ("%p != %p", cur->vp, vp);
		      sctk_ethread_print_task (cur);
		      sctk_assert (cur->vp == vp);}
    );
    sctk_assert (vp->current == cur);
    /*
       #if !defined(SCTK_ia64_ARCH_SCTK)
       sctk_assert_func
       (
       do{
       if(cur->stack != NULL){
       if(cur->stack[SCTK_ETHREAD_STACK_SIZE] != 123){
       sctk_error("Stack overflow %d",cur->stack[SCTK_ETHREAD_STACK_SIZE]);
       abort();
       }
       }
       }while(0);
       );
       #endif
     */
  }
#endif

  static inline int
    __sctk_ethread_sched_proceed_signals (sctk_ethread_per_thread_t *
					 restrict cur)
  {
#ifndef WINDOWS_SYS
    int i;
    int done = 0;
    mpc_common_spinlock_lock (&(cur->spinlock));
    for (i = 0; i < SCTK_NSIG; i++)
      {
	if (expect_false (cur->thread_sigpending[i] != 0))
	  {
	    if (sigismember ((sigset_t *) & (cur->thread_sigset), i + 1) == 0)
	      {
		cur->thread_sigpending[i] = 0;
		cur->nb_sig_pending--;
		done++;
		kthread_kill (kthread_self (), i + 1);
	      }
	  }
      }
    mpc_common_spinlock_unlock (&(cur->spinlock));
    cur->nb_sig_proceeded += done;
    return done;
#else
    return 0;
#endif
  }

  static inline int __sctk_ethread_sigpending (sctk_ethread_per_thread_t *
					       cur, sigset_t * set)
  {
#ifndef WINDOWS_SYS
    int i;
    sigemptyset (set);
    mpc_common_spinlock_lock (&(cur->spinlock));
    for (i = 0; i < SCTK_NSIG; i++)
      {
	if (cur->thread_sigpending[i] != 0)
	  {
	    sigaddset (set, i + 1);
	  }
      }
    mpc_common_spinlock_unlock (&(cur->spinlock));
#endif
    return 0;
  }

  static inline void __sctk_ethread_exit (void *retval,
					  sctk_ethread_per_thread_t *
					  th_data);
  extern void sctk_thread_exit (void *__retval);

  static inline void SCTK_THREAD_CHECK_SIGNALS (sctk_ethread_per_thread_t
						* restrict cur,
						int cancel_type)
  {
    if (expect_false (cur->nb_sig_pending > 0))
      {
	__sctk_ethread_sched_proceed_signals (cur);
      }
    if (expect_false (cur->cancel_status > 0))
      {
	sctk_nodebug ("%p %d %d", cur,
		      (cur->cancel_state != SCTK_THREAD_CANCEL_DISABLE),
		      ((cur->cancel_type != SCTK_THREAD_CANCEL_DEFERRED)
		       && cancel_type));
	if ((cur->cancel_state != SCTK_THREAD_CANCEL_DISABLE)
	    && ((cur->cancel_type != SCTK_THREAD_CANCEL_DEFERRED)
		|| !cancel_type))
	  {
	    cur->cancel_status = 0;
	    sctk_thread_exit ((void *) SCTK_THREAD_CANCELED);
	  }
      }
  }

  static inline int __sctk_ethread_sigmask (sctk_ethread_per_thread_t *
					    cur, int how,
					    const sigset_t * newmask,
					    sigset_t * oldmask)
  {
    int res = -1;
#ifndef WINDOWS_SYS
    sigset_t set;
    kthread_sigmask (SIG_SETMASK, (sigset_t *) & (cur->thread_sigset), &set);
    res = kthread_sigmask (how, newmask, oldmask);
    kthread_sigmask (SIG_SETMASK, &set, (sigset_t *) & (cur->thread_sigset));
    SCTK_THREAD_CHECK_SIGNALS (cur, 1);
#endif
    return res;
  }

  int sctk_thread_yield (void);

  static inline int __sctk_ethread_sigsuspend (sctk_ethread_per_thread_t *
					       cur, sigset_t * set)
  {
#ifndef WINDOWS_SYS
    sigset_t oldmask;
    sigset_t pending;
    int i;
    cur->nb_sig_proceeded = 0;
    __sctk_ethread_sigmask (cur, SIG_SETMASK, set, &oldmask);
    while (cur->nb_sig_proceeded == 0)
      {
	sctk_thread_yield ();
	sigpending (&pending);
	for (i = 0; i < SCTK_NSIG; i++)
	  {
	    if ((sigismember
		 ((sigset_t *) & (cur->thread_sigset), i + 1) == 1)
		&& (sigismember (&pending, i + 1) == 1))
	      {
		kthread_kill (kthread_self (), i + 1);
		cur->nb_sig_proceeded = 1;
	      }
	  }
      }
    __sctk_ethread_sigmask (cur, SIG_SETMASK, &oldmask, NULL);
    errno = EINTR;
#endif
    return -1;
  }


  static inline
    void __sctk_ethread_testcancel (sctk_ethread_per_thread_t * owner)
  {
    SCTK_THREAD_CHECK_SIGNALS (owner, 0);
  }

  static inline int __sctk_ethread_kill (sctk_ethread_per_thread_t *
					 thread, int val)
  {
    sctk_nodebug ("__sctk_ethread_kill %p %d set", thread, val);
    if (thread->status == ethread_joined)
      {
	return ESRCH;
      }
    if (val == 0)
      return 0;
    val--;
    if ((val < 0) || (val >= SCTK_NSIG))
      return EINVAL;

    mpc_common_spinlock_lock (&(thread->spinlock));
    if (thread->thread_sigpending[val] == 0)
      {
	thread->thread_sigpending[val] = 1;
	thread->nb_sig_pending++;
      }
    mpc_common_spinlock_unlock (&(thread->spinlock));
    return 0;
  }

  extern sctk_thread_key_t stck_task_data;

#if defined(SCTK_USE_THREAD_DEBUG)
  void sctk_thread_print_stack_out (void);
  static inline void sctk_ethread_set_status (sctk_ethread_per_thread_t *
					      cur, sctk_thread_status_t s)
  {
    sctk_thread_data_t *tmp;
    tmp = (sctk_thread_data_t *) cur->tls[stck_task_data];
    sctk_nodebug ("TMP = %p", tmp);

    /** ** **/
    sctk_refresh_thread_debug (cur, s);
    /** */

    if (tmp != NULL)
      {
	if (s == sctk_thread_check_status)
	  {
	    switch (cur->status)
	      {
	      case ethread_ready:
		s = sctk_thread_running_status;
		break;
	      default:
		s = tmp->status;
	      }
	  }

	sctk_getcontext (&(cur->ctx));

	tmp->status = s;
      }
  }
#else
#define sctk_ethread_set_status(a,b) (void)(0)
#endif
  static inline void
    __sctk_ethread_sched_yield_vp_tail (sctk_ethread_virtual_processor_t
					* restrict vp,
					sctk_ethread_per_thread_t *
					restrict cur)
  {
    sctk_ethread_set_status (cur, sctk_thread_check_status);

    vp = (sctk_ethread_virtual_processor_t *) cur->vp;
    sctk_nodebug ("thread %p(%p) restart vp->current %p", cur,
		  &(cur->ctx), vp->current);


    sctk_nodebug ("%p %d", cur, cur->nb_sig_pending);
    SCTK_THREAD_CHECK_SIGNALS (cur, 1);

    if (expect_false (vp->to_check))
      {
	if (vp->migration != NULL)
	  {
	    sctk_nodebug ("mirgation in yield");
	    vp->migration->
	      migration_func ((sctk_ethread_per_thread_t *) vp->migration);
	    vp->migration = NULL;
	    sctk_nodebug ("mirgation in yield done");
	  }
	if (vp->dump != NULL)
	  {
	    char name[MPC_COMMON_MAX_FILENAME_SIZE];
#if 1
	    not_implemented();
#else
	    if (vp->dump->dump_for_migration == 0)
	      {
		sprintf (name, "%s/%s", sctk_store_dir,
			 vp->dump->file_to_dump);
	      }
	    else
	      sprintf (name, "%s/mig_task_%p", sctk_store_dir, vp->dump);
#endif

/* 	  vp->dump->status = ethread_ready; */

	    vp->dump->dump_for_migration = 0;
	    __sctk_dump_tls (vp->dump->tls_mem, name);
	    vp->dump = NULL;
	  }
	if (vp->zombie != NULL)
	  {
	    if (vp->zombie->attr.detached == SCTK_ETHREAD_CREATE_DETACHED)
	      {
		__sctk_ethread_enqueue_task ((sctk_ethread_per_thread_t
					      *)
					     vp->zombie,
					     &(vp->zombie_queue),
					     &(vp->zombie_queue_tail));
	      }
	    vp->zombie->status = ethread_zombie;
	    sctk_nodebug ("%p thread become zombie", vp->zombie);
	    vp->zombie = NULL;
	  }
	vp->to_check = 0;
      }

    sctk_assert ((cur->status == ethread_ready) ||
		 (cur->status == ethread_idle) ||
		 (cur->status == ethread_inside_polling));
    sctk_assert (vp->current == cur);

    cur->no_auto_enqueue = 0;

  }

  static inline
    int __sctk_ethread_sched_yield_vp (sctk_ethread_virtual_processor_t
				       * restrict vp,
				       sctk_ethread_per_thread_t *
				       restrict cur)
  {
    sctk_ethread_per_thread_t *restrict new_task = NULL;
    sctk_ethread_status_t status;
    int request_migration = 0;

#ifdef SCTK_SCHED_CHECK
    __sctk_ethread_sched_yield_vp_head (vp, cur);
#endif

    status = cur->status;

    if (expect_false (status == ethread_ready))
      {
	sctk_ethread_set_status (cur, sctk_thread_sleep_status);
	if (cur->no_auto_enqueue == 0)
	  {
	    sctk_ethread_enqueue_task (vp, cur);
	  }
      }
    else
      {
	switch (status)
	  {
	  case ethread_blocked:
	    {
	      sctk_ethread_set_status (cur, sctk_thread_blocked_status);
	    }
	    break;
	  case ethread_idle:
	    {
	      __sctk_ethread_poll_vp (vp, cur);
	      /*New tasks */
	      sctk_ethread_get_incomming_tasks (vp);
	      /*Old tasks */
	      sctk_ethread_get_old_tasks (vp);

	      sctk_ethread_set_status (cur, sctk_thread_sleep_status);
	    }
	    break;
	  case ethread_block_on_vp:
	    {
	      sctk_ethread_set_status (cur, sctk_thread_blocked_status);
	      cur->status = ethread_ready;
	    }
	    break;
	  case ethread_inside_polling:
	    {
	      /*New tasks */
	      sctk_ethread_get_incomming_tasks (vp);
	      /*Old tasks */
	      sctk_ethread_get_old_tasks (vp);

	      sctk_ethread_set_status (cur, sctk_thread_sleep_status);
	    }
	    break;
	  case ethread_dump:
	    {
	      if (vp->dump != NULL)
		{
		  char name[MPC_COMMON_MAX_FILENAME_SIZE];
#if 1
		  not_implemented();
#else
		  if (vp->dump->dump_for_migration == 0)
		    {
		      sprintf (name, "%s/%s", sctk_store_dir,
			       vp->dump->file_to_dump);
		    }
		  else
		    sprintf (name, "%s/mig_task_%p", sctk_store_dir,
			     vp->dump);
#endif
		  /*      vp->dump->status = ethread_ready; */

		  vp->dump->dump_for_migration = 0;
		  __sctk_dump_tls (vp->dump->tls_mem, name);
		  vp->dump = NULL;
		}
	      sctk_ethread_set_status (cur, sctk_thread_sleep_status);
	      vp->dump = cur;
	      vp->to_check = 1;
	      if (cur->dump_for_migration == 0)
		{
		  cur->status = ethread_ready;
		  sctk_ethread_enqueue_task (vp, cur);
		}
	    }
	    break;
	  case ethread_migrate:
	    {
	      if (vp->migration != NULL)
		{
		  sctk_nodebug ("mirgation in yield");
		  vp->migration->
		    migration_func ((sctk_ethread_per_thread_t *) vp->
				    migration);
		  vp->migration = NULL;
		  sctk_nodebug ("mirgation in yield done");
		}
	      sctk_ethread_set_status (cur, sctk_thread_sleep_status);
	      sctk_nodebug ("ethread_migrate");
	      cur->status = ethread_ready;
	      vp->migration = cur;
	      vp->to_check = 1;
	      request_migration = 1;
	    }
	    break;
	  default:
	    sctk_ethread_set_status (cur, sctk_thread_blocked_status);
	  }
      }
    new_task = sctk_ethread_dequeue_task_for_run (vp);

    if (expect_false (new_task == NULL))
      {
	if (cur->status != ethread_idle)
	  {
	    new_task = vp->idle;
	  }
	else
	  {
	    sctk_assert (vp->current == cur);
	    return 0;
	  }
      }
    else
      {
	SCTK_ACTIVITY_UP (vp);
      }

    sctk_assert (new_task != NULL);
    sctk_assert ((new_task->status == ethread_ready) ||
		 (new_task->status == ethread_idle) ||
		 (new_task->status == ethread_inside_polling));

    if (expect_true (cur != new_task))
      {

	vp->current = new_task;
	sctk_nodebug ("Jump %p to %p(%p) on vp %d", cur, new_task,
		      &(new_task->ctx), vp->rank);
	sctk_swapcontext (&(cur->ctx), &(new_task->ctx));

      }

    __sctk_ethread_sched_yield_vp_tail (vp, cur);

    if(request_migration == 1){
      sctk_refresh_thread_debug_migration(cur);
    }

    return 1;
  }

  static inline int
    __sctk_ethread_sched_yield_vp_idle (sctk_ethread_virtual_processor_t
					* restrict vp,
					sctk_ethread_per_thread_t *
					restrict cur)
  {
    sctk_ethread_per_thread_t *restrict new_task;

    __sctk_ethread_poll_vp (vp, cur);
    /*New tasks */
    sctk_ethread_get_incomming_tasks (vp);
    /*Old tasks */
    sctk_ethread_get_old_tasks (vp);

    new_task = sctk_ethread_dequeue_task_for_run (vp);

    if (expect_true (new_task != NULL))
      {
	sctk_assert (new_task != NULL);
	sctk_assert (new_task->status == ethread_ready);

	if (expect_true (cur != new_task))
	  {
	    sctk_ethread_set_status (cur, sctk_thread_sleep_status);
	    SCTK_ACTIVITY_UP (vp);

	    vp->current = new_task;
	    sctk_nodebug ("idle Jump %p to %p(%p) on vp %d", cur,
			  new_task, &(new_task->ctx), vp->rank);
	    sctk_swapcontext (&(cur->ctx), &(new_task->ctx));
	    sctk_nodebug ("idle thread %p(%p) restart vp->current %p",
			  cur, &(cur->ctx), vp->current);

	  }
	else
	  {
	    SCTK_ACTIVITY_DOWN (vp);
	  }
	__sctk_ethread_sched_yield_vp_tail (vp, cur);
      }
    else
      {
	SCTK_ACTIVITY_DOWN (vp);
      }

    return 0;
  }

  static inline int
    __sctk_ethread_sched_yield_vp_poll (sctk_ethread_virtual_processor_t
					* restrict vp,
					sctk_ethread_per_thread_t *
					restrict cur)
  {
    sctk_ethread_per_thread_t *restrict new_task;

#ifdef SCTK_SCHED_CHECK
    __sctk_ethread_sched_yield_vp_head (vp, cur);
#endif

    sctk_ethread_set_status (cur, sctk_thread_blocked_status);

    new_task = sctk_ethread_dequeue_task_for_run (vp);

    if (expect_false (new_task == NULL))
      {
	new_task = vp->idle;
      }
    else
      {
	SCTK_ACTIVITY_UP (vp);
      }

    sctk_nodebug ("NEW task %p cur %p vp %p", new_task, cur, vp);
    sctk_assert (new_task != NULL);
    sctk_assert (new_task != cur);
    sctk_assert ((new_task->status == ethread_ready) ||
		 (new_task->status == ethread_idle) ||
		 (new_task->status == ethread_inside_polling));


    vp->current = new_task;
    sctk_nodebug ("Jump %p to %p(%p) on vp %d", cur, new_task,
		  &(new_task->ctx), vp->rank);
    sctk_swapcontext (&(cur->ctx), &(new_task->ctx));


    __sctk_ethread_sched_yield_vp_tail (vp, cur);

    return 0;
  }

  static inline int
    __sctk_ethread_sched_yield_vp_freeze
    (sctk_ethread_virtual_processor_t * restrict vp,
     sctk_ethread_per_thread_t * restrict cur)
  {
    sctk_ethread_per_thread_t *restrict new_task;

#ifdef SCTK_SCHED_CHECK
    __sctk_ethread_sched_yield_vp_head (vp, cur);
#endif

    sctk_ethread_set_status (cur, sctk_thread_blocked_status);

    new_task = sctk_ethread_dequeue_task_for_run (vp);

    if (expect_false (new_task == NULL))
      {
	new_task = vp->idle;
      }
    else
      {
	SCTK_ACTIVITY_UP (vp);
      }

    sctk_assert (new_task != NULL);
    sctk_assert (new_task != cur);
    sctk_assert ((new_task->status == ethread_ready) ||
		 (new_task->status == ethread_idle) ||
		 (new_task->status == ethread_inside_polling));


    vp->current = new_task;
    sctk_nodebug ("Jump %p to %p(%p) on vp %d", cur, new_task,
		  &(new_task->ctx), vp->rank);
    sctk_swapcontext (&(cur->ctx), &(new_task->ctx));


    __sctk_ethread_sched_yield_vp_tail (vp, cur);

    return 0;
  }

  /*Thread polling */
  static inline void
  __sctk_ethread_wait_for_value_and_poll( sctk_ethread_virtual_processor_t *restrict vp,
										  sctk_ethread_per_thread_t *restrict cur,
										  volatile int *restrict data, int value, void ( *func )( void * ), void *arg )
  {
	  sctk_ethread_polling_t cell;
	  sctk_ethread_status_t stat_sav;
    int i=0;
	  if ( cur->status != ethread_ready )
	  {
		  while ( *data != value )
		  {
			  if ( func != NULL )
			  {
				  func( arg );
			  }

			  if ( 128 < i )
			  {
				  __sctk_ethread_sched_yield_vp( vp, cur );
				  i = 0;
			  }
			  i++;
		  }

		  return;
	  }

	  assume( ( cur->status == ethread_ready ) );

	  sctk_assert( vp->current == cur );

    for( i = 0 ; i < vp->eagerness ; i++)
    {
      if ( *data != value )
      {
        if( func != NULL)
          func( arg );
      } else {
        vp->eagerness=SCTK_ETHREAD_VP_EAGERNESS;
        return;
      }
    }

    if(vp->eagerness)
      vp->eagerness--;
    else
      vp->eagerness = 1;

    cell.forced = 0;
    cell.data = data;
    cell.value = value;
    cell.func = func;
    cell.arg = arg;
    cell.my_self = cur;
    stat_sav = cell.my_self->status;
    cell.my_self->status = ethread_polling;
    cell.next = (sctk_ethread_polling_t *) vp->poll_list;
    vp->poll_list = &cell;

    sctk_nodebug( "Wait %p from polling", cell.my_self );
    __sctk_ethread_sched_yield_vp_poll( vp, cell.my_self );
    sctk_nodebug( "Wait %p from polling done", cell.my_self );

    sctk_assert( cell.forced == 0 );
    sctk_assert_func( if ( *data != value ) {
      sctk_error ("%d != %d", *data, value);
      sctk_ethread_print_task (cur);
      sctk_assert (*data == value); } );

    cell.my_self->status = stat_sav;
	  
  }

  static inline void __sctk_grab_zombie( sctk_ethread_virtual_processor_t
											 *vp )
  {
	  sctk_ethread_per_thread_t *tmp_pid;

	  tmp_pid = __sctk_ethread_dequeue_task( &( vp->zombie_queue ),
											 &( vp->zombie_queue_tail ) );
	  while ( tmp_pid != NULL )
	  {
		  struct sctk_alloc_chain *tls;
		  if ( tmp_pid->attr.stack == NULL )
			  sctk_free( tmp_pid->stack );
		  tls = tmp_pid->tls_mem;
		  sctk_free( tmp_pid );

		  if ( tls != sctk_thread_tls )
		  {
			  __sctk_delete_thread_memory_area( tls );
		  }
		  tmp_pid = __sctk_ethread_dequeue_task( &( vp->zombie_queue ),
												 &( vp->zombie_queue_tail ) );
	  }
  }

  static inline
    int __sctk_ethread_join (sctk_ethread_virtual_processor_t * vp,
			     sctk_ethread_per_thread_t * cur,
			     sctk_ethread_t th, void **val)
  {
    /*
       ERRORS:
       ESRCH  No  thread could be found corresponding to that specified by th.
       EINVAL The th thread has been detached.
       EINVAL Another thread is already waiting on termination of th.
       EDEADLK The th argument refers to the calling thread.
     */
    sctk_ethread_status_t *status;
    sctk_nodebug ("Join Thread %p", th);
    __sctk_ethread_testcancel (cur);
    if (th != cur)
      {
	if (th->status == ethread_joined)
	  {
	    return ESRCH;
	  }
	if (th->attr.detached != SCTK_ETHREAD_CREATE_JOINABLE)
	  return EINVAL;

	th->nb_wait_for_join++;
	if (th->nb_wait_for_join != 1)
	  return EINVAL;

	status = (sctk_ethread_status_t *) & (th->status);
	sctk_nodebug ("TO Join Thread %p", th);
	__sctk_ethread_wait_for_value_and_poll (vp, cur,
						(volatile int *) status,
						ethread_zombie, NULL, NULL);
	sctk_nodebug ("Joined Thread %p", th);
	if (val != NULL)
	  *val = th->ret_val;
	th->status = ethread_joined;
/* 	__sctk_ethread_enqueue_task (th, */
/* 				     &(vp->zombie_queue), */
/* 				     &(vp->zombie_queue_tail)); */

	sctk_nodebug ("Joined Thread %p grab ", th);
	__sctk_grab_zombie (vp);
	sctk_nodebug ("Joined Thread %p grab done", th);

      }
    else
      {
	return EDEADLK;
      }
    return 0;
  }

  /*Mutex management */
  static inline int __sctk_ethread_mutex_init (sctk_ethread_mutex_t *
					       lock,
					       sctk_ethread_mutexattr_t *
					       attr)
  {
    sctk_ethread_mutex_t lock_tmp = SCTK_ETHREAD_MUTEX_INIT;
    if (attr != NULL)
      lock_tmp.type = attr->kind;

    *lock = lock_tmp;

    return 0;
  }

  static inline int
    __sctk_ethread_mutex_lock (sctk_ethread_virtual_processor_t *
			       restrict vp,
			       sctk_ethread_per_thread_t * restrict owner,
			       sctk_ethread_mutex_t * restrict lock)
  {

    sctk_ethread_mutex_cell_t cell;

    sctk_nodebug ("%p lock on %p %d", owner, lock, lock->lock);
    if (lock->owner == owner)
      {
	if (lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
	  {
	    sctk_assert (lock->lock >= 1);
	    mpc_common_spinlock_lock (&lock->spinlock);
	    lock->lock++;
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    return 0;
	  }
	else if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK)
	  {
/* 	    assume (lock->owner != owner); */
	    return EDEADLK;
	  }
      }

    mpc_common_spinlock_lock (&lock->spinlock);

    if (lock->lock >= 1)
      {
	cell.my_self = owner;
	cell.next = NULL;
	cell.wake = 0;
	if (lock->list == NULL)
	  {
	    lock->list = &cell;
	    lock->list_tail = &cell;
	  }
	else
	  {
	    lock->list_tail->next = &cell;
	    lock->list_tail = &cell;
	  }
	sctk_nodebug ("%p blocked on %p", owner, lock);
	if (owner->status == ethread_ready)
	  {
	    owner->status = ethread_blocked;
	    owner->no_auto_enqueue = 1;
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    __sctk_ethread_sched_yield_vp_poll (vp, owner);
	    owner->status = ethread_ready;
	  }
	else
	  {
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    while (cell.wake != 1)
	      {
		__sctk_ethread_sched_yield_vp (vp, owner);
	      }
	    /*Courte phase d'attente en cas de lib�ration d'une t�che de polling */
	    while (lock->owner != owner)
	      {
		__sctk_ethread_sched_yield_vp (vp, owner);
	      }
	  }
      }
    else
      {
	lock->lock = 1;
	lock->owner = owner;
	mpc_common_spinlock_unlock (&lock->spinlock);
      }
    sctk_assert (lock->lock >= 1);

    return 0;
  }

#define SCTK_ETHREAD_SPIN_DELAY 10
  static inline int
    __sctk_ethread_mutex_spinlock (sctk_ethread_virtual_processor_t * vp,
				   sctk_ethread_per_thread_t * owner,
				   sctk_ethread_mutex_t * lock)
  {

    sctk_ethread_mutex_cell_t cell;

    sctk_nodebug ("%p lock on %p %d", owner, lock, lock->lock);
    if (lock->owner == owner)
      {
	if (lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
	  {
	    sctk_assert (lock->lock >= 1);
	    mpc_common_spinlock_lock (&lock->spinlock);
	    lock->lock++;
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    return 0;
	  }
	else if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK)
	  {
	    /*      assume (lock->owner != owner); */
	    return EDEADLK;
	  }
      }

    mpc_common_spinlock_lock (&lock->spinlock);

    if (lock->lock >= 1)
      {
	long i = sctk_runtime_config_get()->modules.thread.spin_delay;
	long j = sctk_runtime_config_get()->modules.thread.spin_delay;
	cell.my_self = owner;
	cell.next = NULL;
	cell.wake = 0;
	if (lock->list == NULL)
	  {
	    lock->list = &cell;
	    lock->list_tail = &cell;
	  }
	else
	  {
	    lock->list_tail->next = &cell;
	    lock->list_tail = &cell;
	  }
	sctk_nodebug ("%p blocked on %p", owner, lock);
	if (owner->status == ethread_ready)
	  {
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    while ((cell.wake) != 1 && (lock->owner != owner))
	      {
		i--;
		if (i <= 0)
		  {
		    j--;
		    sched_yield ();
		    i = sctk_runtime_config_get()->modules.thread.spin_delay;
		  }
		else
		  sctk_cpu_relax ();


		if (j <= 0)
		  {
		    __sctk_ethread_sched_yield_vp (vp, owner);
		    j = sctk_runtime_config_get()->modules.thread.spin_delay;
		  }
	      }
	  }
	else
	  {
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    while (cell.wake != 1)
	      {
		__sctk_ethread_sched_yield_vp (vp, owner);
	      }
	    /*Courte phase d'attente en cas de lib�ration d'une t�che de polling */
	    while (lock->owner != owner)
	      {
		__sctk_ethread_sched_yield_vp (vp, owner);
	      }
	  }
      }
    else
      {
	lock->lock = 1;
	lock->owner = owner;
	mpc_common_spinlock_unlock (&lock->spinlock);
      }
    sctk_assert (lock->lock >= 1);

    return 0;
  }

  static inline int
    __sctk_ethread_mutex_trylock (sctk_ethread_virtual_processor_t *
				  restrict vp,
				  sctk_ethread_per_thread_t *
				  restrict owner,
				  sctk_ethread_mutex_t * restrict lock)
  {
    sctk_nodebug ("%p trylock on %p %d", owner, lock, lock->lock);


    if (lock->owner == owner)
      {
	if (lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
	  {
	    sctk_nodebug ("on est dans le mutex r�cursif");
	    mpc_common_spinlock_lock (&lock->spinlock);
	    lock->lock++;
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    return 0;
	  }
	else if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK)
	  {
/* 	    assume (lock->owner != owner); */
	    return EDEADLK;
	  }

      }

    if (lock->lock > 0)
      return EBUSY;

    /*
       In order to limit contention
     */
    /*
       On met ce test pour limiter au maximum la contention sur le spinlock.
       Si on arrive pas � prendre le spinlock, c'est qu'il y a du mon et donc que l'on est
       pas prioritaire.
     */
    if (mpc_common_spinlock_trylock (&lock->spinlock))
      {
	return EBUSY;
      }


    if (lock->lock > 0)
      {
	mpc_common_spinlock_unlock (&lock->spinlock);
	return EBUSY;
      }
    else
      {
	lock->lock = 1;
	lock->owner = owner;
	mpc_common_spinlock_unlock (&lock->spinlock);
      }

    sctk_assert (lock->lock > 0);
    /*Courte pahse d'attente en cas de lib�ration d'une t�che de polling */
    while (lock->owner != owner)
      {
	__sctk_ethread_sched_yield_vp (vp, owner);
      }
    if (lock->owner != owner)
      {
	sctk_error ("%p != %p in %s at line %d",
		    lock->owner, owner, __FILE__, __LINE__);
	sctk_ethread_print_task (owner);
	sctk_ethread_print_task ((sctk_ethread_per_thread_t *) lock->owner);
	sctk_assert (lock->owner == owner);
	return EPERM;
      }

    return 0;
  }

  static inline int
    __sctk_ethread_mutex_unlock (__UNUSED__ sctk_ethread_virtual_processor_t *
				 restrict vp,
				 sctk_ethread_per_thread_t *
				 restrict owner, void (*retrun_task)
				 (sctk_ethread_per_thread_t *
				  restrict task),
				 sctk_ethread_mutex_t * restrict lock)
  {

    sctk_ethread_mutex_cell_t *cell;
    sctk_nodebug
      (" owner = %p ; le lock appartient � : %p -- on est : %p",
       owner, lock->owner, vp->current);
    if (lock->owner != owner)
      {
	return EPERM;
      }

    if (lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
      {
	if (lock->lock > 1)
	  {
	    mpc_common_spinlock_lock (&lock->spinlock);
	    lock->lock--;
	    mpc_common_spinlock_unlock (&lock->spinlock);
	    return 0;
	  }
      }
    mpc_common_spinlock_lock (&lock->spinlock);
    sctk_assert (lock->lock == 1);
    if (lock->list != NULL)
      {
	sctk_ethread_per_thread_t *to_wake;
	cell = (sctk_ethread_mutex_cell_t *) lock->list;
	lock->owner = cell->my_self;

	lock->list = lock->list->next;
	if (lock->list == NULL)
	  {
	    lock->list_tail = NULL;
	  }
	to_wake = (sctk_ethread_per_thread_t *) lock->owner;
	cell->wake = 1;
	sctk_nodebug ("a r�veiller %p", to_wake);
	retrun_task (to_wake);
      }
    else
      {
	lock->owner = NULL;
	lock->lock = 0;
      }
    mpc_common_spinlock_unlock (&lock->spinlock);


    sctk_nodebug ("%p unlock on %p %d", owner, lock, lock->lock);
    return 0;
  }

  /*Key management */
  static inline
    int __sctk_ethread_key_create (int *key,
				   stck_ethread_key_destr_function_t
				   destr_func)
  {
    if (sctk_ethread_key_pos < SCTK_THREAD_KEYS_MAX)
      {
	mpc_common_spinlock_lock (&sctk_ethread_key_spinlock);
	sctk_ethread_destr_func_tab[sctk_ethread_key_pos] = destr_func;
	*key = sctk_ethread_key_pos;
	sctk_ethread_key_pos++;
	mpc_common_spinlock_unlock (&sctk_ethread_key_spinlock);
	return 0;
      }
    else
      {
	return EAGAIN;
      }
  }

  static inline int __sctk_ethread_key_delete (__UNUSED__ sctk_ethread_per_thread_t *
					       cur, int key)
  {
    if ((key < SCTK_THREAD_KEYS_MAX) && (key >= 0))
      {
	if (sctk_ethread_destr_func_tab[key] != NULL)
	  {
	    sctk_ethread_destr_func_tab[key] = NULL;
	  }
	return 0;
      }
    return EINVAL;
  }

  static inline
    int __sctk_ethread_setspecific (sctk_ethread_per_thread_t * cur,
				    int key, void *val)
  {
    if ((key < SCTK_THREAD_KEYS_MAX) && (key >= 0))
      {
	cur->tls[key] = val;
	return 0;
      }
    else
      {
	return EINVAL;
      }
  }

  static inline
    void *__sctk_ethread_getspecific (sctk_ethread_per_thread_t * cur,
				      int key)
  {
    void *res;
    if ((key < SCTK_THREAD_KEYS_MAX) && (key >= 0))
      {
	res = cur->tls[key];
      }
    else
      {
	res = NULL;
      }
    return res;
  }

  /*Attribut creation */
  static inline int __sctk_ethread_attr_init (sctk_ethread_attr_intern_t *
					      attr)
  {
    sctk_ethread_attr_intern_t attr_init = SCTK_ETHREAD_ATTR_INIT;
    *attr = attr_init;
    return 0;
  }

  static inline int
    __sctk_ethread_sched_yield_vp_exit (sctk_ethread_virtual_processor_t
					* vp, sctk_ethread_per_thread_t * cur)
  {
    sctk_ethread_per_thread_t *new_task;

#ifdef SCTK_SCHED_CHECK
    __sctk_ethread_sched_yield_vp_head (vp, cur);
#endif

/*     sctk_report_death (cur); */
/*     sctk_thread_remove (); */
/*     sctk_ethread_set_status (cur, sctk_thread_undef_status); */
    
    if(cur->attr.guardsize != 0){
      int res;
      char* start_point; 

      start_point = sctk_align_ptr_to_page((char*)cur->stack);
      if((size_t)start_point < (size_t)cur->stack){
	start_point = sctk_align_ptr_to_page((char*)cur->stack + sctk_align_size_to_page(1));
      }
 
      res = mprotect(start_point,
		     sctk_align_size_to_page(cur->attr.guardsize), PROT_READ | PROT_WRITE);    
      if(res != 0){
	perror("mprotect error");
	/* not_reachable(); */
      }
    }

    new_task = vp->idle;

    sctk_assert (new_task != NULL);
    sctk_assert ((new_task->status == ethread_ready) ||
		 (new_task->status == ethread_idle) ||
		 (new_task->status == ethread_inside_polling));

    vp->current = new_task;
    sctk_nodebug ("Jump %p to %p(%p) on vp %d", cur, new_task,
		  &(new_task->ctx), vp->rank);
    sctk_swapcontext (&(cur->ctx), &(new_task->ctx));

    __sctk_ethread_sched_yield_vp_tail (vp, cur);

    return 0;
  }

  static inline void __sctk_ethread_exit (void *retval,
					  sctk_ethread_per_thread_t * th_data)
  {
    sctk_ethread_virtual_processor_t *vp;
    int i;

    SCTK_THREAD_CHECK_SIGNALS (th_data, 1);

    vp = (sctk_ethread_virtual_processor_t *) th_data->vp;
    __sctk_grab_zombie (vp);

    th_data->ret_val = retval;

    sctk_nodebug ("thread %p key liberation", th_data);
    /*key liberation */
    for (i = 0; i < sctk_ethread_key_pos; i++)
      {
	if (th_data->tls[i] != NULL)
	  if (sctk_ethread_destr_func_tab[i] != NULL)
	    {
	      sctk_ethread_destr_func_tab[i] (th_data->tls[i]);
	      th_data->tls[i] = NULL;
	    }
      }
    sctk_nodebug ("thread %p key liberation done", th_data);

    vp = (sctk_ethread_virtual_processor_t *) th_data->vp;

    sctk_nodebug ("thread %p ends", th_data);


    vp->zombie = th_data;
    vp->to_check = 1;
    __sctk_ethread_sched_yield_vp_exit (vp, th_data);
    not_reachable ();
  }

  static inline void __sctk_ethread_exit_kernel (void *retval,
						 sctk_ethread_per_thread_t
						 * th_data)
  {
    sctk_ethread_virtual_processor_t *vp;
    int i;

    SCTK_THREAD_CHECK_SIGNALS (th_data, 1);

    vp = (sctk_ethread_virtual_processor_t *) th_data->vp;
    __sctk_grab_zombie (vp);

    th_data->ret_val = retval;

    sctk_nodebug ("thread %p key liberation", th_data);
    /*key liberation */
    for (i = 0; i < sctk_ethread_key_pos; i++)
      {
	if (th_data->tls[i] != NULL)
	  if (sctk_ethread_destr_func_tab[i] != NULL)
	    {
	      sctk_ethread_destr_func_tab[i] (th_data->tls[i]);
	      th_data->tls[i] = NULL;
	    }
      }
    sctk_nodebug ("thread %p key liberation done", th_data);

    vp = (sctk_ethread_virtual_processor_t *) th_data->vp;
    vp->zombie = th_data;
    vp->to_check = 1;

    sctk_nodebug ("thread %p ends", th_data);

    vp->up = 0;
    __sctk_ethread_sched_yield_vp_exit (vp, th_data);
    not_reachable ();
  }

  static inline void __sctk_start_routine (void *arg)
  {
    sctk_ethread_per_thread_t *th_data;

    th_data = (sctk_ethread_per_thread_t *) arg;

    th_data->status = ethread_ready;
    th_data->ret_val = th_data->start_routine (th_data->arg);

    __sctk_ethread_exit (th_data->ret_val, th_data);
  }

  static inline void __sctk_start_routine_system (void *arg)
  {
    sctk_ethread_per_thread_t *th_data;

    th_data = (sctk_ethread_per_thread_t *) arg;

    th_data->status = ethread_ready;
    th_data->ret_val = th_data->start_routine (th_data->arg);

    __sctk_ethread_exit_kernel (th_data->ret_val, th_data);
  }

  static inline void __sctk_ethread_idle_task (void *arg)
  {
    sctk_ethread_per_thread_t *th_data;
    sctk_ethread_virtual_processor_t *vp;
    unsigned sctk_long_long last_timer = 0;

    th_data = (sctk_ethread_per_thread_t *) arg;
    vp = (sctk_ethread_virtual_processor_t *) th_data->vp;
    sctk_nodebug ("Idle %p starts on vp %d", th_data, vp->rank);

    th_data->status = ethread_idle;
    last_timer = ___timer_thread_ticks;

    /** ** **/
    sctk_init_idle_thread_dbg (th_data, __sctk_ethread_idle_task) ;
    /** **/

    while (___timer_thread_running)
      {
	__sctk_grab_zombie (vp);
	__sctk_ethread_sched_yield_vp_idle (vp, th_data);
	sctk_assert (vp->idle == th_data);
	if (___timer_thread_ticks != last_timer)
	  {
	    double activity;
	    double idle_activity;
	    activity = (double) vp->activity;
	    idle_activity = (double) vp->idle_activity;
	    vp->usage = activity / (activity + idle_activity);
	    vp->idle_activity = 0;
	    vp->activity = 1;
	    last_timer = ___timer_thread_ticks;
	  }
/* Idle function is called here to avoid deadlocks.
 * Actually, when calling sctk_thread_yield(), the polling
 * function is not called. */
#ifdef MPC_Message_Passing
    #ifdef __MIC__
        if ((vp->ready_queue_used == NULL) &&
           (vp->incomming_queue == NULL) &&
           (vp->ready_queue == NULL) && (vp->poll_list == NULL))
        {
            int i = 0; 
            while((vp->ready_queue_used == NULL) &&
                      (vp->incomming_queue == NULL) &&
                      (vp->ready_queue == NULL))
        {
                    _mm_delay_32(4000);
                    i++;
                    if(i >= 10000) break;
            }
         }
    else 
    {    
            if ((vp->ready_queue_used == NULL) &&
                    (vp->incomming_queue == NULL) &&
                    (vp->ready_queue == NULL) )
            {
                _mm_delay_32(400);
            }
        }     
    #else
        TODO("CHECK CONSEQUENCES OF COMMENT");
        //sctk_network_notify_idle_message();
    #endif
#endif
# if 0
    if ((vp->ready_queue_used == NULL) &&
        (vp->incomming_queue == NULL) &&
        (vp->ready_queue == NULL) && (vp->poll_list == NULL))
      {
        sctk_cpu_relax();
      } else {
      if ((vp->ready_queue_used == NULL) &&
          (vp->incomming_queue == NULL) &&
          (vp->ready_queue == NULL) )
        {
          sctk_cpu_relax();
        } 
    }
#endif

      }
    /** ** **/
    sctk_free_idle_thread_dbg (th_data) ;
    /** **/
    __sctk_grab_zombie (vp);
  }

  static inline void __sctk_ethread_kernel_idle_task (void *arg)
  {
    sctk_ethread_per_thread_t *th_data;
    sctk_ethread_virtual_processor_t *vp;
    unsigned sctk_long_long last_timer = 0;

    th_data = (sctk_ethread_per_thread_t *) arg;
    vp = (sctk_ethread_virtual_processor_t *) th_data->vp;
    sctk_nodebug ("Kernel Idle %p starts on vp %d", th_data, vp->rank);

    th_data->status = ethread_idle;
    last_timer = ___timer_thread_ticks;

    /** ** **/
    sctk_init_idle_thread_dbg (th_data, __sctk_ethread_kernel_idle_task) ;
    /** **/
    while (vp->up == 1)
      {
	__sctk_ethread_sched_yield_vp_idle (vp, th_data);
	sctk_assert (vp->current == th_data);
	sctk_assert (vp == th_data->vp);
	sctk_assert (vp->idle == th_data);
	if (___timer_thread_ticks != last_timer)
	  {
	    double activity;
	    double idle_activity;
	    activity = (double) vp->activity;
	    idle_activity = (double) vp->idle_activity;
	    vp->usage = activity / (activity + idle_activity);
	    vp->idle_activity = 0;
	    vp->activity = 1;
	    last_timer = ___timer_thread_ticks;
	  }
	kthread_usleep (20);
      }
    /** ** **/
    sctk_free_idle_thread_dbg (th_data) ;
    /** **/
    __sctk_grab_zombie (vp);
    sctk_nodebug ("Kernel Idle %p ends on vp %d", th_data, vp->rank);
  }


  sctk_ethread_virtual_processor_t *sctk_ethread_start_kernel_thread (void);
  static inline int __sctk_ethread_create (sctk_ethread_status_t status,
					   sctk_ethread_virtual_processor_t
					   * vp,
					   sctk_ethread_per_thread_t *
					   father,
					   sctk_ethread_t * threadp,
					   sctk_ethread_attr_intern_t *
					   attr,
					   void *(*start_routine) (void
								   *),
					   void *arg)
  {
    sctk_ethread_attr_intern_t default_attr = SCTK_ETHREAD_ATTR_INIT;
    sctk_ethread_per_thread_t *th_data;
    char *stack = NULL;
    size_t stack_size = 0;
    sctk_thread_data_t tmp;
    int i;

    if (arg != NULL)
      {
	tmp = *((sctk_thread_data_t *) arg);
	if (tmp.tls == NULL)
	  {
	    tmp.tls = sctk_thread_tls;
	  }
      }
    else
      {
	tmp.tls = sctk_thread_tls;
      }

    th_data = (sctk_ethread_per_thread_t *)
      __sctk_malloc (sizeof (sctk_ethread_per_thread_t), tmp.tls);
    assume (th_data != NULL);
    *threadp = th_data;
    sctk_nodebug ("Thread data %p", th_data);

    sctk_ethread_init_data (th_data);
    th_data->start_routine = start_routine;
    th_data->arg = arg;
    th_data->vp = vp;
    th_data->status = status;
    th_data->tls_mem = tmp.tls;
    th_data->dump_for_migration = 0;
    th_data->stack_size = 0;
    th_data->no_auto_enqueue = 0;
    th_data->cancel_state = 0;


    for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
      {
	th_data->tls[i] = NULL;
      }

    if (attr == NULL)
      {
	attr = &default_attr;
      }
    sctk_print_attr (attr);

    if (attr->detached == SCTK_ETHREAD_CREATE_JOINABLE)
      th_data->attr.detached = SCTK_ETHREAD_CREATE_JOINABLE;
    else
      th_data->attr.detached = SCTK_ETHREAD_CREATE_DETACHED;

    stack = attr->stack;
    stack_size = attr->stack_size;

    /*Consitency_check */
    /*    assume (attr->priority == 0); */
    if (attr->priority != 0)
      {
	__sctk_free (th_data, tmp.tls);
	return EPERM;
      }
    if (attr->scope == SCTK_ETHREAD_SCOPE_SYSTEM)
      {
	status = ethread_system;
/*
	__sctk_free (th_data, tmp.tls);
	return EINVAL;
*/
      }

    if (attr->schedpolicy != SCTK_ETHREAD_SCHED_OTHER)
      {
	__sctk_free (th_data, tmp.tls);
	return EPERM;
      }

    /*Check_inherited */
    if (attr->inheritsched == SCTK_ETHREAD_INHERIT_SCHED)
      {
	sctk_ethread_attr_intern_t father_attr;
	father_attr = father->attr;
	attr->schedpolicy = father_attr.schedpolicy;
	attr->priority = father_attr.priority;
      }

    th_data->attr = *attr;
    if ((attr->guardsize != 0) && (stack == NULL)) {
      th_data->attr.guardsize = sctk_align_size_to_page(attr->guardsize);
    } else {
      th_data->attr.guardsize = 0;
    }

    sctk_nodebug ("Thread data %p create stack", th_data);

    if (stack == NULL)
      {
	if (mpc_common_get_flags()->is_fortran == 1 && stack_size <= 0)
	  stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	else if (stack_size <= 0)
	  stack_size = SCTK_ETHREAD_STACK_SIZE;
	if (stack == NULL)
	  {
	    stack_size = sctk_align_size_to_page(stack_size);
	    stack = (char *) __sctk_malloc (stack_size + 8, tmp.tls);
	    if (stack == NULL)
	      {
		__sctk_free (th_data, tmp.tls);
		return EAGAIN;
	      }
	  }
	th_data->stack = stack;
	th_data->stack_size = stack_size;
/*
	th_data->attr.stack = stack;
	th_data->attr.stack_size = stack_size;
*/
	stack[stack_size] = 123;

      }
    else if (stack_size <= 0)
      {
	__sctk_free (th_data, tmp.tls);
	return EINVAL;
      }
    else
      {
	th_data->stack = stack;
	th_data->stack_size = stack_size;
/*
	th_data->attr.stack = stack;
	th_data->attr.stack_size = stack_size;
*/
      }

    if(th_data->attr.guardsize != 0){
      int res;
      char* start_point; 

      start_point = sctk_align_ptr_to_page((char*)stack);
      if((size_t)start_point < (size_t)stack){
	start_point = sctk_align_ptr_to_page((char*)stack + sctk_align_size_to_page(1));
      }

      fprintf(stderr,"Stack %p size %lu guards %lu point %p\n",stack,stack_size,sctk_align_size_to_page(th_data->attr.guardsize), start_point);
      
      res = mprotect(start_point,
		     sctk_align_size_to_page(th_data->attr.guardsize), PROT_NONE);      
      if(res != 0){
	perror("mprotect error");
	/* not_reachable(); */
      }
    }

    if (status == ethread_ready)
      {
	sctk_makecontext (&(th_data->ctx),
			  (void *) th_data,
			  __sctk_start_routine, stack, stack_size);
	mpc_common_spinlock_lock (&vp->spinlock);
	__sctk_ethread_enqueue_task
	  (th_data,
	   (sctk_ethread_per_thread_t **) & (vp->incomming_queue),
	   (sctk_ethread_per_thread_t **) & (vp->incomming_queue_tail));
	mpc_common_spinlock_unlock (&vp->spinlock);

      }
    else if (status == ethread_idle)
      {
	sctk_makecontext (&(th_data->ctx),
			  (void *) th_data,
			  __sctk_ethread_idle_task, stack, stack_size);
	vp->idle = th_data;
      }
    else if (status == ethread_system)
      {
	sctk_ethread_virtual_processor_t *new_kernel_vp;

	new_kernel_vp = sctk_ethread_start_kernel_thread ();

	th_data->status = ethread_ready;

	sctk_makecontext (&(th_data->ctx),
			  (void *) th_data,
			  __sctk_start_routine_system, stack, stack_size);

	th_data->vp = new_kernel_vp;

	mpc_common_spinlock_lock (&new_kernel_vp->spinlock);
	__sctk_ethread_enqueue_task
	  (th_data,
	   (sctk_ethread_per_thread_t **) & (new_kernel_vp->
					     incomming_queue),
	   (sctk_ethread_per_thread_t **) & (new_kernel_vp->
					     incomming_queue_tail));
	mpc_common_spinlock_unlock (&new_kernel_vp->spinlock);

      }
    else
      {
	verb_abort ();
      }

    sctk_print_attr (attr);
    return 0;
  }

  static inline void
    __sctk_ethread_freeze_thread_on_vp (sctk_ethread_virtual_processor_t *
					vp,
					sctk_ethread_per_thread_t * cur,
					int (*func_unlock)
					(sctk_ethread_mutex_t * lock),
					sctk_ethread_mutex_t * lock,
					volatile void **list_tmp)
  {
    volatile sctk_ethread_freeze_on_vp_t cell;
    sctk_ethread_freeze_on_vp_t *list;

    sctk_assert (vp->current == cur);

    if (*list_tmp == NULL)
      {
	cell.queue = NULL;
	cell.queue_tail = NULL;
	cell.vp = vp;
	*list_tmp = &cell;
	sctk_nodebug ("task %p create new cell %p", cur, &cell);
      }

    list = (sctk_ethread_freeze_on_vp_t *) (*list_tmp);
    sctk_nodebug ("Add task in %p %d task %p %s", list, vp->rank, cur,
		  sctk_ethread_debug_status (cur->status));
    sctk_assert (list->vp == vp);
    sctk_assert (cur->status != ethread_idle);
    sctk_assert (cur->status == ethread_ready);

    __sctk_ethread_enqueue_task (cur, &(list->queue), &(list->queue_tail));
    /*
       cur->status = ethread_block_on_vp;
     */
    func_unlock (lock);

    __sctk_ethread_sched_yield_vp_freeze (vp, cur);
    sctk_nodebug ("Add task in %p %d task %p done", list, vp->rank, cur);
  }

  static inline void
    __sctk_ethread_wake_thread_on_vp (sctk_ethread_virtual_processor_t *
				      vp, void **list_tmp)
  {
    sctk_ethread_freeze_on_vp_t *list;

    list = (sctk_ethread_freeze_on_vp_t *) (*list_tmp);

    if (list == NULL)
      return;

    if (list->vp->rank != vp->rank)
      sctk_thread_proc_migration (list->vp->rank);

    vp = list->vp;

    sctk_nodebug ("Wake tasks in %p", list);
    sctk_assert (list != NULL);

    sctk_assert_func (if (list->vp != vp)
		      {
		      sctk_error ("%d %p != %p %d", list->vp->rank,
				  list->vp, vp, vp->rank);
		      sctk_error ("list->queue %p", list->queue);
		      sctk_error ("list->queue_tail %p",
				  list->queue_tail);
		      sctk_ethread_print_task ((sctk_ethread_per_thread_t
						*)
					       vp->current);
		      sctk_assert (list->vp == vp);}
    );

    *list_tmp = NULL;

    if (vp->ready_queue != NULL)
      {
	vp->ready_queue_tail->next = list->queue;
      }
    else
      {
	vp->ready_queue = list->queue;
      }
    vp->ready_queue_tail = list->queue_tail;

  }



#ifdef __cplusplus
}
#endif
#endif
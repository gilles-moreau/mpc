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
#include <stdio.h>
#include <mpcthread_config.h>
#include <sctk_thread.h>
#include <sctk_thread_generic.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "sctk_kernel_thread.h"
#include <mpc_topology.h>
#include <string.h>
#include <mpc_common_flags.h>

#define SCTK_BLOCKING_LOCK_TABLE_SIZE    6

/***************************************/
/* THREADS                             */
/***************************************/

static __thread sctk_thread_generic_p_t *_mpc_threads_generic_self_data;

sctk_thread_generic_t _mpc_threads_generic_self()
{
	return _mpc_threads_generic_self_data;
}

void _mpc_threads_generic_self_set(sctk_thread_generic_t th)
{
	_mpc_threads_generic_self_data = th;
}

/***************************************/
/* KEYS                                */
/***************************************/

/* Globals */

static mpc_common_spinlock_t __keys_lock = SCTK_SPINLOCK_INITIALIZER;
static char __keys_in_use[SCTK_THREAD_KEYS_MAX + 1];


typedef void (*__keys_destructors_t) (void *);

static __keys_destructors_t __keys_destructors[SCTK_THREAD_KEYS_MAX + 1];

/* Interface */

static int _mpc_threads_generic_setspecific(sctk_thread_key_t __key, const void *__pointer)
{

	if(__keys_in_use[__key] == 1)
	{
		const void **keys = _mpc_threads_generic_self()->keys.keys;
		keys[__key] = __pointer;
		return 0;
	}
	else
	{
		return EINVAL;
	}
}

static void * _mpc_threads_generic_getspecific(sctk_thread_key_t __key)
{
	if(__keys_in_use[__key] == 1)
	{
		const void **keys = _mpc_threads_generic_self()->keys.keys;
		return (void *)keys[__key];
	}
	else
	{
		return NULL;
	}
}

static int _mpc_threads_generic_key_create(sctk_thread_key_t *__key,
                                    void (*__destr_function)(void *) )
{
	int i;

	mpc_common_spinlock_lock(&__keys_lock);
	for(i = 1; i < SCTK_THREAD_KEYS_MAX + 1; i++)
	{
		if(__keys_in_use[i] == 0)
		{
			__keys_in_use[i]        = 1;
			__keys_destructors[i] = __destr_function;
			*__key = i;
			break;
		}
	}
	mpc_common_spinlock_unlock(&__keys_lock);
	if(i == SCTK_THREAD_KEYS_MAX)
	{
		return EAGAIN;
	}

	return 0;
}

static int _mpc_threads_generic_key_delete(sctk_thread_key_t __key)
{
	if(__keys_in_use[__key] == 1)
	{
		if(__keys_destructors[__key] != NULL)
		{
			const void **keys = _mpc_threads_generic_self()->keys.keys;
			__keys_destructors[__key] (&keys[__key]);
			keys[__key] = NULL;
		}
	}

	return 0;
}

void _mpc_threads_generic_key_init_thread(sctk_thread_generic_keys_t *keys)
{
	int i;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		keys->keys[i] = NULL;
	}
}


/* Init and release */

static inline void __keys_init()
{
	int i;

	sctk_thread_generic_check_size(int, sctk_thread_key_t);
	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		__keys_in_use[i]        = 0;
		__keys_destructors[i] = NULL;
	}
}

static inline void __keys_delete_all(sctk_thread_generic_keys_t *keys)
{
	int i;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		if(__keys_in_use[i] == 1)
		{
			if(__keys_destructors[i] != NULL)
			{
				__keys_destructors[i] (&keys->keys[i]);
				keys->keys[i] = NULL;
			}
		}
	}
}


/***************************************/
/* MUTEX                               */
/***************************************/

/* Interface */

static int _mpc_threads_generic_mutexattr_destroy(sctk_thread_mutexattr_t *attr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	return 0;
}

static int _mpc_threads_generic_mutexattr_getpshared(sctk_thread_mutexattr_t *pattr, int *pshared)
{
	/*
	 *     ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */
	sctk_thread_generic_mutexattr_t *attr = (sctk_thread_generic_mutexattr_t*)pattr;

	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}

	(*pshared) = ( (attr->attrs >> 2) & 1);

	return 0;
}

/*
 * static int
 * sctk_thread_generic_mutexattr_getprioceiling( sctk_thread_mutexattr_t* attr,
 *                              int* prioceiling ){
 * return sctk_thread_generic_mutexes_mutexattr_getprioceiling((sctk_thread_generic_mutexattr_t *) attr,
 *                                              prioceiling );
 * }
 *
 * static int
 * sctk_thread_generic_mutexattr_setprioceiling( sctk_thread_mutexattr_t* attr,
 *                              int prioceiling ){
 * return sctk_thread_generic_mutexes_mutexattr_setprioceiling((sctk_thread_generic_mutexattr_t *) attr,
 *                                              prioceiling );
 * }
 *
 * static int
 * sctk_thread_generic_mutexattr_getprotocol( sctk_thread_mutexattr_t* attr,
 *                              int* protocol ){
 * return sctk_thread_generic_mutexes_mutexattr_getprotocol((sctk_thread_generic_mutexattr_t *) attr,
 *                                              protocol );
 * }
 *
 * static int
 * sctk_thread_generic_mutexattr_setprotocol( sctk_thread_mutexattr_t* attr,
 *                              int protocol ){
 * return sctk_thread_generic_mutexes_mutexattr_setprotocol((sctk_thread_generic_mutexattr_t *) attr,
 *                                              protocol );
 * }
 */

static int _mpc_threads_generic_mutexattr_gettype(sctk_thread_mutexattr_t *pattr, int *kind)
{
	/*
	 *     ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */
	sctk_thread_generic_mutexattr_t *attr = (sctk_thread_generic_mutexattr_t*)pattr;


	if(attr == NULL || kind == NULL)
	{
		return EINVAL;
	}

	(*kind) = (attr->attrs & 3);

	return 0;
}

static int _mpc_threads_generic_mutexattr_init(sctk_thread_mutexattr_t *pattr)
{
	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 * ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize
	 *       the read-write lock attributes object
	 */
	sctk_thread_generic_mutexattr_t *attr = (sctk_thread_generic_mutexattr_t*)pattr;


	if(attr == NULL)
	{
		return EINVAL;
	}

	attr->attrs  = ( (attr->attrs & ~3) | (0 & 3) );
	attr->attrs &= ~(1 << 2);

	return 0;}

static int _mpc_threads_generic_mutexattr_setpshared(sctk_thread_mutexattr_t *pattr, int pshared)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */
	sctk_thread_generic_mutexattr_t *attr = (sctk_thread_generic_mutexattr_t*)pattr;


	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE && pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}

	int ret = 0;
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		attr->attrs |= (1 << 2);
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared mutexes\n");
		ret = ENOTSUP;
	}
	else
	{
		attr->attrs &= ~(1 << 2);
	}

	return ret;
}

static int _mpc_threads_generic_mutexattr_settype(sctk_thread_mutexattr_t *pattr, int kind)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	sctk_thread_generic_mutexattr_t *attr = (sctk_thread_generic_mutexattr_t*)pattr;

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(kind != PTHREAD_MUTEX_NORMAL && kind != PTHREAD_MUTEX_ERRORCHECK &&
	   kind != PTHREAD_MUTEX_RECURSIVE && kind != PTHREAD_MUTEX_DEFAULT)
	{
		return EINVAL;
	}

	attr->attrs = (attr->attrs & ~3) | (kind & 3);

	return 0;
}

static int _mpc_threads_generic_mutex_destroy(sctk_thread_mutex_t *plock)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 *    EBUSY  The specified lock is currently owned by a thread or
	 *               another thread is currently using the mutex in a cond
	 */
	sctk_thread_generic_mutex_t *lock = (sctk_thread_generic_mutex_t*)plock;


	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->owner != NULL)
	{
		return EBUSY;
	}

	return 0;}

static int _mpc_threads_generic_mutex_init(sctk_thread_mutex_t *lock,
                               		  const sctk_thread_mutexattr_t *pattr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 * EBUSY  |>NOT IMPLEMENTED<| The specified lock has already been
	 *               initialized
	 *    EAGAIN |>NOT IMPLEMENTED<| The system lacked the necessary
	 *               resources (other than memory) to initialize another mutex
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to
	 *               initialize the mutex
	 *    EPERM  |>NOT IMPLEMENTED<| The caller does not have the
	 *               privilege to perform the operation
	 */

	sctk_thread_generic_mutexattr_t *attr = (sctk_thread_generic_mutexattr_t*)pattr;

	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	sctk_thread_generic_mutex_t  local_lock = SCTK_THREAD_GENERIC_MUTEX_INIT;
	sctk_thread_generic_mutex_t *local_ptr  = &local_lock;

	if(attr != NULL)
	{
		if( ( (attr->attrs >> 2) & 1) == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared mutexes\n");
			ret = ENOTSUP;
		}
		local_ptr->type = (attr->attrs & 3);
	}

	memcpy(lock, &local_lock, sizeof(sctk_thread_generic_mutex_t));

	return ret;
}

static int _mpc_threads_generic_mutex_lock(sctk_thread_mutex_t *plock)
{
	/*
	 *    ERRORS:
	 *    EINVAL  The value specified for the argument is not correct
	 *    EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum
	 *                number of recursive locks for mutex has been exceeded
	 *    EDEADLK The current thread already owns the mutex
	 */

	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);
	sctk_thread_generic_mutex_t *lock = (sctk_thread_generic_mutex_t*)plock;


	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	sctk_thread_generic_mutex_cell_t cell;
	void **tmp = (void **)sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;
	mpc_common_spinlock_lock(&(lock->lock) );
	if(lock->owner == NULL)
	{
		lock->owner = sched;
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
		{
			lock->nb_call++;
		}
		mpc_common_spinlock_unlock(&(lock->lock) );
		// We can force sched_yield here to increase calls to the priority scheduler
		// sctk_thread_generic_sched_yield(sched);
		return ret;
	}
	else
	{
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE &&
		   lock->owner == sched)
		{
			lock->nb_call++;
			mpc_common_spinlock_unlock(&(lock->lock) );
			// We can force sched_yield here to increase calls to the priority
			// scheduler
			// sctk_thread_generic_sched_yield(sched);
			return ret;
		}
		if(lock->type == SCTK_THREAD_MUTEX_ERRORCHECK &&
		   lock->owner == sched)
		{
			ret = EDEADLK;
			mpc_common_spinlock_unlock(&(lock->lock) );
			return ret;
		}

		cell.sched = sched;
		DL_APPEND(lock->blocked, &cell);
		tmp[sctk_thread_generic_mutex] = (void *)lock;

		sctk_thread_generic_thread_status(sched, sctk_thread_generic_blocked);
		sctk_nodebug("WAIT MUTEX LOCK sleep %p", sched);
		sctk_thread_generic_register_spinlock_unlock(sched, &(lock->lock) );
		sctk_thread_generic_sched_yield(sched);
		tmp[sctk_thread_generic_mutex] = NULL;
	}
	return ret;
}

static int _mpc_threads_generic_mutex_trylock(sctk_thread_mutex_t *plock)
{
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);
	sctk_thread_generic_mutex_t *lock = (sctk_thread_generic_mutex_t*)plock;

	/*
	 *    ERRORS:
	 *    EINVAL  The value specified for the argument is not correct
	 *    EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum
	 *                number of recursive locks for mutex has been exceeded
	 *    EBUSY   the mutex is already owned by another thread or the calling thread
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	if(/*mpc_common_spinlock_trylock(&(lock->lock)) == 0 */ 1)
	{
		mpc_common_spinlock_lock(&(lock->lock) );
		if(lock->owner == NULL)
		{
			lock->owner = sched;
			if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
			{
				lock->nb_call++;
			}
			mpc_common_spinlock_unlock(&(lock->lock) );
			return ret;
		}
		else
		{
			if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE &&
			   lock->owner == sched)
			{
				lock->nb_call++;
				mpc_common_spinlock_unlock(&(lock->lock) );
				return ret;
			}
			ret = EBUSY;
			mpc_common_spinlock_unlock(&(lock->lock) );
		}
	}
	else
	{
		ret = EBUSY;
	}

	return ret;
}

static int _mpc_threads_generic_mutex_timedlock(sctk_thread_mutex_t *plock,
                                    	       const struct timespec *restrict time)
{
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);
	sctk_thread_generic_mutex_t *lock = (sctk_thread_generic_mutex_t*)plock;

	/*
	 *    ERRORS:
	 *    EINVAL    The value specified by argument lock does not refer to an initialized
	 *                      mutex object, or the abs_timeout nanosecond value is less than zero or
	 *                      greater than or equal to 1000 million
	 *    ETIMEDOUT The lock could not be acquired in specified time
	 *    EAGAIN    |> NOT IMPLEMENTED <| The read lock could not be acquired because the
	 *                      maximum number of recursive locks for mutex has been exceeded
	 */

	if(lock == NULL || time == NULL)
	{
		return EINVAL;
	}
	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		return EINVAL;
	}

	int             ret = 0;
	struct timespec t_current;

	do
	{
		if(mpc_common_spinlock_trylock(&(lock->lock) ) == 0)
		{
			if(lock->owner == NULL)
			{
				lock->owner = sched;
				if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
				{
					lock->nb_call++;
				}
			}
			else
			{
				if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE &&
				   lock->owner == sched)
				{
					lock->nb_call++;
				}
				ret = EBUSY;
			}
		}
		else
		{
			ret = EBUSY;
		}
		mpc_common_spinlock_unlock(&(lock->lock) );
		clock_gettime(CLOCK_REALTIME, &t_current);
	} while(ret != 0 && (t_current.tv_sec < time->tv_sec ||
	                     (t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec) ) );

	if(ret != 0)
	{
		return ETIMEDOUT;
	}

	return ret;
}

static int _mpc_threads_generic_mutex_spinlock(sctk_thread_mutex_t *plock)
{
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);
	sctk_thread_generic_mutex_t *lock = (sctk_thread_generic_mutex_t*)plock;

	/*
	 *    ERRORS:
	 *    EINVAL  The value specified for the argument is not correct
	 *    EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum
	 *                number of recursive locks for mutex has been exceeded
	 *    EDEADLK The current thread already owns the mutex
	 */

	/* TODO: FIX BUG when called by "sctk_thread_lock_lock" (same issue with mutex
	 * unlock when called by "sctk_thread_lock_unlock") with sched null beacause calling
	 * functions only have one argument instead of two*/
	if(lock == NULL /*|| (lock->m_attrs & 1) != 1*/)
	{
		return EINVAL;
	}

	int ret = 0;
	sctk_thread_generic_mutex_cell_t cell;

	mpc_common_spinlock_lock(&(lock->lock) );
	if(lock->owner == NULL)
	{
		lock->owner   = sched;
		lock->nb_call = 1;
		mpc_common_spinlock_unlock(&(lock->lock) );
		return ret;
	}
	else
	{
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
		{
			lock->nb_call++;
			mpc_common_spinlock_unlock(&(lock->lock) );
			return ret;
		}
		if(lock->type == SCTK_THREAD_MUTEX_ERRORCHECK)
		{
			if(lock->owner == sched)
			{
				ret = EDEADLK;
				mpc_common_spinlock_unlock(&(lock->lock) );
				return ret;
			}
		}

		cell.sched = sched;
		DL_APPEND(lock->blocked, &cell);

		mpc_common_spinlock_unlock(&(lock->lock) );
		do
		{
			sctk_thread_generic_sched_yield(sched);
		} while(lock->owner != sched);
	}
	return ret;
}

static int _mpc_threads_generic_mutex_unlock(sctk_thread_mutex_t *plock)
{
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);
	sctk_thread_generic_mutex_t *lock = (sctk_thread_generic_mutex_t*)plock;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified by argument lock does not refer to an initialized mutex
	 *    EAGAIN |> NOT IMPLEMENTED <| The read lock could not be acquired because the
	 *                maximum number of recursive locks for mutex has been exceeded
	 *    EPERM  The current thread does not own the mutex
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}

	if(lock->owner != sched)
	{
		return EPERM;
	}
	if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
	{
		if(lock->nb_call > 1)
		{
			lock->nb_call--;
			return 0;
		}
	}

	mpc_common_spinlock_lock(&(lock->lock) );
	{
		sctk_thread_generic_mutex_cell_t *head;
		head = lock->blocked;
		if(head == NULL)
		{
			lock->owner   = NULL;
			lock->nb_call = 0;
		}
		else
		{
			lock->owner   = head->sched;
			lock->nb_call = 1;
			DL_DELETE(lock->blocked, head);
			if(head->sched->status != sctk_thread_generic_running)
			{
				sctk_nodebug("ADD MUTEX UNLOCK wake %p", head->sched);
				sctk_thread_generic_wake(head->sched);
			}
		}
	}
	mpc_common_spinlock_unlock(&(lock->lock) );
	return 0;
}

/* Init */

void __mutex_init()
{
	sctk_thread_generic_check_size(sctk_thread_generic_mutex_t, sctk_thread_mutex_t);
	sctk_thread_generic_check_size(sctk_thread_generic_mutexattr_t, sctk_thread_mutexattr_t);

	{
		static sctk_thread_generic_mutex_t loc  = SCTK_THREAD_GENERIC_MUTEX_INIT;
		static sctk_thread_mutex_t         glob = SCTK_THREAD_MUTEX_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(sctk_thread_generic_mutex_t) ) == 0);
	}
}


/***************************************/
/* CONDITIONS                          */
/***************************************/

static int _mpc_threads_generic_condattr_destroy(sctk_thread_condattr_t *pattr)
{
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;
	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	return 0;}

static int _mpc_threads_generic_condattr_getpshared(sctk_thread_condattr_t *pattr,
                                        	    int *pshared)
{
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}

	(*pshared) = ( (attr->attrs >> 2) & 1);

	return 0;
}

static int _mpc_threads_generic_condattr_init(sctk_thread_condattr_t *pattr)
{
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the condition
	 *               variable attributes object
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	attr->attrs &= ~(1 << 2);
	attr->attrs  = ( (attr->attrs & ~3) | (0 & 3) );

	return 0;}

static int _mpc_threads_generic_condattr_setpshared(sctk_thread_condattr_t *pattr,
                                       		   int pshared)
{
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE && pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}

	int ret = 0;
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		attr->attrs |= (1 << 2);
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared conds\n");
		ret = ENOTSUP;
	}
	else
	{
		attr->attrs &= ~(1 << 2);
	}

	return ret;
}

static int _mpc_threads_generic_condattr_setclock(sctk_thread_condattr_t *pattr,
                                      		 clockid_t clock_id)
{
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC &&
	   clock_id != CLOCK_PROCESS_CPUTIME_ID && clock_id != CLOCK_THREAD_CPUTIME_ID)
	{
		return EINVAL;
	}
	if(clock_id == CLOCK_PROCESS_CPUTIME_ID || clock_id == CLOCK_THREAD_CPUTIME_ID)
	{
		return EINVAL;
	}

	attr->attrs = ( (attr->attrs & ~3) | (clock_id & 3) );

	return 0;
}

static int _mpc_threads_generic_condattr_getclock(sctk_thread_condattr_t *pattr,
                                      		 clockid_t *clock_id)
{
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL || clock_id == NULL)
	{
		return EINVAL;
	}

	(*clock_id) = (attr->attrs & 3);

	return 0;
}

static int _mpc_threads_generic_cond_destroy(sctk_thread_cond_t *plock)
{
	sctk_thread_generic_cond_t * lock = (sctk_thread_generic_cond_t*)plock;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 *    EBUSY  The lock argument is currently used
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->blocked != NULL)
	{
		return EBUSY;
	}

	lock->clock_id = -1;

	return 0;
}

static int _mpc_threads_generic_cond_init(sctk_thread_cond_t *plock,
                              		  const sctk_thread_condattr_t *pattr)
{
	sctk_thread_generic_cond_t * lock = (sctk_thread_generic_cond_t*)plock;
	sctk_thread_generic_condattr_t *attr = (sctk_thread_generic_condattr_t *)pattr;
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);

	/*
	 *    ERRORS:
	 *    EAGAIN |>NOT IMPLEMENTED<| The system lacked the necessary resources
	 *               (other than memory) to initialize another condition variable
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize
	 *               the condition variable
	 *    EBUSY  |>NOT IMPLEMENTED<| The argument lock is already initialize
	 *               and must be destroy before reinitializing it
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	sctk_thread_generic_cond_t  local = SCTK_THREAD_GENERIC_COND_INIT;
	sctk_thread_generic_cond_t *ptrl  = &local;

	if(attr != NULL)
	{
		if( ( (attr->attrs >> 2) & 1) == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared conds\n");
			ret = ENOTSUP;
		}
		ptrl->clock_id = (attr->attrs & 3);
	}

	(*lock) = local;

	return ret;
}

static int _mpc_threads_generic_cond_wait(sctk_thread_cond_t *pcond,
                              		  sctk_thread_mutex_t *pmutex)
{
	sctk_thread_generic_cond_t * cond = (sctk_thread_generic_cond_t*)pcond;
	sctk_thread_generic_mutex_t * mutex = (sctk_thread_generic_mutex_t*)pmutex;
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL    The value specified by argument cond, mutex, or time is invalid
	 *                      or different mutexes were supplied for concurrent pthread_cond_wait()
	 *                      operations on the same condition variable
	 *    EPERM     The mutex was not owned by the current thread
	 */

	if(cond == NULL || mutex == NULL)
	{
		return EINVAL;
	}

	/* test cancel */
	sctk_thread_generic_check_signals(0);

	int ret = 0;
	sctk_thread_generic_cond_cell_t cell;
	void **tmp = (void **)sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;
	mpc_common_spinlock_lock(&(cond->lock) );
	if(cond->blocked == NULL)
	{
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	else
	{
		if(cond->blocked->binded != mutex)
		{
			return EINVAL;
		}
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	_mpc_threads_generic_mutex_unlock((sctk_thread_mutex_t*)mutex);
	cell.sched = sched;
	DL_APPEND(cond->blocked, &cell);
	tmp[sctk_thread_generic_cond] = (void *)cond;

	sctk_nodebug("WAIT on %p", sched);

	sctk_thread_generic_thread_status(sched, sctk_thread_generic_blocked);
	sctk_thread_generic_register_spinlock_unlock(sched, &(cond->lock) );
	sctk_thread_generic_sched_yield(sched);

	tmp[sctk_thread_generic_cond] = NULL;
	_mpc_threads_generic_mutex_lock((sctk_thread_mutex_t*)mutex);

	/* test cancel */
	sctk_thread_generic_check_signals(0);
	return ret;
}

static int _mpc_threads_generic_cond_signal(sctk_thread_cond_t *pcond)
{
	sctk_thread_generic_cond_t * cond = (sctk_thread_generic_cond_t*)pcond;
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(cond == NULL)
	{
		return EINVAL;
	}

	sctk_thread_generic_cond_cell_t *task;
	mpc_common_spinlock_lock(&(cond->lock) );
	task = cond->blocked;
	if(task != NULL)
	{
		DL_DELETE(cond->blocked, task);
		sctk_thread_generic_wake(task->sched);
	}
	mpc_common_spinlock_unlock(&(cond->lock) );
	return 0;
}

struct __timedwait_arg_s
{
	const struct timespec *restrict      timedout;
	int *                                timeout;
	sctk_thread_generic_scheduler_t *    sched;
	sctk_thread_generic_cond_t *restrict cond;
};

void __timedwait_arg_init(
        struct __timedwait_arg_s *arg,
        const struct timespec *restrict timedout, int *timeout,
        sctk_thread_generic_scheduler_t *sched,
        sctk_thread_generic_cond_t *restrict cond)
{
	arg->timedout = timedout;
	arg->timeout  = timeout;
	arg->sched    = sched;
	arg->cond     = cond;
}

void __timedwait_task_init(sctk_thread_generic_task_t *task,
                                           volatile int *data, int value,
                                           void (*func)(void *), void *arg)
{
	task->is_blocking = 0;
	task->data        = data;
	task->value       = value;
	task->func        = func;
	task->arg         = arg;
	task->sched       = NULL;
	task->free_func   = sctk_free;
	task->prev        = NULL;
	task->next        = NULL;
}

static void __timedwait_test_timeout(void *args)
{
	struct __timedwait_arg_s *arg       = (struct __timedwait_arg_s *)args;
	sctk_thread_generic_cond_cell_t *  lcell     = NULL;
	sctk_thread_generic_cond_cell_t *  lcell_tmp = NULL;
	struct timespec t_current;

	if(mpc_common_spinlock_trylock(&(arg->cond->lock) ) == 0)
	{
		clock_gettime(arg->cond->clock_id, &t_current);

		if(t_current.tv_sec > arg->timedout->tv_sec ||
		   (t_current.tv_sec == arg->timedout->tv_sec && t_current.tv_nsec > arg->timedout->tv_nsec) )
		{
			DL_FOREACH_SAFE(arg->cond->blocked, lcell, lcell_tmp)
			{
				if(lcell->sched == arg->sched)
				{
					*(arg->timeout) = 1;
					DL_DELETE(arg->cond->blocked, lcell);
					lcell->next = NULL;
					sctk_thread_generic_wake(lcell->sched);
				}
			}
		}
		mpc_common_spinlock_unlock(&(arg->cond->lock) );
	}
}

static int _mpc_threads_generic_cond_timedwait(sctk_thread_cond_t *pcond,
                                               sctk_thread_mutex_t *pmutex,
                                               const struct timespec *restrict time)
{
	sctk_thread_generic_cond_t * cond = (sctk_thread_generic_cond_t*)pcond;
	sctk_thread_generic_mutex_t * mutex = (sctk_thread_generic_mutex_t*)pmutex;
	sctk_thread_generic_scheduler_t *sched = &(_mpc_threads_generic_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL    The value specified by argument cond, mutex, or time is invalid
	 *                  or different mutexes were supplied for concurrent pthread_cond_timedwait() or
	 *                      pthread_cond_wait() operations on the same condition variable
	 *    EPERM     The mutex was not owned by the current thread
	 *    ETIMEDOUT The time specified by time has passed
	 *    ENOMEM    Lack of memory
	 */

	if(cond == NULL || mutex == NULL || time == NULL)
	{
		return EINVAL;
	}
	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		return EINVAL;
	}
	if(sched != mutex->owner)
	{
		return EPERM;
	}

	/* test cancel */
	sctk_thread_generic_check_signals(0);

	int             timeout = 0;
	struct timespec t_current;
	sctk_thread_generic_thread_status_t *status;
	sctk_thread_generic_cond_cell_t      cell;
	sctk_thread_generic_task_t *         cond_timedwait_task;
	struct __timedwait_arg_s *  args;
	void **tmp = (void **)sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;

	cond_timedwait_task = (sctk_thread_generic_task_t *)sctk_malloc(sizeof(sctk_thread_generic_task_t) );
	if(cond_timedwait_task == NULL)
	{
		return ENOMEM;
	}
	args = (struct __timedwait_arg_s *)sctk_malloc(sizeof(struct __timedwait_arg_s) );
	if(args == NULL)
	{
		return ENOMEM;
	}

	mpc_common_spinlock_lock(&(cond->lock) );
	if(cond->blocked == NULL)
	{
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	else
	{
		if(cond->blocked->binded != mutex)
		{
			return EINVAL;
		}
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	_mpc_threads_generic_mutex_unlock((sctk_thread_mutex_t*)mutex);
	cell.sched = sched;
	DL_APPEND(cond->blocked, &cell);
	tmp[sctk_thread_generic_cond] = (void *)cond;

	sctk_nodebug("WAIT on %p", sched);

	status = (sctk_thread_generic_thread_status_t *)&(sched->status);

	__timedwait_arg_init(args,
	                                      time, &timeout, sched, cond);

	__timedwait_task_init(cond_timedwait_task,
				(volatile int *)status,
				sctk_thread_generic_running,
				__timedwait_test_timeout,
				(void *)args);

	clock_gettime(cond->clock_id, &t_current);
	if(t_current.tv_sec > time->tv_sec ||
	   (t_current.tv_sec == time->tv_sec && t_current.tv_nsec > time->tv_nsec) )
	{
		sctk_free(args);
		sctk_free(cond_timedwait_task);
		DL_DELETE(cond->blocked, &cell);
		tmp[sctk_thread_generic_cond] = NULL;
		_mpc_threads_generic_mutex_lock((sctk_thread_mutex_t*)mutex);
		mpc_common_spinlock_unlock(&(cond->lock) );
		return ETIMEDOUT;
	}

	sctk_thread_generic_thread_status(sched, sctk_thread_generic_blocked);
	sctk_thread_generic_register_spinlock_unlock(sched, &(cond->lock) );
	sctk_thread_generic_add_task(cond_timedwait_task);
	sctk_thread_generic_sched_yield(sched);

	tmp[sctk_thread_generic_cond] = NULL;
	_mpc_threads_generic_mutex_lock((sctk_thread_mutex_t*)mutex);

	/* test cancel */
	sctk_thread_generic_check_signals(0);
	if(timeout)
	{
		return ETIMEDOUT;
	}

	return 0;
}

static int _mpc_threads_generic_cond_broadcast(sctk_thread_cond_t *pcond)
{
	sctk_thread_generic_cond_t * cond = (sctk_thread_generic_cond_t*)pcond;
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(cond == NULL)
	{
		return EINVAL;
	}

	sctk_thread_generic_cond_cell_t *task;
	sctk_thread_generic_cond_cell_t *task_tmp;
	mpc_common_spinlock_lock(&(cond->lock) );
	DL_FOREACH_SAFE(cond->blocked, task, task_tmp)
	{
		DL_DELETE(cond->blocked, task);
		sctk_nodebug("ADD BCAST cond wake %p from %p", task->sched, sched);
		sctk_thread_generic_wake(task->sched);
	}
	mpc_common_spinlock_unlock(&(cond->lock) );
	return 0;
}

/* Init */


static inline void __cond_init()
{
	sctk_thread_generic_check_size(sctk_thread_generic_cond_t, sctk_thread_cond_t);
	sctk_thread_generic_check_size(sctk_thread_generic_condattr_t, sctk_thread_condattr_t);

	{
		static sctk_thread_generic_cond_t loc  = SCTK_THREAD_GENERIC_COND_INIT;
		static sctk_thread_cond_t         glob = SCTK_THREAD_COND_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(sctk_thread_generic_cond_t) ) == 0);
	}
}


/***************************************/
/* SEMAPHORES                          */
/***************************************/

static int
sctk_thread_generic_sem_init(sctk_thread_sem_t *sem, int pshared, unsigned int value)
{
	return sctk_thread_generic_sems_sem_init( (sctk_thread_generic_sem_t *)sem,
	                                          pshared, value);
}

static int
sctk_thread_generic_sem_wait(sctk_thread_sem_t *sem)
{
	return sctk_thread_generic_sems_sem_wait( (sctk_thread_generic_sem_t *)sem,
	                                          &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_trywait(sctk_thread_sem_t *sem)
{
	return sctk_thread_generic_sems_sem_trywait( (sctk_thread_generic_sem_t *)sem,
	                                             &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_timedwait(sctk_thread_sem_t *sem,
                                  const struct timespec *time)
{
	return sctk_thread_generic_sems_sem_timedwait( (sctk_thread_generic_sem_t *)sem,
	                                               time,
	                                               &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_post(sctk_thread_sem_t *sem)
{
	return sctk_thread_generic_sems_sem_post( (sctk_thread_generic_sem_t *)sem,
	                                          &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_getvalue(sctk_thread_sem_t *sem, int *sval)
{
	return sctk_thread_generic_sems_sem_getvalue( (sctk_thread_generic_sem_t *)sem,
	                                              sval);
}

static int
sctk_thread_generic_sem_destroy(sctk_thread_sem_t *sem)
{
	return sctk_thread_generic_sems_sem_destroy( (sctk_thread_generic_sem_t *)sem);
}

static sctk_thread_sem_t *
sctk_thread_generic_sem_open(const char *name, int oflag, ...)
{
	if(oflag & O_CREAT)
	{
		va_list args;
		va_start(args, oflag);
		mode_t       mode  = va_arg(args, mode_t);
		unsigned int value = va_arg(args, int);
		va_end(args);
		return (sctk_thread_sem_t *)sctk_thread_generic_sems_sem_open(name, oflag, mode, value);
	}
	else
	{
		return (sctk_thread_sem_t *)sctk_thread_generic_sems_sem_open(name, oflag);
	}
}

static int
sctk_thread_generic_sem_close(sctk_thread_sem_t *sem)
{
	return sctk_thread_generic_sems_sem_close( (sctk_thread_generic_sem_t *)sem);
}

static int
sctk_thread_generic_sem_unlink(const char *name)
{
	return sctk_thread_generic_sems_sem_unlink(name);
}


/***************************************/
/* READ/WRITE LOCKS                    */
/***************************************/

static int
sctk_thread_generic_rwlockattr_destroy(sctk_thread_rwlockattr_t *attr)
{
	return sctk_thread_generic_rwlocks_rwlockattr_destroy( (sctk_thread_generic_rwlockattr_t *)attr);
}

static int
sctk_thread_generic_rwlockattr_getpshared(const sctk_thread_rwlockattr_t *attr, int *val)
{
	return sctk_thread_generic_rwlocks_rwlockattr_getpshared( (sctk_thread_generic_rwlockattr_t *)attr,
	                                                          val);
}

static int
sctk_thread_generic_rwlockattr_init(sctk_thread_rwlockattr_t *attr)
{
	return sctk_thread_generic_rwlocks_rwlockattr_init( (sctk_thread_generic_rwlockattr_t *)attr);
}

static int
sctk_thread_generic_rwlockattr_setpshared(sctk_thread_rwlockattr_t *attr, int val)
{
	return sctk_thread_generic_rwlocks_rwlockattr_setpshared( (sctk_thread_generic_rwlockattr_t *)attr,
	                                                          val);
}

static int
sctk_thread_generic_rwlock_destroy(sctk_thread_rwlock_t *lock)
{
	return sctk_thread_generic_rwlocks_rwlock_destroy( (sctk_thread_generic_rwlock_t *)lock);
}

static int
sctk_thread_generic_rwlock_init(sctk_thread_rwlock_t *lock, sctk_thread_rwlockattr_t *attr)
{
	return sctk_thread_generic_rwlocks_rwlock_init( (sctk_thread_generic_rwlock_t *)lock,
	                                                (sctk_thread_generic_rwlockattr_t *)attr, &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_rdlock(sctk_thread_rwlock_t *lock)
{
	return sctk_thread_generic_rwlocks_rwlock_rdlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                  &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_wrlock(sctk_thread_rwlock_t *lock)
{
	return sctk_thread_generic_rwlocks_rwlock_wrlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                  &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_tryrdlock(sctk_thread_rwlock_t *lock)
{
	return sctk_thread_generic_rwlocks_rwlock_tryrdlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                     &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_trywrlock(sctk_thread_rwlock_t *lock)
{
	return sctk_thread_generic_rwlocks_rwlock_trywrlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                     &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_unlock(sctk_thread_rwlock_t *lock)
{
	return sctk_thread_generic_rwlocks_rwlock_unlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                  &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_timedrdlock(sctk_thread_rwlock_t *lock, const struct timespec *restrict time)
{
	return sctk_thread_generic_rwlocks_rwlock_timedrdlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                       time, &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_timedwrlock(sctk_thread_rwlock_t *lock, const struct timespec *restrict time)
{
	return sctk_thread_generic_rwlocks_rwlock_timedwrlock( (sctk_thread_generic_rwlock_t *)lock,
	                                                       time, &(_mpc_threads_generic_self()->sched) );
}

/*
 * Defined but not used
 *
 * static int
 * sctk_thread_generic_rwlockattr_setkind_np( sctk_thread_rwlockattr_t* attr, int pref ){
 * return sctk_thread_generic_rwlocks_rwlockattr_setkind_np( (sctk_thread_generic_rwlockattr_t*) attr,
 *                                              pref );
 * }
 *
 * static int
 * sctk_thread_generic_rwlockattr_getkind_np( sctk_thread_rwlockattr_t* attr, int* pref ){
 * return sctk_thread_generic_rwlocks_rwlockattr_getkind_np( (sctk_thread_generic_rwlockattr_t*) attr,
 *                                              pref );
 * }
 */

/***************************************/
/* THREAD BARRIER                      */
/***************************************/

static int
sctk_thread_generic_barrierattr_destroy(sctk_thread_barrierattr_t *attr)
{
	return sctk_thread_generic_barriers_barrierattr_destroy( (sctk_thread_generic_barrierattr_t *)attr);
}

static int
sctk_thread_generic_barrierattr_init(sctk_thread_barrierattr_t *attr)
{
	return sctk_thread_generic_barriers_barrierattr_init( (sctk_thread_generic_barrierattr_t *)attr);
}

static int
sctk_thread_generic_barrierattr_getpshared(const sctk_thread_barrierattr_t *restrict attr,
                                           int *restrict pshared)
{
	return sctk_thread_generic_barriers_barrierattr_getpshared( (sctk_thread_generic_barrierattr_t *)attr,
	                                                            pshared);
}

static int
sctk_thread_generic_barrierattr_setpshared(sctk_thread_barrierattr_t *attr,
                                           int pshared)
{
	return sctk_thread_generic_barriers_barrierattr_setpshared( (sctk_thread_generic_barrierattr_t *)attr,
	                                                            pshared);
}

static int
sctk_thread_generic_barrier_destroy(sctk_thread_barrier_t *barrier)
{
	return sctk_thread_generic_barriers_barrier_destroy( (sctk_thread_generic_barrier_t *)barrier);
}

static int
sctk_thread_generic_barrier_init(sctk_thread_barrier_t *restrict barrier,
                                 const sctk_thread_barrierattr_t *restrict attr, unsigned count)
{
	return sctk_thread_generic_barriers_barrier_init( (sctk_thread_generic_barrier_t *)barrier,
	                                                  (sctk_thread_generic_barrierattr_t *)attr, count);
}

static int
sctk_thread_generic_barrier_wait(sctk_thread_barrier_t *barrier)
{
	return sctk_thread_generic_barriers_barrier_wait( (sctk_thread_generic_barrier_t *)barrier,
	                                                  &(_mpc_threads_generic_self()->sched) );
}

/***************************************/
/* THREAD SPINLOCK                     */
/***************************************/

static int
sctk_thread_generic_spin_destroy(sctk_thread_spinlock_t *spinlock)
{
	return sctk_thread_generic_spinlocks_spin_destroy( (sctk_thread_generic_spinlock_t *)spinlock);
}

static int
sctk_thread_generic_spin_init(sctk_thread_spinlock_t *spinlock, int pshared)
{
	return sctk_thread_generic_spinlocks_spin_init( (sctk_thread_generic_spinlock_t *)spinlock,
	                                                pshared);
}

static int
sctk_thread_generic_spin_lock(sctk_thread_spinlock_t *spinlock)
{
	return sctk_thread_generic_spinlocks_spin_lock( (sctk_thread_generic_spinlock_t *)spinlock,
	                                                &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_spin_trylock(sctk_thread_spinlock_t *spinlock)
{
	return sctk_thread_generic_spinlocks_spin_trylock( (sctk_thread_generic_spinlock_t *)spinlock,
	                                                   &(_mpc_threads_generic_self()->sched) );
}

static int
sctk_thread_generic_spin_unlock(sctk_thread_spinlock_t *spinlock)
{
	return sctk_thread_generic_spinlocks_spin_unlock( (sctk_thread_generic_spinlock_t *)spinlock,
	                                                  &(_mpc_threads_generic_self()->sched) );
}

/***************************************/
/* THREAD SIGNALS                      */
/***************************************/

static sigset_t sctk_thread_default_set;
static int      main_thread_sigs_initialized = 0;
int             errno;

static void sctk_thread_generic_init_default_sigset()
{
#ifndef WINDOWS_SYS
	sigset_t set;
	sigemptyset(&set);

	if(!main_thread_sigs_initialized)
	{
		main_thread_sigs_initialized++;
		kthread_sigmask(SIG_SETMASK, &set, &sctk_thread_default_set);
		kthread_sigmask(SIG_SETMASK, &sctk_thread_default_set, NULL);
	}
	else
	{
		kthread_sigmask(SIG_SETMASK, (sigset_t *)&(_mpc_threads_generic_self()->attr.thread_sigset), &set);
		kthread_sigmask(SIG_SETMASK, &set, &sctk_thread_default_set);
	}
#endif
}

static void
sctk_thread_generic_attr_init_sigs(const sctk_thread_generic_attr_t *attr)
{
	int i;

	for(i = 0; i < SCTK_NSIG; i++)
	{
		attr->ptr->thread_sigpending[i] = 0;
	}

	attr->ptr->thread_sigset = sctk_thread_default_set;
	sigemptyset( (sigset_t *)&(attr->ptr->sa_sigset_mask) );
}

static void sctk_thread_generic_treat_signals(sctk_thread_generic_t threadp)
{
#ifndef WINDOWS_SYS
	sctk_thread_generic_p_t *th = threadp;
	int        i, done = 0;
	static int nb_inner_calls = 0;
	sigset_t   set, current_set;
	sigemptyset(&set);
	if(nb_inner_calls == 0)
	{
		th->attr.old_thread_sigset = th->attr.thread_sigset;
	}
	kthread_sigmask(SIG_SETMASK, &set, &current_set);
	kthread_sigmask(SIG_SETMASK, &current_set, NULL);
	if( (*( (unsigned long *)&(th->attr.sa_sigset_mask) ) > 0) || (*( (unsigned long *)&(current_set) ) > 0) )
	{
		kthread_sigmask(SIG_SETMASK, (sigset_t *)&(th->attr.thread_sigset), NULL);
		kthread_sigmask(SIG_BLOCK, &current_set, NULL);
		kthread_sigmask(SIG_BLOCK, (sigset_t *)&(th->attr.sa_sigset_mask), NULL);
		kthread_sigmask(SIG_SETMASK, &current_set, (sigset_t *)&(th->attr.thread_sigset) );
	}

	if(&(th->attr.spinlock) != &(_mpc_threads_generic_self()->attr.spinlock) )
	{
		mpc_common_spinlock_lock(&(th->attr.spinlock) );

		for(i = 0; i < SCTK_NSIG; i++)
		{
			if(expect_false(th->attr.thread_sigpending[i] != 0) )
			{
				if(sigismember( (sigset_t *)&(th->attr.thread_sigset), i + 1) == 0)
				{
					th->attr.thread_sigpending[i] = 0;
					th->attr.nb_sig_pending--;
					done++;
					nb_inner_calls++;

					kthread_kill(kthread_self(), i + 1);
					nb_inner_calls--;

					if(nb_inner_calls == 0)
					{
						th->attr.thread_sigset = th->attr.old_thread_sigset;
						sigemptyset( (sigset_t *)&(th->attr.sa_sigset_mask) );
					}
					th->attr.nb_sig_treated += done;
				}
			}
		}

		th->attr.nb_sig_treated += done;
		mpc_common_spinlock_unlock(&(th->attr.spinlock) );
	}
	else
	{
		for(i = 0; i < SCTK_NSIG; i++)
		{
			if(expect_false(th->attr.thread_sigpending[i] != 0) )
			{
				if(sigismember( (sigset_t *)&(th->attr.thread_sigset), i + 1) == 0)
				{
					th->attr.thread_sigpending[i] = 0;
					th->attr.nb_sig_pending--;
					done++;
					nb_inner_calls++;

					kthread_kill(kthread_self(), i + 1);
					nb_inner_calls--;

					if(nb_inner_calls == 0)
					{
						th->attr.thread_sigset = th->attr.old_thread_sigset;
						sigemptyset( (sigset_t *)&(th->attr.sa_sigset_mask) );
					}
					th->attr.nb_sig_treated += done;
				}
			}
		}
	}
#endif
}

int __sctk_thread_generic_sigpending(sctk_thread_generic_t threadp,
                                     sigset_t *set)
{
	int i;

	sigemptyset(set);
	sctk_thread_generic_p_t *th = threadp;

	mpc_common_spinlock_lock(&(th->attr.spinlock) );

	for(i = 0; i < SCTK_NSIG; i++)
	{
		if(th->attr.thread_sigpending[i] != 0)
		{
			sigaddset(set, i + 1);
		}
	}

	mpc_common_spinlock_unlock(&(th->attr.spinlock) );
	return 0;
}

static int
sctk_thread_generic_sigpending(sigset_t *set)
{
	/*
	 *    ERRORS:
	 *    EFAULT |>NOT IMPLEMENTED<|
	 */

#ifndef WINDOWS_SYS
	sctk_thread_generic_p_t *th = _mpc_threads_generic_self();
	return __sctk_thread_generic_sigpending(th, set);
#endif
	return 0;
}

int __sctk_thread_generic_sigmask(sctk_thread_generic_t threadp, int how,
                                  const sigset_t *newmask, sigset_t *oldmask)
{
	int res = -1;

	if(how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK)
	{
		return EINVAL;
	}
	sctk_thread_generic_p_t *th = threadp;
	sigset_t set;

	kthread_sigmask(SIG_SETMASK, (sigset_t *)&(th->attr.thread_sigset), &set);
	res = kthread_sigmask(how, newmask, oldmask);
	kthread_sigmask(SIG_SETMASK, &set, (sigset_t *)&(th->attr.thread_sigset) );

	sctk_thread_generic_check_signals(1);

	return res;
}

static int
sctk_thread_generic_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
	/*
	 *    ERRORS:
	 *    EINVAL  The value specified in how was invalid
	 */
	int res = -1;

#ifndef WINDOWS_SYS
	sctk_thread_generic_p_t *th = _mpc_threads_generic_self();
	res = __sctk_thread_generic_sigmask(th, how, newmask, oldmask);
#endif
	return res;
}

int __sctk_thread_generic_sigsuspend(sctk_thread_generic_t threadp,
                                     const sigset_t *mask)
{
	sigset_t oldmask;
	sigset_t pending;
	int      i;
	sctk_thread_generic_p_t *th = threadp;

	th->attr.nb_sig_treated = 0;
	__sctk_thread_generic_sigmask(th, SIG_SETMASK, mask, &oldmask);

	while(th->attr.nb_sig_treated == 0)
	{
		sctk_thread_generic_sched_yield(&(th->sched) );
		sigpending(&pending);

		for(i = 0; i < SCTK_NSIG; i++)
		{
			if( (sigismember( (sigset_t *)&(th->attr.thread_sigset), i + 1) == 1) &&
			    (sigismember(&pending, i + 1) == 1) )
			{
				kthread_kill(kthread_self(), i + 1);
				th->attr.nb_sig_treated = 1;
			}
		}
	}

	__sctk_thread_generic_sigmask(th, SIG_SETMASK, &oldmask, NULL);
	errno = EINTR;
	return -1;
}

static int
sctk_thread_generic_sigsuspend(const sigset_t *mask)
{
	/*
	 *    ERRORS:
	 *    EINTR  The call was interrupted by a signal
	 *    EFAULT |>NOT IMPLEMENTED<| mask points to memory which is not a valid
	 *               part of the process address space
	 */

#ifndef WINDOWS_SYS
	sctk_thread_generic_p_t *th = _mpc_threads_generic_self();
	return __sctk_thread_generic_sigsuspend(th, mask);
#endif
	return -1;
}

/***************************************/
/* THREAD KILL                         */
/***************************************/

static void
sctk_thread_generic_wake_on_barrier(sctk_thread_generic_scheduler_t *sched,
                                    void *lock, int remove_from_lock_list)
{
	sctk_thread_generic_barrier_t *     barrier = (sctk_thread_generic_barrier_t *)lock;
	sctk_thread_generic_barrier_cell_t *list;

	mpc_common_spinlock_lock(&(barrier->lock) );
	DL_FOREACH(barrier->blocked, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(barrier->blocked, list);
		barrier->nb_current -= 1;
		if(list->sched->th->attr.cancel_type != PTHREAD_CANCEL_DEFERRED)
		{
			sctk_thread_generic_register_spinlock_unlock(&(_mpc_threads_generic_self()->sched),
			                                             &(barrier->lock) );
			sctk_generic_swap_to_sched(sched);
		}
		else
		{
			mpc_common_spinlock_unlock(&(barrier->lock) );
			sctk_thread_generic_wake(sched);
		}
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(barrier->lock) );
	}
}

static void
sctk_thread_generic_wake_on_cond(sctk_thread_generic_scheduler_t *sched,
                                 void *lock, int remove_from_lock_list)
{
	sctk_thread_generic_cond_t *     cond = (sctk_thread_generic_cond_t *)lock;
	sctk_thread_generic_cond_cell_t *list;

	mpc_common_spinlock_lock(&(cond->lock) );
	DL_FOREACH(cond->blocked, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(cond->blocked, list);
		mpc_common_spinlock_unlock(&(cond->lock) );
		sctk_thread_generic_wake(sched);
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(cond->lock) );
	}
}

static void
sctk_thread_generic_wake_on_mutex(sctk_thread_generic_scheduler_t *sched,
                                  void *lock, int remove_from_lock_list)
{
	sctk_thread_generic_mutex_t *     mu = (sctk_thread_generic_mutex_t *)lock;
	sctk_thread_generic_mutex_cell_t *list;

	mpc_common_spinlock_lock(&(mu->lock) );
	DL_FOREACH(mu->blocked, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(mu->blocked, list);
		if(list->sched->th->attr.cancel_type != PTHREAD_CANCEL_DEFERRED)
		{
			sctk_thread_generic_register_spinlock_unlock(&(_mpc_threads_generic_self()->sched),
			                                             &(mu->lock) );
			sctk_generic_swap_to_sched(sched);
		}
		else
		{
			mpc_common_spinlock_unlock(&(mu->lock) );
			sctk_thread_generic_wake(sched);
		}
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(mu->lock) );
	}
}

static void
sctk_thread_generic_wake_on_rwlock(sctk_thread_generic_scheduler_t *sched,
                                   void *lock, int remove_from_lock_list)
{
	sctk_thread_generic_rwlock_t *     rw = (sctk_thread_generic_rwlock_t *)lock;
	sctk_thread_generic_rwlock_cell_t *list;

	mpc_common_spinlock_lock(&(rw->lock) );
	DL_FOREACH(rw->waiting, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(rw->waiting, list);
		rw->count--;
		if(list->sched->th->attr.cancel_type != PTHREAD_CANCEL_DEFERRED)
		{
			sctk_thread_generic_register_spinlock_unlock(&(_mpc_threads_generic_self()->sched),
			                                             &(rw->lock) );
			sctk_generic_swap_to_sched(sched);
			/* Maybe need to remove the lock from taken thread s rwlocks list*/
		}
		else
		{
			mpc_common_spinlock_unlock(&(rw->lock) );
			sctk_thread_generic_wake(sched);
		}
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(rw->lock) );
	}
}

static void
sctk_thread_generic_wake_on_sem(sctk_thread_generic_scheduler_t *sched,
                                void *lock, int remove_from_lock_list)
{
	sctk_thread_generic_sem_t *       sem = (sctk_thread_generic_sem_t *)lock;
	sctk_thread_generic_mutex_cell_t *list;

	mpc_common_spinlock_lock(&(sem->spinlock) );
	DL_FOREACH(sem->list, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(sem->list, list);
		mpc_common_spinlock_unlock(&(sem->spinlock) );
		sctk_thread_generic_wake(sched);
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(sem->spinlock) );
	}
}

static void
sctk_thread_generic_handle_blocked_thread(sctk_thread_generic_t threadp,
                                          int remove_from_lock_list)
{
	sctk_thread_generic_p_t *th = threadp;
	int    i;
	void **list = (void **)th->attr.sctk_thread_generic_pthread_blocking_lock_table;

	for(i = 0; i <= SCTK_BLOCKING_LOCK_TABLE_SIZE; i++)
	{
		if( (i < SCTK_BLOCKING_LOCK_TABLE_SIZE) &&
		    (list[i] != NULL) )
		{
			break;
		}
	}

	switch(i)
	{
	case sctk_thread_generic_barrier:
		sctk_thread_generic_wake_on_barrier(&(th->sched), list[i], remove_from_lock_list);
		break;

	case sctk_thread_generic_cond:
		sctk_thread_generic_wake_on_cond(&(th->sched), list[i], remove_from_lock_list);
		break;

	case sctk_thread_generic_mutex:
		sctk_thread_generic_wake_on_mutex(&(th->sched), list[i], remove_from_lock_list);
		break;

	case sctk_thread_generic_rwlock:
		sctk_thread_generic_wake_on_rwlock(&(th->sched), list[i], remove_from_lock_list);
		break;

	case sctk_thread_generic_sem:
		sctk_thread_generic_wake_on_sem(&(th->sched), list[i], remove_from_lock_list);
		break;

	case sctk_thread_generic_task_lock:
		sctk_thread_generic_wake_on_task_lock(&(th->sched), remove_from_lock_list);
		break;

	default:
		sctk_nodebug("No saved lock, thread %p has been wakened", th);
	}
}

static int
sctk_thread_generic_kill(sctk_thread_generic_t threadp, int val)
{
	/*
	 *    ERRORS:
	 *    ESRCHH No thread with the ID threadp could be found
	 *    EINVAL An invalid signal was specified
	 */

	sctk_nodebug("sctk_thread_generic_kill %p %d set", threadp, val);

	sctk_thread_generic_p_t *th = threadp;
	if(th->sched.status == sctk_thread_generic_joined ||
	   th->sched.status == sctk_thread_generic_zombie)
	{
		return ESRCH;
	}

	if(val == 0)
	{
		return 0;
	}
	val--;
	if(val < 0 || val > SCTK_NSIG)
	{
		errno = EINVAL;
		return EINVAL;
	}

	if( (&(th->attr.spinlock) ) != (&(_mpc_threads_generic_self()->attr.spinlock) ) )
	{
		mpc_common_spinlock_lock(&(th->attr.spinlock) );
		if(th->attr.thread_sigpending[val] == 0)
		{
			th->attr.thread_sigpending[val] = 1;
			th->attr.nb_sig_pending++;
		}
		mpc_common_spinlock_unlock(&(th->attr.spinlock) );
	}
	else
	{
		if(th->attr.thread_sigpending[val] == 0)
		{
			th->attr.thread_sigpending[val] = 1;
			th->attr.nb_sig_pending++;
		}
	}

	sigset_t set;
	kthread_sigmask(SIG_SETMASK, (sigset_t *)&(th->attr.sa_sigset_mask), &set);
	kthread_sigmask(SIG_BLOCK, &set, NULL);
	kthread_sigmask(SIG_SETMASK, &set, (sigset_t *)&(th->attr.sa_sigset_mask) );

	if(th->sched.status == sctk_thread_generic_blocked)
	{
		sctk_thread_generic_handle_blocked_thread(threadp, 0);
	}

	return 0;
}

/*
 * defined but not used
 *
 * static int
 * sctk_thread_generic_kill_other_threads_np(){
 * return 0;
 * }
 */

/***************************************/
/* THREAD CREATION                     */
/***************************************/

void sctk_thread_generic_alloc_pthread_blocking_lock_table(
        const sctk_thread_generic_attr_t *attr)
{
	int    i;
	void **lock_table = (void **)sctk_malloc(SCTK_BLOCKING_LOCK_TABLE_SIZE * sizeof(void *) );

	if(lock_table == NULL)
	{
		abort();
	}
	for(i = 0; i < SCTK_BLOCKING_LOCK_TABLE_SIZE; i++)
	{
		lock_table[i] = NULL;
	}
	attr->ptr->sctk_thread_generic_pthread_blocking_lock_table = (void *)lock_table;
}

static inline void sctk_thread_generic_intern_attr_init(sctk_thread_generic_intern_attr_t *attr)
{
	memset(attr, 0, sizeof(sctk_thread_generic_intern_attr_t) );

	attr->scope        = SCTK_THREAD_SCOPE_PROCESS;
	attr->detachstate  = SCTK_THREAD_CREATE_JOINABLE;
	attr->inheritsched = SCTK_THREAD_EXPLICIT_SCHED;
	attr->cancel_state = PTHREAD_CANCEL_ENABLE;
	attr->cancel_type  = PTHREAD_CANCEL_DEFERRED;
	attr->bind_to      = -1;
	mpc_common_spinlock_init(&attr->spinlock, 0);
	sctk_thread_generic_kind_t knd = sctk_thread_generic_kind_init;
	attr->kind                = knd;
	attr->basic_priority      = 10;
	attr->current_priority    = 10;
	attr->timestamp_threshold = 1;
	attr->timestamp_base      = -1;
}

int
sctk_thread_generic_attr_init(sctk_thread_generic_attr_t *attr)
{
	sctk_thread_generic_intern_attr_t init;

	sctk_thread_generic_intern_attr_init(&init);
	attr->ptr = (sctk_thread_generic_intern_attr_t *)
	            sctk_malloc(sizeof(sctk_thread_generic_intern_attr_t) );

	*(attr->ptr) = init;
	sctk_thread_generic_attr_init_sigs(attr);
	sctk_thread_generic_alloc_pthread_blocking_lock_table(attr);
	return 0;
}

static int
sctk_thread_generic_getattr_np(sctk_thread_generic_t threadp,
                               sctk_thread_generic_attr_t *attr)
{
	/*
	 *    ERRORS:
	 *    EINVAL
	 *    ESRCH
	 */

	if(threadp == NULL || attr == NULL)
	{
		return EINVAL;
	}
	sctk_thread_generic_p_t *th = threadp;
	if(th->sched.status == sctk_thread_generic_joined ||
	   th->sched.status == sctk_thread_generic_zombie)
	{
		return ESRCH;
	}

	/* if(attr->ptr == NULL){ */
	sctk_thread_generic_attr_init(attr);
	/* } */
	*(attr->ptr) = th->attr;
	return 0;
}

int
sctk_thread_generic_attr_destroy(sctk_thread_generic_attr_t *attr)
{
	sctk_free(attr->ptr);
	attr->ptr = NULL;
	return 0;
}

static int
sctk_thread_generic_attr_getscope(const sctk_thread_generic_attr_t *attr, int *scope)
{
	sctk_thread_generic_intern_attr_t init;

	sctk_thread_generic_intern_attr_init(&init);
	sctk_thread_generic_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		/* sctk_thread_generic_attr_init_sigs( attr ); */
		/* sctk_thread_generic_alloc_pthread_blocking_lock_table( attr ); */
	}
	*scope = ptr->scope;
	return 0;
}

static int
sctk_thread_generic_attr_setscope(sctk_thread_generic_attr_t *attr, int scope)
{
	if(scope != PTHREAD_SCOPE_SYSTEM && scope != PTHREAD_SCOPE_PROCESS)
	{
		return EINVAL;
	}
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}
	attr->ptr->scope = scope;
	return 0;
}

static void __sctk_start_routine(void *arg)
{
	sctk_thread_generic_p_t *thread;

	thread = arg;

	sctk_nodebug("Before yield %p", &(thread->sched) );
	/*It is mandatory to have two yields for pthread mode*/
	_mpc_threads_generic_self_set(thread);
	sctk_thread_generic_sched_yield(&(thread->sched) );
	sctk_thread_generic_sched_yield(&(thread->sched) );

	sctk_nodebug("Start %p %p", &(thread->sched), thread->attr.arg);
	thread->attr.return_value = thread->attr.start_routine(thread->attr.arg);
	sctk_nodebug("End %p %p", &(thread->sched), thread->attr.arg);

	/* Handel Exit */
	if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM)
	{
		sctk_swapcontext(&(thread->sched.ctx), &(thread->sched.ctx_bootstrap) );
	}
	else
	{
		sctk_thread_generic_thread_status(&(thread->sched), sctk_thread_generic_zombie);
		sctk_thread_generic_sched_yield(&(thread->sched) );
	}
	not_reachable();
}

static int
sctk_thread_generic_attr_setbinding(sctk_thread_generic_attr_t *attr, int binding)
{
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}
	attr->ptr->bind_to = binding;
	return 0;
}

static int
sctk_thread_generic_attr_getbinding(sctk_thread_generic_attr_t *attr, int *binding)
{
	sctk_thread_generic_intern_attr_t init;

	sctk_thread_generic_intern_attr_init(&init);
	sctk_thread_generic_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		sctk_thread_generic_attr_init_sigs(attr);
		sctk_thread_generic_alloc_pthread_blocking_lock_table(attr);
	}
	*binding = ptr->bind_to;
	return 0;
}

static int
sctk_thread_generic_attr_getstacksize(sctk_thread_generic_attr_t *attr,
                                      size_t *stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL The stacksize argument is not valide
	 */

	if(stacksize == NULL)
	{
		return EINVAL;
	}

	sctk_thread_generic_intern_attr_t init;
	sctk_thread_generic_intern_attr_init(&init);
	sctk_thread_generic_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		sctk_thread_generic_attr_init_sigs(attr);
		sctk_thread_generic_alloc_pthread_blocking_lock_table(attr);
	}
	*stacksize = ptr->stack_size;
	return 0;
}

static int
sctk_thread_generic_attr_setstacksize(sctk_thread_generic_attr_t *attr,
                                      size_t stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL The stacksize value is less than PTHREAD_STACK_MIN
	 */

	if(stacksize < SCTK_THREAD_STACK_MIN)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}
	attr->ptr->stack_size = stacksize;
	return 0;
}

static int
sctk_thread_generic_attr_getstackaddr(sctk_thread_generic_attr_t *attr,
                                      void **addr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The addr argument is not valide
	 */

	if(addr == NULL)
	{
		return EINVAL;
	}

	sctk_thread_generic_intern_attr_t init;
	sctk_thread_generic_intern_attr_init(&init);
	sctk_thread_generic_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		sctk_thread_generic_attr_init_sigs(attr);
		sctk_thread_generic_alloc_pthread_blocking_lock_table(attr);
	}

	*addr = ptr->stack;
	return 0;
}

static int
sctk_thread_generic_attr_setstackaddr(sctk_thread_generic_attr_t *attr, void *addr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The addr argument is not valide
	 */

	if(addr == NULL)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}
	attr->ptr->stack      = addr;
	attr->ptr->user_stack = attr->ptr->stack;
	return 0;
}

static int
sctk_thread_generic_attr_getstack(const sctk_thread_generic_attr_t *attr,
                                  void **stackaddr, size_t *stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Argument is not valide
	 */

	if(attr == NULL || stackaddr == NULL ||
	   stacksize == NULL)
	{
		return EINVAL;
	}

	*stackaddr = (void *)attr->ptr->stack;
	*stacksize = attr->ptr->stack_size;

	return 0;
}

static int
sctk_thread_generic_attr_setstack(sctk_thread_generic_attr_t *attr,
                                  void *stackaddr, size_t stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Argument is not valide or the stacksize value is less
	 *               than PTHREAD_STACK_MIN
	 */

	if(stackaddr == NULL || stacksize < SCTK_THREAD_STACK_MIN)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	attr->ptr->stack      = stackaddr;
	attr->ptr->user_stack = attr->ptr->stack;
	attr->ptr->stack_size = stacksize;

	return 0;
}

static int
sctk_thread_generic_attr_getguardsize(sctk_thread_generic_attr_t *attr,
                                      size_t *guardsize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function
	 */

	if(attr == NULL || guardsize == NULL)
	{
		return EINVAL;
	}
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}
	(*guardsize) = attr->ptr->stack_guardsize;

	return 0;
}

static int
sctk_thread_generic_attr_setguardsize(sctk_thread_generic_attr_t *attr,
                                      size_t guardsize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	attr->ptr->stack_guardsize = guardsize;

	return 0;
}

struct sched_param;

static int
sctk_thread_generic_attr_getschedparam(sctk_thread_generic_attr_t *attr,
                                       struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL  Arguments of function are invalid
	 */

	if(attr == NULL || param == NULL)
	{
		return EINVAL;
	}
	param->__sched_priority = 0;
	return 0;
}

static int
sctk_thread_generic_attr_setschedparam(sctk_thread_generic_attr_t *attr,
                                       const struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL  Arguments of function are invalid
	 *    ENOTSUP Only priority 0 (default) is supported
	 */

	if(attr == NULL || param == NULL)
	{
		return EINVAL;
	}
	if(param->__sched_priority != 0)
	{
		return ENOTSUP;
	}
	return 0;
}

static int
sctk_thread_generic_attr_setschedpolicy(sctk_thread_generic_attr_t *attr,
                                        int policy)
{
	if(policy == SCTK_SCHED_FIFO ||
	   policy == SCTK_SCHED_RR)
	{
		return ENOTSUP;
	}
	if(policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	attr->ptr->schedpolicy = policy;

	return 0;
}

static int
sctk_thread_generic_attr_getschedpolicy(sctk_thread_generic_attr_t *attr,
                                        int *policy)
{
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	(*policy) = attr->ptr->schedpolicy;

	return 0;
}

static int
sctk_thread_generic_attr_getinheritsched(sctk_thread_generic_attr_t *attr,
                                         int *inheritsched)
{
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	(*inheritsched) = attr->ptr->inheritsched;

	return 0;
}

static int
sctk_thread_generic_attr_setinheritsched(sctk_thread_generic_attr_t *attr,
                                         int inheritsched)
{
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}
	if(inheritsched != SCTK_THREAD_INHERIT_SCHED &&
	   inheritsched != SCTK_THREAD_EXPLICIT_SCHED)
	{
		return EINVAL;
	}

	attr->ptr->inheritsched = inheritsched;

	return 0;
}

static int
sctk_thread_generic_attr_getdetachstate(sctk_thread_generic_attr_t *attr,
                                        int *detachstate)
{
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	(*detachstate) = attr->ptr->detachstate;

	return 0;
}

static int
sctk_thread_generic_attr_setdetachstate(sctk_thread_generic_attr_t *attr,
                                        int detachstate)
{
	if(detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	attr->ptr->detachstate = detachstate;

	return 0;
}

int
sctk_thread_generic_user_create(sctk_thread_generic_t *threadp,
                                sctk_thread_generic_attr_t *attr,
                                void *(*start_routine)(void *), void *arg)
{
	sctk_thread_generic_intern_attr_t init;

	sctk_thread_generic_intern_attr_init(&init);
	sctk_thread_generic_intern_attr_t *ptr;
	sctk_thread_data_t    tmp;
	sctk_thread_generic_t thread_id;
	char * stack;
	size_t stack_size;
	size_t stack_guardsize;

	if(attr == NULL)
	{
		ptr = &init;
	}
	else
	{
		ptr = attr->ptr;

		if(ptr == NULL)
		{
			ptr = &init;
		}
	}
	sctk_thread_generic_attr_t lattr;
	lattr.ptr = ptr;

	/****** SIGNALS ******/
	sctk_thread_generic_init_default_sigset();
	sctk_thread_generic_attr_init_sigs(&lattr);

	sctk_thread_generic_alloc_pthread_blocking_lock_table(&lattr);

	sctk_nodebug("Create %p", arg);

	if(arg != NULL)
	{
		tmp = *( (sctk_thread_data_t *)arg);
		if(tmp.tls == NULL)
		{
			tmp.tls = sctk_thread_tls;
		}
	}
	else
	{
		tmp.tls = sctk_thread_tls;
	}

	/*Create data struct*/
	{
		thread_id =
		        __sctk_malloc(sizeof(sctk_thread_generic_p_t), tmp.tls);

		thread_id->attr = *ptr;

		sctk_thread_generic_scheduler_init_thread(&(thread_id->sched), thread_id);
		_mpc_threads_generic_key_init_thread(&(thread_id->keys) );
	}

	/*Allocate stack*/
	{
		stack           = thread_id->attr.stack;
		stack_size      = thread_id->attr.stack_size;
		stack_guardsize = thread_id->attr.stack_guardsize;
		size_t old_stack_size      = 0;
		size_t old_stack_guardsize = 0;

		if(stack == NULL)
		{
			if(mpc_common_get_flags()->is_fortran == 1 && stack_size <= 0)
			{
				stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
			}
			else if(stack_size <= 0)
			{
				stack_size = SCTK_ETHREAD_STACK_SIZE;
			}
			if(stack_guardsize > 0)
			{
				long _page_size_ = 0;
				int  ret;
#ifndef _SC_PAGESIZE
  #ifndef PAGE_SIZE
	#error "Unknown system macro for pagesize"
  #else
				_page_size_ = sysconf(PAGESIZE);
  #endif
#else
				_page_size_ = sysconf(_SC_PAGESIZE);
#endif
				old_stack_guardsize = stack_guardsize;
				stack_guardsize     = (stack_guardsize + _page_size_ - 1) & ~(_page_size_ - 1);
				stack_size          = (stack_size + _page_size_ - 1) & ~(_page_size_ - 1);
				old_stack_size      = stack_size;
				stack_size          = stack_size + stack_guardsize;
				int ret_mem = posix_memalign( (void *)&stack, _page_size_, stack_size);
				if(stack == NULL || ret_mem != 0)
				{
					sctk_free(thread_id);
					return EAGAIN;
				}
				if( (ret = mprotect(stack, stack_guardsize, PROT_NONE) ) != 0)
				{
					sctk_free(stack);
					sctk_free(thread_id);
					return ret;
				}
			}
			if(stack == NULL)
			{
				stack = (char *)__sctk_malloc(stack_size + 8, tmp.tls);
				if(stack == NULL)
				{
					sctk_free(thread_id);
					return EAGAIN;
				}
			}
		}
		else if(stack_size <= 0)
		{
			sctk_free(thread_id);
			return EINVAL;
		}

		if(stack_guardsize > 0)
		{
			thread_id->attr.user_stack = stack;
			thread_id->attr.stack      = stack + old_stack_guardsize;
			thread_id->attr.stack_size = old_stack_size;
		}
		else
		{
			thread_id->attr.stack      = stack;
			thread_id->attr.stack_size = stack_size;
		}
	}


	/*Create context*/
	{
		thread_id->attr.start_routine = start_routine;
		thread_id->attr.arg           = arg;
		sctk_nodebug("STACK %p STACK SIZE %lu", stack, stack_size);
		sctk_makecontext(&(thread_id->sched.ctx),
		                 (void *)thread_id,
		                 __sctk_start_routine, stack, stack_size);
	}

	thread_id->attr.nb_wait_for_join = 0;

	*threadp = thread_id;

	sctk_thread_generic_sched_create(thread_id);
	return 0;
}

static int
sctk_thread_generic_create(sctk_thread_generic_t *threadp,
                           sctk_thread_generic_attr_t *attr,
                           void *(*start_routine)(void *), void *arg)
{
	static unsigned int pos = 0;
	int res;
	sctk_thread_generic_attr_t tmp_attr;

	tmp_attr.ptr = NULL;

	if(attr == NULL)
	{
		attr = &tmp_attr;
	}
	if(attr->ptr == NULL)
	{
		sctk_thread_generic_attr_init(attr);
	}

	if(attr->ptr->bind_to == -1)
	{
		attr->ptr->bind_to = sctk_get_init_vp(pos);
		pos++;
	}

	res = sctk_thread_generic_user_create(threadp, attr, start_routine, arg);

	sctk_thread_generic_attr_destroy(&tmp_attr);

	return res;
}

static int
sctk_thread_generic_atfork(__UNUSED__ void (*prepare)(void), __UNUSED__ void (*parent)(void),
                           __UNUSED__ void (*child)(void) )
{
	not_implemented();
	return 0;
}

static int
sctk_thread_generic_sched_get_priority_min(int sched_policy)
{
	/*
	 *    ERRORS:
	 *    EINVAL the value of the argument is not valide
	 */

	if(sched_policy != SCTK_SCHED_FIFO && sched_policy != SCTK_SCHED_RR &&
	   sched_policy != SCTK_SCHED_OTHER)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
sctk_thread_generic_sched_get_priority_max(int sched_policy)
{
	/*
	 *    ERRORS:
	 *    EINVAL the value of the argument is not valide
	 */

	if(sched_policy != SCTK_SCHED_FIFO && sched_policy != SCTK_SCHED_RR &&
	   sched_policy != SCTK_SCHED_OTHER)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
sctk_thread_generic_getschedparam(sctk_thread_generic_t threadp,
                                  int *policy, struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function call
	 *    ESRCH No thread with the ID threadp could be found
	 */

	if(threadp == NULL || policy == NULL || param == NULL)
	{
		return EINVAL;
	}
	sctk_thread_generic_p_t *th = threadp;
	if(th->sched.status == sctk_thread_generic_zombie ||
	   th->sched.status == sctk_thread_generic_joined)
	{
		return ESRCH;
	}

	param->__sched_priority = 0;
	(*policy) = SCHED_OTHER;

	return 0;
}

static int
sctk_thread_generic_setschedparam(sctk_thread_generic_t threadp,
                                  int policy, const struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function call
	 *    ESRCH No thread with the ID threadp could be found
	 *    EPERM |>NOT IMPLEMENTED<| The caller does not have appropriate
	 *    privileges to set the specified scheduling policy and parameters
	 */

	if(threadp == NULL || param == NULL)
	{
		return EINVAL;
	}
	if(policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}
	sctk_thread_generic_p_t *th = threadp;
	if(th->sched.status == sctk_thread_generic_zombie ||
	   th->sched.status == sctk_thread_generic_joined)
	{
		return ESRCH;
	}
	if(param->__sched_priority != 0)
	{
		return EINVAL;
	}

	return 0;
}

/***************************************/
/* THREAD CANCELATION                  */
/***************************************/

void sctk_thread_generic_check_signals(int select)
{
	sctk_thread_generic_scheduler_t *sched;
	sctk_thread_generic_p_t *        current;

	/* Get the current thread */
	sched = &(_mpc_threads_generic_self()->sched);
	sctk_nodebug("sctk_thread_generic_check_signals %p", sched);
	current = sched->th;
	sctk_assert(&current->sched == sched);

	if(expect_false(current->attr.nb_sig_pending > 0) )
	{
		sctk_thread_generic_treat_signals(current);
	}

	if(expect_false(current->attr.cancel_status > 0) )
	{
		mpc_common_debug(
		        "%p %d %d", current,
		        (current->attr.cancel_state != SCTK_THREAD_CANCEL_DISABLE),
		        ( (current->attr.cancel_type != SCTK_THREAD_CANCEL_DEFERRED) && select) );

		if( (current->attr.cancel_state != SCTK_THREAD_CANCEL_DISABLE) &&
		    ( (current->attr.cancel_type != SCTK_THREAD_CANCEL_DEFERRED) ||
		      !select) )
		{
			mpc_common_debug("Exit Thread %p", current);
			current->attr.cancel_status = 0;
			current->attr.return_value  = ( (void *)SCTK_THREAD_CANCELED);
			sctk_thread_exit_cleanup();
			mpc_common_debug("thread %p key liberation", current);
			__keys_delete_all(&(current->keys) );
			mpc_common_debug("thread %p key liberation done", current);
			mpc_common_debug("thread %p ends", current);

			sctk_thread_generic_thread_status(&(current->sched),
			                                  sctk_thread_generic_zombie);
			sctk_thread_generic_sched_yield(&(current->sched) );
		}
	}
}

static int
sctk_thread_generic_cancel(sctk_thread_generic_t threadp)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid value for the argument
	 */

	sctk_thread_generic_p_t *th = threadp;

	mpc_common_debug("thread to cancel: %p\n", th);

	if(th == NULL)
	{
		return EINVAL;
	}
	if(th->sched.status == sctk_thread_generic_zombie ||
	   th->sched.status == sctk_thread_generic_joined)
	{
		return ESRCH;
	}

	if(th->attr.cancel_state == PTHREAD_CANCEL_ENABLE)
	{
		th->attr.cancel_status = 1;
		mpc_common_debug("thread %p canceled\n", th);
	}

	if(th->sched.status == sctk_thread_generic_blocked)
	{
		sctk_thread_generic_handle_blocked_thread(threadp, 1);
	}

	return 0;
}

static int
sctk_thread_generic_setcancelstate(int state, int *oldstate)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid value for state
	 */

	sctk_thread_generic_attr_t attr;

	attr.ptr = &(_mpc_threads_generic_self()->attr);

	if(oldstate != NULL)
	{
		(*oldstate) = attr.ptr->cancel_state;
	}

	if(state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE)
	{
		return EINVAL;
	}

	attr.ptr->cancel_state = state;

	return 0;
}

static int
sctk_thread_generic_setcanceltype(int type, int *oldtype)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid value for type
	 */

	sctk_thread_generic_attr_t attr;

	attr.ptr = &(_mpc_threads_generic_self()->attr);

	if(oldtype != NULL)
	{
		(*oldtype) = attr.ptr->cancel_type;
	}

	if(type != PTHREAD_CANCEL_DEFERRED &&
	   type != PTHREAD_CANCEL_ASYNCHRONOUS)
	{
		return EINVAL;
	}

	attr.ptr->cancel_type = type;

	return 0;
}

static int
sctk_thread_generic_setschedprio(sctk_thread_generic_t threadp, int prio)
{
	/*
	 *    ERRORS
	 *    EINVAL prio is not valid for the scheduling policy of the specified thread
	 *    EPERM  |>NOT IMPLEMENTED<| The caller does not have appropriate privileges
	 *               to set the specified priority
	 *    ESRCH  No thread with the ID thread could be found
	 */

	if(threadp == NULL || prio != 0)
	{
		return EINVAL;
	}

	sctk_thread_generic_p_t *th = threadp;
	if(th->sched.status == sctk_thread_generic_joined ||
	   th->sched.status == sctk_thread_generic_zombie)
	{
		return ESRCH;
	}

	sctk_nodebug("Only Priority 0 handled in current version of MPC, priority remains the same as before the call");

	return 0;
}

static void
sctk_thread_generic_testcancel()
{
	sctk_thread_generic_check_signals(0);
}

/***************************************/
/* THREAD POLLING                      */
/***************************************/

void
sctk_thread_generic_wait_for_value_and_poll(volatile int *data, int value,
                                            void (*func)(void *), void *arg)
{
	sctk_thread_generic_task_t task;

	if(func)
	{
		func(arg);
	}
	if(*data == value)
	{
		return;
	}

	task.sched       = &(_mpc_threads_generic_self()->sched);
	task.func        = func;
	task.value       = value;
	task.arg         = arg;
	task.data        = data;
	task.is_blocking = 1;
	task.free_func   = NULL;
	task.next        = NULL;
	task.prev        = NULL;

	sctk_thread_generic_add_task(&task);
}

/***************************************/
/* THREAD JOIN                         */
/***************************************/

static int
sctk_thread_generic_join(sctk_thread_generic_t threadp, void **val)
{
	/*
	 *    ERRORS:
	 *    ESRCH  No  thread could be found corresponding to that specified by th.
	 *    EINVAL The th thread has been detached.
	 *    EINVAL Another thread is already waiting on termination of th.
	 *    EDEADLK The th argument refers to the calling thread.
	 */

	sctk_thread_generic_p_t *            th = threadp;
	sctk_thread_generic_scheduler_t *    sched;
	sctk_thread_generic_p_t *            current;
	sctk_thread_generic_thread_status_t *status;

	/* Get the current thread */
	sched   = &(_mpc_threads_generic_self()->sched);
	current = sched->th;
	sctk_assert(&current->sched == sched);

	sctk_nodebug("Join Thread %p", th);

	/* test cancel */
	sctk_thread_generic_check_signals(0);

	if(th != current)
	{
		if(th->sched.status == sctk_thread_generic_joined)
		{
			return ESRCH;
		}
		if(th->attr.detachstate != 0)
		{
			return EINVAL;
		}

		th->attr.nb_wait_for_join++;
		if(th->attr.nb_wait_for_join != 1)
		{
			return EINVAL;
		}

		status = (sctk_thread_generic_thread_status_t *)&(th->sched.status);
		sctk_nodebug("TO Join Thread %p", th);
		sctk_thread_generic_wait_for_value_and_poll( (volatile int *)status,
		                                             sctk_thread_generic_zombie, NULL, NULL);

		/* test cancel */
		sctk_thread_generic_check_signals(0);

		sctk_nodebug("Joined Thread %p", th);

		if(val != NULL)
		{
			*val = th->attr.return_value;
		}
		th->sched.status = sctk_thread_generic_joined;

		/* Free thread memory */
		//sctk_thread_generic_handle_zombies( &(th->sched.generic ));
	}
	else
	{
		return EDEADLK;
	}

	return 0;
}

/***************************************/
/* THREAD EXIT                         */
/***************************************/

static void
sctk_thread_generic_exit(void *retval)
{
	sctk_thread_generic_scheduler_t *sched;
	sctk_thread_generic_p_t *        current;

	/* test cancel */
	sctk_thread_generic_check_signals(0);

	/* Get the current thread */
	sched   = &(_mpc_threads_generic_self()->sched);
	current = sched->th;
	sctk_assert(&current->sched == sched);

	current->attr.return_value = retval;

	sctk_nodebug("thread %p key liberation", current);
	/*key liberation */
	__keys_delete_all(&(current->keys) );

	sctk_nodebug("thread %p key liberation done", current);

	sctk_nodebug("thread %p ends", current);

	sctk_thread_generic_thread_status(&(current->sched), sctk_thread_generic_zombie);
	sctk_thread_generic_sched_yield(&(current->sched) );
	not_reachable();
}

/***************************************/
/* THREAD MEMORY                       */
/***************************************/

void sctk_thread_generic_handle_zombies(
        sctk_thread_generic_scheduler_generic_t *th)
{
	sctk_thread_generic_scheduler_generic_t *schedg = th;


	sctk_nodebug("th %p to be freed", th->sched->th);
	if(schedg->sched->th->attr.user_stack == NULL)
	{
		sctk_free(schedg->sched->th->attr.stack);
	}
	sctk_free(schedg->sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table);
	sctk_free(schedg->sched->th);
}

/***************************************/
/* THREAD EQUAL                        */
/***************************************/

static int
sctk_thread_generic_equal(sctk_thread_generic_t th1, sctk_thread_generic_t th2)
{
	return th1 == th2;
}

/***************************************/
/* THREAD DETACH                       */
/***************************************/

static int
sctk_thread_generic_detach(sctk_thread_generic_t threadp)
{
	/*
	 *    ERRORS:
	 *    EINVAL threadp is not a joinable thread or already detach
	 *    ESRCH  No thread with the ID threadp could be found
	 */

	if(threadp == NULL)
	{
		return ESRCH;
	}
	sctk_thread_generic_p_t *th = threadp;

	if(th->sched.status == sctk_thread_generic_joined /*||
	                                                   * th->sched.status == sctk_thread_generic_zombie*/)
	{
		return ESRCH;
	}
	if(th->attr.detachstate == SCTK_THREAD_CREATE_DETACHED)
	{
		return EINVAL;
	}

	th->attr.detachstate = SCTK_THREAD_CREATE_DETACHED;

	return 0;
}

/***************************************/
/* THREAD CONCURRENCY                  */
/***************************************/

static int
sctk_thread_generic_setconcurrency(int new_level)
{
	if(new_level < 0)
	{
		return EINVAL;
	}
	if(new_level != 0)
	{
		return ENOTSUP;
	}
	return 0;
}

static int
sctk_thread_generic_getconcurrency(void)
{
	return 0;
}

/***************************************/
/* THREAD CPUCLOCKID                   */
/***************************************/

static int
sctk_thread_generic_getcpuclockid(sctk_thread_generic_t threadp,
                                  clockid_t *clock_id)
{
	/*
	 *    ERRORS:
	 *    ENOENT |>NOT IMPLEMENTED<| Per-thread CPU time clocks
	 *               are not supported by the system
	 *    ESRCH  No thread with the ID thread could be found
	 *    EINVAL Invalid arguments for function call
	 */

	if(threadp == NULL || clock_id == NULL)
	{
		return EINVAL;
	}
	sctk_thread_generic_p_t *th = threadp;
	if(th->sched.status == sctk_thread_generic_zombie ||
	   th->sched.status == sctk_thread_generic_joined)
	{
		return ESRCH;
	}

	int ret = 0;
#ifdef _POSIX_THREAD_CPUTIME
	(*clock_id) = CLOCK_THREAD_CPUTIME_ID;
#else
	ret = ENOENT;
#endif

	return ret;
}

/***************************************/
/* THREAD ONCE                         */
/***************************************/

static int __sctk_thread_generic_once_initialized(
        sctk_thread_once_t *once_control)
{
#ifdef sctk_thread_once_t_is_contiguous_int
	return *( (sctk_thread_once_t *)once_control) == SCTK_THREAD_ONCE_INIT;

#else
	sctk_thread_once_t once_init = SCTK_THREAD_ONCE_INIT;
	return memcpy
	               ( (void *)&once_init, (void *)once_control,
	               sizeof(sctk_thread_once_t) ) == 0;
#endif
}

static int
sctk_thread_generic_once(sctk_thread_once_t *once_control,
                         void (*init_routine)(void) )
{
	static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

	if(__sctk_thread_generic_once_initialized(once_control) )
	{
		sctk_thread_mutex_lock(&lock);
		if(__sctk_thread_generic_once_initialized(once_control) )
		{
			init_routine();
#ifdef sctk_thread_once_t_is_contiguous_int
			*once_control = !SCTK_THREAD_ONCE_INIT;
#else
			once_control[0] = 1;
#endif
		}
		sctk_thread_mutex_unlock(&lock);
	}
	return 0;
}

/***************************************/
/* YIELD                               */
/***************************************/
static int
sctk_thread_generic_thread_sched_yield()
{
	sctk_thread_generic_sched_yield(&(_mpc_threads_generic_self()->sched) );
	return 0;
}

/***************************************/
/* INIT                                */
/***************************************/
static void
sctk_thread_generic_thread_init(char *thread_type, char *scheduler_type, int vp_number)
{
	sctk_only_once();
	_mpc_threads_generic_self_data = sctk_malloc(sizeof(sctk_thread_generic_p_t) );

	sctk_thread_generic_check_size(sctk_thread_generic_t, sctk_thread_t);
	sctk_add_func_type(_mpc_threads_generic, self, sctk_thread_t (*)(void) );

	/****** SIGNALS ******/
	sctk_thread_generic_init_default_sigset();


	/****** SCHEDULER ******/
	sctk_thread_generic_scheduler_init(thread_type, scheduler_type, vp_number);
	sctk_thread_generic_scheduler_init_thread(&(_mpc_threads_generic_self()->sched),
	                                          _mpc_threads_generic_self() );
	{
		sctk_thread_generic_attr_t         lattr;
		sctk_thread_generic_intern_attr_t *ptr;
		sctk_thread_generic_intern_attr_t  init;
		sctk_thread_generic_intern_attr_init(&init);
		sctk_thread_generic_scheduler_t *sched;
		sctk_thread_generic_p_t *        current;

		ptr       = &init;
		lattr.ptr = ptr;
		sctk_thread_generic_attr_init_sigs(&lattr);
		sctk_thread_generic_alloc_pthread_blocking_lock_table(&lattr);
		sched         = &(_mpc_threads_generic_self()->sched);
		current       = sched->th;
		current->attr = *ptr;

		mpc_common_debug("%d", current->attr.cancel_status);
	}



	/****** JOIN ******/
	sctk_add_func_type(sctk_thread_generic, join,
	                   int (*)(sctk_thread_t, void **) );

	/****** EXIT ******/
	sctk_add_func_type(sctk_thread_generic, exit,
	                   void (*)(void *) );

	/****** EQUAL ******/
	sctk_add_func_type(sctk_thread_generic, equal,
	                   int (*)(sctk_thread_t, sctk_thread_t) );

	/****** DETACH ******/
	sctk_add_func_type(sctk_thread_generic, detach,
	                   int (*)(sctk_thread_t) );

	/****** CONCURRENCY ******/
	sctk_add_func_type(sctk_thread_generic, getconcurrency,
	                   int (*)(void) );
	sctk_add_func_type(sctk_thread_generic, setconcurrency,
	                   int (*)(int) );

	/****** CPUCLOCKID ******/
	sctk_add_func_type(sctk_thread_generic, getcpuclockid,
	                   int (*)(sctk_thread_t, clockid_t *) );

	/****** KEYS ******/
	__keys_init();
	sctk_add_func_type(_mpc_threads_generic, key_create,
	                   int (*)(sctk_thread_key_t *, void (*)(void *) ) );
	sctk_add_func_type(_mpc_threads_generic, key_delete,
	                   int (*)(sctk_thread_key_t) );
	sctk_add_func_type(_mpc_threads_generic, setspecific,
	                   int (*)(sctk_thread_key_t, const void *) );
	sctk_add_func_type(_mpc_threads_generic, getspecific,
	                   void *(*)(sctk_thread_key_t) );
	_mpc_threads_generic_key_init_thread(&(_mpc_threads_generic_self()->keys) );

	/****** MUTEX ******/
	__mutex_init();
	sctk_add_func_type(_mpc_threads_generic, mutexattr_destroy,
	                   int (*)(sctk_thread_mutexattr_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutexattr_setpshared,
	                   int (*)(sctk_thread_mutexattr_t *, int) );
	sctk_add_func_type(_mpc_threads_generic, mutexattr_getpshared,
	                   int (*)(const sctk_thread_mutexattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_generic, mutexattr_settype,
	                   int (*)(sctk_thread_mutexattr_t *, int) );
	sctk_add_func_type(_mpc_threads_generic, mutexattr_gettype,
	                   int (*)(const sctk_thread_mutexattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_generic, mutexattr_init,
	                   int (*)(sctk_thread_mutexattr_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_lock,
	                   int (*)(sctk_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_spinlock,
	                   int (*)(sctk_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_trylock,
	                   int (*)(sctk_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_timedlock,
	                   int (*)(sctk_thread_mutex_t *, const struct timespec *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_unlock,
	                   int (*)(sctk_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_destroy,
	                   int (*)(sctk_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_generic, mutex_init,
	                   int (*)(sctk_thread_mutex_t *, const sctk_thread_mutexattr_t * ));

	/****** COND ******/
	__cond_init();
	sctk_add_func_type(_mpc_threads_generic, cond_init,
			    int (*)(sctk_thread_cond_t *, const sctk_thread_condattr_t *) );
	sctk_add_func_type(_mpc_threads_generic, condattr_destroy,
	                   int (*)(sctk_thread_condattr_t *) );
	sctk_add_func_type(_mpc_threads_generic, condattr_getpshared,
	                   int (*)(const sctk_thread_condattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_generic, condattr_init,
	                   int (*)(sctk_thread_condattr_t *) );
	sctk_add_func_type(_mpc_threads_generic, condattr_setpshared,
	                   int (*)(sctk_thread_condattr_t *, int) );
	sctk_add_func_type(_mpc_threads_generic, condattr_setclock,
	                   int (*)(sctk_thread_condattr_t *, int) );
	sctk_add_func_type(_mpc_threads_generic, condattr_getclock,
	                   int (*)(sctk_thread_condattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_generic, cond_destroy,
	                   int (*)(sctk_thread_cond_t *) );
	sctk_add_func_type(_mpc_threads_generic, cond_wait,
	                   int (*)(sctk_thread_cond_t *, sctk_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_generic, cond_signal,
	                   int (*)(sctk_thread_cond_t *) );
	sctk_add_func_type(_mpc_threads_generic, cond_timedwait,
	                   int (*)(sctk_thread_cond_t *, sctk_thread_mutex_t *, const struct timespec *) );
	sctk_add_func_type(_mpc_threads_generic, cond_broadcast,
	                   int (*)(sctk_thread_cond_t *) );

	/****** RWLOCK ******/
	sctk_thread_generic_rwlocks_init();
	//__sctk_ptr_thread_rwlock_init = sctk_thread_generic_rwlock_init;

	sctk_add_func_type(sctk_thread_generic, rwlock_init,
	                   int (*)(sctk_thread_rwlock_t *, const sctk_thread_rwlockattr_t *) );
	sctk_add_func_type(sctk_thread_generic, rwlockattr_destroy,
	                   int (*)(sctk_thread_rwlockattr_t *attr) );
	sctk_add_func_type(sctk_thread_generic, rwlockattr_getpshared,
	                   int (*)(const sctk_thread_rwlockattr_t *attr, int *val) );
	sctk_add_func_type(sctk_thread_generic, rwlockattr_init,
	                   int (*)(sctk_thread_rwlockattr_t *attr) );
	sctk_add_func_type(sctk_thread_generic, rwlockattr_setpshared,
	                   int (*)(sctk_thread_rwlockattr_t *attr, int val) );
	sctk_add_func_type(sctk_thread_generic, rwlock_destroy,
	                   int (*)(sctk_thread_rwlock_t *lock) );
	sctk_add_func_type(sctk_thread_generic, rwlock_rdlock,
	                   int (*)(sctk_thread_rwlock_t *lock) );
	sctk_add_func_type(sctk_thread_generic, rwlock_wrlock,
	                   int (*)(sctk_thread_rwlock_t *lock) );
	sctk_add_func_type(sctk_thread_generic, rwlock_tryrdlock,
	                   int (*)(sctk_thread_rwlock_t *lock) );
	sctk_add_func_type(sctk_thread_generic, rwlock_trywrlock,
	                   int (*)(sctk_thread_rwlock_t *lock) );
	sctk_add_func_type(sctk_thread_generic, rwlock_unlock,
	                   int (*)(sctk_thread_rwlock_t *lock) );
	sctk_add_func_type(sctk_thread_generic, rwlock_timedrdlock,
	                   int (*)(sctk_thread_rwlock_t *lock, const struct timespec *time) );
	sctk_add_func_type(sctk_thread_generic, rwlock_timedwrlock,
	                   int (*)(sctk_thread_rwlock_t *lock, const struct timespec *time) );

	/*sctk_add_func_type (sctk_thread_generic, rwlockattr_setkind_np,
	 *                      int (*)( sctk_thread_rwlockattr_t* attr, int ));
	 * sctk_add_func_type (sctk_thread_generic, rwlockattr_getkind_np,
	 *                      int (*)( sctk_thread_rwlockattr_t* attr, int* ));*/

	/****** SEMAPHORE ******/
	sctk_thread_generic_sems_init();
	__sctk_ptr_thread_sem_init = sctk_thread_generic_sem_init;
	sctk_add_func_type(sctk_thread_generic, sem_wait,
	                   int (*)(sctk_thread_sem_t *) );
	sctk_add_func_type(sctk_thread_generic, sem_trywait,
	                   int (*)(sctk_thread_sem_t *) );
	sctk_add_func_type(sctk_thread_generic, sem_timedwait,
	                   int (*)(sctk_thread_sem_t *, const struct timespec *) );
	sctk_add_func_type(sctk_thread_generic, sem_post,
	                   int (*)(sctk_thread_sem_t *) );
	sctk_add_func_type(sctk_thread_generic, sem_getvalue,
	                   int (*)(sctk_thread_sem_t *, int *) );
	sctk_add_func_type(sctk_thread_generic, sem_destroy,
	                   int (*)(sctk_thread_sem_t *) );
	sctk_add_func_type(sctk_thread_generic, sem_open,
	                   sctk_thread_sem_t * (*)(const char *, int, ...) );
	sctk_add_func_type(sctk_thread_generic, sem_close,
	                   int (*)(sctk_thread_sem_t *) );
	sctk_add_func_type(sctk_thread_generic, sem_unlink,
	                   int (*)(const char *) );

	/****** THREAD BARRIER *******/
	sctk_thread_generic_barriers_init();
	__sctk_ptr_thread_barrier_init = sctk_thread_generic_barrier_init;
	sctk_add_func_type(sctk_thread_generic, barrierattr_destroy,
	                   int (*)(sctk_thread_barrierattr_t *) );
	sctk_add_func_type(sctk_thread_generic, barrierattr_init,
	                   int (*)(sctk_thread_barrierattr_t *) );
	sctk_add_func_type(sctk_thread_generic, barrierattr_getpshared,
	                   int (*)(const sctk_thread_barrierattr_t *restrict, int *restrict) );
	sctk_add_func_type(sctk_thread_generic, barrierattr_setpshared,
	                   int (*)(sctk_thread_barrierattr_t *, int) );
	sctk_add_func_type(sctk_thread_generic, barrier_destroy,
	                   int (*)(sctk_thread_barrier_t *) );
	sctk_add_func_type(sctk_thread_generic, barrier_init,
	                   int (*)(sctk_thread_barrier_t *restrict,
	                           const sctk_thread_barrierattr_t *restrict, unsigned) );
	sctk_add_func_type(sctk_thread_generic, barrier_wait,
	                   int (*)(sctk_thread_barrier_t *) );

	/****** THREAD SPINLOCK ******/
	sctk_thread_generic_spinlocks_init();
	__sctk_ptr_thread_spin_init = sctk_thread_generic_spin_init;
	sctk_add_func_type(sctk_thread_generic, spin_destroy,
	                   int (*)(sctk_thread_spinlock_t *) );
	sctk_add_func_type(sctk_thread_generic, spin_lock,
	                   int (*)(sctk_thread_spinlock_t *) );
	sctk_add_func_type(sctk_thread_generic, spin_trylock,
	                   int (*)(sctk_thread_spinlock_t *) );
	sctk_add_func_type(sctk_thread_generic, spin_unlock,
	                   int (*)(sctk_thread_spinlock_t *) );

	/****** THREAD CREATION ******/
	sctk_thread_generic_check_size(sctk_thread_generic_attr_t, sctk_thread_attr_t);
	sctk_add_func_type(sctk_thread_generic, attr_init,
	                   int (*)(sctk_thread_attr_t *) );
	sctk_add_func_type(sctk_thread_generic, attr_destroy,
	                   int (*)(sctk_thread_attr_t *) );
	sctk_add_func_type(sctk_thread_generic, getattr_np,
	                   int (*)(sctk_thread_t, sctk_thread_attr_t *) );

	sctk_add_func_type(sctk_thread_generic, attr_getscope,
	                   int (*)(const sctk_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_thread_generic, attr_setscope,
	                   int (*)(sctk_thread_attr_t *, int) );

	sctk_add_func_type(sctk_thread_generic, attr_getbinding,
	                   int (*)(sctk_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_thread_generic, attr_setbinding,
	                   int (*)(sctk_thread_attr_t *, int) );

	sctk_add_func_type(sctk_thread_generic, attr_getstacksize,
	                   int (*)(const sctk_thread_attr_t *, size_t *) );
	sctk_add_func_type(sctk_thread_generic, attr_setstacksize,
	                   int (*)(sctk_thread_attr_t *, size_t) );

	sctk_add_func_type(sctk_thread_generic, attr_getstackaddr,
	                   int (*)(const sctk_thread_attr_t *, void **) );
	sctk_add_func_type(sctk_thread_generic, attr_setstackaddr,
	                   int (*)(sctk_thread_attr_t *, void *) );

	sctk_add_func_type(sctk_thread_generic, attr_setstack,
	                   int (*)(sctk_thread_attr_t *, void *, size_t) );
	sctk_add_func_type(sctk_thread_generic, attr_getstack,
	                   int (*)(const sctk_thread_attr_t *, void **, size_t *) );

	sctk_add_func_type(sctk_thread_generic, attr_setguardsize,
	                   int (*)(sctk_thread_attr_t *, size_t) );
	sctk_add_func_type(sctk_thread_generic, attr_getguardsize,
	                   int (*)(const sctk_thread_attr_t *, size_t *) );

	sctk_add_func_type(sctk_thread_generic, attr_setschedparam,
	                   int (*)(sctk_thread_attr_t *, const struct sched_param *) );
	sctk_add_func_type(sctk_thread_generic, attr_getschedparam,
	                   int (*)(const sctk_thread_attr_t *, struct sched_param *) );

	sctk_add_func_type(sctk_thread_generic, attr_getschedpolicy,
	                   int (*)(const sctk_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_thread_generic, attr_setschedpolicy,
	                   int (*)(sctk_thread_attr_t *, int) );

	sctk_add_func_type(sctk_thread_generic, attr_setinheritsched,
	                   int (*)(sctk_thread_attr_t *, int) );
	sctk_add_func_type(sctk_thread_generic, attr_getinheritsched,
	                   int (*)(const sctk_thread_attr_t *, int *) );

	sctk_add_func_type(sctk_thread_generic, attr_setdetachstate,
	                   int (*)(sctk_thread_attr_t *, int) );
	sctk_add_func_type(sctk_thread_generic, attr_getdetachstate,
	                   int (*)(const sctk_thread_attr_t *, int *) );

	sctk_add_func_type(sctk_thread_generic, user_create,
	                   int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
	                           void *(*)(void *), void *) );
	sctk_add_func_type(sctk_thread_generic, create,
	                   int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
	                           void *(*)(void *), void *) );

	sctk_add_func_type(sctk_thread_generic, atfork,
	                   int (*)(void (*)(void),
	                           void (*)(void),
	                           void (*)(void) ) );
	sctk_add_func_type(sctk_thread_generic, sched_get_priority_min,
	                   int (*)(int) );
	sctk_add_func_type(sctk_thread_generic, sched_get_priority_max,
	                   int (*)(int) );

	sctk_add_func_type(sctk_thread_generic, getschedparam,
	                   int (*)(sctk_thread_t, int *, struct sched_param *) );
	sctk_add_func_type(sctk_thread_generic, setschedparam,
	                   int (*)(sctk_thread_t, int, const struct sched_param *) );

	/****** THREAD SIGNALS ******/
	sctk_add_func_type(sctk_thread_generic, sigsuspend,
	                   int (*)(sigset_t *) );
	sctk_add_func_type(sctk_thread_generic, sigpending,
	                   int (*)(sigset_t *) );
	sctk_add_func_type(sctk_thread_generic, sigmask,
	                   int (*)(int, const sigset_t *, sigset_t *) );
	/****** THREAD CANCEL ******/
	sctk_add_func_type(sctk_thread_generic, cancel,
	                   int (*)(sctk_thread_t) );
	sctk_add_func_type(sctk_thread_generic, setcancelstate,
	                   int (*)(int, int *) );
	sctk_add_func_type(sctk_thread_generic, setcanceltype,
	                   int (*)(int, int *) );
	sctk_add_func_type(sctk_thread_generic, setschedprio,
	                   int (*)(sctk_thread_t, int) );
	sctk_add_func_type(sctk_thread_generic, testcancel,
	                   void (*)() );

	/****** THREAD KILL ******/
	sctk_add_func_type(sctk_thread_generic, kill,
	                   int (*)(sctk_thread_t, int) );
	/****** THREAD POLLING ******/
	sctk_add_func(sctk_thread_generic, wait_for_value_and_poll);

	/****** THREAD ONCE ******/
	sctk_thread_generic_check_size(sctk_thread_once_t, sctk_thread_once_t);
	sctk_add_func_type(sctk_thread_generic, once,
	                   int (*)(sctk_thread_once_t *, void (*)(void) ) );

	/****** YIELD ******/
	__sctk_ptr_thread_yield = sctk_thread_generic_thread_sched_yield;

	sctk_multithreading_initialised = 1;

	sctk_thread_data_init();
	sctk_thread_generic_polling_init(vp_number);
}

/***************************************/
/* IMPLEMENTATION SPECIFIC             */
/***************************************/

int sctk_get_env_cpu_nuber()
{
	int   cpu_number;
	char *env;

	cpu_number = mpc_topology_get_pu_count();
	env        = getenv("SCTK_SET_CORE_NUMBER");
	if(env != NULL)
	{
		cpu_number = atoi(env);
		assume(cpu_number > 0);
	}
	return cpu_number;
}

/********* ETHREAD MXN ************/
void
sctk_ethread_mxn_ng_thread_init(void)
{
	mpc_common_get_flags()->new_scheduler_engine_enabled = 1;
	// sctk_thread_generic_thread_init
	// ("ethread_mxn","generic/multiple_queues",sctk_get_env_cpu_nuber());
	sctk_thread_generic_thread_init("ethread_mxn",
	                                "generic/multiple_queues_with_priority",
	                                sctk_get_env_cpu_nuber() );
}

/********* ETHREAD ************/
void
sctk_ethread_ng_thread_init(void)
{
	mpc_common_get_flags()->new_scheduler_engine_enabled = 1;
	sctk_thread_generic_thread_init("ethread_mxn", "generic/multiple_queues", 1);
}

/********* PTHREAD ************/
void
sctk_pthread_ng_thread_init(void)
{
	mpc_common_get_flags()->new_scheduler_engine_enabled = 1;
	sctk_thread_generic_thread_init("pthread", "generic/multiple_queues", sctk_get_env_cpu_nuber() );
}

#ifndef MPC_THREAD_CONFIG_H_
#define MPC_THREAD_CONFIG_H_

#include <mpc_config.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/***********
 * HELPERS *
 ***********/

#define TYPEDEF_STRUCT( size_macro, mpc_type ) typedef struct { char _data[size_macro]; } mpc_type

#if SIZEOF_LONG == 8
        #define sctk_long_long long
#else
        #define sctk_long_long long long
#endif

/**************
 * SEMAPHORES *
 **************/

#ifdef HAVE_SEMAPHORE_H

#include <semaphore.h>

TYPEDEF_STRUCT(SIZEOF_SEM_T, sctk_thread_sem_t);

#ifdef SCTK_SEM_VALUE_MAX
        #define SCTK_SEM_VALUE_MAX SEM_VALUE_MAX
#else
        #define SCTK_SEM_VALUE_MAX 2147483647
#endif

#define SCTK_SEM_FAILED SEM_FAILED

#endif

typedef void* sctk_thread_t;

/*************************
 * PTHREAD COMPATIBILITY *
 *************************/

#define SCTK_USE_PTHREAD_BARRIER


#ifdef HAVE_PTHREAD_H

#include <pthread.h>

/* Pthread Cond */

TYPEDEF_STRUCT(SIZEOF_PTHREAD_MUTEX_T, sctk_thread_mutex_t);
#define SCTK_THREAD_MUTEX_INITIALIZER { 0 }

#if SIZEOF_PTHREAD_MUTEXATTR_T == 4
        typedef int sctk_thread_mutexattr_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_MUTEXATTR_T, sctk_thread_mutexattr_t);
#endif

#define SCTK_THREAD_MUTEX_NORMAL PTHREAD_MUTEX_NORMAL
#define SCTK_THREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE
#define SCTK_THREAD_MUTEX_ERRORCHECK PTHREAD_MUTEX_ERRORCHECK
#define SCTK_THREAD_MUTEX_DEFAULT PTHREAD_MUTEX_DEFAULT

#if SIZEOF_PTHREAD_CONDATTR_T == 4
        typedef int sctk_thread_condattr_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_CONDATTR_T, sctk_thread_condattr_t);
#endif

TYPEDEF_STRUCT(SIZEOF_PTHREAD_COND_T, sctk_thread_cond_t);
#define SCTK_THREAD_COND_INITIALIZER { 0 }


#if SIZEOF_PTHREAD_ONCE_T == 4
        typedef int sctk_thread_once_t;
        #define SCTK_THREAD_ONCE_INIT 0
        #define sctk_thread_once_t_is_contiguous_int 1
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_ONCE_T, sctk_thread_once_t);
        #define SCTK_THREAD_ONCE_INIT { 0 }
#endif

TYPEDEF_STRUCT(SIZEOF_PTHREAD_ATTR_T, sctk_thread_attr_t);

#if SIZEOF_PTHREAD_BARRIERATTR_T == 4
        typedef int sctk_thread_barrierattr_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_BARRIERATTR_T, sctk_thread_barrierattr_t);
#endif

TYPEDEF_STRUCT(SIZEOF_PTHREAD_BARRIER_T, sctk_thread_barrier_t);

#ifdef SCTK_USE_PTHREAD_BARRIER
        #define SCTK_THREAD_BARRIER_SERIAL_THREAD PTHREAD_BARRIER_SERIAL_THREAD
#else
        #define SCTK_THREAD_BARRIER_SERIAL_THREAD -1
  sctk_define_arg_ndef (SCTK_THREAD_BARRIER_SERIAL_THREAD, -1);
#endif


#if SIZEOF_PTHREAD_SPINLOCK_T == 4
        typedef int sctk_thread_spinlock_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_SPINLOCK_T, sctk_thread_spinlock_t);
#endif

#if SIZEOF_PTHREAD_KEY_T == 4
        typedef int sctk_thread_key_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_KEY_T, sctk_thread_key_t);
#endif

#define SCTK_THREAD_KEYS_MAX MPC_THREAD_KEYS_MAX

#if SIZEOF_PTHREAD_ONCE_T == 4
        typedef int sctk_thread_once_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_ONCE_T, sctk_thread_once_t);
#endif

#if SIZEOF_PTHREAD_RWLOCKATTR_T == 8
        typedef sctk_long_long sctk_thread_rwlockattr_t;
#else
        TYPEDEF_STRUCT(SIZEOF_PTHREAD_RWLOCKATTR_T, sctk_thread_rwlockattr_t);
#endif

TYPEDEF_STRUCT(SIZEOF_PTHREAD_RWLOCK_T, sctk_thread_rwlock_t);
#define SCTK_THREAD_RWLOCK_INITIALIZER { 0 }

#define SCTK_THREAD_CREATE_JOINABLE PTHREAD_CREATE_JOINABLE
#define SCTK_THREAD_CREATE_DETACHED PTHREAD_CREATE_DETACHED
#define SCTK_THREAD_INHERIT_SCHED PTHREAD_INHERIT_SCHED
#define SCTK_THREAD_EXPLICIT_SCHED PTHREAD_EXPLICIT_SCHED
#define SCTK_THREAD_SCOPE_SYSTEM PTHREAD_SCOPE_SYSTEM
#define SCTK_THREAD_SCOPE_PROCESS PTHREAD_SCOPE_PROCESS
#define SCTK_THREAD_PROCESS_PRIVATE PTHREAD_PROCESS_PRIVATE
#define SCTK_THREAD_PROCESS_SHARED PTHREAD_PROCESS_SHARED
#define SCTK_THREAD_CANCEL_ENABLE PTHREAD_CANCEL_ENABLE
#define SCTK_THREAD_CANCEL_DISABLE PTHREAD_CANCEL_DISABLE
#define SCTK_THREAD_CANCEL_DEFERRED PTHREAD_CANCEL_DEFERRED
#define SCTK_THREAD_CANCEL_ASYNCHRONOUS PTHREAD_CANCEL_ASYNCHRONOUS
#define SCTK_THREAD_CANCELED (void*)PTHREAD_CANCELED

#define SCTK_SCHED_OTHER SCHED_OTHER
#define SCTK_SCHED_RR SCHED_RR
#define SCTK_SCHED_FIFO SCHED_FIFO

#ifdef PTHREAD_STACK_MIN
        #define SCTK_THREAD_STACK_MIN PTHREAD_STACK_MIN
#else
        #define SCTK_THREAD_STACK_MIN 8192
#endif

#endif

/*********************
 * NUMBER OF SIGNALS *
 *********************/

#include <signal.h>

#if defined(NSIG)
        #define SCTK_NSIG NSIG
#elif defined(_NSIG)
        #define SCTK_NSIG _NSIG
#else
    #if SIZEOF_SIGSET_T < 4
        #define SCTK_NSIG 32
    #else
        #define SCTK_NSIG (SIZEOF_SIGSET_T * 8)
    #endif
#endif

/*****************
 * MEMORY LIMITS *
 *****************/

#define SCTK_MAX_MEMORY_SIZE ((unsigned long)52776558132224)
#define SCTK_MAX_MEMORY_OFFSET ((unsigned long)2147483648)
#define SCTK_PAGE_SIZE (sysconf(_SC_PAGESIZE))

#endif /* MPC_THREAD_CONFIG_H_ */
/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_COMMON_INCLUDE_SCTK_DEBUG_H_
#define MPC_COMMON_INCLUDE_SCTK_DEBUG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <mpc_config.h>
#include <mpc_common_debugger.h>
#include <mpc_arch.h>
#include <assert.h>

#include "mpc_common_types.h"
#include "mpc_keywords.h"


/*********
* ABORT *
*********/

void mpc_common_debug_abort(void) __attribute__( (__noreturn__) );

/** Print an error message and exit. It use the print formatting convention.
 * **/
#define mpc_common_debug_fatal(...)                                                 \
	{                                                                           \
		mpc_common_debug_error("Fatal error at %s!%d", __FILE__, __LINE__); \
		mpc_common_debug_error(__VA_ARGS__);                                \
		mpc_common_debug_abort();                                           \
	}

void mpc_common_debug_abort_log(FILE *stream, const int line,
                                const char *file, const char *func,
                                const char *fmt, ...) __attribute__( (__noreturn__) );



void mpc_common_debug_check_size_equal(size_t a, size_t b, char *ca, char *cb,
                                       char *file, int line);


#define mpc_common_debug_check_type_equal(a, b)    mpc_common_debug_check_size_equal(sizeof(a), sizeof(b), MPC_STRING(a), MPC_STRING(b), __FILE__, __LINE__)

void mpc_common_debug_check_large_enough(size_t a, size_t b, char *ca, char *cb,
                                         char *file, int line);

#define MPC_COMMON_DEBUG_INFO    stderr, __LINE__, __FILE__, MPC_FUNCTION

#define bad_parameter(message, ...)     mpc_common_debug_abort_log(MPC_COMMON_DEBUG_INFO, message, __VA_ARGS__)

#define not_implemented()               mpc_common_debug_abort_log(MPC_COMMON_DEBUG_INFO, "Function not implemented!!!!!")

#define not_available()                 mpc_common_debug_abort_log(MPC_COMMON_DEBUG_INFO, "Function not available!!!!!")

#define not_reachable()                 mpc_common_debug_abort_log(MPC_COMMON_DEBUG_INFO, "Function not reachable!!!!!")

/*********************************
* LOGGING AND ERROR LEVEL PRINT *
*********************************/

void MPC_printf(const char *fmt, ...);

void mpc_common_debug_error(const char *fmt, ...);
void mpc_common_debug_warning(const char *fmt, ...);
void mpc_common_debug_log(const char *fmt, ...);
void mpc_common_debug_info(const char *fmt, ...);

#ifdef MPC_ENABLE_DEBUG_MESSAGES
void mpc_common_debug(const char *fmt, ...);

#else
	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define mpc_common_debug(fmt, ...)    (void)(0)
	#else
static inline void mpc_common_debug(const char *fmt, ...)
{
}
	#endif
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define mpc_common_nodebug(fmt, ...)    (void)(0)
#else
static inline void mpc_common_nodebug(const char *fmt, ...)
{
}
#endif

void mpc_common_debug_log_file(FILE *file, const char *fmt, ...);

/*****************
* TODO AND INFO *
*****************/

#if (defined HAVE_PRAGMA_MESSAGE) && (defined MPC_ENABLE_DEBUG_MESSAGES)

/* Add todo support (as stated in GCC doc
 * Supported since GCC 4.4.7 ignored in previous versions*/
#define DO_PRAGMA(x)    _Pragma( #x)

#define TODO(x)         DO_PRAGMA(message("TODO - " #x) )
#define INFO(x)         DO_PRAGMA(message("INFO - " #x) )
#else
#define TODO(x)
#define INFO(x)
#endif

/**********************************
* MANUAL BREAKPOINTING AND CRASH *
**********************************/

#define MPC_GDB_INTR()         \
	do                     \
	{                      \
		raise(SIGINT); \
	} while(0)

#define MPC_GDB_BREAKPOINT()                                                                            \
	do                                                                                              \
	{                                                                                               \
		char _hostname[300];                                                                    \
		gethostname(_hostname, 300);                                                            \
		int _block = 1;                                                                         \
		mpc_common_debug_error("Breakpoint set to %s:%d on %s", __FILE__, __LINE__, _hostname); \
		mpc_common_debug_error("You can trace/unblock this process by running under GDB:");     \
		mpc_common_debug_error("(gdb) attach %d", getpid() );                                   \
		mpc_common_debug_error("(gdb) p _block = 0");                                           \
		mpc_common_debug_error("(gdb) continue");                                               \
		while(_block){                                                                          \
			; }                                                                             \
	} while(0)

/* Some Debug Helpers */
#define MPC_CRASH()                                                                                              \
	do                                                                                                       \
	{                                                                                                        \
		if(getenv("MPC_DEBUG_CRASH") )                                                                   \
		{                                                                                                \
			mpc_common_debug_error("MPC will now create a \"breakpoint\" where the SIGSEGV occurs"); \
			MPC_GDB_BREAKPOINT();                                                                    \
		}                                                                                                \
		else                                                                                             \
		{                                                                                                \
			( (void (*)() ) 0x0)();                                                                  \
		}                                                                                                \
	} while(0)



/* DEBUG FUNCTIONS END */

#ifndef MPC_Debugger
	#define sctk_thread_add(a, b)                     (void)(0)
	#define sctk_thread_remove(a)                     (void)(0)
	#define sctk_enable_lib_thread_db()               (void)(0)

	#define sctk_init_thread_debug(a)                 (void)(0)
	#define sctk_refresh_thread_debug(a, b)           (void)(0)
	#define sctk_refresh_thread_debug_migration(a)    (void)(0)

	#define sctk_init_idle_thread_dbg(a, b)           (void)(0)
	#define sctk_free_idle_thread_dbg(a)              (void)(0)

	#define sctk_report_creation(a)                   (void)(0)
	#define sctk_report_death(a)                      (void)(0)
#else
	#include "sctk_thread_dbg.h"
#endif





//If inline is not supported, disable assertions

#define assume_m(x, ...)                                                              \
	if(!(x) )                                                                     \
	{                                                                             \
		mpc_common_debug_error("Error at %s!%d\n%s", __FILE__, __LINE__, #x); \
		mpc_common_debug_error(__VA_ARGS__);                                  \
		mpc_common_debug_abort();                                             \
	}


#ifndef SCTK_NO_INLINE
#undef NO_INTERNAL_ASSERT
#define NO_INTERNAL_ASSERT
#endif


void mpc_common_debug_assert_print(FILE *stream, const int line,
                                   const char *file, const char *func,
                                   const char *fmt, ...);


//for standard assert function, rewrite but maintain -DNDEBUG convention
#if !defined(NDEBUG) || !defined(NO_INTERNAL_ASSERT)
	#undef assert
	#define assert(op)                                                                        \
	do                                                                                        \
	{                                                                                         \
		if(expect_false(!(op) ) ){                                                        \
			mpc_common_debug_assert_print(MPC_COMMON_DEBUG_INFO, MPC_STRING(op) ); } \
	} while(0)
	#endif //NDEBUG, NO_INTERNAL_ASSERT

/** Assume stay present independently of NDEBUG/NO_INTERNAL_ASSERT **/
	#define assume(op)                                                                        \
	do                                                                                        \
	{                                                                                         \
		if(expect_false(!(op) ) ){                                                        \
			mpc_common_debug_assert_print(MPC_COMMON_DEBUG_INFO, MPC_STRING(op) ); } \
	} while(0)

	#ifdef NO_INTERNAL_ASSERT
		#define sctk_assert(op)         (void)(0)
		#define sctk_assert_func(op)    (void)(0)
	#else //NO_INTERNAL_ASSERT
		#define sctk_assert_func(op) \
	do                                   \
	{                                    \
		op                           \
	} while(0)

		#define sctk_assert(op)                              \
	if(expect_false(!(op) ) )                                    \
		mpc_common_debug_assert_print(MPC_COMMON_DEBUG_INFO, \
		                              MPC_STRING(op) )
#endif //NO_INTERNAL_ASSERT

#define mpc_common_debug_only_once()                                                                           \
	do                                                                                                     \
	{                                                                                                      \
		static int mpc_common_debug_only_once_initialized = 0;                                         \
		if(mpc_common_debug_only_once_initialized == 1)                                                \
		{                                                                                              \
			fprintf(stderr, "Multiple intialisation on line %d in file %s\n", __LINE__, __FILE__); \
			mpc_common_debug_abort();                                                              \
		}                                                                                              \
		mpc_common_debug_only_once_initialized++;                                                      \
	} while(0)

#ifdef __cplusplus
} /* end of extern "C" */
#endif
#endif
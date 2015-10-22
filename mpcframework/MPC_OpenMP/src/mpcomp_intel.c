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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpcomp_abi.h>
#include <sctk_debug.h>
#include "mpcomp_internal.h"

/* Declaration from MPC
TODO: put these declarations in header file
*/
void __mpcomp_internal_begin_parallel_region(int arg_num_threads,
		mpcomp_new_parallel_region_info_t info ) ;

void __mpcomp_internal_end_parallel_region( mpcomp_instance_t * instance ) ;

/*
 *
 * Target high-priority directives:
 *
 * ---------------
 * SIGNATURE ONLY:
 * ---------------
 *
 * __kmpc_for_static_init_4u
 * __kmpc_for_static_init_8u
 * __kmpc_flush
 *
 * __kmpc_set_lock
 * __kmpc_destroy_lock
 * __kmpc_init_lock
 * __kmpc_unset_lock
 *
 * __kmpc_begin
 * __kmpc_end
 * __kmpc_serialized_parallel
 * __kmpc_end_serialized_parallel
 *
 * __kmpc_atomic_fixed4_wr
 * __kmpc_atomic_fixed8_add
 * __kmpc_atomic_float8_add
 *
 * ---------------
 * IMPLEMENTATION:
 * ---------------
 *
 * __kmpc_for_static_init_8
 * __kmpc_for_static_init_4
 * __kmpc_for_static_fini
 *
 * __kmpc_dispatch_init_4
 * __kmpc_dispatch_next_4
 *
 * __kmpc_critical
 * __kmpc_end_critical
 *
 * __kmpc_single
 * __kmpc_end_single
 *
 * __kmpc_barrier
 *
 * __kmpc_push_num_threads
 * __kmpc_ok_to_fork
 * __kmpc_fork_call
 *
 * __kmpc_master
 * __kmpc_end_master
 *
 * __kmpc_atomic_fixed4_add
 *
 * __kmpc_global_thread_num
 * 
 * __kmpc_reduce
 * __kmpc_end_reduce
 * __kmpc_reduce_nowait
 * __kmpc_end_reduce_nowait
 *
 *
 */

/* KMP_OS.H */

typedef char               kmp_int8;
typedef unsigned char      kmp_uint8;
typedef short              kmp_int16;
typedef unsigned short     kmp_uint16;
typedef int                kmp_int32;
typedef unsigned int       kmp_uint32;
typedef long long          kmp_int64;
typedef unsigned long long kmp_uint64;


typedef float   kmp_real32;
typedef double  kmp_real64;

/* KMP.H */

typedef struct ident {
  kmp_int32 reserved_1 ;
  kmp_int32 flags ;
  kmp_int32 reserved_2 ;
  kmp_int32 reserved_3 ;
  char const *psource ;
} ident_t ;


typedef void (*kmpc_micro) (kmp_int32 * global_tid, kmp_int32 * bound_tid, ...) ;

enum sched_type {
    kmp_sch_lower                     = 32,   /**< lower bound for unordered values */
    kmp_sch_static_chunked            = 33,
    kmp_sch_static                    = 34,   /**< static unspecialized */
    kmp_sch_dynamic_chunked           = 35,
    kmp_sch_guided_chunked            = 36,   /**< guided unspecialized */
    kmp_sch_runtime                   = 37,
    kmp_sch_auto                      = 38,   /**< auto */
    kmp_sch_trapezoidal               = 39,

    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_sch_static_greedy             = 40,
    kmp_sch_static_balanced           = 41,
    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_sch_guided_iterative_chunked  = 42,
    kmp_sch_guided_analytical_chunked = 43,

    kmp_sch_static_steal              = 44,   /**< accessible only through KMP_SCHEDULE environment variable */

    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_sch_upper                     = 45,   /**< upper bound for unordered values */

    kmp_ord_lower                     = 64,   /**< lower bound for ordered values, must be power of 2 */
    kmp_ord_static_chunked            = 65,
    kmp_ord_static                    = 66,   /**< ordered static unspecialized */
    kmp_ord_dynamic_chunked           = 67,
    kmp_ord_guided_chunked            = 68,
    kmp_ord_runtime                   = 69,
    kmp_ord_auto                      = 70,   /**< ordered auto */
    kmp_ord_trapezoidal               = 71,
    kmp_ord_upper                     = 72,   /**< upper bound for ordered values */

#if OMP_40_ENABLED
    /* Schedules for Distribute construct */
    kmp_distribute_static_chunked     = 91,   /**< distribute static chunked */
    kmp_distribute_static             = 92,   /**< distribute static unspecialized */
#endif

    /*
     * For the "nomerge" versions, kmp_dispatch_next*() will always return
     * a single iteration/chunk, even if the loop is serialized.  For the
     * schedule types listed above, the entire iteration vector is returned
     * if the loop is serialized.  This doesn't work for gcc/gcomp sections.
     */
    kmp_nm_lower                      = 160,  /**< lower bound for nomerge values */

    kmp_nm_static_chunked             = (kmp_sch_static_chunked - kmp_sch_lower + kmp_nm_lower),
    kmp_nm_static                     = 162,  /**< static unspecialized */
    kmp_nm_dynamic_chunked            = 163,
    kmp_nm_guided_chunked             = 164,  /**< guided unspecialized */
    kmp_nm_runtime                    = 165,
    kmp_nm_auto                       = 166,  /**< auto */
    kmp_nm_trapezoidal                = 167,

    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_nm_static_greedy              = 168,
    kmp_nm_static_balanced            = 169,
    /* accessible only through KMP_SCHEDULE environment variable */
    kmp_nm_guided_iterative_chunked   = 170,
    kmp_nm_guided_analytical_chunked  = 171,
    kmp_nm_static_steal               = 172,  /* accessible only through OMP_SCHEDULE environment variable */

    kmp_nm_ord_static_chunked         = 193,
    kmp_nm_ord_static                 = 194,  /**< ordered static unspecialized */
    kmp_nm_ord_dynamic_chunked        = 195,
    kmp_nm_ord_guided_chunked         = 196,
    kmp_nm_ord_runtime                = 197,
    kmp_nm_ord_auto                   = 198,  /**< auto */
    kmp_nm_ord_trapezoidal            = 199,
    kmp_nm_upper                      = 200,  /**< upper bound for nomerge values */

    kmp_sch_default = kmp_sch_static  /**< default scheduling algorithm */
};

typedef kmp_int32 kmp_critical_name[8] ;



/*!
 * Values for bit flags used in the ident_t to describe the fields.
 * */
/*! Use trampoline for internal microtasks */
#define KMP_IDENT_IMB             0x01
/*! Use c-style ident structure */
#define KMP_IDENT_KMPC            0x02
/* 0x04 is no longer used */
/*! Entry point generated by auto-parallelization */
#define KMP_IDENT_AUTOPAR         0x08
/*! Compiler generates atomic reduction option for kmpc_reduce* */
#define KMP_IDENT_ATOMIC_REDUCE   0x10
/*! To mark a 'barrier' directive in user code */
#define KMP_IDENT_BARRIER_EXPL    0x20
/*! To Mark implicit barriers. */
#define KMP_IDENT_BARRIER_IMPL           0x0040
#define KMP_IDENT_BARRIER_IMPL_MASK      0x01C0
#define KMP_IDENT_BARRIER_IMPL_FOR       0x0040
#define KMP_IDENT_BARRIER_IMPL_SECTIONS  0x00C0

#define KMP_IDENT_BARRIER_IMPL_SINGLE    0x0140
#define KMP_IDENT_BARRIER_IMPL_WORKSHARE 0x01C0

/*******************************
  * REDUCTION
  *****************************/

/* different reduction cases */
enum _reduction_method 
{
    reduction_method_not_defined = 0,
    critical_reduce_block        = ( 1 << 8 ),
    atomic_reduce_block          = ( 2 << 8 ),
    tree_reduce_block            = ( 3 << 8 ),
    empty_reduce_block           = ( 4 << 8 )
};

int __kmp_force_reduction_method = reduction_method_not_defined;

#define FAST_REDUCTION_ATOMIC_METHOD_GENERATED ( ( loc->flags & ( KMP_IDENT_ATOMIC_REDUCE ) ) == ( KMP_IDENT_ATOMIC_REDUCE ) )
#define FAST_REDUCTION_TREE_METHOD_GENERATED   ( ( reduce_data ) && ( reduce_func ) )

/*******************************
  * THREADPRIVATE
  ******************************/
/* keeps tracked of threadprivate cache allocations for cleanup later */
typedef void    (*microtask_t)( int *gtid, int *npr, ... );
typedef void *(* kmpc_ctor)(void *) ;
typedef void  (* kmpc_dtor)(void *) ;
typedef void *(* kmpc_cctor)(void *, void *) ;
typedef void *(* kmpc_ctor_vec)(void *, size_t) ;
typedef void  (* kmpc_dtor_vec)(void *, size_t) ;
typedef void *(* kmpc_cctor_vec)(void *, void *, size_t) ;

typedef struct kmp_cached_addr {
    void                      **addr;           /* address of allocated cache */
    struct kmp_cached_addr     *next;           /* pointer to next cached address */
} kmp_cached_addr_t;

kmp_cached_addr_t  *__kmp_threadpriv_cache_list = NULL; /* list for cleaning */

struct private_common {
    struct private_common     *next;
    struct private_common     *link;
    void                      *gbl_addr;
    void                      *par_addr;        /* par_addr == gbl_addr for MASTER thread */
    size_t                     cmn_size;
};

struct shared_common
{
    struct shared_common      *next;
    struct private_data       *pod_init;
    void                      *obj_init;
    void                      *gbl_addr;
    union {
        kmpc_ctor              ctor;
        kmpc_ctor_vec          ctorv;
    } ct;
    union {
        kmpc_cctor             cctor;
        kmpc_cctor_vec         cctorv;
    } cct;
    union {
        kmpc_dtor              dtor;
        kmpc_dtor_vec          dtorv;
    } dt;
    size_t                     vec_len;
    int                        is_vec;
    size_t                     cmn_size;
};

struct shared_table {
    struct shared_common *data[ KMP_HASH_TABLE_SIZE ];
};

struct shared_table __kmp_threadprivate_d_table;

/* MY STRUCTS */

typedef struct wrapper {
  microtask_t f ;
  int argc ;
  void ** args ;
} wrapper_t ;


/********************************
  * FUNCTIONS
  *******************************/

extern int __kmp_xchg_fixed32( volatile int * p, int d ) ;
extern int __kmp_test_then_add32( volatile int * addr, int data ) ;
extern long __kmp_test_then_add64( volatile long * addr, long data ) ;
extern double __kmp_test_then_add_real64( volatile double *addr, double data );
extern int __kmp_invoke_microtask(kmpc_micro pkfn, int gtid, int npr, int argc, void *argv[] );

void *
wrapper_function( void * arg ) 
{
    int rank;
    wrapper_t * w;

    rank = mpcomp_get_thread_num();
    w = (wrapper_t *) arg;

    __kmp_invoke_microtask( w->f, rank, rank, w->argc, w->args );

    return NULL ;
}

/********************************
  * STARTUP AND SHUTDOWN
  *******************************/

void   
__kmpc_begin ( ident_t * loc, kmp_int32 flags ) 
{
// not_implemented() ;
}

void   
__kmpc_end ( ident_t * loc )
{
// not_implemented() ;
}


/********************************
  * PARALLEL FORK/JOIN 
  *******************************/

kmp_int32
__kmpc_ok_to_fork(ident_t * loc)
{
  sctk_nodebug( "__kmpc_ok_to_fork: entering..." ) ;
  return 1 ;
}

/* TEMP */
#if 0
int intel_temp_argc = 10 ;
void ** intel_temp_args_copy = NULL ;
#endif

void
__kmpc_fork_call(ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ...)
{
  va_list args ;
  int i ;
  void ** args_copy ;
  wrapper_t w ;
  mpcomp_thread_t *t;

  sctk_nodebug( "__kmpc_fork_call: entering w/ %d arg(s)...", argc ) ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab info on the current thread */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

// fprintf( stderr, "__kmpc_fork_call: parallel region w/ %d args\n", argc ) ;

#if 0
if ( intel_temp_args_copy == NULL ) 
{
intel_temp_args_copy = (void **)malloc( intel_temp_argc * sizeof( void * ) ) ;
}
if ( argc <= intel_temp_argc ) 
{
args_copy = intel_temp_args_copy ;
} else 
{
  args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
  sctk_assert( args_copy ) ;
}
#else
  args_copy = (void **)malloc( argc * sizeof( void * ) ) ;
  sctk_assert( args_copy ) ;
#endif

  va_start( args, microtask ) ;

  for ( i = 0 ; i < argc ; i++ ) {
    args_copy[i] = va_arg( args, void * ) ;
  }

  va_end(args) ;

  w.f = microtask ;
  w.argc = argc ;
  w.args = args_copy ;

  __mpcomp_start_parallel_region( t->push_num_threads, wrapper_function, &w ) ;

  /* restore the number of threads w/ num_threads clause */
  t->push_num_threads = -1 ;
}

void
__kmpc_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_nodebug( "__kmpc_serialized_parallel: entering (%d) ...", global_tid ) ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	info.func = NULL ; /* No function to call */
	info.shared = NULL ;
	info.num_threads = 1 ;
	info.new_root = NULL ;
	info.icvs = t->info.icvs ; 
	info.combined_pragma = MPCOMP_COMBINED_NONE;

	__mpcomp_internal_begin_parallel_region( 1, info ) ;

	/* Save the current tls */
	t->children_instance->mvps[0]->threads[0].father = sctk_openmp_thread_tls ;

	/* Switch TLS to nested thread for region-body execution */
	sctk_openmp_thread_tls = &(t->children_instance->mvps[0]->threads[0]) ;
}

void
__kmpc_end_serialized_parallel(ident_t *loc, kmp_int32 global_tid) 
{
  mpcomp_thread_t * t ;

  sctk_nodebug( "__kmpc_end_serialized_parallel: entering (%d)...", global_tid ) ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	/* Restore the previous thread info */
	sctk_openmp_thread_tls = t->father ;

	__mpcomp_internal_end_parallel_region( t->instance ) ;

}

void
__kmpc_push_num_threads( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_threads )
{
  mpcomp_thread_t * t ;

  sctk_nodebug( "__kmpc_push_num_threads: pushing %d thread(s)", num_threads ) ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_assert( t->push_num_threads == -1 ) ;

  t->push_num_threads = num_threads ;
}

void 
__kmpc_push_num_teams(ident_t *loc, kmp_int32 global_tid, kmp_int32 num_teams,
    kmp_int32 num_threads )
{
  not_implemented() ;
}

void
__kmpc_fork_teams( ident_t * loc, kmp_int32 argc, kmpc_micro microtask, ... )
{
  not_implemented() ;
}

/********************************
  * SYNCHRONIZATION 
  *******************************/

kmp_int32
__kmpc_global_thread_num(ident_t * loc)
{
  sctk_nodebug( "__kmpc_global_thread_num: " ) ;
  return mpcomp_get_thread_num() ;
}

kmp_int32
__kmpc_global_num_threads(ident_t * loc)
{
  sctk_nodebug( "__kmpc_global_num_threads: " ) ;
  return mpcomp_get_num_threads() ;
}

kmp_int32
__kmpc_bound_thread_num(ident_t * loc)
{
  sctk_nodebug( "__kmpc_bound_thread_num: " ) ;
  return mpcomp_get_thread_num() ;
}

kmp_int32
__kmpc_bound_num_threads(ident_t * loc)
{
  sctk_nodebug( "__kmpc_bound_num_threads: " ) ;
  return mpcomp_get_num_threads() ;
}

kmp_int32
__kmpc_in_parallel(ident_t * loc)
{
  sctk_nodebug( "__kmpc_in_parallel: " ) ;
  return mpcomp_in_parallel() ;
}


/********************************
  * SYNCHRONIZATION 
  *******************************/

void
__kmpc_flush(ident_t *loc, ...) 
{
  /* TODO depending on the compiler, call the right function
   * Maybe need to do the same for mpcomp_flush...
   */
  __sync_synchronize() ;
}

void
__kmpc_barrier(ident_t *loc, kmp_int32 global_tid) 
{
  sctk_nodebug( "[%d] __kmpc_barrier: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;

  __mpcomp_barrier() ;
}

kmp_int32
__kmpc_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;

  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }

  __mpcomp_barrier() ;

  return 0 ;
}

void
__kmpc_end_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;

  __mpcomp_barrier() ;
}

kmp_int32
__kmpc_barrier_master_nowait(ident_t *loc, kmp_int32 global_tid)
{
  not_implemented() ;

  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }

  return 0 ;
}

int __kmp_determine_reduction_method( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, 
                                      size_t reduce_size, void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
                                      kmp_critical_name *lck, mpcomp_thread_t * t )
{
    int retval = critical_reduce_block;
    int team_size;
    int teamsize_cutoff = 4;
    team_size = t->instance->team->info.num_threads;
    
    if (team_size == 1)
    {
        return empty_reduce_block;
    }
    else
    {
        int atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
        int tree_available   = FAST_REDUCTION_TREE_METHOD_GENERATED;
        
        if( tree_available ) 
        {
            if( team_size <= teamsize_cutoff ) 
            {
                if ( atomic_available ) 
                {
                    retval = atomic_reduce_block;
                }
            } 
            else 
            {
                retval = tree_reduce_block;
            }
        } 
        else if ( atomic_available ) 
        {
            retval = atomic_reduce_block;
        }
    }
    
    /* force reduction method */
    if( __kmp_force_reduction_method != reduction_method_not_defined ) 
    {
        int forced_retval;
        int atomic_available, tree_available;
        switch( ( forced_retval = __kmp_force_reduction_method ) )
        {
            case critical_reduce_block:
                sctk_assert(lck);
                if(team_size <= 1)
                    forced_retval = empty_reduce_block;
            break;
            case atomic_reduce_block:
                atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
                sctk_assert(atomic_available);
            break;
            case tree_reduce_block:
                tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;
                sctk_assert(tree_available);
                forced_retval = tree_reduce_block;
            break;
            default:
                forced_retval = critical_reduce_block;
                sctk_debug("Unknown reduction method");
        }
        retval = forced_retval;
    }
    return retval;
}

kmp_int32 
__kmpc_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
    int retval = 0;
    int packed_reduction_method;
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

    /* get reduction method */
    packed_reduction_method = __kmp_determine_reduction_method( loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t);
    t->reduction_method = packed_reduction_method; 
    if( packed_reduction_method == critical_reduce_block ) 
    {
        __mpcomp_anonymous_critical_begin() ;
        retval = 1;
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
        retval = 1;
    }
    else if( packed_reduction_method == atomic_reduce_block )
    {
        retval = 2;
    }
    else if ( packed_reduction_method == tree_reduce_block )
    {
        __mpcomp_anonymous_critical_begin() ;
        retval = 1;
    }
    else
    {
        sctk_error("__kmpc_reduce_nowait: Unexpected reduction method %d", packed_reduction_method);
        sctk_abort();
    }
    return retval;
}

void 
__kmpc_end_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck ) 
{
    int packed_reduction_method;

    /* get reduction method */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    
    packed_reduction_method = t->reduction_method;
    
    if( packed_reduction_method == critical_reduce_block )
    {
        __mpcomp_anonymous_critical_end() ;
        __mpcomp_barrier();
    } 
    else if( packed_reduction_method == empty_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == atomic_reduce_block ) 
    {
    } 
    else if( packed_reduction_method == tree_reduce_block ) 
    {
        __mpcomp_anonymous_critical_end() ;
    } 
    else 
    {
        sctk_error("__kmpc_end_reduce_nowait: Unexpected reduction method %d", packed_reduction_method);
        sctk_abort();
    }
}

kmp_int32 
__kmpc_reduce( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck ) 
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /*
     TODO
     lck!=NULL ==> CRITICAL version
     reduce_data!=NULL && reduce_func!=NULL ==> TREE version
     (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE ==> ATOMIC version

     Need to add one variable to remember the choice taken.
     This variable will be written here and read in the end_reduce function

     To be compliant, we execute only the CRITICAL version which is supposed to be available
     in every situation.
   */


  sctk_debug( "[%d] __kmpc_reduce %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;

#if 0
  printf( "[%d] __kmpc_reduce %d var(s) of size %ld, "
      "CRITICAL ? %d, TREE ? %d, ATOMIC ? %d\n",
      t->rank, num_vars, reduce_size, 
      (lck!=NULL), (reduce_data!=NULL && reduce_func!=NULL), 
      ( (loc->flags & KMP_IDENT_ATOMIC_REDUCE) == KMP_IDENT_ATOMIC_REDUCE )
      
      ) ;
#endif

#if 0
  /* Version with atomic operation */
  return 2 ;
#endif

  /* Version with critical section */
  sctk_assert( lck != NULL ) ;
  __mpcomp_anonymous_critical_begin() ;
  return 1 ;
}

void 
__kmpc_end_reduce( ident_t *loc, kmp_int32 global_tid, kmp_critical_name *lck )
{

  sctk_debug( "[%d] __kmpc_end_reduce",
      ( (mpcomp_thread_t *) sctk_openmp_thread_tls )->rank ) ;
#if 0
  /* Version with atomic operation */
  return ;
#endif


#if 1
  /* Version with critical section */
  sctk_assert( lck != NULL ) ;
  __mpcomp_anonymous_critical_end() ;
  __mpcomp_barrier();
  return ;
#endif

  // not_implemented() ;
}



/********************************
  * WORK_SHARING
  *******************************/

kmp_int32
__kmpc_master(ident_t *loc, kmp_int32 global_tid)
{
  mpcomp_thread_t * t ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_nodebug( "[%d] __kmp_master: entering",
      t->rank ) ;

  if ( t->rank == 0 ) {
    return 1 ;
  }
  return 0 ;

}

void
__kmpc_end_master(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_end_master: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void 
__kmpc_ordered(ident_t *loc, kmp_int32 global_tid)
{
    __mpcomp_ordered_begin();
}

void
__kmpc_end_ordered(ident_t *loc, kmp_int32 global_tid)
{
    __mpcomp_ordered_end();
}

void
__kmpc_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  sctk_nodebug( "[%d] __kmpc_critical: enter %p (%d,%d,%d,%d,%d,%d,%d,%d)",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      crit,
      (*crit)[0], (*crit)[1], (*crit)[2], (*crit)[3],
      (*crit)[4], (*crit)[5], (*crit)[6], (*crit)[7]
      ) ;
  /* TODO Handle named critical */
__mpcomp_anonymous_critical_begin () ;
}

void
__kmpc_end_critical(ident_t *loc, kmp_int32 global_tid, kmp_critical_name *crit)
{
  /* TODO Handle named critical */
  __mpcomp_anonymous_critical_end () ;
}

kmp_int32
__kmpc_single(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_single: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;

  return __mpcomp_do_single() ;
}

void
__kmpc_end_single(ident_t *loc, kmp_int32 global_tid)
{
  sctk_nodebug( "[%d] __kmpc_end_single: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank  ) ;
}

void
__kmpc_for_static_init_4( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int32 * plower, kmp_int32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
  long from, to ;
  mpcomp_thread_t *t;
  int res ;
  //save old pupper
  kmp_int32 pupper_old = *pupper;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);   

  fprintf(stderr, "[%d] __kmpc_for_static_init_4: "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d, num_threads = %d\n"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride, t->instance->team->info.num_threads
      ) ;

  sctk_nodebug( "[%d] __kmpc_for_static_init_4: <%s> "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d *plastiter=%d *pstride=%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
      loc->psource,
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk, *plastiter, *pstride
      ) ;

  switch( schedtype ) {
    case kmp_sch_static:

      /* Get the single chunk for the current thread */
      res = __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr,
	  &from, &to ) ;

      /* Chunk to execute? */
      if ( res ) {
      sctk_nodebug( "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static): "
	  "%ld -> %ld excl %ld incl [%d]"
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  from, to, to-incr, incr
	  ) ;

      /* Remarks:
	 - MPC chunk has not-inclusive upper bound while Intel runtime includes
	 upper bound for calculation 
	 - Need to cast from long to int because MPC handles everything has a long
	 (like GCC) while Intel creates different functions
	 */
      *plower=(kmp_int32)from ;
      *pupper=(kmp_int32)to-incr;

      } else {
	/* No chunk */
	*pupper=*pupper+incr;
	*plower=*pupper;
      }

      //* TODO what about pstride and plastiter? */
      // *pstride = incr ;
      // *plastiter = 1 ;
      
      *pstride = *pupper - *plower;

      if(*pupper == pupper_old)
          *plastiter = 1;
      else
          *plastiter = 0;
      
      break ;
    case kmp_sch_static_chunked:

      sctk_assert( chunk >= 1 ) ;


      // span = chunk * incr;
      *pstride = (chunk * incr) * t->info.num_threads ;
      *plower = *plower + ((chunk * incr)* t->rank );
      *pupper = *plower + (chunk * incr) - incr;

      if(*pupper == pupper_old)
          *plastiter = 1;
      else
          *plastiter = 0;

      /* __mpcomp_static_schedule_get_specific_chunk( *plower, *pupper+incr, incr,
	  chunk, 0, &from, &to ) ;
	  */

      sctk_nodebug( "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static_chunked): "
	  "%ld -> %ld excl %ld incl [%d]"
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  *plower, *pupper, *pupper-incr, incr
	  ) ;

      /* Remarks:
	 - MPC chunk has not-inclusive upper bound while Intel runtime includes
	 upper bound for calculation 
	 - Need to cast from long to int because MPC handles everything has a long
	 (like GCC) while Intel creates different functions
	 */
      // *plower=(kmp_int32)from ;
      // *pupper=(kmp_int32)to-incr;

      /* TODO what should we do w/ plastiter? */
      /* TODO what if the number of chunk is > 1? */

      // not_implemented() ;
      break ;
    default:
      not_implemented() ;
      break ;
  } 
      fprintf(stderr, "[%d] Results for __kmpc_for_static_init_4 (kmp_sch_static_chunked): "
	  "%ld -> %ld excl %ld incl [%d] => plastiter = %d\n"
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  *plower, *pupper, *pupper-incr, *pstride, *plastiter
	  ) ;
}

void
__kmpc_for_static_init_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_uint32 * plower, kmp_uint32 * pupper,
    kmp_int32 * pstride, kmp_int32 incr, kmp_int32 chunk ) 
{
  not_implemented() ;
}

void
__kmpc_for_static_init_8( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_int64 * plower, kmp_int64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
  long from, to ;
  mpcomp_thread_t *t;
  int res ;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);   

  sctk_nodebug( "[%d] __kmpc_for_static_init_8: "
      "schedtype=%d, %d? %ld -> %ld incl. [%ld], incr=%ld chunk=%ld "
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk
      ) ;

  switch( schedtype ) {
    case kmp_sch_static:

      /* Get the single chunk for the current thread */
      res = __mpcomp_static_schedule_get_single_chunk( *plower, *pupper+incr, incr,
	  &from, &to ) ;

      if ( res ) {
	sctk_nodebug( "[%d] Results for __kmpc_for_static_init_8: "
	    "%ld -> %ld [%ld]"
	    ,
	    ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	    from, to, incr
	    ) ;

	/* Remarks:
	   - MPC chunk has not-inclusive upper bound while Intel runtime includes
	   upper bound for calculation 
	   - Need to case from long to int because MPC handles everything has a long
	   (like GCC) while Intel creates different functions
	   */
	*plower=(kmp_int64)from ;
	*pupper=(kmp_int64)to-incr;
      } else {
	/* No chunk */
	*pupper=*pupper+incr;
	*plower=*pupper;
      }
      // *pstride = incr ;
      // *plastiter = 1 ;
      break ;
    case kmp_sch_static_chunked:

      sctk_assert( chunk >= 1 ) ;

      *pstride = (chunk * incr) * t->info.num_threads ;
      *plower = *plower + ((chunk * incr)* t->rank );
      *pupper = *plower + (chunk * incr) - incr;


      sctk_nodebug( "[%d] Results for __kmpc_for_static_init_8 (kmp_sch_static_chunked): "
	  "%ld -> %ld excl %ld incl [%d]"
	  ,
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, 
	  *plower, *pupper, *pupper-incr, incr
	  ) ;

      /* Remarks:
	 - MPC chunk has not-inclusive upper bound while Intel runtime includes
	 upper bound for calculation 
	 - Need to cast from long to int because MPC handles everything has a long
	 (like GCC) while Intel creates different functions
	 */

      /* TODO what should we do w/ plastiter? */
      /* TODO what if the number of chunk is > 1? */

      break ;
    default:
      not_implemented() ;
      break ;
  } 

}

void
__kmpc_for_static_init_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 * plastiter, kmp_uint64 * plower, kmp_uint64 * pupper,
    kmp_int64 * pstride, kmp_int64 incr, kmp_int64 chunk ) 
{
  /* TODO: the same as unsigned long long in GCC... */

  sctk_nodebug( "__kmpc_for_static_init_8u: siweof long = %d, sizeof long long %d",
      sizeof( long ), sizeof( long long ) ) ;

  not_implemented() ;
}

void
__kmpc_for_static_fini( ident_t * loc, kmp_int32 global_tid ) 
{
  sctk_nodebug( "[%d] __kmpc_for_static_fini: entering...",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank ) ;

  /* Nothing to do here... */
}

void 
__kmpc_dispatch_init_4(ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
    kmp_int32 lb, kmp_int32 ub, kmp_int32 st, kmp_int32 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_4: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
    break ;
    default:
      sctk_error("schedule %d not handled\n", schedule);
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_4u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_uint32 lb, kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_4u: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
    break ;
    default:
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_8( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                        kmp_int64 lb, kmp_int64 ub,
                        kmp_int64 st, kmp_int64 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_8: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, 0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  lb, ub+st, st, chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	 lb, ub+st, st, chunk ) ;
    break ;
    default:
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}

void
__kmpc_dispatch_init_8u( ident_t *loc, kmp_int32 gtid, enum sched_type schedule,
                         kmp_uint64 lb, kmp_uint64 ub,
                         kmp_int64 st, kmp_int64 chunk )
{
    sctk_debug(
      "[%d] __kmpc_dispatch_init_8u: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d"
      ,
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank,
      lb, ub, ub+st, st, chunk, schedule ) ;
    
    /* save the scheduling type */
    mpcomp_thread_t * t ;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;
    t->schedule_type = schedule;
    t->static_chunk_size_intel = 1; /* initialize default nb_chunks */

  switch( schedule ) {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
    case kmp_ord_dynamic_chunked:
      __mpcomp_dynamic_loop_init( 
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
      break ;
    case kmp_sch_static:
    case kmp_ord_static:
    case kmp_sch_auto:
    case kmp_ord_auto:
      t->static_chunk_size_intel = 0;
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)0) ;
      break ; 
    case kmp_sch_static_chunked:
    case kmp_ord_static_chunked:
      __mpcomp_static_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk) ;
      break ;
    case kmp_sch_guided_chunked:
    case kmp_ord_guided_chunked:
      __mpcomp_dynamic_loop_init(
	  ((mpcomp_thread_t *) sctk_openmp_thread_tls),
	  (long)lb, (long)ub+(long)st, (long)st, (long)chunk ) ;
    break ;
    default:
      not_implemented() ;
      break ;
  }
  t->first_iteration = 1;
}



int
__kmpc_dispatch_next_4( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int32 *p_lb, kmp_int32 *p_ub, kmp_int32 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_int32) from ;
  *p_ub = (kmp_int32)to - (kmp_int32)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_4u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint32 *p_lb, kmp_uint32 *p_ub, kmp_int32 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_uint32) from ;
  *p_ub = (kmp_uint32)to - (kmp_uint32)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_8( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_int64 *p_lb, kmp_int64 *p_ub, kmp_int64 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_int64) from ;
  *p_ub = (kmp_int64)to - (kmp_int64)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

int
__kmpc_dispatch_next_8u( ident_t *loc, kmp_int32 gtid, kmp_int32 *p_last,
                        kmp_uint64 *p_lb, kmp_uint64 *p_ub, kmp_int64 *p_st )
{
  int ret, schedule ;
  mpcomp_thread_t *t ;
  long from, to ;
    
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert(t != NULL);
  /* get scheduling type used in kmpc_dispatch_init */
  schedule = t->schedule_type;
  
  /* Fix for intel */
  if(t->first_iteration && t->static_chunk_size_intel > 0)
    t->static_current_chunk = -1;

  sctk_nodebug("__kmpc_dispatch_next_4: p_lb %ld, p_ub %ld, p_st %ld", *p_lb, *p_ub, *p_st);
  switch( schedule ) 
  {
    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        ret = __mpcomp_dynamic_loop_next( &from, &to ) ;
    break;
    case kmp_sch_static:
    case kmp_sch_auto:
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_static_loop_next( &from, &to );
    break;
    case kmp_sch_static_chunked:
        ret = __mpcomp_static_loop_next( &from, &to ) ;
    break;
    case kmp_sch_guided_chunked:
        ret = __mpcomp_guided_loop_next( &from, &to ) ;
    break ;
    /* ordered scheduling */
    case kmp_ord_dynamic_chunked:
        ret = __mpcomp_ordered_dynamic_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_static:
    case kmp_ord_auto:
    case kmp_ord_static_chunked:
        /* handle intel case for default chunk */
        if(t->static_chunk_size_intel == 0 && t->first_iteration)
            ret = __mpcomp_static_schedule_get_single_chunk ((long)(*p_lb), (long)((*p_ub)+(*p_st)), (long)(*p_st), &from, &to);
        else
            ret = __mpcomp_ordered_static_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break;
    case kmp_ord_guided_chunked:
        ret = __mpcomp_ordered_guided_loop_next( &from, &to ) ;
        t->current_ordered_iteration = from;
    break ;
    default:
        not_implemented();
    break;
  }
  
  /* we passed first iteration */
  if(t->first_iteration)
      t->first_iteration = 0;

  sctk_nodebug( 
      "[%d] __kmpc_dispatch_next_4: %ld -> %ld excl, %ld incl [%d] (ret=%d)",
      t->rank,
      from, to, to - t->info.loop_incr, *p_st, ret ) ;
  
  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel) */
  *p_lb = (kmp_uint64) from ;
  *p_ub = (kmp_uint64)to - (kmp_uint64)t->info.loop_incr ;

  if ( ret == 0 ) {
      switch( schedule )   
      {
        //regular scheduling
        case kmp_sch_dynamic_chunked:
            __mpcomp_dynamic_loop_end_nowait() ;
        break;
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_static_chunked:
            __mpcomp_static_loop_end_nowait() ;
        break;
        case kmp_sch_guided_chunked:
            __mpcomp_guided_loop_end_nowait() ;
        break ;
        //ordered scheduling
        case kmp_ord_dynamic_chunked:
            __mpcomp_ordered_dynamic_loop_end_nowait() ;
        break;
        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
            __mpcomp_ordered_static_loop_end_nowait() ;
        break;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
        break ;
        default:
            not_implemented();
        break;
      }
  }

  return ret ;
}

void
__kmpc_dispatch_fini_4( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_4 */
}

void
__kmpc_dispatch_fini_8( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_4u */
}

void
__kmpc_dispatch_fini_4u( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_8 */
}

void
__kmpc_dispatch_fini_8u( ident_t *loc, kmp_int32 gtid )
{
    /* Nothing to do here, the end is handled by kmp_dispatch_next_8u */
}



/********************************
  * THREAD PRIVATE DATA SUPPORT
  *******************************/
struct private_common *
__kmp_threadprivate_find_task_common( struct common_table *tbl, int gtid, void *pc_addr )

{
    struct private_common *tn;
    for (tn = tbl->data[ KMP_HASH(pc_addr) ]; tn; tn = tn->next) 
    {
        if (tn->gbl_addr == pc_addr) 
        {
            return tn;
        }   
    }
    return 0;
}

struct private_common *
kmp_threadprivate_insert( int gtid, void *pc_addr, void *data_addr, size_t pc_size )
{
    struct private_common *tn, **tt;
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    mpcomp_thread_t *t ; 
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

    /* critical section */
    sctk_thread_mutex_lock (&lock);
        tn = (struct private_common *) sctk_malloc( sizeof (struct private_common) );
        tn->gbl_addr = pc_addr;
        tn->cmn_size = pc_size;
        tn->par_addr = (void *) sctk_malloc( tn->cmn_size );
        memcpy(tn->par_addr, pc_addr, pc_size);
    sctk_thread_mutex_unlock (&lock);
    /* end critical section */
    
    tt = &(t->th_pri_common->data[ KMP_HASH(pc_addr) ]);
    tn->next = *tt;
    *tt = tn;
    tn->link = t->th_pri_head;
    t->th_pri_head = tn;
    return tn;
}

void * 
__kmpc_threadprivate ( ident_t *loc, kmp_int32 global_tid, void * data, size_t size )
{
    void *ret = NULL;
    struct private_common *tn;
    mpcomp_thread_t *t ; 
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

    tn = __kmp_threadprivate_find_task_common( t->th_pri_common, global_tid, data );
    if (!tn)
    {
        tn = kmp_threadprivate_insert( global_tid, data, data, size );
    }
    
    ret = tn->par_addr;
    
    return ret ;
}

void
__kmpc_copyprivate(ident_t *loc, kmp_int32 global_tid, size_t cpy_size, void *cpy_data,
    void (*cpy_func)(void *, void *), kmp_int32 didit )
{
    mpcomp_thread_t *t ;    /* Info on the current thread */

    void **data_ptr;
    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

    /* In this flow path, the number of threads should not be 1 */
    sctk_assert( t->info.num_threads > 0 ) ;

    /* Grab the team info */
    sctk_assert( t->instance != NULL ) ;
    sctk_assert( t->instance->team != NULL ) ;
    
    data_ptr = &(t->instance->team->single_copyprivate_data);
    if (didit) *data_ptr = cpy_data;
    
    __mpcomp_barrier();

    if (! didit) (*cpy_func)( cpy_data, *data_ptr );

    __mpcomp_barrier();
}

void *
__kmpc_threadprivate_cached( ident_t *loc, kmp_int32 global_tid, void *data, size_t size, void *** cache)
{  
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  if (*cache == 0)
  {
    sctk_thread_mutex_lock (&lock);
    if (*cache == 0)
    {
        //handle cache to be dealt with later
        void ** my_cache;
        my_cache = (void**) malloc(sizeof( void * ) * 8 + sizeof ( kmp_cached_addr_t ));
        kmp_cached_addr_t *tp_cache_addr;
        tp_cache_addr = (kmp_cached_addr_t *) & my_cache[8];
        tp_cache_addr -> addr = my_cache;
        tp_cache_addr -> next = __kmp_threadpriv_cache_list;
        __kmp_threadpriv_cache_list = tp_cache_addr;
        *cache = my_cache;
    }
    sctk_thread_mutex_unlock (&lock);
  }

  void *ret = NULL;
  if ((ret = (*cache)[ global_tid ]) == 0)
  {
      ret = __kmpc_threadprivate( loc, global_tid, data, (size_t) size);
      (*cache)[ global_tid ] = ret;
  }
  
  return ret;
}

void
__kmpc_threadprivate_register( ident_t *loc, void *data, kmpc_ctor ctor, kmpc_cctor cctor,
    kmpc_dtor dtor)
{
  sctk_error("Detection of threadprivate variables w/ Intel Compiler: please re-compile with automatic privatization for MPC") ;
  sctk_abort() ;
}

void
__kmpc_threadprivate_register_vec(ident_t *loc, void *data, kmpc_ctor_vec ctor, 
    kmpc_cctor_vec cctor, kmpc_dtor_vec dtor, size_t vector_length )
{
  sctk_error("Detection of threadprivate variables w/ Intel Compiler: please re-compile with automatic privatization for MPC") ;
  sctk_abort() ;
}

/********************************
  * TASKQ INTERFACE
  *******************************/

typedef struct kmpc_thunk_t {
} kmpc_thunk_t ;

typedef void (*kmpc_task_t) (kmp_int32 global_tid, struct kmpc_thunk_t *thunk);

typedef struct kmpc_shared_vars_t {
} kmpc_shared_vars_t;

kmpc_thunk_t * 
__kmpc_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_task_t taskq_task, size_t sizeof_thunk,
    size_t sizeof_shareds, kmp_int32 flags, kmpc_shared_vars_t **shareds)
{
not_implemented() ;
return NULL ;
}

void 
__kmpc_end_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmp_int32 
__kmpc_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk, kmp_int32 status)
{
not_implemented() ;
}

void __kmpc_end_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmpc_thunk_t * 
__kmpc_task_buffer (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *taskq_thunk, kmpc_task_t task)
{
not_implemented() ;
return NULL ;
}

/********************************
  * TASKING SUPPORT
  *******************************/

typedef kmp_int32 (* kmp_routine_entry_t)( kmp_int32, void * );

typedef struct kmp_task {                   /* GEH: Shouldn't this be aligned somehow? */
    void *              shareds;            /**< pointer to block of pointers to shared vars   */
    kmp_routine_entry_t routine;            /**< pointer to routine to call for executing task */
    kmp_int32           part_id;            /**< part id for the task                          */
#if OMP_40_ENABLED
    kmp_routine_entry_t destructors;        /* pointer to function to invoke deconstructors of firstprivate C++ objects */
#endif // OMP_40_ENABLED
    /*  private vars  */
} kmp_task_t;

/* From kmp_os.h for this type */
typedef long             kmp_intptr_t;

typedef struct kmp_depend_info {
     kmp_intptr_t               base_addr;
     size_t 	                len;
     struct {
         bool                   in:1;
         bool                   out:1;
     } flags;
} kmp_depend_info_t;

struct kmp_taskdata {                                 /* aligned during dynamic allocation       */
#if 0
    kmp_int32               td_task_id;               /* id, assigned by debugger                */
    kmp_tasking_flags_t     td_flags;                 /* task flags                              */
    kmp_team_t *            td_team;                  /* team for this task                      */
    kmp_info_p *            td_alloc_thread;          /* thread that allocated data structures   */
                                                      /* Currently not used except for perhaps IDB */
    kmp_taskdata_t *        td_parent;                /* parent task                             */
    kmp_int32               td_level;                 /* task nesting level                      */
    ident_t *               td_ident;                 /* task identifier                         */
                            // Taskwait data.
    ident_t *               td_taskwait_ident;
    kmp_uint32              td_taskwait_counter;
    kmp_int32               td_taskwait_thread;       /* gtid + 1 of thread encountered taskwait */
    kmp_internal_control_t  td_icvs;                  /* Internal control variables for the task */
    volatile kmp_uint32     td_allocated_child_tasks;  /* Child tasks (+ current task) not yet deallocated */
    volatile kmp_uint32     td_incomplete_child_tasks; /* Child tasks not yet complete */
#if OMP_40_ENABLED
    kmp_taskgroup_t *       td_taskgroup;         // Each task keeps pointer to its current taskgroup
    kmp_dephash_t *         td_dephash;           // Dependencies for children tasks are tracked from here
    kmp_depnode_t *         td_depnode;           // Pointer to graph node if this task has dependencies
#endif
#if KMP_HAVE_QUAD
    _Quad                   td_dummy;             // Align structure 16-byte size since allocated just before kmp_task_t
#else
    kmp_uint32              td_dummy[2];
#endif
#endif
} ;
typedef struct kmp_taskdata  kmp_taskdata_t;

/* FOR OPENMP 3 */

kmp_int32
__kmpc_omp_task( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
not_implemented() ;
return 0 ;
}

kmp_task_t*
__kmpc_omp_task_alloc( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 flags,
                       size_t sizeof_kmp_task_t, size_t sizeof_shareds,
                       kmp_routine_entry_t task_entry )
{
not_implemented() ;
return NULL ;
}

void
__kmpc_omp_task_begin_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void
__kmpc_omp_task_complete_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}

kmp_int32
__kmpc_omp_task_parts( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskwait( ident_t *loc_ref, kmp_int32 gtid )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskyield( ident_t *loc_ref, kmp_int32 gtid, int end_part )
{
not_implemented() ;
return 0 ;
}

/* Following 2 functions should be enclosed by "if TASK_UNUSED" */

void 
__kmpc_omp_task_begin( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void 
__kmpc_omp_task_complete( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}


/* FOR OPENMP 4 */

void 
__kmpc_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

void 
__kmpc_end_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

kmp_int32 
__kmpc_omp_task_with_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task,
    kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_omp_wait_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
}

void 
__kmp_release_deps ( kmp_int32 gtid, kmp_taskdata_t *task )
{
not_implemented() ;
}







/********************************
  * OTHERS FROM KMP.H
  *******************************/

void *
kmpc_malloc( size_t size )
{
not_implemented() ;
return NULL ;
}

void *
kmpc_calloc( size_t nelem, size_t elsize )
{
not_implemented() ;
return NULL ;
}

void *
kmpc_realloc( void *ptr, size_t size )
{
not_implemented() ;
return NULL ;
}

void  kmpc_free( void *ptr )
{
not_implemented() ;
}

int 
__kmpc_invoke_task_func( int gtid )
{
not_implemented() ;
return 0 ;
}

void 
kmpc_set_blocktime (int arg)
{
not_implemented() ;
}

#if 0
void
omp_init_lock( mpcomp_lock_t * lock)
{
not_implemented() ;
}
#endif

/*
 * Lock interface routines (fast versions with gtid passed in)
 */
void 
__kmpc_init_lock( ident_t *loc, kmp_int32 gtid,  void **user_lock )
{
  sctk_debug( "[%d] __kmpc_init_lock: "
      "Addr %p",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, user_lock ) ;

  not_implemented() ;
}

void 
__kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_destroy_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_destroy_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_set_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_set_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_unset_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

void 
__kmpc_unset_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
}

int 
__kmpc_test_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
  return 0 ;
}

int 
__kmpc_test_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  not_implemented() ;
  return 0 ;
}










/********************************
  * ATOMIC_OPERATIONS
  *******************************/

void
__kmpc_atomic_fixed4_add( ident_t * loc, int global_tid, kmp_int32 * lhs, kmp_int32 rhs )
{

  sctk_nodebug( "[%d] __kmpc_atomic_fixed4_add: "
      "Add %d",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __kmp_test_then_add32( lhs, rhs ) ;

#if 0
  __mpcomp_atomic_begin() ;

  *lhs += rhs ;

  __mpcomp_atomic_end() ;

  /* TODO: use assembly functions by Intel if available */
#endif

}


void 
__kmpc_atomic_fixed4_wr(  ident_t *id_ref, int gtid, kmp_int32   * lhs, kmp_int32   rhs ) 
{
  sctk_nodebug( "[%d] __kmpc_atomic_fixed4_wr: "
      "Write %d",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __kmp_xchg_fixed32( lhs, rhs ) ;

#if 0
  __mpcomp_atomic_begin() ;

  *lhs = rhs ;

  __mpcomp_atomic_end() ;

  /* TODO: use assembly functions by Intel if available */
#endif
}


void 
__kmpc_atomic_fixed8_add(  ident_t *id_ref, int gtid, kmp_int64 * lhs, kmp_int64 rhs )
{
  sctk_nodebug( "[%d] __kmpc_atomic_fixed8_add: "
      "Add %ld",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

  __kmp_test_then_add64( lhs, rhs ) ;

#if 0
  __mpcomp_atomic_begin() ;

  *lhs += rhs ;

  __mpcomp_atomic_end() ;
  /* TODO: use assembly functions by Intel if available */
#endif
}

void 
__kmpc_atomic_float8_add(  ident_t *id_ref, int gtid, kmp_real64 * lhs, kmp_real64 rhs )
{
  sctk_nodebug( "[%d] (ASM) __kmpc_atomic_float8_add: "
      "Add %g",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, rhs ) ;

#if 0
  __kmp_test_then_add_real64( lhs, rhs ) ;
#endif

  /* TODO check how we can add this function to asssembly-dedicated module */

/* TODO use MPCOMP_MIC */

#if __MIC__ || __MIC2__
#warning "MIC => atomic locks"
  __mpcomp_atomic_begin() ;

  *lhs += rhs ;

  __mpcomp_atomic_end() ;
#else 
#warning "NON MIC => atomic optim"
  __kmp_test_then_add_real64( lhs, rhs ) ;
#endif


  /* TODO: use assembly functions by Intel if available */
}


#if __MIC__ || __MIC2__
    #define DO_PAUSE _mm_delay_32( 1 )
#else
    #define DO_PAUSE __kmp_x86_pause()
#endif

#define ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE, RET_TYPE) \
RET_TYPE __kmpc_atomic_##TYPE_ID##_##OP_ID( ident_t *id_ref, int gtid, TYPE * lhs, TYPE rhs ) \
{

#define ATOMIC_CMPXCHG(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                      \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    TYPE old_value, new_value;                                                                              \
    old_value = *(TYPE volatile *) lhs;                                                                     \
    new_value = old_value OP rhs;                                                                           \
    while ( ! __kmp_compare_and_store##BITS( (kmp_int##BITS *) lhs,                                                  \
                                          *(volatile kmp_int##BITS *) &old_value,                                    \
                                          *(volatile kmp_int##BITS *) &new_value ))                                  \
    {                                                                                                       \
        DO_PAUSE;                                                                                  \
        old_value = *(TYPE volatile *)lhs;                                                                  \
        new_value = old_value OP rhs;                                                                       \
    }                                                                                                       \
}

#define ATOMIC_CMPX_L(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                     \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}


#define MIN_MAX_COMPXCHG(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define MIN_MAX_CRITICAL(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPX_EQV(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CRITICAL(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}


#define MIN_MAX_COMPXCHG_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define MIN_MAX_CRITICAL_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPXCHG_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CMPX_EQV_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}


#define ATOMIC_CRITICAL_CPT(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_CRITICAL_CPT_WRK(TYPE_ID,OP_ID,TYPE,BITS,OP,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

#define ATOMIC_FIXED_ADD(TYPE_ID,OP_ID,TYPE,BITS,OP,LCK_ID,MASK,GOMP_FLAG)                                               \
ATOMIC_BEGIN(TYPE_ID,OP_ID,TYPE,void)                                                                       \
    not_implemented();                                                                                      \
}

// Routines for ATOMIC 4-byte operands addition and subtraction
//ATOMIC_FIXED_ADD( fixed4, add, kmp_int32,  32, +, 4i, 3, 0            )  // __kmpc_atomic_fixed4_add
ATOMIC_FIXED_ADD( fixed4, sub, kmp_int32,  32, -, 4i, 3, 0            )  // __kmpc_atomic_fixed4_sub

ATOMIC_CMPXCHG( float4,  add, kmp_real32, 32, +,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_add
ATOMIC_CMPXCHG( float4,  sub, kmp_real32, 32, -,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_sub

// Routines for ATOMIC 8-byte operands addition and subtraction
//ATOMIC_FIXED_ADD( fixed8, add, kmp_int64,  64, +, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_add
ATOMIC_FIXED_ADD( fixed8, sub, kmp_int64,  64, -, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_sub

//ATOMIC_CMPXCHG( float8,  add, kmp_real64, 64, +,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_add
ATOMIC_CMPXCHG( float8,  sub, kmp_real64, 64, -,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_sub

// ------------------------------------------------------------------------
// Routines for ATOMIC integer operands, other operators
// ------------------------------------------------------------------------
//              TYPE_ID,OP_ID, TYPE,          OP, LCK_ID, GOMP_FLAG
ATOMIC_CMPXCHG( fixed1,  add, kmp_int8,    8, +,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_add
ATOMIC_CMPXCHG( fixed1, andb, kmp_int8,    8, &,  1i, 0, 0            )  // __kmpc_atomic_fixed1_andb
ATOMIC_CMPXCHG( fixed1,  div, kmp_int8,    8, /,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_div
ATOMIC_CMPXCHG( fixed1u, div, kmp_uint8,   8, /,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_div
ATOMIC_CMPXCHG( fixed1,  mul, kmp_int8,    8, *,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_mul
ATOMIC_CMPXCHG( fixed1,  orb, kmp_int8,    8, |,  1i, 0, 0            )  // __kmpc_atomic_fixed1_orb
ATOMIC_CMPXCHG( fixed1,  shl, kmp_int8,    8, <<, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shl
ATOMIC_CMPXCHG( fixed1,  shr, kmp_int8,    8, >>, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_shr
ATOMIC_CMPXCHG( fixed1u, shr, kmp_uint8,   8, >>, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1u_shr
ATOMIC_CMPXCHG( fixed1,  sub, kmp_int8,    8, -,  1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_sub
ATOMIC_CMPXCHG( fixed1,  xor, kmp_int8,    8, ^,  1i, 0, 0            )  // __kmpc_atomic_fixed1_xor
ATOMIC_CMPXCHG( fixed2,  add, kmp_int16,  16, +,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_add
ATOMIC_CMPXCHG( fixed2, andb, kmp_int16,  16, &,  2i, 1, 0            )  // __kmpc_atomic_fixed2_andb
ATOMIC_CMPXCHG( fixed2,  div, kmp_int16,  16, /,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_div
ATOMIC_CMPXCHG( fixed2u, div, kmp_uint16, 16, /,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_div
ATOMIC_CMPXCHG( fixed2,  mul, kmp_int16,  16, *,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_mul
ATOMIC_CMPXCHG( fixed2,  orb, kmp_int16,  16, |,  2i, 1, 0            )  // __kmpc_atomic_fixed2_orb
ATOMIC_CMPXCHG( fixed2,  shl, kmp_int16,  16, <<, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shl
ATOMIC_CMPXCHG( fixed2,  shr, kmp_int16,  16, >>, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_shr
ATOMIC_CMPXCHG( fixed2u, shr, kmp_uint16, 16, >>, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2u_shr
ATOMIC_CMPXCHG( fixed2,  sub, kmp_int16,  16, -,  2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_sub
ATOMIC_CMPXCHG( fixed2,  xor, kmp_int16,  16, ^,  2i, 1, 0            )  // __kmpc_atomic_fixed2_xor
ATOMIC_CMPXCHG( fixed4, andb, kmp_int32,  32, &,  4i, 3, 0            )  // __kmpc_atomic_fixed4_andb
ATOMIC_CMPXCHG( fixed4,  div, kmp_int32,  32, /,  4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_div
ATOMIC_CMPXCHG( fixed4u, div, kmp_uint32, 32, /,  4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_div
ATOMIC_CMPXCHG( fixed4,  mul, kmp_int32,  32, *,  4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_mul
ATOMIC_CMPXCHG( fixed4,  orb, kmp_int32,  32, |,  4i, 3, 0            )  // __kmpc_atomic_fixed4_orb
ATOMIC_CMPXCHG( fixed4,  shl, kmp_int32,  32, <<, 4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shl
ATOMIC_CMPXCHG( fixed4,  shr, kmp_int32,  32, >>, 4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4_shr
ATOMIC_CMPXCHG( fixed4u, shr, kmp_uint32, 32, >>, 4i, 3, KMP_ARCH_X86 )  // __kmpc_atomic_fixed4u_shr
ATOMIC_CMPXCHG( fixed4,  xor, kmp_int32,  32, ^,  4i, 3, 0            )  // __kmpc_atomic_fixed4_xor
ATOMIC_CMPXCHG( fixed8, andb, kmp_int64,  64, &,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_andb
ATOMIC_CMPXCHG( fixed8,  div, kmp_int64,  64, /,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_div
ATOMIC_CMPXCHG( fixed8u, div, kmp_uint64, 64, /,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_div
ATOMIC_CMPXCHG( fixed8,  mul, kmp_int64,  64, *,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_mul
ATOMIC_CMPXCHG( fixed8,  orb, kmp_int64,  64, |,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_orb
ATOMIC_CMPXCHG( fixed8,  shl, kmp_int64,  64, <<, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shl
ATOMIC_CMPXCHG( fixed8,  shr, kmp_int64,  64, >>, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_shr
ATOMIC_CMPXCHG( fixed8u, shr, kmp_uint64, 64, >>, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8u_shr
ATOMIC_CMPXCHG( fixed8,  xor, kmp_int64,  64, ^,  8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_xor
ATOMIC_CMPXCHG( float4,  div, kmp_real32, 32, /,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_div
ATOMIC_CMPXCHG( float4,  mul, kmp_real32, 32, *,  4r, 3, KMP_ARCH_X86 )  // __kmpc_atomic_float4_mul
ATOMIC_CMPXCHG( float8,  div, kmp_real64, 64, /,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_div
ATOMIC_CMPXCHG( float8,  mul, kmp_real64, 64, *,  8r, 7, KMP_ARCH_X86 )  // __kmpc_atomic_float8_mul
//              TYPE_ID,OP_ID, TYPE,          OP, LCK_ID, GOMP_FLAG

/* ------------------------------------------------------------------------ */
/* Routines for C/C++ Reduction operators && and ||                         */
/* ------------------------------------------------------------------------ */

ATOMIC_CMPX_L( fixed1, andl, char,       8, &&, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_andl
ATOMIC_CMPX_L( fixed1,  orl, char,       8, ||, 1i, 0, KMP_ARCH_X86 )  // __kmpc_atomic_fixed1_orl
ATOMIC_CMPX_L( fixed2, andl, short,     16, &&, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_andl
ATOMIC_CMPX_L( fixed2,  orl, short,     16, ||, 2i, 1, KMP_ARCH_X86 )  // __kmpc_atomic_fixed2_orl
ATOMIC_CMPX_L( fixed4, andl, kmp_int32, 32, &&, 4i, 3, 0 )             // __kmpc_atomic_fixed4_andl
ATOMIC_CMPX_L( fixed4,  orl, kmp_int32, 32, ||, 4i, 3, 0 )             // __kmpc_atomic_fixed4_orl
ATOMIC_CMPX_L( fixed8, andl, kmp_int64, 64, &&, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_andl
ATOMIC_CMPX_L( fixed8,  orl, kmp_int64, 64, ||, 8i, 7, KMP_ARCH_X86 )  // __kmpc_atomic_fixed8_orl

/* ------------------------------------------------------------------------- */
/* Routines for Fortran operators that matched no one in C:                  */
/* MAX, MIN, .EQV., .NEQV.                                                   */
/* Operators .AND., .OR. are covered by __kmpc_atomic_*_{andl,orl}           */
/* Intrinsics IAND, IOR, IEOR are covered by __kmpc_atomic_*_{andb,orb,xor}  */
/* ------------------------------------------------------------------------- */

MIN_MAX_COMPXCHG( fixed1,  max, char,        8, <, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_max
MIN_MAX_COMPXCHG( fixed1,  min, char,        8, >, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_min
MIN_MAX_COMPXCHG( fixed2,  max, short,      16, <, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_max
MIN_MAX_COMPXCHG( fixed2,  min, short,      16, >, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_min
MIN_MAX_COMPXCHG( fixed4,  max, kmp_int32,  32, <, 4i, 3, 0 )            // __kmpc_atomic_fixed4_max
MIN_MAX_COMPXCHG( fixed4,  min, kmp_int32,  32, >, 4i, 3, 0 )            // __kmpc_atomic_fixed4_min
MIN_MAX_COMPXCHG( fixed8,  max, kmp_int64,  64, <, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_max
MIN_MAX_COMPXCHG( fixed8,  min, kmp_int64,  64, >, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_min
MIN_MAX_COMPXCHG( float4,  max, kmp_real32, 32, <, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_max
MIN_MAX_COMPXCHG( float4,  min, kmp_real32, 32, >, 4r, 3, KMP_ARCH_X86 ) // __kmpc_atomic_float4_min
MIN_MAX_COMPXCHG( float8,  max, kmp_real64, 64, <, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_max
MIN_MAX_COMPXCHG( float8,  min, kmp_real64, 64, >, 8r, 7, KMP_ARCH_X86 ) // __kmpc_atomic_float8_min
#if KMP_HAVE_QUAD
    MIN_MAX_CRITICAL( float16, max,     QUAD_LEGACY,      <, 16r,   1 )            // __kmpc_atomic_float16_max
    MIN_MAX_CRITICAL( float16, min,     QUAD_LEGACY,      >, 16r,   1 )            // __kmpc_atomic_float16_min
    #if ( KMP_ARCH_X86 )
        MIN_MAX_CRITICAL( float16, max_a16, Quad_a16_t,     <, 16r,   1 )            // __kmpc_atomic_float16_max_a16
        MIN_MAX_CRITICAL( float16, min_a16, Quad_a16_t,     >, 16r,   1 )            // __kmpc_atomic_float16_min_a16
    #endif
#endif
ATOMIC_CMPXCHG(  fixed1, neqv, kmp_int8,   8,   ^, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_neqv
ATOMIC_CMPXCHG(  fixed2, neqv, kmp_int16, 16,   ^, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_neqv
ATOMIC_CMPXCHG(  fixed4, neqv, kmp_int32, 32,   ^, 4i, 3, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_neqv
ATOMIC_CMPXCHG(  fixed8, neqv, kmp_int64, 64,   ^, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_neqv
ATOMIC_CMPX_EQV( fixed1, eqv,  kmp_int8,   8,  ^~, 1i, 0, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_eqv
ATOMIC_CMPX_EQV( fixed2, eqv,  kmp_int16, 16,  ^~, 2i, 1, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_eqv
ATOMIC_CMPX_EQV( fixed4, eqv,  kmp_int32, 32,  ^~, 4i, 3, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_eqv
ATOMIC_CMPX_EQV( fixed8, eqv,  kmp_int64, 64,  ^~, 8i, 7, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_eqv

// ------------------------------------------------------------------------
// Routines for Extended types: long double, _Quad, complex flavours (use critical section)
//     TYPE_ID, OP_ID, TYPE - detailed above
//     OP      - operator
//     LCK_ID  - lock identifier, used to possibly distinguish lock variable

/* ------------------------------------------------------------------------- */
// routines for long double type
ATOMIC_CRITICAL( float10, add, long double,     +, 10r,   1 )            // __kmpc_atomic_float10_add
ATOMIC_CRITICAL( float10, sub, long double,     -, 10r,   1 )            // __kmpc_atomic_float10_sub
ATOMIC_CRITICAL( float10, mul, long double,     *, 10r,   1 )            // __kmpc_atomic_float10_mul
ATOMIC_CRITICAL( float10, div, long double,     /, 10r,   1 )            // __kmpc_atomic_float10_div




MIN_MAX_COMPXCHG_CPT( fixed1,  max_cpt, char,        8, <, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed1,  min_cpt, char,        8, >, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_min_cpt
MIN_MAX_COMPXCHG_CPT( fixed2,  max_cpt, short,      16, <, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed2,  min_cpt, short,      16, >, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_min_cpt
MIN_MAX_COMPXCHG_CPT( fixed4,  max_cpt, kmp_int32,  32, <, 0 )            // __kmpc_atomic_fixed4_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed4,  min_cpt, kmp_int32,  32, >, 0 )            // __kmpc_atomic_fixed4_min_cpt
MIN_MAX_COMPXCHG_CPT( fixed8,  max_cpt, kmp_int64,  64, <, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_max_cpt
MIN_MAX_COMPXCHG_CPT( fixed8,  min_cpt, kmp_int64,  64, >, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_min_cpt
MIN_MAX_COMPXCHG_CPT( float4,  max_cpt, kmp_real32, 32, <, KMP_ARCH_X86 ) // __kmpc_atomic_float4_max_cpt
MIN_MAX_COMPXCHG_CPT( float4,  min_cpt, kmp_real32, 32, >, KMP_ARCH_X86 ) // __kmpc_atomic_float4_min_cpt
MIN_MAX_COMPXCHG_CPT( float8,  max_cpt, kmp_real64, 64, <, KMP_ARCH_X86 ) // __kmpc_atomic_float8_max_cpt
MIN_MAX_COMPXCHG_CPT( float8,  min_cpt, kmp_real64, 64, >, KMP_ARCH_X86 ) // __kmpc_atomic_float8_min_cpt
#if KMP_HAVE_QUAD
    MIN_MAX_CRITICAL_CPT( float16, max_cpt, QUAD_LEGACY,    <, 16r,   1 )     // __kmpc_atomic_float16_max_cpt
    MIN_MAX_CRITICAL_CPT( float16, min_cpt, QUAD_LEGACY,    >, 16r,   1 )     // __kmpc_atomic_float16_min_cpt
    #if ( KMP_ARCH_X86 )
        MIN_MAX_CRITICAL_CPT( float16, max_a16_cpt, Quad_a16_t, <, 16r,  1 )  // __kmpc_atomic_float16_max_a16_cpt
        MIN_MAX_CRITICAL_CPT( float16, min_a16_cpt, Quad_a16_t, >, 16r,  1 )  // __kmpc_atomic_float16_mix_a16_cpt
    #endif
#endif

ATOMIC_CMPXCHG_CPT(  fixed1, neqv_cpt, kmp_int8,   8,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_neqv_cpt
ATOMIC_CMPXCHG_CPT(  fixed2, neqv_cpt, kmp_int16, 16,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_neqv_cpt
ATOMIC_CMPXCHG_CPT(  fixed4, neqv_cpt, kmp_int32, 32,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_neqv_cpt
ATOMIC_CMPXCHG_CPT(  fixed8, neqv_cpt, kmp_int64, 64,   ^, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_neqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed1, eqv_cpt,  kmp_int8,   8,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed1_eqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed2, eqv_cpt,  kmp_int16, 16,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed2_eqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed4, eqv_cpt,  kmp_int32, 32,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed4_eqv_cpt
ATOMIC_CMPX_EQV_CPT( fixed8, eqv_cpt,  kmp_int64, 64,  ^~, KMP_ARCH_X86 ) // __kmpc_atomic_fixed8_eqv_cpt

// ------------------------------------------------------------------------
// Routines for Extended types: long double, _Quad, complex flavours (use critical section)
//     TYPE_ID, OP_ID, TYPE - detailed above
//     OP      - operator
//     LCK_ID  - lock identifier, used to possibly distinguish lock variable

// routines for long double type                      
ATOMIC_CRITICAL_CPT( float10, add_cpt, long double,     +, 10r,   1 )            // __kmpc_atomic_float10_add_cpt
ATOMIC_CRITICAL_CPT( float10, sub_cpt, long double,     -, 10r,   1 )            // __kmpc_atomic_float10_sub_cpt
ATOMIC_CRITICAL_CPT( float10, mul_cpt, long double,     *, 10r,   1 )            // __kmpc_atomic_float10_mul_cpt
ATOMIC_CRITICAL_CPT( float10, div_cpt, long double,     /, 10r,   1 )            // __kmpc_atomic_float10_div_cpt
#if KMP_HAVE_QUAD
    // routines for _Quad type
    ATOMIC_CRITICAL_CPT( float16, add_cpt, QUAD_LEGACY,     +, 16r,   1 )            // __kmpc_atomic_float16_add_cpt
    ATOMIC_CRITICAL_CPT( float16, sub_cpt, QUAD_LEGACY,     -, 16r,   1 )            // __kmpc_atomic_float16_sub_cpt
    ATOMIC_CRITICAL_CPT( float16, mul_cpt, QUAD_LEGACY,     *, 16r,   1 )            // __kmpc_atomic_float16_mul_cpt
    ATOMIC_CRITICAL_CPT( float16, div_cpt, QUAD_LEGACY,     /, 16r,   1 )            // __kmpc_atomic_float16_div_cpt
    #if ( KMP_ARCH_X86 )
        ATOMIC_CRITICAL_CPT( float16, add_a16_cpt, Quad_a16_t, +, 16r,  1 )          // __kmpc_atomic_float16_add_a16_cpt
        ATOMIC_CRITICAL_CPT( float16, sub_a16_cpt, Quad_a16_t, -, 16r,  1 )          // __kmpc_atomic_float16_sub_a16_cpt
        ATOMIC_CRITICAL_CPT( float16, mul_a16_cpt, Quad_a16_t, *, 16r,  1 )          // __kmpc_atomic_float16_mul_a16_cpt
        ATOMIC_CRITICAL_CPT( float16, div_a16_cpt, Quad_a16_t, /, 16r,  1 )          // __kmpc_atomic_float16_div_a16_cpt
    #endif
#endif

#if 0
// routines for complex types
// cmplx4 routines to return void
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  add_cpt, kmp_cmplx32, +, 8c,    1 )            // __kmpc_atomic_cmplx4_add_cpt
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  sub_cpt, kmp_cmplx32, -, 8c,    1 )            // __kmpc_atomic_cmplx4_sub_cpt
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  mul_cpt, kmp_cmplx32, *, 8c,    1 )            // __kmpc_atomic_cmplx4_mul_cpt
ATOMIC_CRITICAL_CPT_WRK( cmplx4,  div_cpt, kmp_cmplx32, /, 8c,    1 )            // __kmpc_atomic_cmplx4_div_cpt

ATOMIC_CRITICAL_CPT( cmplx8,  add_cpt, kmp_cmplx64, +, 16c,   1 )            // __kmpc_atomic_cmplx8_add_cpt
ATOMIC_CRITICAL_CPT( cmplx8,  sub_cpt, kmp_cmplx64, -, 16c,   1 )            // __kmpc_atomic_cmplx8_sub_cpt
ATOMIC_CRITICAL_CPT( cmplx8,  mul_cpt, kmp_cmplx64, *, 16c,   1 )            // __kmpc_atomic_cmplx8_mul_cpt
ATOMIC_CRITICAL_CPT( cmplx8,  div_cpt, kmp_cmplx64, /, 16c,   1 )            // __kmpc_atomic_cmplx8_div_cpt
ATOMIC_CRITICAL_CPT( cmplx10, add_cpt, kmp_cmplx80, +, 20c,   1 )            // __kmpc_atomic_cmplx10_add_cpt
ATOMIC_CRITICAL_CPT( cmplx10, sub_cpt, kmp_cmplx80, -, 20c,   1 )            // __kmpc_atomic_cmplx10_sub_cpt
ATOMIC_CRITICAL_CPT( cmplx10, mul_cpt, kmp_cmplx80, *, 20c,   1 )            // __kmpc_atomic_cmplx10_mul_cpt
ATOMIC_CRITICAL_CPT( cmplx10, div_cpt, kmp_cmplx80, /, 20c,   1 )            // __kmpc_atomic_cmplx10_div_cpt
#if KMP_HAVE_QUAD
    ATOMIC_CRITICAL_CPT( cmplx16, add_cpt, CPLX128_LEG, +, 32c,   1 )            // __kmpc_atomic_cmplx16_add_cpt
    ATOMIC_CRITICAL_CPT( cmplx16, sub_cpt, CPLX128_LEG, -, 32c,   1 )            // __kmpc_atomic_cmplx16_sub_cpt
    ATOMIC_CRITICAL_CPT( cmplx16, mul_cpt, CPLX128_LEG, *, 32c,   1 )            // __kmpc_atomic_cmplx16_mul_cpt
    ATOMIC_CRITICAL_CPT( cmplx16, div_cpt, CPLX128_LEG, /, 32c,   1 )            // __kmpc_atomic_cmplx16_div_cpt
    #if ( KMP_ARCH_X86 )
        ATOMIC_CRITICAL_CPT( cmplx16, add_a16_cpt, kmp_cmplx128_a16_t, +, 32c,   1 )   // __kmpc_atomic_cmplx16_add_a16_cpt
        ATOMIC_CRITICAL_CPT( cmplx16, sub_a16_cpt, kmp_cmplx128_a16_t, -, 32c,   1 )   // __kmpc_atomic_cmplx16_sub_a16_cpt
        ATOMIC_CRITICAL_CPT( cmplx16, mul_a16_cpt, kmp_cmplx128_a16_t, *, 32c,   1 )   // __kmpc_atomic_cmplx16_mul_a16_cpt
        ATOMIC_CRITICAL_CPT( cmplx16, div_a16_cpt, kmp_cmplx128_a16_t, /, 32c,   1 )   // __kmpc_atomic_cmplx16_div_a16_cpt
    #endif
#endif
#endif

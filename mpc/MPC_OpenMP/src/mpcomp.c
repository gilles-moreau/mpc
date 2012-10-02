///* ############################# MPC License ############################## */
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
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "sctk_topology.h"
#include "sctk_runtime_config.h"
#include "mpcomp_internal.h"
#include <sys/time.h>

/*****************
  ****** MACROS
  *****************/

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 0

__thread void *sctk_openmp_thread_tls = NULL ;

/* Schedule type */
static omp_sched_t OMP_SCHEDULE = 1;
/* Schedule modifier */
static int OMP_MODIFIER_SCHEDULE = -1;
/* Defaults number of threads */
static int OMP_NUM_THREADS = 0;
/* Is dynamic adaptation on? */
static int OMP_DYNAMIC = 0;
/* Is nested parallelism handled or flaterned? */
static int OMP_NESTED = 0;
/* Number of VPs for each OpenMP team */
static int OMP_MICROVP_NUMBER = 0;
/* Default tree for OpenMP instances? (number of nodes per level) */
static int * OMP_TREE = NULL ;
/* Number of level for the default tree (size of OMP_TREE) */
static int OMP_TREE_DEPTH = 0 ;
/* Total number of leaves for the tree (product of OMP_TREE elements) */
static int OMP_TREE_NB_LEAVES = 0 ;


/*****************
  ****** FUNCTIONS  
  *****************/

TODO(" function __mpcomp_tokenizer is inspired from sctk_launch.c. Need to merge")
static char ** __mpcomp_tokenizer( char * string_to_tokenize, int * nb_tokens ) {
    /*    size_t len;*/
    char *cursor;
    int i;
    char **new_argv;
    int new_argc = 0;

    /* TODO check sizes of every malloc... */

    new_argv = (char **) sctk_malloc ( 32 * sizeof (char *));
    sctk_assert( new_argv != NULL ) ;

    cursor = string_to_tokenize ;

    while (*cursor == ' ')
      cursor++;
    while (*cursor != '\0')
    {
      int word_len = 0;
      new_argv[new_argc] = (char *) sctk_malloc (1024 * sizeof (char));
      while ((word_len < 1024) && (*cursor != '\0') && (*cursor != ' '))
      {
        new_argv[new_argc][word_len] = *cursor;
        cursor++;
        word_len++;
      }
      new_argv[new_argc][word_len] = '\0';
      new_argc++;
      while (*cursor == ' ')
        cursor++;
    }
    new_argv[new_argc] = NULL;

    *nb_tokens = new_argc ;
    return new_argv ;

}

static inline void __mpcomp_read_env_variables() {
  char * env ;
  int nb_threads ;

  sctk_nodebug ("__mpcomp_read_env_variables: Read env vars (MPC rank: %d)",
      sctk_get_task_rank ());

  /******* OMP_VP_NUMBER *********/
  env = getenv ("OMP_VP_NUMBER");
  OMP_MICROVP_NUMBER = sctk_get_processor_number (); /* DEFAULT */
  if (env != NULL)
  {
    int arg = atoi( env ) ;
    if ( arg <= 0 ) {
      fprintf( stderr, 
	  "Warning: Incorrect number of microVPs (OMP_MICROVP_NUMBER=<%s>) -> "
	  "Switching to default value %d\n", env, OMP_MICROVP_NUMBER ) ;
    } else if ( arg > sctk_get_processor_number () ) {
      fprintf( stderr,
	  "Warning: Number of microVPs should be at most the number of cores per node: %d\n"
	  "Switching to default value %d\n", sctk_get_processor_number (), OMP_MICROVP_NUMBER ) ;
    }
    else {
      OMP_MICROVP_NUMBER = arg ;
    }
  }

  /******* OMP_SCHEDULE *********/
  env = getenv ("OMP_SCHEDULE");
  OMP_SCHEDULE = 1 ;	/* DEFAULT */
  if (env != NULL)
  {
    int ok = 0 ;
    int offset = 0 ;
    if ( strncmp (env, "static", 6) == 0 ) {
      OMP_SCHEDULE = 1 ;
      offset = 6 ;
      ok = 1 ;
    } 
    if ( strncmp (env, "dynamic", 7) == 0 ) {
      OMP_SCHEDULE = 2 ;
      offset = 7 ;
      ok = 1 ;
    } 
    if ( strncmp (env, "guided", 6) == 0 ) {
      OMP_SCHEDULE = 3 ;
      offset = 6 ;
      ok = 1 ;
    } 
    if ( strncmp (env, "auto", 4) == 0 ) {
      OMP_SCHEDULE = 4 ;
      offset = 4 ;
      ok = 1 ;
    } 
    if ( ok ) {
      int chunk_size = 0 ;
      /* Check for chunk size, if present */
      sctk_nodebug( "Remaining string for schedule: <%s>", &env[offset] ) ;
      switch( env[offset] ) {
	case ',':
	  sctk_nodebug( "There is a chunk size -> <%s>", &env[offset+1] ) ;
	  chunk_size = atoi( &env[offset+1] ) ;
	  if ( chunk_size <= 0 ) {
	    fprintf (stderr,
		"Warning: Incorrect chunk size within OMP_SCHEDULE variable: <%s>\n", env ) ;
	    chunk_size = 0 ;
	  } else {
	    OMP_MODIFIER_SCHEDULE = chunk_size ;
	  }
	  break ;
	case '\0':
	  sctk_nodebug( "No chunk size\n" ) ;
	  break ;
	default:
	  fprintf (stderr,
	      "Syntax error with envorinment variable OMP_SCHEDULE <%s>,"
	      " should be \"static|dynamic|guided|auto[,chunk_size]\"\n", env ) ;
	  exit( 1 ) ;
      }
    }  else {
      fprintf (stderr,
	  "Warning: Unknown schedule <%s> (must be static, guided or dynamic),"
	  " fallback to default schedule <%d>\n", env,
	  OMP_SCHEDULE);
    }
  }


  /******* OMP_NUM_THREADS *********/
  env = getenv ("OMP_NUM_THREADS");
  OMP_NUM_THREADS = OMP_MICROVP_NUMBER;	/* DEFAULT */
  if (env != NULL)
  {
    nb_threads = atoi (env);
    if (nb_threads > 0)
    {
      OMP_NUM_THREADS = nb_threads;
    }
  }


  /******* OMP_DYNAMIC *********/
  env = getenv ("OMP_DYNAMIC");
  OMP_DYNAMIC = 0;	/* DEFAULT */
  if (env != NULL)
  {
    if (strcmp (env, "true") == 0)
    {
      OMP_DYNAMIC = 1;
    }
  }


  /******* OMP_NESTED *********/
  env = getenv ("OMP_NESTED");
  OMP_NESTED = 0;	/* DEFAULT */
  if (env != NULL)
  {
    if (strcmp (env, "1") == 0 || strcmp (env, "TRUE") == 0 || strcmp (env, "true") == 0 )
    {
      OMP_NESTED = 1;
    }
  }



  /******* ADDITIONAL VARIABLES *******/

  /******* OMP_TREE *********/
  env = getenv ("OMP_TREE");
  if (env != NULL)
  {
    /* TODO check the OMP_TREE variable */
    fprintf( stderr, "OMP_TREE variable ignored <%s>...\n", env ) ;

    int nb_tokens = 0 ;
    char ** tokens = NULL ;
    int i ;

    tokens = __mpcomp_tokenizer( env, &nb_tokens ) ;
    sctk_assert( tokens != NULL ) ;

#if 0
    fprintf( stderr, "OMP_TREE: Found %d token(s)\n", nb_tokens ) ;

    for ( i = 0 ; i < nb_tokens ; i++ ) {
      fprintf( stderr, "OMP_TREE\tToken %d -> <%s>\n", i, tokens[i] ) ;
    }
#endif

    /* TODO check that arguments are ok and #leaves is correct */

    OMP_TREE = (int *)sctk_malloc( nb_tokens * sizeof( int ) ) ;
    OMP_TREE_DEPTH = nb_tokens ;
    OMP_TREE_NB_LEAVES = 1 ;
    for ( i = 0 ; i < nb_tokens ; i++ ) {
      OMP_TREE[i] = atoi( tokens[ i ] ) ;
      OMP_TREE_NB_LEAVES *= OMP_TREE[i] ;
    }

#if 1
    fprintf( stderr, "OMP_TREE: tree w/ %d level(s)\n", OMP_TREE_DEPTH ) ;

    for ( i = 0 ; i < nb_tokens ; i++ ) {
      fprintf( stderr, "OMP_TREE\tLevel %d -> %d children\n", i, OMP_TREE[i] ) ;
    }
#endif

  }

  /***** PRINT SUMMARY ******/
  if (getenv ("MPC_DISABLE_BANNER") == NULL) {
    fprintf (stderr,
	"MPC OpenMP version %d.%d (NUMA-aware DEV)\n",
	SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR);
    fprintf (stderr, "\tOMP_SCHEDULE %d\n", OMP_SCHEDULE);
    fprintf (stderr, "\tOMP_NUM_THREADS %d\n", OMP_NUM_THREADS);
    fprintf (stderr, "\tOMP_DYNAMIC %d\n", OMP_DYNAMIC);
    fprintf (stderr, "\tOMP_NESTED %d\n", OMP_NESTED);
    fprintf (stderr, "\t%d microVPs (OMP_MICROVP_NUMBER)\n", OMP_MICROVP_NUMBER);
#if MPCOMP_MALLOC_ON_NODE
    fprintf( stderr, "\tNUMA allocation for tree nodes\n" ) ;
#endif
#if MPCOMP_USE_ATOMICS 
    fprintf( stderr, "\tUse atomics for tree operations\n" ) ;
#endif
  }
}


/* Initialization of the OpenMP runtime
   Called during the initialization of MPC
 */
void __mpcomp_init() {
  static volatile int done = 0;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  mpcomp_team_t * team_info ;

  /* Need to initialize the current team */
  if ( sctk_openmp_thread_tls == NULL ) {
    mpcomp_instance_t * instance ;
    mpcomp_thread_t * t ;
    mpcomp_icv_t icvs;

    sctk_thread_mutex_lock (&lock);
    /* Need to initialize the whole runtime (environment variables) */
    if ( done == 0 ) {
      __mpcomp_read_env_variables() ;
      done = 1;
    }


    /* Allocate an instance of OpenMP */
    instance = (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
    sctk_assert( instance != NULL ) ;

    /* Initialization of the OpenMP instance */
    __mpcomp_instance_init( instance, OMP_MICROVP_NUMBER ) ;

    /* Allocate information for the sequential region */
    t = (mpcomp_thread_t *)sctk_malloc( sizeof(mpcomp_thread_t) ) ;
    sctk_assert( t != NULL ) ;

    /* Initialize default ICVs */
    icvs.nthreads_var = OMP_NUM_THREADS;
    icvs.dyn_var = OMP_DYNAMIC;
    icvs.nest_var = OMP_NESTED;
    icvs.run_sched_var = OMP_SCHEDULE;
    icvs.modifier_sched_var = OMP_MODIFIER_SCHEDULE ;
    icvs.def_sched_var = omp_sched_static ;
    icvs.nmicrovps_var = OMP_MICROVP_NUMBER ;

    /* Initialize team information */
    team_info = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
    sctk_assert( team_info != NULL ) ;

    /* Team initialization */
    __mpcomp_team_init( team_info ) ;

    // TODO: these lines should be in a function __mpcomp_thread_init(t)
    __mpcomp_thread_init( t, icvs, instance, team_info ) ;

    /* Current thread information is 't' */
    sctk_openmp_thread_tls = t ;

    sctk_thread_mutex_unlock (&lock);


    sctk_nodebug( "__mpcomp_init: Init done..." ) ;
  }

}


void __mpcomp_instance_init( mpcomp_instance_t * instance, int nb_mvps ) {

  sctk_nodebug( "__mpcomp_instance_init: Entering..." ) ;

  /* Alloc memory for 'nb_mvps' microVPs */
  instance->mvps = (mpcomp_mvp_t **)sctk_malloc( nb_mvps * sizeof( mpcomp_mvp_t * ) ) ;
  sctk_assert( instance->mvps != NULL ) ;

  instance->nb_mvps = nb_mvps ;

  if ( OMP_TREE == NULL ) {
    __mpcomp_build_default_tree( instance ) ;
  } else {
    __mpcomp_build_tree( instance, OMP_TREE_NB_LEAVES, OMP_TREE_DEPTH, OMP_TREE ) ; 
  }

  /* Do we need a TLS for the openmp instance for every microVPs? */
}

void __mpcomp_start_parallel_region(int arg_num_threads, void *(*func)
    (void *), void *shared) {
  mpcomp_thread_t * t ;
  mpcomp_node_t * root ;
  int num_threads ;
  int i ;

  __mpcomp_init() ;

  sctk_nodebug( "__mpcomp_start_parallel_region: ========== NEW PARALLEL REGION (f %p) =============", func ) ;

  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Compute the number of threads for this parallel region */
  num_threads = t->icvs.nthreads_var;
  if ( arg_num_threads > 0 && arg_num_threads < MPCOMP_MAX_THREADS ) {
    num_threads = arg_num_threads;
  }

  sctk_nodebug( "__mpcomp_start_parallel_region: Final number of threads %d (default %d)",
      num_threads, t->icvs.nthreads_var ) ;

  /* Bypass if the parallel region contains only 1 thread */
  if (num_threads == 1)
  {
    sctk_nodebug( "__mpcomp_start_parallel_region: starting w/ 1 thread" ) ;
    t->num_threads = 1 ;
    t->rank = 0 ;

    func(shared) ;
    return ;
  }

#if 0
  /* TODO develop some sequential tests to check the validity of the tree and the corresponding variables */
  /* Count the number of STOP symbol inside SINGLE variables */
  int count_single_stop = 0 ;
  int index_single_stop = -1 ;
  for ( i = 0 ; i <= MPCOMP_MAX_ALIVE_SINGLE ; i++ ) {
    sctk_nodebug( "Value of single[%d] -> %d", i, sctk_atomics_load_int(
	  &(t->team->single_nb_threads_entered[i].i) ) ) ;
    if ( sctk_atomics_load_int( 
	  &(t->team->single_nb_threads_entered[i].i) )
	== MPCOMP_MAX_THREADS ) {
      count_single_stop++ ;
      index_single_stop = i ;
    }
  }
  sctk_assert( count_single_stop == 1 ) ;
  sctk_assert( index_single_stop == t->team->single_last_current ) ;
#endif

  /* First level of parallel region (no nesting) */
  if ( t->team->depth == 0 ) {
    mpcomp_instance_t * instance ;
    mpcomp_node_t * n ;			/* Temp node */
    int num_threads_per_child ;
    int max_index_with_more_threads ;

    /* Get the OpenMP instance already allocated during the initializatino (mpcomp_init) */
    instance = t->children_instance ;
    sctk_assert( instance != NULL ) ;

    /* Get the root node of the main tree */
    root = instance->root ;
    sctk_assert( root != NULL ) ;

    /* Root is awake -> propagate the values to children */
    num_threads_per_child = num_threads / instance->nb_mvps ;
    max_index_with_more_threads = (num_threads % instance->nb_mvps)-1 ;

    mpcomp_node_t * new_root ; /* Where to spin for the end of the // region */
#if 1
    if ( instance->nb_mvps == 128 && num_threads <= 32 ) {
      if ( num_threads <= 8 ) {
	n = root->children.node[0]->children.node[0] ;
	n->team_info = t->team ;
	// n->barrier_num_threads = num_threads ;
	n->father = NULL ;
      } else {
	n = root->children.node[0] ;
	n->team_info = t->team ;
	// n->barrier_num_threads = num_threads ;
	n->father = NULL ;
      }
    } else {
      n = root ;
    }
#endif

#if 0
    n = root ;
#endif

    new_root = n ;

    while ( n->child_type != MPCOMP_CHILDREN_LEAF ) {
      int nb_children_involved = 1 ;
      /*
	 n->children.node[0]->func = func ;
	 n->children.node[0]->shared = shared ;
	 n->children.node[0]->num_threads = num_threads ;
       */
      n->children.node[0]->team_info = t->team ;
      for ( i = 1 ; i < n->nb_children ; i++ ) {
	int temp_num_threads ;

	/* Compute the number of threads for the smallest child of the subtree 'n' */
	temp_num_threads = num_threads_per_child ;
	if ( n->children.node[i]->min_index < max_index_with_more_threads ) temp_num_threads++ ;

	if ( temp_num_threads > 0 ) {
	  n->children.node[i]->func = func ;
	  n->children.node[i]->shared = shared ;
	  n->children.node[i]->num_threads = num_threads ;
	  n->children.node[i]->team_info = t->team ;
	  sctk_assert( t->team != NULL ) ;
	  n->children.node[i]->slave_running = 1 ;
	  nb_children_involved++ ;
	} 
	/*
	   else {
	   break ;
	   }
	 */

      }
      sctk_nodebug( "__mpcomp_start_parallel_region: nb_children_involved=%d\n", nb_children_involved ) ;
      n->barrier_num_threads = nb_children_involved ;
      n = n->children.node[0] ;
    }

    n->func = func ;
    n->shared = shared ;
    n->num_threads = num_threads ;
    /* Wake up children leaf */
    sctk_assert( n->child_type == MPCOMP_CHILDREN_LEAF ) ;
    int nb_leaves_involved = 1 ;
    for ( i = 1 ; i < n->nb_children ; i++ ) {
      int num_threads_leaf ;
      num_threads_leaf = num_threads / instance->nb_mvps ;
      if ( n->children.leaf[i]->rank < (num_threads%instance->nb_mvps) ) num_threads_leaf++ ;
      if ( num_threads_leaf> 0 ) {
	n->children.leaf[i]->slave_running = 1 ;
	nb_leaves_involved++ ;
      }
    }
    sctk_nodebug( "__mpcomp_start_parallel_region: nb_leaves_involved=%d\n", nb_leaves_involved) ;
    n->barrier_num_threads = nb_leaves_involved ;

    // instance->mvps[0]->region_id = (instance->mvps[0]->region_id+1)%2;
    instance->mvps[0]->func = func ;
    instance->mvps[0]->shared = shared ;
    instance->mvps[0]->nb_threads = 1 ;

    /* Compute the number of OpenMP threads and their rank running on microVP #0 */
    instance->mvps[0]->threads[0].num_threads = num_threads ;
    instance->mvps[0]->threads[0].rank = 0 ;
    instance->mvps[0]->threads[0].team = t->team ;
    instance->mvps[0]->threads[0].mvp = instance->mvps[0] ;
    instance->mvps[0]->threads[0].single_current = t->team->single_last_current ;
    instance->mvps[0]->threads[0].for_dyn_current = t->team->for_dyn_last_current ;

    sctk_nodebug( "__mpcomp_start_parallel_region: starting with current_single = %d",
	t->team->single_last_current ) ;

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]) ;

    sctk_nodebug( "__mpcomp_start_parallel_region: end of in-order scheduling" ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( instance->mvps[0] ) ;

    /* Update team info for last values */
    /* TODO put inside an inlined function */
    t->team->single_last_current = instance->mvps[0]->threads[0].single_current ;
    t->team->for_dyn_last_current = instance->mvps[0]->threads[0].for_dyn_current ;

    sctk_nodebug( "__mpcomp_start_parallel_region: ending with current_single = %d",
	t->team->single_last_current ) ;

    /* Finish the half barrier by spinning on the root value */
#if MPCOMP_USE_ATOMICS
    // while (sctk_atomics_load_int(&(root->barrier)) != root->barrier_num_threads ) 
    while (sctk_atomics_load_int(&(new_root->barrier)) != new_root->barrier_num_threads ) 
    {
      sctk_thread_yield() ;
    }
    // sctk_atomics_store_int(&(root->barrier), 0) ;
    sctk_atomics_store_int(&(new_root->barrier), 0) ;
#else
    // while (root->barrier != root->barrier_num_threads ) 
    while (new_root->barrier != new_root->barrier_num_threads ) 
    {
      sctk_thread_yield() ;
    }
    // root->barrier = 0 ;
    new_root->barrier = 0 ;
#endif


    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t ;


#if 0
    /* Sequential check of tree coherency */
    sctk_assert( root->barrier == 0 ) ;
    for ( i = 0 ; i < root->nb_children ; i++ ) {
      sctk_assert( root->children.node[i]->barrier == 0 ) ;
    }
#endif

  } else {
    /* TODO */
    not_implemented() ;
  }

  sctk_nodebug( "__mpcomp_start_parallel_region: ========== EXIT PARALLEL REGION =============" ) ;

}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void * mpcomp_slave_mvp_node( void * arg ) {
  int index ;
  mpcomp_mvp_t * mvp ; /* Current microVP */
  mpcomp_node_t * n ; /* Spinning node + Traversing node */
  mpcomp_node_t * root ; /* Tree root */

  /* Get the current microVP */
  mvp = (mpcomp_mvp_t *)arg ;

  /* Capture the node to spin on */
  n = mvp->to_run ;

  /* Get the root of the topology tree */
  root = mvp->root ;

  sctk_nodebug( "mpcomp_slave_mvp_node: Polling address %p -> father is %p",
      &(mvp->slave_running), mvp->father ) ;

  while (mvp->enable) {
    int i ;
    int num_threads_mvp ;

    sctk_nodebug( "mpcomp_slave_mvp_node: Polling address %p with ID %d",
	&(n->slave_running), 0) ;

    /* Spinning on the designed node */
    sctk_thread_wait_for_value_and_poll( (int*)&(n->slave_running), 1, NULL, NULL ) ;
    n->slave_running = 0 ;

    sctk_nodebug( "mpcomp_slave_mvp_node: wake up -> go to children using function %p", n->func ) ;

    /* Wake up children node */
    while ( n->child_type != MPCOMP_CHILDREN_LEAF ) {
      sctk_nodebug( "mpcomp_slave_mvp_node: children node passing function %p", n->func ) ;
	n->children.node[0]->func = n->func ;
	n->children.node[0]->shared = n->shared ;
	n->children.node[0]->team_info = n->team_info ;
	sctk_assert( n->team_info != NULL ) ;
	n->children.node[0]->num_threads = n->num_threads ;
      for ( i = 1 ; i < n->nb_children ; i++ ) {
	n->children.node[i]->func = n->func ;
	n->children.node[i]->shared = n->shared ;
	n->children.node[i]->team_info = n->team_info ;
	sctk_assert( n->team_info != NULL ) ;
	n->children.node[i]->num_threads = n->num_threads ;
	n->children.node[i]->slave_running = 1 ;
      }
     TODO(put the real barrier_num_threads)
      n->barrier_num_threads = n->nb_children ;
      n = n->children.node[0] ;
    }
    /* Wake up children leaf */
    sctk_assert( n->child_type == MPCOMP_CHILDREN_LEAF ) ;
    sctk_nodebug( "mpcomp_slave_mvp_node: children leaf will use %p", n->func ) ;
    for ( i = 1 ; i < n->nb_children ; i++ ) {
      /*
      n->children.leaf[i]->func = n->func ;
      n->children.leaf[i]->shared = n->shared ;
      n->children.leaf[i]->num_threads = n->num_threads ;
      */
      n->children.leaf[i]->slave_running = 1 ;
    }
    n->barrier_num_threads = n->nb_children ;

    n = mvp->to_run ;
    mvp->func = n->func ;
    mvp->shared = n->shared ;

    /* Compute the number of threads for this microVP */
    /* TODO */
    int fetched_num_threads = n->num_threads ;
    num_threads_mvp = 1 ;
    if ( fetched_num_threads <= mvp->rank ) {
      num_threads_mvp = 0 ;
    }

    for ( i = 0 ; i < num_threads_mvp ; i++ ) {
      int rank ;
      /* Allocate this thread if needed */


      /* Compute the rank of this thread */
      rank = mvp->rank ;

      mvp->threads[i].rank = rank ;
      mvp->threads[i].num_threads = n->num_threads ;
      mvp->threads[i].mvp = mvp ;
      mvp->threads[i].team =  mvp->father->team_info ;
      mvp->threads[i].single_current = mvp->father->team_info->single_last_current ;
      mvp->threads[i].for_dyn_current = mvp->father->team_info->for_dyn_last_current ;
      //mvp->threads[i].start_steal_chunk = -1; //CHUNK STEALING
      sctk_nodebug( "mpcomp_slave_mvp: Got num threads %d",
	  mvp->threads[i].num_threads ) ;
      /* TODO finish */
    }

    /* Update the total number of threads for this microVP */
    mvp->nb_threads = num_threads_mvp ;

    /* Run */
    in_order_scheduler( mvp ) ;

    sctk_nodebug( "mpcomp_slave_mvp_node: end of in-order scheduling" ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( mvp ) ;

  }

  return NULL ;
}

/**
  Entry point for microVP working on their own
  Spinning on a variables inside the microVP.
 */
void * mpcomp_slave_mvp_leaf( void * arg ) {
  mpcomp_mvp_t * mvp ;

  /* Grab the current microVP */
  mvp = (mpcomp_mvp_t *)arg ;

  sctk_nodebug( "mpcomp_slave_mvp_leaf: Polling address %p -> father is %p",
      &(mvp->slave_running), mvp->father ) ;
  sctk_nodebug( "mpcomp_slave_mvp_leaf: Will get value from father %p", mvp->father ) ;

  /* Spin while this microVP is alive */
  while (mvp->enable) {
    int i ;
    int num_threads_mvp ;

    sctk_nodebug( "mpcomp_slave_mvp_leaf: Polling address %p with ID %d",
	&(mvp->slave_running), 0 ) ;

    /* Spin for new parallel region */
    sctk_thread_wait_for_value_and_poll( (int*)&(mvp->slave_running), 1, NULL, NULL ) ;
    TODO("maybe wrong if multiple mVPs are waitning on the same node")
    mvp->slave_running = 0 ;

    sctk_nodebug( "mpcomp_slave_mvp_leaf: wake up" ) ;

    mvp->func = mvp->father->func ;
    mvp->shared = mvp->father->shared ;

    sctk_nodebug( "mpcomp_slave_mvp_leaf: Function for this mvp %p, #threads %d (father %p)", 
	mvp->func, mvp->father->num_threads, mvp->father ) ;

    /* Compute the number of threads for this microVP */
    /* TODO */
    int fetched_num_threads = mvp->father->num_threads ;
    num_threads_mvp = 1 ;
    if ( fetched_num_threads <= mvp->rank ) {
      num_threads_mvp = 0 ;
    }

INFO("__mpcomp_flush: need to call mpcomp_macro_scheduler")

    for ( i = 0 ; i < num_threads_mvp ; i++ ) {
      int rank ;
      /* Allocate this thread if needed */

      mvp->threads[i].mvp = mvp ;

      /* Compute the rank of this thread */
      rank = mvp->rank ;

      mvp->threads[i].rank = rank ;

      /* Copy information */
      mvp->threads[i].num_threads = mvp->father->num_threads ;

      mvp->threads[i].team = mvp->father->team_info ;

      mvp->threads[i].single_current = mvp->father->team_info->single_last_current ;
      mvp->threads[i].for_dyn_current = mvp->father->team_info->for_dyn_last_current ;
      //mvp->threads[i].start_steal_chunk = -1; //CHUNK STEALING
      sctk_nodebug( "mpcomp_slave_mvp_leaf: Got num threads %d",
	  mvp->threads[i].num_threads ) ;
      /* TODO finish */
    }

    /* Update the total number of threads for this microVP */
    mvp->nb_threads = num_threads_mvp ;

    /* Run */
    in_order_scheduler( mvp ) ;

    sctk_nodebug( "mpcomp_slave_mvp_leaf: end of in-order scheduling" ) ;

    /* Half barrier */
    __mpcomp_internal_half_barrier( mvp ) ;

  }

  return NULL ;
}

void in_order_scheduler( mpcomp_mvp_t * mvp ) {

  /*
     for i = 0 ; i < #threads in this mVP ; i++ )
     	if (ctx ==0) switch HLS, switch thread
     	call function of threads[i]
	done = 1
     #threads = 0
     */

  int i ;

    sctk_nodebug( "in_order_scheduler: Starting to schedule %d thread(s)", mvp->nb_threads ) ;

  for ( i = 0 ; i < mvp->nb_threads ; i++ ) {
    /* TODO handle out of order */

    sctk_openmp_thread_tls = &mvp->threads[i];

    sctk_assert( ((mpcomp_thread_t *)sctk_openmp_thread_tls)->team != NULL ) ;
    sctk_assert( mvp != NULL);  

    mvp->func( mvp->shared ) ;
    mvp->threads[i].done = 1 ;
  }
}

/* Create contextes for non-terminated threads of the same microVP */
void expand_microthreads() {
  /*
     NEW version of fork_when_blocked

     get mpcomp_thread
     if (ctx==0 && #threads > 1)
     	get index inside the microVP
	for ( i = index+1 ; i < #threads ; i++ ) {
	  context=1
	  update func to wrapper_out_of_order_scheduler
	  create stack+ctx
	}
	context=2
     */
}

/* Function call when scheduling another thread while the current one is still alive */
void out_of_order_scheduler() {
  /* Assume 'expand_microthreads' has been called */

  /* Exit after scheduling one thread */

  /*
  i = index of current thread in current mVP
  while i < #threads
    if is_running[i]!=0 && done[i] == 0
    	swap context between i and current
	restore tls mpcomp_thread
	break ;
    i++
    */
}



/*
 * Return the number of threads used in the current team (direct enclosing
 * parallel region).
 * If called from a sequential part of the program, this function returns 1.
 * (See OpenMP API 2.5 Section 3.2.2)
 */
int
mpcomp_get_num_threads (void)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  sctk_nodebug( "mpcomp_get_num_threads: entering" ) ;

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  return t->num_threads;
}

void
mpcomp_set_num_threads(int num_threads) 
{
 mpcomp_thread_t * t;

 __mpcomp_init ();

 sctk_nodebug("mpcomp_set_num_threads: entering");

 t = sctk_openmp_thread_tls;
 sctk_assert( t != NULL);

 t->num_threads = num_threads;

}

/*
 * Return the thread number (ID) of the current thread whitin its team.
 * The master thread of each team has a thread ID equals to 0.
 * This function returns 0 if called from a serial part of the code (or from a
 * flatterned nested parallel region).
 * See OpenMP API 2.5 Section 3.2.4
 */
int
mpcomp_get_thread_num (void)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  sctk_nodebug( "mpcomp_get_thread_num: entering" ) ;

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  return t->rank;

}

/*
 * Return the maximum number of threads that can be used inside a parallel region.
 * This function may be called either from serial or parallel parts of the program.
 * See OpenMP API 2.5 Section 3.2.3
 */
int
mpcomp_get_max_threads (void)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  return t->icvs.nthreads_var;
}

/* timing routines */
double
mpcomp_get_wtime (void)
{
  double res;
  struct timeval tp;

  gettimeofday (&tp, NULL);
  res = tp.tv_sec + tp.tv_usec * 0.000001;

  sctk_nodebug ("Wtime = %f", res);
  return res;
}

double 
mpcomp_get_wtick (void)
{
  return 10e-6;
}

void __mpcomp_flush() 
{
/* TODO TEMP only return, but need to handle oversubscribing */
}





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
/* #                                                                      # */
/* ######################################################################## */
#ifndef __mpcomp_internal__H
#define __mpcomp_internal__H

#include "mpcomp.h"
#include "sctk.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"
#include "sctk_context.h"
#include "sctk_tls.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*****************
 ****** MACROS 
 *****************/

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 1

/* Maximum number of threads for each team of a parallel region */
#define MPCOMP_MAX_THREADS		256
/* Number of threads per microVP */
#define MPCOMP_MAX_THREADS_PER_MICROVP	8


/* Maximum number of alive 'for dynamic' construct */
#define MPCOMP_MAX_ALIVE_FOR_DYN 	1023
/* Maximum number of alive 'for guided' construct */
#define MPCOMP_MAX_ALIVE_FOR_GUIDED 	1023


#define MPCOMP_NOWAIT_STOP_SYMBOL	(-2)

/* Uncomment to enable coherency checking */
// #define MPCOMP_COHERENCY_CHECKING 1
#define MPCOMP_OVERFLOW_CHECKING 0

#define MPCOMP_COMBINED_NONE 0
#define MPCOMP_COMBINED_SECTION 1
#define MPCOMP_COMBINED_LOOP 2

/* MACRO FOR PERFORMANCE */
#define MPCOMP_MALLOC_ON_NODE	1
#define MPCOMP_TRANSFER_INFO_ON_NODES 0

#define MPCOMP_CHUNKS_NOT_AVAIL 1
#define MPCOMP_CHUNKS_AVAIL     2

#define MPCOMP_TASK 1

#if MPCOMP_TASK

#define MPCOMP_TASK_LARCENY_MODE 0
/*******************************
 * 0: hierarchical stealing
 * 1: random stealing
 * 2: round-robin stealing
 * 3: production factor stealing
 ********************************/

#define MPCOMP_TASK_MAX_DELAYED 1024

/* Tasks property bitmasks */
#define MPCOMP_TASK_UNDEFERRED   0x00000001 /* A task for which execution is not deferred
					       with respect to its generating task region */
#define MPCOMP_TASK_TIED         0x00000002 /* A task that must be executed entirely by
					       the same thread */
#define MPCOMP_TASK_IMPLICIT     0x00000004 /* A task generated by implicit parallel region
					       or when a parallel construct is encountered */
#define MPCOMP_TASK_INCLUDED     0x00000008 /* A undeferred task that is sequentially included
					       in generating task region (executed immediately) */
#define MPCOMP_TASK_FINAL        0x00000010 /* A task which forces his children to be included  */
#endif //MPCOMP_TASK

/*****************
 ************ ENUM 
 *****************/

     /* Type of element in the stack for dynamic work stealing */
     typedef enum mpcomp_elem_stack_type_t {
	  MPCOMP_ELEM_STACK_NODE = 1,
	  MPCOMP_ELEM_STACK_LEAF = 2,
     } mpcomp_elem_stack_type_t;

     /* Type of children in the topology tree */
     typedef enum mpcomp_children_t {
	  MPCOMP_CHILDREN_NODE = 1,
	  MPCOMP_CHILDREN_LEAF = 2,
     } mpcomp_children_t;


     typedef enum mpcomp_context_t {
	  MPCOMP_CONTEXT_IN_ORDER = 1,
	  MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
	  MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
     } mpcomp_context_t;

     enum mpcomp_topo_obj_type {
	  MPCOMP_TOPO_OBJ_SOCKET, 
	  MPCOMP_TOPO_OBJ_CORE, 
	  MPCOMP_TOPO_OBJ_THREAD, 
	  MPCOMP_TOPO_OBJ_COUNT
     };

#if MPCOMP_TASK
    typedef enum mpcomp_tasklist_type_t {
	  MPCOMP_TASK_TYPE_NEW = 0,
	  MPCOMP_TASK_TYPE_UNTIED = 1,
	  MPCOMP_TASK_TYPE_COUNT = 2,
     } mpcomp_tasklist_type_t;

     typedef enum mpcomp_task_larceny_mode_t {
	  MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL = 0,
	  MPCOMP_TASK_LARCENY_MODE_RANDOM = 1,
	  MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER = 2,
	  MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN = 3,
	  MPCOMP_TASK_LARCENY_MODE_PRODUCER = 4,
	  MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER = 5,
	  MPCOMP_TASK_LARCENY_MODE_COUNT = 6,
     } mpcomp_task_larceny_mode_t;
#endif


/*****************
 ****** STRUCTURES 
 *****************/

     /* Global Internal Control Variables */
     /* One structure per OpenMP instance */
     typedef struct mpcomp_global_icv_s 
     {
	  omp_sched_t def_sched_var;	/* Default schedule when no 'schedule' clause
					   is present */
	  int bind_var;                 /* Is the OpenMP threads bound to cpu cores */
	  int stacksize_var;            /* Size of the stack per thread (in Bytes) */
	  int active_wait_policy_var;   /* Is the threads wait policy active or passive */
	  int thread_limit_var;         /* Number of Open threads to use for the whole program */
	  int max_active_levels_var;    /* Maximum number of nested active parallel regions */
	  int nmicrovps_var;		/* Number of VPs */
     } mpcomp_global_icv_t;


     /* Local Internal Control Variables */
     /* One structure per OpenMP thread */
     typedef struct mpcomp_local_icv_s 
     {
	  int nthreads_var;		/* Number of threads for the next team 
					   creation */
	  int dyn_var;		        /* Is dynamic thread adjustement on? */
	  int nest_var;		        /* Is nested OpenMP handled/allowed? */
	  omp_sched_t run_sched_var;	/* Schedule to use when a 'schedule' clause is
					   set to 'runtime' */
	  int modifier_sched_var;	/* Size of chunks for loop schedule */
     } mpcomp_local_icv_t;

     
    /* Integer atomic with padding to avoid false sharing */
     typedef struct mpcomp_atomic_int_pad_s
     {
	  sctk_atomics_int i;   /* Value of the integer */
	  char pad[8];          /* Padding */
     } mpcomp_atomic_int_pad_t;


     /* Element of the stack for dynamic work stealing */
     typedef struct mpcomp_elem_stack_s
     {
	  union node_leaf {
	       struct mpcomp_node_s *node;
	       struct mpcomp_mvp_s *leaf;
	  } elem;                               /* Stack element */
	  enum mpcomp_elem_stack_type_t type;   /* Type of the 'elem' field */
     } mpcomp_elem_stack_t;

     
     /* Stack structure for dynamic work stealing */
     typedef struct mpcomp_stack_node_leaf_s
     {
	  struct mpcomp_elem_stack_s **elements;   /* List of elements */
	  enum mpcomp_children_t *child_type;      /* Type of childs : nodes or leafs */
	  int max_elements;                        /* Number of max elements */
	  int n_elements;                          /* Corresponds to the head of the stack */
     } mpcomp_stack_node_leaf_t;

#if MPCOMP_TASK
     /* Property of an OpenMP task */
     typedef unsigned int mpcomp_task_property_t;

     /* OpenMP task data structure */
     struct mpcomp_task_s
     {
	  void *(*func) (void *);             /* Function to execute */
	  void *data;                         /* Arguments of the function */
	  mpcomp_task_property_t property;        /* Task property */
	  struct mpcomp_task_s *parent;       /* Mother task */
	  struct mpcomp_task_s *children;     /* Children list */
	  struct mpcomp_task_s *prev_child;   /* Prev sister task in mother task's children list */
	  struct mpcomp_task_s *next_child;   /* Next sister task in mother task's children list */
	  sctk_spinlock_t children_lock;      /* Lock for the task's children list */
	  struct mpcomp_thread_s *thread;     /* The thread owning the task */
	  struct mpcomp_task_list_s *list;    /* The current list of the task */
	  struct mpcomp_task_s *prev;         /* Prev task in the thread's task list */
	  struct mpcomp_task_s *next;         /* Next task in the thread's task list */
	  int depth;
     };

     /* OpenMP task list data structure */
     struct mpcomp_task_list_s
     {
	  sctk_atomics_int nb_elements;    /* Number of tasks in the list */
	  sctk_spinlock_t lock;            /* Lock of the list */
	  struct mpcomp_task_s *head;      /* First task of the list */
	  struct mpcomp_task_s *tail;      /* Last task of the list */
 	  int total;                       /* Total number of tasks pushed in the list */
	  sctk_atomics_int nb_larcenies;   /* Number of tasks in the list */
    };
#endif //MPCOMP_TASK

/* Information to transfer to every thread
 * when entering a new parallel region
 *
 * WARNING: these variables should be written in sequential part
 * and read in parallel region.
 */
typedef struct mpcomp_new_parallel_region_info_s {
	/* MANDATORY INFO */
	void *(*func) (void *);	  /* Function to call by every thread */
	void *shared;		          /* Shared variables (for every thread) */
	long num_threads;		/* Current number of threads in the team */
	struct mpcomp_node_s * new_root ;
	int single_sections_current_save ;
	int for_dyn_current_save;
	long combined_pragma;
	mpcomp_local_icv_t icvs;	/* Set of ICVs for the child team */
	/* OPTIONAL INFO */
	long loop_lb;		        /* Lower bound */
	long loop_b;			/* Upper bound */
	long loop_incr;		/* Step */
	long loop_chunk_size;	        /* Size of each chunk */
	int nb_sections ;
} mpcomp_new_parallel_region_info_t ;

	/* Team of OpenMP threads */
	typedef struct mpcomp_team_s
	{
		mpcomp_new_parallel_region_info_t info ; /* Info for new parallel region */
		int depth;  /* Depth of the current thread (0 = sequential region) */

		/* -- SINGLE/SECTIONS CONSTRUCT -- */
		sctk_atomics_int single_sections_last_current ;
		void *single_copyprivate_data;

		/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
		mpcomp_atomic_int_pad_t for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN + 1];

		/* ORDERED CONSTRUCT */
		volatile int next_ordered_offset; 

#if MPCOMP_TASK
		volatile int tasking_init_done;	        /* Thread team task's init tag */
		sctk_atomics_int tasklist_init_done;
		int tasklist_depth[MPCOMP_TASK_TYPE_COUNT];   /* Depth in the tree of task lists */
		int task_larceny_mode;
	  int task_nesting_max;
	  sctk_atomics_int nb_tasks;
#endif //MPCOMP_TASK
	} mpcomp_team_t;


     /* OpenMP thread */
typedef struct mpcomp_thread_s
{
	sctk_mctx_t uc;		/* Context (initializes registers, ...)
											 Initialized when another thread is 
											 blocked -- NOT USED -- */
	char *stack;                  /* The stack (initialized when another
																	 thread is blocked) -- NOT USED -- */
	mpcomp_new_parallel_region_info_t info ; /* Information needed when
																							entering a new parallel
																							region */
	long rank;			/* OpenMP rank (0 for master thread per team) */
	struct mpcomp_mvp_s *mvp;     /* Micro-vp of the thread*/
	volatile int done;            /* Done value for in-order scheduler */
	struct mpcomp_instance_s *children_instance; /* Instance for nested parallelism */

	struct mpcomp_instance_s *instance; /* Current instance (team + tree) */

	/* -- SINGLE/SECTIONS CONSTRUCT -- */
	int single_sections_current ;
	int single_sections_target_current ; /* When should we stop the current sections construct? */
	int single_sections_start_current ; /* When started the last sections construct? */

	/* LOOP CONSTRUCT */

	/* -- STATIC FOR LOOP CONSTRUCT -- */
	int static_nb_chunks;         /* Number of total static chunk for the thread */
	int static_current_chunk;     /* Current chunk index */

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	int for_dyn_current;    /* Current position in 'for_dyn_chunk_info' array  */
	/* Chunks array for loop dynamic schedule constructs */
	mpcomp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1]; 
	int for_dyn_total ;
	int * for_dyn_target ; /* Coordinates of target thread to steal */
	int * for_dyn_shift ; /* Shift of target thread to steal */

	/* ORDERED CONSTRUCT */
	int current_ordered_iteration; 

#if MPCOMP_TASK 
	int tasking_init_done;                   /* Thread task's init tag */
	struct mpcomp_task_s *current_task;	   /* Currently running task */
	struct mpcomp_task_list_s *tied_tasks;   /* List of suspended tied tasks */
	int *larceny_order;
#endif //MPCOMP_TASK
} mpcomp_thread_t;


     /* Instance of OpenMP runtime */
     typedef struct mpcomp_instance_s
     {
	  int nb_mvps;	        	  /* Number of microVPs for this instance */
	  struct mpcomp_mvp_s **mvps;	  /* All microVPs of this instance */
	  struct mpcomp_node_s *root;	  /* Root to the tree linking the microVPs */
	  struct mpcomp_team_s *team;   /* Information on the team */
	  int tree_depth ; /* Depth of the tree */
	  int *tree_base; /* Degree per level of the tree 
						 (array of size 'tree_depth' */
	  int *tree_cumulative ; /* Initialized in __mpcomp_build_tree */
	  hwloc_topology_t topology;
#if MPCOMP_TASK
	  int *tree_level_size;
	  int tree_array_size;
	  int *tree_array_first_rank;
#endif /* MPCOMP_TASK */
     } mpcomp_instance_t;


     /* OpenMP Micro VP */
     typedef struct mpcomp_mvp_s
     {
	  sctk_mctx_t vp_context;   /* Context including registers, stack pointer, ... */
	  sctk_thread_t pid; 	    /* Thread ID */

	  mpcomp_new_parallel_region_info_t info ;

	  int nb_threads;           /* Total number of threads running on the Micro VP */
	  int next_nb_threads;
	  struct mpcomp_thread_s threads[MPCOMP_MAX_THREADS_PER_MICROVP];

	  int *tree_rank; 	          /* Array of rank in every node of the tree */
	  int rank;    	                  /* Rank within the microVPs */
	  int min_index ;
          int vp;                         /* VP on which microVP is executed */
	  char pad0[64];                  /* Padding */
	  volatile int enable;
	  char pad1[64];                  /* Padding */
	  struct mpcomp_node_s *root;     /* Root of the topology tree */
	  char pad1_1[64];                /* Padding */
	  struct mpcomp_node_s *father;   /* Father node in the topology tree */
	  char pad1_2[64];                /* Padding */
	  struct mpcomp_node_s *to_run;   /* Specify where the spin waiting for a new 
					     parallel region */
	  char pad2[64];                  /* Padding */
	  volatile int slave_running;
	  char pad3[64];                  /* Padding */

	  /* OMP 3.0 */
#if MPCOMP_TASK
	  struct mpcomp_node_s **tree_array;               /* Array reprensentation of the tree */
	  unsigned tree_array_rank;                        /* Rank in tree_array */ 
	  int *path;                                       /* Path in the tree */
	  struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT]; /* Lists of tasks */
	  struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
	  int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
	  struct drand48_data *tasklist_randBuffer;
#endif /* MPCOMP_TASK */

     } mpcomp_mvp_t;

 

     /* OpenMP Node */
     typedef struct mpcomp_node_s
     {
	  struct mpcomp_node_s *father;   /* Father in the topology tree */
	  long rank;                      /* Rank among children of my father */
	  int depth;                      /* Depth in the tree */
	  int nb_children;                /* Number of children */

	  mpcomp_new_parallel_region_info_t info ;

	  /* The following indices correspond to the 'rank' value in microVPs */
	  int min_index;   /* Flat min index of leaves in this subtree */
	  int max_index;   /* Flat max index of leaves in this subtree */

	  mpcomp_instance_t * instance ; /* Information on the whole team */

	  mpcomp_children_t child_type;       /* Kind of children (node or leaf) */
	  union node_or_leaf {
	       struct mpcomp_node_s **node;
	       struct mpcomp_mvp_s **leaf;
	  } children;                         /* Children list */

	  sctk_spinlock_t lock;	        /* Lock for structure updates */
	  char pad0[64];                /* Padding */
	  volatile int slave_running;
	  char pad1[64];                /* Padding */

	  sctk_atomics_int barrier;	                /* Barrier for the child team */
	  sctk_atomics_int chunks_avail;                /* Flag for presence of chunks 
							   under current node */
	  sctk_atomics_int nb_chunks_empty_children;    /* Counter for presence of chunks */

	  char pad2[64];                       /* Padding */
	  volatile long barrier_done;          /* Is the barrier (for the child team) over? */
	  char pad3[64];                       /* Padding */
	  volatile long barrier_num_threads;   /* Number of threads involved in the barrier */
	  char pad4[64];                       /* Padding */


#if MPCOMP_TASK
	  struct mpcomp_node_s **tree_array;                /* Array reprensentation of the tree */
	  unsigned tree_array_rank;                         /* Rank in tree_array */ 
	  int *path;                                        /* Path in the tree */
	  struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT]; /* Lists of tasks */
	  struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
	  struct drand48_data *tasklist_randBuffer;
#endif /* MPCOMP_TASK */

	  int id_numa;  /* NUMA node on which this node is allocated */
        
	  int num_threads;          /* Number of threads in the current team */
	  void *(*func) (void *);
	  void *shared;
     } mpcomp_node_t;


     /* Stack (maybe the same that mpcomp_stack_node_leaf_s structure) */
     typedef struct mpcomp_stack {
	  mpcomp_node_t **elements;
	  int max_elements;
	  int n_elements;             /* Corresponds to the head of the stack */
     } mpcomp_stack_t;


/*****************
 ****** VARIABLES 
 *****************/

     extern mpcomp_global_icv_t mpcomp_global_icvs;


/*****************
 ****** FUNCTIONS  
 *****************/

     static inline void *mpcomp_malloc(int numa_aware, int size, int node)
     {
     	  if (numa_aware && MPCOMP_MALLOC_ON_NODE)	
     	       return sctk_malloc_on_node(size, node);		
     	  					       
	  return sctk_malloc(size);			
     }

     static inline void mpcomp_free(int numa_aware, void *p, int size)
     {
	  sctk_free(p);			
     }     
     
	static inline void __mpcomp_new_parallel_region_info_init( 
			mpcomp_new_parallel_region_info_t * info ) {
		info->func = NULL ;
		info->shared = NULL ;
		info->num_threads = 0 ;
		info->new_root = NULL ;
		info->combined_pragma = MPCOMP_COMBINED_NONE ;
		info->loop_lb = 0 ;
		info->loop_b = 0 ;
		info->loop_incr = 0 ;
		info->loop_chunk_size = 0 ;
		info->nb_sections = 0 ;
		info->single_sections_current_save = 0 ;
		info->for_dyn_current_save = 0 ;
	}

     
     static inline void __mpcomp_team_init(mpcomp_team_t *team_info)
     {
	  int i;

	  /* -- TEAM INFO -- */
	  __mpcomp_new_parallel_region_info_init( &(team_info->info) ) ;
	  team_info->depth = 0;

	  /* -- SINGLE/SECTIONS CONSTRUCT -- */
	  sctk_atomics_store_int(&(team_info->single_sections_last_current), 0);
	  team_info->single_copyprivate_data = NULL;

	  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	  for (i=0; i<MPCOMP_MAX_ALIVE_FOR_DYN; i++)
	       sctk_atomics_store_int(&(team_info->for_dyn_nb_threads_exited[i].i), 0);
	  sctk_atomics_store_int(
				&(team_info->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i),
				 MPCOMP_NOWAIT_STOP_SYMBOL);

#if MPCOMP_TASK
	  sctk_atomics_store_int(&(team_info->tasklist_init_done), 0);
#endif /* MPCOMP_TASK */
     }

     static inline void __mpcomp_thread_init(mpcomp_thread_t *t, mpcomp_local_icv_t icvs,
					     mpcomp_instance_t *instance )
     {
	  int i;

	  t->stack = NULL;
	  __mpcomp_new_parallel_region_info_init( &(t->info) ) ;
	  t->info.num_threads = 1 ;
	  t->info.icvs = icvs;
	  t->rank = 0;
	  t->mvp = NULL;
	  t->done = 0;
	  t->instance = instance;
	  t->children_instance = NULL;

	  /* -- SINGLE CONSTRUCT -- */
	  t->single_sections_target_current = 0 ;
	  t->single_sections_start_current = 0 ;

	  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
		t->for_dyn_current = 0 ;
		t->for_dyn_total = 0 ;
	  for (i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN+1; i++)
	       sctk_atomics_store_int(&(t->for_dyn_remain[i].i), -1);
	  t->for_dyn_target = NULL ; /* Initialized during the first steal */
	  t->for_dyn_shift = NULL ; /* Initialized during the first steal */

#if MPCOMP_TASK
	  t->tasking_init_done = 0;
	  t->tied_tasks = NULL;
	  t->current_task = NULL;
	  t->larceny_order = NULL;
#endif /* MPCOMP_TASK */
     }


#if MPCOMP_TASK
    /* Task property primitives */
     static inline void mpcomp_task_reset_property(mpcomp_task_property_t *property)
     {
	  *property =0;
     }

     static inline void mpcomp_task_set_property(mpcomp_task_property_t *property, mpcomp_task_property_t mask)
     {
	  *property |= mask;
     }

     static inline void mpcomp_task_unset_property(mpcomp_task_property_t *property, mpcomp_task_property_t mask)
     {
	  *property &= ~(mask);
     }

     static inline int mpcomp_task_property_isset(mpcomp_task_property_t property, mpcomp_task_property_t mask)
     {
	  return (property & mask);
     }


/* Initialization of a task structure */

     static inline void __mpcomp_task_init
     (struct mpcomp_task_s *task,
      void *(*func) (void *),
      void *data,
      mpcomp_thread_t *thread)
     {
	  task->func = func;
	  task->data = data;
	  mpcomp_task_reset_property(&(task->property));
	  task->parent = thread->current_task;
	  if (task->parent)
	       task->depth = task->parent->depth + 1;
	  else
	       task->depth = 0;
	  task->children = NULL;
	  task->prev_child = NULL;
	  task->next_child = NULL;
	  task->children_lock = SCTK_SPINLOCK_INITIALIZER;
	  task->thread = NULL;
	  task->prev = NULL;
	  task->next = NULL;
	  task->list = NULL;
     }


/* Task list primitives */

     static inline void mpcomp_task_list_new(struct mpcomp_task_list_s *list)
     {
	  sctk_atomics_store_int(&list->nb_elements, 0);
	  list->lock = SCTK_SPINLOCK_INITIALIZER;
	  list->head = NULL;
	  list->tail = NULL;
	  list->total = 0;
	  sctk_atomics_store_int(&list->nb_larcenies, 0);
     }

     static inline void mpcomp_task_list_free(struct mpcomp_task_list_s *list)
     {
	  mpcomp_free(1, list, sizeof(struct mpcomp_task_list_s));
     }

     static inline int mpcomp_task_list_isempty(struct mpcomp_task_list_s *list)
     {
	  return (sctk_atomics_load_int(&list->nb_elements) == 0);
     }

     static inline void mpcomp_task_list_pushtohead(struct mpcomp_task_list_s *list, struct mpcomp_task_s *task)
     {
	  if (mpcomp_task_list_isempty(list)) {
	       task->prev = NULL;
	       task->next = NULL;
	       list->head = task;
	       list->tail = task;
	  } else {
	       task->prev = NULL;
	       task->next = list->head;
	       list->head->prev = task;
	       list->head = task;
	  }

	  sctk_atomics_incr_int(&list->nb_elements);
	  list->total++;
	  task->list = list;
     }

     static inline void mpcomp_task_list_pushtotail(struct mpcomp_task_list_s *list, struct mpcomp_task_s *task)
     {
	  if (mpcomp_task_list_isempty(list)) {
	       task->prev = NULL;
	       task->next = NULL;
	       list->head = task;
	       list->tail = task;
	  } else {
	       task->prev = list->tail;
	       task->next = NULL;
	       list->tail->next = task;
	       list->tail = task;
	  }
	  
	  sctk_atomics_incr_int(&list->nb_elements);
	  list->total++;
	  task->list = list;
     }

     static inline struct mpcomp_task_s *mpcomp_task_list_popfromhead(struct mpcomp_task_list_s *list)
     {
	  if (!mpcomp_task_list_isempty(list)) {
	       struct mpcomp_task_s *task = list->head;
	       if (task->list != NULL) {
		    if (task->next)
			 task->next->prev = NULL;
		    list->head = task->next;
		    sctk_atomics_decr_int(&list->nb_elements);
		    task->list = NULL;

		    return task;
	       }
	  }
	  
	  return NULL;
     }

     static inline struct mpcomp_task_s *mpcomp_task_list_popfromtail(struct mpcomp_task_list_s *list)
     {
	  if (!mpcomp_task_list_isempty(list)) {
	       struct mpcomp_task_s *task = list->tail;
	       if (task->list != NULL) {
		    if (task->prev)
			 task->prev->next = NULL;
		    list->tail = task->prev;
		    sctk_atomics_decr_int(&list->nb_elements);
		    task->list = NULL;

		    return task;
	       }
	  }

	  return NULL;
     } 

     static inline int mpcomp_task_list_remove(struct mpcomp_task_list_s *list, struct mpcomp_task_s *task)
     {
	  if (task->list == NULL || mpcomp_task_list_isempty(list))
	       return -1;

	  if (task == list->head)
	       list->head = task->next;
	  
	  if (task == list->tail)
	       list->tail = task->prev;

	  if (task->next)
	       task->next->prev = task->prev;
	  
	  if (task->prev)
	       task->prev->next = task->next;
	  
	  sctk_atomics_decr_int(&list->nb_elements);
	  task->list = NULL;

	  return 1;	  
     }

     static inline void mpcomp_task_list_lock(struct mpcomp_task_list_s *list)
     {
	  sctk_spinlock_lock(&(list->lock));
     }

     static inline void mpcomp_task_list_unlock(struct mpcomp_task_list_s *list)
     {
	  sctk_spinlock_unlock(&(list->lock));
     }

     static inline void mpcomp_task_list_trylock(struct mpcomp_task_list_s *list)
     {
	  sctk_spinlock_trylock(&(list->lock));
     }

#endif /* MPCOMP_TASK */


     /* mpcomp.c */
     void __mpcomp_init(void);
     void __mpcomp_exit(void);
     void __mpcomp_instance_init(mpcomp_instance_t *instance, int nb_mvps, 
			 struct mpcomp_team_s * team);
     void in_order_scheduler(mpcomp_mvp_t * mvp);
     void * mpcomp_slave_mvp_node(void *arg);
     void * mpcomp_slave_mvp_leaf(void *arg);
     void __mpcomp_internal_half_barrier();
     void __mpcomp_internal_full_barrier();
     void __mpcomp_barrier();

     /* mpcomp_tree.c */
     int __mpcomp_flatten_topology(hwloc_topology_t topology, hwloc_topology_t *flatTopology);
     int __mpcomp_restrict_topology(hwloc_topology_t *restrictedTopology, int nb_mvps);
     int __mpcomp_check_tree_parameters(int n_leaves, int depth, int *degree);
     int __mpcomp_build_default_tree(mpcomp_instance_t *instance);
     int __mpcomp_build_tree(mpcomp_instance_t *instance, int n_leaves, int depth, int *degree);
     int __mpcomp_check_tree_coherency(mpcomp_instance_t *instance);
     void __mpcomp_print_tree(mpcomp_instance_t *instance);
     int *__mpcomp_compute_topo_tree_array(hwloc_topology_t topology, int *depth, int *index) ;

     /* mpcomp_loop_dyn.c */

     /* mpcomp_sections.c */
	 void __mpcomp_sections_init( mpcomp_thread_t * t, int nb_sections ) ;

     /* mpcomp_single.c */
     void __mpcomp_single_coherency_entering_parallel_region(mpcomp_team_t *team_info);

     /* Stack primitives */
     mpcomp_stack_t * __mpcomp_create_stack(int max_elements);
     int __mpcomp_is_stack_empty(mpcomp_stack_t *s);
     void __mpcomp_push(mpcomp_stack_t *s, mpcomp_node_t *n);
     mpcomp_node_t * __mpcomp_pop(mpcomp_stack_t *s);
     void __mpcomp_free_stack(mpcomp_stack_t *s);

     mpcomp_stack_node_leaf_t * __mpcomp_create_stack_node_leaf(int max_elements);
     int __mpcomp_is_stack_node_leaf_empty(mpcomp_stack_node_leaf_t *s);
     void __mpcomp_push_node(mpcomp_stack_node_leaf_t *s, mpcomp_node_t *n);
     void __mpcomp_push_leaf(mpcomp_stack_node_leaf_t *s, mpcomp_mvp_t *n);
     mpcomp_elem_stack_t * __mpcomp_pop_elem_stack(mpcomp_stack_node_leaf_t *s);
     void __mpcomp_free_stack_node_leaf(mpcomp_stack_node_leaf_t *s);


	void
__mpcomp_get_specific_chunk_per_rank (int rank, int nb_threads,
		long lb, long b, long incr,
		long chunk_size, long chunk_num,
		long *from, long *to) ;

long
__mpcomp_get_static_nb_chunks_per_rank (int rank, int nb_threads, long lb,
					long b, long incr, long chunk_size) ;


#if MPCOMP_TASK
     /* mpcomp_task.c */
     void __mpcomp_task_schedule();
     void __mpcomp_task_exit();
#endif /* MPCOMP_TASK */

#ifdef __cplusplus
}
#endif
#endif

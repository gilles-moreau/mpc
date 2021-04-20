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

#ifndef __MPCOMP_TYPES_H__
#define __MPCOMP_TYPES_H__

#define AD_USE_LOCK

#include "mpc_common_debug.h"
#include <mpc_common_asm.h>
#include "sctk_context.h"
#include "mpc_common_types.h"

#include "mpcomp.h"
#include "ompt.h"

#include "sctk_alloc.h"

#include "mpc_common_mcs.h"

#include <stdbool.h>

// #define TLS_ALLOCATORS

#ifdef MPCOMP_USE_INTEL_ABI
	#include "omp_intel_types.h"
#endif /* MPCOMP_USE_INTEL_ABI */

/*******************
 * OMP DEFINITIONS *
 *******************/

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 1

#define MPCOMP_TASK 1
#define MPCOMP_TASKGROUP 1
#define MPCOMP_USE_INTEL_ABI 1

#define MPCOMP_OPENMP_3_0

/* Activate tasking support if OMP 3.0 is on */
#if defined( MPCOMP_OPENMP_3_0 )
	#define MPCOMP_TASK 1
#endif /* MPCOMP_OPENMP_3_0 */

#define MPCOMP_USE_TASKDEP 1

/* Enable/Disable OMPT support */
#define OMPT_SUPPORT 0

/* Maximum number of alive 'for dynamic' and 'for guided'  construct */
#define MPCOMP_MAX_ALIVE_FOR_DYN 3

#define MPCOMP_NOWAIT_STOP_SYMBOL ( -2 )

/* Uncomment to enable coherency checking */
#define MPCOMP_COHERENCY_CHECKING 0
#define MPCOMP_OVERFLOW_CHECKING 0

/* MACRO FOR PERFORMANCE */
#define MPCOMP_MALLOC_ON_NODE 1
#define MPCOMP_TRANSFER_INFO_ON_NODES 0
#define MPCOMP_ALIGN 1

#define MPCOMP_CHUNKS_NOT_AVAIL 1
#define MPCOMP_CHUNKS_AVAIL 2

/* Define macro MPCOMP_MIC in case of Xeon Phi compilation */
#if __MIC__ || __MIC2__
	#define MPCOMP_MIC 1
#endif /* __MIC__ || __MIC2__ */

#if MPCOMP_TASK

	#define MPCOMP_NB_REUSABLE_TASKS 100
	#define MPCOMP_TASKS_DEPTH_JUMP 10

	#if KMP_ARCH_X86
		#define KMP_SIZE_T_MAX ( 0xFFFFFFFF )
	#else
		#define KMP_SIZE_T_MAX ( 0xFFFFFFFFFFFFFFFF )
	#endif

	#define MPCOMP_OMP_4_0

	#define MPCOMP_TASK_DEFAULT_ALIGN 8

	#define MPCOMP_TASK_MAX_DELAYED 600

	/*** Tasks property bitmasks ***/

	/* Task list type and lock type */

	#define MPCOMP_USE_MCS_LOCK 1

	/*** Tasks property bitmasks ***/

	/* A task for which execution is not deferred
	* with respect to its generating task region */
	#define MPCOMP_TASK_UNDEFERRED 0x00000001

	/* A task that must be executed entirely by
	* the same thread */#define MPCOMP_TASK_DEP_GOMP_DEPS_FLAG 8

	#define MPCOMP_TASK_DEP_INTEL_HTABLE_SIZE 997
	#define MPCOMP_TASK_DEP_INTEL_HTABLE_SEED 6

	#define MPCOMP_TASK_DEP_MPC_HTABLE_SIZE 1001 /* 7 * 11 * 13 */
	#define MPCOMP_TASK_DEP_MPC_HTABLE_SEED 2

	#define MPCOMP_TASK_DEP_LOCK_NODE(node) mpc_common_spinlock_lock(&(node->lock))

	#define MPCOMP_TASK_DEP_UNLOCK_NODE(node) mpc_common_spinlock_unlock(&(node->lock))

	#define MPCOMP_TASK_TIED 0x00000002

	/** A task generated by implicit parallel region
	*	or when a parallel construct is encountered 	*/
	#define MPCOMP_TASK_IMPLICIT 0x00000004

	/** A undeferred task that is sequentially
	*	included in generating task region
	* (executed immediately)
	*/
	#define MPCOMP_TASK_INCLUDED 0x00000008

	/* A task which forces his children to be included  */
	#define MPCOMP_TASK_FINAL 0x00000010


	#define MPCOMP_TASK_DEP_GOMP_DEPS_FLAG 8

	#define MPCOMP_TASK_DEP_INTEL_HTABLE_SIZE 997
	#define MPCOMP_TASK_DEP_INTEL_HTABLE_SEED 6

	#define MPCOMP_TASK_DEP_MPC_HTABLE_SIZE 1001 /* 7 * 11 * 13 */
	#define MPCOMP_TASK_DEP_MPC_HTABLE_SEED 2

	#define MPCOMP_TASK_DEP_LOCK_NODE(node) mpc_common_spinlock_lock(&(node->lock))

	#define MPCOMP_TASK_DEP_UNLOCK_NODE(node) mpc_common_spinlock_unlock(&(node->lock))

	enum mpcomp_task_larceny_mode_t
	{
		MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL,
		MPCOMP_TASK_LARCENY_MODE_RANDOM,
		MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER,
		MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN,
		MPCOMP_TASK_LARCENY_MODE_PRODUCER,
		MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER,
		MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM,
		MPCOMP_TASK_LARCENY_MODE_COUNT
	};


#endif

/* OpenMP 5.0 Memory Management */
#define MPCOMP_MAX_ALLOCATORS 256

/****************
 * OPENMP ENUMS *
 ****************/

typedef enum
{
	MPCOMP_META_TREE_UNDEF      = 0,
	MPCOMP_META_TREE_NODE       = 1,
	MPCOMP_META_TREE_LEAF       = 2,
	MPCOMP_META_TREE_NONE       = 3
} mpcomp_meta_tree_type_t;


typedef enum mpcomp_combined_mode_e
{
	MPCOMP_COMBINED_NONE = 0,
	MPCOMP_COMBINED_SECTION = 1,
	MPCOMP_COMBINED_STATIC_LOOP = 2,
	MPCOMP_COMBINED_DYN_LOOP = 3,
	MPCOMP_COMBINED_GUIDED_LOOP = 4,
	MPCOMP_COMBINED_RUNTIME_LOOP = 5,
	MPCOMP_COMBINED_COUNT = 6
} mpcomp_combined_mode_t;

/* Type of element in the stack for dynamic work stealing */
typedef enum mpcomp_elem_stack_type_e
{
	MPCOMP_ELEM_STACK_NODE = 1,
	MPCOMP_ELEM_STACK_LEAF = 2,
	MPCOMP_ELEM_STACK_COUNT = 3,
} mpcomp_elem_stack_type_t;

/* Type of children in the topology tree */
typedef enum mpcomp_children_e
{
	MPCOMP_CHILDREN_NULL = 0,
	MPCOMP_CHILDREN_NODE = 1,
	MPCOMP_CHILDREN_LEAF = 2,
} mpcomp_children_t;

typedef enum mpcomp_context_e
{
	MPCOMP_CONTEXT_IN_ORDER = 1,
	MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
	MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
} mpcomp_context_t;

typedef enum mpcomp_topo_obj_type_e
{
	MPCOMP_TOPO_OBJ_SOCKET,
	MPCOMP_TOPO_OBJ_CORE,
	MPCOMP_TOPO_OBJ_THREAD,
	MPCOMP_TOPO_OBJ_COUNT
} mpcomp_topo_obj_type_t;

typedef enum mpcomp_mode_e
{
	MPCOMP_MODE_SIMPLE_MIXED,
	MPCOMP_MODE_OVERSUBSCRIBED_MIXED,
	MPCOMP_MODE_ALTERNATING,
	MPCOMP_MODE_FULLY_MIXED,
	MPCOMP_MODE_COUNT
} mpcomp_mode_t;

typedef enum mpcomp_affinity_e
{
	MPCOMP_AFFINITY_COMPACT,  /* Distribute over logical PUs */
	MPCOMP_AFFINITY_SCATTER,  /* Distribute over memory controllers */
	MPCOMP_AFFINITY_BALANCED, /* Distribute over physical PUs */
	MPCOMP_AFFINITY_NB
} mpcomp_affinity_t;

/*********
 * new meta elt for task initialisation
 */

typedef enum mpcomp_tree_meta_elt_type_e
{
	MPCOMP_TREE_META_ELT_MVP,
	MPCOMP_TREE_META_ELT_NODE,
	MPCOMP_TREE_META_ELT_COUNT
} mpcomp_tree_meta_elt_type_t;

typedef union mpcomp_tree_meta_elt_ptr_u
{
	struct mpcomp_node_s *node;
	struct mpcomp_mvp_s *mvp;
} mpcomp_tree_meta_elt_ptr_t;

typedef struct mpcomp_tree_meta_elt_s
{
	mpcomp_tree_meta_elt_type_t type;
	mpcomp_tree_meta_elt_ptr_t ptr;
} mpcomp_tree_meta_elt_t;

typedef enum
{
	MPCOMP_MVP_STATE_UNDEF = 0,
	MPCOMP_MVP_STATE_SLEEP,
	MPCOMP_MVP_STATE_AWAKE,
	MPCOMP_MVP_STATE_READY
} mpcomp_mvp_state_t;


typedef enum mpcomp_loop_gen_type_e
{
	MPCOMP_LOOP_TYPE_LONG,
	MPCOMP_LOOP_TYPE_ULL,
} mpcomp_loop_gen_type_t;


#if MPCOMP_TASK

/*** MPCOMP_TASK_INIT_STATUS ***/

typedef enum mpcomp_task_init_status_e {
  MPCOMP_TASK_INIT_STATUS_UNINITIALIZED,
  MPCOMP_TASK_INIT_STATUS_INIT_IN_PROCESS,
  MPCOMP_TASK_INIT_STATUS_INITIALIZED,
  MPCOMP_TASK_INIT_STATUS_COUNT
} mpcomp_task_init_status_t;

typedef enum mpcomp_tasklist_type_e
{
	MPCOMP_TASK_TYPE_NEW = 0,
	MPCOMP_TASK_TYPE_UNTIED = 1,
	MPCOMP_TASK_TYPE_COUNT = 2,
} mpcomp_tasklist_type_t;

/**
 * Don't modify order NONE < IN < OUT */
typedef enum mpcomp_task_dep_type_e {
  MPCOMP_TASK_DEP_NONE = 0,
  MPCOMP_TASK_DEP_IN = 1,
  MPCOMP_TASK_DEP_OUT = 2,
  MPCOMP_TASK_DEP_COUNT = 3,
} mpcomp_task_dep_type_t;

__UNUSED__ static char *mpcomp_task_dep_type_to_string[MPCOMP_TASK_DEP_COUNT] = {
    "MPCOMP_TASK_DEP_NONE", /*  MPCOMP_TASK_DEP_NONE    = 0 */
    "MPCOMP_TASK_DEP_IN  ", /*  MPCOMP_TASK_DEP_IN      = 1 */
    "MPCOMP_TASK_DEP_OUT "  /*  MPCOMP_TASK_DEP_IN      = 2 */
};

typedef enum mpcomp_task_dep_htable_op_e {
  MPCOMP_TASK_DEP_HTABLE_OP_INSERT = 0,
  MPCOMP_TASK_DEP_HTABLE_OP_DELETE = 1,
  MPCOMP_TASK_DEP_HTABLE_OP_SEARCH = 2,
  MPCOMP_TASK_DEP_HTABLE_OP_COUNT = 3,
} mpcomp_task_dep_htable_op_t;

__UNUSED__ static char
    *mpcomp_task_dep_htable_op_to_string[MPCOMP_TASK_DEP_HTABLE_OP_COUNT] = {
        "MPCOMP_TASK_DEP_HTABLE_OP_INSERT", "MPCOMP_TASK_DEP_HTABLE_OP_DELETE",
        "MPCOMP_TASK_DEP_HTABLE_OP_SEARCH"};

typedef enum mpcomp_task_dep_task_status_e {
  MPCOMP_TASK_DEP_TASK_PROCESS_DEP = 0,
  MPCOMP_TASK_DEP_TASK_NOT_EXECUTE = 1,
  MPCOMP_TASK_DEP_TASK_RELEASED = 2,
  MPCOMP_TASK_DEP_TASK_FINALIZED = 3,
  MPCOMP_TASK_DEP_TASK_COUNT = 4
} mpcomp_task_dep_task_status_t;

__UNUSED__ static char *mpcomp_task_dep_task_status_to_string[MPCOMP_TASK_DEP_TASK_COUNT] =
    {"MPCOMP_TASK_DEP_TASK_PROCESS_DEP", "MPCOMP_TASK_DEP_TASK_NOT_EXECUTE",
     "MPCOMP_TASK_DEP_TASK_RELEASED", "MPCOMP_TASK_DEP_TASK_FINALIZED"};

#endif

/**********************
 * STRUCT DEFINITIONS *
 **********************/

/* Global Internal Control Variables
 * One structure per OpenMP instance */
typedef struct mpcomp_global_icv_s
{
	omp_sched_t def_sched_var;  /**< Default schedule when no 'schedule' clause is
                                 present 			*/
	int bind_var;				/**< Is the OpenMP threads bound to cpu cores
                                 */
	int stacksize_var;			/**< Size of the stack per thread (in Bytes)
                                 */
	int active_wait_policy_var; /**< Is the threads wait policy active or passive
                                 */
	int thread_limit_var;		/**< Number of Open threads to use for the whole program
                           */
	int max_active_levels_var;  /**< Maximum number of nested active parallel
                                regions 				*/
	int nmicrovps_var;			/**< Number of VPs
                                */
	int warn_nested;			/**< Emit warning for nested parallelism?
                                */
	int affinity;				/**< OpenMP threads affinity
                                */
} mpcomp_global_icv_t;

/** Local Internal Control Variables
 * One structure per OpenMP thread 				*/
typedef struct mpcomp_local_icv_s
{
	unsigned int nthreads_var; /**< Number of threads for the next team creation
                                */
	int dyn_var;			   /**< Is dynamic thread adjustement on?
                                */
	int nest_var;			   /**< Is nested OpenMP handled/allowed?
                                */
	omp_sched_t run_sched_var; /**< Schedule to use when a 'schedule' clause is
                                set to 'runtime' */
	int modifier_sched_var;	/**< Size of chunks for loop schedule
                                */
	int active_levels_var;	 /**< Number of nested, active enclosing parallel
                                regions 			*/
	int levels_var;			   /**< Number of nested enclosing parallel regions
                                */
  omp_allocator_handle_t def_allocator_var ; /**< Memory allocator to be used by
											   memory allocation routines
											   */
} mpcomp_local_icv_t;

#if MPCOMP_TASK

typedef struct mpcomp_tree_array_global_info_s
{
	int *tree_shape;
	int max_depth;
} mpcomp_tree_array_global_info_t;

typedef struct mpcomp_meta_tree_node_s
{
	/* Generic information */
	mpcomp_meta_tree_type_t type;
	/* Father information */
	int *fathers_array;
	unsigned int fathers_array_size;
	/* Children information */
	int *first_child_array;
	unsigned int *children_num_array;
	unsigned int children_array_size;

	/* Places */

	/* Min index */
	void *user_pointer;
	OPA_int_t children_ready;
} mpcomp_meta_tree_node_t;

typedef struct mpcomp_mvp_thread_args_s
{
	unsigned int rank;
	mpcomp_meta_tree_node_t *array;
	unsigned int place_id;
	unsigned int target_vp;
	unsigned int place_depth;
} mpcomp_mvp_thread_args_t;


/** Property of an OpenMP task */
typedef unsigned int mpcomp_task_property_t;

/* Data structures */
typedef struct mpcomp_task_taskgroup_s
{
	struct mpcomp_task_s *children;
	struct mpcomp_task_taskgroup_s *prev;
	OPA_int_t children_num;
} mpcomp_task_taskgroup_t;

#define MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING 128

#ifdef MPCOMP_USE_MCS_LOCK
	typedef sctk_mcslock_t mpcomp_task_lock_t;
#else  /* MPCOMP_USE_MCS_LOCK */
	typedef mpc_common_spinlock_t mpcomp_task_lock_t;
#endif /* MPCOMP_USE_MCS_LOCK */

/** OpenMP task list data structure */
typedef struct mpcomp_task_list_s
{
	OPA_ptr_t lockfree_head;
	OPA_ptr_t lockfree_tail;
	char pad1[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING - 2 * sizeof( OPA_ptr_t )];
	OPA_ptr_t lockfree_shadow_head;
	char pad2[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING - 1 * sizeof( OPA_ptr_t )];
	OPA_int_t nb_elements;					  /**< Number of tasks in the list */
	mpcomp_task_lock_t mpcomp_task_lock; /**< Lock of the list                                 */
	int total;
	struct mpcomp_task_s *head; /**< First task of the list */
	struct mpcomp_task_s *tail; /**< Last task of the list */
	OPA_int_t nb_larcenies;		/**< Number of tasks in the list */
} mpcomp_task_list_t;

static inline void __task_list_reset( mpcomp_task_list_t *list )
{
	memset( list, 0, sizeof( mpcomp_task_list_t ) );
}


/** OpenMP task data structure */
typedef struct mpcomp_task_s
{
	int depth;						 /**< nested task depth */
	void ( *func )( void * );		 /**< Function to execute */
	void *data;						 /**< Arguments of the function 				    */
	OPA_int_t refcount;				 /**< refcount delay task free                   */
	mpcomp_task_property_t property; /**< Task property
                                      */
	struct mpcomp_task_s *parent;	/**< Mother task
                                      */
	struct mpcomp_thread_s
		*thread; /**< The thread owning the task 				*/
	struct mpcomp_task_list_s
		*list; /**< The current list of the task 				*/

	OPA_ptr_t lockfree_next;	/**< Prev task in the thread's task list if lists are lockfree */
	OPA_ptr_t lockfree_prev;	/**< Next task in the thread's task list if lists are lockfree */
	struct mpcomp_task_s *prev; /**< Prev task in the thread's task list if lists are locked */
	struct mpcomp_task_s *next; /**< Next task in the thread's task list if lists are locked */

	mpcomp_local_icv_t icvs;	  /**< ICVs of the thread that create the task    */
	mpcomp_local_icv_t prev_icvs; /**< ICVs of the thread that create the task */
	struct mpcomp_task_taskgroup_s *taskgroup;
#ifdef MPCOMP_OMP_4_0 /* deps infos */
	struct mpcomp_task_dep_task_infos_s *task_dep_infos;
#endif /* MPCOMP_GOMP_4_0 */

#if 1 //OMPT_SUPPORT
	ompt_data_t ompt_task_data;
	ompt_frame_t ompt_task_frame;
	ompt_task_flag_t ompt_task_type;
#endif /* OMPT_SUPPORT */
	/* TODO if INTEL */
	struct mpcomp_task_s *last_task_alloc; /**< last task allocated by the thread doing if0 pragma */

	bool is_stealed;					/**< is task have been stealed or not */
	int task_size;						/**< Size of allocation task */
	struct mpcomp_task_s *far_ancestor; /**< far parent (MPCOMP_TASKS_DEPTH_JUMP) of the task */

} mpcomp_task_t;

/**
 * Used by mpcomp_mvp_t and mpcomp_node_t
 */
typedef struct mpcomp_task_node_infos_s
{
	/** Lists of  tasks */
	struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT];
	struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
	int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
	struct drand48_data *tasklist_randBuffer;
} mpcomp_task_node_infos_t;

/** mvp and node share same struct */
typedef struct mpcomp_task_mvp_infos_s
{
	/** Lists of  tasks */
	struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT];
	struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
	int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
	struct drand48_data *tasklist_randBuffer;
	int last_thief;
	int lastStolen_tasklist_rank[MPCOMP_TASK_TYPE_COUNT];
} mpcomp_task_mvp_infos_t;

/**
 * Extend mpcomp_thread_t struct for openmp task support
 */
typedef struct mpcomp_task_thread_infos_s
{
	int *larceny_order;
	OPA_int_t status;					   /**< Thread task's init tag */
	struct mpcomp_task_s *current_task;	/**< Currently running task */
	struct mpcomp_task_list_s *tied_tasks; /**< List of suspended tied tasks */
	void *opaque;						   /**< use mcslock buffered */
	int nb_reusable_tasks;				   /**< Number of current tasks reusable */
	struct mpcomp_task_s **reusable_tasks; /**< Reusable tasks buffer */
	int max_task_tot_size;				   /**< max task size */
	bool one_list_per_thread;			   /** True if there is one list for each thread */
	size_t sizeof_kmp_task_t;
} mpcomp_task_thread_infos_t;

/**
 * Extend mpcomp_instance_t struct for openmp task support
 */
typedef struct mpcomp_task_instance_infos_s
{
	int array_tree_total_size;
	int *array_tree_level_first;
	int *array_tree_level_size;
	bool is_initialized;
} mpcomp_task_instance_infos_t;

/**
 * Extend mpcomp_team_t struct for openmp task support
 */
typedef struct mpcomp_task_team_infos_s
{
	int task_nesting_max;						/**< Task max depth in task generation */
	int task_larceny_mode;						/**< Task stealing policy
                                */
	OPA_int_t status;							/**< Thread team task's init status tag */
	OPA_int_t use_task;							/**< Thread team task create task */
	int tasklist_depth[MPCOMP_TASK_TYPE_COUNT]; /**< Depth in the tree of task
                                                 lists 			*/
} mpcomp_task_team_infos_t;


typedef struct mpcomp_task_dep_node_s {
  mpc_common_spinlock_t lock;
  OPA_int_t ref_counter;
  OPA_int_t predecessors;
  struct mpcomp_task_s *task;
  OPA_int_t status;
  struct mpcomp_task_dep_node_list_s *successors;
  bool if_clause;
#if OMPT_SUPPORT
  ompt_dependence_t* ompt_task_deps;
#endif
} mpcomp_task_dep_node_t;

typedef struct mpcomp_task_dep_node_list_s {
  mpcomp_task_dep_node_t *node;
  struct mpcomp_task_dep_node_list_s *next;
} mpcomp_task_dep_node_list_t;

typedef struct mpcomp_task_dep_ht_entry_s {
  uintptr_t base_addr;
  mpcomp_task_dep_node_t *last_out;
  mpcomp_task_dep_node_list_t *last_in;
  struct mpcomp_task_dep_ht_entry_s *next;
} mpcomp_task_dep_ht_entry_t;

typedef struct mpcomp_task_dep_ht_bucket_s {
  int num_entries;
  uint64_t base_addr;
  bool redundant_out;
  uint64_t *base_addr_list;
  mpcomp_task_dep_type_t type;
  mpcomp_task_dep_ht_entry_t *entry;
} mpcomp_task_dep_ht_bucket_t;

/**
 * \param[in] 	addr	uintptr_t	depend addr ptr
 * \param[in] 	size	uint32_t 	max hash size value
 * \param[in] 	seed	uint32_t 	seed value (can be ignored)
 * \return 		hash 	uint32_t 	hash computed value
 */
typedef uint32_t (*mpcomp_task_dep_hash_func_t)(uintptr_t, uint32_t, uint32_t);

typedef struct mpcomp_task_dep_ht_table_s {
  uint32_t hsize;
  uint32_t hseed;
  mpc_common_spinlock_t hlock;
  mpcomp_task_dep_hash_func_t hfunc;
  struct mpcomp_task_dep_ht_bucket_s *buckets;
} mpcomp_task_dep_ht_table_t;

typedef struct mpcomp_task_dep_task_infos_s {
  mpcomp_task_dep_node_t *node;
  struct mpcomp_task_dep_ht_table_s *htable;
} mpcomp_task_dep_task_infos_t;

typedef struct mpcomp_task_stealing_funcs_s {
  char *namePolicy;
  int allowMultipleVictims;
  int (*pre_init)(void);
  int (*get_victim)(int globalRank, int index, mpcomp_tasklist_type_t type);
}mpcomp_task_stealing_funcs_t;

#endif /* MPCOMP_TASK */

/* Loops */


typedef struct mpcomp_loop_long_iter_s
{
	bool up;
	long lb;		 /* Lower bound          */
	long b;			 /* Upper bound          */
	long incr;		 /* Step                 */
	long chunk_size; /* Size of each chunk   */
	long cur_ordered_iter;
} mpcomp_loop_long_iter_t;

typedef struct mpcomp_loop_ull_iter_s
{
	bool up;
	unsigned long long lb;		   /* Lower bound              */
	unsigned long long b;		   /* Upper bound              */
	unsigned long long incr;	   /* Step                     */
	unsigned long long chunk_size; /* Size of each chunk       */
	unsigned long long cur_ordered_iter;
} mpcomp_loop_ull_iter_t;

typedef union mpcomp_loop_gen_iter_u
{
	mpcomp_loop_ull_iter_t mpcomp_ull;
	mpcomp_loop_long_iter_t mpcomp_long;
} mpcomp_loop_gen_iter_t;

typedef struct mpcomp_loop_gen_info_s
{
	int fresh;
	int ischunked;
	mpcomp_loop_gen_type_t type;
	mpcomp_loop_gen_iter_t loop;
} mpcomp_loop_gen_info_t;

/********************
 ****** OpenMP 5.0
 ****** Memory Management
 ********************/

/* Combine a memspace to a full set of traits */
typedef struct mpcomp_alloc_s
{
  omp_memspace_handle_t memspace;
  omp_alloctrait_t traits[omp_atk_partition]; // Assume here that omp_atk_partition is the last argument of the omp_alloctrait_t enum
} mpcomp_alloc_t;

typedef struct mpcomp_recycl_alloc_info_s
{
  int idx_list[MPCOMP_MAX_ALLOCATORS];
  int nb_recycl_allocators;
} mpcomp_recycl_alloc_info_t;

typedef struct mpcomp_alloc_list_s
{
  mpcomp_alloc_t list[MPCOMP_MAX_ALLOCATORS];
  int nb_init_allocators;
  int last_index;
  mpcomp_recycl_alloc_info_t recycling_info;
	mpc_common_spinlock_t lock;
} mpcomp_alloc_list_t;

/********************
 ****** Threadprivate
 ********************/

typedef struct mpcomp_atomic_int_pad_s
{
	OPA_int_t i; /**< Value of the integer */
	char pad[8]; /* Padding */
} mpcomp_atomic_int_pad_t;

/* Information to transfer to every thread
 * when entering a new parallel region
 *
 * WARNING: these variables should be written in sequential part
 * and read in parallel region.
 */
typedef struct mpcomp_new_parallel_region_info_s
{
	/* MANDATORY INFO */
	void *( *func )( void * ); /* Function to call by every thread */
	void *shared;			   /* Shared variables (for every thread) */
	long num_threads;		   /* Current number of threads in the team */
	struct mpcomp_new_parallel_region_info_s *parent;
	struct mpcomp_node_s *new_root;
	int single_sections_current_save;
	mpc_common_spinlock_t update_lock;
	int for_dyn_current_save;
	long combined_pragma;
	mpcomp_local_icv_t icvs; /* Set of ICVs for the child team */

	/* META OPTIONAL INFO FOR ULL SUPPORT */
	mpcomp_loop_gen_info_t loop_infos;
	/* OPTIONAL INFO */
	long loop_lb;		  /* Lower bound */
	long loop_b;		  /* Upper bound */
	long loop_incr;		  /* Step */
	long loop_chunk_size; /* Size of each chunk */
	int nb_sections;

#if 1 // OMPT_SUPPORT
	ompt_data_t ompt_region_data;
        int doing_single;
        int in_single;
#endif /* OMPT_SUPPORT */

} mpcomp_parallel_region_t;

static inline void
__mpcomp_parallel_region_infos_reset( mpcomp_parallel_region_t *info )
{
	assert( info );
	memset( info, 0, sizeof( mpcomp_parallel_region_t ) );
}

static inline void
__mpcomp_parallel_region_infos_init( mpcomp_parallel_region_t *info )
{
	assert( info );
	__mpcomp_parallel_region_infos_reset( info );
	info->combined_pragma = MPCOMP_COMBINED_NONE;
}

/* Team of OpenMP threads */
typedef struct mpcomp_team_s
{
	mpcomp_parallel_region_t info; /* Info for new parallel region */
	int depth;					   /* Depth of the current thread (0 = sequential region) */
	int id;						   /* team unique id */

	/* -- SINGLE/SECTIONS CONSTRUCT -- */
	OPA_int_t single_sections_last_current;
	void *single_copyprivate_data;

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	mpcomp_atomic_int_pad_t
	for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN + 1];

	/* GUIDED LOOP CONSTRUCT */
	volatile int is_first[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
	OPA_ptr_t guided_from[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
	mpc_common_spinlock_t lock;

	/* ORDERED CONSTRUCT */
	int next_ordered_index;
	volatile long next_ordered_offset;
	volatile unsigned long long next_ordered_offset_ull;
	OPA_int_t next_ordered_offset_finalized;
  
	/* ATOMIC/CRITICAL CONSTRUCT */
  mpc_common_spinlock_t atomic_lock;
  mpc_common_spinlock_t critical_lock;
  
#if MPCOMP_TASK
	struct mpcomp_task_team_infos_s task_infos;
#endif // MPCOMP_TASK

} mpcomp_team_t;

/** OpenMP thread struct
 * An OpenMP thread is attached to a MVP,
 * one thread per nested level */
typedef struct mpcomp_thread_s
{

	/* -- Internal specific informations -- */
	unsigned int depth;
	/** OpenMP rank (0 for master thread per team) */
	long rank;
	long tree_array_rank;
	int *tree_array_ancestor_path;
	/* Micro-vp of the thread*/
	struct mpcomp_mvp_s *mvp;
	/** Root node for nested parallel region */
	struct mpcomp_node_s *root;
	/* -- Parallel region information -- */
	/** Information needed when entering a new parallel region */
	mpcomp_parallel_region_t info;
	/** Current instance (team + tree) */
	struct mpcomp_instance_s *instance;
	/** Instance for nested parallelism */
	struct mpcomp_instance_s *children_instance;
	/* -- SINGLE/SECTIONS CONSTRUCT -- */
	/** Thread last single or sections generation number */
	int single_sections_current;
	/** Thread last sections generation number in sections construct */
	int single_sections_target_current;
	/**  Thread first sections generation number in sections construct */
	int single_sections_start_current;
	/** Thread sections number in sections construct */
	int nb_sections;
	/* -- LOOP CONSTRUCT -- */
	/** Handle scheduling type */
	int schedule_type;
	/** Handle scheduling type from wrapper */
	int schedule_is_forced;
	/* -- STATIC FOR LOOP CONSTRUCT -- */
	/* Thread number of total chunk for static loop */
	int static_nb_chunks;
	/** Thread current index for static loop */
	int static_current_chunk;
	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	//TODO MERGE THESE TWO STRUCT ...
	/* Current position in 'for_dyn_chunk_info' array  */
	int for_dyn_current;
	OPA_int_t for_dyn_ull_current;
	/* Chunks array for loop dynamic schedule constructs */
	mpcomp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
	/* Total number of chunks of the thread */
	unsigned long long for_dyn_total[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
	/** Coordinates of target thread to steal */
	int *for_dyn_target;
	/* last target */
	struct mpcomp_thread_s *dyn_last_target;
	/* Shift of target thread to steal */
	int *for_dyn_shift;
	mpc_common_spinlock_t *for_dyn_lock;
	/* Did we just execute the last iteration of the original loop? ( pr35196.c )*/
	int for_dyn_last_loop_iteration;
	/* Thread next ordered index ( common for all active loop in parallel region )*/
	int next_ordered_index;
#if MPCOMP_TASK
	struct mpcomp_task_thread_infos_s task_infos;
#endif // MPCOMP_TASK
#ifdef MPCOMP_USE_INTEL_ABI
	/** Reduction methode index for Intel Runtim */
	int reduction_method;
	/** Thread private common pointer for Intel Runtime */
	struct common_table *th_pri_common;
	/** Thread private head pointer for Intel Runtime */
	struct private_common *th_pri_head;
	/** Number of threads for Intel Runtime */
	long push_num_threads;
	/* -- STATIC FOR LOOP CONSTRUCT -- */
	/** Static Loop is first iteration for Intel Runtime */
	int first_iteration;
	/** Chunk number in static loop construct for Intel Runtime */
	int static_chunk_size_intel;
#endif /* MPCOMP_USE_INTEL_ABI */
#if 1  // OMPT_SUPPORT
	/** Thread state for OMPT */
	ompt_state_t state;
	/** Thread data for OMPT */
	ompt_data_t ompt_thread_data;
#endif /* OMPT_SUPPORT */

  /* OpenMP 5.0 -- Memory Management */

  omp_allocator_handle_t default_allocator;
  // Allocators set (including default memory allocator)
  //  - 1 x (Memspace + Traits) / allocator_handle
  // struct mpcomp_alloc_s *alloc_set;
#ifdef TLS_ALLOCATORS
  struct mpcomp_alloc_list_s allocators;
#endif

	/* MVP prev context */
	//int instance_ghost_rank;
	struct mpcomp_node_s *father_node;

	/** Nested thread chaining ( heap ) */
	struct mpcomp_thread_s *next;
	/** Father thread */
	struct mpcomp_thread_s *father;

	/* copy __kmpc_fork_call args */
	void **args_copy;
	/* Current size of args_copy */
	int temp_argc;

} mpcomp_thread_t;

/* Instance of OpenMP runtime */
typedef struct mpcomp_instance_s
{
	/** Root to the tree linking the microVPs */
	struct mpcomp_node_s *root;
	/** OpenMP information on the team */
	struct mpcomp_team_s *team;

	struct mpcomp_thread_s *master;

	/*-- Instance MVP --*/
	int buffered;

	/** Number of microVPs for this instance   */
	/** All microVPs of this instance  */
	unsigned int nb_mvps;
	struct mpcomp_generic_node_s *mvps;
	int tree_array_size;
	struct mpcomp_generic_node_s *tree_array;
	/*-- Tree array informations --*/
	/** Depth of the tree */
	int tree_depth;
	/** Degree per level of the tree */
	int *tree_base;
	/** Instance microVPs child number per depth */
	int *tree_cumulative;
	/** microVPs number at each depth */
	int *tree_nb_nodes_per_depth;
	int *tree_first_node_per_depth;

	mpcomp_thread_t *thread_ancestor;

#if MPCOMP_TASK
	/** Task information of this instance */
	struct mpcomp_task_instance_infos_s task_infos;
#endif /* MPCOMP_TASK */

} mpcomp_instance_t;

typedef union mpcomp_node_or_leaf_u
{
	struct mpcomp_node_s **node;
	struct mpcomp_mvp_s **leaf;
} mpcomp_node_or_leaf_t;

/* OpenMP Micro VP */
typedef struct mpcomp_mvp_s
{
	sctk_mctx_t vp_context; /* Context including registers, stack pointer, ... */
	mpc_thread_t pid;
	/* -- MVP Thread specific informations --                   */
	/** VP on which microVP is executed                         */
	int thread_vp_idx;
	/** MVP thread structure pointer                            */
	mpc_thread_t thread_self;
	/** MVP keep alive after fall asleep                        */
	volatile int enable;
	/** MVP spinning value in topology tree                     */
	volatile int spin_status;
	struct mpcomp_node_s *spin_node;
	/* -- MVP Tree related informations                         */
	unsigned int depth;
	/** Root of the topology tree                               */
	struct mpcomp_node_s *root;
	/** Father node in the topology tree                        */
	struct mpcomp_node_s *father;
	/** Left sibbling mvp                                       */
	struct mpcomp_mvp_s *prev_brother;
	/* Right sibbling mvp                                       */
	struct mpcomp_mvp_s *next_brother;
	/** Which thread currently running over MVP                 */
	struct mpcomp_thread_s *threads;
	/* -- Parallel Region Information Transfert --              */
	/* Transfert OpenMP instance pointer to OpenMP thread       */
	struct mpcomp_instance_s *instance;
	/* Index in mvps instance array of last nested instance     */
	int instance_mvps_index;
#if MPCOMP_TRANSFER_INFO_ON_NODES
	/* Transfert OpenMP region information to OpenMP thread     */
	mpcomp_parallel_region_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
	mpcomp_thread_t *buffered_threads;
	/* -- Tree array MVP information --                         */
	/* Rank within the microVPs */
	//TODO Remove duplicate value
	int rank;
	/** Rank within sibbling MVP        */
	int local_rank;
	int *tree_array_ancestor_path;
	long tree_array_rank;
	/** Rank within topology tree MVP depth level */
	int stage_rank;
	int global_rank;
	/** Size of topology tree MVP depth level    */
	int stage_size;
	int *tree_rank; /* Array of rank in every node of the tree */
	struct mpcomp_node_s *tree_array_father;
	int min_index[MPCOMP_AFFINITY_NB];
#if MPCOMP_TASK /*      OMP 3.0     */
	struct mpcomp_task_mvp_infos_s task_infos;
#endif /* MPCOMP_TASK */
	struct mpcomp_mvp_rank_chain_elt_s *rank_list;
	struct mpcomp_mvp_saved_context_s *prev_node_father;
} mpcomp_mvp_t;

/* OpenMP Node */
typedef struct mpcomp_node_s
{
	int already_init;
	/* -- MVP Thread specific informations --                   */
	struct mpcomp_meta_tree_node_s *tree_array;
	int *tree_array_ancestor_path;
	long tree_array_rank;
	/** MVP spinning as a node                                  */
	struct mpcomp_mvp_s *mvp;
	/** MVP spinning value in topology tree                     */
	volatile int spin_status;
	/* NUMA node on which this node is allocated                */
	int id_numa;
	/* -- MVP Tree related information --                       */
	/** Depth in the tree                                       */
	int depth;
	/** Father node in the topology tree                        */
	int num_threads;
	int mvp_first_id;
	struct mpcomp_node_s *father;
	struct mpcomp_node_s *prev_father;
	/** Rigth brother node in the topology tree                 */
	struct mpcomp_node_s *prev_brother;
	/** Rigth brother node in the topology tree                 */
	struct mpcomp_node_s *next_brother;
	/* Kind of children (node or leaf)                          */
	mpcomp_children_t child_type;
	/* Number of children                                       */
	int nb_children;
	/* Children list                                            */
	mpcomp_node_or_leaf_t children;
	/* -- Parallel Region Information Transfert --              */
	/* Transfert OpenMP instance pointer to OpenMP thread       */
	struct mpcomp_instance_s *instance;
	/* Index in mvps instance array of last nested instance     */
	int instance_mvps_index;
#if MPCOMP_TRANSFER_INFO_ON_NODES
	/* Transfert OpenMP region information to OpenMP thread     */
	mpcomp_parallel_region_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */

#if defined( MPCOMP_OPENMP_3_0 )
	int instance_stage_size;
	int instance_global_rank;
	int instance_stage_first_rank;
#endif /* MPCOMP_OPENMP_3_0 */
	/* -- Tree array MVP information --                         */
	/** Rank among children of my father -> local rank          */
	int tree_depth;
	int *tree_base;
	int *tree_cumulative;
	int *tree_nb_nodes_per_depth;

	long rank;
	int local_rank;
	int stage_rank;
	int stage_size;
	int global_rank;

	/* Flat min index of leaves in this subtree                 */
	int min_index[MPCOMP_AFFINITY_NB];
	/* -- Barrier specific information --                       */
	/** Threads number already wait on node                     */
	OPA_int_t barrier;
	/** Last generation barrier id perform on node              */
	volatile long barrier_done;
	/** Number of threads involved in the barrier               */
	volatile long barrier_num_threads;
#if MPCOMP_TASK
	struct mpcomp_task_node_infos_s task_infos;
#endif /* MPCOMP_TASK */
	struct mpcomp_node_s *mvp_prev_father;

	/* Tree reduction flag */
	volatile int *isArrived;
	/* Tree reduction data */
	void **reduce_data;

} mpcomp_node_t;


typedef struct mpcomp_node_s *mpcomp_node_ptr_t;

typedef struct mpcomp_mvp_rank_chain_elt_s
{
	struct mpcomp_mvp_rank_chain_elt_s *prev;
	unsigned int rank;
} mpcomp_mvp_rank_chain_elt_t;

typedef struct mpcomp_mvp_saved_context_s
{
	struct mpcomp_node_s *father;
	struct mpcomp_node_s *node;
	unsigned int rank;
	struct mpcomp_mvp_saved_context_s *prev;
} mpcomp_mvp_saved_context_t;

typedef union mpcomp_node_gen_ptr_u
{
	struct mpcomp_mvp_s *mvp;
	struct mpcomp_node_s *node;
} mpcomp_node_gen_ptr_t;

typedef struct mpcomp_generic_node_s
{
	mpcomp_node_gen_ptr_t ptr;
	/* Kind of children (node or leaf) */
	mpcomp_children_t type;
} mpcomp_generic_node_t;

/* Element of the stack for dynamic work stealing */
typedef struct mpcomp_elem_stack_s
{
	union node_leaf
	{
		struct mpcomp_node_s *node;
		struct mpcomp_mvp_s *leaf;
	} elem; /**< Stack element
             */
	mpcomp_elem_stack_type_t
	type; /**< Type of the 'elem' field 				*/
} mpcomp_elem_stack_t;

/* Stack structure for dynamic work stealing */
typedef struct mpcomp_stack_node_leaf_s
{
	mpcomp_elem_stack_t **elements; /**< List of elements
                                     */
	int max_elements;				/**< Number of max elements
                                     */
	int n_elements;					/**< Corresponds to the head of the stack */
} mpcomp_stack_node_leaf_t;

/* Stack (maybe the same that mpcomp_stack_node_leaf_s structure) */
typedef struct mpcomp_stack
{
	struct mpcomp_node_s **elements;
	int max_elements;
	int n_elements; /* Corresponds to the head of the stack */
} mpcomp_stack_t;

/**************
 * OPENMP TLS *
 **************/

extern __thread void *sctk_openmp_thread_tls;
extern mpcomp_global_icv_t mpcomp_global_icvs;
// extern mpcomp_alloc_list_t mpcomp_global_allocators;

#endif /* __MPCOMP_TYPES_H__ */

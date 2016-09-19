#if (!defined(__SCTK_MPCOMP_TASK_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_H__


#if KMP_ARCH_X86
# define KMP_SIZE_T_MAX (0xFFFFFFFF)
#else
# define KMP_SIZE_T_MAX (0xFFFFFFFFFFFFFFFF)
#endif

#define MPCOMP_TASK_LARCENY_MODE 0

/*******************************
 * 0: hierarchical stealing
 * 1: random stealing
 * 2: round-robin stealing
 * 3: production factor stealing
 ********************************/

#define MPCOMP_TASK_MAX_DELAYED 16

/*** Tasks property bitmasks ***/

/* A task for which execution is not deferred
 * with respect to its generating task region */
#define MPCOMP_TASK_UNDEFERRED   0x00000001 

/* A task that must be executed entirely by 
 * the same thread */
#define MPCOMP_TASK_TIED         0x00000002 

/** A task generated by implicit parallel region
 *	or when a parallel construct is encountered 	*/
#define MPCOMP_TASK_IMPLICIT     0x00000004 

/** A undeferred task that is sequentially 
 *	included in generating task region 
 * (executed immediately) 
 */
#define MPCOMP_TASK_INCLUDED     0x00000008 

/* A task which forces his children to be included  */
#define MPCOMP_TASK_FINAL        0x00000010 

typedef enum mpcomp_tasklist_type_t 
{
	MPCOMP_TASK_TYPE_NEW 	= 0,
	MPCOMP_TASK_TYPE_UNTIED = 1,
	MPCOMP_TASK_TYPE_COUNT 	= 2,
} mpcomp_tasklist_type_t;


/** Property of an OpenMP task */
typedef unsigned int mpcomp_task_property_t;

/** OpenMP task data structure */
typedef struct mpcomp_task_s
{
	void (*func) (void *);             	/**< Function to execute 										*/
   void *data;                         /**< Arguments of the function 								*/
   mpcomp_task_property_t property;    /**< Task property 												*/
   struct mpcomp_task_s *parent;       /**< Mother task 													*/
   struct mpcomp_task_s *children;     /**< Children list 												*/
   struct mpcomp_task_s *prev_child;   /**< Prev sister task in mother task's children list 	*/
   struct mpcomp_task_s *next_child;   /**< Next sister task in mother task's children list 	*/
   sctk_spinlock_t children_lock;      /**< Lock for the task's children list 					*/
   struct mpcomp_thread_s *thread;     /**< The thread owning the task 								*/
   struct mpcomp_task_list_s *list;    /**< The current list of the task 							*/
   struct mpcomp_task_s *prev;         /**< Prev task in the thread's task list 					*/
   struct mpcomp_task_s *next;         /**< Next task in the thread's task list 					*/
   int depth;									/**< nested task depth 											*/
	mpcomp_local_icv_t icvs;            /**< ICVs of the thread that create the task 			*/

	/*TODO remove after rebase */
   int isGhostTask;							
} mpcomp_task_t;


/** OpenMP task list data structure */
typedef struct mpcomp_task_list_s
{
	sctk_atomics_int nb_elements;    	/**< Number of tasks in the list 							*/
	sctk_spinlock_t lock;            	/**< Lock of the list 											*/
	struct mpcomp_task_s *head;      	/**< First task of the list 									*/
	struct mpcomp_task_s *tail;     	 	/**< Last task of the list  									*/
	int total;                       	/**< Total number of tasks pushed in the list 			*/
	sctk_atomics_int nb_larcenies;   	/**< Number of tasks in the list 							*/
} mpcomp_task_list_t;

/** Declaration in mpcomp_internal.h 
 *  Break loop of include 
 */
struct mpcomp_node_s; 

/**
 * Used by mpcomp_mvp_t and mpcomp_node_t 
 */
typedef struct mpcomp_task_tree_list_s
{
	struct mpcomp_node_s **tree_array;  										/**< Array representation of the tree 	*/
	unsigned tree_array_rank;           										/**< Rank in tree_array 					*/
	int *path;                          										/**< Path in the tree 						*/
   mpcomp_task_list_t *tasklist[MPCOMP_TASK_TYPE_COUNT]; 				/**< Lists of tasks 							*/
   mpcomp_task_list_t *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
   int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
   struct drand48_data *tasklist_randBuffer;
} mpcomp_task_tree_list_t;


/*** Task property primitives ***/

static inline void 
mpcomp_task_reset_property(mpcomp_task_property_t *property)
{
	*property = 0;
}

static inline void 
mpcomp_task_set_property(mpcomp_task_property_t *property, mpcomp_task_property_t mask)
{
	*property |= mask;
}

static inline void 
mpcomp_task_unset_property(mpcomp_task_property_t *property, mpcomp_task_property_t mask)
{
	*property &= ~( mask );
}

static inline int 
mpcomp_task_property_isset(mpcomp_task_property_t property, mpcomp_task_property_t mask)
{
	return ( property & mask );
}

static inline mpcomp_thread_task_init( mpcomp_thread_t *thread )
{
	sctk_assert( thread != NULL );
	thread->tasking_init_done = 0;
	thread->tied_tasks = NULL;
	thread->current_task = NULL;
	thread->larceny_order = NULL;
	return;
}

/* Initialization of a task structure */
static inline void 
__mpcomp_task_init( mpcomp_task_t *task, void (*func) (void *), void *data, mpcomp_thread_t *thread, int isGhostTask)
{
	sctk_assert( task != NULL );
	sctk_assert( func != NULL ); 
	sctk_assert( thread != NULL );
	/* data can be NULL ?! */

	task->isGhostTask = isGhostTask;
  	task->func = func;
 	task->data = data;
  	mpcomp_task_reset_property(&(task->property));
  	task->parent = thread->current_task;
   task->depth = 0;
   task->children = NULL;
   task->prev_child = NULL;
   task->next_child = NULL;
   task->children_lock = SCTK_SPINLOCK_INITIALIZER;
   task->thread = NULL;
   task->prev = NULL;
   task->next = NULL;
   task->list = NULL;

  	if (task->parent)
	{
          task->depth = task->parent->depth + 1;
	}
}

/* mpcomp_task.c */
void __mpcomp_task_schedule();
void __mpcomp_task_exit();
 
// Round up a size to a power of two specified by val
// Used to insert padding between structures co-allocated using a single malloc() call
static size_t
__kmp_round_up_to_val( size_t size, size_t val ) 
{
    if ( size & ( val - 1 ) ) 
    {
        size &= ~ ( val - 1 );
        if ( size <= KMP_SIZE_T_MAX - val ) 
            size += val;    // Round up if there is no overflow.
    }
    return size;
} // __kmp_round_up_to_va



#endif /* __SCTK_MPCOMP_TASK_H__ */

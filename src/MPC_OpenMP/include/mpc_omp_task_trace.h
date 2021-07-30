# ifndef __MPCOMP_TASK_TRACE_H__
#  define __MPCOMP_TASK_TRACE_H__

# define MPC_OMP_TASK_TRACE_FILE_VERSION    1
# define MPC_OMP_TASK_TRACE_FILE_MAGIC      (0x6B736174) /* 't' 'a' 's' 'k' */

# define MPC_OMP_TASK_TRACE_RECYCLER_CAPACITY   131072

/**
 *  *  A trace writer (1 per thread)
 *   */
typedef struct  mpcomp_task_trace_writer_s
{
    /* the fd */
    int fd;
}               mpcomp_task_trace_writer_t;

/**
 *  * A trace file header
 *   */
typedef struct  mpcomp_task_trace_file_header_s
{
    /* magic */
    int magic;

    /* trace file version */
    int version;

    /* Process ID : kernel PID one (or MPI rank if under a MPI context) */
    int pid;

    /* omp rank */
    int rank;
}               mpcomp_task_trace_file_header_t;

typedef enum    mpcomp_task_trace_record_type_e
{
    MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY,
    MPC_OMP_TASK_TRACE_TYPE_SCHEDULE,
    MPC_OMP_TASK_TRACE_TYPE_CREATE,
    MPC_OMP_TASK_TRACE_TYPE_ASYNC,
    MPC_OMP_TASK_TRACE_TYPE_COUNT
}               mpcomp_task_trace_record_type_t;

/**
 *  * A generic trace file entry
 *   */
typedef struct  mpcomp_task_trace_record_s
{
    /* event time */
    double time;

    /* the record type */
    mpcomp_task_trace_record_type_t type;
}               mpcomp_task_trace_record_t;

typedef struct  mpcomp_task_trace_record_schedule_s
{
    /* inherithance */
    mpcomp_task_trace_record_t parent;

    /* the task label */
    char label[MPC_OMP_TASK_LABEL_MAX_LENGTH];

    /* the task uid */
    int uid;

    /* the task->priority attribute */
    int priority;

    /* the task->omp_priority attribute */
    int omp_priority;

    /* the task properties */
    int properties;

    /* number of predecessors (data dependencies) */
    int predecessors;

    /* number of tasks that were scheduled before this one */
    int schedule_id;
}               mpcomp_task_trace_record_schedule_t;

/* task creation record is the same as schedule one */
typedef mpcomp_task_trace_record_schedule_t mpcomp_task_trace_record_create_t;

typedef struct  mpcomp_task_trace_record_dependency_s
{
    /* inherithance */
    mpcomp_task_trace_record_t parent;

    /* out task uid */
    int out_uid;

    /* in task uid */
    int in_uid;
}               mpcomp_task_trace_record_dependency_t;

typedef struct  mpcomp_task_trace_record_famine_overlap_s
{
    /* inherithance */
    mpcomp_task_trace_record_t parent;

    /* 0 on start, 1 on finish */
    int status;
}               mpcomp_task_trace_record_famine_overlap_t;

typedef struct  mpcomp_task_trace_record_async_s
{
    /* inherithance */
    mpcomp_task_trace_record_t parent;

    /* 0 on start, 1 on finish */
    int status;

    /* when the call occured */
    int when;
}               mpcomp_task_trace_record_async_t;

/**
 *  A record node
 */
typedef struct  mpcomp_task_trace_node_s
{
    /* next node */
    struct mpcomp_task_trace_node_s * next;

    /* the record is allocated right after this struct */
}               mpcomp_task_trace_node_t;

struct mpcomp_task_s;

/**
 *  Per thread trace informations
 */
typedef struct  mpcomp_thread_task_trace_infos_s
{
    /* the trace writer */
    mpcomp_task_trace_writer_t writer;

    /* the head node */
    mpcomp_task_trace_node_t * head;

    /* the tail node */
    mpcomp_task_trace_node_t * tail;

    /* number of nodes to be flushed */
    size_t length;

    /* node recycler */
    mpc_common_recycler_t recyclers[MPC_OMP_TASK_TRACE_TYPE_COUNT];

    /* 1 if between 'begin' / 'end' calls */
    int begun;
}               mpcomp_thread_task_trace_infos_t;

/**
 *  Flush the thread events to the file descriptor
 */
void _mpc_omp_task_trace_flush(void);

/**
 *  Return true if between a 'begin' and 'end' trace section
 */
int _mpc_omp_task_trace_begun(void);

/**
 * Add events to the thread event queue
 */
void _mpc_omp_task_trace_dependency(struct mpcomp_task_s * out, struct mpcomp_task_s * in);
void _mpc_omp_task_trace_schedule(struct mpcomp_task_s * task);
void _mpc_omp_task_trace_create(struct mpcomp_task_s * task);
void _mpc_omp_task_trace_async(int when, int status);

# endif


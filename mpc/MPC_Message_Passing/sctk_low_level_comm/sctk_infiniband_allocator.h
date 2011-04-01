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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK__INFINIBAND_ALLOCATOR_H_
#define __SCTK__INFINIBAND_ALLOCATOR_H_

#include "sctk_list.h"
#include "sctk_launch.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_scheduling.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_qp.h"
#include "sctk.h"

sctk_net_ibv_qp_rail_t   *rail;

/* RC SR structures */
sctk_net_ibv_qp_local_t *rc_sr_local;

/* RC RDMA structures */
sctk_net_ibv_qp_local_t   *rc_rdma_local;


/*-----------------------------------------------------------
 *  PROCESS
 *----------------------------------------------------------*/

typedef struct
{
  sctk_net_ibv_rc_sr_process_t        *rc_sr;
  sctk_net_ibv_rc_rdma_process_t      *rc_rdma;

  /* sequence numbers */
//  sctk_net_ibv_sched_entry_t          *sched;
  sctk_spinlock_t sched_lock;

  /* for debug */
  uint32_t nb_ptp_msg_transfered;
  uint32_t nb_ptp_msg_received;

  /* locks */
  sctk_thread_mutex_t rc_sr_lock;
  sctk_thread_mutex_t rc_rdma_lock;

} sctk_net_ibv_allocator_entry_t;

typedef struct
{
  sctk_net_ibv_allocator_entry_t* entry;
} sctk_net_ibv_allocator_t;

typedef struct sctk_net_ibv_allocator_request_s
{
  sctk_net_ibv_ibuf_type_t      ibuf_type;
  sctk_net_ibv_ibuf_ptp_type_t  ptp_type;
  sctk_net_ibv_allocator_type_t channel;

  int dest_process;
  int dest_task;
  void* msg;
  void* msg_header;

  size_t size;
} sctk_net_ibv_allocator_request_t;


/* lookup for a remote thread entry. The only requirement is to know the
 * process where the task is located. Once, we check in the allocator list.
 * The task entry is created if it doesn't exist*/
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;



#define ALLOCATOR_LOCK(rank, type)                                                  \
{                                                                                   \
  switch(type) {                                                                   \
    case IBV_CHAN_RC_SR:                                                            \
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);      \
    break;                                                                        \
    case IBV_CHAN_RC_RDMA:                                                          \
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);    \
    break;                                                                        \
    default: assume(0); break;                                                      \
  }                                                                                 \
}                                                                                   \

#define ALLOCATOR_UNLOCK(rank, type)                                                \
{                                                                                   \
  switch(type) {                                                                   \
    case IBV_CHAN_RC_SR:                                                            \
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);    \
    break;                                                                        \
    case IBV_CHAN_RC_RDMA:                                                          \
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);  \
    break;                                                                        \
    default: assume(0); break;                                                      \
  }                                                                                 \
}                                                                                   \

/* local tasks */
typedef struct
{
  /* local task nb */
  int task_nb;

  sched_sn_t* remote;
  /* one pending list for each local task */
  struct sctk_list pending_msg;
  struct sctk_list** frag_eager_list;
} sctk_net_ibv_allocator_task_t;


sctk_net_ibv_allocator_task_t all_tasks[MAX_NB_TASKS_PER_PROCESS];
UNUSED static sctk_spinlock_t all_tasks_lock = SCTK_SPINLOCK_INITIALIZER;

/*-----------------------------------------------------------
 *  TASKS
 *----------------------------------------------------------*/
/*
 * lookup for a local thread id. If the entry isn't found, we lock the
 * array and create the entry */
UNUSED static inline int LOOKUP_LOCAL_THREAD_ENTRY(int id) {
  int i;
  int entry_nb = -1;

  for (i=0; i<MAX_NB_TASKS_PER_PROCESS; ++i) {
    sctk_nodebug("(i:%d) Search for entry %d: %d", i, id, all_tasks[i].task_nb);

    if (all_tasks[i].task_nb == -1)
    {
      /* We take the lock and verify if the entry hasn't
       * been modified before locking */
      sctk_spinlock_lock(&all_tasks_lock);
      if (all_tasks[i].task_nb == -1) {

        entry_nb = i;
        /* init list for pending frag eager */
        all_tasks[i].frag_eager_list =
          sctk_calloc(sizeof(struct sctk_list*), sctk_get_total_tasks_number());
        assume(all_tasks[i].frag_eager_list);

        /* init list for pendint messages */
        sctk_list_new(&all_tasks[i].pending_msg, 1,
            sizeof(sctk_net_ibv_sched_pending_header));

        all_tasks[i].remote = sctk_calloc(sizeof(sched_sn_t), sctk_get_total_tasks_number());
        assume(all_tasks[i].remote);

        /* finily, we set the task_nb. Must be done
         * at the end */
        all_tasks[i].task_nb = id;
      }
      sctk_spinlock_unlock(&all_tasks_lock);
    }

    if (all_tasks[i].task_nb == id)
    {
      entry_nb = i;
      break;
    }
  }
  /* Check if the entry has been found */
  if(entry_nb == -1) {
    sctk_debug("Local thread entry %d not found !", id);
    abort();
  }

  return entry_nb;
}


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

sctk_net_ibv_allocator_t *sctk_net_ibv_allocator_new();

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry, sctk_net_ibv_allocator_type_t type);

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_allocator_rc_rdma_process_next_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma);

void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process );

void
sctk_net_ibv_allocator_send_coll_message (
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_ptp_type_t type);

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_allocator_ptp_poll_all();

void sctk_net_ibv_allocator_ptp_lookup_all(int dest);

void
sctk_net_ibv_allocator_unlock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_allocator_lock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest);

char*
sctk_net_ibv_tcp_request(int process, sctk_net_ibv_qp_remote_t *remote, char* host, int* port);

int sctk_net_ibv_tcp_client(char* host, int port, int dest, sctk_net_ibv_qp_remote_t *remote);

void sctk_net_ibv_tcp_server();

void
sctk_net_ibv_allocator_initialize_threads();

#endif

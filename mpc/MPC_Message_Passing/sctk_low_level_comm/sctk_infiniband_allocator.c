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

#include "sctk_list.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_config.h"
#include "sctk_infiniband_profiler.h"
#include "sctk_infiniband_scheduling.h"
#include "sctk_bootstrap.h"
#include "sctk.h"
#include "sctk_thread.h"
#include "sctk_config.h"

sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern pending_entry_t pending[MAX_NB_TASKS_PER_PROCESS];

  sctk_net_ibv_allocator_t*
sctk_net_ibv_allocator_new()
{
  int i;

  /* FIXME: change to HashTables */
  size_t size = sctk_process_number * sizeof(sctk_net_ibv_allocator_entry_t);

  sctk_net_ibv_allocator = sctk_malloc(sizeof(sctk_net_ibv_allocator_t));

  sctk_net_ibv_allocator->entry = sctk_malloc(size);
  assume(sctk_net_ibv_allocator->entry);

  sctk_nodebug("creation : %p", sctk_net_ibv_allocator->entry);
  memset(sctk_net_ibv_allocator->entry, 0, size);

  /* set up procs list */
  for (i=0; i < sctk_process_number; ++i)
  {
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_sr_lock, NULL);
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_rdma_lock, NULL);
    sctk_net_ibv_allocator->entry[i].sched_lock = SCTK_SPINLOCK_INITIALIZER;
  }

  /* set up tasks list */
  for(i=0; i < MAX_NB_TASKS_PER_PROCESS; ++i)
  {
    all_tasks[i].task_nb = -1;
  }

  return sctk_net_ibv_allocator;
}

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      if (sctk_net_ibv_allocator->entry[rank].rc_sr)
      {
        sctk_error("Process %d already registered to sctk_net_ibv_allocator RC_SR", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_sr = entry;

      sctk_ibv_profiler_inc(IBV_QP_CONNECTED);
      break;

    case IBV_CHAN_RC_RDMA:
      if (sctk_net_ibv_allocator->entry[rank].rc_rdma)
      {
        sctk_error("Process %d already registered to chan RC_RDMA", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_rdma = entry;
      sctk_nodebug("Registering %p", &sctk_net_ibv_allocator->entry[rank]);
      break;

    default: assume(0); break;
  }
}

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      return sctk_net_ibv_allocator->entry[rank].rc_sr;
      break;

    case IBV_CHAN_RC_RDMA:
      return sctk_net_ibv_allocator->entry[rank].rc_rdma;
      break;

    default: assume(0); break;
  }
  return NULL;
}

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_send(wc, rc_sr_local, lookup);
}

void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_rdma_local, lookup, dest);
}

/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/
void
sctk_net_ibv_allocator_ptp_lookup(int dest, sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR :
      sctk_net_ibv_cq_lookup(rc_sr_local->recv_cq, 1, sctk_net_ibv_rc_sr_recv_cq, dest, type | IBV_CHAN_RECV);
      sctk_net_ibv_cq_lookup(rc_sr_local->send_cq, ibv_wc_out_number, sctk_net_ibv_rc_sr_send_cq, dest, type | IBV_CHAN_SEND);
      break;

    default: assume(0); break;
  }
}

static sctk_spinlock_t ptp_lock = SCTK_SPINLOCK_INITIALIZER;
void sctk_net_ibv_allocator_ptp_poll_all()
{
  int ret;
  int i = 0;
  if (sctk_spinlock_trylock(&ptp_lock) == 0)
  {
    while((all_tasks[i].task_nb != -1) && (i < MAX_NB_TASKS_PER_PROCESS))
    {
      sctk_nodebug("tasks %d", all_tasks[i].task_nb);
      //      do {
      ret = sctk_net_ibv_sched_poll_pending_msg(i);
      //        } while(ret);
      ++i;
    }

    sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, ibv_wc_in_number, sctk_net_ibv_rc_sr_recv_cq, IBV_CHAN_RECV);
    sctk_net_ibv_cq_poll(rc_sr_local->send_cq, ibv_wc_out_number, sctk_net_ibv_rc_sr_send_cq, IBV_CHAN_SEND);

    sctk_spinlock_unlock(&ptp_lock);
  }
}


void sctk_net_ibv_allocator_ptp_lookup_all(int dest)
{
  int ret;
  int i = 0;

  while( (all_tasks[i].task_nb != -1) && (i < MAX_NB_TASKS_PER_PROCESS))
  {
    //    do {
    /* poll pending messages */
    ret = sctk_net_ibv_sched_poll_pending_msg(i);
    //    } while(ret);
    ++i;
  }

  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_SR);
}

/**
 *  \brief Fonction which sends a PTP message
 */
void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process ) {
  DBG_S(1);
  size_t size;
  sctk_net_ibv_allocator_request_t req;

  /* profile if message is contigous or not (if we can
   * use the zerocopy method or not) */
#if IBV_ENABLE_PROFILE == 1
  void* entire_msg = NULL;
  entire_msg = sctk_net_if_one_msg_in_buffer(msg);
  if (entire_msg) sctk_ibv_profiler_inc(IBV_MSG_DIRECTLY_PINNED);
  else sctk_ibv_profiler_inc(IBV_MSG_NOT_DIRECTLY_PINNED);
  sctk_ibv_profiler_add(IBV_PTP_SIZE, (size + sizeof(sctk_thread_ptp_message_t)));
  sctk_ibv_profiler_inc(IBV_PTP_NB);
#endif

  /* determine message number */
  size = sctk_net_determine_message_size(msg);

  req.size = size;
  req.msg = msg;
  req.dest_process = dest_process;
  req.dest_task = ((sctk_thread_ptp_message_t*) msg)->header.destination;
  req.ptp_type = IBV_PTP;

  if ( (size + sizeof(sctk_thread_ptp_message_t)) +
      RC_SR_HEADER_SIZE <= ibv_eager_threshold) {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);

    /*
     * EAGER MSG
     */
    req.channel = IBV_CHAN_RC_SR;

    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, req);

  } else if ( (size + sizeof(sctk_thread_ptp_message_t)) <= ibv_frag_eager_threshold) {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_ibv_profiler_add(IBV_FRAG_EAGER_SIZE, size + sizeof(sctk_thread_ptp_message_t));

    /*
     * FRAG MSG
     */
    req.channel = IBV_CHAN_RC_SR_FRAG;

    sctk_net_ibv_comp_rc_sr_send_frag_ptp_message (rc_sr_local, req);
  } else {
    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);

    /*
     * RENDEZVOUS
     */
    req.channel = IBV_CHAN_RC_RDMA;

    sctk_net_ibv_comp_rc_rdma_send_request (
        rail, rc_sr_local, req, 1);
  }

  sctk_net_ibv_allocator->entry[dest_process].nb_ptp_msg_transfered++;

  DBG_E(1);
}


void
sctk_net_ibv_allocator_send_coll_message (
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_ptp_type_t type)
{
  DBG_S(1);
  sctk_net_ibv_allocator_request_t req;

  req.ptp_type = type;
  req.size = size;
  req.msg   = msg;
  req.dest_process  = dest_process;
  req.dest_task     = -1;

  if ( (size + RC_SR_HEADER_SIZE) <= ibv_eager_threshold) {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);

    /*
     * EAGER MSG
     */
    req.channel = IBV_CHAN_RC_SR;
    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, req);
//  } else if ( (size + sizeof(sctk_thread_ptp_message_t)) <= ibv_frag_eager_threshold) {
#if 0
  } else {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_ibv_profiler_add(IBV_FRAG_EAGER_SIZE, size + sizeof(sctk_thread_ptp_message_t));

    assume(0);
    /*
     * FRAG MSG
     */
    req.channel = IBV_CHAN_RC_SR_FRAG;
    sctk_net_ibv_comp_rc_sr_send_coll_frag_ptp_message (rc_sr_local, req);
#endif
  }
else {
    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);

    /*
     * RENDEZVOUS
     */
    req.channel = IBV_CHAN_RC_RDMA;
    sctk_net_ibv_comp_rc_rdma_send_request (
        rail, rc_sr_local, req, 0);
  }

  DBG_E(1);
}

/**
 *  Init thread-specific structures. Each thread init its own structures
 *  \param
 *  \return
 */
  void
sctk_net_ibv_allocator_initialize_threads()
{
#if 0
  int task_id;
  int thread_id;
  int i;
  int entry_nb = -1;
  sctk_get_thread_info (&task_id, &thread_id);

  sctk_spinlock_lock(&all_tasks_lock);

  /* lookup for a free entry */
  for (i=0; i < MAX_NB_TASKS_PER_PROCESS; ++i)
  {
    sctk_nodebug("Task nb : %d", all_tasks[i].task_nb);

    if (all_tasks[i].task_nb == -1)
    {
      entry_nb = i;
      sctk_spinlock_unlock(&all_tasks_lock);
      all_tasks[entry_nb].task_nb = task_id;
      all_tasks[entry_nb].frag_eager_list =
        sctk_calloc(sizeof(struct sctk_list*), sctk_get_total_tasks_number());
      assume(all_tasks[entry_nb].frag_eager_list);

//      sctk_list_new(&all_tasks[entry_nb].frag_eager_list, 0, 0);
      sctk_nodebug("%p - Found entry %d -> %d", &all_tasks[entry_nb], i, task_id);
      break;
    }
  }

#warning "We have to move it to support other modules"
  if (entry_nb == -1) {
//    sctk_spinlock_unlock(&all_tasks_lock);
//    assume(0);
  }


  /* init structures */
  sctk_list_new(&all_tasks[entry_nb].pending_msg, 1, sizeof(sctk_net_ibv_sched_pending_header));

  sctk_nodebug("Initializing thread %d with entry %d", task_id, entry_nb);
#endif
}

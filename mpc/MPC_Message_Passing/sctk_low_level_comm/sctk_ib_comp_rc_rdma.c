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

#ifdef MPC_USE_INFINIBAND

#include "sctk_ib_comp_rc_rdma.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_lib.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_coll.h"
#include "sctk_ib_config.h"
#include "sctk_alloc.h"
#include "sctk_buffered_fifo.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_ib_profiler.h"
#include "sctk.h"

#define WAIT_READY(x) \
  while(!x->ready) sctk_thread_yield()

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern sctk_net_ibv_com_entry_t* com_entries;

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
  sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_rdma_create_local(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_qp_local_t* local;

  local = sctk_net_ibv_qp_new(rail);

  sctk_net_ibv_pd_init(local);
  sctk_nodebug("New PD %p", local->pd);

  /* completion queues */
  local->send_cq = sctk_net_ibv_cq_init(rail);
  local->recv_cq = sctk_net_ibv_cq_init(rail);

  return local;
}


sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_allocate_init(
    unsigned int rank,
    sctk_net_ibv_qp_local_t *local)
{
  sctk_net_ibv_rc_rdma_process_t* rdma;

  rdma = sctk_malloc(sizeof(sctk_net_ibv_rc_rdma_process_t));
  assume(rdma);

  sctk_thread_mutex_init(&rdma->lock, NULL);
  SCTK_LIST_HEAD_INIT(&rdma->send);
  SCTK_LIST_HEAD_INIT(&rdma->recv);

  rdma->ready = 0;

  return rdma;
}

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/**
 *  Prepare a RDMA PTP message. Check if the message can be
 *  directely pinned. If it can't, we create a temporary copy
 *  of the message and we pin it.
 *  \param
 *  \return
 */
static inline int
sctk_net_ibv_comp_rc_rdma_prepare_ptp(
    sctk_net_ibv_allocator_request_t req,
    sctk_thread_ptp_message_t *msg_header,
    sctk_thread_ptp_message_t **msg_header_ptr,
    void  **msg_payload_aligned_ptr,
    size_t* aligned_size,
    int directly_pinned)
{
  void* entire_msg = NULL;
  size_t page_size;
  int rc;


  /* 1 STEP : check if the msg isn't packed.
   * If it's not, we can directly pin the msg and avoid an
   * allocation */
  page_size = sctk_net_ibv_mmu_get_pagesize();

  entire_msg = sctk_net_if_one_msg_in_buffer(req.msg);

  if (entire_msg)
  {
    sctk_nodebug("msg _CAN_ be directly pinned");

    /*TODO: use an offset */
    *msg_payload_aligned_ptr =
      entire_msg - (((unsigned long) entire_msg) % page_size);

    *aligned_size =
      req.size + (((unsigned long) entire_msg) % page_size);
    rc = 1;

    sctk_nodebug("%p->%p (%lu->%lu)", entire_msg, *msg_payload_aligned_ptr, req.size, aligned_size);
    *msg_header_ptr = entire_msg;
  } else {
    sctk_nodebug("The msg cannot be directly pinned (size %lu) %p", req.size, req.msg);

    *aligned_size = req.size;
    sctk_posix_memalign(msg_payload_aligned_ptr, page_size, *aligned_size);
    assume(*msg_payload_aligned_ptr);
    sctk_ibv_profiler_inc(IBV_MEM_TRACE);

    *msg_header_ptr = *msg_payload_aligned_ptr;
    rc = 0;

    sctk_net_copy_in_buffer(req.msg, *msg_payload_aligned_ptr);
  }

  /* we copy the MPC header of the message in the request */
  memcpy (msg_header, req.msg,
      sizeof ( sctk_thread_ptp_message_t ));


  return rc;
}

  static sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_get_entry(int dest_process)
{
  sctk_net_ibv_rc_rdma_process_t* rc_rdma_entry;
  rc_rdma_entry = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);

  /* Double check for avoiding to take a lock if we don't need it.*/
  if (!rc_rdma_entry)
  {
    ALLOCATOR_LOCK(dest_process, IBV_CHAN_RC_RDMA);
    rc_rdma_entry = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);

    /*
     * check if the dest process exists in the RDVZ remote list.
     * If it doesn't, we create a structure for the RC_RDMA entry
     */
    if (!rc_rdma_entry)
    {
      sctk_nodebug("Need connection to process %d", dest_process);

      rc_rdma_entry = sctk_net_ibv_comp_rc_rdma_allocate_init(dest_process, rc_rdma_local);

      sctk_net_ibv_allocator_register(dest_process, rc_rdma_entry, IBV_CHAN_RC_RDMA);
    }
    ALLOCATOR_UNLOCK(dest_process, IBV_CHAN_RC_RDMA);
  }

  return rc_rdma_entry;
}

void
sctk_net_ibv_comp_rc_rdma_send_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req,
    uint8_t directly_pinned)
{
  sctk_net_ibv_ibuf_t* ibuf;
  size_t aligned_size = 0;
  sctk_net_ibv_rc_rdma_request_t* request;
  sctk_net_ibv_mmu_entry_t* mmu_entry = NULL;
  size_t size_to_copy = 0;
  int is_directly_pinned = 0;
  sctk_thread_ptp_message_t *msg_header_ptr = NULL;
  void *msg_payload_aligned_ptr;
  sctk_net_ibv_ibuf_header_t* msg_header;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t *entry;
  size_t page_size;

  /* check if the rdma entry is already set. If not, we create the entry */
  entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_get_entry(req.dest_process);

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  request = (sctk_net_ibv_rc_rdma_request_t* )
    RC_SR_PAYLOAD(ibuf->buffer);
  msg_header = RC_SR_HEADER(ibuf->buffer);

  switch (req.ptp_type) {
    case IBV_PTP:
      is_directly_pinned =
        sctk_net_ibv_comp_rc_rdma_prepare_ptp(
            req, &request->msg_header,
            &msg_header_ptr, &msg_payload_aligned_ptr, &aligned_size, directly_pinned);
      size_to_copy = sizeof(sctk_net_ibv_rc_rdma_request_t);
      break;

    case IBV_REDUCE:
    case IBV_BCAST:
      sctk_nodebug("Send RDVZ collective");
      is_directly_pinned = 0;
      page_size = sctk_net_ibv_mmu_get_pagesize();

      aligned_size = req.size;
      sctk_posix_memalign(&msg_payload_aligned_ptr, page_size, req.size);
      assume(msg_payload_aligned_ptr);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);
      msg_header_ptr = msg_payload_aligned_ptr;
      memcpy(msg_payload_aligned_ptr, req.msg, req.size);

      size_to_copy = sizeof(sctk_net_ibv_rc_rdma_request_t)
        - sizeof(sctk_thread_ptp_message_t);
      break;

    default:
      assume(0);
      break;
  }


  /* we register the memory where the message is stored, whatever the
   * RDVZ protocol used */
  mmu_entry = sctk_net_ibv_mmu_register (
      rail, local_rc_sr, msg_payload_aligned_ptr, aligned_size);


  switch (ibv_rdvz_protocol)
  {
    case IBV_RDVZ_READ_PROTOCOL:
      sctk_nodebug("Initialize READ request");

      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);
      entry->ready = 0;
      /* add entry to the list */
      sctk_ib_list_lock(&entry_rc_rdma->recv);
      sctk_ib_list_push_tail(&entry_rc_rdma->recv, &entry->list_header);
      sctk_ib_list_unlock(&entry_rc_rdma->recv);

      request->protocol = RC_RDMA_REQ_READ;
      request->read_req.address = (uintptr_t) msg_header_ptr;
      request->read_req.rkey  = mmu_entry->mr->rkey;
      request->entry_rc_rdma  = entry_rc_rdma;
      request->entry          = entry;
      request->ptp_type         = req.ptp_type;
      request->requested_size = req.size;

      /* prepare and send message  request*/
      sctk_net_ibv_ibuf_send_init(ibuf, size_to_copy + RC_SR_HEADER_SIZE);
      req.size          = size_to_copy;
      req.payload_size  = size_to_copy;
      req.buff_nb       = 1;
      req.total_buffs   = 1;
      sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_RDVZ_REQUEST);

      /* prepare rendezvous entry */
      entry->mmu_entry      = mmu_entry;
      entry->msg_header_ptr = req.msg;
      entry->com_id         = req.com_id;
      entry->requested_size = req.size;
      entry->src_process    = req.dest_process;
      entry->entry_rc_rdma  = entry_rc_rdma;
      entry->psn            = req.psn;
      entry->msg_payload_aligned_ptr = msg_payload_aligned_ptr;
      entry->directly_pinned        = is_directly_pinned;
      entry->ptp_type               = req.ptp_type;
      entry->ready                  = 1;
      break;

    case IBV_RDVZ_WRITE_PROTOCOL:
      not_implemented();
      break;
  }
}

static sctk_net_ibv_rc_rdma_entry_t*
sctk_net_ibv_comp_rc_rdma_entry_send_new(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_ibuf_header_t* msg_header,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_rc_rdma_entry_t* entry_send;
  sctk_net_ibv_mmu_entry_t* mmu_entry;
  size_t page_size = sctk_net_ibv_mmu_get_pagesize();

  /* fill the receive entry */
  entry_send = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
  assume(entry_send);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  entry_send->creation_timestamp = MPC_Wtime();

  sctk_posix_memalign((void**) &(entry_send->msg_payload_ptr),
      page_size, request->requested_size);
  assume(entry_send->msg_payload_ptr);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  mmu_entry = sctk_net_ibv_mmu_register (
      rail, local_rc_sr,
      entry_send->msg_payload_ptr, request->requested_size);
  entry_send->mmu_entry = mmu_entry;
  entry_send->entry_rc_rdma = entry_rc_rdma;

  memcpy(&entry_send->msg_header, &request->msg_header, sizeof(sctk_thread_ptp_message_t));

  entry_send->requested_size = request->requested_size;
  entry_send->src_process = msg_header->src_process;
  entry_send->com_id = msg_header->com_id;
  entry_send->src_task = entry_send->msg_header.header.source;
  entry_send->dest_task = entry_send->msg_header.header.destination;
  entry_send->psn = msg_header->psn;
  entry_send->remote_entry = request->entry;
  sctk_nodebug("request entry : %p", entry_send->remote_entry);
  entry_send->ptp_type = request->ptp_type;

  return entry_send;
}


/**
 *  Send the finish message to the remote msg source to
 *  ack the message.
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_rdma_send_finish(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_ibuf_t   *ibuf_f;
  sctk_net_ibv_rc_rdma_done_t* finish;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  sctk_net_ibv_allocator_request_t req;

  entry = (sctk_net_ibv_rc_rdma_entry_t*) ibuf->supp_ptr;

  /* fill the receive entry */
  ibuf_f = sctk_net_ibv_ibuf_pick(0, 1);
  finish = (sctk_net_ibv_rc_rdma_done_t*) RC_SR_PAYLOAD(ibuf_f->buffer);
  finish->entry_rc_rdma = entry->entry_rc_rdma;
  finish->entry = entry->remote_entry;

  sctk_nodebug("Send finish with psn %lu to process %d", entry->psn, entry->src_process);

  sctk_net_ibv_ibuf_send_init(ibuf_f, sizeof(sctk_net_ibv_rc_rdma_done_t) + RC_SR_HEADER_SIZE);

  req.dest_process = entry->src_process;
  req.dest_task = entry->src_task;
  req.size = sizeof(sctk_net_ibv_rc_rdma_done_t);
  req.payload_size = sizeof(sctk_net_ibv_rc_rdma_done_t);
  req.ptp_type = entry->ptp_type;

  sctk_net_ibv_comp_rc_sr_send(ibuf_f, req, RC_SR_RDVZ_DONE);
}


static void
sctk_net_ibv_comp_rc_rdma_process_rdma_read(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_ibuf_header_t* msg_header,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_ibuf_t   *ibuf;
  sctk_net_ibv_rc_rdma_entry_t* entry;

  /* fill the receive entry */
  ibuf = sctk_net_ibv_ibuf_pick(0, 1);

  entry = sctk_net_ibv_comp_rc_rdma_entry_send_new(
      rail, local_rc_sr, entry_rc_rdma, msg_header, request);
  entry->ready = 0;

  sctk_ib_list_lock(&entry_rc_rdma->send);
  sctk_ib_list_push_tail(&entry_rc_rdma->send, &entry->list_header);
  sctk_ib_list_unlock(&entry_rc_rdma->send);

  sctk_net_ibv_ibuf_rdma_read_init(
      ibuf, entry->msg_payload_ptr,
      entry->mmu_entry->mr->lkey,
      (void*) request->read_req.address, request->read_req.rkey,
      entry->requested_size, entry, entry->src_process);

  sctk_nodebug("Read from %p (size:%lu)", request->read_req.address, entry->requested_size);

  sctk_nodebug("Posting RC_RDMA READ with size %lu", request->requested_size);

  sctk_net_ibv_qp_send_get_wqe(entry->src_process, ibuf);

  ibuf->supp_ptr = entry;

  /* set the entry as ready to be read */
  entry->ready = 1;
}



/**
 *  Analyze the request received
 *  \param
 *  \return
 */
sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_analyze_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_rc_rdma_request_t   *request;
  sctk_net_ibv_rc_rdma_process_t   *entry_rc_rdma = NULL;
  sctk_net_ibv_ibuf_header_t* msg_header;

  request = (sctk_net_ibv_rc_rdma_request_t*) RC_SR_PAYLOAD(ibuf->buffer);
  msg_header = RC_SR_HEADER(ibuf->buffer);
  sctk_nodebug("src_process : %lu", msg_header->src_process);

  /* check if we didn't already initiate a connection */
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(msg_header->src_process, IBV_CHAN_RC_RDMA);

  if (!entry_rc_rdma)
  {

    /* lock the allocator entry for the dest process */
    ALLOCATOR_LOCK(msg_header->src_process, IBV_CHAN_RC_RDMA);

    /* check if we didn't already initiate a connection */
    entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
      sctk_net_ibv_allocator_get(msg_header->src_process, IBV_CHAN_RC_RDMA);

    if (!entry_rc_rdma)
    {
      /* allocate the structure for the RDMA entry */
      entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_allocate_init(msg_header->src_process, local_rc_rdma);
      assume(entry_rc_rdma);

      entry_rc_rdma->ready = 1;

      /* register the entry to the allocator */
      sctk_net_ibv_allocator_register(msg_header->src_process, entry_rc_rdma, IBV_CHAN_RC_RDMA);
    }
    ALLOCATOR_UNLOCK(msg_header->src_process, IBV_CHAN_RC_RDMA);
  }

  if ( request->protocol == RC_RDMA_REQ_WRITE)
  {
    sctk_nodebug("RC_RDMA_WRITE received");
    not_implemented();
    return entry_rc_rdma;
  }

  if (request->protocol == RC_RDMA_REQ_READ)
  {
    sctk_nodebug("RC_RDMA_READ received");

    sctk_net_ibv_comp_rc_rdma_process_rdma_read(
        rail, rc_sr_local, rc_rdma_local,
        entry_rc_rdma, msg_header, request);

    return entry_rc_rdma;
  }

  assume(0);
  return entry_rc_rdma;
}

/**
 *  Read a MPC msg from a recv entry
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_rdma_read_msg(
    sctk_net_ibv_rc_rdma_entry_t* entry, int type)
{
  sctk_nodebug("\t\tEntry with PSN %lu set to sent_to_mpc=1 (%p-%p)",
      entry->psn, &entry->msg_header, entry->sg_payload_ptr);

  /* send the msg to MPC */
  sctk_nodebug("READ RDVZ message %lu %p", entry->psn, entry);
  sctk_net_ibv_send_msg_to_mpc(&entry->msg_header, entry->msg_payload_ptr,
      entry->src_process, type, entry);
}

void
sctk_net_ibv_com_rc_rdma_recv_done(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_rc_rdma_done_t* done;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;


  done = (sctk_net_ibv_rc_rdma_done_t*) RC_SR_PAYLOAD(ibuf->buffer);
  entry = done->entry;
  WAIT_READY(entry);

  entry_rc_rdma = entry->entry_rc_rdma;
  sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);

  switch (entry->ptp_type)
  {
    case IBV_BCAST:
    case IBV_REDUCE:
    case IBV_BCAST_INIT_BARRIER:
      sctk_free(entry->msg_payload_aligned_ptr);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);

      sctk_ib_list_lock(&entry_rc_rdma->recv);
      sctk_ib_list_remove(&entry->list_header);
      sctk_ib_list_unlock(&entry_rc_rdma->recv);

      sctk_free(entry);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    case IBV_PTP:
      entry->msg_header_ptr->completion_flag = 1;

      /* if the message isnt directly pinned we can
       * free the memory */
      if (!entry->directly_pinned)
      {
        sctk_free(entry->msg_payload_aligned_ptr);
        sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      }

      sctk_ib_list_lock(&entry_rc_rdma->recv);
      sctk_ib_list_remove(&entry->list_header);
      sctk_ib_list_unlock(&entry_rc_rdma->recv);

      sctk_free(entry);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    default: assume(0); break;
  }
}

void
sctk_net_ibv_com_rc_rdma_read_finish(
    sctk_net_ibv_ibuf_t* ibuf,
    sctk_net_ibv_qp_local_t* rc_sr_local)
{
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  int ret;

  entry = (sctk_net_ibv_rc_rdma_entry_t*)
    ibuf->supp_ptr;
  WAIT_READY(entry);
  entry_rc_rdma = entry->entry_rc_rdma;

  sctk_nodebug("RDVZ DONE RECV for process %d (PSN: %d), requested_size %lu", entry->src_process, entry->psn, entry->requested_size);

  switch (entry->ptp_type)
  {
    case IBV_BCAST:
      sctk_nodebug("BCAST");
      sctk_net_ibv_collective_push_rc_rdma(&com_entries[entry->com_id].broadcast_fifo, entry);
      break;

    case IBV_REDUCE:
      sctk_nodebug("REDUCE");
      sctk_net_ibv_collective_push_rc_rdma(&com_entries[entry->com_id].reduce_fifo, entry);
      break;

    case IBV_BCAST_INIT_BARRIER:
      not_reachable();
      break;

    case IBV_PTP:
      sctk_nodebug("BEGIN lookup mode");

      ret = sctk_net_ibv_sched_sn_check_and_inc(
          entry->dest_task,
          entry->src_task,
          entry->psn);

      if (ret)
      {
        sctk_nodebug("Push RDVZ msg");
        sctk_net_ibv_sched_pending_push(entry,
            sizeof(sctk_net_ibv_rc_rdma_entry_t), 0,
            IBV_POLL_RC_RDMA_ORIGIN,
            entry->src_process,
            entry->src_task,
            entry->dest_task,
            entry->psn,
            &entry->msg_header,
            entry->msg_payload_ptr,
            entry);

      } else {
        sctk_net_ibv_comp_rc_rdma_read_msg(entry, IBV_RC_RDMA_ORIGIN);
        sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry->src_process, entry->psn);
      }
      break;

    default: assume(0); break;
  }
}
#endif

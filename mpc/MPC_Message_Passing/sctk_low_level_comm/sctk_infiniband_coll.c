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


#include "sctk_list.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_infiniband_coll.h"
#include "sctk_infiniband_ibufs.h"
#include "sctk_infiniband_config.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_profiler.h"

static int *sctk_comm_world_array;

/* rail */
extern  sctk_net_ibv_qp_rail_t   *rail;
/* RC SR structures */
extern  sctk_net_ibv_qp_local_t *rc_sr_local;

/*-----------------------------------------------------------
 *  COLLECTIVE FUNCTIONS
 *----------------------------------------------------------*/
  inline void
  sctk_net_ibv_collective_init()
{
  int i;

  sctk_list_new(&broadcast_fifo, 0, 0);
  sctk_list_new(&reduce_fifo, 0, 0);
  sctk_list_new(&init_barrier_fifo, 0, 0);

  sctk_comm_world_array = sctk_malloc(sctk_process_number * sizeof(unsigned int));

  for (i = 0; i < sctk_process_number; ++i)
  {
    sctk_comm_world_array[i] = i;
  }
}

   void*
  sctk_net_ibv_collective_push_rc_sr(struct sctk_list* list, sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_collective_pending_t* pending;
  sctk_net_ibv_ibuf_header_t* msg_header;

  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  pending = sctk_malloc(sizeof(sctk_net_ibv_collective_pending_t));
  assume(pending);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  pending->payload = ibuf;
  pending->size = ibuf->size;
  pending->src_process = msg_header->src_process;
  pending->type = RC_SR_EAGER;

  sctk_nodebug("Push with src %d", msg_header->src_process);

  sctk_list_lock(list);
  sctk_list_push(list, pending);
  sctk_list_unlock(list);

  return pending;
}

   void*
  sctk_net_ibv_collective_push_rc_rdma(struct sctk_list* list, sctk_net_ibv_rc_rdma_entry_t* entry)
{
  sctk_net_ibv_collective_pending_t* pending;
  sctk_net_ibv_rc_rdma_process_t *entry_rdma;
  struct sctk_list_elem* rc;

  pending = sctk_malloc(sizeof(sctk_net_ibv_collective_pending_t));
  assume(pending);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  pending->payload = entry->msg_payload_ptr;
  pending->src_process = entry->src_process;
  pending->type = RC_SR_RDVZ_READ;
  entry_rdma = entry->entry_rc_rdma;
  sctk_nodebug("Push with src %d, size %lu, ptr %p", entry->src_process, entry->requested_size, entry->msg_payload_ptr);

  /* we remove the entry... */
  sctk_list_lock(&entry_rdma->send);
  rc = sctk_list_remove(&entry_rdma->send, entry->list_elem);
  sctk_list_unlock(&entry_rdma->send);
  assume(rc);

  sctk_nodebug("pushed");

  /* ...and we push the received collective into the pending list */
  sctk_list_lock(list);
  sctk_list_push(list, pending);
  sctk_list_unlock(list);

  sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);
  sctk_free(entry);
  sctk_ibv_profiler_dec(IBV_MEM_TRACE);

  sctk_nodebug("pushed 2");

  return pending;
}


  sctk_net_ibv_collective_pending_t*
sctk_net_ibv_collective_lookup_src(struct sctk_list* list, const int src)
{
  struct sctk_list_elem* elem;
  sctk_net_ibv_collective_pending_t* msg;
  int i = 0;

  if (!list || src < 0)
    return NULL;

  sctk_list_lock(list);
  elem = list->head;
  while(elem)
  {
    msg = (sctk_net_ibv_collective_pending_t*) elem->elem;
    sctk_nodebug("THERE msg %p src process %d", msg, src);

    if (msg->src_process == src)
    {
      sctk_nodebug("Found msg %p src process %d", msg, src);
      sctk_list_remove(list, elem);
      sctk_list_unlock(list);
      return msg;
    }
    sctk_nodebug("i=%d", i);
    i++;
    elem = elem->p_next;
  }
  sctk_list_unlock(list);
  return NULL;
}

  static inline void
sctk_net_ibv_broadcast_recv(void* data, size_t size, int* array,
    const int relative_rank, int* mask, struct sctk_list* list)
{
  int src;
  sctk_net_ibv_collective_pending_t* msg;
  sctk_net_ibv_ibuf_t* ibuf;
  double s, e;


  sctk_nodebug("Broadcast recv");

  while(*mask < sctk_process_number)
  {
    if (relative_rank & *mask)
    {
      src = sctk_process_rank - *mask;
      if (src < 0)
        src += sctk_process_number;

      sctk_nodebug("Waiting broadcast from process %d", src);
      /* received from process src */
      s = MPC_Wtime();
      do {
        msg = sctk_net_ibv_collective_lookup_src(list, src);

//        sctk_net_ibv_allocator_ptp_poll_all();
        sctk_thread_yield();
      } while(msg == NULL);
      e = MPC_Wtime();
      sctk_ibv_profiler_add_double(IBV_BCAST_WAIT_SIZE, (( e - s * 1e6)));
      sctk_ibv_profiler_inc(IBV_BCAST_WAIT_NB);

//      sctk_debug("Wait : %g", (e - s) * 1e6);

      sctk_nodebug("Broadcast received from process %d size %lu", src, msg->size);

      if (msg->type == RC_SR_EAGER)
      {
        ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;
        memcpy(data,
          RC_SR_PAYLOAD(ibuf->buffer), size);
        /* restore buffers */
        sctk_net_ibv_ibuf_release(ibuf, 1);
        sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
      } else {
        memcpy(data,
          msg->payload, size);
        free(msg->payload);
      }

      sctk_free(msg);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;
    }
    *mask <<=1;
  }
}

  static inline void
sctk_net_ibv_broadcast_send(sctk_collective_communications_t * com,
    void* data, size_t size, int* array, unsigned int relative_rank, int* mask, sctk_net_ibv_ibuf_msg_type_t type)
{
  int dest;

  sctk_nodebug("Broadcast send, relative_rank %d, mask %d", relative_rank, *mask);

  *mask >>= 1;
  sctk_nodebug("mask %d", *mask);
  while(*mask > 0)
  {
    if(relative_rank + *mask < sctk_process_number)
    {
      dest = sctk_process_rank + *mask;
      if (dest >= sctk_process_number) dest -= sctk_process_number;

      sctk_nodebug("Send broadcast msg to process %d", dest);

    sctk_net_ibv_allocator_send_coll_message(
     rail, rc_sr_local, data, dest, size, type);

    }
  *mask >>= 1;
  }
}

void
sctk_net_ibv_broadcast ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem, const int root) {
  int size;
  int relative_rank;
  int mask;
  int root_process;

  size = elem_size * nb_elem;
  /* FIXME: Other communicators */
  assume(com->id == 0);

  /*
   * we have to compute where the task is located, in which process.
   * The root argument given to the function returns the rank of the MPI
   * task
   */
  root_process = sctk_get_ptp_process_localisation(root);
  sctk_nodebug("Broadcast from root %d", root_process);

  sctk_nodebug("my_vp->data.data_out %s, my_vp->data.data_in %s", my_vp->data.data_out, my_vp->data.data_in);

  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root_process) ?
    sctk_process_rank - root_process :
    sctk_process_rank - root_process + sctk_process_number;

  sctk_net_ibv_broadcast_recv(my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, &broadcast_fifo);


  if ( root_process == sctk_process_rank )
  {
    if(my_vp->data.data_out)
      memcpy ( my_vp->data.data_out, my_vp->data.data_in, size );

    sctk_net_ibv_broadcast_send(com, my_vp->data.data_in, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST);

  } else {
    sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST);
  }

  sctk_nodebug("Leave broadcast");
}

static void*
walk_list(void* elem, int cond)
{
  sctk_net_ibv_collective_pending_t* msg;
  msg = (sctk_net_ibv_collective_pending_t* ) elem;

  if (msg->src_process == cond)
  {
    sctk_nodebug("Found for process %d", cond);
    return msg;
  }

  return NULL;
}

void
sctk_net_ibv_allreduce ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void ( *func ) ( const void *, void *, size_t,
      sctk_datatype_t ),
    const sctk_datatype_t data_type ) {

  int size;
  int i;
  sctk_net_ibv_collective_pending_t* msg;
  sctk_net_ibv_ibuf_t* ibuf;
  int mask;
  int relative_rank;
  /* root process is process 0 */
  int root = 0;
  int child;
  int parent;

  sctk_nodebug("reduce operation");

  size = elem_size * nb_elem;
  /* FIXME: Other communicators */
  assume(com->id == 0);

  /* receive from children */
  for (i = 1; i <= 2; ++i)
  {
    child = 2*sctk_process_rank + i;
    if (child < sctk_process_number)
    {
      sctk_nodebug("Receive reduce from process %d", child);

      while(! (msg = sctk_list_walk_on_cond(&reduce_fifo, child, walk_list, 1)))
      {
//        sctk_net_ibv_allocator_ptp_poll_all();
        sctk_thread_yield();
      }
      ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;
      sctk_nodebug("Done msg from %d", child);

      /* an entry is ready to be read : we pop the element from list */
      sctk_nodebug("Msg received from %d", msg->src_process);

      if (msg->type == RC_SR_EAGER)
      {
        ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;
        /* function execution */
        func ( RC_SR_PAYLOAD(ibuf->buffer), my_vp->data.data_out, nb_elem, data_type );
        /* restore buffers */
        sctk_net_ibv_ibuf_release(ibuf, 1);
        sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
      } else {
        func ( msg->payload, my_vp->data.data_out, nb_elem, data_type );
        free(msg->payload);
      }

      free(msg);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
    }
  }

  /* send to parent */
  parent = (sctk_process_rank - 1) / 2;
  if (parent >= 0)
  {
    if (parent != sctk_process_rank)
    {
      sctk_nodebug("Send reduce to msg %d", parent);
      sctk_net_ibv_allocator_send_coll_message(
          rail, rc_sr_local, my_vp->data.data_out,
          parent, size, RC_SR_REDUCE);
    }
  }

  sctk_nodebug("\t\t\t\t  Finished for process");

  /* broadcast */
  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root) ?
    sctk_process_rank - root :
    sctk_process_rank - root + sctk_process_number;

  sctk_net_ibv_broadcast_recv(my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, &broadcast_fifo);

  sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST);

  sctk_nodebug ( "Broadcast received", ((int*) my_vp->data.data_out)[0] );

}

/*-----------------------------------------------------------
 *  BARRIER
 *----------------------------------------------------------*/
typedef struct
{
  int* entry;

  sctk_net_ibv_mmu_entry_t        *mmu_entry;

  struct ibv_recv_wr  wr;
  struct ibv_sge      list;

} local_entry_t;


typedef struct
{
  int* entry;
  uint32_t rkey;

  sctk_net_ibv_ibuf_t ibuf;

} remote_entry_t;

typedef struct
{
  uint32_t rkey;
  uint32_t lkey;

} barrier_local_t;


local_entry_t   local_entry;
remote_entry_t*  remote_entry;

#define N_WAY_DISSEMINATION 1

typedef struct
{
  int* entry;
  uint32_t rkey;
} request_msg_t;

/* FIXME Problem with uint8_t */
static uint64_t counter = 0;

static int is_barrier_initialized = 0;

void
sctk_net_ibv_broadcast_barrier (
    void* data, const size_t size, const int root) {

  int relative_rank;
  int mask;

  sctk_nodebug("Broadcast barrier init from root %d", root);

  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root) ?
    sctk_process_rank - root :
    sctk_process_rank - root + sctk_process_number;

  sctk_net_ibv_broadcast_recv(data, size, sctk_comm_world_array, relative_rank, &mask, &init_barrier_fifo);

  sctk_nodebug("SEND");
  sctk_net_ibv_broadcast_send(NULL, data, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST_INIT_BARRIER);

  sctk_nodebug("Leave broadcast");
}

  static void
sctk_net_ibv_barrier_post_recv(local_entry_t* local)
{

  /* allocate memory */
  sctk_posix_memalign( (void*) &local->entry,
      sctk_net_ibv_mmu_get_pagesize(), sizeof(int) * sctk_process_number);
  assume(local->entry);
  memset(local_entry.entry, 0, sizeof(int) * sctk_process_number);

  /* register memory */
  local->mmu_entry =
    sctk_net_ibv_mmu_register(rail, rc_sr_local,
        local->entry, sizeof(int) * sctk_process_number);

}

  static inline void
sctk_net_ibv_barrier_send(local_entry_t* local, remote_entry_t* remote,
    int entry_nb, int dest_process)
{
  sctk_net_ibv_qp_remote_t* remote_rc_sr;

  sctk_net_ibv_ibuf_barrier_send_init(&remote[dest_process].ibuf,
      &local->entry[entry_nb],
      local->mmu_entry->mr->lkey,
      ((int*)remote[dest_process].entry + entry_nb),
      remote[dest_process].rkey,
      sizeof(int));

  remote_rc_sr = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  sctk_nodebug("Entry id %d for process %d", remote[dest_process].wr.wr_id, dest_process);

  sctk_net_ibv_qp_send_get_wqe(remote_rc_sr, &(remote[dest_process].ibuf));

  sctk_nodebug("sent");
}

  void
sctk_net_ibv_barrier_init ()
{
  int i;
  request_msg_t request_msg;

  sctk_nodebug("Begin init barrier");

  remote_entry = sctk_malloc(sizeof(remote_entry_t) * sctk_process_number);

  sctk_net_ibv_barrier_post_recv(&local_entry);

  /* Exchange barrier addresses between peers */
  for (i=0; i < sctk_process_number; ++i)
  {
    if (i == sctk_process_rank)
    {
      request_msg.entry = local_entry.entry;
      request_msg.rkey  = local_entry.mmu_entry->mr->rkey;

      sctk_nodebug("Send entry %p (rkey: %lu)", local_entry.entry, request_msg.rkey);
      sctk_net_ibv_broadcast_barrier(&request_msg, sizeof(request_msg_t), i);
    }
    else
    {
      sctk_net_ibv_broadcast_barrier(&request_msg, sizeof(request_msg_t), i);

      remote_entry[i].entry = request_msg.entry;
      remote_entry[i].rkey = request_msg.rkey;
    }
  }

  sctk_nodebug("End init barrier");
}

  void
sctk_net_ibv_barrier ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp )
{
  int round = 0;
  uint32_t sendpeer;
  uint32_t recvpeer;
  int i;


  if (is_barrier_initialized == 0)
  {
    sctk_net_ibv_barrier_init ();
    is_barrier_initialized = 1;
  }

  if (sctk_process_rank == 0)
  {
    int i;

    for (i=0; i < sctk_process_number; ++i)
    {
      if (sctk_process_rank != i)
        sctk_nodebug("Address for process %d: %p, rkey: %lu",i, remote_entry[i].entry, remote_entry[i].rkey);
    }
  }

  counter += 1;
  if (counter == 0)
  {
    sctk_warning("COUNTER reset to 0. Bug not fixed for now");
    assume(0);
  }

  int limit = ceil(log(sctk_process_number) / log(N_WAY_DISSEMINATION+1));
  for( round = 0; round < limit ; ++round)
  {
    if (sctk_process_rank == 0)
      sctk_nodebug("Round: %d, ceil(log(sctk_process_number) / log(2): %f", round, ceil(log(sctk_process_number) / log(N_WAY_DISSEMINATION+1)));

    for (i=1; i <= N_WAY_DISSEMINATION; ++i)
    {
      uint32_t tmp = (sctk_process_rank) + i*pow((N_WAY_DISSEMINATION+1), round);
      sendpeer = fmod(tmp, sctk_process_number);

      if (sendpeer != sctk_process_rank)
      {
        local_entry.entry[sctk_process_rank] = counter;

        sctk_nodebug("%d - Send to process %u", i, sendpeer);
        sctk_net_ibv_barrier_send(&local_entry, remote_entry, sctk_process_rank, sendpeer);

      }
    }

    for (i=1; i <= N_WAY_DISSEMINATION; ++i)
    {
      uint32_t tmp = (sctk_process_number) + ((sctk_process_rank) - i*pow((N_WAY_DISSEMINATION+1), round));
      recvpeer = fmod(tmp, sctk_process_number);

      sctk_nodebug("%d - Recv from process %u", i, recvpeer);

      if (recvpeer != sctk_process_rank)
      {
        while(local_entry.entry[recvpeer] < counter)
        {
//           sctk_net_ibv_allocator_ptp_poll_all();
           sctk_thread_yield();
        }
        sctk_nodebug("Got counter : %d",local_entry.entry[recvpeer]);
      }
    }
  }
}

/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include <sctk_debug.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_low_level_comm.h>
#include <sctk_multirail_ib.h>
#include <sctk_route.h>
#include <sctk_tcp.h>
#include <opa_primitives.h>
#include <sctk_checksum.h>
#include "sctk_ib_fallback.h"
#include "sctk_ib_mpi.h"
#include "sctk_ib_config.h"
#include "sctk_ib_toolkit.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_cp.h"

#define NB_RAILS 2
static sctk_rail_info_t** rails = NULL;
static char is_ib_used = 0;

struct sctk_rail_info_s** sctk_network_get_rails() {
  return rails;
}

#define IB_RAIL_DATA 0          /* Data rail */
#define IB_RAIL_SIGNALIZATION 1 /* Signalization rail */

TODO("The following value MUST be determined dynamically!!")
#define IB_MEM_THRESHOLD_ALIGNED_SIZE (256*1024) /* RDMA threshold */
#define IB_MEM_ALIGNMENT        (4096) /* Page size */

/* Return which rail is used for MPI communications */
int sctk_network_ib_get_rail_data() {
  assume(rails);
  int rail_nb = IB_RAIL_DATA;
  assume (rail_nb < NB_RAILS);
  return rail_nb;
}

/* Return which rail is used for signalization */
int sctk_network_ib_get_rail_signalization() {
  assume(rails);
  int rail_nb = IB_RAIL_SIGNALIZATION;
  assume (rail_nb < NB_RAILS);
  return rail_nb;
}

static void
sctk_network_send_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i ;
  /* XXX:Calculating checksum */
#ifdef SCTK_USE_CHECKSUM
  sctk_checksum_register(msg);
#endif
  sctk_prepare_send_message_to_network_reorder(msg);

  const specific_message_tag_t tag = msg->body.header.specific_message_tag;
  if (IS_PROCESS_SPECIFIC_CONTROL_MESSAGE(tag)) {
    i = sctk_network_ib_get_rail_signalization();
  } else {
    i = sctk_network_ib_get_rail_data();
  }
  /* Always send using the MPI network */
  sctk_nodebug("Send message using rail %d", i);
  rails[i]->send_message(msg,rails[i]);
}

static void
sctk_network_notify_recv_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_recv_message(msg,rails[i]);
  }
}

static void
sctk_network_notify_matching_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_matching_message(msg,rails[i]);
  }
}

static void
sctk_network_notify_perform_message_multirail_ib (int remote_process, int remote_task_id){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_perform_message(remote_process, remote_task_id,rails[i]);
  }
}

static void
sctk_network_notify_idle_message_multirail_ib (){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_idle_message(rails[i]);
  }
}

static void
sctk_network_notify_any_source_message_multirail_ib (){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_any_source_message(rails[i]);
  }
}

/* Returns the status of the message polled */
static
int sctk_send_message_from_network_multirail_ib (sctk_thread_ptp_message_t * msg){
  int ret = sctk_send_message_from_network_reorder(msg);
  if(ret == REORDER_NO_NUMBERING){
    /*
      No reordering
    */
    sctk_send_message_try_check(msg,1);
  }

  return ret;
}

/************ INIT ****************/
/* XXX: polling thread used for 'fully-connected' topology initialization. Because
 * of PMI lib, when a barrier occurs, the IB polling cannot be executed.
 * This thread is disbaled when the route is initialized */
static void* __polling_thread(void *arg) {
  sctk_rail_info_t* rail = (sctk_rail_info_t*) arg;
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;
  sctk_ib_polling_t poll;
  POLL_INIT(&poll);

  /* XXX hack to disable CP when fully is used */
  config->ibv_steal = -1;
  while(1) {
    if (sctk_route_is_finalized())
    {
      config->ibv_steal = steal;
      break;
    }
    sctk_network_poll_all_and_steal(rail, &poll);
  }
  return NULL;
}

static void sctk_network_init_polling_thread (sctk_rail_info_t* rail) {
  assume(0);
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, __polling_thread, (void*) rail);
}

/* Choose the topology of the signalization network (ring, torus) */
static char *__get_signalization_topology(char* topo, size_t size) {
    char *value;

    if ( (value = getenv("MPC_IBV_SIGN_TOPO")) != NULL )
        snprintf(topo, size, "%s", value);
    else  /* Torus is the default */
        snprintf(topo, size, "%s", "torus");

    return topo;
}

/************ INIT ****************/
static
void sctk_network_init_multirail_ib_all(char* name, char* topology){
  int i;

  /* FIXME: for the moment, IB requires an ondemand connexion.
   * Exiting else... */
  if (strcmp(topology, "ondemand") && strcmp(topology, "fully")) {
      sctk_error("IB requires the 'ondemand' or the 'fully' topology! Exiting...");
      sctk_abort();
  }

  sctk_route_set_rail_nb(NB_RAILS);
  sctk_ib_prof_init(NB_RAILS);
  rails = sctk_malloc(NB_RAILS*sizeof(sctk_rail_info_t*));
  memset(rails, 0, NB_RAILS*sizeof(sctk_rail_info_t*));

  is_ib_used = 1;
  /* MPI IB network */
  i = 0;
  rails[i] = sctk_route_get_rail(i);
  rails[i]->rail_number = i;
  rails[i]->send_message_from_network = sctk_send_message_from_network_multirail_ib;
  sctk_route_init_in_rail(rails[i],topology);
  sctk_network_init_mpi_ib(rails[i]);
  /* If fully mode, we activate the polling thread */
  /* FIXME: no more needed as the signalization network now
   * uses async events
  if ( strcmp(topology, "fully") == 0) {
    sctk_network_init_polling_thread (rails[i]);
  } */

  /* Fallback IB network */
  char signalization_topology[256];
  __get_signalization_topology(signalization_topology, 256);
  i = 1;
  rails[i] = sctk_route_get_rail(i);
  rails[i]->rail_number = i;
  rails[i]->send_message_from_network = sctk_send_message_from_network_multirail_ib;
  sctk_route_init_in_rail(rails[i],signalization_topology);
  sctk_network_init_fallback_ib(rails[i]);
  /* FIXME: no more needed as the signalization network now
   * uses async events
  if ( strcmp(signalization_topology, "torus") == 0) {
    sctk_network_init_polling_thread (rails[i]);
  } */

  /* Set the rail as a signalization rail */
  sctk_route_set_signalization_rail(rails[i]);

  sctk_network_send_message_set(sctk_network_send_message_multirail_ib);
  sctk_network_notify_recv_message_set(sctk_network_notify_recv_message_multirail_ib);
  sctk_network_notify_matching_message_set(sctk_network_notify_matching_message_multirail_ib);
  sctk_network_notify_perform_message_set(sctk_network_notify_perform_message_multirail_ib);
  sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_multirail_ib);
  sctk_network_notify_any_source_message_set(sctk_network_notify_any_source_message_multirail_ib);

}
void sctk_network_init_multirail_ib(char* name, char* topology){
  sctk_network_init_multirail_ib_all(name,topology);
}

void sctk_network_init_ib(char* name, char* topology){
  sctk_network_init_multirail_ib_all("mutirail_ib",topology);
}

char sctk_network_is_ib_used() {
  return is_ib_used;
}

/************ INITIALIZE TASK ****************/
void sctk_network_initialize_task_multirail_ib (int rank, int vp){
  if(sctk_process_number > 1 && sctk_network_is_ib_used() ){
    /* Register task for QP prof */
    sctk_ib_prof_init_task(rank, vp);
    /* Register task for buffers */
    sctk_ibuf_init_task(rank, vp);
    /* Register task for collaborative polling */
    sctk_ib_cp_init_task(rank, vp);
  }
}

/************ FINALIZE PROCESS ****************/
void sctk_network_finalize_multirail_ib (){
/* Do not report timers */
  int i;
  if (rails) {
    for(i = 0; i < NB_RAILS; i++){
      sctk_ib_prof_finalize(&rails[i]->network.ib);
    }
  }
}

/************ FINALIZE TASK ****************/
void sctk_network_finalize_task_multirail_ib (int rank){
  if(sctk_process_number > 1 && sctk_network_is_ib_used() ){
    sctk_ib_cp_finalize_task(rank);
    sctk_ib_prof_qp_finalize_task(rank);
  }
}

/************ MEMORY ALLOCATOR HOOK  ****************/
sctk_size_t sctk_network_memory_allocator_hook_ib (sctk_size_t size){
    if (size > IB_MEM_THRESHOLD_ALIGNED_SIZE ) {
    return ( (size + (IB_MEM_ALIGNMENT-1) ) & ( ~ (IB_MEM_ALIGNMENT-1) ) );
  }
}



#endif

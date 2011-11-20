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
/* #                                                                      # */
/* ######################################################################## */


#include <sctk_debug.h>
#include <sctk_ib_toolkit.h>
#include <sctk_route.h>
#include <sctk_net_tools.h>

#include <sctk_ib_fallback.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include "sctk_ib_qp.h"

static void
sctk_network_send_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t* tmp;
  sctk_ibuf_t *ibuf;

  sctk_debug("send message through rail %d",rail->rail_number);

  if(msg->body.header.specific_message_tag == process_specific_message_tag){
    tmp = sctk_get_route_to_process(msg->sctk_msg_get_destination,rail);
    sctk_debug("1 Route to %d", msg->sctk_msg_get_destination);
  } else {
    tmp = sctk_get_route(msg->sctk_msg_get_glob_destination,rail);
    sctk_debug("2 Route to %d", msg->sctk_msg_get_glob_destination);
  }

  ibuf = sctk_ibuf_pick(rail_ib, 1, 0);
  sctk_debug("Picked buffer %p", ibuf);
}

static void
sctk_network_notify_recv_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_debug("Recv_message");
}

static void
sctk_network_notify_matching_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
}

static void
sctk_network_notify_perform_message_ib (int remote,sctk_rail_info_t* rail){
}

static void
sctk_network_notify_idle_message_ib (sctk_rail_info_t* rail){
}

static void
sctk_network_notify_any_source_message_ib (sctk_rail_info_t* rail){
}

/************ INIT ****************/
void sctk_network_init_ib(sctk_rail_info_t* rail){
  rail->send_message = sctk_network_send_message_ib;
  rail->notify_recv_message = sctk_network_notify_recv_message_ib;
  rail->notify_matching_message = sctk_network_notify_matching_message_ib;
  rail->notify_perform_message = sctk_network_notify_perform_message_ib;
  rail->notify_idle_message = sctk_network_notify_idle_message_ib;
  rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
  rail->network_name = "IB";

  /* Infiniband Init */
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_device_t *device;
  struct ibv_srq_init_attr srq_attr;
  /* Open config */
  sctk_ib_config_init(rail_ib);
  /* Open device */
  device = sctk_ib_device_open(rail_ib, 0);
  /* Init Proctection Domain */
  sctk_ib_pd_init(device);
  /* Init Completion Queues */
  device->send_cq = sctk_ib_cq_init(device, rail_ib->config);
  device->recv_cq = sctk_ib_cq_init(device, rail_ib->config);
  /* Init SRQ */
  srq_attr = sctk_ib_srq_init_attr(rail_ib);
  sctk_ib_srq_init(rail_ib, &srq_attr);
  /* Print config */
  sctk_ib_config_print(rail_ib);
  sctk_ib_mmu_init(rail_ib);
  sctk_ibuf_pool_init(rail_ib);
  /* Fill SRQ with buffers */
  sctk_ibuf_srq_check_and_post(rail_ib, rail_ib->config->ibv_max_srq_ibufs);

  /* Initialize network */
  sctk_network_init_ib_all(rail, rail->route, rail->route_init);
}

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
#include <sctk_net_tools.h>
#include <sctk_ib.h>
#include <sctk_pmi.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sctk.h>
#include <netdb.h>
#include <sctk_spinlock.h>
#include <sctk_net_tools.h>
#include <sctk_ib.h>
#include <sctk_ib_qp.h>
#include <sctk_ib_toolkit.h>

#define MAX_STRING_SIZE  2048
static void sctk_ib_add_static_route(int dest, sctk_route_table_t *tmp,
    sctk_rail_info_t* rail){
  sctk_add_static_route(dest,tmp,rail);
}

static sctk_route_table_t *
sctk_ib_create_remote(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_data_t *route_ib;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);

  sctk_debug("Creating QP for dest %d", dest);
  tmp = sctk_malloc(sizeof(sctk_route_table_t));
  memset(tmp,0,sizeof(sctk_route_table_t));

  route_ib=&tmp->data.ib;
  route_ib->remote = sctk_ib_qp_new();
  sctk_ib_qp_allocate_init(rail_ib, dest, route_ib->remote);

  sctk_ib_add_static_route(dest, tmp, rail);

  return tmp;
}

void sctk_network_init_ib_all(sctk_rail_info_t* rail,
			       int (*route)(int , sctk_rail_info_t* ),
			       void(*route_init)(sctk_rail_info_t*)){

  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int dest_rank;
  int src_rank;
  char src_connection_infos[MAX_STRING_SIZE];
  char dest_connection_infos[MAX_STRING_SIZE];
  sctk_route_table_t* route_table;
  sctk_ib_data_t *route_data;
  sctk_ib_qp_keys_t keys;

  assume(rail->send_message_from_network != NULL);

  /* FIXME: register pointers */

  dest_rank = (sctk_process_rank + 1) % sctk_process_number;
  src_rank = (sctk_process_rank + sctk_process_number - 1) % sctk_process_number;

  route_table = sctk_ib_create_remote(dest_rank, rail);
  route_data=&route_table->data.ib;

  sctk_ib_qp_keys_send(rail_ib, route_data->remote);
  sctk_pmi_barrier();
  keys = sctk_ib_qp_keys_recv(route_data->remote, src_rank);

  sctk_debug("Recv from %d, send to %d", src_rank, dest_rank);
}


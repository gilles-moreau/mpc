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
#ifndef __SCTK__IB_CM_H_
#define __SCTK__IB_CM_H_

#include "sctk_route.h"
/*-----------------------------------------------------------
 *  MACROS
 *----------------------------------------------------------*/
#define ONDEMAND_MASK_TAG (1<<3)
#define ONDEMAN_REQ_TAG (1)
#define ONDEMAN_ACK_TAG (2)
#define ONDEMAN_DONE_TAG (3)

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/* Fully-connected */
void sctk_ib_cm_connect_to(int from, int to, struct sctk_rail_info_s* rail);
void sctk_ib_cm_connect_from(int from, int to,sctk_rail_info_t* rail);

/* On-demand connexions */
int sctk_ib_cm_on_demand_recv_check(sctk_thread_ptp_message_t *msg, void* request);
int sctk_ib_cm_on_demand_recv(struct sctk_rail_info_s *rail,
    sctk_thread_ptp_message_t *msg, struct sctk_ibuf_s* ibuf, int recopy);
sctk_route_table_t *sctk_route_dynamic_safe_add(int dest, sctk_rail_info_t* rail, sctk_route_table_t* (*func)(int dest, sctk_rail_info_t* rail), int *added);
#endif
#endif

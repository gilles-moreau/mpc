#ifndef SCTK_PTL_TOOLKIT_H_
#define SCTK_PTL_TOOLKIT_H_

#include "sctk_route.h"
void sctk_ptl_create_ring(sctk_rail_info_t *rail);
sctk_ptl_id_t sctk_ptl_map_id(sctk_rail_info_t* rail, int dest);
void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state);
void sctk_ptl_init_interface(sctk_rail_info_t* rail);
void sctk_ptl_fini_interface(sctk_rail_info_t* rail);

void sctk_ptl_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint);
void sctk_ptl_recv_message(sctk_thread_ptp_message_t* msg, sctk_rail_info_t* rail);
#endif

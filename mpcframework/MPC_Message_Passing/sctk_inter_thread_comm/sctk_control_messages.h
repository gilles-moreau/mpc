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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_CONTROL_MESSAGE_H
#define SCTK_CONTROL_MESSAGE_H

#include <sctk_inter_thread_comm.h>


/************************************************************************/
/* Control Messages Types                                               */
/************************************************************************/

/** This is the context of the control message engine */
struct sctk_control_message_context
{
	void (*sctk_user_control_message)( int source_process, int source_rank, char subtype, char param, void * data, size_t size ); /**< This function is called when the application has registered a function */
};

void sctk_control_message_context_set_user( void (*fn)( int , int , char , char , void * , size_t ) );

/************************************************************************/
/* Process Level Control messages                                       */
/************************************************************************/

typedef enum {
  SCTK_PROCESS_FENCE,
  SCTK_PROCESS_RDMA_WIN_MAPTO,
  SCTK_PROCESS_RDMA_WIN_RELAX,
  SCTK_PROCESS_RDMA_EMULATED_WRITE,
  SCTK_PROCESS_RDMA_EMULATED_READ,
  SCTK_PROCESS_RDMA_EMULATED_FETCH_AND_OP,
  SCTK_PROCESS_RDMA_EMULATED_CAS,
  SCTK_PROCESS_RDMA_CONTROL_MESSAGE
} sctk_control_msg_process_t;

/************************************************************************/
/* Control Messages Interface                                           */
/************************************************************************/

void sctk_control_messages_send_process(int dest_process, int subtype,
                                        char param, void *buffer, size_t size);
void sctk_control_messages_send_to_task(int dest_task, sctk_communicator_t comm,
                                        int subtype, char param, void *buffer,
                                        size_t size);
void sctk_control_messages_send_rail( int dest, int subtype, char param, void *buffer, size_t size, int  rail_id );
void control_message_submit(sctk_message_class_t class, int rail_id,
                            int source_process, int source_rank, int subtype,
                            int param, void *data, size_t msg_size);
void sctk_control_messages_incoming( sctk_thread_ptp_message_t * msg );
void sctk_control_messages_perform(sctk_thread_ptp_message_t *msg, int force);

struct sctk_control_message_fence_ctx
{
	int source;
	int remote;
        int comm;
};

void sctk_control_message_fence(int target_task, sctk_communicator_t comm);
void sctk_control_message_fence_req(int target_task, sctk_communicator_t comm,
                                    sctk_request_t *req);
void sctk_control_message_fence_handler( struct sctk_control_message_fence_ctx *ctx );

/************************************************************************/
/* Control Messages List                                                */
/************************************************************************/

void sctk_control_message_init();
void sctk_control_message_push( sctk_thread_ptp_message_t * msg );
void sctk_control_message_process();
void sctk_control_message_process_all();
int sctk_control_message_process_local(int rank);

#endif /* SCTK_CONTROL_MESSAGE_H */
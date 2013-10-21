/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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

#include "sctk_ib_config.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_NAME "CONFIG"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_runtime_config.h"

/*-----------------------------------------------------------
 *  CONSTS
 *----------------------------------------------------------*/
/* For RDMA connection */
/* FOR PAPER */
//#define IBV_RDMA_MIN_SIZE (2 * 1024)
//#define IBV_RDMA_MAX_SIZE (32 * 1024)
//#define IBV_RDMA_MIN_NB (16)
//#define IBV_RDMA_MAX_NB (1024)

#define IBV_RDMA_MIN_SIZE (3*1024)
#define IBV_RDMA_MAX_SIZE (3*1024)
#define IBV_RDMA_MIN_NB (2048)
#define IBV_RDMA_MAX_NB (2048)
//#define IBV_RDMA_MIN_SIZE (1*1024)
//#define IBV_RDMA_MAX_SIZE (128*1024)
//#define IBV_RDMA_MIN_NB (2)
//#define IBV_RDMA_MAX_NB (16384)
/*  BEST RDMA on CURIE */
//#define IBV_RDMA_MIN_SIZE (32 * 1024)
//#define IBV_RDMA_MAX_SIZE (32 * 1024)
//#define IBV_RDMA_MIN_NB (128)
//#define IBV_RDMA_MAX_NB (128)
/* For RDMA resizing */
/* FOR PAPER */
//#define IBV_RDMA_RESIZING_MIN_SIZE (2 * 1024)
//#define IBV_RDMA_RESIZING_MAX_SIZE (12 * 1024)
//#define IBV_RDMA_RESIZING_MIN_NB (16)
//#define IBV_RDMA_RESIZING_MAX_NB (1024)
#define IBV_RDMA_RESIZING_MIN_SIZE (1*1024)
#define IBV_RDMA_RESIZING_MAX_SIZE (128 * 1024)
#define IBV_RDMA_RESIZING_MIN_NB (2)
#define IBV_RDMA_RESIZING_MAX_NB (16384)

char* steal_names[2] = {
  "Normal mode",
  "Collaborative-polling mode"};

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_config_check(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);

  /* TODO: MMU cache is buggy and do not intercept free calls */
  if ( (sctk_process_rank == 0)
      && (config->mmu_cache_enabled == 1) ) {
    sctk_error("MMU cache enabled: use it at your own risk!");
  }

  if ( (sctk_process_rank == 0)
      && (config->low_memory) ) {
    sctk_error("LOW mem module enabled: use it at your own risk!");
  }

  /* Good conf, we return */
  return;
}

#define EXPERIMENTAL(str) #str" (experimental)"
void sctk_ib_config_print(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  if (sctk_process_rank == 0) {
    fprintf(stderr,
        "############# IB configuration for %s\n"
        "eager_limit      = %d\n"
        "buffered_limit = %d\n"
        "max_rdma_connections = %d\n"
        "rdma_min_size = %d\n"
        "rdma_max_size = %d\n"
        "rdma_min_nb = %d\n"
        "rdma_max_nb = %d\n"
        "rdma_resizing_min_size, = %d\n"
        "rdma_resizing_max_size = %d\n"
        "rdma_resizing_min_nb = %d\n"
        "rdma_resizing_max_nb = %d\n"
        "rdma_resizing    = %d\n"
        "qp_tx_depth      = %d\n"
        "qp_rx_depth      = %d\n"
        "max_sg_sq        = %d\n"
        "max_sg_rq        = %d\n"
        "max_inline       = %d\n"
        "init_ibufs        = %d\n"
        "init_recv_ibufs        = %d\n"
        "max_srq_ibufs_posted    = %d\n"
        "max_srq_ibufs    = %d\n"
        "size_ibufs_chunk = %d\n"
        "size_recv_ibufs_chunk = %d\n"
        "srq_credit_limit = %d\n"
        "srq_credit_thread_limit = %d\n"
        "rdvz_protocol    = %d\n"
        "wc_in_number     = %d\n"
        "wc_out_number    = %d\n"
        "init_mr          = %d\n"
        "mmu_cache_enabled = %d\n"
        "mmu_cache_entries = %d\n"
        "adm_port         = %d\n"
        "rdma_depth       = %d\n"
        "rdma_dest_depth  = %d\n"
        "quiet_crash      = %d\n"
        "async_thread     = %d\n"
        EXPERIMENTAL(steal)"            = %d\n"
        "Stealing desc        = %s\n"
        EXPERIMENTAL(low_memory)"            = %d\n"
        "#############\n",
        rail_ib->rail->network_name,
        config->eager_limit,
        config->buffered_limit,
        config->max_rdma_connections,
        config->rdma_min_size,
        config->rdma_max_size,
        config->rdma_min_nb,
        config->rdma_max_nb,
        config->rdma_resizing_min_size,
        config->rdma_resizing_max_size,
        config->rdma_resizing_min_nb,
        config->rdma_resizing_max_nb,

        config->rdma_resizing,
        config->qp_tx_depth,
        config->qp_rx_depth,
        config->max_sg_sq,
        config->max_sg_rq,
        config->max_inline,
        config->init_ibufs,
        config->init_recv_ibufs,
        config->max_srq_ibufs_posted,
        config->max_srq_ibufs,
        config->size_ibufs_chunk,
        config->size_recv_ibufs_chunk,
        config->srq_credit_limit,
        config->srq_credit_thread_limit,
        config->rdvz_protocol,
        config->wc_in_number,
        config->wc_out_number,
        config->init_mr,
        config->mmu_cache_enabled,
        config->mmu_cache_entries,
        config->adm_port,
        config->rdma_depth,
        config->rdma_dest_depth,
        config->quiet_crash,
        config->async_thread,
        config->steal, steal_names[config->steal],
        config->low_memory);
  }
}

void sctk_ib_config_mutate(sctk_ib_rail_info_t *rail_ib) {
  LOAD_CONFIG(rail_ib);

  config->eager_limit       = ALIGN_ON (config->eager_limit + IBUF_GET_EAGER_SIZE, 64);
  config->buffered_limit  = (config->buffered_limit + sizeof(sctk_thread_ptp_message_body_t));
}

//#define SET_RUNTIME_CONFIG(name) config->ibv_##name = runtime_config->name

/*
static void load_ib_load_config(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  struct sctk_runtime_config_struct_net_driver_infiniband * runtime_config =
    &rail_ib->rail->runtime_config_driver_config->driver.value.infiniband;

  SET_RUNTIME_CONFIG(size_mr_chunk);
  SET_RUNTIME_CONFIG(init_ibufs);
  SET_RUNTIME_CONFIG(max_rdma_connections);
  SET_RUNTIME_CONFIG(rdma_resizing);
  SET_RUNTIME_CONFIG(qp_tx_depth);
  SET_RUNTIME_CONFIG(qp_rx_depth);
  SET_RUNTIME_CONFIG(cq_depth);
  SET_RUNTIME_CONFIG(max_sg_sq);
  SET_RUNTIME_CONFIG(max_sg_rq);
  SET_RUNTIME_CONFIG(max_inline);
  SET_RUNTIME_CONFIG(max_srq_ibufs_posted);
  SET_RUNTIME_CONFIG(max_srq_ibufs);
  SET_RUNTIME_CONFIG(srq_credit_limit);
  SET_RUNTIME_CONFIG(srq_credit_thread_limit);
  SET_RUNTIME_CONFIG(verbose_level);
  SET_RUNTIME_CONFIG(wc_in_number);
  SET_RUNTIME_CONFIG(wc_out_number);
  SET_RUNTIME_CONFIG(init_mr);
  SET_RUNTIME_CONFIG(size_ibufs_chunk);
  SET_RUNTIME_CONFIG(mmu_cache_enabled);
  SET_RUNTIME_CONFIG(mmu_cache_entries);
  SET_RUNTIME_CONFIG(adm_port);
  SET_RUNTIME_CONFIG(rdma_depth);
  SET_RUNTIME_CONFIG(rdma_dest_depth);
  SET_RUNTIME_CONFIG(steal);
  SET_RUNTIME_CONFIG(quiet_crash);
  SET_RUNTIME_CONFIG(async_thread);

  config->ibv_rdma_resizing_min_size = IBV_RDMA_RESIZING_MIN_SIZE;
  config->ibv_rdma_resizing_max_size = IBV_RDMA_RESIZING_MAX_SIZE;
  config->ibv_rdma_resizing_min_nb = IBV_RDMA_RESIZING_MIN_NB;
  config->ibv_rdma_resizing_max_nb = IBV_RDMA_RESIZING_MAX_NB;

  TODO("Move to configuration file");
  config->ibv_init_recv_ibufs = 10000;
  config->ibv_size_recv_ibufs_chunk = 1000;


}
*/

#if 0
/* Set IB configure with env variables */
void set_ib_env(sctk_ib_rail_info_t *rail_ib)
{
  /* Format: "x:x:x:x-x:x:x:x". Buffer sizes are in KB */
  if ( (value = getenv("MPC_IBV_RDMA_EAGER")) != NULL) {
    sscanf(value,"%d-(%d:%d:%d:%d)-%d-(%d:%d:%d:%d)",
        &c->ibv_max_rdma_connections,
        &c->ibv_rdma_min_size,
        &c->ibv_rdma_max_size,
        &c->ibv_rdma_min_nb,
        &c->ibv_rdma_max_nb,
        &c->ibv_rdma_resizing,
        &c->ibv_rdma_resizing_min_size,
        &c->ibv_rdma_resizing_max_size,
        &c->ibv_rdma_resizing_min_nb,
        &c->ibv_rdma_resizing_max_nb);
    c->ibv_rdma_resizing_min_size *=1024;
    c->ibv_rdma_resizing_max_size *=1024;
  }sctk_ib_config_mutate(sctk_ib_rail_info_t *rail_ib) {
  LOAD_CONFIG(rail_ib);
}

    c->ibv_rdma_min_size          *=1024;
    c->ibv_rdma_max_size          *=1024;
    c->ibv_rdma_resizing_min_size *=1024;
    c->ibv_rdma_resizing_max_size *=1024;
  }
}
#endif

void sctk_ib_config_init(sctk_ib_rail_info_t *rail_ib, char* network_name)
{
  LOAD_CONFIG(rail_ib);

  config = sctk_malloc(sizeof(sctk_ib_config_t));
  assume(config);
  memset(config, 0, sizeof(sctk_ib_config_t));
  rail_ib->config = &rail_ib->rail->runtime_config_driver_config->driver.value.infiniband;
  sctk_ib_config_mutate(rail_ib);
  //rail_ib->config->network_name = strdup(network_name);

  //load_ib_load_config(rail_ib);

  //Check if the variables are well set
  sctk_ib_config_check(rail_ib);
}

#endif

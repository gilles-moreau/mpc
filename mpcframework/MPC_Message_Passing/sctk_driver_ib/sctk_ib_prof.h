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

#ifndef __SCTK__IB_PROF_H_
#define __SCTK__IB_PROF_H_

#ifdef MPC_USE_INFINIBAND

#include <mpc_common_spinlock.h>
#include <sctk_debug.h>
#include <sctk_config.h>
#include "opa_primitives.h"
#include "sctk_stdint.h"
#include <mpc_common_asm.h>

/* Rail to profile */
#define PROF_RAIL_NUMBER 1

/* Uncomment to enable Counters */
#ifdef MPC_Profiler
#define SCTK_IB_PROF
#endif
/* Uncomment to enable QP profiling */
//#define SCTK_IB_QP_PROF
/* Uncomment to enable MEM profiling */
//#define SCTK_IB_MEM_PROF

#ifdef SCTK_IB_PROF
#include <mpc_profiler.h>
#define PROF_DECL(type, name) type name

extern __thread struct sctk_ib_prof_s *sctk_ib_profiler;
extern __thread struct sctk_ib_prof_s *sctk_ib_profiler_start;

typedef enum sctk_ib_prof_glob_counters_e
{
    SCTK_IB_IBUS_RDMA_SIZE = 0,
    SCTK_IB_RDMA_CONNECTION,
    SCTK_IB_RDMA_RESIZING,
    SCTK_IB_RDMA_DECONNECTION,
    IB_PROF_GLOB_COUNTERS_MAX
} sctk_ib_prof_glob_counters_t;

extern OPA_int_t sctk_ib_prof_glob_counters[IB_PROF_GLOB_COUNTERS_MAX];

static const char *sctk_ib_prof_glob_counters_name[IB_PROF_GLOB_COUNTERS_MAX]  =
{
	"SCTK_IB_IBUS_RDMA_SIZE",
	"SCTK_IB_RDMA_CONNECTION",
	"SCTK_IB_RDMA_RESIZING",
	"SCTK_IB_RDMA_DECONNECTION",
};


#define PROF_INC_GLOB(r,x) \
    OPA_incr_int(&sctk_ib_prof_glob_counters[x]);

#define PROF_ADD_GLOB(r,x,y) \
    OPA_add_int(&sctk_ib_prof_glob_counters[x], y);

#define PROF_DECR_GLOB(r,x,y)  \
    OPA_add_int(&sctk_ib_prof_glob_counters[x], -(y));

// End global counters

#define PROF_TIME_START(r, x) \
  SCTK_PROFIL_START_DECLARE(x);  \
  /* if (r->rail_number == PROF_RAIL_NUMBER ) */ {  \
     SCTK_PROFIL_START_INIT(x);  \
  }

#define PROF_TIME_END(r, x)     \
  /* if (r->rail_number == PROF_RAIL_NUMBER ) */ {     \
    SCTK_PROFIL_END(x);   \
  }

#define PROF_INC(r,x)  \
  /* if (r->rail_number == PROF_RAIL_NUMBER ) */ {      \
    SCTK_COUNTER_INC(x, 1);      \
  }

#define PROF_ADD(r,x,y)     \
  /* if (r->rail_number == PROF_RAIL_NUMBER ) */ {    \
    SCTK_COUNTER_INC(x, y);   \
  }

#define PROF_DECR(r,x,y)    \
  /* if (r->rail_number == PROF_RAIL_NUMBER) */ {   \
    SCTK_COUNTER_DEC(x, y);    \
  }


void sctk_ib_prof_init_reference_clock();
double sctk_ib_prof_get_time_stamp();

#else

#define PROF_TIME_START(x,y) (void)(0)
#define PROF_TIME_END(x,y) (void)(0)
#define PROF_INC_RAIL_IB(x,y) (void)(0)
#define PROF_INC(x,y) (void)(0)
#define PROF_INC_RAIL_IB(x,y) (void)(0)
#define PROF_DECR(r,x,y) (void)(0)
#define PROF_ADD(r,x,y) (void)(0)
#define sctk_ib_prof_get_time_stamp() 0
#define sctk_ib_prof_init_reference_clock() (void)(0)

/* Global values */
#define PROF_INC_GLOB(x,y) (void)(0)
#define PROF_DECR_GLOB(r,x,y) (void)(0)
#define PROF_ADD_GLOB(r,x,y) (void)(0)
#endif

void sctk_ib_prof_init_task ( int rank, int vp );
void sctk_ib_prof_init();
void sctk_ib_prof_finalize ( sctk_ib_rail_info_t *rail_ib );

/*
 * QP profiling
 */
#define PROF_QP_SEND 0
#define PROF_QP_RECV 1
#define PROF_QP_SYNC 2
#define PROF_QP_CREAT 3
#define PROF_QP_RDMA_CONNECTED 4
#define PROF_QP_RDMA_RESIZING 5
#define PROF_QP_RDMA_DISCONNECTED 6

#ifdef SCTK_IB_QP_PROF
/* QP profiling */
void sctk_ib_prof_qp_init();
void sctk_ib_prof_qp_init_task ( int task_id, int vp );
void sctk_ib_prof_qp_flush();
void sctk_ib_prof_qp_finalize_task ( int task_id );
void sctk_ib_prof_qp_write ( int proc, size_t size, double ts, char from );
#else
/* QP profiling */
#define sctk_ib_prof_qp_init(x) (void)(0)
#define sctk_ib_prof_qp_init_task(x,y) (void)(0)
#define sctk_ib_prof_qp_flush() (void)(0)
#define sctk_ib_prof_qp_finalize_task(x) (void)(0)
#define sctk_ib_prof_qp_write(a,b,c,d) (void)(0)
#endif


/*
 * Memory profiling
 */
/* TODO: no more working. Should be fixed */
#ifdef SCTK_IB_MEM_PROF
double sctk_ib_prof_get_mem_used();
void sctk_ib_prof_mem_init ( sctk_ib_rail_info_t *rail_ib );
void sctk_ib_prof_mem_flush();
void sctk_ib_prof_mem_write ( double ts, double mem );
#else
#define sctk_ib_prof_mem_init(x) (void)(0)
#define sctk_ib_prof_mem_flush(x) (void)(0)
#define sctk_ib_prof_mem_write(x,y) (void)(0)
#define sctk_ib_prof_get_mem_used() 0
#if 0
static double sctk_ib_prof_get_mem_used()
{
	return 0;
}
#endif
#endif  /* SCTK_IB_PROF */

#endif  /* MPC_USE_INFINIBAND */
#endif  /* __SCTK__IB_PROF_H_ */

/* In Bytes */
#define IBV_MEM_USED_LIMIT (33.0 * 1000.0 * 1000.0 * 1024.0)

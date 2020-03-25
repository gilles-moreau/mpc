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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_taskq.h"

kmpc_thunk_t *__kmpc_taskq(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                          __UNUSED__  kmpc_task_t taskq_task, __UNUSED__ size_t sizeof_thunk,
                          __UNUSED__  size_t sizeof_shareds, __UNUSED__ kmp_int32 flags,
                           __UNUSED__ kmpc_shared_vars_t **shareds) {
  not_implemented();
  return (kmpc_thunk_t *)NULL;
}

void __kmpc_end_taskq(__UNUSED__ ident_t *loc,__UNUSED__  kmp_int32 global_tid, __UNUSED__ kmpc_thunk_t *thunk) {
  not_implemented();
}

kmp_int32 __kmpc_task(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ kmpc_thunk_t *thunk) {
  not_implemented();
  return (kmp_int32)0;
}

void __kmpc_taskq_task(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ kmpc_thunk_t *thunk,
                       __UNUSED__ kmp_int32 status) {
  not_implemented();
}

void __kmpc_end_taskq_task(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                           __UNUSED__ kmpc_thunk_t *thunk) {
  not_implemented();
}

kmpc_thunk_t *__kmpc_task_buffer(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                                 __UNUSED__ kmpc_thunk_t *taskq_thunk, __UNUSED__ kmpc_task_t task) {
  not_implemented();
  return (kmpc_thunk_t *)NULL;
}
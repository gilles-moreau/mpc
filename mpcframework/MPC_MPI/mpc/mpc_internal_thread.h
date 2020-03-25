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

#ifndef MPC_INTERNAL_THREAD_H
#define MPC_INTERNAL_THREAD_H

#include <sctk_atomics.h>

struct sctk_task_specific_s;
struct sctk_thread_specific_s;

extern void __MPC_reinit_task_specific (struct sctk_task_specific_s *tmp);
extern int __MPC_atexit_task_specific(void (*function)(void));
extern void __MPC_init_thread_specific();
extern void __MPC_delete_thread_specific();


/* Disguisement Fast Path Checker */

extern sctk_atomics_int ________is_disguised;

static inline int __MPC_Maybe_disguised()
{
    return sctk_atomics_load_int(&________is_disguised);
}

#endif /* end of include guard: MPC_INTERNAL_THREAD_H */
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

#ifndef __MPCOMP_CORE_H__
#define __MPCOMP_CORE_H__

#include "mpcomp_types.h" /* need mpcomp_mvp_t && mpcomp_instance_t */

/**************
 * ALLOC HOOK *
 **************/

static inline void* mpcomp_alloc( int size ) 
{
  return sctk_malloc(size);
}

static inline void mpcomp_free( void *p ) 
{ 
    sctk_free(p); 
}

/*******************************
 * INITIALIZATION AND FINALIZE *
 *******************************/

void __mpcomp_init(void);
void __mpcomp_exit(void);
void __mpcomp_instance_init(mpcomp_instance_t *, int, mpcomp_team_t *);
void __mpcomp_in_order_scheduler(mpcomp_thread_t *);
void __mpcomp_flush(void);

#endif /* __MPCOMP_CORE_H__ */
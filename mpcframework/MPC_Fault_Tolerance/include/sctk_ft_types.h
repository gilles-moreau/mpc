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
/* #   - ADAM Julien <adamj@paratools.com>                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_FT_TYPES_H
#define __SCTK_FT_TYPES_H

typedef enum sctk_ft_state_e
{
  /* MPC_STATE_NO_SUPPORT IS 0
    and DEFINED IN MPC */
  MPC_STATE_ERROR=1,
  MPC_STATE_CHECKPOINT,
  MPC_STATE_RESTART,
  MPC_STATE_IGNORE,
  MPC_STATE_COUNT
} sctk_ft_state_t;

#endif /* ifndef __SCTK_FT_TYPES_H */

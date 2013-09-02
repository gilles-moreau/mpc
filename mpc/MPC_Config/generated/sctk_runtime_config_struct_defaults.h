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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #   - AUTOMATIC GENERATION                                             # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdlib.h>

#ifndef SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H
#define SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H

/******************************** STRUCTURE *********************************/
/* forward declaration functions */
struct sctk_runtime_config;

/********************************* FUNCTION *********************************/
/* reset functions */
void sctk_runtime_config_struct_init_allocator(void * struct_ptr);
void sctk_runtime_config_struct_init_launcher(void * struct_ptr);
void sctk_runtime_config_struct_init_profiler(void * struct_ptr);
void sctk_runtime_config_reset(struct sctk_runtime_config * config);

#endif /* SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H */

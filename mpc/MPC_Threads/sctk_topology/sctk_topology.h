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
#ifndef __SCTK_TOPOLOGY_H_
#define __SCTK_TOPOLOGY_H_

#include <stdio.h>

#include "hwloc.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /*
    Numbering rules

    Core_id are renubered according to a topological numbering. All threads sharing a 
    cache level must have continuous numbers.
  */


/*! \brief Initialize the topology module
*/
  void sctk_topology_init (void);

/*! \brief Destroy the topology module
*/
  void sctk_topology_destroy (void);

/*! \brief Return the current core_id 
*/
  int sctk_get_cpu (void);

/*! \brief Return the NUMA node according to the code_id number
 * @param cpu core_id
*/
  int sctk_get_node_from_cpu (const int cpu);

/*! \brief Return the total number of core for the process 
*/
  int sctk_get_cpu_number (void);

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
  int sctk_is_numa_node (void);

/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
*/
  void sctk_print_topology (FILE * fd);

/*! \brief Bind the current thread 
 * @ param i The cpu_id to bind
*/
  int sctk_bind_to_cpu (int i);

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
*/
  int sctk_get_first_cpu_in_node (int node);

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
  char *sctk_get_processor_name (void);

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
*/
  int sctk_set_cpu_number (int n);

/*! \brief Return the hostname
*/
  char *sctk_get_node_name (void);

/*! \brief Return the closest core_id 
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
  void sctk_get_neighborhood(int cpuid, int nb_cpus, int* neighborhood);

#ifdef __cplusplus
}
#endif
#endif

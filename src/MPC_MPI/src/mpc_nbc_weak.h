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
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

// PROFILER MPI_NBC MPI Non Blocking Collectives
/* Non Blocking Collectives  */
#pragma weak MPI_Ibarrier = PMPI_Ibarrier
#pragma weak MPI_Iallgatherv = PMPI_Iallgatherv
#pragma weak MPI_Ialltoallv = PMPI_Ialltoallv
#pragma weak MPI_Ialltoallw = PMPI_Ialltoallw
#pragma weak MPI_Ireduce_scatter = PMPI_Ireduce_scatter
#pragma weak MPI_Iscan = PMPI_Iscan
#pragma weak MPI_Iexscan = PMPI_Iexscan


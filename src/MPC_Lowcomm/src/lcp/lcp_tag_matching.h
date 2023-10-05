/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef LCP_TAG_MATCHING_H
#define LCP_TAG_MATCHING_H

#include <mpc_common_spinlock.h>

#include <mpc_common_datastructure.h>

#include "lcp_def.h"

#include "lcp_tag_lists.h"

typedef struct lcp_prq_match_entry_s
{
	uint64_t             comm_key;
	lcp_mtch_prq_list_t *pr_queue;
} lcp_prq_match_entry_t;

typedef struct lcp_prq_match_table_s
{
	mpc_common_spinlock_t  lock;
	struct mpc_common_hashtable ht;
} lcp_prq_match_table_t;

lcp_prq_match_table_t * lcp_prq_match_table_init();

typedef struct lcp_umq_match_entry_s
{
	uint64_t             comm_key;
	lcp_mtch_umq_list_t *um_queue;
} lcp_umq_match_entry_t;

typedef struct lcp_umq_match_table_s
{
	mpc_common_spinlock_t  lock;
	struct mpc_common_hashtable ht;
} lcp_umq_match_table_t;

lcp_umq_match_table_t * lcp_umq_match_table_init();


lcp_umq_match_entry_t *lcp_get_umq_entry(
		 lcp_umq_match_table_t *table,
		 uint64_t comm);

void lcp_append_prq(lcp_prq_match_table_t *prq, lcp_request_t *req,
		    uint64_t comm_id, int tag, uint64_t src);
lcp_request_t *lcp_match_prq(lcp_prq_match_table_t *prq, 
		    uint64_t comm_id, int tag, uint64_t src);

void lcp_append_umq(lcp_umq_match_table_t *umq, void *req,
		    uint64_t comm_id, int tag, uint64_t src);
void *lcp_match_umq(lcp_umq_match_table_t *umq,
		    uint64_t comm_id, int tag, uint64_t src);
void *lcp_search_umq(lcp_umq_match_table_t *umq,
                     uint64_t comm_id, int tag, uint64_t src);

void lcp_fini_matching_engine(lcp_umq_match_table_t *umq, 
			      lcp_prq_match_table_t *prq);
#endif

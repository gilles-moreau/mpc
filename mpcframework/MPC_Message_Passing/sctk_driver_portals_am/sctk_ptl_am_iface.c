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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#if defined(MPC_USE_PORTALS) && defined(MPC_Active_Message)
#include <limits.h>
#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "sctk_io_helper.h"
#include "ptl_layer.h"
#include "sctk_ptl_am_iface.h"
#include "sctk_ptl_am_types.h"
#include "sctk_atomics.h"
#include "sctk_pmi.h"
#include "sctk_accessor.h"

/**
 * Will print the Portals structure.
 * \param[in] srail the Portals rail to dump
 */
void sctk_ptl_am_print_structure(sctk_ptl_am_rail_info_t* srail)
{
	sctk_ptl_limits_t l = srail->max_limits;
	sctk_info(
	"\n======== PORTALS AM STRUCTURE ========\n"
	"\n"
	" PORTALS ENTRIES       : \n"
	"  - PTE flags          : 0x%x\n"
	"  - Comm. nb entries   : %d\n"
	"  - each EQ size       : %d\n"
	"\n"
	" ME management         : \n"
	"  - ME PUT flags       : 0x%x\n"
	"  - ME GET flags       : 0x%x\n"
	"\n"
	" MD management         : \n"
	"  - shared EQ size     : %d\n"
	"  - MD PUT flags       : 0x%x\n"
	"  - MD GET flags       : 0x%x\n"
	"\n"
	" PORTALS LIMITS        : \n"
	"  - MAX PT entries     : %d\n"
	"  - MAX unexpected     : %d\n"
	"  - MAX MDs            : %d\n"
	"  - MAX MEs            : %d\n"
	"  - MAX CTs            : %d\n"
	"  - MAX EQs            : %d\n"
	"  - MAX IOVECs         : %d\n"
	"  - MAX MEs/PTE        : %d\n"
	"  - MAX TriggeredOps   : %d\n"
	"\n===================================",
	SCTK_PTL_AM_PTE_FLAGS,
	srail->nb_entries,
	SCTK_PTL_AM_EQ_PTE_SIZE,

	SCTK_PTL_AM_ME_PUT_FLAGS,
	SCTK_PTL_AM_ME_GET_FLAGS,

	SCTK_PTL_AM_EQ_MDS_SIZE,
	SCTK_PTL_AM_MD_PUT_FLAGS,
	SCTK_PTL_AM_MD_GET_FLAGS,

	l.max_pt_index,
	l.max_unexpected_headers,
	l.max_mds,
	l.max_entries,
	l.max_cts,
	l.max_eqs,
	l.max_iovecs,
	l.max_list_size,
	l.max_triggered_ops
	);
	
}

/**
 * Init max values for driver config limits.
 * \param[in] l the limits to set
 */
static inline void __sctk_ptl_set_limits(sctk_ptl_limits_t* l)
{
	*l = (ptl_ni_limits_t){
		.max_entries            = INT_MAX,      /* Max number of entries allocated at any time */
		.max_unexpected_headers = INT_MAX,      /* max number of headers at any time */
		.max_mds                = INT_MAX,      /* Max number of MD allocated at any time */
		.max_cts                = INT_MAX,      /* Max number of CT allocated at any time */
		.max_eqs                = INT_MAX,      /* Max number of EQ allocated at any time */
		.max_pt_index           = INT_MAX,      /* Max PT index */
		.max_iovecs             = INT_MAX,      /* max number of iovecs for a single MD */
		.max_list_size          = INT_MAX,      /* Max number of entrie for one PT entry */
		.max_triggered_ops      = INT_MAX,      /* Max number of triggered ops */
		.max_msg_size           = PTL_SIZE_MAX, /* max message's size */
		.max_atomic_size        = PTL_SIZE_MAX, /* max size writable atomically */
		.max_fetch_atomic_size  = PTL_SIZE_MAX, /* Max size for a fetch op */
		.max_waw_ordered_size   = PTL_SIZE_MAX, /* Max length for ordering-guarantee */
		.max_war_ordered_size   = PTL_SIZE_MAX, /* max length for ordering guarantee */
		.max_volatile_size      = PTL_SIZE_MAX, /* Max length for MD w/ VOLATILE flag */
		.features               = 0             /* could be a combination or TARGET_BIND_INACCESSIBLE | TOTAL_DATA_ORDERING | COHERENT_ATOMIC */
	};
}

/**
 * Create the link between our program and the real driver.
 * \return the Portals rail object
 */
sctk_ptl_am_rail_info_t sctk_ptl_am_hardware_init()
{
	sctk_ptl_am_rail_info_t res;
	sctk_ptl_limits_t l;

	/* init the driver */
	sctk_ptl_chk(PtlInit());

	/* set max values */
	__sctk_ptl_set_limits(&l);

	/* init one physical interface */
	sctk_ptl_chk(PtlNIInit(
		PTL_IFACE_DEFAULT,                 /* Type of interface */
		PTL_NI_MATCHING | PTL_NI_PHYSICAL, /* physical NI, w/ matching enabled */
		PTL_PID_ANY,                       /* Let Portals decide process's PID */
		&l,                                /* desired Portals boundaries */
		&res.max_limits,                   /* effective Portals limits */
		&res.iface                         /* THE handler */
	));

	/* retrieve the process identifier */
	sctk_ptl_chk(PtlGetPhysId(
		res.iface, /* The NI handler */
		&res.id    /* the structure to store it */
	));

	return res;
}

/**
 * Shut down the link between our program and the driver.
 * \param[in] srail the Portals rail, created by sctk_ptl_hardware_init
 */
void sctk_ptl_am_hardware_fini(sctk_ptl_am_rail_info_t* srail)
{
	/* tear down the interface */
	sctk_ptl_chk(PtlNIFini(srail->iface));

	/* tear down the driver */
	PtlFini();
}

static void sctk_ptl_am_pte_populate(sctk_ptl_am_rail_info_t* srail, int srv_idx, int me_type, size_t nb_elts, int me_flags)
{
	size_t j = 0;
	sctk_ptl_am_pte_t* pte = srail->pte_table[srv_idx];

	sctk_assert(pte);
	sctk_ptl_am_matchbits_t match = (sctk_ptl_am_matchbits_t){
		.data.srvcode  = srv_idx,
		.data.rpcode   = 0,
		.data.tag      = 0,
		.data.inc_data = 0,
		.data.is_req   = me_type,
		.data.is_large = SCTK_PTL_AM_OP_SMALL
	};

	sctk_ptl_am_matchbits_t ign = (me_type == SCTK_PTL_AM_REQ_TYPE) ? SCTK_PTL_IGN_FOR_REQ : SCTK_PTL_IGN_FOR_REP;
	
	sctk_ptl_am_chunk_t* last = NULL, *first = NULL, *prev = NULL;

	for (j = 0; j < nb_elts; ++j) 
	{
		prev = last;
		sctk_ptl_am_local_data_t* block;
		last = (sctk_ptl_am_chunk_t*) sctk_calloc(1, sizeof(sctk_ptl_am_chunk_t));
		sctk_atomics_store_int(&last->refcnt, 0);
		sctk_atomics_store_int(&last->noff, 0);

		int tag = (me_type == SCTK_PTL_AM_REQ_TYPE) ? 0 : sctk_atomics_fetch_and_incr_int(&pte->next_tag);
		match.data.tag = tag;
		last->tag = tag;

		block = sctk_ptl_am_me_create(
				last->buf,
				SCTK_PTL_AM_CHUNK_SZ,
				SCTK_PTL_ANY_PROCESS,
				match,
				ign,
				me_flags
				);
		sctk_ptl_am_me_register(srail, block, pte);
		last->uptr = block;
		last->next = NULL;
		
		/* remember the first block, to append into our list */
		if(!first)
			first = last;
		if(prev)
			prev->next = last;

		// sctk_warning("POPULATE %s %p - %p (%llu)", ((me_type == SCTK_PTL_AM_REQ_TYPE) ? "REQ" : "REP"), last->buf, last->buf + SCTK_PTL_AM_CHUNK_SZ, SCTK_PTL_AM_CHUNK_SZ);
		// sctk_error("W/ match = %s", __sctk_ptl_am_match_str(sctk_malloc(70),70, match.raw));
		// sctk_error("W/ ign   = %s", __sctk_ptl_am_ign_str(sctk_malloc(70),70, ign.raw));
	}

	/* store the new start */
	if(first)
	{
		if(me_type == SCTK_PTL_AM_REQ_TYPE)
			pte->req_head = first;
		else
			pte->rep_head = first;
	}
}

/**
 * Map the Portals structure in user-space to be ready for communication.
 *
 * After this functions, Portals entries should be ready to use. This function
 * is different from hardware init beceause it is possible to have multiple 
 * software implementation relying on the same NI (why not?).
 * \param[in] srail the Portals rail
 * \param[in] dims PT dimensions
 */
void sctk_ptl_am_software_init(sctk_ptl_am_rail_info_t* srail, int nb_srv)
{
	size_t i, j;
	size_t eager_size = srail->eager_limit;
	srail->nb_entries = 0;

	srail->pte_table = (sctk_ptl_am_pte_t**) sctk_calloc(nb_srv, sizeof(sctk_ptl_am_pte_t*));

	/* Not initialize each PTE */
	for (i = 0; i < nb_srv; ++i)
	{
		sctk_ptl_am_pte_create(srail, i);
	}

	/* create the global EQ, shared by pending MDs */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,        /* The NI handler */
		SCTK_PTL_AM_EQ_MDS_SIZE, /* number of slots available for events */
		&srail->mds_eq        /* out: the EQ handler */
	));

	srail->md_slot.slot.md = (sctk_ptl_md_t){
		.start = NULL,
		.length = ~(0ULL),
		.options = SCTK_PTL_AM_MD_FLAGS,
		.eq_handle = srail->mds_eq,
		.ct_handle = PTL_CT_NONE
	};
	sctk_ptl_chk(PtlMDBind(
		srail->iface,     /* the NI handler */
		&srail->md_slot.slot.md ,   /* the MD to bind with memory region */
		&srail->md_slot.slot_h.mdh /* out: the MD handler */
	));

	sctk_error("MYSELF: %d/%d", srail->id.phys.nid, srail->id.phys.pid);
	sctk_ptl_am_print_structure(srail);
}

/**
 * Dynamically create a new Portals entry.
 * \param[in] srail the Portals rail
 * \param[out] pte Portals entry pointer, to init
 * \param[in] key the Portals desired ID
 */
void sctk_ptl_am_pte_create(sctk_ptl_am_rail_info_t* srail, size_t key)
{
	size_t eager_size = srail->eager_limit;
	sctk_ptl_am_pte_t* pte = srail->pte_table[key];

	if(pte)
		return;

	pte = (sctk_ptl_am_pte_t*)sctk_malloc(sizeof(sctk_ptl_am_pte_t));
	srail->pte_table[key] = pte;
	/* create the EQ for this PT */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,            /* the NI handler */
		SCTK_PTL_AM_EQ_PTE_SIZE, /* the number of slots in the EQ */
		&pte->eq                 /* the returned handler */
	));

	/* register the PT */
	sctk_ptl_chk(PtlPTAlloc(
		srail->iface,          /* the NI handler */
		SCTK_PTL_AM_PTE_FLAGS, /* PT entry specific flags */
		pte->eq,               /* the EQ for this entry */
		key,                   /* the desired index value */
		&pte->idx              /* the effective index value */
	));

	pte->pte_lock = SCTK_SPINLOCK_INITIALIZER;
	sctk_atomics_store_int(&pte->next_tag, 0);
	pte->req_head = NULL;
	pte->rep_head = NULL;

	sctk_ptl_am_pte_populate(srail, key, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_REQ_NB_DEF, SCTK_PTL_AM_ME_PUT_FLAGS | PTL_ME_MANAGE_LOCAL);
	sctk_ptl_am_pte_populate(srail, key, SCTK_PTL_AM_REP_TYPE, SCTK_PTL_AM_REP_NB_DEF, SCTK_PTL_AM_ME_PUT_FLAGS);

	srail->nb_entries++;
}

sctk_ptl_am_msg_t* sctk_ptl_am_send_request(sctk_ptl_am_rail_info_t* srail, int srv, int rpc, const void* start_in, size_t sz_in, void** start_out, size_t* sz_out, sctk_ptl_am_msg_t* msg)
{
	/*
	 * 1. if large, prepare a dedicated ME
	 * 2. if no more MD space, create a new chunk
	 * 3. Save somewhere the offset where the request has been put into the MD (to be provided when calling Put())
	 */
	sctk_assert(sz_in >= 0);
	sctk_ptl_am_pte_t* pte = srail->pte_table[srv];
	sctk_ptl_am_msg_t* msg_ret = NULL;

	sctk_assert(pte);
	int tag = -1, dedicated_me = 0;
	/* find the MD where data could be memcpy'd */
	size_t space = 0;
	sctk_ptl_am_imm_data_t me_off;
	sctk_ptl_am_chunk_t* c_me;
	sctk_ptl_id_t remote = msg->remote;
	sctk_ptl_am_matchbits_t md_match, md_ign;

	/******************************************/
	/******************************************/
	/******************************************/
	/* 1. Look for a place to store the response, if needed */
	if(start_out)
	{
		int found_space;
		do
		{
			found_space = 1;
			c_me = pte->rep_head;
			while(c_me)
			{
				me_off.data.offset = sctk_atomics_fetch_and_add_int(&c_me->noff, SCTK_PTL_AM_REP_CELL_SZ);
				if(me_off.data.offset <= SCTK_PTL_AM_CHUNK_SZ) /* last cell */
					break;

				sctk_atomics_fetch_and_add_int(&c_me->noff, (-1) * (int)(SCTK_PTL_AM_REP_CELL_SZ));
				c_me = c_me->next;
			}
			if(!c_me)
			{
				sctk_ptl_am_pte_populate(srail, srv, SCTK_PTL_AM_REP_TYPE, 1, SCTK_PTL_AM_ME_PUT_FLAGS);
				found_space = 0;
			}
		} while (!found_space);
		tag = c_me->tag;
		/* ok, here me_off.data.offset is the cell offset for the response.
		 * This cell contains an header and we don't want Portals to erase it.
		 * 1. Copy the msg to the cell
		 * 2. shift the found offset to map the 'data' member into the imm_data
		 */
		msg_ret = (sctk_ptl_am_msg_t*)(c_me->buf + me_off.data.offset);
		msg_ret->completed = 0;
		msg_ret->remote = msg->remote;
		me_off.data.offset += SCTK_PTL_AM_REP_HDR_SZ;

	}
	
	/******************************************/
	/******************************************/
	/******************************************/
	/* 2. If the data to send are too large --> dedicate a GET ME */
	if(sz_in >= SCTK_PTL_AM_CHUNK_SZ)
	{
		sctk_ptl_am_local_data_t* user_ptr;
		sctk_ptl_am_matchbits_t match = SCTK_PTL_MATCH_BUILD(srv, rpc, tag, 1, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_OP_LARGE);
		sctk_ptl_am_matchbits_t ign = SCTK_PTL_IGN_FOR_LARGE;

		/* register the ME */	
		user_ptr = sctk_ptl_am_me_create((void*)start_in, sz_in, remote , match, ign, SCTK_PTL_AM_ME_GET_FLAGS);
		sctk_ptl_am_me_register(srail, user_ptr, pte);

		/* sz = 0 --> empty PUT() */
		sz_in = 0;
		dedicated_me = 1;
	}
	
	/* Set flags to match a generic request slot */
	md_match = SCTK_PTL_MATCH_BUILD(srv, rpc, tag , !dedicated_me, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_OP_SMALL);
	md_ign = SCTK_PTL_IGN_FOR_REQ;

	sctk_ptl_am_emit_put(&srail->md_slot, sz_in, remote, pte, md_match, (size_t)start_in, SCTK_PTL_AM_ME_NO_OFFSET, me_off, NULL);

	return msg_ret;
}

void sctk_ptl_am_send_response(sctk_ptl_am_rail_info_t* srail, int srv, int rpc, void* start, size_t sz, int remote_id, sctk_ptl_am_msg_t* msg)
{
	sctk_ptl_am_pte_t* pte = srail->pte_table[srv];
	sctk_ptl_am_matchbits_t md_match, md_ign;
	sctk_ptl_id_t remote = msg->remote;
	int dedicated_me = 0;

	/******************************************/
	/******************************************/
	/******************************************/
	/* 2. If the data to send are too large --> dedicate a GET ME */
	if(sz >= SCTK_PTL_AM_CHUNK_SZ)
	{
		sctk_ptl_am_local_data_t* user_ptr;
		sctk_ptl_am_matchbits_t match = SCTK_PTL_MATCH_BUILD(srv, rpc, msg->tag, 1, SCTK_PTL_AM_REP_TYPE, SCTK_PTL_AM_OP_LARGE);
		sctk_ptl_am_matchbits_t ign = SCTK_PTL_IGN_FOR_LARGE;

		/* register the ME */	
		user_ptr = sctk_ptl_am_me_create((void*)start, sz, remote , match, ign, SCTK_PTL_AM_ME_GET_FLAGS);
		sctk_ptl_am_me_register(srail, user_ptr, pte);

		/* sz = 0 --> empty PUT() */
		sz = 0;
		dedicated_me = 1;
	}
	
	/* Set flags to match a pre-reserved response slot */
	md_match = SCTK_PTL_MATCH_BUILD(srv, rpc, msg->tag , !dedicated_me, SCTK_PTL_AM_REP_TYPE, SCTK_PTL_AM_OP_SMALL);
	md_ign = SCTK_PTL_IGN_FOR_REP;


	/* emit the PUT */
	//sctk_ptl_am_emit_put(&srail->md_slot, sz, remote, pte, md_match, (size_t)start, msg->offset, SCTK_PTL_NO_IMM_DATA, NULL);
	sctk_ptl_am_emit_put(&srail->md_slot, sz, remote, pte, md_match, (size_t)start, msg->offset, SCTK_PTL_NO_IMM_DATA, NULL);
}

void sctk_ptl_am_wait_response(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_msg_t* msg)
{
	while(!msg->completed)
	{
		sctk_thread_yield();
		sctk_ptl_am_incoming_lookup(srail);
	}
}

/**
 * Release Portals structure from our program.
 *
 * After here, no communication should be made on these resources
 * \param[in] srail the Portals rail
 */
void sctk_ptl_am_software_fini(sctk_ptl_am_rail_info_t* srail)
{
	int table_dims = srail->nb_entries;
	assert(table_dims > 0);
	int i;
	void* base_ptr = NULL;

	/* don't want to hang the NIC */
	return;

	for(i = 0; i < table_dims; i++)
	{
		sctk_ptl_am_pte_t* cur = srail->pte_table[i];
		if(i==0)
			base_ptr = cur;

		sctk_ptl_chk(PtlEQFree(
			cur->eq       /* the EQ handler */
		));

		sctk_ptl_chk(PtlPTFree(
			srail->iface,     /* the NI handler */
			cur->idx      /* the PTE to destroy */
		));
	}

	sctk_ptl_chk(PtlEQFree(
		srail->mds_eq             /* the EQ handler */
	));

	/* write 'NULL' to be sure */
	sctk_free(base_ptr);
	srail->nb_entries = 0;
}

/**
 * This function creates a new memory entry.
 *
 * \param[in] start first buffer address
 * \param[in] size number of bytes to include into the ME
 * \param[in] remote the target process id
 * \param[in] match the matching bits for this slot
 * \param[in] ign the ignoring bits from the above parameter
 * \param[in] flags ME-specific flags (PUT,GET,...)
 * \return the allocated request
 */
sctk_ptl_am_local_data_t* sctk_ptl_am_me_create(void * start, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_matchbits_t match, sctk_ptl_am_matchbits_t ign, int flags )
{
	sctk_ptl_am_local_data_t* user_ptr = (sctk_ptl_am_local_data_t*) sctk_malloc(sizeof(sctk_ptl_am_local_data_t));

	*user_ptr = (sctk_ptl_am_local_data_t){
		.slot.me = (sctk_ptl_me_t){
			.start = start,
			.length = (ptl_size_t) size,
			.ct_handle = PTL_CT_NONE,
			.uid = PTL_UID_ANY,
			.match_id = remote,
			.match_bits = match.raw,
			.ignore_bits = ign.raw,
			.options = flags,
			.min_free = 1
		},
		.slot_h.meh = PTL_INVALID_HANDLE,
	};

	return user_ptr;
}
				
int sctk_ptl_am_incoming_lookup(sctk_ptl_am_rail_info_t* srail)
{
	sctk_ptl_event_t ev;
	sctk_ptl_am_pte_t* pte = NULL;
	int ret = -1;
	size_t n_ptes = srail->nb_entries, i = 0;
	while(!pte && i < n_ptes)
	{
			pte = srail->pte_table[i++];
	}
	
	if(!pte) /* No valid PTE found */
		return 2;

	ret = sctk_ptl_am_eq_poll_me(srail, pte, &ev);
	if(ret == PTL_OK)
	{
		if(ev.ni_fail_type != PTL_NI_OK) 
		{
				sctk_error("ME: Failed event %s: %d", sctk_ptl_am_event_decode(ev), ev.ni_fail_type);
				CRASH();
		}

		/* repopulate a new slot if needed */
		sctk_ptl_am_local_data_t* chunk = (sctk_ptl_am_local_data_t*)ev.user_ptr;
		unsigned long long start_offset = (unsigned long long)ev.start - (unsigned long long)chunk->slot.me.start;
		if( start_offset <= SCTK_PTL_AM_TRSH_FOR_NEW_CHUNK && start_offset + ev.mlength > SCTK_PTL_AM_TRSH_FOR_NEW_CHUNK)
		{
			sctk_ptl_am_pte_populate(srail, ev.pt_index, SCTK_PTL_AM_REQ_TYPE, 1, SCTK_PTL_AM_ME_PUT_FLAGS | PTL_ME_MANAGE_LOCAL);
		}

		switch ( ev.type )
		{
			case PTL_EVENT_PUT:
			{
				sctk_ptl_am_matchbits_t m;
				m.raw = ev.match_bits;
				sctk_arpc_context_t ctx = ( sctk_arpc_context_t ){.dest = -1, .srvcode = ev.pt_index, .rpcode = m.data.rpcode};
				sctk_ptl_am_imm_data_t hdr_data;
				hdr_data.raw = ev.hdr_data;
				sctk_ptl_am_msg_t msg = ( sctk_ptl_am_msg_t ){
					.remote = ev.initiator,
					.offset = hdr_data.data.offset,
					.tag = m.data.tag
				};
				void *resp_buf = NULL;
				size_t sz = 0;

				if ( m.data.is_req == SCTK_PTL_AM_REQ_TYPE ) /* received an RPC to handle */
				{
					if ( m.data.is_large )
					{
						/* TODO: Get() data first and replace ev.start below */
					}
					arpc_recv_call_ptl( &ctx, ev.start, ev.mlength, &resp_buf, &sz, &msg );
				}
				else /* Received a response */
				{
					if ( m.data.is_large )
					{
						/* TODO: Get() data first */
					}
					sctk_ptl_am_msg_t *msg = container_of( ev.start, sctk_ptl_am_msg_t, data );

					msg->completed = 1;
				}
				break;
			}
			default:
				sctk_warning("Not handled ME event: %s", sctk_ptl_am_event_decode(ev));
		}
		return 0;
	}

	/* no event found in any valid EQ */
	return 1;
}

int sctk_ptl_am_outgoing_lookup(sctk_ptl_am_rail_info_t* srail)
{
	int ret;
	sctk_ptl_event_t ev;

	ret = sctk_ptl_am_eq_poll_md(srail, &ev);

	if(ret == PTL_OK)
	{
		sctk_assert(ev.ni_fail_type == PTL_NI_OK);
	}

	return ret;
}

/**
 * Register a memory entry to the specific PTE.
 *
 * \param[in] slot the already created ME
 * \param[in] pte  the PT index where the ME will be attached to
 * \param[in] list in which list this ME has to be registed ? PRIORITY || OVERFLOW
 */
void sctk_ptl_am_me_register(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_local_data_t* user_ptr, sctk_ptl_am_pte_t* pte)
{
	assert(user_ptr);
	sctk_ptl_chk(PtlMEAppend(
		srail->iface,         /* the NI handler */
		pte->idx,             /* the targeted PT entry */
		&user_ptr->slot.me,   /* the ME to register in the table */
		SCTK_PTL_PRIORITY_LIST, /* in which list the ME has to be appended */
		user_ptr,             /* usr_ptr: forwarded when polling the event */
		&user_ptr->slot_h.meh /* out: the ME handler */
	));
}

/**
 * Remove the ME from the Portals table.
 *
 * The unlink will generate an event that will be in charge of freeing memory.
 * This function should not be used if ME can be set with PTL_ME_USE_ONCE flag.
 * That's why we have \see sctk_ptl_me_free.
 *
 *  \param[in] meh the ME handler.
 */
void sctk_ptl_am_me_release(sctk_ptl_am_local_data_t* request)
{
	/* WARN: This function can return
	 * PTL_IN_USE if the targeted ME is in OVERFLOW_LIST
	 * and at least one unexpected header exists for it
	 */
	sctk_ptl_chk(PtlMEUnlink(
		request->slot_h.meh
	));
}

/**
 * Free the memory associated with a Portals request and free the request depending on parameters.
 * \param[in] request the request to free
 * \param[in] free_buffer is the 'start' buffer to be freed too ?
 */
void sctk_ptl_am_me_free(sctk_ptl_am_local_data_t* request, int free_buffer)
{
	if(free_buffer)
	{
		sctk_free(request->slot.me.start);
	}
	sctk_free(request);
}

/**
 * Get the current process Portals ID.
 * \param[in] srail the Portals-specific info struct
 * \return the current sctk_ptl_id_t
 */
sctk_ptl_id_t sctk_ptl_am_self(sctk_ptl_am_rail_info_t* srail)
{
	return srail->id;
}

/**
 * Send a PTL Get() request.
 *
 * \param[in] user the preset request, associated to a MD slot
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_off the offset in the local buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] user_ptr the request to attach to the Portals command.
 * \return PTL_OK, abort() otherwise.
 */
int sctk_ptl_am_emit_get(sctk_ptl_am_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_pte_t* pte, sctk_ptl_am_matchbits_t match, size_t local_off, size_t remote_off, void* user_ptr)
{
	sctk_ptl_chk(PtlGet(
		user->slot_h.mdh,
		local_off,
		size,
		remote,
		pte->idx,
		match.raw,
		remote_off,
		user_ptr
	));

	return PTL_OK;
}

/**
 * Emit a PTL Put() request.
 * \param[in] user the preset request, associated to a MD slot
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_off the offset in the local buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] user_ptr the request to attach to the Portals command.
 * \return PTL_OK, abort() otherwise.
 */
int sctk_ptl_am_emit_put(sctk_ptl_am_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_pte_t* pte, sctk_ptl_am_matchbits_t match, size_t local_off, size_t remote_off, sctk_ptl_am_imm_data_t extra, void* user_ptr)
{
	assert (size <= user->slot.md.length);

	//sctk_warning("PUT-%d %p+%llu -> %llu MATCH=%s (EXTRA=%llu)", pte->idx, user->slot.md.start+local_off, size, remote_off, __sctk_ptl_am_match_str((char*)sctk_malloc(70), 70, match.raw), extra.raw);
	
	sctk_ptl_chk(PtlPut(
		user->slot_h.mdh,
		local_off, /* offset */
		size,
		PTL_ACK_REQ,
		remote,
		pte->idx,
		match.raw,
		remote_off, /* offset */
		user_ptr,
		extra.raw
	));

	return PTL_OK;
}

/**
 * Create the initial Portals topology: the ring.
 * \param[in] rail the just-initialized Portals rail
 */
void sctk_ptl_am_register_process ( sctk_ptl_am_rail_info_t *srail )
{
	int tmp_ret;

	/** Portals initialization : Ring topology.
	 * 1. bind to right process through PMI
	 * 2. if process number > 2 create a route to the process to the left
	 * => Bidirectional ring
	 */
	
	/* serialize the id_t to a string, compliant with PMI handling */
	srail->connection_infos_size = sctk_ptl_am_data_serialize(
			&srail->id,              /* the process it to serialize */
			sizeof (sctk_ptl_id_t),  /* size of the Portals ID struct */
			srail->connection_infos, /* the string to store the serialization */
			MAX_STRING_SIZE          /* max allowed string's size */
	);
	assert(srail->connection_infos_size > 0);

	/* register the serialized id into the PMI */
	tmp_ret = sctk_pmi_put_connection_info (
			srail->connection_infos,      /* the string to publish */
			srail->connection_infos_size, /* string size */
			SCTK_PTL_AM_PMI_TAG             /* rail ID: PMI tag */
	);
	assert(tmp_ret == 0);

	//Wait for all processes to complete the ring topology init */
	sctk_pmi_barrier();
}

/** 
 * Retrieve the ptl_process_id object, associated with a given MPC process rank.
 * \param[in] rail the Portals rail
 * \param[in] dest the MPC process rank
 * \return the Portals process id
 */
sctk_ptl_id_t sctk_ptl_am_map_id(sctk_ptl_am_rail_info_t* srail, int dest)
{
	int tmp_ret;
	char connection_infos[MAX_STRING_SIZE];
	sctk_ptl_id_t id = SCTK_PTL_ANY_PROCESS;

	/* retrieve the right neighbour id struct */
	tmp_ret = sctk_pmi_get_connection_info (
			connection_infos,  /* the recv buffer */
			MAX_STRING_SIZE,   /* the recv buffer max size */
			SCTK_PTL_AM_PMI_TAG, /* rail IB: PMI tag */
			dest               /* which process we are targeting */
	);

	assert(tmp_ret == 0);
	
	sctk_ptl_am_data_deserialize(
			connection_infos, /* the buffer containing raw data */
			&id,               /* the target struct */
			sizeof (id )      /* target struct size */
	);
	return id;
}
#endif

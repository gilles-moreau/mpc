#ifdef MPC_USE_PORTALS

#include "sctk_types.h"
#include "sctk_route.h"
#include "sctk_ptl_rdma.h"
#include "sctk_ptl_iface.h"
#include "sctk_atomics.h"

static sctk_ptl_pte_t* rdma_pte = NULL;

static inline ptl_datatype_t __sctk_ptl_convert_type(RDMA_type type)
{
	switch(type)
	{
		case RDMA_TYPE_CHAR:               return PTL_INT8_T; break;
		case RDMA_TYPE_DOUBLE:             return PTL_DOUBLE; break;
		case RDMA_TYPE_FLOAT:              return PTL_FLOAT; break;
		case RDMA_TYPE_INT:                return PTL_INT32_T; break;
		case RDMA_TYPE_LONG:               return PTL_INT64_T; break;
		case RDMA_TYPE_LONG_DOUBLE:        return PTL_LONG_DOUBLE; break;
		case RDMA_TYPE_LONG_LONG:          return PTL_INT64_T; break;
		case RDMA_TYPE_LONG_LONG_INT:      return PTL_INT64_T; break;
		case RDMA_TYPE_SHORT:              return PTL_INT16_T; break;
		case RDMA_TYPE_SIGNED_CHAR:        return PTL_INT8_T; break;
		case RDMA_TYPE_UNSIGNED:           return PTL_UINT32_T; break;
		case RDMA_TYPE_UNSIGNED_CHAR:      return PTL_UINT8_T; break;
		case RDMA_TYPE_UNSIGNED_LONG:      return PTL_UINT64_T; break;
		case RDMA_TYPE_UNSIGNED_LONG_LONG: return PTL_UINT64_T; break;
		case RDMA_TYPE_UNSIGNED_SHORT:     return PTL_UINT16_T; break;
		case RDMA_TYPE_WCHAR:              return PTL_INT16_T; break;
		default: 
			sctk_fatal("Type not handled by Portals: %d", type);
	}
	return 0;
}

static inline ptl_op_t __sctk_ptl_convert_op(RDMA_op op)
{
	switch(op)
	{
		case RDMA_SUM: return PTL_SUM; break;
		case RDMA_MIN: return PTL_MIN; break;
		case RDMA_MAX: return PTL_MAX; break;
		case RDMA_PROD: return PTL_PROD; break;
		case RDMA_LAND: return PTL_LAND; break;
		case RDMA_BAND: return PTL_BAND; break;
		case RDMA_LOR: return PTL_LOR; break;
		case RDMA_BOR: return PTL_BOR; break;
		case RDMA_LXOR: return PTL_LXOR; break;
		case RDMA_BXOR: return PTL_BXOR; break;
		default:
			sctk_fatal("Operation not supported by Portals %d", op);
	}
	return 0;
}

int sctk_ptl_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type )
{
	return 1;
}

void sctk_ptl_rdma_fetch_and_op(  sctk_rail_info_t *rail,
		sctk_thread_ptp_message_t *msg,
		void * fetch_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * add,
		RDMA_op op,
		RDMA_type type )
{
	not_implemented();
}

int sctk_ptl_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type )
{
	return 1;
}

void sctk_ptl_rdma_cas(sctk_rail_info_t *rail,
		sctk_thread_ptp_message_t *msg,
		void *  res_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * comp,
		void * new,
		RDMA_type type )
{
	sctk_ptl_local_data_t* md  = local_key->pin.ptl.md_data;
	sctk_ptl_local_data_t* me  = local_key->pin.ptl.me_data;
	sctk_ptl_id_t remote       = remote_key->pin.ptl.origin;
	sctk_ptl_rdma_type_t ptype = __sctk_ptl_convert_type(type);
	size_t local_getoff, local_putoff, remote_off;



	not_implemented();
#if 0
	/*TODO: 'new' is not in a mapped region !*/
	local_getoff = res_addr - ;
	local_putoff = ;
	remote_off   = remote_addr - me->slot.me.start;
	
	sctk_ptl_emit_swap(
		md,                                               /* The base MD for this windows */
		RDMA_type_size(type),                             /* request size */
		remote,                                           /* target process */
		rdma_pte, /* Portals entry */
		(sctk_ptl_matchbits_t)me->slot.me.match_bits,     /* match_bits */
		local_getoff, local_putoff, remote_off,           /* offsets */
		comp, ptype                                       /* value to compare with + RDMA type */
	);
#endif
}

void sctk_ptl_rdma_write(sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
		void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
		size_t size)
{
	sctk_ptl_id_t remote = remote_key->pin.ptl.origin;
	void* remote_start   = remote_key->pin.ptl.start;
	void* local_start    = local_key->pin.ptl.start;
	size_t local_off, remote_off;
	sctk_ptl_local_data_t* copy = NULL;

	/* sanity checks */
	sctk_assert(src_addr  >= local_start);
	sctk_assert(dest_addr >= remote_start);
	
	/* compute offsets, WRITE --> src = local, dest = remote */
	local_off  = src_addr  - local_start;
	remote_off = dest_addr - remote_start;

	/* THIS IS NOT A GOOD IDEA !!! */
	copy = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	*copy = *local_key->pin.ptl.md_data;
	copy->msg = msg;
	msg->tail.ptl.user_ptr = copy;
	
	sctk_ptl_emit_put(
		local_key->pin.ptl.md_data, /* The base MD */
		size,                      /* request size */
		remote,                    /* target process */
		rdma_pte,                  /* Portals entry */
		remote_key->pin.ptl.match, /* match bits */
		local_off, remote_off,     /* offsets */
		0,                         /* Number of bytes sent */
		copy
	);
}

void sctk_ptl_rdma_read(sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
		void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
		size_t size)
{
	sctk_ptl_id_t remote = remote_key->pin.ptl.origin;
	void* remote_start   = remote_key->pin.ptl.start;
	void* local_start    = local_key->pin.ptl.start;
	size_t local_off, remote_off;
	sctk_ptl_local_data_t* copy = NULL;

	sctk_warning("GET %p <- %p (sz: %llu)", dest_addr, src_addr, size);
	/* sanity checks */
	sctk_assert(src_addr  >= remote_start);
	sctk_assert(dest_addr >= local_start);
	
	/* compute offsets, READ --> src = remote, dest = local */
	local_off  = dest_addr - local_start;
	remote_off = src_addr  - remote_start;

	/* THIS IS NOT A GOOD IDEA !!! */
	copy = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	*copy = *local_key->pin.ptl.md_data;
	copy->msg = msg;
	msg->tail.ptl.user_ptr = copy;

	sctk_ptl_emit_get(
		local_key->pin.ptl.md_data, /* the base MD */
		size,                      /* request size */
		remote,                    /* target Process */
		rdma_pte,                  /* Portals entry */
		remote_key->pin.ptl.match, /* match_bits */
		local_off, remote_off,     /* offsets */
		copy
	);
}

/* Pinning */
void sctk_ptl_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	sctk_ptl_rail_info_t* srail    = &rail->network.ptl;
	sctk_ptl_local_data_t *md_request , *me_request;
	void  *md_start                   , *me_start;
	size_t md_size                    , me_size;
	int md_flags                      , me_flags;
	sctk_ptl_matchbits_t md_match     , me_match, me_ign;
	sctk_ptl_pte_t *md_pte            , *me_pte;
	sctk_ptl_id_t md_remote          , me_remote;

	if(rdma_pte==NULL)
		rdma_pte = MPCHT_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);

	md_request = me_request = NULL;
	md_match   = me_match   = me_ign = SCTK_PTL_MATCH_INIT;
	md_pte     = me_pte     = NULL;
	md_remote  = me_remote  = SCTK_PTL_ANY_PROCESS;
	md_start   = me_start   = NULL;
	md_size    = me_size    = 0;
	md_flags   = me_flags   = 0;

	/* Configure the MD */
	md_start              = addr;
	md_size               = size;
	md_flags              = SCTK_PTL_MD_PUT_FLAGS | SCTK_PTL_MD_GET_FLAGS;
	md_pte                = rdma_pte;
	md_request            = sctk_ptl_md_create(srail, md_start, md_size, md_flags);
	sctk_ptl_md_register(srail, md_request);

	/* configure the ME */
	me_start              = addr;
	me_size               = size;
	me_flags              = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ME_GET_FLAGS;
	me_match.data.tag     = sctk_atomics_fetch_and_incr_int(&rail->network.ptl.rdma_cpt);
	me_ign.data.rank      = SCTK_PTL_IGN_RANK;
	me_pte                = rdma_pte;
	me_remote             = SCTK_PTL_ANY_PROCESS;
	me_request            = sctk_ptl_me_create(me_start, me_size, me_remote, me_match, me_ign, me_flags);
	sctk_ptl_me_register(srail, me_request, me_pte);

	list->rail_id         = rail->rail_number;
	list->pin.ptl.me_data = me_request;
	list->pin.ptl.md_data = md_request;
	list->pin.ptl.origin  = srail->id;
	list->pin.ptl.start   = me_start;
	list->pin.ptl.match   = me_match;

	sctk_nodebug("REGISTER RDMA %p->%p (match=%s)", me_start, me_start + me_size,  __sctk_ptl_match_str(sctk_malloc(32), 32, me_match.raw));
}
void sctk_ptl_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	sctk_ptl_md_release(list->pin.ptl.md_data);
	sctk_ptl_me_release(list->pin.ptl.me_data);
	sctk_nodebug("FAKE RELEASE RDMA %p->%p", list->pin.ptl.me_data->slot.me.start, list->pin.ptl.me_data->slot.me.start + list->pin.ptl.me_data->slot.me.length);
}
#endif

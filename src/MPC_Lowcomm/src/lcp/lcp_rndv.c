#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

static size_t lcp_proto_rndv_pack(void *dest, void *data)
{
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.rndv.comm_id;
	hdr->base.dest         = req->send.rndv.dest;
	hdr->base.src          = req->send.rndv.src;
	hdr->base.tag          = req->send.rndv.tag;
	hdr->base.seqn         = req->msg_number;
	
	hdr->dest        = req->send.rndv.dest;
	hdr->msg_id      = req->msg_id;
	hdr->total_size  = req->send.length;
        hdr->remote_addr = 0; //FIXME: check how to differenciate with rndv put

	return sizeof(*hdr);
}

static size_t lcp_proto_ack_pack(void *dest, void *data)
{
	lcp_rndv_ack_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.am.comm_id;
	hdr->base.am_id = req->send.am.am_id;
	
	hdr->dest       = req->send.am.dest;
	hdr->msg_id     = req->msg_id;

	return sizeof(*hdr);
}

static size_t lcp_proto_rndv_get_pack(void *dest, void *data)
{
	int i;
	void *p;
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.rndv.comm_id;
	hdr->base.dest         = req->send.rndv.dest;
	hdr->base.src          = req->send.rndv.src;
	hdr->base.tag          = req->send.rndv.tag;
	hdr->base.seqn         = req->msg_number;
	
	hdr->dest        = req->send.rndv.dest;
	hdr->msg_id      = req->msg_id;
	hdr->total_size  = req->send.length;
        hdr->remote_addr = req->send.rndv.remote_addr;

	/* copy memory descriptors of all memory pins */
	p = (void *)(hdr + 1);
	for (i=0; i<req->send.rndv.lmem->num_ifaces; i++) {
		memcpy(p, (void *)(req->send.rndv.lmem->mems[i].memp),
			sizeof(lcr_memp_t));
		p += sizeof(lcr_memp_t);
	}

	return sizeof(*hdr) + req->send.rndv.lmem->num_ifaces * sizeof(lcr_memp_t);
}

static size_t lcp_proto_rndv_put_pack(void *dest, void *data)
{
	int i;
	void *p;
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.rndv.comm_id;
	hdr->base.dest         = req->send.rndv.dest;
	hdr->base.src          = req->send.rndv.src;
	hdr->base.tag          = req->send.rndv.tag;
	hdr->base.seqn         = req->msg_number;
	
	hdr->dest        = req->send.rndv.dest;
	hdr->msg_id      = req->msg_id;
	hdr->total_size  = req->send.length;
        hdr->remote_addr = 0;

	/* copy memory descriptors of all mpins*/
	p = (void *)(hdr + 1);
	for (i=0; i<req->send.rndv.lmem->num_ifaces; i++) {
		memcpy(p, (void *)(&req->send.rndv.lmem->mems[i].memp),
			sizeof(lcr_memp_t));
		p += sizeof(lcr_memp_t);
	}

	return sizeof(*hdr) + req->send.rndv.lmem->num_ifaces * sizeof(lcr_memp_t);
}

int lcp_request_unpack_mem_pins(lcp_mem_h *mem_p, lcr_memp_t *mems, size_t length, int num_mems)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;

        mem = sctk_malloc(sizeof(struct lcp_mem));
        if (mem == NULL) {
                mpc_common_debug_error("LCP: could not allocated mem when unpacking");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mem->num_ifaces = num_mems;
        mem->length     = length;
        mem->mems       = sctk_malloc(num_mems * sizeof(struct lcp_memp));
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocated mem when unpacking");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        for (i=0; i<num_mems; i++) {
                mem->mems[i].memp = sctk_malloc(sizeof(lcr_memp_t));
                if (mem->mems[i].memp == NULL) {
                        mpc_common_debug_error("LCP: could not allocated mem pin when unpacking");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                memcpy(mem->mems[i].memp, &mems[i], sizeof(lcr_memp_t));
        }

        *mem_p = mem;
err:
        return rc;

}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

//NOTE: completion callback are used:
//      1) for send zcopy (tag-matching and no tag-matching)
//      2) for send bcopy (tag-matching)
//      But not used for recv since handler is called and will
//      handle request completion. 
void lcp_request_complete_rndv(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

	//NOTE: completion may be called after ack is received 
	//      overriding thus MPC_LOWCOMM_LCP_RPUT_FRAG state.
	//      Thus it blocks the rendez-vous in SYNC state and 
	//      deadlock.
	//      Therefore, there is nothing to be done here.
        mpc_common_debug("LCP: rndv request completed. req=%p, msg_id=%llu",
			req, req->msg_id);
}

int lcp_send_tag_eager_ack_bcopy(lcp_request_t *req, uint8_t am_id);
void lcp_request_complete_ack_put(lcr_completion_t *comp)
{
        int rc;
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_t *ack_req;
                mpc_common_debug("LCP: rndv put request completed. req=%p, msg_id=%llu",
                                 req, req->msg_id);
                /* Send ack */
                rc = lcp_request_create(&ack_req);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        return;
                }
                lcp_request_init_ack(ack_req, req->send.ep, 
                                     req->send.rndv.comm_id, 
                                     req->send.rndv.dest,
                                     req->send.rndv.seqn,
                                     req->msg_id);

                mpc_common_debug("LCP: send ack comm_id=%llu, dest=%d, msg_id=%llu",
                                 ack_req->send.am.comm_id, ack_req->send.am.dest, 
                                 ack_req->msg_id);
                rc = lcp_send_tag_eager_ack_bcopy(ack_req, MPC_LOWCOMM_ACK_PUT_TM_MESSAGE);
                lcp_request_complete(req);
        }
}

void lcp_request_complete_rndv_get(lcr_completion_t *comp)
{
        int rc;
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_t *ack_req;
                lcp_request_t *rreq = req->super;
                mpc_common_debug("LCP: rndv get request completed. req=%p, msg_id=%llu",
                                 req, req->msg_id);
                /* Unregister memory */
                rc = lcp_mem_deregister(req->send.ep->ctx, req->send.rndv.lmem);
                /* Send ack */
                rc = lcp_request_create(&ack_req);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        return;
                }
                lcp_request_init_ack(ack_req, req->send.ep, 
                                     req->send.rndv.comm_id, 
                                     req->send.rndv.dest,
                                     req->send.rndv.seqn,
                                     req->msg_id);

                mpc_common_debug("LCP: send ack comm_id=%llu, dest=%d, msg_id=%llu",
                                 ack_req->send.am.comm_id, ack_req->send.am.dest, 
                                 ack_req->msg_id);
                rc = lcp_send_tag_eager_ack_bcopy(ack_req, MPC_LOWCOMM_ACK_GET_TM_MESSAGE);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not send ack get");
                        return;
                }
                lcp_request_complete(req);
                lcp_request_complete(rreq);
        }
}

void lcp_request_complete_ack(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        req->state.status = MPC_LOWCOMM_LCP_DONE;
	lcp_request_complete(req);
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

int lcp_send_am_eager_rndv_bcopy(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_rndv 
	};
	req->send.t_ctx.comp = cp;

	payload = lcp_send_do_am_bcopy(lcr_ep, 
			MPC_LOWCOMM_RDV_MESSAGE, 
			lcp_proto_rndv_pack, req);
	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}

	return rc;
}

int lcp_send_tag_eager_rndv_get_bcopy(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_tag_t tag = { .t_tag = {
		.src = req->send.rndv.src,
		.tag = req->send.rndv.tag,
		.seqn = req->msg_number }
	};
	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_RDV_GET_TM_MESSAGE,
		.seqn = req->msg_number }
	};
	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_rndv
	};

	req->send.t_ctx.arg     = ep->ctx;
	req->send.t_ctx.req     = req;
	req->send.t_ctx.comm_id = req->send.rndv.comm_id;
	req->send.t_ctx.comp    = cp;

	req->flags |= LCP_REQUEST_OFFLOADED;
	payload = lcp_send_do_tag_bcopy(lcr_ep, 
			tag, 
			imm.raw, 
			lcp_proto_rndv_get_pack, 
			req, 
			&(req->send.t_ctx));

	if (payload == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug_error("LCP: no resource available.");
		rc = MPC_LOWCOMM_NO_RESOURCE;
        }

	return rc;
}

int lcp_send_tag_eager_rndv_put_bcopy(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_tag_t tag = { .t_tag = {
		.src = req->send.rndv.src,
		.tag = req->send.rndv.tag,
		.seqn = req->msg_number }
	};
	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_RDV_PUT_TM_MESSAGE,
		.seqn = req->msg_number }
	};
	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_rndv 
	};

	req->send.t_ctx.arg     = ep->ctx;
	req->send.t_ctx.req     = req;
	req->send.t_ctx.comm_id = req->send.rndv.comm_id;
	req->send.t_ctx.comp    = cp;

	req->flags |= LCP_REQUEST_OFFLOADED;
	payload = lcp_send_do_tag_bcopy(lcr_ep, 
			tag, 
			imm.raw, 
			lcp_proto_rndv_put_pack, 
			req, 
			&(req->send.t_ctx));

	if (payload == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug_error("LCP: no resource available.");
		rc = MPC_LOWCOMM_NO_RESOURCE;
	} 

	return rc;
}

int lcp_send_tag_eager_rndv_bcopy(lcp_request_t *req) 
{
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_tag_t tag = { .t_tag = {
		.src = req->send.rndv.src,
		.tag = req->send.rndv.tag,
		.seqn = req->msg_number }
	};
	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_RDV_TM_MESSAGE,
		.seqn = req->msg_number }
	};
	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_rndv 
	};

	req->send.t_ctx.arg     = ep->ctx;
	req->send.t_ctx.req     = req;
	req->send.t_ctx.comm_id = req->send.rndv.comm_id;
	req->send.t_ctx.comp    = cp;

	req->flags |= LCP_REQUEST_OFFLOADED;
	payload = lcp_send_do_tag_bcopy(lcr_ep, 
			tag, 
			imm.raw, 
			lcp_proto_rndv_pack, 
			req, 
			&(req->send.t_ctx));

	if (payload == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug_error("LCP: no resource available.");
		rc = MPC_LOWCOMM_NO_RESOURCE;
	} else if ((unsigned long)payload < sizeof(lcp_rndv_hdr_t)) {
		mpc_common_debug_error("LCP: could not pack all data.");
		rc = MPC_LOWCOMM_ERROR;
	} 

	return rc;
}

int lcp_send_am_eager_ack_bcopy(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	int payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	payload = lcp_send_do_am_bcopy(lcr_ep,
			MPC_LOWCOMM_ACK_MESSAGE,
			lcp_proto_ack_pack, req);
	if (payload == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug_error("LCP: no resource available.");
		rc = MPC_LOWCOMM_NO_RESOURCE;
	} else if ((unsigned long)payload < sizeof(lcp_rndv_ack_hdr_t)) {
		mpc_common_debug_error("LCP: could not pack all data.");
		rc = MPC_LOWCOMM_ERROR;
	} 

	return rc;
}

int lcp_send_tag_eager_ack_bcopy(lcp_request_t *req, uint8_t am_id)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];
	
	lcr_tag_t tag = {.t_frag = {
		.f_id = 0,
		.m_id = req->msg_id }
	};

	lcp_tm_imm_t imm = { .hdr = {
		.prot = am_id,
		.seqn = req->msg_number }
	};
	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_ack 
	};

	req->send.t_ctx.arg     = ep->ctx;
	req->send.t_ctx.req     = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;
	req->send.t_ctx.comp    = cp;

	payload = lcp_send_do_tag_bcopy(lcr_ep, 
			tag, 
			imm.raw, 
			lcp_proto_ack_pack, 
			req, 
			&(req->send.t_ctx));
	if (payload == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug_error("LCP: no resource available.");
		rc = MPC_LOWCOMM_NO_RESOURCE;
	} else if ((unsigned long)payload < sizeof(lcp_rndv_ack_hdr_t)) {
		mpc_common_debug_error("LCP: could not pack all data.");
		rc = MPC_LOWCOMM_ERROR;
	} 

	return rc;
}

int lcp_recv_tag_ack(lcp_context_h ctx, lcp_request_t *req, sctk_rail_info_t *iface);
int lcp_send_rndv_start(lcp_request_t *req) 
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_ep_h ep = req->send.ep;
	lcp_chnl_idx_t cc = ep->priority_chnl;

	mpc_common_debug_info("LCP: send rndv req=%p, comm_id=%lu, tag=%d, "
			      "src=%d, dest=%d, msg_id=%d.", req, req->send.rndv.comm_id, 
			      req->send.rndv.tag, req->send.rndv.src, 
			      req->send.rndv.dest, req->msg_id);

        req->state.status = MPC_LOWCOMM_LCP_RPUT_SYNC;
	req->flags        |= LCP_REQUEST_SEND_FRAG;

	/* register req request inside table */
	if (lcp_pending_create(ep->ctx->pend_send_req, req, 
				     req->msg_id) == NULL) {
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	if (LCR_IFACE_IS_TM(ep->lct_eps[cc]->rail)) {
		if (ep->ep_config.rndv.max_get_zcopy > 0) {
                        rc = lcp_mem_register(ep->ctx, &req->send.rndv.lmem,
                                              req->send.buffer, 
                                              req->send.length);
                        req->send.rndv.remote_addr = (uint64_t)req->send.buffer;
                        if (rc != MPC_LOWCOMM_SUCCESS)
                                goto err;
			req->send.func = lcp_send_tag_eager_rndv_get_bcopy;
		} else if (ep->ep_config.rndv.max_put_zcopy > 0) {
                        rc = lcp_mem_register(ep->ctx, &req->send.rndv.lmem, 
                                              req->send.buffer, 
                                              req->send.length);
                        if (rc != MPC_LOWCOMM_SUCCESS)
                                goto err;
			req->send.func = lcp_send_tag_eager_rndv_put_bcopy;
		} else {
			req->send.func = lcp_send_tag_eager_rndv_bcopy;
		}
	} else {
		req->send.func = lcp_send_am_eager_rndv_bcopy;
	}

	if (LCR_IFACE_IS_TM(ep->lct_eps[cc]->rail)) {
		rc = lcp_recv_tag_ack(ep->ctx, req, ep->lct_eps[cc]->rail);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not post ack.");
		}
	}
err:
	return rc;
}

int lcp_rndv_matched(lcp_context_h ctx, 
		lcp_request_t *rreq,
		lcp_rndv_hdr_t *hdr) 
{
	int rc;
	lcp_ep_ctx_t *ctx_ep;
	lcp_ep_h ep;
	lcp_request_t *ack_req;

	/* Init all protocol data */
	rreq->msg_id            = hdr->msg_id;
	rreq->recv.send_length  = hdr->total_size; 
        rreq->state.remaining   = hdr->total_size;
	rreq->state.offset      = 0;
	rreq->msg_number        = hdr->base.seqn;
	rreq->state.status      = MPC_LOWCOMM_LCP_RPUT_FRAG;

	/* Get LCP endpoint if exists */
	HASH_FIND(hh, ctx->ep_ht, &hdr->base.src, sizeof(uint64_t), ctx_ep);
	if (ctx_ep == NULL) {
		rc = lcp_ep_create(ctx, &ep, hdr->base.src, 0);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not create ep after match.");
			goto err;
		}
	} else {
		ep = ctx_ep->ep;
	}

        if (rreq->flags & LCP_REQUEST_RNDV_GET_TAG) {
                lcp_request_t *get_req;
                lcp_request_create(&get_req);
                if (get_req == NULL) {
			mpc_common_debug_error("LCP: could not allocate rndv req.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                lcp_request_init_get(get_req, ep, rreq->recv.buffer, rreq->recv.tag.comm_id,
                                     hdr->dest, hdr->base.seqn, hdr->msg_id,
                                     hdr->total_size, hdr->remote_addr);
                lcr_completion_t cp = {
                        .comp_cb = lcp_request_complete_rndv_get
                };
                get_req->state.comp = cp;
                get_req->super = rreq;
                get_req->flags |= LCP_REQUEST_SEND_CTRL;
                get_req->ctx   = rreq->ctx;
                rc = lcp_mem_register(ep->ctx, 
                                      &(get_req->send.rndv.lmem), 
                                      rreq->recv.buffer, 
                                      hdr->total_size);
                if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not register mem during rget.");
                        goto err;
                }
                //FIXME: verify if same number of iface
                lcp_request_unpack_mem_pins(&(get_req->send.rndv.rmem), 
                                            (lcr_memp_t *)(hdr + 1), hdr->total_size,
                                            get_req->send.rndv.lmem->num_ifaces);

                get_req->send.func = lcp_send_get_zcopy_multi;
                if (lcp_pending_create(ctx->pend_send_req, get_req, 
                                       get_req->msg_id) == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                get_req->state.status = MPC_LOWCOMM_LCP_RPUT_FRAG;

                return rc;
        }

	/* register request inside table */
        if (lcp_pending_create(ctx->pend_recv_req, rreq, 
                               rreq->msg_id) == NULL) {
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Post receive if tag offload interface exists */
	if (LCR_IFACE_IS_TM(ep->lct_eps[ep->priority_chnl]->rail)) {
		rc = lcp_recv_tag_zcopy_multi(ep, rreq);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
	}

	/* Send ack */
	rreq->flags             |= LCP_REQUEST_RECV_FRAG;
	rc = lcp_request_create(&ack_req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto err;
	}
	lcp_request_init_ack(ack_req, ep, 
                             hdr->base.base.comm_id, 
                             hdr->base.dest,
                             hdr->base.seqn,
                             hdr->msg_id);

	mpc_common_debug("LCP: send ack comm_id=%llu, dest=%d, msg_id=%llu",
			 ack_req->send.am.comm_id, ack_req->send.am.dest, 
			 ack_req->msg_id);
	if (LCR_IFACE_IS_TM(ep->lct_eps[ep->priority_chnl]->rail)) {
		rc = lcp_send_tag_eager_ack_bcopy(ack_req, MPC_LOWCOMM_ACK_PUT_TM_MESSAGE);
	} else {
		rc = lcp_send_am_eager_ack_bcopy(ack_req);
	}

err:
	return rc;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */

//NOTE: counter-intuitively, we use the context of a send request here
//      even though we post a recv request.
int lcp_recv_tag_ack(lcp_context_h ctx, 
		lcp_request_t *sreq, 
		sctk_rail_info_t *iface)
{
	lcr_tag_t tag = { .t_frag = {
		.f_id = 0,
		.m_id = sreq->msg_id }
	};
	lcr_tag_t ign_tag = { .t = 0 };

	struct iovec iov = { 
                .iov_base = NULL,
                .iov_len  = 0
	};

	sreq->send.t_ctx.arg = ctx;
	sreq->send.t_ctx.req = sreq;
	sreq->send.t_ctx.comm_id = sreq->send.rndv.comm_id;

	return lcp_recv_do_tag_zcopy(iface,
			tag,
			ign_tag,
			&iov,
			1,
			&(sreq->send.t_ctx));
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

//NOTE: flags could be used to avoid duplicating functions for
//      AM and TAG
int lcp_rndv_am_tag_handler(void *arg, void *data,
			 size_t length, 
			 __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_rndv_hdr_t *hdr = data;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;

	/* No payload in rndv message for now */
	assert(length - sizeof(lcp_rndv_hdr_t) == 0);

	LCP_CONTEXT_LOCK(ctx);

	mpc_common_debug_info("LCP: recv rndv header src=%d, dest=%d, msg_id=%llu",
			      hdr->base.src_tsk, hdr->base.dest_tsk, hdr->msg_id);

	req = lcp_match_prq(ctx->prq_table, 
			    hdr->base.base.comm_id, 
			    hdr->base.tag, hdr->base.src);
	if (req != NULL) {
		mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
				 "tag=%d, src=%lu.", req, req->recv.tag.comm_id, 
				 req->recv.tag.tag, req->recv.tag.src);
		LCP_CONTEXT_UNLOCK(ctx); //NOTE: unlock context to enable endpoint creation.
		rc = lcp_rndv_matched(ctx, req, hdr);
	} else {
		lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
					    LCP_RECV_CONTAINER_UNEXP_RNDV);
		lcp_append_umq(ctx->umq_table, (void *)ctnr, hdr->base.base.comm_id, 
			       hdr->base.tag, hdr->base.src);
		LCP_CONTEXT_UNLOCK(ctx);
		rc = MPC_LOWCOMM_SUCCESS;
	}


	return rc;
}

static int lcp_rndv_tag_handler(void *arg, void *data, size_t length,
		__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_context_h ctx = t_ctx->arg;
	lcp_rndv_hdr_t *hdr = data;
	lcp_request_t *req  = t_ctx->req;

	/* No payload in rndv message for now */
	assert(length == sizeof(lcp_rndv_hdr_t));

	mpc_common_debug_info("LCP: recv rndv offload header src=%d, dest=%d, msg_id=%llu",
			      hdr->base.src_tsk, hdr->base.dest_tsk, hdr->msg_id);


	rc = lcp_rndv_matched(ctx, req, hdr);

	return rc;
}

static int lcp_rndv_get_tag_handler(void *arg, void *data, size_t length,
		__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_context_h ctx = t_ctx->arg;
	lcp_rndv_hdr_t *hdr = data;
	lcp_request_t *req  = t_ctx->req;
        UNUSED(length);

	mpc_common_debug_info("LCP: recv rndv offload header src=%d, dest=%d, msg_id=%llu",
			      hdr->base.src_tsk, hdr->base.dest_tsk, hdr->msg_id);


        req->flags |= LCP_REQUEST_RNDV_GET_TAG;
	rc = lcp_rndv_matched(ctx, req, hdr);

	return rc;
}

static int lcp_ack_am_tag_handler(void *arg, void *data,
				__UNUSED__ size_t size, 
				__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_rndv_ack_hdr_t *hdr = data;
	lcp_request_t *req;

	mpc_common_debug_info("LCP: recv tag ack header comm_id=%llu, src=%d, dest=%d, "
			      "msg_id=%llu.", hdr->base.base.comm_id, hdr->src, hdr->dest, 
			      hdr->msg_id);

	/* Retrieve request */
        req = lcp_pending_get_request(ctx->pend_send_req, hdr->msg_id);
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not find ctrl msg: comm id=%llu, "
				       "msg id=%llu, src=%d, dest=%d.", 
				       hdr->base.base.comm_id, hdr->msg_id,
				       hdr->src, hdr->dest);
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Set send function that will be called in lcp_progress */
	req->send.func    = lcp_send_am_zcopy_multi;

	/* Update request state */
	req->state.status = MPC_LOWCOMM_LCP_RPUT_FRAG;
err:
	return rc;
}

static int lcp_ack_tag_handler(void *arg, void *data,
		size_t size, 
		__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_rndv_ack_hdr_t *hdr = data;
	lcp_request_t *req = t_ctx->req;

        assert(size == 0 && data == NULL);

	mpc_common_debug_info("LCP: recv tag offload ack header req=%p, msg_id=%llu",
                              req, req->msg_id);

	/* Set send function that will be called in lcp_progress */
        if (req->flags & LCP_REQUEST_RNDV_PUT_TAG) {
                req->send.rndv.remote_addr = hdr->remote_addr;
                lcp_request_unpack_mem_pins(&req->send.rndv.rmem,
                                            (lcr_memp_t *)(hdr + 1), req->send.length,
                                            req->send.rndv.lmem->num_ifaces);
                lcr_completion_t cb = {
                        .comp_cb = lcp_request_complete_ack_put
                };
                req->state.comp = cb;
                req->send.func    = lcp_send_put_zcopy_multi;
        } else {
                req->send.func    = lcp_send_tag_zcopy_multi;
        }

	/* Update request state */
	req->state.status = MPC_LOWCOMM_LCP_RPUT_FRAG;

	return rc;
}

static int lcp_ack_get_tag_handler(void *arg, void *data, 
                                   size_t size,
                                   __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_rndv_ack_hdr_t *hdr = data;
	lcp_request_t *req = t_ctx->req;

	mpc_common_debug_info("LCP: recv tag ack get header req=%p, msg_id=%llu",
                              req, req->msg_id);


        rc = lcp_mem_deregister(req->send.ep->ctx, req->send.rndv.lmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not deregister put memory");
                goto err;
        }
        lcp_request_complete(req);
err:
        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_ACK_MESSAGE, lcp_ack_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_ACK_TM_MESSAGE, lcp_ack_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_ACK_GET_TM_MESSAGE, lcp_ack_get_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_MESSAGE, lcp_rndv_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_TM_MESSAGE, lcp_rndv_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_GET_TM_MESSAGE, lcp_rndv_get_tag_handler, 0);

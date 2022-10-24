#include "lcp_types.h"

#include "lcp_def.h"
#include "lcp_request.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"

static inline int lcp_send_get_zcopy(lcp_request_t *req,
                                     _mpc_lowcomm_endpoint_t *lcr_ep,
                                     lcp_chnl_idx_t cc,
                                     uint64_t local_addr,
                                     uint64_t remote_addr,
                                     size_t length)
{
        int rc;

        rc = lcp_get_zcopy(lcr_ep, 
                           local_addr,
                           remote_addr,
                           *req->send.rndv.lmem->mems[cc].memp,
                           *req->send.rndv.rmem->mems[cc].memp,
                           length,
                           &req->state.comp);
        return rc;
}

static inline int lcp_send_put_zcopy(lcp_request_t *req,
                                     _mpc_lowcomm_endpoint_t *lcr_ep,
                                     lcp_chnl_idx_t cc,
                                     uint64_t local_addr,
                                     uint64_t remote_addr,
                                     size_t length)
{
        int rc;

        rc = lcp_put_zcopy(lcr_ep, 
                           local_addr,
                           remote_addr,
                           *req->send.rndv.lmem->mems[cc].memp,
                           *req->send.rndv.rmem->mems[cc].memp,
                           length,
                           &req->state.comp);
        return rc;
}

static inline void lcp_send_get_compute_remote_addr(lcp_request_t *req,
                                                   int f_id,
                                                   int num_chnls,
                                                   size_t frag_size,
                                                   lcp_mem_h lmem,
                                                   uint64_t *local_addr,
                                                   uint64_t *remote_addr,
                                                   size_t *length)
{
        uint64_t local_base  = (uint64_t)req->send.buffer;
        uint64_t remote_base = (uint64_t)req->send.rndv.remote_addr;
        struct lcp_memp memp = lmem->mems[f_id % num_chnls];
        size_t offset        = memp.base_addr + (f_id / num_chnls) * frag_size - local_base;

        *local_addr  = local_base  + offset;
        *remote_addr = remote_base + offset;

        if ((size_t)(local_addr - memp.base_addr + frag_size) > memp.len) {
                *length = memp.len % frag_size;
        } else {
                *length = frag_size;
        }
}

int lcp_send_get_zcopy_multi(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	size_t frag_length, length;
	_mpc_lowcomm_endpoint_t *lcr_ep;
	lcp_ep_h ep = req->send.ep;
        lcr_rail_attr_t attr;
        uint64_t local_addr;
        uint64_t remote_addr;

	/* get frag state */
	size_t remaining = req->state.remaining;
	int f_id         = req->state.f_id;
	ep->current_chnl = req->state.cur;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;

                lcp_send_get_compute_remote_addr(req, f_id, ep->num_chnls, 
                                                 frag_length, req->send.rndv.lmem, 
                                                 &local_addr, &remote_addr, 
                                                 &length);

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, remaining=%llu, "
				 "len=%d", f_id, req->send.tag.src_tsk, 
				 req->send.tag.dest_tsk, req->msg_id, remaining, length,
				 remaining);

                rc = lcp_send_get_zcopy(req, 
                                        lcr_ep, 
                                        ep->current_chnl, 
                                        local_addr, 
                                        remote_addr, 
                                        length);
		if (rc == MPC_LOWCOMM_NO_RESOURCE) {
			/* Save progress */
			req->state.remaining -= length;
			req->state.cur        = ep->current_chnl;
			req->state.f_id       = f_id;
			mpc_common_debug_info("LCP: fragmentation stalled, frag=%d, req=%p, msg_id=%llu, "
					"remaining=%d, ep=%d", f_id, req, req->msg_id, 
					remaining, ep->current_chnl);
			return rc;
		} else if (rc == MPC_LOWCOMM_ERROR) {
			mpc_common_debug_error("LCP: could not send fragment %d, req=%p, "
					"msg_id=%llu.", f_id, req, req->msg_id);
			break;
		}	       

		remaining -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
	}

        //NOTE: request is completed only after ack is sent
        req->state.status = MPC_LOWCOMM_LCP_DONE;

	return rc;
}

int lcp_send_put_zcopy_multi(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	size_t frag_length, length;
	_mpc_lowcomm_endpoint_t *lcr_ep;
	lcp_ep_h ep = req->send.ep;
        lcr_rail_attr_t attr;
        uint64_t local_addr;
        uint64_t remote_addr;

	/* get frag state */
	size_t remaining = req->state.remaining;
	int f_id         = req->state.f_id;
	ep->current_chnl = req->state.cur;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_put_zcopy;

                lcp_send_get_compute_remote_addr(req, req->state.f_id, ep->num_chnls, 
                                                 frag_length, req->send.rndv.lmem, &local_addr, 
                                                 &remote_addr, &length);

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, remaining=%llu, "
				 "len=%d", f_id, req->send.tag.src_tsk, 
				 req->send.tag.dest_tsk, req->msg_id, remaining, length,
				 remaining);

                rc = lcp_send_put_zcopy(req, 
                                        lcr_ep, 
                                        ep->current_chnl, 
                                        local_addr, 
                                        remote_addr, 
                                        length);
		if (rc == MPC_LOWCOMM_NO_RESOURCE) {
			/* Save progress */
			req->state.remaining -= length;
			req->state.cur        = ep->current_chnl;
			req->state.f_id       = f_id;
			mpc_common_debug_info("LCP: fragmentation stalled, frag=%d, req=%p, msg_id=%llu, "
					"remaining=%d, ep=%d", f_id, req, req->msg_id, 
					remaining, ep->current_chnl);
			return rc;
		} else if (rc == MPC_LOWCOMM_ERROR) {
			mpc_common_debug_error("LCP: could not send fragment %d, req=%p, "
					"msg_id=%llu.", f_id, req, req->msg_id);
			break;
		}	       

		remaining -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
	}

        req->state.status = MPC_LOWCOMM_LCP_DONE;

	return rc;
}
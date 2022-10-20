#include "lcp_ep.h"
#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_pending.h"
#include "lcp_header.h"
#include "lcp_rndv.h"
#include "lcp_prototypes.h"

#include "sctk_alloc.h"

int lcp_send_start(lcp_ep_h ep, lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	_mpc_lowcomm_endpoint_t *lcr_ep;

	if (req->send.length < ep->ep_config.max_bcopy) {
		lcr_ep  = ep->lct_eps[ep->priority_chnl];
		if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
			rc = lcp_send_tag_eager_tag_bcopy(req);
		} else {
			rc = lcp_send_am_eager_tag_bcopy(req);
		}
	} else if (req->send.length < ep->ep_config.max_zcopy) {
		lcr_ep  = ep->lct_eps[ep->priority_chnl];
		if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
			rc = lcp_send_tag_eager_tag_zcopy(req);
		} else {
			rc = lcp_send_am_eager_tag_zcopy(req);
		}
	} else {
		rc = lcp_send_rndv_start(req);
	}

		
	if (rc == MPC_LOWCOMM_NO_RESOURCE) {
	 	req->state.status = MPC_LOWCOMM_LCP_PEND;
		req->flags |= LCP_REQUEST_SEND_CTRL; /* to delete ctrl msg upon completion */
		if (lcp_pending_create(ep->ctx->pend_send_req, req, 
					     req->msg_id) == NULL) {
			rc = MPC_LOWCOMM_ERROR;
		}
		mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
				 req->send.tag.dest, req->msg_id);
	}

	return rc;
}

int lcp_send(lcp_ep_h ep, mpc_lowcomm_request_t *request, 
	     void *buffer, uint64_t seqn)
{
	int rc;
	lcp_request_t *req;

	uint64_t msg_id = lcp_rand_uint64();

	rc = lcp_request_create(&req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create request.");
		return MPC_LOWCOMM_ERROR;
	}
	lcp_request_init_send(req, ep, request, buffer, seqn, msg_id);
        req->flags |= LCP_REQUEST_MPI_COMPLETE;

	if (ep->state == LCP_EP_FLAG_CONNECTING) {
	 	req->state.status = MPC_LOWCOMM_LCP_PEND;
		req->flags |= LCP_REQUEST_SEND_CTRL; /* to delete ctrl msg upon completion */
                if (lcp_pending_create(ep->ctx->pend_send_req, req, 
                                       req->msg_id) == NULL) {
			rc = MPC_LOWCOMM_ERROR;
		}
		mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
				 req->send.tag.dest, msg_id);
		return rc;
	}

        rc = lcp_send_start(ep, req);

	return rc;
}

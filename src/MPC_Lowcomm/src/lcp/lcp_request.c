#include "lcp_request.h"
#include "lcp_ep.h"
#include "lcp_pending.h"
#include "lcp_context.h"

#include <sctk_alloc.h>

int lcp_request_create(lcp_request_t **req_p)
{
	lcp_request_t *req;
	req = sctk_malloc(sizeof(lcp_request_t));
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv request.");
		return MPC_LOWCOMM_ERROR;
	}
	memset(req, 0, sizeof(lcp_request_t));

	req->state.status = MPC_LOWCOMM_LCP_TAG;

	*req_p = req;

	return MPC_LOWCOMM_SUCCESS;
}

int lcp_request_init_unexp_ctnr(lcp_unexp_ctnr_t **ctnr_p, void *data, 
				size_t length, unsigned flags)
{
	lcp_unexp_ctnr_t *ctnr;

	ctnr = sctk_malloc(sizeof(lcp_unexp_ctnr_t) + length);
	if (ctnr == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv "
				       "container.");
		return MPC_LOWCOMM_ERROR;
	}

	ctnr->length = length;
	ctnr->flags = 0;
	ctnr->flags |= flags;

	memcpy(ctnr + 1, data, length);

	*ctnr_p = ctnr;
	return MPC_LOWCOMM_SUCCESS;
}

void lcp_request_update_state(lcp_request_t *req, size_t length)
{
	req->state.remaining -= length;
	req->state.offset += length;

	if (req->state.remaining == 0)
		req->state.status = MPC_LOWCOMM_LCP_DONE;
}

int lcp_request_complete(lcp_request_t *req)
{
	mpc_common_debug("LCP: complete req=%p, comm_id=%llu, msg_id=%llu, "
			 "seqn=%d", req, req->send.tag.comm_id, req->msg_id, 
			 req->msg_number);

        //FIXME: modifying mpc request here breaks modularity
        if (req->flags & LCP_REQUEST_RECV_TRUNC)
                req->request->truncated = 1;

	if (req->flags & LCP_REQUEST_MPI_COMPLETE) {
		assert(req->request);
                //FIXME: calling request completion here breaks modularity
		req->request->request_completion_fn(req->request);
	}

	if (req->flags & (LCP_REQUEST_SEND_FRAG |
			  LCP_REQUEST_SEND_CTRL)) {
                lcp_pending_delete(req->send.ctx->pend_send_req,
                                   req->msg_id);
	} else if (req->flags & LCP_REQUEST_RECV_FRAG) {
                lcp_pending_delete(req->recv.ctx->pend_recv_req,
                                   req->msg_id);
	} 

	sctk_free(req);
	return MPC_LOWCOMM_SUCCESS;
}
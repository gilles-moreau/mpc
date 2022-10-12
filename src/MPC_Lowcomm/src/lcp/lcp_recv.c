#include "sctk_alloc.h"

#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_proto.h"
#include "lcp_tag_offload.h" 

int lcp_recv(lcp_context_h ctx, mpc_lowcomm_request_t *request,
	     void *buffer)
{
	int rc;
	lcp_unexp_ctnr_t *match;
	sctk_rail_info_t *iface;
	lcp_request_t *req;

	LCP_CONTEXT_LOCK(ctx);

	rc = lcp_request_create(&req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create request.");
		return MPC_LOWCOMM_ERROR;
	}
	lcp_request_init_recv(req, request, ctx, buffer, request->header.msg_size, 0);
        req->flags |= LCP_REQUEST_MPI_COMPLETE;

	iface = ctx->resources[ctx->priority_rail].iface;
	if (LCR_IFACE_IS_TM(iface)) {
		req->flags |= LCP_REQUEST_OFFLOADED;
		rc = lcp_recv_tag_offload_post(req, iface);

		LCP_CONTEXT_UNLOCK(ctx);

		return rc;
	}

	match = lcp_match_umq(ctx->umq_table,
			      req->recv.tag.comm_id,
			      req->recv.tag.tag,
			      req->recv.tag.src);
	if (match == NULL) {
		lcp_append_prq(ctx->prq_table, req,
			       req->recv.tag.comm_id,
			       req->recv.tag.tag,
			       req->recv.tag.src);

		LCP_CONTEXT_UNLOCK(ctx);
		return MPC_LOWCOMM_SUCCESS;
	}

	//NOTE: unlock context to enable endpoint creation in rndv_matched
	LCP_CONTEXT_UNLOCK(ctx);
	if (match->flags & LCP_RECV_CONTAINER_UNEXP_RNDV) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);
		rc = lcp_rndv_matched(ctx, req, (lcp_rndv_hdr_t *)(match + 1));
	} else if (match->flags & LCP_RECV_CONTAINER_UNEXP_TAG) {
		mpc_common_debug("LCP: matched tag unexp req=%p, flags=%x", req, 
				 match->flags);
		rc = lcp_tag_matched(req, (lcp_tag_hdr_t *)(match + 1), 
				     match->length);
	} else {
		mpc_common_debug_error("LCP: unkown match flag=%x.", match->flags);
		rc = MPC_LOWCOMM_ERROR;
	}

	sctk_free(match);


	return rc;
}
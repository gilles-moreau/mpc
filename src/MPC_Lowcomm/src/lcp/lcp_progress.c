#include "lcp.h"

#include "sctk_rail.h"

#include "lcp_context.h"
#include "lcp_request.h"

#include <uthash.h>

int lcp_progress(lcp_context_h ctx)
{
	int i, rc = MPC_LOWCOMM_SUCCESS;

	for (i=0; i<ctx->num_resources; i++) {
		sctk_rail_info_t *iface = ctx->resources[i].iface;
                iface->iface_progress(iface);
	}

        //FIXME: no request progress needed for am or tag apparently but
        //       this is to be confirmed.
	//lcp_pending_entry_t *entry_e, *entry_tmp;
        //HASH_ITER(hh, ctx->pend->table, entry_e, entry_tmp) {

        //        lcp_request_t *req = entry_e->req;
        //        rc = lcp_request_send(req);
        //        break;
        //}

	return rc;
}

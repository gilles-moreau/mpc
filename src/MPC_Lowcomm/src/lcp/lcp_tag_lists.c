#include <comm.h>
#include <utlist.h>
#include <sctk_alloc.h>

#include "lcp_tag_lists.h"


/*******************************************************
 * PRQ Posted Receive Queue
 ******************************************************/
 /**
  * @brief Cancel and delete a request from the PRQ.
  * 
  * @param list PRQ
  * @param req request to delete
  * @return int 
  */
int lcp_prq_cancel(lcp_mtch_prq_list_t *list, void *req)
{
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		if (req == elem->value) {
			DL_DELETE(list->list, elem);
			mpc_mempool_free(NULL, elem);
			list->size--;
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag in the PRQ.
 * 
 * @param list PRQ
 * @param tag tag of the target request
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_prq_find(lcp_mtch_prq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag & tag) || (!elem->tmask && tag >= 0)) &&
			((elem->src & elem->smask) == (peer & elem->smask));
		if (result) {
			return elem->value;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag and delete it from the PRQ.
 * 
 * @param list PRQ
 * @param tag tag of the request to delete
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_prq_find_dequeue(lcp_mtch_prq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag == tag) || (!elem->tmask && tag >= 0)) &&
			((elem->src & elem->smask) == (peer & elem->smask));
		if (result) {
			DL_DELETE(list->list, elem);	
			list->size--;
			void * ret = elem->value;
			mpc_mempool_free(NULL, elem);
			return ret;
		}
	}
	return 0;
}

//FIXME: add malloc check
/**
 * @brief Add a request to the PRQ.
 * 
 * @param list PRQ
 * @param payload content of the request
 * @param tag tag of the request (can be MPC_ANY_TAG)
 * @param source request source 
 */
void lcp_prq_append(lcp_mtch_prq_list_t *list, void *payload, int tag, uint64_t source)
{
	int mask_tag;
	uint64_t mask_src;

	if((int)source == MPC_ANY_SOURCE) {
		mask_src = 0;
	} else {
		mask_src = ~0;
	}

	if(tag == MPC_ANY_TAG) {
		mask_tag = 0;
	} else {
		mask_tag = ~0;
	}
	
	lcp_mtch_prq_elem_t *elem = mpc_mempool_alloc(&list->elem_pool);

	elem->tag = tag;
	elem->tmask = mask_tag;
	elem->src = source;
	elem->smask = mask_src;
	elem->value = payload;	
	list->size++;

	DL_APPEND(list->list, elem);
}

//FIXME: add malloc check
/**
 * @brief Allocate and initialize a PRQ.
 * 
 * @return lcp_mtch_prq_list_t* initialized request
 */
lcp_mtch_prq_list_t *lcp_prq_init()
{
	lcp_mtch_prq_list_t *list = sctk_malloc(
			sizeof(lcp_mtch_prq_list_t));
	memset(list, 0, sizeof(lcp_mtch_prq_list_t));

	list->size = 0;
	mpc_mempool_init(&list->elem_pool, 10, 1024, sizeof(lcp_mtch_prq_elem_t), sctk_malloc, sctk_free);

	mpc_common_spinlock_init(&list->lock, 0);
	return list;
}
/**
 * @brief Destroy a PRQ.
 * 
 * @param list PRQ to destroy
 */
void lcp_mtch_prq_destroy(lcp_mtch_prq_list_t *list)
{
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		DL_DELETE(list->list, elem);
		mpc_mempool_free(NULL, elem);
	}

	mpc_mempool_empty(&list->elem_pool);

	sctk_free(list->list);
}

/*******************************************************
 * UMQ Unexpected Message Queue
 ******************************************************/
/**
 * @brief Cancel and delete a request from an UMQ
 * 
 * @param list UMQ
 * @param req reqeuest to delete
 * @return int LCP_SUCCESS in case of success
 */
int lcp_umq_cancel(lcp_mtch_umq_list_t *list, void *req)
{
	lcp_mtch_umq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		if (req == elem->value) {
			DL_DELETE(list->list, elem);
			mpc_mempool_free(NULL, elem);
			list->size--;
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag in an UMQ.
 * 
 * @param list UMQ
 * @param tag tag of the target request
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_umq_find(lcp_mtch_umq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_umq_elem_t *elem, *tmp;

	int tmask = ~0;
	uint64_t smask = ~0;
	if (tag == MPC_ANY_TAG) {
		tmask = 0;
	}

	if ((int)peer == MPC_ANY_SOURCE) {
		smask = 0;
	}

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag == tag) || (!tmask && elem->tag >= 0)) &&
			((elem->src & smask) == (peer & smask));
		if (result) {
			void * ret = elem->value;
			mpc_mempool_free(NULL, elem);
			return ret;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag in an UMQ and delete it.
 * 
 * @param list UMQ
 * @param tag tag of the request to delete
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_umq_find_dequeue(lcp_mtch_umq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_umq_elem_t *elem, *tmp;

	int tmask = ~0;
	uint64_t smask = ~0;
	if (tag == MPC_ANY_TAG) {
		tmask = 0;
	}

	if ((int)peer == MPC_ANY_SOURCE) {
		smask = 0;
	}

	DL_FOREACH_SAFE(list->list, elem, tmp){
                result = ((elem->tag == tag) || (!tmask && elem->tag >= 0)) &&
                        ((elem->src & smask) == (peer & smask));
		if (result) {
			DL_DELETE(list->list, elem);	
			list->size--;
			return elem->value;
		}
	}
	return 0;
}

//FIXME: add malloc check
/**
 * @brief Add a request in an UMQ.
 * 
 * @param list UMQ
 * @param payload content of the request
 * @param tag tag of the request (can be MPC_ANY_TAG)
 * @param source request source 
 */
void lcp_umq_append(lcp_mtch_umq_list_t *list, void *payload, int tag, uint64_t source)
{
	lcp_mtch_umq_elem_t *elem = mpc_mempool_alloc(&list->elem_pool);

	elem->tag = tag;
	elem->src = source;
	elem->value = payload;	
	list->size++;

	DL_APPEND(list->list, elem);
}

/**
 * @brief Allocate and initialize an UMQ.
 * 
 * @return lcp_mtch_prq_list_t* initialized request
 */
lcp_mtch_umq_list_t *lcp_umq_init()
{
	lcp_mtch_umq_list_t *list = sctk_malloc(
			sizeof(lcp_mtch_umq_list_t));
	memset(list, 0, sizeof(lcp_mtch_umq_list_t));

	mpc_mempool_init(&list->elem_pool, 10, 1024, sizeof(lcp_mtch_umq_elem_t), sctk_malloc, sctk_free);

	list->size = 0;
	mpc_common_spinlock_init(&list->lock, 0);
	return list;
}

/**
 * @brief Destroy an UMQ
 * 
 * @param list UMQ to destroy
 */
void lcp_mtch_umq_destroy(lcp_mtch_umq_list_t *list)
{
	lcp_mtch_umq_elem_t *elem = NULL, *tmp = NULL;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		DL_DELETE(list->list, elem);
		mpc_mempool_free(NULL, elem);
	}
	sctk_free(list->list);
}

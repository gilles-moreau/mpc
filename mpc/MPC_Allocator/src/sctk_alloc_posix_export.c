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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include <stdlib.h>
#include <errno.h>
#include "sctk_alloc_posix.h"
#include "sctk_alloc_debug.h"

//optional header
#ifdef MPC_Common
#include "sctk.h"
#endif

/************************* FUNCTION ************************/
static __inline__ bool sctk_alloc_is_power_of_two(sctk_size_t size)
{
	return ((size != 0) && !(size & (size-1)));
}

/************************* FUNCTION ************************/
void * calloc (size_t nmemb, size_t size)
{
	void * res;
	SCTK_PROFIL_START(calloc);
	res = sctk_calloc(nmemb,size);
	SCTK_PROFIL_END(calloc);
	return res;
}

/************************* FUNCTION ************************/
void * malloc (size_t size)
{
	void * res;
	SCTK_PROFIL_START(malloc);
	res = sctk_malloc(size);
	SCTK_PROFIL_END(malloc);
	return res;
}

/************************* FUNCTION ************************/
void free (void * ptr)
{
	SCTK_PROFIL_START(free);
	sctk_free(ptr);
	SCTK_PROFIL_END(free);
}

/************************* FUNCTION ************************/
void * realloc (void * ptr, size_t size)
{
	void * res;
	SCTK_PROFIL_START(realloc);
	res = sctk_realloc(ptr,size);
	SCTK_PROFIL_END(realloc);
	return res;
}

/************************* FUNCTION ************************/
int posix_memalign(void **memptr, size_t boundary, size_t size)
{
	int res;

	SCTK_PROFIL_START(posix_memalign);
	
	//limit imposed by posix_memalign linux manpage
	if (boundary % sizeof(void*) != 0 && sctk_alloc_is_power_of_two(boundary))
	{
		sctk_alloc_pwarning("memalign boundary not power of 2 or boundary not multiple of sizeof(void*).");
		res = EINVAL;
	} else {
		*memptr = sctk_memalign(boundary,size);

		//check res
		if (memptr == NULL)
			res = ENOMEM;
		else
			res = 0;
	}
	
	SCTK_PROFIL_END(posix_memalign);
	
	return res;
}

/************************* FUNCTION ************************/
void * memalign(size_t boundary,size_t size)
{
	void * res;

	SCTK_PROFIL_START(memalign);

	//limit imposed by posix_memalign linux manpage
	if (sctk_alloc_is_power_of_two(boundary))
	{
		res = sctk_memalign(boundary,size);
	} else {
		sctk_alloc_pwarning("memalign boundary not power of 2.");
		res = NULL;
	}

	SCTK_PROFIL_END(memalign);

	return res;
};

/************************* FUNCTION ************************/
void *valloc(size_t size)
{
	return memalign(SCTK_ALLOC_PAGE_SIZE,size);
}

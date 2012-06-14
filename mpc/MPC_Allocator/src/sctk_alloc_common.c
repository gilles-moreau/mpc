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
#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/mman.h>
#endif

#include "sctk_alloc_common.h"
#include <stdio.h>

/************************* PORTABILITY *************************/
#ifdef _WIN32
	#define MAP_FAILED NULL
#endif

/************************* FUNCTION ************************/
/**
 * Provide a quick wrapper to mmap. It may help for debugging and instrumentation of malloc.
 * @param addr Define the mapping address to request. NULL to not forced. Must be multiple of
 * OS page size as for mmap.
 * @param size Define the requested segement size. Must be multiple of OS page size as for mmap.
**/
#ifdef _WIN32
SCTK_STATIC void* sctk_mmap(void* addr, size_t size)
{
	void * res = NULL;
	if (addr == NULL)
		res = VirtualAlloc(NULL,size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	else
		res = VirtualAlloc(addr,size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	if (res == MAP_FAILED){
		perror("Out of memory, failed to request memory to the OS via VirtualAlloc.");
	}
	return res;
}
#else
void* sctk_mmap(void* addr, size_t size)
{
	void * res = NULL;
	if (addr == NULL)
		res = mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,-1,0);
	else
		res = mmap(addr,size,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE|MAP_FIXED,-1,0);
	if (res == MAP_FAILED)
		perror("Out of memory, failed to request memory to the OS via mmap.");
	return res;
}
#endif

/************************* FUNCTION ************************/
/**
 * Provide a quick wrapper to munmap, it may help for debugging and instrumentation of malloc.
 * @param addr Define the starting address to unmap. As for munmap, it must be multiple of OS
 * page size.
 * @param size Define the size of the segment to unmap. As for munmap, it must be multiple of
 * OS page size.
**/
#ifdef _WIN32
#else
void sctk_munmap(void * addr,size_t size)
{
	munmap(addr,size);
}
#endif

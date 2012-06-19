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
#define _GNU_SOURCE

#include "sctk_alloc_inlined.h"
#include "sctk_alloc_lock.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_light_mm_source.h"
#include "sctk_alloc_topology.h"

#ifndef _WIN32
	//options Linux specific (mremap)
	#include <unistd.h>
	#include <sys/mman.h>
#endif

//optional for NUMA
#ifdef HAVE_LIBNUMA
#include <hwloc.h>
#endif

/**
 * Maximum bloc to keep in memory source for future resuse at free time.
 * The main goal is to resduce the number of calls to mmap and avoid to pass
 * too much time in kernek zero page function.
 * The larger it is, the more it keep the memory in the process.
 * It must be multiples of SCTK_MACRO_BLOC_SIZE.
 * With huge pvalues it tend to consume as much memory than TCMalloc.
**/
#define SCTK_ALLOC_MACRO_BLOC_REUSE_THREASHOLD (1*SCTK_MACRO_BLOC_SIZE)

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
/**
 * Setup the nodset structure of hwloc to request NUMA binding of the pages of the
 * managed by the current memory source.
 * @param numa_node Define the specific NUMA node on which to bind.
**/
SCTK_STATIC  hwloc_nodeset_t sctk_alloc_mm_source_light_init_nodeset(int numa_node)
{
	//allocate node set
	hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();

	//set value
	hwloc_bitmap_set(nodeset,numa_node);

	//return it
	return nodeset;
}
#endif

/************************* FUNCTION ************************/
/**
 * Function used to setup the light memory source.
 * @param source Define the memory source to init.
 * @param numa_node Define the NUMA node on which to bind the memory source pages. Use -1 for none.
 * @param mode Enable some flags defined in sctk_alloc_mm_source_light to configure the memory source.
 * @todo Define a constante instead of -1 for non defined NUMA node.
**/
void sctk_alloc_mm_source_light_init(struct sctk_alloc_mm_source_light * source,int numa_node,enum sctk_alloc_mm_source_light_flags mode)
{
	//errors
	assert(source != NULL);
	assert(numa_node >= 0 || numa_node == SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE);

	//basic setup
	sctk_alloc_mm_source_base_init(&source->source);
	
	//setup functions
	source->source.cleanup = sctk_alloc_mm_source_light_cleanup;
	source->source.free_memory = sctk_alloc_mm_source_light_free_memory;
	source->source.request_memory = sctk_alloc_mm_source_light_request_memory;
	#ifdef HAVE_MREMAP
	source->source.remap = sctk_alloc_mm_source_light_remap;
	#endif
	#ifdef HAVE_LIBNUMA
	source->nodeset = NULL;
	#endif

	//init spinlock
	sctk_alloc_spinlock_init(&source->spinlock,PTHREAD_PROCESS_PRIVATE);

	//if numa is disable we don't know anything, so remove info
	#ifndef HAVE_LIBNUMA
	if (mode |= SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT)
		sctk_alloc_pwarning("Caution, you request NUMA strict but allocator was compiled without NUMA support.");
	mode &= ~SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT;
	numa_node = SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE;
	#else
	if (mode |= SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT && numa_node != -1)
		source->nodeset = sctk_alloc_mm_source_light_init_nodeset(numa_node);
	else
		source->nodeset = NULL;
	#endif

	//mark the mode and numa node
	source->numa_node = numa_node;
	source->mode = mode;

	//setup blocs counter to know if we can safetely delete. (more for debug)
	source->counter = 0;

	//start with empty local cache
	source->cache = NULL;
}


/************************* FUNCTION ************************/
/**
 * Register a macro bloc into the memory source for future reuse. The bloc is
 * inserted at the beginning of the fist.
 * @param light_source Define the memory source in which to register.
 * @param free_bloc Define the free_bloc to register in the list.
 * @todo Try to use an atomic list.
**/
SCTK_STATIC void sctk_alloc_mm_source_light_reg_in_cache(struct sctk_alloc_mm_source_light * light_source,struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc)
{
	//taks the lock
	sctk_alloc_spinlock_lock(&light_source->spinlock);

	//insert in list at beginin pos
	free_bloc->next = light_source->cache;
	light_source->cache = free_bloc;

	//taks the lock
	sctk_alloc_spinlock_unlock(&light_source->spinlock);
}

/************************* FUNCTION ************************/
/**
 * Setup the header of a macro bloc from a raw memory bloc.
 * @param ptr Define the base pointer of the macro bloc to setup.
 * @param size Define the size of the macro_bloc to setup.
**/
SCTK_STATIC struct sctk_alloc_mm_source_light_free_macro_bloc * sctk_alloc_mm_source_light_setup_free_macro_bloc(void * ptr,sctk_size_t size)
{
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc = ptr;
	free_bloc->next = NULL;
	free_bloc->size = size;
	return free_bloc;
}

/************************* FUNCTION ************************/
/**
 * Convert a macro bloc into a free macro bloc. It mostly update the header structure.
 * @param macro_bloc Define the macro bloc to mark as free macro_bloc.
**/
SCTK_STATIC struct sctk_alloc_mm_source_light_free_macro_bloc * sctk_alloc_mm_source_light_to_free_macro_bloc(struct sctk_alloc_macro_bloc * macro_bloc)
{
	return sctk_alloc_mm_source_light_setup_free_macro_bloc(macro_bloc,sctk_alloc_get_chunk_header_large_size(&macro_bloc->header));
}

/************************* FUNCTION ************************/
/**
 * Permit to the user to insert manually some memory segment into the memory source.
 * In script mode, hwlock will be used to force the binding on the given memory segment.
 * @param light_source The memory source in which to insert the segment.
 * @param base The base address of the segment to insert.
 * @param size The size of the memory segment to insert.
**/
SCTK_STATIC void sctk_alloc_mm_source_light_insert_segment(struct sctk_alloc_mm_source_light* light_source, void* base, sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc = base;

	//errors
	assert(light_source != NULL);
	assert(base != NULL);
	assert(size > 0);

	//some runtime errors
	if (base == NULL || size == 0)
		return;

	//Check requirements on params, need page aligned address, size smaller or equals to SCTK_ALLOC_MACRO_BLOC_SIZE
	assume((sctk_addr_t)base % SCTK_ALLOC_PAGE_SIZE == 0,"Buffer base adderss must be aligned to page size.");
	assume(size % SCTK_MACRO_BLOC_SIZE == 0,"Buffer size must be aligned to page size.");

	//if need to force binding, as we didn't now the origin, call meme binding method
	#ifdef HAVE_LIBNUMA
	if (light_source->mode | SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT && light_source->numa_node != SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE)
		sctk_alloc_force_segment_binding(light_source,base,size,light_source->numa_node);
	#endif //HAVE_LIBNUMA

	//setup header
	free_bloc->size = size;
	free_bloc->next = NULL;

	//insert
	sctk_alloc_mm_source_light_reg_in_cache(light_source,free_bloc);
}

/************************* FUNCTION ************************/
/**
 * Force the NUMA binding of a particular segment.
 * @param light_source Define the memory source for which to bind the segment.
 * @param base The base address of the segment to bind.
 * @param size The size of the segment to bind.
 * @param numa_node The NUMA node on which to bind (unused, to remote).
 * @todo Remove numa_node param.
**/
#ifdef HAVE_LIBNUMA
SCTK_STATIC void sctk_alloc_force_segment_binding(struct sctk_alloc_mm_source_light * light_source,void* base, sctk_size_t size,int numa_node)
{
	//vars
	int res;

	//errors
	assert(base != NULL);
	assert(size > SCTK_ALLOC_PAGE_SIZE);
	assert(size % SCTK_ALLOC_PAGE_SIZE == 0);
	assert(numa_node >= 0);

	//use hwloc to bind the segment
	if (light_source->nodeset != NULL)
		res = hwloc_set_area_membind_nodeset(sctk_get_topology_object(),base,size,light_source->nodeset,HWLOC_MEMBIND_BIND ,HWLOC_MEMBIND_THREAD);
}
#endif //HAVE_LIBNUMA

/************************* FUNCTION ************************/
/**
 * This is more for unit tests, this function can be called to cleanup the memory source.
 * It will return all free macro blocs to the system.
**/
SCTK_STATIC void sctk_alloc_mm_source_light_cleanup(struct sctk_alloc_mm_source* source)
{
	//vars
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc;
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc_next;
	struct sctk_alloc_mm_source_light * light_source = (struct sctk_alloc_mm_source_light *)source;

	//errors
	assert(source != NULL);

	//take the lock
	sctk_alloc_spinlock_lock(&light_source->spinlock);

	//check counters to know if all blocs are returned to the mm source.
	assume(light_source->counter == 0, "Invalid counter status, must be 0 to call cleanup.");

	//loop on the blocs in cache to free them
	free_bloc = light_source->cache;
	while (free_bloc != NULL)
	{
		free_bloc_next = free_bloc->next;
		sctk_munmap(free_bloc,free_bloc->size);
		free_bloc = free_bloc_next;
	}

	//free the nodeset
	#ifdef HAVE_LIBNUMA
	if (light_source->nodeset != NULL)
		hwloc_bitmap_free(light_source->nodeset);
	#endif

	//never free the lock as this structure may go away
}

/************************* FUNCTION ************************/
/**
 * Convert a macro bloc into a free one. It only update the header of the bloc.
**/
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_to_macro_bloc(struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc = (struct sctk_alloc_macro_bloc * )free_bloc;
	
	if (free_bloc != NULL)
	{
		sctk_alloc_get_large(sctk_alloc_setup_chunk(free_bloc,free_bloc->size,NULL));
		macro_bloc->chain = NULL;
	}

	return macro_bloc;
}

/************************* FUNCTION ************************/
/**
 * Permit to reuse some macro blocs which didn't have the good size for memory requests
 * by using mremap() it permit to use the non used pages without needs of too much system
 * zero pages.
**/
#ifdef HAVE_MREMAP
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_to_macro_bloc_resized(struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc,sctk_size_t size)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc;

	//cases
	if (free_bloc == NULL)
	{
		return NULL;
	} else if (free_bloc->size == size) {
		return sctk_alloc_mm_source_light_to_macro_bloc(free_bloc);
	} else {
		macro_bloc = sctk_alloc_mm_source_light_to_macro_bloc(free_bloc);
		SCTK_PDEBUG("Reuse but internal remap %p -> %llu -> %llu",macro_bloc,macro_bloc->header.size,size);
		macro_bloc = sctk_alloc_mm_source_light_remap(macro_bloc,size);
		return macro_bloc;
	}
}
#endif

/************************* FUNCTION ************************/
/**
 * Search in the cache a page with a good size. If not found, it return NULL.
 * @param light_source Define the memory source in which to search.
 * @param size Define the requested size (accounting the header).
**/
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_mm_source_light_find_in_cache(struct sctk_alloc_mm_source_light * light_source,sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc = NULL;
	struct sctk_alloc_mm_source_light_free_macro_bloc * prev_bloc = NULL;
	#ifdef HAVE_MREMAP_FOR_FIND_IN_CACHE
	struct sctk_alloc_mm_source_light_free_macro_bloc * non_exact = NULL;
	struct sctk_alloc_mm_source_light_free_macro_bloc * non_exact_prev = NULL;
	#endif //HAVE_MREMAP

	//errors
	assert(light_source != NULL);
	assert(size > 0);

	//trivial case, we know that we didn't start multi-macro bloc chunks in the cache so return directory
	//if (size > SCTK_MACRO_BLOC_SIZE)
	//	return NULL;

	//take the lock
	sctk_alloc_spinlock_lock(&light_source->spinlock);

	//loop in the list the find the first on which is good.
	free_bloc = light_source->cache;
	prev_bloc = NULL;
	while (free_bloc != NULL && free_bloc->size != size)
	{
		#ifdef HAVE_MREMAP_FOR_FIND_IN_CACHE
		if (non_exact == NULL)
		{
			non_exact = free_bloc;
			non_exact_prev = prev_bloc;
		} else if (abs(size - free_bloc->size) < abs(size - non_exact->size)) {
			non_exact = free_bloc;
			non_exact_prev = prev_bloc;
		}
		#endif
		prev_bloc = free_bloc;
		free_bloc = free_bloc->next;
	}

	//if can't find exact, try non exact and resize it to reuse pages
	#ifdef HAVE_MREMAP_FOR_FIND_IN_CACHE
	if (free_bloc == NULL)
	{
		free_bloc = non_exact;
		prev_bloc = non_exact_prev;
	}
	#endif

	//update counter and remove from list
	if (free_bloc != NULL)
	{
		if (prev_bloc == NULL)
			light_source->cache = free_bloc->next;
		else
			prev_bloc->next = free_bloc->next;
		light_source->counter++;
	}

	//unlock
	sctk_alloc_spinlock_unlock(&light_source->spinlock);

	//return
	#ifdef HAVE_MREMAP_FOR_FIND_IN_CACHE
	return sctk_alloc_mm_source_light_to_macro_bloc_resized(free_bloc,size);
	#else
	return sctk_alloc_mm_source_light_to_macro_bloc(free_bloc);
	#endif
}

/************************* FUNCTION ************************/
/**
 * Implement the request_memory function required to implement a memory source.
**/
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_mm_source_light_request_memory(struct sctk_alloc_mm_source* source, sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_light * light_source = (struct sctk_alloc_mm_source_light *)source;
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;

	//errors
	assert(source != NULL);
	assert(size > 0);
	assert(size % SCTK_MACRO_BLOC_SIZE == 0 );

	//runtime errors
	if (size == 0)
		return NULL;

	//try to find memory in source local cache if smaller that one macro bloc (we now that we didn't store such blocs here)
	if (size <= SCTK_ALLOC_MACRO_BLOC_REUSE_THREASHOLD)
	{
		macro_bloc = sctk_alloc_mm_source_light_find_in_cache(light_source,size);
		SCTK_PDEBUG("LMMSRC %p : Try to reuse macro bloc %p -> %llu",source,macro_bloc,size);
	}

	//allocate a new one
	if (macro_bloc == NULL)
		macro_bloc = sctk_alloc_mm_source_light_mmap_new_segment(light_source,size);

	//warn if out of memory
	#ifndef NDEBUG
	if (macro_bloc == NULL)
		SCTK_PDEBUG("Memory source get out of memory and can't request more to system.");
	#endif

	//return if
	return macro_bloc;
}

/************************* FUNCTION ************************/
/**
 * Internal function to request new segments if can't reuse one.
 * @todo add support of NUMA binding.
**/
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_mmap_new_segment(struct sctk_alloc_mm_source_light* light_source,sctk_size_t size)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;

	//errors
	assert(size % SCTK_ALLOC_PAGE_SIZE == 0);

	//call mmap to get a macro blocs
	macro_bloc = sctk_mmap(NULL,size);
	SCTK_PDEBUG("LMMSRC : Map new macro_bloc %p -> %llu",macro_bloc,size);

	//setup header
	macro_bloc = sctk_alloc_setup_macro_bloc(macro_bloc,size);

	//increment counters
	sctk_alloc_spinlock_lock(&light_source->spinlock);
	light_source->counter++;
	sctk_alloc_spinlock_unlock(&light_source->spinlock);

	//return
	return macro_bloc;
}

/************************* FUNCTION ************************/
/**
 * Free a macro bloc comming from the allocator. Depending on a threashold, the memory is returned to the system or
 * kept for a future reuse.
**/
SCTK_STATIC void sctk_alloc_mm_source_light_free_memory(struct sctk_alloc_mm_source * source,struct sctk_alloc_macro_bloc * bloc)
{
	//vars
	struct sctk_alloc_mm_source_light * light_source = (struct sctk_alloc_mm_source_light *)source;
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc;

	//errors
	assert(source != NULL);
	assert(bloc != NULL);

	//free bloc
	free_bloc = sctk_alloc_mm_source_light_to_free_macro_bloc(bloc);

	//if larger than basic macro bloc size, return to system immediately
	if (free_bloc->size > SCTK_ALLOC_MACRO_BLOC_REUSE_THREASHOLD)
	{
		SCTK_PDEBUG("LMMSRC %p : Do munmap %p -> %llu",source,free_bloc,free_bloc->size);
		sctk_munmap(free_bloc,free_bloc->size);
	} else {
		SCTK_PDEBUG("LMMSRC %p : Keep bloc for future %p -> %llu",source,free_bloc,free_bloc->size);
		sctk_alloc_mm_source_light_reg_in_cache(light_source,free_bloc);
	}

	//increment counters
	sctk_alloc_spinlock_lock(&light_source->spinlock);
	light_source->counter--;
	sctk_alloc_spinlock_unlock(&light_source->spinlock);
}

/************************* FUNCTION ************************/
/**
 * Function used to resize a macro bloc. This method is used by the allocation chain to implement
 * realloc of huge segments.
 * @param macro_bloc Define the macro bloc to resize.
 * @param size Define the new size of the segment (accounting the header).
**/
#ifdef HAVE_MREMAP
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_remap(struct sctk_alloc_macro_bloc * macro_bloc,sctk_size_t size)
{
	//errors
	assert(macro_bloc != NULL);
	assert(size % SCTK_ALLOC_PAGE_SIZE == 0);
	assert((sctk_addr_t)macro_bloc % SCTK_ALLOC_PAGE_SIZE == 0);

	//use mremap
	SCTK_PDEBUG("LMMSRC : Use mremap : %p -> %llu",macro_bloc,size);
	macro_bloc = mremap(macro_bloc,macro_bloc->header.size,size,MREMAP_MAYMOVE);

	if (macro_bloc == MAP_FAILED)
	{
		perror("Error while using mremap in sctk_realloc()");
		abort();
	}

	//setup header
	macro_bloc = sctk_alloc_setup_macro_bloc(macro_bloc,size);

	//return it
	return macro_bloc;
}
#endif


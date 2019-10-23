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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_CHUNK_H
#define SCTK_ALLOC_CHUNK_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#ifdef MPC_Common
#include "mpc_common_types.h"
#else
#include <stdint.h>
#endif
/********************************** ENUM ************************************/
/**
 * Constants to define the type of current block, currently, only small (<=256o) or large.
**/
enum sctk_alloc_chunk_type
{
	/** This is the default chunk type. **/
	SCTK_ALLOC_CHUNK_TYPE_LARGE      = 1,
	/** Used for padding (posix_memalign...). **/
	SCTK_ALLOC_CHUNK_TYPE_PADDED     = 2
};

/********************************** ENUM ************************************/
/** Define the state of current bloc, free or allocated. **/
enum sctk_alloc_chunk_state
{
	/** Mark the chunk as free for reuse. **/
	SCTK_ALLOC_CHUNK_STATE_FREE      = 1,
	/** The chunk is allocated by application, not ready for reuse. **/
	SCTK_ALLOC_CHUNK_STATE_ALLOCATED = 2
};

/********************************** ENUM ************************************/
/** Mapping enum for macro blocs. **/
enum sctk_alloc_mapping_state
{
	/** The macro bloc is unmapped and need to be remapped for reuse. **/
	SCTK_ALLOC_BLOC_UNMAPPED         = 0,
	/** The macro bloc is fully mapped and can be reused without any actions. **/
	SCTK_ALLOC_BLOC_MAPPED           = 1,
	/** The macro bloc is partially mapped so we need to use mmap to fully map it. **/
	SCTK_ALLOC_BLOC_PARTIALY         = 2
};

/******************************** STRUCTURE *********************************/
/**
 * Bloc common header between small and large bloc. It define the type and status of current bloc.
 * With this values, we can determine the position of the size value and read it.
**/
struct sctk_alloc_chunk_info
{
	/** Type of bloc (large of small for now) **/
	unsigned char type:2;
	/** Status of current bloc, free or allocated. **/
	unsigned char state:2;
	/** Unused bits, filled with a magic constant to be used as a canary for now and detect bugs. **/
	unsigned char unused_magik:4;
};

/******************************** STRUCTURE *********************************/
/** Chunk header for large blocs (>256o). **/
struct sctk_alloc_chunk_header_large
{
	#ifdef _WIN32
	//For windows we cannot use bitfields with 64bit wide members, in work inly with members
	//shorten or equal to 32bit.
	unsigned char size[7];
	unsigned char addr;
	unsigned char prevSize[7];
	struct sctk_alloc_chunk_info info;
	#else
	/** Size of the bloc content (counting the header size). **/
	uint64_t size:56;
	/** Address of the bloc, this is a sort of canary to detect bugs. **/
	unsigned char addr;
	/** Size of previous bloc. **/
	uint64_t prevSize:56;
	/**
	 * Common part of the header, if is mostly used to determine the status and type of blocs
	 * It must be at the end of the header for each header type.
	**/
	struct sctk_alloc_chunk_info info;
	#endif
};

/******************************** STRUCTURE *********************************/
/**
 * Chunk header for padded blocs generated by memalign/posix_memalign.
 * Such blocs are included in small or lage blocs (more large due to additional header of 8o).
 * It imply padding up to 2^56 only (64To).
 * We can made another choice with 2^24 => 16Mo with a total header size of 4o, but we ignore
 * this approach for now, it may be interesting when really using the small chunk, for large
 * we never get such small alignment (<8).
**/
struct sctk_alloc_chunk_header_padded
{
	#ifdef _WIN32
	//For windows we cannot use bitfields with 64bit wide members, in work inly with members
	//shorten or equal to 32bit.
	unsigned char padding[7];
	struct sctk_alloc_chunk_info info;
	#else
	/**
	 * Padding, it give the space between body address of real chunk and base address of
	 * sctk_alloc_chunk_header_padded struct. So to get the previous header, we just need
	 * to remove the padding from current header address, and call sctk_alloc_get_chunk().
	**/
	uint64_t padding:56;
	/**
	 * Common part of the header, if is mostly used to determine the status and type of blocs
	 * It must be at the end of the header for each header type.
	**/
	struct sctk_alloc_chunk_info info;
	#endif
};

/******************************** STRUCTURE *********************************/
/**
 * Entry of the free list. Blocs are chained by a double linked list.
 * It only work for large chunks as we can't store two pointers in the body of small chunks.
**/
struct sctk_alloc_free_chunk
{
	/** Inherit from large header. **/
	struct sctk_alloc_chunk_header_large header;
	/** Next free chunk in the related free list. **/
	struct sctk_alloc_free_chunk * next;
	/** Prev free chunk in the related free list. **/
	struct sctk_alloc_free_chunk * prev;
};

/********************************** TYPES ***********************************/

/**
 * Define a free chunk list. It is mostly represented by an empty free chunk to get a transparent
 * cycle of the double link list and avoid managing NULL pointers on borders.
**/
typedef struct sctk_alloc_free_chunk sctk_alloc_free_list_t;
/**
 * Abstract description of a chunk, is is used to manage transparently the two type of blocs (small
 * or large) in functions.
**/
typedef struct sctk_alloc_chunk_info * sctk_alloc_vchunk;

/********************************* FUNCTION *********************************/
//chunk management.
//static __inline__ sctk_alloc_vchunk sctk_alloc_get_chunk(sctk_addr_t ptr);
//static inline sctk_alloc_vchunk sctk_alloc_setup_chunk(void * ptr, sctk_size_t size, void * prev);
//static __inline__ void sctk_alloc_setup_macro_bloc(struct sctk_alloc_macro_bloc * macro_bloc);
sctk_alloc_vchunk sctk_alloc_setup_chunk_padded(sctk_alloc_vchunk chunk,
                                                sctk_size_t boundary);
sctk_size_t sctk_alloc_calc_chunk_size(sctk_size_t user_size);
sctk_size_t sctk_alloc_calc_body_size(sctk_size_t chunk_size);
void *sctk_alloc_chunk_body(sctk_alloc_vchunk vchunk);
void sctk_alloc_create_stopper(void *ptr, void *prev);
sctk_size_t sctk_alloc_align_size(sctk_size_t size, sctk_size_t align);
sctk_alloc_vchunk sctk_alloc_get_prev_chunk(sctk_alloc_vchunk chunk);
void sctk_alloc_dump_chunk(void* ptr);
void sctk_alloc_dump_chunk_obj(sctk_alloc_vchunk chunk);
//static __inline__ sctk_alloc_vchunk sctk_alloc_get_next_chunk(sctk_alloc_vchunk chunk);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_CHUNK_H */

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

#ifndef SCTK_ALLOC_H
#define SCTK_ALLOC_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#if defined(_WIN32)
	#include <windows.h>
#else	
	#include <sys/mman.h>
#endif
#include "sctk_alloc_common.h"
#include "sctk_alloc_lock.h"
#include "sctk_alloc_stats.h"
#include "sctk_alloc_spy.h"
#ifdef _MSC_VER
	#define int32_t __int32
	#define uint32_t unsigned __int32
	#define int64_t __int64
	#define uint64_t unsigned __int64
#else
#include <stdint.h>
#endif
/** Size class for the free lists. **/
extern const sctk_size_t SCTK_ALLOC_FREE_SIZES[SCTK_ALLOC_NB_FREE_LIST];

/*************************** ENUM **************************/
/**
 * C unavailability of boolean type sucks.
**/
#ifndef __cplusplus
#include <stdbool.h>
// #define bool unsigned char
// #define true 1
// #define false 0
#endif //__cplusplus

/*************************** ENUM **************************/
/**
 * Constants to define the type of current block, currently, only small (<=256o) or large.
**/
enum sctk_alloc_chunk_type
{
	SCTK_ALLOC_CHUNK_TYPE_LARGE      = 1,
	SCTK_ALLOC_CHUNK_TYPE_PADDED     = 2
};

/*************************** ENUM **************************/
/** Define the state of current bloc, free or allocated. **/
enum sctk_alloc_chunk_state
{
	SCTK_ALLOC_CHUNK_STATE_FREE      = 0,
	SCTK_ALLOC_CHUNK_STATE_ALLOCATED = 1
};

/*************************** ENUM **************************/
/** Mapping enum for macro blocs. **/
enum sctk_alloc_mapping_state
{
	SCTK_ALLOC_BLOC_UNMAPPED         = 0,
	SCTK_ALLOC_BLOC_MAPPED           = 1,
	SCTK_ALLOC_BLOC_PARTIALY         = 2
};

/*************************** ENUM **************************/
/** Define how to insert a given bloc in free list, at the end or begening. **/
enum sctk_alloc_insert_mode
{
	SCTK_ALLOC_INSERT_AT_END,
	SCTK_ALLOC_INSERT_AT_START
};

/*************************** ENUM **************************/
/** List of flags to enable options of an allocation chain. **/
enum sctk_alloc_chain_flags
{
	/** Default options. By default it is thread safe. **/
	SCTK_ALLOC_CHAIN_FLAGS_DEFAULT = 0,
	/** Enable the locks in the chain to use it from multiple threads. **/
	SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE = 1,
	/** Disable chunk merge at free time. **/
	SCTK_ALLOC_CHAIN_DISABLE_MERGE = 2
};

/************************** STRUCT *************************/
/**
 * Bloc common header between small and large bloc. It define the type and status of current bloc.
 * With this values, we can determin the position of the size value and read it.
**/
struct sctk_alloc_chunk_info
{
	/** Type of bloc (large of small for now) **/
	unsigned char type:2;
	/** Status of current bloc, free or allocated. **/
	unsigned char state:1;
	/** Unused bits, filled with a magik constant to be used as a canary for now and detect bugs. **/
	unsigned char unused_magik:5;
};

/************************** STRUCT *************************/
/** Chunk header for large blocs (>256o). **/
struct sctk_alloc_chunk_header_large
{
	#ifdef _WIN32
	uint32_t size;
	unsigned char padding0[3];
	unsigned char addr;
	uint32_t prevSize;
	unsigned char padding1[3];
	struct sctk_alloc_chunk_info info;
	
	#else
	/** Size of the bloc content (counting the header size). **/
	sctk_size_t size:56;
	/** Addr of the bloc, this is a sort of canary to dected bugs. **/
	unsigned char addr;
	/** Size of previous bloc. **/
	sctk_size_t prevSize:56;
	/**
	 * Common part of the hreader, if is mostly used to determine the status and type of blocs
	 * It must be at the end of the header for each header type.
	**/
	struct sctk_alloc_chunk_info info;
	#endif
};

/************************** STRUCT *************************/
/**
 * Chunk header for padded blocs generated by memalign/posix_memalign.
 * Such blocs are included in small or lage blocs (more large due to additional header of 8o).
 * It imply padding up to 2^56 only (64To).
 * We can made another choice with 2^24 => 16Mo with a total header size of 4o, but we ignore
 * this approach for now, it may be interesting when really using the small chunk, for large
 * we never get such small alignement (<8).
**/
struct sctk_alloc_chunk_header_padded
{
	#ifdef _WIN32
	uint32_t padding;
	unsigned char padding0[3];
	struct sctk_alloc_chunk_info info;
	#else
	/**
	 * Padding, it give the space between body addr of real chunk and base addr of
	 * sctk_alloc_chunk_header_padded struct. So to get the previous header, we just need
	 * to remove the padding from current header addr, and call sctk_alloc_get_chunk().
	**/
	sctk_size_t padding:56;
	/**
	 * Common part of the hreader, if is mostly used to determine the status and type of blocs
	 * It must be at the end of the header for each header type.
	**/
	struct sctk_alloc_chunk_info info;
	#endif
};

/************************** STRUCT *************************/
/**
 * Abstract description of a chunk, is is used to manage tranparently the two type of blocs (small
 * or large) in functions.
**/
typedef struct sctk_alloc_chunk_info * sctk_alloc_vchunk;

/************************** STRUCT *************************/
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

/************************** STRUCT *************************/
/**
 * A thread pool is created for each active threads and will be pluged on top of a common memory
 * source. It manage the free small blocs ans exchange with the memory source by "pages". Each
 * "page" is splited to get the meomry chunk allocated by the user.
 * 
 * A thread pool can also be used to generated a user managed allocator chain by providing a static
 * memory source based on a specific segement allocated by the user. We only provide the chunk
 * management to avoid the user to handle bloc lists.... which is the work of an allocator.
**/
struct sctk_thread_pool
{
	/**
	 * Lists of free element managed by the thread pool. All elements are place in specific lists
	 * depending on their size to be quicly find (best fit method).
	**/
	struct sctk_alloc_free_chunk free_lists[SCTK_ALLOC_NB_FREE_LIST];
	/**
	 * Store free list empty status as a bitmap, it permit a quicker search for allocation when
	 * we got only a large bloc in the last one which is the default at start time of just after
	 * memory refilling.
	**/
	bool free_list_status[SCTK_ALLOC_NB_FREE_LIST];
	/**
	 * Define the size of each class of free blocs, must be terminated by -1.
	 * and must must contain at least SCTK_ALLOC_NB_FREE_LIST elements.
	**/
	const sctk_size_t * alloc_free_sizes;
	/** Define the number of entries in free lists (must be lower than SCTK_ALLOC_NB_FREE_LIST). **/
	short int nb_free_lists;
};

/************************** STRUCT *************************/
/**
 * Define a macro bloc which serve for exchange between the thread pool and the memory source.
 * It reprensent the minimal bloc size which can be requested to the system by the memory source
 * via mmap.
 * Currently, the memory source manage the free list of macro bloc direcly by reusing the thread
 * pool functions, this is why this header start with a standard chunk header.
**/
struct sctk_alloc_macro_bloc
{
	/** Inherit from large header. **/
	struct sctk_alloc_chunk_header_large header;
	/** Pointer to the alloc chain which manage this bloc **/
	struct sctk_alloc_chain * chain;
	/** Some padding to maintain multiples**/
	char padding[8];
};

/************************** STRUCT *************************/
/**
 * Define a free macro bloc, keep same common header part, but contain extra informations
 * to be placed in free list and to know the mapping state.
**/
struct sctk_alloc_free_macro_bloc
{
	struct sctk_alloc_free_chunk header;
	enum sctk_alloc_mapping_state maping;
};

/************************** STRUCT *************************/
/**
 * Define a memory source which will be used to manage (and cache) memory macro blocs exchange
 * between the thread pool of the system. It can also be used to provide base for management of
 * a user memory segment.
 * This structure mainly provide function pointers to select the required function implementation
 * for init, request memory, free memory and final cleanup.
**/
struct sctk_alloc_mm_source
{
	struct sctk_alloc_macro_bloc * (*request_memory)(struct sctk_alloc_mm_source * source,sctk_size_t size);
	void (*free_memory)(struct sctk_alloc_mm_source * source,struct sctk_alloc_macro_bloc * bloc);
	void (*cleanup)(struct sctk_alloc_mm_source * source);
	struct sctk_alloc_macro_bloc * (*remap)(struct sctk_alloc_macro_bloc * old_macro_bloc,sctk_size_t new_size);
};

/************************** STRUCT *************************/
/**
 * A simple memory source which use mmap to allocate memory. It will force mmap alignement to a
 * constant size : SCTK_MACRO_BLOC_SIZE to maintain macro bloc header alignement which permit
 * to find them quicly from a specific related chunk.
**/
struct sctk_alloc_mm_source_default
{
	struct sctk_alloc_mm_source source;
	void * heap_addr;
	sctk_size_t heap_size;
	struct sctk_thread_pool pool;
	sctk_alloc_spinlock_t spinlock;
};

/************************** STRUCT *************************/
/**
 * Struct used to register free chunks into the free queue list on a remote allocation chaine.
 * RFQ = Remote Free Queue
**/
struct sctk_alloc_rfq_entry
{
	struct sctk_alloc_rfq_entry * next;
	void * ptr;
};

/************************** STRUCT *************************/
/**
 * Struct to define a remote free queue for parallel free. RFQ = Remote Free Queue
**/
struct sctk_alloc_rfq
{
	sctk_alloc_spinlock_t lock;
	struct sctk_alloc_rfq_entry * first;
	struct sctk_alloc_rfq_entry * last;
};

/************************** STRUCT *************************/
/**
 * The allocation permit to link a memory source to a thread pool. It can represent a user specific
 * allocation chain or a default thread allocation chain. This will be selected by a TLS variable
 * managed by allocation methods exposed to the user.
**/
struct sctk_alloc_chain
{
	struct sctk_thread_pool pool;
	struct sctk_alloc_mm_source * source;
	void * base_addr;
	void * end_addr;
	/** Flags to enable some options of the allocation chain. **/
	enum sctk_alloc_chain_flags flags;
	/** Spinlock used to protect the thread pool if it was shared, ignored otherwire. **/
	sctk_alloc_spinlock_t lock;
	/** Remote free queue **/
	struct sctk_alloc_rfq rfq;
	/**
	 * If not NULL, function to call when empty for final destruction.
	 * This is more to be used internally for thread destruction.
	**/
	void (*destroy_handler)(struct sctk_alloc_chain * chain);
	/**
	 * Count the total number of macro bloc currently managed by the chain.
	 * It serve to known when to call the destroy_handler method.
	**/
	int cnt_macro_blocs;
	/** Stats of the given allocation chain. **/
	#ifndef SCTK_ALLOC_STATS
		SCTK_ALLOC_STATS_HOOK(struct sctk_alloc_stats_chain stats);
	#endif

	/** Struct specific for allocation chain spying. **/
	#ifdef SCTK_ALLOC_SPY
		SCTK_ALLOC_SPY_HOOK(struct sctk_alloc_spy_chain spy);
	#endif
};

/************************** STRUCT *************************/
struct sctk_alloc_region_entry
{
	struct sctk_alloc_macro_bloc * macro_bloc;
};

/************************** STRUCT *************************/
/**
 * Define an entry from region header. For now it simply contain a pointer to the related allocation
 * chain. NULL if not used.
**/
struct sctk_alloc_region
{
	struct sctk_alloc_region_entry entries[SCTK_REGION_HEADER_ENTRIES];
};

/************************* GLOBALS *************************/
#ifndef SCTK_ALLOC_DEBUG
/** @todo TO REMOVE **/
extern struct sctk_alloc_chain * sctk_alloc_chain_list[2];
#endif //SCTK_ALLOC_DEBUG

/************************** TYPES **************************/
/**
 * Define a free chunk list. It is mostly represented by an empty free chunk to get a transparent
 * cycle of the double link list and avoid managing NULL pointers on borders.
**/
typedef struct sctk_alloc_free_chunk sctk_alloc_free_list_t;

/************************* FUNCTION ************************/
//chunk management.
//static __inline__ sctk_alloc_vchunk sctk_alloc_get_chunk(sctk_addr_t ptr);
//static inline sctk_alloc_vchunk sctk_alloc_setup_chunk(void * ptr, sctk_size_t size, void * prev);
//static __inline__ void sctk_alloc_setup_macro_bloc(struct sctk_alloc_macro_bloc * macro_bloc);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_setup_chunk_padded(sctk_alloc_vchunk chunk,sctk_size_t boundary);
SCTK_STATIC sctk_size_t sctk_alloc_calc_chunk_size(sctk_size_t user_size);
SCTK_STATIC sctk_size_t sctk_alloc_calc_body_size(sctk_size_t chunk_size);
SCTK_STATIC void * sctk_alloc_chunk_body(sctk_alloc_vchunk vchunk);
SCTK_STATIC void sctk_alloc_create_stopper(void * ptr,void * prev);
SCTK_STATIC sctk_size_t sctk_alloc_align_size(sctk_size_t size,sctk_size_t align);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_get_prev_chunk(sctk_alloc_vchunk chunk);
//static __inline__ sctk_alloc_vchunk sctk_alloc_get_next_chunk(sctk_alloc_vchunk chunk);

/************************* FUNCTION ************************/
//thread pool management
SCTK_STATIC void sctk_alloc_thread_pool_init(struct sctk_thread_pool * pool,const sctk_size_t alloc_free_sizes[SCTK_ALLOC_NB_FREE_LIST]);
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_get_free_list(struct sctk_thread_pool * pool,sctk_size_t size);
SCTK_STATIC sctk_size_t sctk_alloc_get_list_class(struct sctk_thread_pool* pool,sctk_alloc_free_list_t * list);
SCTK_STATIC void sctk_alloc_free_list_insert_raw(struct sctk_thread_pool * pool,void* ptr, sctk_size_t size,void * prev);
SCTK_STATIC void sctk_alloc_free_list_insert(struct sctk_thread_pool * pool,struct sctk_alloc_chunk_header_large * chunk_large,enum sctk_alloc_insert_mode insert_mode);
SCTK_STATIC void sctk_alloc_free_list_remove(struct sctk_thread_pool * pool,struct sctk_alloc_free_chunk * fchunk);
SCTK_STATIC struct sctk_alloc_free_chunk * sctk_alloc_find_adapted_free_chunk(sctk_alloc_free_list_t * list,sctk_size_t size);
SCTK_STATIC struct sctk_alloc_free_chunk * sctk_alloc_find_free_chunk(struct sctk_thread_pool * pool,sctk_size_t size);
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_get_next_list(const struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list);
SCTK_STATIC bool sctk_alloc_free_list_empty(const sctk_alloc_free_list_t * list);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_merge_chunk(struct sctk_thread_pool * pool,sctk_alloc_vchunk chunk,sctk_alloc_vchunk first_page_chunk,sctk_addr_t max_address);
SCTK_STATIC void sctk_alloc_free_chunk_range(struct sctk_thread_pool * pool,sctk_alloc_vchunk first,sctk_alloc_vchunk last);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_split_free_bloc(sctk_alloc_vchunk * chunk,sctk_size_t size);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_free_chunk_to_vchunk(struct sctk_alloc_free_chunk * chunk);
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_find_first_free_non_empty_list(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list);
SCTK_STATIC void sctk_alloc_free_list_mark_empty(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list);
SCTK_STATIC void sctk_alloc_free_list_mark_non_empty(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list);
SCTK_STATIC bool sctk_alloc_free_list_is_not_empty_quick(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list);

/************************* FUNCTION ************************/
//allocation chain management
SCTK_STATIC void sctk_alloc_chain_base_init(struct sctk_alloc_chain * chain,enum sctk_alloc_chain_flags flags);
void sctk_alloc_chain_user_init(struct sctk_alloc_chain * chain,void * buffer,sctk_size_t size,enum sctk_alloc_chain_flags flags);
void sctk_alloc_chain_default_init(struct sctk_alloc_chain * chain,struct sctk_alloc_mm_source * source,enum sctk_alloc_chain_flags flags);
void * sctk_alloc_chain_alloc(struct sctk_alloc_chain * chain,sctk_size_t size);
void * sctk_alloc_chain_alloc_align(struct sctk_alloc_chain * chain,sctk_size_t boundary,sctk_size_t size);
void sctk_alloc_chain_free(struct sctk_alloc_chain * chain,void * ptr);
SCTK_STATIC bool sctk_alloc_chain_can_destroy(struct sctk_alloc_chain* chain);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_request_mem(struct sctk_alloc_chain* chain,sctk_size_t size);
SCTK_STATIC bool sctk_alloc_chain_refill_mem(struct sctk_alloc_chain* chain,sctk_size_t size);
void sctk_alloc_chain_destroy(struct sctk_alloc_chain * chain,bool force);
void sctk_alloc_chain_purge_rfq(struct sctk_alloc_chain * chain);
SCTK_STATIC void sctk_alloc_chain_free_macro_bloc(struct sctk_alloc_chain * chain,sctk_alloc_vchunk vchunk);
SCTK_STATIC bool sctk_alloc_chain_can_remap(struct sctk_alloc_chain * chain);
void * sctk_alloc_chain_realloc(struct sctk_alloc_chain * chain, void * ptr, sctk_size_t size);
void sctk_alloc_chain_numa_migrate(struct sctk_alloc_chain * chain, int target_numa_node,bool migrate_chain_struct,bool migrate_content,struct sctk_alloc_mm_source * new_mm_source);
bool sctk_alloc_chain_is_thread_safe(struct sctk_alloc_chain * chain);
void sctk_alloc_chain_make_thread_safe(struct sctk_alloc_chain * chain,bool value);
void sctk_alloc_chain_mark_for_destroy(struct sctk_alloc_chain * chain,void (*destroy_handler)(struct sctk_alloc_chain * chain));


/************************* FUNCTION ************************/
//Base of memory source
void sctk_alloc_mm_source_base_init(struct sctk_alloc_mm_source * source);

/************************* FUNCTION ************************/
//default memory source functions
void sctk_alloc_mm_source_default_init(struct sctk_alloc_mm_source_default * source,sctk_addr_t heap_base,sctk_size_t heap_size);
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_default_request_memory(struct sctk_alloc_mm_source * source,sctk_size_t size);
SCTK_STATIC void sctk_alloc_mm_source_default_free_memory(struct sctk_alloc_mm_source * source,struct sctk_alloc_macro_bloc * bloc);
SCTK_STATIC void sctk_alloc_mm_source_default_cleanup(struct sctk_alloc_mm_source * source);
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_get_macro_bloc(void * ptr);
SCTK_STATIC void sctk_alloc_mm_source_insert_segment(struct sctk_alloc_mm_source_default* source,void * base,sctk_size_t size);

/************************* FUNCTION ************************/
//Region management
SCTK_STATIC struct sctk_alloc_region * sctk_alloc_region_setup(void * addr);
SCTK_STATIC struct sctk_alloc_region * sctk_alloc_region_get(void * addr);
SCTK_STATIC void sctk_alloc_region_del(struct sctk_alloc_region * region);
struct sctk_alloc_region_entry * sctk_alloc_region_get_entry(void* addr);
SCTK_STATIC bool sctk_alloc_region_exist(void * addr);
SCTK_STATIC void sctk_alloc_region_init(void);
SCTK_STATIC void sctk_alloc_region_del_all(void);
SCTK_STATIC void sctk_alloc_region_set_entry(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * macro_bloc);
SCTK_STATIC int sctk_alloc_region_get_id(void * addr);
SCTK_STATIC bool sctk_alloc_region_has_ref(struct sctk_alloc_region * region);
SCTK_STATIC void sctk_alloc_region_del_chain(struct sctk_alloc_region * region,struct sctk_alloc_chain * chain);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_prepare_and_reg_macro_bloc(struct sctk_alloc_chain * chaine,struct sctk_alloc_macro_bloc * macro_bloc);
SCTK_STATIC void sctk_alloc_region_unset_entry(struct sctk_alloc_macro_bloc * macro_bloc);
struct sctk_alloc_macro_bloc * sctk_alloc_region_get_macro_bloc(void * ptr);

/************************* FUNCTION ************************/
//remote free queue management for allocation chains
SCTK_STATIC void sctk_alloc_rfq_init(struct sctk_alloc_rfq * rfq);
SCTK_STATIC bool sctk_alloc_rfq_empty(struct sctk_alloc_rfq * rfq);
void sctk_alloc_rfq_register(struct sctk_alloc_rfq * rfq,void * ptr);
SCTK_STATIC struct sctk_alloc_rfq_entry * sctk_alloc_rfq_extract(struct sctk_alloc_rfq * rfq);
SCTK_STATIC int sctk_alloc_rfq_count_entries(struct sctk_alloc_rfq_entry * entries);
SCTK_STATIC void sctk_alloc_rfq_destroy(struct sctk_alloc_rfq * rfq);

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_H

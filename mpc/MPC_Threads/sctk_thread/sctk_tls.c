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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #                                                                      # */
/* ######################################################################## */


#define _GNU_SOURCE
#include "sctk_config.h"
#include "sctk_alloc.h"
#include "sctk_topology.h"
#include "sctk_accessor.h"
#include "sctk_atomics.h"
#include <sctk_debug.h>
#include <unistd.h>
#include <sctk_tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hwloc.h"

#if defined(SCTK_USE_TLS)
__thread void *sctk_extls = NULL;
#endif

#if defined(SCTK_USE_TLS) && defined(Linux_SYS)
#include <link.h>
#include <stdlib.h>
#include <stdio.h>

/* to set GS register */
#include <asm/prctl.h>
#include <asm/unistd.h>
//#include <asm/unistd_64.h>

static __thread unsigned long p_memsz;
static __thread unsigned long p_filesz;
static __thread void *p_vaddr;
static __thread size_t current_module;
static __thread size_t total_module;
static size_t page_size = 0;

static int
callback (struct dl_phdr_info *info, size_t size, void *data)
{
  int j;


  for (j = 0; j < info->dlpi_phnum; j++)
    {
      if (info->dlpi_phdr[j].p_type == PT_TLS)
	{
	  total_module++;
	  if (total_module == current_module)
	    {
	      p_memsz = info->dlpi_phdr[j].p_memsz;
	      p_filesz = info->dlpi_phdr[j].p_filesz;
	      p_vaddr =
		(void *) ((char *) info->dlpi_addr +
			  info->dlpi_phdr[j].p_vaddr);
	      sctk_nodebug ("TLS in %d %p size %lu start %p %lu", j,
			    info->dlpi_addr, p_memsz, p_vaddr,
			    info->dlpi_phdr[j].p_filesz);
	      return 1;
	    }
	  else
	    {
	      sctk_nodebug ("TLS SKIP in %d %p %d != %d", j, info->dlpi_addr,
			    total_module, current_module);
	    }
	}
    }
  return 0;
}

typedef struct
{
  unsigned long int ti_module;
  unsigned long int ti_offset;
} tls_index;

typedef char *sctk_tls_module_t;

typedef struct
{
  volatile size_t size;
  volatile sctk_tls_module_t *volatile modules;
  sctk_spinlock_t lock;
} tls_level;

typedef struct
{
  tls_level level ;
  sctk_atomics_int toenter ; /* total number of threads on this level */
  sctk_atomics_int entered ; /* number of VPs that entered the hls barrier */
  volatile int generation ;  /* number of barrier/single encountered */
  sctk_atomics_int nowait_generation ; /* number of single nowait encountered */
} hls_level ;

typedef struct
{
  int wait   ; /* number of barrier/single encountered on this thread */
  int nowait ; /* number of single nowait encountered on this thread */
} hls_generation_t ;

static hls_level **sctk_hls_repository ; /* global per process */
static __thread hls_level* sctk_hls[sctk_hls_max_scope] ; /* per VP */
__thread void* sctk_hls_generation ; /* per thread */
/* need to be saved and restored at context switch */

__thread void *sctk_tls_module_vp[sctk_extls_max_scope+sctk_hls_max_scope] ;
/* store a direct pointer to each tls module */
/* used by tls optimized in the linker */
/* gs register contains the address of this array */
/* per thread: need to be updated at context switch */
/* real type: sctk_tls_module_t** */
__thread void **sctk_tls_module ;

static inline void
sctk_tls_init_level (tls_level * level)
{
  level->size = 0;
  level->modules = NULL;
  level->lock = 0;
}

static inline void
sctk_tls_write_level (tls_level * level)
{
  sctk_spinlock_lock (&(level->lock));
}

static inline void
sctk_tls_unlock_level (tls_level * level)
{
  sctk_spinlock_unlock (&(level->lock));
}

static inline void
sctk_hls_init_level(hls_level * level)
{
	int i ;
	sctk_tls_init_level(&level->level);
	sctk_atomics_store_int ( &level->toenter, 0 ) ;
	sctk_atomics_store_int ( &level->entered, 0 ) ;
	level->generation = 0 ;
	sctk_atomics_store_int ( &level->nowait_generation, 0 ) ;
}

static inline size_t
sctk_determine_module_size (size_t m)
{
  return p_memsz;
}

static inline void
sctk_init_module (size_t m, char *module, size_t size)
{
  /*  memset (module, 0, size);  Already done thanks to the mmap allocation */

  if (p_filesz)
    {
      memcpy (module, p_vaddr, p_filesz);
    }
}

#define SCTK_COW_TLS
static inline int
sctk_get_module_file_decr (size_t m, size_t module_size)
{
#if defined(SCTK_COW_TLS)
  static sctk_spinlock_t lock = 0;
  static int *fds = NULL;
  static size_t size = 0;
  int fd = -1;
  size_t i;
  char *tls_module;

  sctk_spinlock_lock (&(lock));
  if (size <= m)
    {
      fds = sctk_realloc (fds, m + 1);
      for (i = size; i < m + 1; i++)
	{
	  fds[i] = -1;
	}
      size = m + 1;
    }
  fd = fds[m];

  if (fd == -1)
    {
      static char name[4096];
      sprintf (name, "/tmp/cp_on_write_get_module_file_decr_%d_module_%lu",
	       getpid (), m);

      remove (name);
      fd = open (name, O_CREAT | O_RDWR | O_TRUNC,S_IRWXU);
      assert (fd > 0);

      tls_module = sctk_malloc (module_size);

      memset (tls_module, 0, module_size);
      sctk_init_module (m, tls_module, module_size);

      assume(write (fd, tls_module, module_size) == module_size);
      free (tls_module);
      remove (name);

      fds[m] = fd;
    }
  sctk_spinlock_unlock (&(lock));
  return fd;
#else
  return -1;
#endif
}

static char *
sctk_alloc_module (size_t m, tls_level * tls_level)
{
  char *tls_module;
  tls_module = (char *) tls_level->modules[m - 1];
  if (tls_module == NULL)
    {
      size_t module_size;
      size_t module_size_rest;
      int fd;

      current_module = m;
      total_module = 0;
      dl_iterate_phdr (callback, NULL);

      module_size = sctk_determine_module_size (m);
      assume (page_size != 0);


      module_size_rest = module_size % page_size;
      if (module_size_rest != 0)
	{
	  module_size = module_size + (page_size - module_size_rest);
	}


      fd = sctk_get_module_file_decr (m, module_size);

      if (fd == -1)
	{
	  tls_module = sctk_user_mmap (NULL, module_size,
				       PROT_READ | PROT_WRITE,
				       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	  sctk_init_module (m, tls_module, module_size);
	}
      else
	{
	  tls_module = sctk_user_mmap (NULL, module_size,
				       PROT_READ | PROT_WRITE,
				       MAP_PRIVATE, fd, 0);
	}

      assume (tls_module != NULL);

      tls_level->modules[m - 1] = tls_module;
      sctk_nodebug ("Init module %d size %lu, %d", m, module_size,
		    *((int *) p_vaddr));
    }
  return tls_module;
}

static inline void *
__sctk__tls_get_addr__generic_scope ( size_t module_id,
	size_t offset, tls_level *tls_level)
{
  void *res;
  char *tls_module;

  sctk_nodebug ("Module %lu Offset %lu", module_id, offset);

  assert ( tls_level != NULL ) ;
  assert ( module_id >= 1 ) ;

  /* resize modules array if needed */
  if (expect_false(module_id > tls_level->size))
  {
	  size_t i;
	  sctk_tls_write_level (tls_level);
	  if (module_id > tls_level->size) {
		  tls_level->modules = sctk_realloc ((void *) (tls_level->modules), module_id * sizeof (char *));
		  for (i = tls_level->size; i < module_id; i++)
			  tls_level->modules[i] = NULL;
		  sctk_nodebug ("Init modules to size %ld->%ld", tls_level->size, m);
		  tls_level->size = module_id;
	  }
	  sctk_tls_unlock_level (tls_level);
  }

  /* alloc module if needed */
  if (expect_false(tls_level->modules[module_id-1] == NULL))
  {
	  sctk_tls_write_level (tls_level);
	  if ( tls_level->modules[module_id-1] == NULL )
		  tls_module = sctk_alloc_module (module_id, tls_level);
	  sctk_tls_unlock_level (tls_level);
  }

  tls_module = (char *) tls_level->modules[module_id - 1];
  res = tls_module + offset;
  return res;
}


static void
sctk_extls_create ()
{
  tls_level **extls;
  int i;

  /* page_size = getpagesize ();
     has been put in sctk_hls_build_repository() */

  extls = sctk_malloc (sctk_extls_max_scope * sizeof (tls_level *));

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      extls[i] = sctk_malloc (sizeof (tls_level));
      sctk_tls_init_level (extls[i]);
    }

  sctk_nodebug ("Init extls root %p", extls);
  sctk_extls = extls;
}

/*
  take TLS from father
*/
void
sctk_extls_duplicate (void **new)
{
  tls_level **extls;
  tls_level **new_extls;
  int i;

  extls = sctk_extls;
  if (extls == NULL)
    {
      sctk_extls_create ();
    }

  new_extls = sctk_malloc (sctk_extls_max_scope * sizeof (tls_level*));
  *(tls_level***)new = new_extls;

  extls = sctk_extls;
  sctk_nodebug ("Duplicate %p->%p", extls, new_extls);

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      new_extls[i] = extls[i];
    }
}

/*
   only keep some tls and create new ones
*/
void
sctk_extls_keep (int *scopes)
{
  tls_level **extls;
  int i;
  extls = sctk_extls;

  assert (extls != NULL);

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      if (scopes[i] == 0)
	{
	  sctk_nodebug ("Remove %d in %p", i, extls);
	  extls[i] = sctk_malloc (sizeof (tls_level));
	  sctk_tls_init_level (extls[i]);
	}
    }
}

/*
  used by openmp in MPC_OpenMP/src/mpcomp_internal.h
*/
void
sctk_extls_keep_non_current_thread (void **extls, int *scopes)
{
  int i;

  assert (extls != NULL);

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      if (scopes[i] == 0)
	{
	  sctk_nodebug ("Remove %d in %p", i, extls);
	  extls[i] = sctk_malloc (sizeof (tls_level));
	  sctk_tls_init_level (extls[i]);
	}
    }
}

void
sctk_extls_delete ()
{
#warning "Pas de liberation des extls"
}

/*
 * At MPC startup, create all HLS levels
 * Called in sctk_perform_initialisation in sctk_launch.c
 */

#define MAX(a,b) ((a)>=(b)?(a):(b))

void sctk_hls_build_repository ()
{
  page_size = getpagesize ();

  hwloc_topology_t topology = sctk_get_topology_object() ;
  const int topodepth = hwloc_topology_get_depth(topology);
  const int numa_node_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) ;
  int * level_number ; /* per numa node */
  hwloc_obj_t* stack ;
  int stack_idx ;
  const int is_numa_node = ( numa_node_number != 0 ) ;
  int i;

  level_number = alloca(MAX(1,numa_node_number)*sizeof(int)) ;
  for ( i = 0 ; i < MAX(1,numa_node_number) ; ++i )
	  level_number[i] = 0 ;

  stack = alloca( (topodepth+1) * sizeof(hwloc_obj_t) );
  stack_idx = 0 ;
  stack[stack_idx]   = hwloc_get_root_obj ( topology ) ;
  stack[stack_idx+1] = NULL ;

  do{
	  assert(stack_idx < topodepth+1 ) ;
	  hwloc_obj_t cur_obj    = stack[stack_idx] ;
	  hwloc_obj_t prev_child = stack[stack_idx+1] ;
	  hwloc_obj_t next_child = hwloc_get_next_child(topology,cur_obj,prev_child);

	  if ( prev_child == NULL ) {
		  char level_id[8] ;
		  int numa_id = 0 ;
		  if ( is_numa_node && cur_obj->type != HWLOC_OBJ_MACHINE ) {
			  if ( cur_obj->type == HWLOC_OBJ_NODE )
				  numa_id = cur_obj->logical_index ;
			  else
			      numa_id = hwloc_get_ancestor_obj_by_type(topology,HWLOC_OBJ_NODE,cur_obj)->logical_index ;
		  }
		  sprintf ( level_id, "%d", level_number[numa_id] ) ;
		  hwloc_obj_add_info ( cur_obj, "hls_level", level_id ) ;
		  level_number[numa_id] += 1 ;
	  }

	  if ( next_child != NULL && next_child->type != HWLOC_OBJ_PU ) {
		  stack_idx += 1 ;
		  stack[stack_idx] = next_child ;
		  stack[stack_idx+1] = NULL ;
	  }else{
		  stack_idx -= 1 ;
	  }

  }while ( stack_idx >= 0 ) ;

  sctk_hls_repository = sctk_malloc(MAX(1,numa_node_number)*sizeof(hls_level*));
  for ( i = 0 ; i < MAX(1,numa_node_number) ; ++i ) {
	  int j ;
	  sctk_hls_repository[i] = sctk_malloc_on_node ( level_number[i]*sizeof(hls_level), i ) ;
	  for ( j=0 ; j<level_number[i] ; ++j )
		  sctk_hls_init_level ( sctk_hls_repository[i] + j ) ;
  }
}

/*
 * During VP initialization, take HLS levels according to the VP topology
 * from the repository of all HLS levels created by sctk_hls_build_repository
 * at initialization.
 */
void sctk_hls_checkout_on_vp ()
{
  /* check if this has already been done */
  if ( sctk_hls[sctk_hls_node_scope] == NULL ) {

  hwloc_topology_t topology = sctk_get_topology_object() ;
  const int socket_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  const int core_depth   = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
  hwloc_obj_t pu = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU,sctk_get_cpu());
  const int numa_id = sctk_is_numa_node() ?
	  				  hwloc_get_ancestor_obj_by_type(topology,HWLOC_OBJ_NODE,pu)->logical_index
					  : 0 ;

  int i ;
  int level_id ;
  hwloc_obj_t obj ;

  sctk_hls[sctk_hls_node_scope] = sctk_hls_repository[0] ;
  for ( i = 1 ; i < sctk_hls_max_scope ; ++i )
	  sctk_hls[i] = NULL ;

  obj = hwloc_get_ancestor_obj_by_depth (topology, socket_depth-1, pu) ;
  if ( obj != NULL && obj->type == HWLOC_OBJ_NODE ) {
	  level_id = atol ( hwloc_obj_get_info_by_name (obj, "hls_level") ) ;
	  sctk_hls[sctk_hls_numa_level_1_scope] = sctk_hls_repository[numa_id] + level_id ;
  }

  obj = hwloc_get_ancestor_obj_by_type (topology, HWLOC_OBJ_SOCKET, pu) ;
  if ( obj != NULL ) {
	  level_id = atol ( hwloc_obj_get_info_by_name (obj, "hls_level") ) ;
	  sctk_hls[sctk_hls_socket_scope] = sctk_hls_repository[numa_id] + level_id ;
  }

  for ( i = 1 ; i <= 3 ; ++i ) {
	  obj = hwloc_get_ancestor_obj_by_depth (topology, core_depth-i, pu) ;
	  if ( obj != NULL && obj->type == HWLOC_OBJ_CACHE ) {
		  level_id = atol ( hwloc_obj_get_info_by_name (obj, "hls_level") ) ;
		  sctk_hls[sctk_hls_core_scope-i] = sctk_hls_repository[numa_id] + level_id ;
	  }
  }

  obj = hwloc_get_ancestor_obj_by_type (topology, HWLOC_OBJ_CORE, pu) ;
  if ( obj != NULL ) {
	  level_id = atol ( hwloc_obj_get_info_by_name (obj, "hls_level") ) ;
	  sctk_hls[sctk_hls_core_scope] = sctk_hls_repository[numa_id] + level_id ;
  }

  }
}

/*
 * Called by each thread before running user code
 * Update the number of threads to enter hls levels
 * Allocate and initialize sctk_hls_generation
 */
void sctk_hls_register_thread ()
{
  int i ;
  for ( i = sctk_hls_socket_scope ; i < sctk_hls_max_scope ; ++i )
	  if ( sctk_hls[i] != NULL )
		  sctk_atomics_incr_int ( &sctk_hls[i]->toenter ) ;

  if ( sctk_is_numa_node() )
  {
	  const int toenter = sctk_atomics_fetch_and_incr_int ( &sctk_hls[sctk_hls_numa_level_1_scope]->toenter ) ;
	  if ( toenter == 0 ) {
		  for ( i = 0 ; i < sctk_hls_numa_level_1_scope ; ++i )
			  if ( sctk_hls[i] != NULL )
				  sctk_atomics_incr_int ( &sctk_hls[i]->toenter ) ;
	  }
  }
  else
  {
	  sctk_atomics_incr_int ( &sctk_hls[sctk_hls_node_scope]->toenter ) ;
  }

  sctk_hls_generation = sctk_calloc(sctk_hls_max_scope,sizeof(hls_generation_t));
}

/*
 * Not yet called
 */
void sctk_hls_free ()
{
  int i ;
  if ( sctk_is_numa_node() ) {
	  const int numa_level_1_number   = sctk_get_numa_node_number() ;
	  for ( i=0 ; i < numa_level_1_number ; ++i )
		  free ( sctk_hls_repository[i] ) ;
  }else{
	  free ( sctk_hls_repository[0] ) ;
  }
  free ( sctk_hls_repository ) ;
}

/*
 * Set the GS register to contain the address
 * of the tls_module array
 * to be called on each VP
 */
void
sctk_tls_module_set_gs_register ()
{
	static __thread int done_on_this_vp = 0 ;
	if ( done_on_this_vp == 1 )
		return ;

	int result;
	void *gs = (void*)sctk_tls_module_vp ;
	asm volatile ("syscall"
			: "=a" (result)
			: "0" ((unsigned long int ) __NR_arch_prctl),
			"D" ((unsigned long int ) ARCH_SET_GS),
			"S" (gs)
			: "memory", "cc", "r11", "cx");
	assume(result == 0);
	done_on_this_vp = 1 ;
}

/*
 * Allocate and fill the tls_module for a thread
 * to be called before the user code starts
 */
void
sctk_tls_module_alloc_and_fill ()
{
	int i ;
	tls_level **extls = (tls_level**) sctk_extls ;
	sctk_tls_module_t *tls_module = sctk_calloc(sctk_extls_max_scope+sctk_hls_max_scope,sizeof(sctk_tls_module_t));

	for ( i=0 ; i<sctk_extls_max_scope ; ++i ) {
		if ( extls[i] == NULL ) {
			/* this scope is not used */
			tls_module[i] = NULL ;
			continue ;
		}
		/* optimized tls access are in the first module */
		/* generate a dummy access to an ex-tls variable to initialize memory if needed */
		if ( extls[i]->modules == NULL || extls[i]->modules[0] == NULL ) {
			void *dummy = __sctk__tls_get_addr__generic_scope (1,0,extls[i]) ;
		}
		assert ( extls[i]->modules[0] != NULL ) ;
		tls_module[i] = extls[i]->modules[0] ;
	}

	for ( i=0 ; i<sctk_hls_max_scope ; ++i ) {
		if ( sctk_hls[i] == NULL ) {
			/* this scope is not used */
			tls_module[sctk_extls_max_scope+i] = NULL ;
			continue ;
		}
		/* optimized tls access are in the first module */
		/* generate a dummy access to an hls variable to initialize memory if needed */
		if ( sctk_hls[i]->level.modules == NULL || sctk_hls[i]->level.modules[0] == NULL ) {
			void *dummy = __sctk__tls_get_addr__generic_scope (1,0,&sctk_hls[i]->level) ;
		}
		assert ( sctk_hls[i]->level.modules[0] != NULL ) ;
		tls_module[sctk_extls_max_scope+i] = sctk_hls[i]->level.modules[0] ;
	}

	for ( i=0; i<sctk_extls_max_scope+sctk_hls_max_scope; ++i )
		sctk_tls_module_vp[i] = tls_module[i] ;

	sctk_tls_module = (void**)tls_module ;
}


#if defined(SCTK_i686_ARCH_SCTK) || defined (SCTK_x86_64_ARCH_SCTK)

void *
__sctk__tls_get_addr__process_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__process_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_process_scope]);
  return res;
}

void *
__sctk__tls_get_addr__task_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__task_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_task_scope]);
  return res;
}

void *
__sctk__tls_get_addr__thread_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__thread_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_thread_scope]);
  return res;
}


#if defined (MPC_OpenMP)
void *
__sctk__tls_get_addr__openmp_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__openmp_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_openmp_scope]);
  return res;
}
#endif

void *
__sctk__tls_get_addr__node_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__node_scope %p", &sctk_hls );
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_node_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__numa_level_2_scope (tls_index * tmp)
{
  void *res;
  tls_level **hls;
  sctk_nodebug ("__sctk__tls_get_addr__numa_level_2_scope on numa node %d",
    sctk_get_node_from_cpu(sctk_get_cpu()));
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_numa_level_2_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__numa_level_1_scope (tls_index * tmp)
{
  void *res;
  tls_level **hls;
  sctk_nodebug ("__sctk__tls_get_addr__numa_level_1_scope on numa node %d",
    sctk_get_node_from_cpu(sctk_get_cpu()));
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_numa_level_1_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__socket_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__socket_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_socket_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__cache_level_3_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_level_3_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_level_3_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__cache_level_2_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_level_2_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_level_2_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__cache_level_1_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_level_1_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_level_1_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__core_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__core_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_core_scope]->level);
  return res;
}


#elif defined (SCTK_ia64_ARCH_SCTK)

void *
__sctk__tls_get_addr__process_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_process_scope]);
  return res;
}


void *
__sctk__tls_get_addr__task_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_task_scope]);
  return res;
}


void *
__sctk__tls_get_addr__thread_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_thread_scope]);
  return res;
}

#if defined (MPC_OpenMP)
void *
__sctk__tls_get_addr__openmp_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_openmp_scope]);
  return res;
}
#endif

void *
__sctk__tls_get_addr__node_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_node_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__numa_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_numa_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__socket_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_socket_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__cache_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_cache_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__core_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_core_scope].level);
  return res;
}

#else
#error "Architecture not available for TLS support"
#endif

void __sctk__hls_single_done ( sctk_hls_scope_t scope ) ;

int
__sctk__hls_single ( sctk_hls_scope_t scope ) {

	sctk_nodebug ("call to hls single with scope %d", scope) ;

	hls_level * const level = sctk_hls[scope];
	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	const int mygeneration = ++hls_generation[scope].wait ;

	if ( sctk_is_numa_node() &&
	   ( scope == sctk_hls_node_scope || scope == sctk_hls_numa_level_2_scope ) )
	{
		if ( __sctk__hls_single ( sctk_hls_numa_level_1_scope ) ) {
			const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
			if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
				return 1 ;
			}else{
				sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
				__sctk__hls_single_done ( sctk_hls_numa_level_1_scope ) ;
				return 0 ;
			}
		}else{
			return 0 ;
		}
	}

	const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
	if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
		return 1 ;
	}else{
		sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
		return 0 ;
	}
}

void
__sctk__hls_single_done ( sctk_hls_scope_t scope ) {

	sctk_nodebug ("call to hls single done with scope %d", scope) ;

	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	hls_level * const level = sctk_hls[scope];

	sctk_atomics_store_int ( &level->entered, 0 ) ;
	sctk_atomics_write_barrier() ;
	level->generation = hls_generation[scope].wait ;

	if ( sctk_is_numa_node() &&
	   ( scope == sctk_hls_node_scope || scope == sctk_hls_numa_level_2_scope ) )
	{
		__sctk__hls_single_done ( sctk_hls_numa_level_1_scope ) ;
	}

	return ;
}

void
__sctk__hls_barrier ( sctk_hls_scope_t scope ) {

	sctk_nodebug ("call to hls barrier with scope %d", scope ) ;

	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	hls_level * const level = sctk_hls[scope];
	const int mygeneration = ++hls_generation[scope].wait ;
	if ( sctk_is_numa_node() &&
	   ( scope == sctk_hls_node_scope || scope == sctk_hls_numa_level_2_scope ) )
	{
		if ( __sctk__hls_single ( sctk_hls_numa_level_1_scope ) ) {
			const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
			if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
				sctk_atomics_store_int ( &level->entered, 0 ) ;
				sctk_atomics_write_barrier() ;
				level->generation = mygeneration ;
			}else{
				sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
			}
			__sctk__hls_single_done ( sctk_hls_numa_level_1_scope ) ;
		}
		return ;
	}

	const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
	if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
		sctk_atomics_store_int ( &level->entered, 0 ) ;
		sctk_atomics_write_barrier() ;
		level->generation = mygeneration ;
	}else{
		sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
	}
}

int
__sctk__hls_single_nowait ( sctk_hls_scope_t scope ) {

	sctk_nodebug ("call to hls single nowait with scope %d", scope) ;

	int execute = 0 ;
	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	hls_level * const level = sctk_hls[scope];
	const int generation = sctk_atomics_load_int ( &level->nowait_generation ) ;
	int * const mygeneration = &hls_generation[scope].nowait ;

	if ( *mygeneration == generation )
		execute = sctk_atomics_cas_int ( &level->nowait_generation, generation, generation+1 ) ;

	*mygeneration = generation ;
	return execute ;
}

#else
#warning "Experimental Ex-TLS support desactivated"

void
sctk_extls_create ()
{
}

void
sctk_extls_duplicate (void **new)
{
  *new = NULL;
}

void
sctk_extls_keep (int *scopes)
{
}

void
sctk_extls_keep_non_current_thread (void **tls, int *scopes)
{
}

void
sctk_extls_delete ()
{
}

#endif

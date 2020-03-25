/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sctk_config.h>
#include <sctk_spinlock.h>
#include <sctk_alloc.h>
#include "sctk_stdint.h"

#ifndef __SCTK__LIST__
#define __SCTK__LIST__


struct sctk_internal_list_elem {
  void* elem;
  struct sctk_internal_list_elem *p_prev;
  struct sctk_internal_list_elem *p_next;
};

struct sctk_list {
  sctk_uint64_t elem_count;
  sctk_uint8_t is_collector;
  sctk_uint8_t is_initialized;
  size_t size_payload;
  sctk_spinlock_t   lock;
  struct sctk_internal_list_elem *head;
  struct sctk_internal_list_elem *tail;
  sctk_alloc_buffer_t alloc_buff;
};

void
sctk_list_new(struct sctk_list* list, sctk_uint8_t is_collector, size_t size_payload);

void *
sctk_list_push(struct sctk_list* list, void *elem);

void *
sctk_list_get_from_head(struct sctk_list* list, sctk_uint32_t n);

  void*
sctk_list_remove(struct sctk_list* list, struct sctk_internal_list_elem* elem);

int sctk_list_is_empty(struct sctk_list* list);

void* sctk_list_walk_on_cond(struct sctk_list* list, int cond,
    void* (*funct) (void* elem, int cond), int remove);

void* sctk_list_walk(struct sctk_list* list,
    void* (*funct) (void* elem), int remove);

void*
  sctk_list_pop(struct sctk_list* list);

#define sctk_list_lock(list)  sctk_spinlock_lock(&(list)->lock)
#define sctk_list_unlock(list) sctk_spinlock_unlock(&(list)->lock)
#define sctk_list_trylock(list) sctk_spinlock_trylock(&(list)->lock)
#define sctk_list_is_initialized(list) (list)->is_initialized

#endif				/*  */
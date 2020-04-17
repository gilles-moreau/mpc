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
#include <sctk.h>
#include "sctk_list.h"

/* TODO: Collector mode not implemented */
void
sctk_list_new(struct sctk_list* list, sctk_uint8_t is_collector, size_t size_payload)
{
  list->elem_count = 0;
  list->lock = SCTK_SPINLOCK_INITIALIZER;
  list->is_collector = is_collector;
  list->size_payload = size_payload;
  list->is_initialized = 1;
  sctk_buffered_alloc_create(&list->alloc_buff, size_payload + sizeof(struct sctk_internal_list_elem));
  list->head = NULL;
  list->tail = NULL;
}


static struct sctk_internal_list_elem*
sctk_list_alloc_elem(struct sctk_list* list, void* elem, size_t size, sctk_uint8_t collector )
{
  struct sctk_internal_list_elem *tmp = (struct sctk_internal_list_elem *)
    sctk_buffered_malloc(&list->alloc_buff, sizeof(struct sctk_internal_list_elem) + size);

  if (collector)
  {
    tmp->elem = (char*) tmp + sizeof(struct sctk_internal_list_elem);
    memcpy(tmp->elem, elem, size);
  }
  else
  {
    tmp->elem = elem;
  }
  sctk_nodebug("Elem : %p (elem %p, size %d collector %d)", tmp->elem, elem, size, collector);

  return tmp;
}

void *
sctk_list_get_from_head(struct sctk_list* list, sctk_uint32_t n)
{
  struct sctk_internal_list_elem *tmp = list->head;

  assume(list->is_initialized);

  while (tmp && n--) {
    tmp = tmp->p_next;
  }

  if (tmp != NULL)
    return tmp->elem;
  else
    return NULL;
}

  void*
sctk_list_remove(struct sctk_list* list, struct sctk_internal_list_elem* elem)
{
  void* payload;

  assume(list->is_initialized);

  if (!list || !elem || !list->head)
    return NULL;
  /* 1st elem */
  if (list->head->p_next == NULL)
  {
    sctk_nodebug("Removed (1) (%p)", list);
    list->head = NULL;
    list->tail = NULL;
  } else if (!elem->p_prev) {
    sctk_nodebug("Removed (2) (%p)", list);
    elem->p_next->p_prev = NULL;
    list->head = elem->p_next;
  /* last elem */
  } else if(!elem->p_next) {
    sctk_nodebug("Removed (3) (%p)", list);
    elem->p_prev->p_next = NULL;
    list->tail = elem->p_prev;
  /* somewhere else */
  } else {
    sctk_nodebug("Removed (4) (%p)", list);
    elem->p_prev->p_next = elem->p_next;
    elem->p_next->p_prev = elem->p_prev;
  }

  payload = elem->elem;
  list->elem_count--;
  sctk_nodebug("Elem removed : %p", elem->elem);
  sctk_buffered_free(elem);
  return payload;
}

  void*
sctk_list_pop(struct sctk_list* list)
{
  void* ret;
  assume(list->is_initialized);

  sctk_list_lock(list);
  ret = sctk_list_remove(list, list->head);
  sctk_list_unlock(list);
  return ret;
}

void *
sctk_list_push(struct sctk_list* list, void *elem)
{
  struct sctk_internal_list_elem *new_elem = NULL;

  assume(list->is_initialized);

  new_elem = sctk_list_alloc_elem(list, elem, list->size_payload, list->is_collector);
  assume(new_elem);

  if (list->tail == NULL)
  {
    sctk_nodebug("Push (1) (%p)", list);
    new_elem->p_prev = NULL;
    new_elem->p_next = NULL;
    list->head = new_elem;
    list->tail = new_elem;
  } else {
    sctk_nodebug("Push (2) (%p)", list);
    new_elem->p_prev = list->tail;
    new_elem->p_next = NULL;
    list->tail->p_next = new_elem;
    list->tail = new_elem;
  }

  sctk_nodebug("head : %p", elem);
  list->elem_count++;
  return new_elem->elem;
}

void* sctk_list_search_and_free(struct sctk_list* list,
    void* elem)
{
  struct sctk_internal_list_elem *tmp = list->head;
  int i = 0;
  sctk_nodebug("Free : %p", list->head->elem);

  assume(list->is_initialized);

  while (tmp) {
    sctk_nodebug("CMP %p <-> %p", tmp->elem, elem);
    if (tmp->elem == elem)
    {
        sctk_nodebug("iter %d", i);
        sctk_nodebug("Ptr 2: %p", tmp);
        return sctk_list_remove(list, tmp);
    }

    ++i;
    tmp = tmp->p_next;
  }

  sctk_nodebug("iter %d", i);
  return NULL;
}

void* sctk_list_walk(struct sctk_list* list,
    void* (*funct) (void* elem), int remove)
{
  struct sctk_internal_list_elem *tmp = list->head;
  assume(funct);
  void* ret = NULL;
  int i = 0;

  assume(list->is_initialized);

  while (tmp) {

    ret = funct(tmp->elem);
    if (ret)
    {
      sctk_nodebug("iter %d", i);
      if (remove)
        sctk_list_remove(list, tmp);
      return ret;
    }

    ++i;
    tmp = tmp->p_next;
  }
  return ret;
}


void* sctk_list_walk_on_cond(struct sctk_list* list, int cond,
    void* (*funct) (void* elem, int cond), int remove)
{
  struct sctk_internal_list_elem *tmp = NULL;
  assume(funct);
  void* ret = NULL;
  int i = 0;

  assume(list->is_initialized);

  sctk_list_lock(list);
  tmp = list->head;
  while (tmp) {

    ret = funct(tmp->elem, cond);
    if (ret)
    {
      if (remove)
        sctk_list_remove(list, tmp);
      sctk_list_unlock(list);
      sctk_nodebug("iter %d", i);
      return ret;
    }

    ++i;
    tmp = tmp->p_next;
  }
  sctk_list_unlock(list);
  sctk_nodebug("iter %d", i);
  return ret;
}


int sctk_list_is_empty(struct sctk_list* list)
{
  assume(list->is_initialized);

  if(list->head) return 0;
  else return 1;
}

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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_loop.h"
#include "mpcomp_openmp_tls.h"
#include "mpcomp_loop_static_ull.h"

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */

void __mpcomp_static_schedule_get_specific_chunk_ull(
    unsigned long long rank, unsigned long long num_threads,
    mpcomp_loop_ull_iter_t *loop, unsigned long long chunk_num,
    unsigned long long *from, unsigned long long *to) {

  const unsigned long long decal =
      chunk_num * num_threads * loop->chunk_size * loop->incr;
  
  *from = loop->lb + decal + loop->chunk_size * loop->incr * rank;
  *to = *from + loop->chunk_size * loop->incr;

  *to = (loop->up && *to > loop->b) ? loop->b : *to;
  *to = (!loop->up && *to < loop->b) ? loop->b : *to;
}

void __mpcomp_static_loop_init_ull(__UNUSED__ mpcomp_thread_t *t, __UNUSED__ unsigned long long lb,
                                   __UNUSED__ unsigned long long b,
                                   __UNUSED__ unsigned long long incr,
                                   __UNUSED__ unsigned long long chunk_size) {
  return;
}

int __mpcomp_static_loop_begin_ull(bool up, unsigned long long lb,
                                   unsigned long long b,
                                   unsigned long long incr,
                                   unsigned long long chunk_size,
                                   unsigned long long *from,
                                   unsigned long long *to) {
  mpcomp_loop_ull_iter_t *loop;

  __mpcomp_init();

  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_STATIC_LOOP;
  t->schedule_is_forced = 0;

  /* Fill private info about the loop */
  t->info.loop_infos.type = MPCOMP_LOOP_TYPE_ULL;
  loop = &(t->info.loop_infos.loop.mpcomp_ull);
  loop->up = up;
  loop->lb = lb;
  loop->b = b;
  loop->incr = incr;
  loop->chunk_size = chunk_size;
  t->static_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank_ull(
      t->rank, t->info.num_threads, loop);

  /* As the loop_next function consider a chunk as already been realised
       we need to initialize to 0 minus 1 */
  t->static_current_chunk = -1;

  if (!from && !to) {
    return -1;
  }

  return __mpcomp_static_loop_next_ull(from, to);
}

int __mpcomp_static_loop_next_ull(unsigned long long *from,
                                  unsigned long long *to) {
  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  /* Retrieve the number of threads and the rank of this thread */
  const unsigned long long num_threads = t->info.num_threads;
  const unsigned long long rank = t->rank;

  /* Next chunk */
  t->static_current_chunk++;

  /* Check if there is still a chunk to execute */
  if (t->static_current_chunk >= t->static_nb_chunks)
    return 0;

  __mpcomp_static_schedule_get_specific_chunk_ull(
      rank, num_threads, &(t->info.loop_infos.loop.mpcomp_ull),
      t->static_current_chunk, from, to);
  return 1;
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int __mpcomp_ordered_static_loop_begin_ull(bool up, unsigned long long lb,
                                           unsigned long long b,
                                           unsigned long long incr,
                                           unsigned long long chunk_size,
                                           unsigned long long *from,
                                           unsigned long long *to) {
  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int ret =
      __mpcomp_static_loop_begin_ull(up, lb, b, incr, chunk_size, from, to);
  if (!from) {
    return -1;
  }
  t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
  return ret;
}

int __mpcomp_ordered_static_loop_next_ull(unsigned long long *from,
                                          unsigned long long *to) {
  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int ret = __mpcomp_static_loop_next_ull(from, to);
  t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
  return ret;
}
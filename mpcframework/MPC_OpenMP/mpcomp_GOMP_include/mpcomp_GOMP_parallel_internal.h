#ifndef __MPCOMP_GOMP_PARALLEL_H__
#define __MPCOMP_GOMP_PARALLEL_H__

#include "mpcomp_types.h"

void mpcomp_internal_GOMP_start_parallel_region(void (*fn)(void *), void *data,
                                                unsigned num_threads);
void mpcomp_internal_GOMP_end_parallel_region(void);
void mpcomp_internal_GOMP_parallel_start(void (*fn)(void *), void *data,
                                         unsigned num_threads,
                                         unsigned int flags);
void mpcomp_internal_GOMP_parallel_loop_generic_start(
    void (*fn)(void *), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, long combined_pragma);
void mpcomp_internal_GOMP_parallel_loop_guided_start(
    void (*fn)(void *), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size);
void mpcomp_GOMP_parallel_loop_runtime_start(void (*fn)(void *), void *data,
                                             unsigned num_threads, long start,
                                             long end, long incr);
void mpcomp_internal_GOMP_parallel_sections_start(void (*fn)(void *),
                                                  void *data,
                                                  unsigned num_threads,
                                                  unsigned count);
void mpcomp_start_sections_parallel_region(int arg_num_threads,
                                           void *(*func)(void *), void *shared,
                                           int nb_sections);
void mpcomp_internal_GOMP_parallel_loop_runtime_start(void (*fn)(void *),
                                                      void *data,
                                                      unsigned num_threads,
                                                      long start, long end,
                                                      long incr);

#endif /* __MPCOMP_GOMP_PARALLEL_H__ */
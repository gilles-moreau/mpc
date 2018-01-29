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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPCOMP_PLACES_ENV_H__
#define __MPCOMP_PLACES_ENV_H__

#include "hwloc.h"
#include "utlist.h"
#include "sctk_debug.h"
#include "sctk_alloc.h"

typedef struct mpcomp_places_info_s
{
    unsigned int id;
    hwloc_bitmap_t interval;
    hwloc_bitmap_t logical_interval;
    struct mpcomp_places_info_s *prev, *next;
} mpcomp_places_info_t; 

hwloc_bitmap_t mpcomp_places_get_default_include_bitmap(const int );

static inline void
mpcomp_places_restrict_bitmap(hwloc_bitmap_t res, const int nb_mvps)
{
    hwloc_bitmap_t default_bitmap;
    default_bitmap = mpcomp_places_get_default_include_bitmap( nb_mvps );
    hwloc_bitmap_and( res, res, default_bitmap );
    hwloc_bitmap_free(default_bitmap);
}

static inline int
mpcomp_safe_atoi( const char* env, char* string, char** next )
{
    long retval = strtol( string, next, 10 );
    assert( retval > INT_MIN && retval < INT_MAX ); 
    if( string == *next )
    {
        fprintf(stderr, "error: missing numeric value\n");
    }
    return (int) retval;
}

static inline mpcomp_places_info_t*
mpcomp_places_infos_init( int allocate_bitmap )
{
    mpcomp_places_info_t* new_place = NULL;
    new_place = (mpcomp_places_info_t*) sctk_malloc(sizeof(mpcomp_places_info_t));
    assert( new_place );
    memset( new_place, 0, sizeof(mpcomp_places_info_t));
    if( allocate_bitmap )
    {
        new_place->interval = hwloc_bitmap_alloc();
        assert( new_place->interval );
        hwloc_bitmap_zero( new_place->interval );
    }
    return new_place; 
}

static inline hwloc_bitmap_t
mpcomp_places_build_interval_bitmap( int res, int num_places, int stride )
{
    int i;
    hwloc_bitmap_t interval;
    interval = hwloc_bitmap_alloc();
    assert(interval );
    hwloc_bitmap_zero( interval );

    for( i = 0; i < num_places; i++ )
    {
        hwloc_bitmap_set( interval, res + i * stride ); 
    } 
    return interval;
}

static inline void
mpcomp_places_merge_interval(   hwloc_bitmap_t res, 
                                const hwloc_bitmap_t include, 
                                const hwloc_bitmap_t exclude,
                                const int nb_mvps )
{

    assert( res );
    assert( include );
    assert( exclude );

    /* merge include et exclude interval */
    const int include_is_empty = !hwloc_bitmap_weight( include );
    const int exclude_is_empty = !hwloc_bitmap_weight( exclude );

    if( exclude_is_empty )
    {
        hwloc_bitmap_copy( res, include ); 
    }
    else
    {
        if( include_is_empty ) 
        {
            hwloc_bitmap_not( res, exclude );
        }
        else
        {
            hwloc_bitmap_andnot( res, include, exclude );
        }
    }
    mpcomp_places_restrict_bitmap( res, nb_mvps );
}


static inline hwloc_bitmap_t
mpcomp_places_dup_with_stride( const hwloc_bitmap_t origin, int stride, int idx, const int nb_mvps )
{
    int index;
    hwloc_bitmap_t new_bitmap;
    
    assert( origin );
    assert( stride != 0 );

    new_bitmap = hwloc_bitmap_alloc();
    assert( new_bitmap );
    hwloc_bitmap_zero( new_bitmap );

    hwloc_bitmap_foreach_begin( index, origin )
    hwloc_bitmap_set( new_bitmap, index + idx * stride );  
    hwloc_bitmap_foreach_end();
   
    mpcomp_places_restrict_bitmap( new_bitmap, nb_mvps );

    return new_bitmap;
}

static inline int 
mpcomp_places_expand_place_bitmap( mpcomp_places_info_t* list, mpcomp_places_info_t* place, int len, int stride, const int nb_mvps)
{
    int i;
    unsigned int index;
    mpcomp_places_info_t* new_place;

    assert( stride != 0 && len > 0);
    const unsigned int place_id = place->id;

    /* i = 0 is place */
    for( i = 1; i <  len; i++ )
    {
        new_place = mpcomp_places_infos_init(0);
        new_place->interval = mpcomp_places_dup_with_stride( place->interval, i, stride, nb_mvps ); 
        if( !( new_place->interval ) ) return 1;
        DL_APPEND( list, new_place ); 
        new_place->id = place_id + i;
    }
    return 0;
}

int mpcomp_places_detect_collision( mpcomp_places_info_t* list )
{
    int colision_count;
    hwloc_bitmap_t bitmap_val = hwloc_bitmap_alloc();
    char string_place[256], string_place2[256], string_place3[256];
    mpcomp_places_info_t* place, *place2, *saveptr, *saveptr2;

    colision_count = 0;
    DL_FOREACH_SAFE( list, place, saveptr )
    {
        DL_FOREACH_SAFE( saveptr, place2, saveptr2 )
        {
            if( place == place2 ) continue;
            
            hwloc_bitmap_and( bitmap_val, place->interval, place2->interval );
            if( hwloc_bitmap_intersects( place->interval, place2->interval ) )
            {
                colision_count++;
                hwloc_bitmap_list_snprintf( string_place, 255, place->interval ); 
                hwloc_bitmap_list_snprintf( string_place2, 255, place2->interval ); 
                hwloc_bitmap_list_snprintf( string_place3, 255, bitmap_val ); 
                string_place[255] = '\0'; string_place2[255] = '\0';
                fprintf(stderr, "warning: colision between place #%d (%s) and place #%d (%s) |%s|\n", place->id, string_place, place2->id, string_place2, string_place3);
            }
        }
    }
    if( colision_count )
    {
        fprintf(stderr, "warning: found %d colision(s)\n", colision_count );
    }
    return colision_count;
}

int mpcomp_places_detect_heretogeneous_places(mpcomp_places_info_t* list )
{
    int invalid_size_count;
    const int first_size = hwloc_bitmap_weight( list->interval );
    mpcomp_places_info_t* place, *saveptr;
    
    invalid_size_count = 0;
    DL_FOREACH_SAFE( list, place, saveptr )
    {
        if( first_size != hwloc_bitmap_weight( place->interval ) )
            invalid_size_count++;
    }

    if( invalid_size_count )
    {
        fprintf(stderr, "warning: Can't build regular tree\n");
    }
    return invalid_size_count;
}

#endif /* __MPCOMP_PLACES_ENV_H__*/

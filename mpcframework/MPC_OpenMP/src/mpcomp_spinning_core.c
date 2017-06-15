#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_barrier.h"
#include "sctk_atomics.h"

#include "mpcomp_spinning_core.h"

/** Reset mpcomp_team informations */
static void __mpcomp_tree_array_team_reset( mpcomp_team_t *team )
{
    sctk_assert(team);
    sctk_atomics_int *last_array_slot;
    last_array_slot = &(team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i);
    sctk_atomics_store_int(last_array_slot, MPCOMP_NOWAIT_STOP_SYMBOL);
}

static inline int __mpcomp_del_mvp_saved_context( mpcomp_mvp_t* mvp )
{
    mpcomp_mvp_saved_context_t* prev_mvp_context = NULL;

    sctk_assert( mvp );

    /* Get previous context */
    prev_mvp_context =  mvp->prev_node_father; 
    sctk_assert( prev_mvp_context );

    /* Restore MVP previous status */
    mvp->rank = prev_mvp_context->rank;
    mvp->father = prev_mvp_context->father; 
    mvp->prev_node_father = prev_mvp_context->prev;

    /* Free -- TODO: reuse mechanism */
    free( prev_mvp_context );
    
    return 0; 
} 

static inline int __mpcomp_update_mvp_saved_context( mpcomp_mvp_t* mvp, mpcomp_node_t* node, mpcomp_node_t* father )
{
    mpcomp_mvp_saved_context_t* prev_mvp_context = NULL;

    sctk_assert( mvp );
    sctk_assert( father );
    
    /* Get previous context */
    prev_mvp_context =  mvp->prev_node_father;
    assume_m( prev_mvp_context, "[%d | %d | %d | %d | %p]", mvp->global_rank, mvp->stage_rank, mvp->local_rank, mvp->rank, mvp );
   
    /* Update prev context information */
    prev_mvp_context->node = node;
    prev_mvp_context->father = mvp->father; 
    mvp->father = father;

    return 0;
}

static inline int __mpcomp_add_mvp_saved_context( mpcomp_mvp_t* mvp, const unsigned int rank )
{
    mpcomp_mvp_saved_context_t* prev_mvp_context = NULL;

    sctk_assert( mvp );

    /* Allocate new chained elt */
    prev_mvp_context = (mpcomp_mvp_saved_context_t* ) malloc( sizeof( mpcomp_mvp_saved_context_t ) );
    sctk_assert( prev_mvp_context );
    memset( prev_mvp_context, 0, sizeof( mpcomp_mvp_saved_context_t ) );

    /* Get previous context */
    prev_mvp_context->prev = mvp->prev_node_father;
    prev_mvp_context->rank = mvp->rank;
    mvp->rank = rank;
    mvp->prev_node_father = prev_mvp_context;
  
    return 0; 
}

static int __mpcomp_tree_rank_get_next_depth( mpcomp_node_t* node, const int expected_nb_mvps, int* nb_mvps )
{
    int next_depth, num_nodes; 

    /* No more nested parallelism 
     * TODO: Add OMP_NESTED value checking */
    if( node == NULL || expected_nb_mvps == 1 )
    {
        next_depth = 1; 
        num_nodes = 1;
    }
    else
    {
        for( next_depth = 0; next_depth < node->tree_depth; next_depth++ )
        {
            num_nodes = node->tree_nb_nodes_per_depth[next_depth]; 
            /* break avoid auto increment when boolean condition is true */
            if( num_nodes >= expected_nb_mvps ) break;
        }
    }
   
    *nb_mvps = num_nodes;
    return next_depth;
}

mpcomp_instance_t* 
__mpcomp_tree_array_instance_init( mpcomp_thread_t* thread, const int expected_nb_mvps )
{
    int i, j;
    mpcomp_node_t* root;
    mpcomp_instance_t* instance;

    sctk_assert( thread );
    sctk_assert( expected_nb_mvps );

    root = thread->root; 

    /* Create instance */
    instance = (mpcomp_instance_t*) malloc(sizeof(mpcomp_instance_t)); 
    sctk_assert( instance );
    memset( instance, 0, sizeof(mpcomp_instance_t) );

    /* -- Init Team information -- */
    instance->team = (mpcomp_team_t*) malloc(sizeof(mpcomp_team_t));
    sctk_assert( instance->team );
    __mpcomp_tree_array_team_reset( instance->team );

    /* -- Find next depth and real_nb_mvps -- */
    instance->tree_depth = __mpcomp_tree_rank_get_next_depth( root, expected_nb_mvps, &(instance->nb_mvps) ) +1; 
    instance->mvps = (mpcomp_mvp_t**) malloc(sizeof(mpcomp_mvp_t*)*instance->nb_mvps);
    sctk_assert( instance->mvps );
    memset( instance->mvps, 0, sizeof(mpcomp_mvp_t*) * instance->nb_mvps);

    /* -- First instance MVP  -- */
    instance->root = root;
    instance->mvps[0] = thread->mvp;

    if( instance->tree_depth > 1 )
    {
        const size_t tree_array_size = sizeof( int ) * instance->tree_depth; 

        /* -- Allocation Tree Base Array -- */
        instance->tree_base = (int*) malloc( tree_array_size );
        sctk_assert( instance->tree_base );
        memset( instance->tree_base, 0, tree_array_size );

        /* -- Allocation Tree cumulative Array -- */ 
        instance->tree_cumulative = (int*) malloc( tree_array_size );
        sctk_assert( instance->tree_cumulative );
        memset( instance->tree_cumulative, 0, tree_array_size );

        /* -- Allocation Tree Node Per Depth -- */
        instance->tree_nb_nodes_per_depth = (int*) malloc( tree_array_size ); 
        sctk_assert( instance->tree_nb_nodes_per_depth ); 
        memset( instance->tree_nb_nodes_per_depth, 0, tree_array_size );

        const int next_depth = __mpcomp_spining_get_instance_max_depth(instance);

        /* -- Instance include topology tree leaf level -- */
        if( instance->tree_depth == root->tree_depth )
        {
            /* Init instance tree array informations */
            memcpy(instance->tree_base, root->tree_base, tree_array_size);
            memcpy(instance->tree_cumulative, root->tree_cumulative, tree_array_size);
            memcpy(instance->tree_nb_nodes_per_depth, root->tree_nb_nodes_per_depth, tree_array_size);
            /* Collect instance mvps */
            mpcomp_mvp_t* cur_mvp = root->mvp;
            for( i = 0; i < instance->nb_mvps; i++, cur_mvp = cur_mvp->next_brother ) 
            {
                instance->mvps[i] = cur_mvp; 
                (void) __mpcomp_add_mvp_saved_context( cur_mvp, (unsigned int) i );
            }
        }
        else
        {
            /* Init instance tree array informations */
            memcpy(instance->tree_base, root->tree_base, tree_array_size);
            memcpy(instance->tree_nb_nodes_per_depth, root->tree_nb_nodes_per_depth, tree_array_size );

            /* Cumulative can't be just copied */
            //TODO update utils function to trigger this use
            for (i = 0; i < instance->tree_depth ; i++) 
            {
                instance->tree_cumulative[i] = 1;
                for (j = i + 1; j < instance->tree_depth; j++)
                    instance->tree_cumulative[i] *= root->tree_base[j];
            }

            /* Find First node @ root->depth + tree_depth */
            mpcomp_node_t* cur_node = root;
            for( i = 0; i < instance->tree_depth-1; i++ )
            {
                cur_node = cur_node->children.node[0];
            }

            /* Collect instance mvps */
            for( i = 0; i < instance->nb_mvps && i < expected_nb_mvps; i++, cur_node = cur_node->next_brother )
            {
                instance->mvps[i] = cur_node->mvp;
                (void) __mpcomp_add_mvp_saved_context( cur_node->mvp, (unsigned int) i );
            }
        }
    }
    return instance;
}

void __mpcomp_wakeup_mvp( mpcomp_mvp_t *mvp) 
{
    mpcomp_local_icv_t icvs;
    mpcomp_thread_t* new_thread;

    /* Sanity check */
    sctk_assert(mvp);

    new_thread = (mpcomp_thread_t*) malloc( sizeof( mpcomp_thread_t) );
    sctk_assert( new_thread );
    memset( new_thread, 0, sizeof( mpcomp_thread_t ) );

    new_thread->father = mvp->threads;
    new_thread->info.num_threads = 1; 
    new_thread->instance = mvp->instance;

#if MPCOMP_TRANSFER_INFO_ON_NODES
    new_thread->info = mvp->info;
#else /* MPCOMP_TRANSFER_INFO_ON_NODES */
    new_thread->info = mvp->instance->team->info;
#endif

    /* Set thread rank */
    new_thread->rank = mvp->rank;
    mvp->threads = new_thread;
    //new_thread->root = mvp->father;
    new_thread->root = (mvp->prev_node_father ) ? mvp->prev_node_father->node : mvp->father;// mvp->father; //TODO just for test
    new_thread->mvp = mvp;
    mvp->slave_running = MPCOMP_MVP_STATE_READY;
}

void __mpcomp_wakeup_leaf( mpcomp_node_t* start_node, const int num_threads )
{
    int i, nb_children_involved;
    mpcomp_mvp_t **leaves_array;

    /*TODO:  Remove global variable */
    const int mpcomp_affinity = mpcomp_global_icvs.affinity; 
    leaves_array = start_node->children.leaf;

    if( start_node->child_type == MPCOMP_CHILDREN_NODE )
        return;

    /* Wake up children leaf */
    //sctk_assert( start_node->child_type == MPCOMP_CHILDREN_LEAF );


#if 0
    /* Count children number involved */
    for( i = 1, nb_children_involved = 1; i < start_node->nb_children; i++ )
    {
        const int leaf_rank = leaves_array[i]->min_index[mpcomp_affinity];
        if( leaf_rank < num_threads ) nb_children_involved++;
    }

    start_node->barrier_num_threads = nb_children_involved;

    for( i = 1; i < start_node->nb_children; i++ )
    {
        const int leaf_rank = leaves_array[i]->min_index[mpcomp_affinity];
        if( leaf_rank < num_threads )
        {
            leaves_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE;
            
        }
    }  
#else
    //assert( start_node->nb_children <= num_threads ); 
    start_node->barrier_num_threads = num_threads;
    for( i = 0; i < num_threads; i++ )
    {
        __mpcomp_update_mvp_saved_context( leaves_array[i], start_node, start_node );
        leaves_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE;
    }
#endif
}

mpcomp_node_t*  __mpcomp_wakeup_node( mpcomp_node_t* start_node, const int num_threads )
{
    int i, j, nb_children_involved;
    mpcomp_node_t *current_node;
    mpcomp_node_t **nodes_array; 

    current_node = start_node;

    if( current_node->child_type != MPCOMP_CHILDREN_NODE )
    {
        return current_node;
    }

    nodes_array = start_node->children.node;
#if 0
    /*TODO:  Remove global variable */
    const int mpcomp_affinity = mpcomp_global_icvs.affinity; 

    for( i = 0; i < start_node->instance->tree_depth; i++ )
    {
        for( j = 1, nb_children_involved = 1; j < current_node->nb_children; j++ )
        {
            const int node_rank = nodes_array[j]->min_index[mpcomp_affinity];
            if( node_rank < num_threads ) nb_children_involved++;
        }

        current_node->barrier_num_threads = nb_children_involved; 

        for( j = 1, nb_children_involved = 1; j < current_node->nb_children; j++ ) 
        {
            const int node_rank = nodes_array[j]->min_index[mpcomp_affinity];
            if( node_rank < num_threads )
            {
                nb_children_involved++;
                nodes_array[j]->slave_running = MPCOMP_MVP_STATE_AWAKE;
            }
        }


        /* Next level must be compute by __mpcomp_wakeup_leaf */ 
        if( current_node->child_type != MPCOMP_CHILDREN_NODE )
            break;

        current_node = current_node->children.node[0];
    }
#else
    assert( start_node->nb_children <= num_threads );
    start_node->barrier_num_threads = num_threads;

    for( i = 1; i < num_threads; i++ )
    {
        nodes_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE; 
    }
#endif
    
    return nodes_array[0];

}

void __mpcomp_wakeup_gen_node( mpcomp_node_t* start_node, const int num_threads )
{
    mpcomp_mvp_t* mvp;
    mpcomp_instance_t* instance;
    mpcomp_node_t* current_node, *old, *new;

    sctk_assert( start_node );

    instance = start_node->instance;

    sctk_assert( instance ); 
    sctk_assert( instance->root );

    mvp = start_node->mvp;
    sctk_assert( mvp );
    const int instance_max_depth = instance->tree_depth + instance->root->depth; 

    /* Continue Wake-Up */
    if( start_node->depth+1 < instance_max_depth )
    {
        current_node = __mpcomp_wakeup_node( start_node, num_threads );
    }
    else
    {
        current_node = start_node;
    } 
   
     
    mvp = current_node->mvp;
    if( current_node->depth+1 < instance_max_depth )
    {
        __mpcomp_wakeup_leaf( current_node, num_threads );
    }
    else
    {
        /* Allocate new chained elt */
        __mpcomp_update_mvp_saved_context( current_node->mvp, current_node, current_node->father );
    }

}

void __mpcomp_start_openmp_thread( mpcomp_mvp_t *mvp )
{
    mpcomp_thread_t* cur_thread = NULL;

    sctk_assert( mvp );
    __mpcomp_wakeup_mvp( mvp );

    sctk_openmp_thread_tls = mvp->threads;
    sctk_assert( sctk_openmp_thread_tls );
    cur_thread = (mpcomp_thread_t*) sctk_openmp_thread_tls; 

    /* Start parallel region */
    __mpcomp_in_order_scheduler( sctk_openmp_thread_tls );

    /* Implicite barrier */
    __mpcomp_internal_half_barrier( mvp );

    /* End barrier for master thread */
    if( !( cur_thread->rank ) )
    {
        mpcomp_node_t* root = cur_thread->instance->root; 
        const int expected_num_threads = root->barrier_num_threads;
        while (sctk_atomics_load_int(&(root->barrier)) != expected_num_threads) 
            sctk_thread_yield();
        
       sctk_atomics_store_int( &( root->barrier ), 0); 
    }
    else
    {
        mpcomp_node_t* root = cur_thread->instance->root; 
    }


    sctk_openmp_thread_tls = mvp->threads->father;
    mvp->threads = mvp->threads->father;

    ( void ) __mpcomp_del_mvp_saved_context( mvp );

    if( sctk_openmp_thread_tls )
    {
        mpcomp_thread_t* test = (mpcomp_thread_t*) sctk_openmp_thread_tls;
        mvp->instance = test->instance;
    }    
 
    /* Free previous thread */
    free( cur_thread );
    
}

/**
  Entry point for microVP working on their own
  Spinning on a variables inside the microVP.
 */
void mpcomp_slave_mvp_leaf( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    assert( mvp );
    volatile int* spinning_val = &( mvp->slave_running ) ;

    /* Sanity check */
    assert( *spinning_val == MPCOMP_MVP_STATE_UNDEF );

    /* Spin while this microVP is alive */
    while( mvp->enable ) 
    {
        /* MVP pause status */
        *spinning_val = MPCOMP_MVP_STATE_SLEEP;
        /* Spin for new parallel region */
        sctk_thread_wait_for_value_and_poll( spinning_val, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
        mvp->instance = mvp->father->instance;
        __mpcomp_start_openmp_thread( mvp );
    }
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void mpcomp_slave_mvp_node( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    int num_threads, rest, quot, father_num_children;
    mpcomp_node_t* traversing_node = NULL;


    sctk_assert( mvp );
    volatile int* spinning_val = &( spinning_node->slave_running );
    sctk_assert( *spinning_val == MPCOMP_MVP_STATE_UNDEF );

    while( mvp->enable ) 
    {
        *spinning_val = MPCOMP_MVP_STATE_SLEEP;
        /* Spinning on the designed node */
        sctk_thread_wait_for_value_and_poll( spinning_val, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
        spinning_node->instance = spinning_node->father->instance;
#if MPCOMP_TRANSFER_INFO_ON_NODES
        spinning_node->info = spinning_node->father->info;
        num_threads = spinning_node->info.num_threads;
#else   /* MPCOMP_TRANSFER_INFO_ON_NODES */
        num_threads = spinning_node->instance->team->info.num_threads;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
        father_num_children = spinning_node->father->nb_children;
        rest = num_threads % father_num_children;
        quot = num_threads / father_num_children; 
        num_threads = quot; // MANAGE non multiple number
	    sctk_assert( num_threads > 0 ) ;
	    /* -- Wake up children nodes -- */
        
        __mpcomp_wakeup_gen_node( spinning_node, num_threads );
        /* -- Start Parallel Region -- */
        mvp->instance = spinning_node->instance;
       __mpcomp_start_openmp_thread( mvp );
    }
}

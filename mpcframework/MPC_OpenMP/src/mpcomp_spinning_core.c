
/* Forward declaration */
void __mpcomp_in_order_scheduler( mpcomp_mvp_t *mvp );
void __mpcomp_internal_half_barrier( mpcomp_mvp_t *mvp );

static void __mpcomp_tree_array_team_reset( mpcomp_team_t *team )
{
    sctk_assert(team);
    sctk_atomics_int_t *for_dyn_nb_threads_exited;
    last_array_slot = &(team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i)
    sctk_atomics_store_int(last_array_slot, MPCOMP_NOWAIT_STOP_SYMBOL);
}

static int __mpcomp_tree_rank_get_next_depth( mpcomp_node_t* node, const int expected_nb_mvps, int* nb_mvps )
{
    int next_depth; 

    if( node = NULL || expected_nb_mvps == 1 )
    {
        next_depth = 1;
        *nb_mvps = expected_nb_mvps;
    }
    else
    {
        for( i = 0; i < node->tree_depth; i++ )
        {
            if( node->tree_nb_nodes_per_depth[i] >= expected_nb_mvps )
                break;
        }
        next_depth = i;
        *nb_mvps = node->tree_nb_nodes_per_depth[next_depth];
    }
    
    return next_depth;
}

mpcomp_instance_t* 
__mpcomp_tree_array_instance_init( mpcomp_thread_t* thread, const int expected_nb_mvps )
{
    mpcomp_node_t* root;
    mpcomp_instance_t* instance;

    sctk_assert( thread );
    sctk_assert( expected_nb_mvps );

    root = thread->root; 

    /* Create instance */
    instance = (mpcomp_instance_t*) malloc(sizeof(mpcomp_instance_t)); 
    sctk_assert( instance );
    memset( instance, 0, sizeof(mpcomp_instance_t*) );

    /* -- Init Team information -- */
    instance->team = (mpcomp_team_t*) malloc(sizeof(mpcomp_team_t));
    sctk_assert( instance->team );
    __mpcomp_tree_array_team_reset( instance->team );

    /* -- Find next depth and real_nb_mvps -- */
    instance->tree_depth = __mpcomp_tree_rank_get_next_depth( root, expected_nb_mvps, &(instance->nb_mvps) ); 
    sctk_assert( instance->tree_depth > 0 );
    instance->mvps = (mpcomp_mvp_t**) malloc(sizeof(mpcomp_mvp_t*)*instance->nb_mvps);
    sctk_assert( instance->mvps );
    memset( instance->mvps, 0, sizeof(mpcomp_mvp_t*) * instance->nb_mvps);

    /* -- First instance MVP  -- */
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

        /* -- Instance include topology tree leaf level -- */
        if(  next_depth == root->tree_depth )
        {
            /* Init instance tree array informations */
            memcpy(instance->tree_base, root->tree_base, tree_array_size);
            memcpy(instance->tree_cumulative, root->tree_cumulative, tree_array_size);
            memcpy(instance->tree_nb_nodes_per_depth, root->tree_nb_nodes_per_depth, tree_array_size);
            /* Collect instance mvps */
            mpcomp_mvp_t* cur_mvp = root->mvp;
            for( i = 1; i < instance->nb_mvps; i++; cur_mvp = cur_mvp->next ) 
            {
                instance->mvps[i] = cur_mvp; 
                /* Enqueue instance mvp father */
                cur_mvp->tree_array_father->prev_father = cur_mvp->father;
                cur_mvp->father = cur_mvp->tree_array_father;
            }
            sctk_assert( cur_mvp == root->mvp );
        }
        else
        {
            /* Init instance tree array informations */
            memcpy(instance->tree_base, root->tree_base, tree_array_size);
            /* -- From mpcomp_tree.c -- */
            for (i = 0; i < instance->tree_depth - 1; i++)
            {
                instance->tree_cumulative[i] = 1;
                for (j = i + 1; j < depth; j++) 
                    instance->tree_cumulative[i] *= instance->tree_base[j];
            }
            instance->tree_nb_nodes_per_depth[0] = 1;
            for (i = 1; i <= depth; i++)
            {
                instance->tree_nb_nodes_per_depth[i] = instance->tree_nb_nodes_per_depth[i - 1];
                instance->tree_nb_nodes_per_depth[i] *= instance->tree_base[i - 1];    
            }
            /* Find First node @ root->depth + tree_depth */
            mpcomp_node_t* cur_node = root;
            for( i = 0; i < tree_depth; i++ )
                cur_node = cur_node->children[0];
            /* Collect instance mvps */
            for( i = 1; i < instance->nb_mvps; i++; cur_node = cur_node->next_brother )
            {
                instance->mvps[i] = cur_node->mvp;
                /* Enqueue instance mvp father */
                cur_node->prev_father = instance->mvps[i]->father;
                instance->mvps[i]->cur_node;
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
    new_thread->rank = mvp->min_index[mpcomp_global_icvs.affinity];
    mvp->threads = new_thread;
}

void __mpcomp_start_openmp_thread( mpcomp_mvp_t *mvp )
{
    mpcomp_thread_t* to_free = NULL;

    sctk_assert( mvp );
    __mpcomp_wakeup_mvp( mvp );

    sctk_openmp_thread_tls = mvp->threads;
    sctk_assert( sctk_openmp_thread_tls );

    /* Start parallel region */
    __mpcomp_in_order_scheduler( mvp );

    to_free = sctk_openmp_thread_tls; 
    sctk_openmp_thread_tls = mvp->threads->father;
    sctk_assert( sctk_openmp_thread_tls );

    /* Free previous thread */
    free( to_free );
    
    /* Implicite barrier */
    __mpcomp_internal_half_barrier( mvp );
}

/**
  Entry point for microVP working on their own
  Spinning on a variables inside the microVP.
 */
void mpcomp_slave_mvp_leaf( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    assert( mvp && !spinning_node );
    volatile int* spinning_val = mvp->slave_running;

    /* Spin while this microVP is alive */
    while( mvp->enable ) 
    {
        /* Spin for new parallel region */
        sctk_thread_wait_for_value_and_poll( spinning_val, 1, NULL, NULL ) ;
        *spinning_val = 0;
        __mpcomp_start_openmp_thread( mvp );
    }
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void mpcomp_slave_mvp_node( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    mpcomp_node_t* traversing_node = NULL;
    sctk_assert( mvp && spinning_node ) ;

    volatile int* spinning_val = &( spinning_node->slave_running );

    while( mvp->enable ) 
    {
        /* Spinning on the designed node */
        sctk_thread_wait_for_value_and_poll( spinning_val, 1, NULL, NULL ) ;
        *spinning_val = 0;
#if MPCOMP_TRANSFER_INFO_ON_NODES
	    num_threads = n->father->info.num_threads;
#else   /* MPCOMP_TRANSFER_INFO_ON_NODES */
	    num_threads = n->instance->team->info.num_threads;
#endif  /* MPCOMP_TRANSFER_INFO_ON_NODES */
	    sctk_assert( num_threads > 0 ) ;
	    /* Wake up children nodes */
	    node = __mpcomp_wakeup_node( 0, node, num_threads, node->instance );
        /* Wake up children leaf */
	    node = __mpcomp_wakeup_leaf( node, num_threads, node->instance );
        /* Start Parallel Region */
        __mpcomp_start_openmp_thread( mvp );
    }
}

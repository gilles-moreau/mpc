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
/* #                                                                      # */
/* ######################################################################## */

/********************************* INCLUDES *********************************/
#include "sctk_communicator.h"
#include "sctk.h"
#include "sctk_handle.h"
#include "sctk_pmi.h"
#include "utarray.h"
#include <opa_primitives.h>
#include <uthash.h>
#include "sctk_handle.h"

typedef struct sctk_process_ht_s
{
	int process_id;
	UT_hash_handle hh;
} sctk_process_ht_t;

/************************* GLOBALS *************************/
/** communicators table **/
static sctk_internal_communicator_t *sctk_communicator_table = NULL;
static sctk_internal_communicator_t *sctk_communicator_array[SCTK_MAX_COMMUNICATOR_TAB];
/** spinlock for communicators table **/
static sctk_spin_rwlock_t sctk_communicator_local_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static sctk_spinlock_t sctk_communicator_all_table_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_spin_rwlock_t sctk_process_from_rank_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
extern volatile int sctk_online_program;

/************************* FUNCTION ************************/
/**
 * This method is used to check if the communicator "communicator" exist.
 * @param communicator Identification number of the communicator.
 * @return the communicator structure corresponding to the identification number
 * or NULL if it doesn't exist.
**/
static inline sctk_internal_communicator_t *sctk_check_internal_communicator_no_lock ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp = NULL;

	assert ( communicator >= 0 );

	if ( communicator >= SCTK_MAX_COMMUNICATOR_TAB )
	{
          while (
              sctk_spinlock_read_trylock(&sctk_communicator_local_table_lock)) {
            mpc_thread_yield();
          }
          //~ check in the hash table
          HASH_FIND(hh, sctk_communicator_table, &communicator,
                    sizeof(sctk_communicator_t), tmp);
          sctk_spinlock_read_unlock(&sctk_communicator_local_table_lock);
        } else {
          //~ else check in the table
          tmp = sctk_communicator_array[communicator];
        }

        return tmp;
}

/************************* FUNCTION ************************/
/**
 * This method is used to check if the communicator "communicator" exist.
 * It calls the function sctk_check_internal_communicator() and check for errors.
 * @param communicator Identification number of the communicator.
 * @return the communicator structure corresponding to the identification number
 * or exit the function after displaying error.
**/
static inline sctk_internal_communicator_t *sctk_get_internal_communicator ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_check_internal_communicator_no_lock ( communicator );

	if ( tmp == NULL )
	{
		sctk_error ( "Communicator %d does not exist", communicator );

		if ( sctk_online_program == 0 )
		{
			exit ( 0 );
		}

		not_reachable();
	}

	return tmp;
}

/**
 * This method is used to retrieve the collective object
 * @param communicator Identification number of the communicator.
 * @return the collective communicator structure corresponding to the
 *identification number
**/
struct sctk_comm_coll *
__sctk_communicator_get_coll(const sctk_communicator_t communicator) {
  sctk_internal_communicator_t *ret =
      sctk_get_internal_communicator(communicator);

  if (!ret) {
    return NULL;
  }

  if (!ret->is_shared_mem)
    return NULL;

  return &ret->coll;
}

/************************* FUNCTION ************************/
/**
 * This method is used to set a communicator in the communicator table.
 * This method don't use spinlocks.
 * @param id Identification number of the communicator.
 * @param tmp Structure of the new communicator.
**/
static inline int sctk_set_internal_communicator_no_lock_no_check ( const sctk_communicator_t id, sctk_internal_communicator_t *tmp )
{
	if ( id >= SCTK_MAX_COMMUNICATOR_TAB )
	{
		sctk_spinlock_write_lock ( &sctk_communicator_local_table_lock );
		//add in the hash table
		HASH_ADD ( hh, sctk_communicator_table, id, sizeof ( sctk_communicator_t ), tmp );
		sctk_spinlock_write_unlock ( &sctk_communicator_local_table_lock );
	}
	else
	{
		//else add in the table
		sctk_nodebug("COMM %d added in table with remote comm %s", id, (tmp->remote_comm == NULL) ? "NULL" : "not NULL");
		sctk_communicator_array[id] = tmp;
	}

        sctk_handle_new_from_id(id, SCTK_HANDLE_COMM);

        return 0;
}

/************************* FUNCTION ************************/
/**
 * This method is used to set a communicator in the communicator table.
 * It calls the method sctk_set_internal_communicator_no_lock_no_check,
 * after checking the entry id
 * @param id Identification number of the communicator.
 * @param tmp Structure of the new communicator.
**/
static inline int sctk_set_internal_communicator_no_lock ( const sctk_communicator_t id, sctk_internal_communicator_t *tmp )
{
	sctk_internal_communicator_t *tmp_check;
	tmp_check = sctk_check_internal_communicator_no_lock ( id );

	//check if it exists
	if ( tmp_check != NULL )
	{
		return 1;
	}

	//insert communicator
	sctk_set_internal_communicator_no_lock_no_check ( id, tmp );
	return 0;
}

/************************* FUNCTION ************************/
/**
 * This method is used to set a communicator in the communicator table.
 * It calls the method sctk_set_internal_communicator_no_lock_no_check,
 * protected by spinlocks.
 * @param id Identification number of the communicator.
 * @param tmp Structure of the new communicator.
**/
static inline int sctk_set_internal_communicator ( const sctk_communicator_t id, sctk_internal_communicator_t *tmp )
{
	sctk_internal_communicator_t *tmp_check;
	//use spinlocks
	sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
	tmp_check = sctk_check_internal_communicator_no_lock ( id );

	//check if it exists
	if ( tmp_check != NULL )
	{
		sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
		return 1;
	}

	//insert communicator
	sctk_set_internal_communicator_no_lock_no_check ( id, tmp );
	sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
	return 0;
}

/************************* FUNCTION ************************/
/**
 * This method is used to delete a communicator in the communicator table.
 * @param id Identification number of the communicator.
**/
static inline int sctk_del_internal_communicator_no_lock_no_check ( const sctk_communicator_t id )
{
	sctk_internal_communicator_t *tmp;

	if ( id >= SCTK_MAX_COMMUNICATOR_TAB )
	{
		sctk_spinlock_write_lock ( &sctk_communicator_local_table_lock );
		//check if it exists
		HASH_FIND ( hh, sctk_communicator_table, &id, sizeof ( sctk_communicator_t ), tmp );

		//if not, exit
		if ( tmp == NULL )
		{
			sctk_spinlock_write_unlock ( &sctk_communicator_local_table_lock );
			return 0;
		}

		//delete in the hash table
		HASH_DELETE ( hh, sctk_communicator_table, tmp );
                sctk_error("COMM %d DEL in HAS_TAB", id);
                sctk_spinlock_write_unlock(&sctk_communicator_local_table_lock);
        } else {
          // else delete in the table
          sctk_communicator_array[id] = NULL;
        }

        sctk_handle_free(id, SCTK_HANDLE_COMM);

        return 0;
}

/************************* FUNCTION ************************/
/**
 * This method determine if the global rank is contained in the local group or the remote group
 * @param communicator Identification number of the inter-communicator.
 * @param rank global rank
**/
int sctk_is_in_local_group_rank ( const sctk_communicator_t communicator , int comm_world_rank )
{
	int i = 0;
	
	sctk_internal_communicator_t *tmp;

	if ( communicator == SCTK_COMM_WORLD )
		return 1;

	tmp = sctk_get_internal_communicator ( communicator );

	assume ( tmp != NULL );

	if ( tmp->local_to_global == NULL )
		return 1;

	for ( i = 0 ; i < tmp->nb_task ; i++ )
	{
		if ( tmp->local_to_global[i] == comm_world_rank )
		{
			return 1;
		}
	}

	if ( tmp->is_inter_comm == 1 )
	{
		assume ( tmp->remote_comm != NULL );

		for ( i = 0 ; i < tmp->remote_comm->nb_task ; i++ )
		{
			if ( tmp->remote_comm->local_to_global[i] == comm_world_rank )
			{
				return 0;
			}
		}
	}

	return -1;
}

/************************* FUNCTION ************************/
/**
 * This method determine if the global rank is contained in the local group or the remote group
 * @param communicator Identification number of the inter-communicator.
**/
int sctk_is_in_local_group ( const sctk_communicator_t communicator )
{
	int comm_world_rank = sctk_get_task_rank();
	
	assume( 0 <= comm_world_rank );
	
	return sctk_is_in_local_group_rank( communicator, comm_world_rank );
}

/************************* FUNCTION ************************/
/**
 * This method get the identification number of the local intracommunicator.
 * @param communicator Identification number of the local intercommunicator.
 * @return the identification number.
**/
sctk_communicator_t sctk_get_local_comm_id ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	assume ( tmp != NULL );

	//check if intercommunicator
	if ( tmp->is_inter_comm == 1 )
	{
		int result = sctk_is_in_local_group ( communicator );

		if ( result == 0 )
		{
			return tmp->remote_comm->local_id;

		}
		else
		{
			return tmp->local_id;
		}
	}
	else
	{
		return tmp->id;
	}
}

static inline void sctk_communicator_intern_write_lock ( sctk_internal_communicator_t *tmp )
{
#ifndef SCTK_MIGRATION_DISABLED

	if ( sctk_migration_mode )
		sctk_spinlock_write_lock ( & ( tmp->lock ) );

#endif
}

static inline void sctk_communicator_intern_write_unlock ( sctk_internal_communicator_t *tmp )
{
#ifndef SCTK_MIGRATION_DISABLED

	if ( sctk_migration_mode )
		sctk_spinlock_write_unlock ( & ( tmp->lock ) );

#endif
}

static inline void sctk_communicator_intern_read_lock ( sctk_internal_communicator_t *tmp )
{
#ifndef SCTK_MIGRATION_DISABLED

	if ( sctk_migration_mode )
		sctk_spinlock_read_lock ( & ( tmp->lock ) );

#endif
}

static inline void sctk_communicator_intern_read_unlock ( sctk_internal_communicator_t *tmp )
{
#ifndef SCTK_MIGRATION_DISABLED

	if ( sctk_migration_mode )
		sctk_spinlock_read_unlock ( & ( tmp->lock ) );

#endif
}

static inline void sctk_comm_reduce ( const sctk_communicator_t *mpc_restrict  in , sctk_communicator_t *mpc_restrict inout , size_t size , sctk_datatype_t datatype )
{
	size_t i;

	for ( i = 0; i < size; i++ )
	{
		if ( inout[i] > in[i] )
			inout[i] = in[i];
	}
}

/************************* FUNCTION ************************/
/**
 * This method get a new identification number for the new intracommunicator.
 * @param local_root if the task is considered as root in its group.
 * @param rank Rank of the task.
 * @param origin_communicator communicator used to create the new one.
 * @param tmp structure of the new communicator.
 * @return the identification number of the new communicator.
**/
static inline sctk_communicator_t sctk_communicator_get_new_id ( int local_root, int rank, const sctk_communicator_t origin_communicator, sctk_internal_communicator_t *tmp )
{
	sctk_communicator_t comm = -1;
	sctk_communicator_t i = 0;
	sctk_communicator_t ti = 0;
	int need_clean = 0;

	sctk_nodebug ( "get new id local_root %d, rank %d, origin_comm %d", local_root, rank, origin_communicator );

	do
	{
		need_clean = 0;

		if ( rank == 0 )
		{
			sctk_internal_communicator_t *tmp_check;

			if ( ( i == SCTK_COMM_WORLD ) || ( i == SCTK_COMM_SELF ) )
				i++;

			if ( ( i == SCTK_COMM_WORLD ) || ( i == SCTK_COMM_SELF ) )
				i++;

			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			i--;

			do
			{
				i++;
				sctk_nodebug ( "rank %d : First check comm %d", rank, i );
				tmp_check = sctk_check_internal_communicator_no_lock ( i );
				sctk_nodebug ( "rank %d : comm %d checked", rank, i );
			}
			while ( tmp_check != NULL );

			sctk_nodebug ( "rank %d : comm %d chosen", rank, i );
			comm = i;
			tmp->id = comm;
			sctk_set_internal_communicator_no_lock ( comm, tmp );
			sctk_nodebug ( "rank %d : Try comm %d", rank, comm );
			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
		}

		/* Broadcast comm */
		sctk_broadcast ( &comm, sizeof ( sctk_communicator_t ), 0, origin_communicator );

		if ( ( rank != 0 ) && ( local_root == 1 ) )
		{
			if ( comm != -1 )
			{
				/*Check if available*/
				tmp->id = comm;
				sctk_internal_communicator_t *tmp_check;
				sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
				sctk_nodebug ( "rank %d : check comm %d", rank, comm );
				tmp_check = sctk_check_internal_communicator_no_lock ( comm );

				if ( tmp_check != NULL )
				{
					comm = -1;
				}
				else
				{
					sctk_set_internal_communicator_no_lock ( comm, tmp );
					need_clean = 1;
				}

				sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
			}
		}

		/* Check if available every where*/
		ti = comm;
		sctk_all_reduce ( &ti, &comm, sizeof ( sctk_communicator_t ), 1, ( sctk_Op_f ) sctk_comm_reduce, origin_communicator, 0 );

		if ( ( comm == -1 ) && ( need_clean == 1 ) && ( local_root == 1 ) )
		{
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			sctk_del_internal_communicator_no_lock_no_check ( ti );
			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
		}

	}
	while ( comm == -1 );

	sctk_nodebug ( "rank %d : comm %d accepted", rank, comm );
	assume ( comm != SCTK_COMM_WORLD );
	assume ( comm != SCTK_COMM_SELF );
	assume ( comm > 0 );
	return comm;
}

static inline int sctk_communicator_compare ( sctk_internal_communicator_t *comm1, sctk_internal_communicator_t *comm2 )
{
	int i = 0;

	if ( comm1->is_inter_comm != comm2->is_inter_comm )
	{
		sctk_nodebug ( "is_inter different" );
		return 0;
	}

	if ( comm1->id != comm2->id )
	{
		sctk_nodebug ( "id different" );
		return 0;
	}

	if ( comm1->nb_task != comm2->nb_task )
	{
		sctk_nodebug ( "nb_task different" );
		return 0;
	}

	for ( i = 0 ; i < comm1->nb_task ; i++ )
	{
		if ( comm1->local_to_global[i] != comm2->local_to_global[i] )
		{
			sctk_nodebug ( "local_to_global[%d] different", i );
			return 0;
		}
	}

	if ( comm1->is_inter_comm == 1 )
	{
		if ( comm1->peer_comm != comm2->peer_comm )
		{
			sctk_nodebug ( "peer_comm different" );
			return 0;
		}

		if ( comm1->local_leader != comm2->local_leader )
		{
			sctk_nodebug ( "local_leader different" );
			return 0;
		}

		if ( comm1->remote_leader != comm2->remote_leader )
		{
			sctk_nodebug ( "remote_leader different" );
			return 0;
		}
	}

	if ( comm1->remote_comm != NULL && comm2->remote_comm != NULL )
	{
		if ( comm1->remote_comm->nb_task != comm2->remote_comm->nb_task )
		{
			sctk_nodebug ( "remote_comm->nb_task different" );
			return 0;
		}

		for ( i = 0 ; i < comm1->remote_comm->nb_task ; i++ )
		{
			if ( comm1->remote_comm->local_to_global[i] != comm2->remote_comm->local_to_global[i] )
			{
				sctk_nodebug ( "remote_comm->local_to_global[%d] different", i );
				return 0;
			}
		}

		if ( comm1->remote_comm->local_leader != comm2->remote_comm->local_leader )
		{
			sctk_nodebug ( "remote_comm->local_leader different" );
			return 0;
		}

		if ( comm1->remote_comm->remote_leader != comm2->remote_comm->remote_leader )
		{
			sctk_nodebug ( "remote_comm->remote_leader different" );
			return 0;
		}
	}

	return 1;

}

/************************* FUNCTION ************************/
/**
 * This method get a new identification number for the new intercommunicator.
 * @param local_root if the task is considered as root in its group.
 * @param rank Rank of the task.
 * @param local_leader root rank of the local group.
 * @param remote_leader root rank of the remote_group.
 * @param origin_communicator communicator used to create the new intercommunicator.
 * @param tmp structure of the new communicator.
 * @return the identification number of the new communicator.
**/
static inline sctk_communicator_t sctk_intercommunicator_get_new_id ( int local_root, int rank,
                                                                      int local_leader, int remote_leader, const sctk_communicator_t origin_communicator, const sctk_communicator_t peer_comm, sctk_internal_communicator_t *tmp )
{
	sctk_internal_communicator_t *tmp_check;
	sctk_communicator_t comm = -1;
	sctk_communicator_t remote_comm = -1;
	sctk_communicator_t i = 0;
	sctk_communicator_t ti = 0;
	sctk_communicator_t remote_ti = 0;
	sctk_status_t status;
	int wrank;
	int need_clean;
	int change_i = 1;
	int tag = 628;
	wrank = sctk_get_task_rank();
	
	sctk_nodebug ( "rank %d : get new id for intercomm, local_leader = %d, remote_leader = %d, local_root = %d, peer_comm = %d", rank, local_leader, remote_leader, local_root, peer_comm );

	int remote_leader_rank = sctk_get_comm_world_rank ( peer_comm, remote_leader );

	do
	{
		need_clean = 0;

		if ( rank == local_leader )
		{
			//~ pass comm world && comm self
			if ( ( i == SCTK_COMM_WORLD ) || ( i == SCTK_COMM_SELF ) )
				i++;

			if ( ( i == SCTK_COMM_WORLD ) || ( i == SCTK_COMM_SELF ) )
				i++;

			//~ take the first NULL
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			i--;

			do
			{
				i++;
				tmp_check = sctk_check_internal_communicator_no_lock ( i );
			}
			while ( tmp_check != NULL );

			comm = i;
				sctk_nodebug("rank %d : FIRST try intercomm %d", rank, comm);
			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
			//~ exchange comm between leaders
			sctk_sendrecv( &comm, sizeof(int), remote_leader_rank, tag, &remote_comm,  remote_leader_rank, SCTK_COMM_WORLD );
			
			if ( comm != -1 )
			{
				if ( remote_comm > comm )
					comm = remote_comm;
			}
			else
				comm = remote_comm;

			tmp->id = comm;
			tmp->remote_comm->id = comm;
			sctk_nodebug("rank %d : SECOND try intercomm %d", rank, comm);
			//~ re-check
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
				tmp_check = sctk_check_internal_communicator_no_lock(comm);

				if(tmp_check != NULL)
				{
					if(!sctk_communicator_compare(tmp_check, tmp))
						comm = -1;
				}	
				
			if ( comm != -1 )
			{
				tmp->id = comm;
				tmp->remote_comm->id = comm;
				sctk_set_internal_communicator_no_lock ( comm, tmp );
			}

			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
			sctk_nodebug("rank %d : THIRD try intercomm %d", rank, comm);
		}

		/* Broadcast comm to local group members*/
		sctk_nodebug("rank %d : broadcast comm %d", rank, origin_communicator);
		sctk_broadcast (&comm,sizeof(sctk_communicator_t),local_leader,origin_communicator);
		sctk_nodebug("rank %d : Every one try %d", rank, comm);

		if ( ( rank != local_leader ) && ( local_root == 1 ) )
		{
			if ( comm != -1 )
			{
				/*Check if available*/
				sctk_internal_communicator_t *tmp_check;
				tmp->id = comm;
				tmp->remote_comm->id = comm;
				sctk_nodebug ( "Check intercomm %d", comm );
				sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
				tmp_check = sctk_check_internal_communicator_no_lock ( comm );

				if ( tmp_check != NULL )
				{
					if ( !sctk_communicator_compare ( tmp_check, tmp ) )
						comm = -1;
				}
				else
				{
					need_clean = 1;
					sctk_set_internal_communicator_no_lock ( comm, tmp );
				}

				sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
			}
		}

		sctk_nodebug ( "Every one try %d allreduce %d", comm, origin_communicator );
		/* Inter-Allreduce */
		ti = comm;
		//~ allreduce par groupe
		sctk_all_reduce ( &ti, &comm, sizeof ( sctk_communicator_t ), 1, ( sctk_Op_f ) sctk_comm_reduce, origin_communicator, 0 );

		//~ echange des resultats du allreduce entre leaders
		if ( rank == local_leader )
		{
			sctk_sendrecv( &ti, sizeof(int), remote_leader_rank, tag, &remote_ti,  remote_leader_rank, SCTK_COMM_WORLD );			
		}
		//~ broadcast aux autres membres par groupe
		sctk_broadcast (&remote_ti,sizeof(sctk_communicator_t),local_leader,origin_communicator);

		//~ on recupère le plus grand des resultats ou -1
		if ( comm != -1 && remote_ti != -1 )
		{
			if ( remote_ti > comm )
				comm = remote_ti;
		}
		else
			comm = -1;

		if ( ( comm == -1 ) && ( need_clean == 1 ) && ( local_root == 1 ) )
		{
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			sctk_del_internal_communicator_no_lock_no_check ( ti );
			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
		}
	}
	while ( comm == -1 );

	sctk_nodebug ( "rank %d : finished intercomm id = %d", rank, comm );
	assume ( comm != SCTK_COMM_WORLD );
	assume ( comm != SCTK_COMM_SELF );
	assume ( comm > 0 );
	return comm;
}

/************************* FUNCTION ************************/
/**
 * This method get a new identification number for the new intercommunicator.
 * @param local_root if the task is considered as root in its group.
 * @param rank Rank of the task.
 * @param local_leader root rank of the local group.
 * @param remote_leader root rank of the remote_group.
 * @param origin_communicator communicator used to create the new intercommunicator.
 * @param tmp structure of the new communicator.
 * @return the identification number of the new communicator.
**/
static inline sctk_communicator_t sctk_communicator_get_new_id_from_intercomm ( int local_root, int rank,
                                                                                int local_leader, int remote_leader, const sctk_communicator_t origin_communicator, sctk_internal_communicator_t *tmp )
{
	sctk_internal_communicator_t *tmp_check;
	sctk_communicator_t comm = -1;
	sctk_communicator_t remote_comm = -1;
	sctk_communicator_t i = 0;
	sctk_communicator_t ti = 0;
	sctk_communicator_t peer_comm = 0;
	sctk_communicator_t remote_ti = 0;
	sctk_status_t status;
	int need_clean;
	int change_i = 1;
	int tag = 628;
	sctk_nodebug ( "rank %d : get new id from intercomm %d, local_root %d, lleader %d, rleader %d", rank, origin_communicator, local_root, local_leader, remote_leader );

	//~ get the peer comm used to create origin_communicator
	peer_comm = sctk_get_peer_comm ( origin_communicator );
	tmp_check = sctk_check_internal_communicator_no_lock ( peer_comm );

	int remote_leader_rank = sctk_get_comm_world_rank ( peer_comm, remote_leader );


	if ( tmp_check == NULL )
		peer_comm = SCTK_COMM_WORLD;

	do
	{
		need_clean = 0;

		if ( rank == local_leader )
		{
			//~ pass comm world && comm self
			if ( ( i == SCTK_COMM_WORLD ) || ( i == SCTK_COMM_SELF ) )
				i++;

			if ( ( i == SCTK_COMM_WORLD ) || ( i == SCTK_COMM_SELF ) )
				i++;

			//~ take the first NULL
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			i--;

			do
			{
				i++;
				tmp_check = sctk_check_internal_communicator_no_lock ( i );
			}
			while ( tmp_check != NULL );

			comm = i;
			sctk_nodebug ( "take comm %d", comm );
			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );

			//~ exchange comm between leaders
			sctk_sendrecv( &comm, sizeof(int), remote_leader_rank, tag, &remote_comm,  remote_leader_rank, SCTK_COMM_WORLD );			

			if ( comm != -1 )
			{
				if ( remote_comm > comm )
					comm = remote_comm;
			}
			else
				comm = remote_comm;

			sctk_nodebug ( "re-take comm %d", comm );
			tmp->id = comm;
			//~ re-check
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
				tmp_check = sctk_check_internal_communicator_no_lock(comm);

				if(tmp_check != NULL)
				{
					if(!sctk_communicator_compare(tmp_check, tmp))
					{
						sctk_nodebug("they are not equal !");
						comm = -1;
					}
				}	
				
			if ( comm != -1 )
			{
				tmp->id = comm;
				sctk_nodebug ( "Add comm from intercomm %d", comm );
				sctk_set_internal_communicator_no_lock ( comm, tmp );
			}

			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
			sctk_nodebug ( "rank %d : try comm from intercomm %d", rank, comm );
		}

		sctk_nodebug ( "broadcast from intercomm" );
		/* Broadcast comm to local group members*/
		sctk_broadcast (&comm,sizeof(sctk_communicator_t),local_leader,sctk_get_local_comm_id(origin_communicator));
		sctk_nodebug ( "Every one try %d", comm );

		if ( ( rank != local_leader ) && ( local_root == 1 ) )
		{
			if ( comm != -1 )
			{
				/*Check if available*/
				sctk_internal_communicator_t *tmp_check;
				tmp->id = comm;
				sctk_nodebug ( "check comm from intercomm %d", comm );
				sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
				tmp_check = sctk_check_internal_communicator_no_lock ( comm );

				if ( tmp_check != NULL )
				{
					if ( !sctk_communicator_compare ( tmp_check, tmp ) )
						comm = -1;
				}
				else
				{
					need_clean = 1;
					sctk_set_internal_communicator_no_lock ( comm, tmp );
				}

				sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
			}
		}

		sctk_nodebug ( "Every one try %d allreduce %d", comm, origin_communicator );
		/* Inter-Allreduce */
		ti = comm;
		//~ allreduce par groupe
		sctk_all_reduce ( &ti, &comm, sizeof ( sctk_communicator_t ), 1, ( sctk_Op_f ) sctk_comm_reduce, sctk_get_local_comm_id ( origin_communicator ), 0 );
		sctk_nodebug ( "after allreduce comm %d", comm );

		//~ echange des resultats du allreduce entre leaders
		if ( rank == local_leader )
		{
			sctk_sendrecv( &comm, sizeof(int), remote_leader_rank, tag, &remote_comm,  remote_leader_rank, SCTK_COMM_WORLD );
		}

		sctk_nodebug ( "after sendrecv comm %d", comm );
		//~ broadcast aux autres membres par groupe
		sctk_broadcast (&remote_comm,sizeof(sctk_communicator_t),local_leader,sctk_get_local_comm_id(origin_communicator));
		sctk_nodebug ( "after roadcast comm = %d && remote_comm = %d", comm, remote_comm );

		//~ on recupère le plus grand des resultats ou -1
		if ( comm != -1 && remote_comm != -1 )
		{
			if ( remote_comm > comm )
				comm = remote_comm;
		}
		else
			comm = -1;

		sctk_nodebug ( "finally com = %d", comm );

		if ( ( comm == -1 ) && ( need_clean == 1 ) && ( local_root == 1 ) )
		{
			sctk_nodebug ( "delete comm %d", ti );
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			sctk_del_internal_communicator_no_lock_no_check ( ti );
			sctk_spinlock_unlock ( &sctk_communicator_all_table_lock );
		}
	}
	while ( comm == -1 );

	assume ( comm != SCTK_COMM_WORLD );
	assume ( comm != SCTK_COMM_SELF );
	assume ( comm > 0 );
	tmp->id = comm;
	sctk_nodebug ( "finished comm = %d", comm );
	return comm;
}

/************************************************************************/
/*Collective accessors                                                  */
/************************************************************************/

/************************* FUNCTION ************************/
/**
 * This method get the collectives related to the communicator.
 * @param communicator Identification number of the local communicator.
 * @return the structure containing the collectives.
**/
struct sctk_internal_collectives_struct_s *sctk_get_internal_collectives ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	return tmp->collectives;
}

/************************* FUNCTION ************************/
/**
 * This method set the collectives related to the communicator.
 * @param id Identification number of the local communicator.
 * @param coll structure containing the collectives.
**/
void sctk_set_internal_collectives ( const sctk_communicator_t id, struct sctk_internal_collectives_struct_s *coll )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( id );
	tmp->collectives = coll;
}

/************************************************************************/
/*INIT/DELETE                                                           */
/************************************************************************/
static int int_cmp ( const void *a, const void *b )
{
	const int *ia = ( const int * ) a;
	const int *ib = ( const int * ) b;
	return *ia  - *ib;
}

int sctk_shared_mem_barrier_sig_init(struct shared_mem_barrier_sig *shmb,
                                     int nb_task) {
  shmb->sig_points = sctk_malloc(sizeof(sctk_atomics_ptr) * nb_task);

  assume(shmb->sig_points != NULL);

  shmb->tollgate = sctk_malloc(nb_task * sizeof(int));

  assume(shmb->tollgate != NULL);

  int i;
  for (i = 0; i < nb_task; ++i) {
    shmb->tollgate[i] = 0;
  }

  sctk_atomics_store_int(&shmb->fare, 0);

  sctk_atomics_store_int(&shmb->counter, nb_task);

  return 0;
}

int sctk_shared_mem_barrier_sig_release(struct shared_mem_barrier_sig *shmb) {
  sctk_free(shmb->sig_points);
  shmb->sig_points = NULL;

  sctk_free((void *)shmb->tollgate);
  shmb->tollgate = NULL;

  return 0;
}

int sctk_shared_mem_barrier_init(struct shared_mem_barrier *shmb, int nb_task) {
  sctk_atomics_store_int(&shmb->counter, nb_task);
  sctk_atomics_store_int(&shmb->phase, 0);
  return 0;
}

int sctk_shared_mem_barrier_release(struct shared_mem_barrier *shmb) {
  return 0;
}

int sctk_shared_mem_reduce_init(struct shared_mem_reduce *shmr, int nb_task) {

  shmr->buffer = sctk_malloc(sizeof(union shared_mem_buffer) * nb_task);

  assume(shmr->buffer != NULL);

  sctk_atomics_store_int(&shmr->owner, -1);

  sctk_atomics_store_int(&shmr->left_to_push, nb_task);

  shmr->target_buff = NULL;

  int pipelined_blocks = sctk_runtime_config_get()
                             ->modules.collectives_shm.reduce_pipelined_blocks;

  shmr->buff_lock = sctk_malloc(sizeof(sctk_spinlock_t) * pipelined_blocks);

  assume(shmr->buff_lock != NULL);

  shmr->pipelined_blocks = pipelined_blocks;

  int i;
  for (i = 0; i < pipelined_blocks; ++i) {
    shmr->buff_lock[i] = 0;
  }

  shmr->tollgate = sctk_malloc(nb_task * sizeof(int));

  assume(shmr->tollgate != NULL);

  for (i = 0; i < nb_task; ++i) {
    shmr->tollgate[i] = 0;
  }

  sctk_atomics_store_int(&shmr->fare, 0);

  return 0;
}

int sctk_shared_mem_reduce_release(struct shared_mem_reduce *shmr) {
  sctk_free(shmr->buffer);
  shmr->buffer = NULL;

  sctk_free((void *)shmr->buff_lock);
  shmr->buff_lock = NULL;

  sctk_free((void *)shmr->tollgate);
  shmr->tollgate = NULL;

  return 0;
}

int sctk_shared_mem_bcast_init(struct shared_mem_bcast *shmb, int nb_task) {
  sctk_atomics_store_int(&shmb->owner, -1);

  sctk_atomics_store_int(&shmb->left_to_pop, nb_task);

  shmb->tollgate = sctk_malloc(nb_task * sizeof(int));

  assume(shmb->tollgate != NULL);

  int i;
  for (i = 0; i < nb_task; ++i) {
    shmb->tollgate[i] = 0;
  }

  sctk_atomics_store_int(&shmb->fare, 0);
  sctk_atomics_store_ptr(&shmb->to_free, 0);

  shmb->scount = 0;
  shmb->stype_size = 0;
  shmb->root_in_buff = 0;

  return 0;
}

int sctk_shared_mem_bcast_release(struct shared_mem_bcast *shmb) {
  sctk_free((void *)shmb->tollgate);
  shmb->tollgate = NULL;

  return 0;
}

int sctk_shared_mem_gatherv_init(struct shared_mem_gatherv *shmgv,
                                 int nb_task) {
  sctk_atomics_store_int(&shmgv->owner, -1);
  sctk_atomics_store_int(&shmgv->left_to_push, nb_task);

  /* Tollgate */
  shmgv->tollgate = sctk_malloc(nb_task * sizeof(int));
  assume(shmgv->tollgate != NULL);
  sctk_atomics_store_int(&shmgv->fare, 0);

  /* Leaf CTX */
  shmgv->src_buffs = sctk_malloc(nb_task * sizeof(sctk_atomics_ptr));
  assume(shmgv->src_buffs != NULL);

  /* Root CTX */
  shmgv->target_buff = NULL;
  shmgv->counts = NULL;
  shmgv->disps = NULL;
  shmgv->rtype_size = 0;
  shmgv->rcount = 0;
  shmgv->let_me_unpack = 0;

  shmgv->send_type_size = sctk_malloc(nb_task * sizeof(size_t));
  assume(shmgv->send_type_size != NULL);

  shmgv->send_count = sctk_malloc(nb_task * sizeof(int));
  assume(shmgv->send_count != NULL);

  /* Fill it all */
  int i;
  for (i = 0; i < nb_task; ++i) {
    shmgv->tollgate[i] = 0;
    shmgv->send_count[i] = 0;
    shmgv->send_type_size[i] = 0;
    sctk_atomics_store_ptr(&shmgv->src_buffs[i], 0);
  }

  return 0;
}

int sctk_shared_mem_gatherv_release(struct shared_mem_gatherv *shmgv) {
  sctk_free((void *)shmgv->tollgate);
  shmgv->tollgate = NULL;

  sctk_free(shmgv->src_buffs);
  shmgv->src_buffs = NULL;

  sctk_free(shmgv->send_type_size);
  shmgv->send_type_size = NULL;

  sctk_free(shmgv->send_count);
  shmgv->send_count = NULL;

  return 0;
}

int sctk_shared_mem_scatterv_init(struct shared_mem_scatterv *shmsv,
                                  int nb_task) {
  sctk_atomics_store_int(&shmsv->owner, -1);
  sctk_atomics_store_int(&shmsv->left_to_pop, nb_task);

  /* Tollgate */
  shmsv->tollgate = sctk_malloc(nb_task * sizeof(int));
  assume(shmsv->tollgate != NULL);
  sctk_atomics_store_int(&shmsv->fare, 0);

  /* Root CTX */
  shmsv->src_buffs = sctk_malloc(nb_task * sizeof(sctk_atomics_ptr));
  assume(shmsv->src_buffs != NULL);

  shmsv->was_packed = 0;
  shmsv->stype_size = 0;
  shmsv->counts = NULL;
  shmsv->disps = NULL;

  /* Fill it all */
  int i;
  for (i = 0; i < nb_task; ++i) {
    shmsv->tollgate[i] = 0;
    sctk_atomics_store_ptr(&shmsv->src_buffs[i], 0);
  }

  return 0;
}

int sctk_shared_mem_scatterv_release(struct shared_mem_scatterv *shmgv) {
  sctk_free((void *)shmgv->tollgate);
  shmgv->tollgate = NULL;

  sctk_free(shmgv->src_buffs);
  shmgv->src_buffs = NULL;

  return 0;
}

int sctk_shared_mem_a2a_init(struct shared_mem_a2a *shmaa, int nb_task) {
  shmaa->infos =
      sctk_malloc(nb_task * sizeof(struct sctk_shared_mem_a2a_infos *));
  assume(shmaa->infos != NULL);

  /* Fill it all */
  int i;
  for (i = 0; i < nb_task; ++i) {
    shmaa->infos[i] = NULL;
  }

  shmaa->has_in_place = 0;

  return 0;
}

int sctk_shared_mem_a2a_release(struct shared_mem_a2a *shmaa) {
  sctk_free(shmaa->infos);
  shmaa->infos = NULL;

  return 0;
}

int powerof2(int x) {
  while (((x % 2) == 0) && x > 1)
    x /= 2;
  return (x == 1);
}

int sctk_comm_coll_init(struct sctk_comm_coll *coll, int nb_task) {
  /* NB task for all */
  coll->comm_size = nb_task;
  /* Allocate coll id array */
  coll->coll_id = sctk_malloc(nb_task * sizeof(unsigned int));
  assume(coll->coll_id != NULL);
  memset(coll->coll_id, 0, sizeof(int) * nb_task);

  int i;
  /* The barrier structure */
  sctk_shared_mem_barrier_init(&coll->shm_barrier, nb_task);
  /* The Signalled Barrier */
  sctk_shared_mem_barrier_sig_init(&coll->shm_barrier_sig, nb_task);
  /* The reduce structure */
  coll->reduce_interleave =
      sctk_runtime_config_get()->modules.collectives_shm.reduce_interleave;

  if (!powerof2(coll->reduce_interleave)) {
    sctk_error("INFO : Reduce interleave is required to be power of 2");
    sctk_error("INFO : now default to 8");
    coll->reduce_interleave = 8;
  }

  coll->shm_reduce =
      sctk_malloc(sizeof(struct shared_mem_reduce) * coll->reduce_interleave);
  assume(coll->shm_reduce != NULL);

  for (i = 0; i < coll->reduce_interleave; i++)
    sctk_shared_mem_reduce_init(&coll->shm_reduce[i], nb_task);

  /* The reduce structure */
  coll->bcast_interleave =
      sctk_runtime_config_get()->modules.collectives_shm.bcast_interleave;

  if (!powerof2(coll->bcast_interleave)) {
    sctk_error("INFO : Bcast interleave is required to be power of 2");
    sctk_error("INFO : now default to 8");
    coll->bcast_interleave = 8;
  }

  coll->shm_bcast =
      sctk_malloc(sizeof(struct shared_mem_bcast) * coll->bcast_interleave);
  assume(coll->shm_bcast != NULL);

  for (i = 0; i < coll->bcast_interleave; i++)
    sctk_shared_mem_bcast_init(&coll->shm_bcast[i], nb_task);
  /* The gatherv structure */
  sctk_shared_mem_gatherv_init(&coll->shm_gatherv, nb_task);
  /* The scatterv structure */
  sctk_shared_mem_scatterv_init(&coll->shm_scatterv, nb_task);
  /* The All2All structure */
  sctk_shared_mem_a2a_init(&coll->shm_a2a, nb_task);

  /* Flag init done */
  coll->init_done = 1;

  return 0;
}

int sctk_comm_coll_release(struct sctk_comm_coll *coll) {
  /* NB task for all */
  coll->comm_size = 0;
  /* Allocate coll id array */
  sctk_free(coll->coll_id);
  coll->coll_id = NULL;

  int i;
  /* The barrier structure */
  sctk_shared_mem_barrier_release(&coll->shm_barrier);
  /* The Signalled Barrier */
  sctk_shared_mem_barrier_sig_release(&coll->shm_barrier_sig);
  /* The reduce structure */

  for (i = 0; i < coll->reduce_interleave; i++)
    sctk_shared_mem_reduce_release(&coll->shm_reduce[i]);

  sctk_free(coll->shm_reduce);
  coll->shm_reduce = NULL;
  coll->reduce_interleave = 0;

  /* The reduce structure */
  for (i = 0; i < coll->bcast_interleave; i++)
    sctk_shared_mem_bcast_release(&coll->shm_bcast[i]);

  sctk_free(coll->shm_bcast);
  coll->shm_bcast = NULL;
  coll->bcast_interleave = 0;

  /* The gatherv structure */
  sctk_shared_mem_gatherv_release(&coll->shm_gatherv);
  /* The scatterv structure */
  sctk_shared_mem_scatterv_release(&coll->shm_scatterv);
  /* The All2All structure */
  sctk_shared_mem_a2a_release(&coll->shm_a2a);

  coll->init_done = 0;

  return 0;
}

/************************* FUNCTION ************************/
/**
 * This method fill the new structure for communicator with the given parameters.
 * @param nb_task			Nombre de tâches.
 * @param last_local		Minimum rank of local task.
 * @param first_local		Maximum rank of local task.
 * @param local_tasks		Number of tasks involved with the communicator for the current process.
 * @param local_to_global	Gives MPI_COMM_WORLD ranks used in this communicator.
 * @param global_to_local	Gives task ranks corresponding to MPI_COMM_WORLD ranks.
 * @param task_to_process	Gives identification of process which schedules the task.
 * @param process_array		Process involved with the communicator.
 * @param process_nb		Number of process for the communicator.
 * @param tmp				New communicator.
**/
static inline void sctk_communicator_init_intern_init_only ( const int nb_task, int last_local, int first_local, int local_tasks,
                                                             int *local_to_global, int *global_to_local, int *task_to_process, int *process_array, int process_nb, sctk_internal_communicator_t *tmp )
{
	sctk_spin_rwlock_t lock = SCTK_SPIN_RWLOCK_INITIALIZER;
	sctk_spinlock_t spinlock = SCTK_SPINLOCK_INITIALIZER;
	tmp->collectives = NULL;
	tmp->nb_task = nb_task;
	tmp->last_local = last_local;
	tmp->first_local = first_local;
	tmp->local_tasks = local_tasks;
	tmp->process = process_array;
	tmp->process_nb = process_nb;

	if ( process_array != NULL )
	{
		qsort ( process_array, process_nb, sizeof ( int ), int_cmp );
	}

	tmp->local_to_global = local_to_global;
	tmp->global_to_local = global_to_local;
	tmp->task_to_process = task_to_process;
	tmp->is_inter_comm = 0;
	tmp->lock = lock;
	tmp->creation_lock = spinlock;
	tmp->has_zero = 0;
	tmp->is_comm_self = 0;
        tmp->is_shared_mem = 0;
        OPA_store_int(&(tmp->nb_to_delete), 0);

        tmp->is_shared_mem = 0;

        /* Set the shared-memory Flag */
        if (local_to_global) {
          tmp->is_shared_mem = 1;
          int i;
          for (i = 0; i < nb_task; i++) {
            tmp->is_shared_mem = 1;
            int trank = local_to_global[i];
            if (sctk_is_net_message(trank)) {
              tmp->is_shared_mem = 0;
              break;
            }
          }
        } else {
          /* We are building comm_world */
          if (sctk_process_number == 1)
            tmp->is_shared_mem = 1;
        }

        if (tmp->is_shared_mem) {
          sctk_comm_coll_init(&tmp->coll, nb_task);
          /* After that
          tmp->coll.init_done == 1
          */
        } else {
          /* Flag as not intialized */
          tmp->coll.init_done = 0;
        }
}

/************************* FUNCTION ************************/
/**
 * This method fill the new structure for communicator with the given parameters by calling.
 * sctk_communicator_init_intern_init_only() method. then it initializes the collective structure.
 * @param nb_task			Nombre de tâches.
 * @param comm 				origin communicator.
 * @param last_local		Minimum rank of local task.
 * @param first_local		Maximum rank of local task.
 * @param local_tasks		Number of tasks involved with the communicator for the current process.
 * @param local_to_global	Gives MPI_COMM_WORLD ranks used in this communicator.
 * @param global_to_local	Gives task ranks corresponding to MPI_COMM_WORLD ranks.
 * @param task_to_process	Gives identification of process which schedules the task  .
 * @param process_array		Process involved with the communicator.
 * @param process_nb		Number of process for the communicator.
 * @param tmp				New communicator.
**/
static inline void sctk_communicator_init_intern_no_alloc ( const int nb_task, const sctk_communicator_t comm, int last_local, int first_local, int local_tasks,
                                                            int *local_to_global, int *global_to_local, int *task_to_process, int *process_array, int process_nb, sctk_internal_communicator_t *tmp )
{
	if ( comm == 0 )
		tmp->id = 0;
	else
		if ( comm == 1 )
			tmp->id = 1;

	sctk_communicator_init_intern_init_only ( nb_task, last_local, first_local, local_tasks, local_to_global, global_to_local, task_to_process, process_array, process_nb, tmp );

	if ( comm == SCTK_COMM_SELF )
		tmp->is_comm_self = 1;

	assume ( sctk_set_internal_communicator ( comm, tmp ) == 0 );
	sctk_collectives_init_hook ( comm );
}

/************************* FUNCTION ************************/
/**
 * This method allocate the memory for the new communicator, then fill the new structure with the given parameters and initializes the collective.
 * structure by calling sctk_communicator_init_intern_no_alloc() method.
 * @param nb_task			Nombre de tâches.
 * @param comm 				origin communicator.
 * @param last_local		Minimum rank of local task.
 * @param first_local		Maximum rank of local task.
 * @param local_tasks		Number of tasks involved with the communicator for the current process.
 * @param local_to_global	Gives MPI_COMM_WORLD ranks used in this communicator.
 * @param global_to_local	Gives task ranks corresponding to MPI_COMM_WORLD ranks.
 * @param task_to_process	Gives identification of process which schedules the task.
 * @param process_array		Process involved with the communicator.
 * @param process_nb		Number of process for the communicator.
**/
static inline void sctk_communicator_init_intern ( const int nb_task, const sctk_communicator_t comm, int last_local, int first_local,
                                                   int local_tasks, int *local_to_global, int *global_to_local, int *task_to_process, int *process_array, int process_nb )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
	memset ( tmp, 0, sizeof ( sctk_internal_communicator_t ) );

	sctk_communicator_init_intern_no_alloc ( nb_task, comm, last_local, first_local, local_tasks, local_to_global, global_to_local, task_to_process, process_array, process_nb, tmp );
}

/************************* FUNCTION ************************/
/**
 * This method initializes the MPI_COMM_SELF communicator.
**/
void sctk_communicator_self_init()
{
	const int last_local = 0;
	const int first_local = 0;
	const int local_tasks = 1;
	int *process_array;

	/* Construct the list of processes */
	process_array = sctk_malloc ( sizeof ( int ) );
	*process_array = sctk_process_rank;
	sctk_communicator_init_intern ( 1, SCTK_COMM_SELF, last_local, first_local, local_tasks, NULL, NULL, NULL, process_array, 1 );
}


/************************* sctk_get_process env variables ************************/

/* This section makes sure we are loading the variables needed by
 * several function in this file only once */

/* Bolean telling if the variables were loaded */
static int sctk_get_process_rank_from_task_rank_context_init_done = 0;
static int sctk_get_process_rank_from_task_rank_context_present = 0;



/* getenv("SCTK_MIC_NB_TASK") != NULL) */
static char *SCTK_MIC_NB_TASK = NULL;
/* getenv("SCTK_HOST_NB_TASK") */
static char *SCTK_HOST_NB_TASK = NULL;
/* getenv("SCTK_NB_HOST") */
static char *SCTK_NB_HOST = NULL;
/* getenv("SCTK_NB_MIC") */
static char *SCTK_NB_MIC = NULL;

/** @brief This function loads the parameters needed by \ref sctk_get_process_rank_from_task_rank,
        \ref sctk_get_node_rank_from_task_rank and \ref sctk_communicator_world_init once for all
 */
static inline void sctk_get_process_setup_env()
{
	if ( sctk_get_process_rank_from_task_rank_context_init_done )
		return;

	SCTK_MIC_NB_TASK = getenv ( "SCTK_MIC_NB_TASK" );
	SCTK_HOST_NB_TASK = getenv ( "SCTK_HOST_NB_TASK" );
	SCTK_NB_HOST = getenv ( "SCTK_NB_HOST" );
	SCTK_NB_MIC = getenv ( "SCTK_NB_MIC" );

	if ( ( SCTK_MIC_NB_TASK != NULL ) ||
          ( SCTK_HOST_NB_TASK != NULL ) ||
          ( SCTK_NB_HOST != NULL ) ||
          ( SCTK_NB_MIC != NULL ) )
		sctk_get_process_rank_from_task_rank_context_present = 1;


	sctk_get_process_rank_from_task_rank_context_init_done = 1;
	
	
}



/************************* FUNCTION ************************/
/**
 * This method initializes the MPI_COMM_WORLD communicator.
 * @param nb_task Number of tasks present in MPI_COMM_WORLD.
**/
void sctk_communicator_world_init ( const int nb_task )
{
	int i;
	int pos;
	int last_local;
	int first_local;
	int local_tasks;
	int *process_array;

	pos = sctk_process_rank;

	sctk_get_process_setup_env();

	if( sctk_get_process_rank_from_task_rank_context_present )
	{
		int node_rank, process_on_node_rank, process_on_node_number;
		int host_number, mic_nb_task, host_nb_task;
		host_number = ( SCTK_NB_HOST != NULL ) ? atoi ( SCTK_NB_HOST ) : 0;
		mic_nb_task = ( SCTK_MIC_NB_TASK != NULL ) ? atoi ( SCTK_MIC_NB_TASK ) : 0;
		host_nb_task = ( SCTK_HOST_NB_TASK != NULL ) ? atoi ( SCTK_HOST_NB_TASK ) : 0;
		sctk_pmi_get_node_rank ( &node_rank );
		sctk_pmi_get_process_on_node_rank ( &process_on_node_rank );
		sctk_pmi_get_process_on_node_number ( &process_on_node_number );

		sctk_nodebug ( "host_number = %d, mic_nb_task = %d, host_nb_task = %d", host_number, mic_nb_task, host_nb_task );

#if __MIC2__
		local_tasks = mic_nb_task / process_on_node_number;

		if ( ( mic_nb_task % process_on_node_number ) > process_on_node_rank )
		{
			local_tasks++;
			first_local = ( host_nb_task * host_number ) + ( ( node_rank - host_number ) * mic_nb_task ) + ( local_tasks * process_on_node_rank );
		}
		else
		{
			int i = 0;
			first_local = ( host_nb_task * host_number ) + ( ( node_rank - host_number ) * mic_nb_task );

			for ( i = 0 ; i < process_on_node_rank ; i++ )
			{
				if ( ( mic_nb_task % process_on_node_number ) > i )
					first_local += ( local_tasks + 1 );
				else
					first_local += local_tasks;
			}
		}

#else
		local_tasks = host_nb_task / process_on_node_number;

		if ( ( host_nb_task % process_on_node_number ) > process_on_node_rank )
		{
			local_tasks++;
			first_local = ( node_rank * host_nb_task ) + ( local_tasks * process_on_node_rank );
		}
		else
		{
			int i = 0;
			first_local = ( node_rank * host_nb_task );

			for ( i = 0 ; i < process_on_node_rank ; i++ )
			{
				if ( ( host_nb_task % process_on_node_number ) > i )
					first_local += ( local_tasks + 1 );
				else
					first_local += local_tasks;
			}
		}

#endif
		last_local = first_local + local_tasks - 1;
		sctk_nodebug ( "lt %d, Process %d %d-%d", local_tasks, sctk_process_rank, first_local, first_local + local_tasks - 1 );
	}
	else
	{
		local_tasks = nb_task / sctk_process_number;

		if ( nb_task % sctk_process_number > pos )
			local_tasks++;

		first_local = sctk_process_rank * local_tasks;

		if ( nb_task % sctk_process_number <= pos )
		{
			first_local += nb_task % sctk_process_number;
		}

		last_local = first_local + local_tasks - 1;
	}

	/* Construct the list of processes */
	process_array = sctk_malloc ( sctk_process_number * sizeof ( int ) );

	for ( i = 0; i < sctk_process_number; ++i )
		process_array[i] = i;

	sctk_communicator_init_intern ( nb_task, SCTK_COMM_WORLD, last_local, first_local, local_tasks, NULL, NULL, NULL, process_array, sctk_process_number );

	if ( sctk_process_number > 1 )
	{
		sctk_pmi_barrier();
	}
}

void sctk_communicator_delete()
{
	if ( sctk_process_number > 1 )
	{
		sctk_pmi_barrier();
	}
}

/************************* FUNCTION ************************/
/**
 * This method delete a communicator.
 * @param comm communicator to delete.
**/
sctk_communicator_t sctk_delete_communicator ( const sctk_communicator_t comm )
{
	if ( ( comm == SCTK_COMM_WORLD ) || ( comm == SCTK_COMM_SELF ) )
	{
		not_reachable();
		return comm;
	}
	else
	{
		sctk_internal_communicator_t *tmp;
		int is_master = 0;
		int val;
		int max_val;
		sctk_barrier (comm);
		tmp = sctk_get_internal_communicator ( comm );
		sctk_barrier (comm);

		max_val = tmp->local_tasks;
		val = OPA_fetch_and_incr_int ( & ( tmp->nb_to_delete ) );

		if ( val == max_val - 1 )
		{
			is_master = 1;
			sctk_spinlock_lock ( &sctk_communicator_all_table_lock );
			sctk_free ( tmp->local_to_global );
			sctk_free ( tmp->global_to_local );
			sctk_free ( tmp->task_to_process );
			sctk_del_internal_communicator_no_lock_no_check ( comm );

                        if (tmp->coll.init_done) {
                          sctk_comm_coll_release(&tmp->coll);
                        }

                        sctk_free(tmp);
                        sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
                } else {
                  is_master = 0;
                }

                return SCTK_COMM_NULL;
        }
}

/************************************************************************/
/*Accessors                                                             */
/************************************************************************/

/************************* FUNCTION ************************/
/**
 * This method get the peer communication id
 * @param communicator given intercommunicator.
 * @return the id.
**/
inline sctk_communicator_t sctk_get_peer_comm ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	sctk_assert ( tmp->is_inter_comm == 1 );
	return tmp->peer_comm;
}

/************************* FUNCTION ************************/
/**
 * This method get the number of tasks involved with the communicator for the
 * current process.
 * @param communicator given communicator.
 * @return the number.
**/
inline int sctk_get_nb_task_local ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	return tmp->local_tasks;
}

/************************* FUNCTION ************************/
/**
 * This method get the minimum rank of a local task present in a given communicator.
 * @param communicator given communicator.
 * @return the number.
**/
inline int sctk_get_last_task_local ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	return tmp->last_local;
}

/************************* FUNCTION ************************/
/**
 * This method get the maximum rank of a local task present in a given communicator.
 * @param communicator given communicator.
 * @return the number.
**/
int sctk_get_first_task_local ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	return tmp->first_local;
}

/************************* FUNCTION ************************/
/**
 * This method get the number of processes involved with a given communicator.
 * @param communicator given communicator.
 * @return the number.
**/
int sctk_get_process_nb_in_array ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	return tmp->process_nb;
}

/************************* FUNCTION ************************/
/**
 * This method get the processes involved with a given communicator.
 * @param communicator given communicator.
 * @return the array of processes.
**/
int *sctk_get_process_array ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	return tmp->process;
}

/************************* FUNCTION ************************/
/**
 * This method get the number of tasks present in a given communicator.
 * @param communicator given communicator.
 * @return the number.
**/
int sctk_get_nb_task_total ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );

	//check if intercommunicator
	if ( tmp->is_inter_comm == 1 )
	{
		int result = sctk_is_in_local_group ( communicator );

		if ( result == 0 )
		{
			return tmp->remote_comm->nb_task;

		}
		else
		{
			return tmp->nb_task;
		}
	}
	else
	{
		return tmp->nb_task;
	}
}

/************************* FUNCTION ************************/
/**
 * This method get the number of tasks present in the remote_group.
 * @param communicator given intercommunicator.
 * @return the number.
**/
int sctk_get_nb_task_remote ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );

	//check if intercommunicator
	if ( tmp->is_inter_comm == 1 )
	{
		int result = sctk_is_in_local_group ( communicator );

		if ( result == 0 )
		{
			return tmp->nb_task;
		}
		else
		{
			return tmp->remote_comm->nb_task;
		}
	}
	else
		return tmp->nb_task;
}

/************************* FUNCTION ************************/
/**
 * This method check if the communicator is an intercommunicator.
 * @param communicator given communicator.
 * @return 1 if it is, 0 if it is not.
**/
int __sctk_is_inter_comm(const sctk_communicator_t communicator) {

  sctk_internal_communicator_t *tmp;
  tmp = sctk_get_internal_communicator(communicator);

  return tmp->is_inter_comm;
}

/************************* FUNCTION ************************/
/**
 * This method check if the communicator is limited to a shared-memory space.
 * @param communicator given communicator.
 * @return 1 if it is, 0 if it is not.
**/
int __sctk_is_shared_mem(const sctk_communicator_t communicator) {

  sctk_internal_communicator_t *tmp;
  tmp = sctk_get_internal_communicator(communicator);

  // sctk_error("%d == %d", communicator, tmp->is_shared_mem );
  return tmp->is_shared_mem;
}

/************************* FUNCTION ************************/
/**
 * This method get the rank of the root task in the local group.
 * @param communicator given communicator.
 * @return the rank of the local_leader.
**/
int sctk_get_local_leader ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	//check if intercommunicator
	assume ( tmp->is_inter_comm == 1 );
	int result = sctk_is_in_local_group ( communicator );

	if ( result )
	{
		return tmp->local_leader;
	}
	else
	{
		return tmp->remote_comm->local_leader;
	}
}

/************************* FUNCTION ************************/
/**
 * This method get the rank of the root task in the remote group.
 * @param communicator given communicator.
 * @return the rank of the remote_leader.
**/
int sctk_get_remote_leader ( const sctk_communicator_t communicator )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( communicator );
	//check if intercommunicator
	assume ( tmp->is_inter_comm == 1 );
	int result = sctk_is_in_local_group ( communicator );

	if ( result )
	{
		return tmp->remote_leader;
	}
	else
	{
		return tmp->remote_comm->remote_leader;
	}
}

/************************* FUNCTION ************************/
/**
 * This method get the rank of a task in a given communicator.
 * @param communicator given communicator.
 * @param comm_world_rank comm world rank of the task.
 * @return the rank.
**/
int sctk_get_rank(const sctk_communicator_t communicator,
                  const int comm_world_rank) {
  sctk_nodebug("give rank for comm %d with comm_world_rank %d", communicator,
               comm_world_rank);
  sctk_internal_communicator_t *tmp;
  int ret;

  if (communicator == SCTK_COMM_WORLD)
    return comm_world_rank;
  else if (communicator == SCTK_COMM_SELF)
    return 0;

  tmp = sctk_get_internal_communicator(communicator);

  if (tmp->is_inter_comm == 1) {
    int result = sctk_is_in_local_group_rank(communicator, comm_world_rank);

    if (result) {
      if (tmp->global_to_local != NULL) {
        sctk_communicator_intern_read_lock(tmp);
        ret = tmp->global_to_local[comm_world_rank];
        sctk_communicator_intern_read_unlock(tmp);
        return ret;
      }
    } else {
      if (tmp->global_to_local != NULL) {
        sctk_communicator_intern_read_lock(tmp);
        ret = tmp->remote_comm->global_to_local[comm_world_rank];
        sctk_communicator_intern_read_unlock(tmp);
        return ret;
      }
    }
  } else {
    if (tmp->global_to_local != NULL) {
      sctk_communicator_intern_read_lock(tmp);
      ret = tmp->global_to_local[comm_world_rank];
      sctk_communicator_intern_read_unlock(tmp);
      return ret;
    } else {
      /* Handle DUP of COMM_SELF case */
      if (tmp->is_comm_self)
        return 0;

      return comm_world_rank;
    }
  }

  sctk_error("HERE IS -1");
  return -1;
}

/************************* FUNCTION ************************/
/**
 * This method get the comm world rank of a task in the remote group of a given communicator.
 * @param communicator given communicator.
 * @param rank rank of the task in the remote group.
 * @return the rank.
**/
int sctk_get_remote_comm_world_rank ( const sctk_communicator_t communicator, const int rank )
{
	sctk_internal_communicator_t *tmp;
	int ret;
	int i;

	/* Other communicators */
	tmp = sctk_get_internal_communicator ( communicator );
	assume ( tmp->is_inter_comm == 1 );

	int result = sctk_is_in_local_group ( communicator );
	assume ( result != -1 );

	if ( result )
	{
		sctk_nodebug ( "rank %d : is in local_group comm %d", rank, communicator );

		if ( tmp->remote_comm->local_to_global != NULL )
		{
			sctk_communicator_intern_read_lock ( tmp );
			ret = tmp->remote_comm->local_to_global[rank];
			sctk_communicator_intern_read_unlock ( tmp );
			return ret;
		}
		else
			return rank;
	}
	else
	{
		sctk_nodebug ( "rank %d : is in remote_group comm %d", rank, communicator );

		if ( tmp->local_to_global != NULL )
		{
			sctk_communicator_intern_read_lock ( tmp );
			ret = tmp->local_to_global[rank];
			sctk_communicator_intern_read_unlock ( tmp );
			return ret;
		}
		else
		{
			/* Handle DUP of COMM_SELF case */
			if ( tmp->is_comm_self )
				return sctk_get_task_rank();

			return rank;
		}
	}

}

/************************* FUNCTION ************************/
/**
 * This method get the comm world rank of a task in the local group of a given communicator.
 * @param communicator given communicator.
 * @param rank rank of the task in the local group.
 * @return the rank.
**/
int sctk_get_comm_world_rank ( const sctk_communicator_t communicator, const int rank )
{
	sctk_internal_communicator_t *tmp;
	int ret;
	int i;

	if ( communicator == SCTK_COMM_WORLD )
		return rank;
	else
		if ( communicator == SCTK_COMM_SELF )
		{
			int cwrank;
			/* get task id */
			cwrank = sctk_get_task_rank();
			return cwrank;
		}

	/* Other communicators */
	tmp = sctk_get_internal_communicator ( communicator );

	if ( tmp->is_inter_comm == 1 )
	{
		int result = sctk_is_in_local_group ( communicator );

		if ( result )
		{
			if ( tmp->local_to_global != NULL )
			{
				sctk_communicator_intern_read_lock ( tmp );
				ret = tmp->local_to_global[rank];
				sctk_communicator_intern_read_unlock ( tmp );
				return ret;
			}
			else
				return rank;
		}
		else
		{
			if ( tmp->remote_comm->local_to_global != NULL )
			{
				sctk_communicator_intern_read_lock ( tmp );
				ret = tmp->remote_comm->local_to_global[rank];
				sctk_communicator_intern_read_unlock ( tmp );
				return ret;
			}
			else
				return rank;
		}
	}
	else
	{
		if ( tmp->local_to_global != NULL )
		{
			sctk_communicator_intern_read_lock ( tmp );
                        assume(rank < tmp->nb_task);
                        ret = tmp->local_to_global[rank];
                        sctk_communicator_intern_read_unlock(tmp);
                        return ret;
                } else {
                  /* Handle DUP of COMM_SELF case */
                  if (tmp->is_comm_self)
                    return sctk_get_task_rank();

                  return rank;
                }
        }
}

/************************* FUNCTION ************************/
/**
 * This method get the number of tasks present in a given communicator and
 * the rank of a task in this one.
 * @param communicator given communicator.
 * @param rank of the task in the communicator.
 * @param size number of tasks in the communicator.
 * @param glob_rank comm world rank of the task.
**/
void sctk_get_rank_size_total ( const sctk_communicator_t communicator, int *rank, int *size, int glob_rank )
{
	*size = sctk_get_nb_task_total ( communicator );
	*rank = sctk_get_rank ( communicator, glob_rank );
}


int sctk_get_node_rank_from_task_rank ( const int rank )
{
	sctk_get_process_setup_env();

	if ( sctk_get_process_rank_from_task_rank_context_present)
	{
		int host_number, mic_nb_task, host_nb_task;

		host_number = ( SCTK_NB_HOST != NULL ) ? atoi ( SCTK_NB_HOST ) : 0;
		mic_nb_task = ( SCTK_MIC_NB_TASK != NULL ) ? atoi ( SCTK_MIC_NB_TASK ) : 0;
		host_nb_task = ( SCTK_HOST_NB_TASK != NULL ) ? atoi ( SCTK_HOST_NB_TASK ) : 0;

		if ( rank > ( ( host_number * host_nb_task ) - 1 ) )
		{
			return ( ( host_number ) + ( ( rank - ( host_number * host_nb_task ) ) / mic_nb_task ) );
		}
		else
		{
			return ( rank / host_nb_task );
		}
	}

	return -1;
}


/************************* FUNCTION ************************/
/**
 * This method get the rank of the process which handle the given task.
 * @param rank rank of the task in comm world.
 * @return the rank.
 **/
int sctk_get_process_rank_from_task_rank ( int rank )
{
    sctk_internal_communicator_t *tmp;
    int proc_rank;

#ifdef SCTK_PROCESS_MODE
    return rank;
#endif

    tmp = sctk_get_internal_communicator(SCTK_COMM_WORLD);

    if (tmp->task_to_process != NULL) {
        sctk_communicator_intern_read_lock(tmp);
        proc_rank = tmp->task_to_process[rank];
        sctk_communicator_intern_read_unlock(tmp);
    } else {

        static __thread int last_query = -1;
        static __thread int last_prank = -1;

        if( last_query == rank )
            return last_prank;

        int local_tasks;
        int remain_tasks;
        local_tasks = tmp->nb_task / sctk_process_number;
        remain_tasks = tmp->nb_task % sctk_process_number;

        if (rank < (local_tasks + 1) * remain_tasks)
            proc_rank = rank / (local_tasks + 1);
        else
            proc_rank =
                remain_tasks +
                ((rank - (local_tasks + 1) * remain_tasks) / local_tasks);

        last_query = rank;
        last_prank = proc_rank;
    }

    return proc_rank;
}

/************************************************************************/
/*Communicator creation                                                 */
/************************************************************************/

/************************* FUNCTION ************************/
/**
 * This method duplicates a communicator.
 * @param origin_communicator communicator to duplicate.
 * @param is_inter_comm determine if it is an intercommunicator or not.
 * @param rank rank of the task which calls the method.
 * @return the identification number of the duplicated communicator.
**/
sctk_communicator_t sctk_duplicate_communicator ( const sctk_communicator_t origin_communicator, int is_inter_comm, int rank )
{
	sctk_internal_communicator_t *tmp;
	tmp = sctk_get_internal_communicator ( origin_communicator );

	sctk_nodebug ( "rank %d : duplicate comm %d", rank, origin_communicator );

	if ( is_inter_comm == 0 )
	{
		sctk_internal_communicator_t *new_tmp;
		int local_root = 0;

		sctk_barrier (origin_communicator);

		tmp = sctk_get_internal_communicator ( origin_communicator );

		sctk_spinlock_lock ( & ( tmp->creation_lock ) );

		if ( tmp->new_comm == NULL )
		{
			local_root = 1;
			tmp->has_zero = 0;

			new_tmp = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
			memset ( new_tmp, 0, sizeof ( sctk_internal_communicator_t ) );

			tmp->new_comm = new_tmp;
			sctk_communicator_init_intern_init_only ( tmp->nb_task,
			                                          tmp->last_local,
			                                          tmp->first_local,
			                                          tmp->local_tasks,
			                                          tmp->local_to_global,
			                                          tmp->global_to_local,
			                                          tmp->task_to_process,
			                                          /* FIXME: process and process_nb have not been
			                                          * tested for now. Could aim to trouble */
			                                          tmp->process,
			                                          tmp->process_nb,
			                                          tmp->new_comm );
			tmp->is_inter_comm = 0;
			tmp->new_comm->local_leader = -1;
			tmp->new_comm->remote_leader = -1;
			tmp->new_comm->local_id = -1;
			tmp->new_comm->remote_id = -1;
			tmp->new_comm->remote_comm = NULL;
		}

		new_tmp = tmp->new_comm;

		if ( tmp->is_comm_self )
		{
			tmp->new_comm = NULL;
		}


		sctk_spinlock_unlock ( & ( tmp->creation_lock ) );
		sctk_barrier (origin_communicator);

		if ( rank == 0 )
		{
			local_root = 1;
			tmp->has_zero = 1;
		}
		sctk_barrier (origin_communicator);

		if ( ( rank != 0 ) && ( tmp->has_zero == 1 ) )
		{
			local_root = 0;
		}

		sctk_nodebug ( "new id DUP INTRA for rank %d, local_root %d, has_zero %d", rank, local_root, tmp->has_zero );
		new_tmp->id = sctk_communicator_get_new_id ( local_root, rank, origin_communicator, new_tmp );
		sctk_network_notify_new_communicator(new_tmp->id, new_tmp->nb_task);

		sctk_get_internal_communicator ( new_tmp->id );
		assume ( new_tmp->id >= 0 );
		sctk_collectives_init_hook ( new_tmp->id );

		if ( origin_communicator != SCTK_COMM_SELF )
		{
			sctk_barrier (origin_communicator);
			tmp->new_comm = NULL;
			sctk_barrier (origin_communicator);
		}

		assume ( new_tmp->id != origin_communicator );

		if ( ( origin_communicator == SCTK_COMM_SELF ) || ( tmp->is_comm_self ) )
		{
			new_tmp->is_comm_self = 1;
		}
		else
		{
			new_tmp->is_comm_self = 0;
		}

		return new_tmp->id;

	}
	else
	{
		assume ( tmp->remote_comm != NULL );
		sctk_internal_communicator_t *new_tmp;
		sctk_internal_communicator_t *remote_tmp;
		int local_root = 0, local_leader = 0, remote_leader = 0;

		if ( sctk_is_in_local_group ( origin_communicator ) )
		{
			local_leader = tmp->local_leader;
			remote_leader = tmp->remote_leader;
		}
		else
		{
			local_leader = tmp->remote_comm->local_leader;
			remote_leader = tmp->remote_comm->remote_leader;
		}

		sctk_barrier ( origin_communicator );
		sctk_spinlock_lock ( & ( tmp->creation_lock ) );

		if ( tmp->new_comm == NULL )
		{
			//~ local structure
			new_tmp = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
			memset ( new_tmp, 0, sizeof ( sctk_internal_communicator_t ) );
			local_root = 1;
			tmp->has_zero = 0;
			tmp->new_comm = new_tmp;

			if ( tmp->new_comm == NULL )
				assume ( 0 );

			//~ initialize the local structure
			sctk_communicator_init_intern_init_only (
			    tmp->nb_task,
			    tmp->last_local,
			    tmp->first_local,
			    tmp->local_tasks,
			    tmp->local_to_global,
			    tmp->global_to_local,
			    tmp->task_to_process,
			    tmp->process,
			    tmp->process_nb,
			    tmp->new_comm );

			if ( tmp->new_comm == NULL )
				assume ( 0 );

			//~ intercomm attributes
			tmp->new_comm->is_inter_comm = 1;
				tmp->new_comm->local_id = tmp->local_id;
				tmp->new_comm->remote_id = tmp->remote_id;
			tmp->new_comm->local_leader = tmp->local_leader;
			tmp->new_comm->remote_leader = tmp->remote_leader;
			tmp->new_comm->peer_comm = tmp->peer_comm;

			if ( tmp->new_comm == NULL )
				assume ( 0 );

			//~ remote structure
			tmp->new_comm->remote_comm = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
			memset ( tmp->new_comm->remote_comm, 0, sizeof ( sctk_internal_communicator_t ) );
			//~ initialize the remote structure
			sctk_communicator_init_intern_init_only (
			    tmp->remote_comm->nb_task,
			    tmp->remote_comm->last_local,
			    tmp->remote_comm->first_local,
			    tmp->remote_comm->local_tasks,
			    tmp->remote_comm->local_to_global,
			    tmp->remote_comm->global_to_local,
			    tmp->remote_comm->task_to_process,
			    tmp->remote_comm->process,
			    tmp->remote_comm->process_nb,
			    tmp->new_comm->remote_comm );
			//~ intercomm attributes
			tmp->new_comm->remote_comm->is_inter_comm = 1;
					tmp->new_comm->remote_comm->local_id = tmp->remote_comm->local_id;
					tmp->new_comm->remote_comm->remote_id = tmp->remote_comm->remote_id;
			tmp->new_comm->remote_comm->local_leader = tmp->remote_comm->local_leader;
			tmp->new_comm->remote_comm->remote_leader = tmp->remote_comm->remote_leader;
			tmp->new_comm->remote_comm->peer_comm = tmp->remote_comm->peer_comm;
			assume ( tmp->new_comm->remote_comm != NULL );




			assume ( tmp->new_comm != NULL );
		}

		sctk_spinlock_unlock ( & ( tmp->creation_lock ) );

		new_tmp = tmp->new_comm;

		if ( ( origin_communicator == SCTK_COMM_SELF ) || ( tmp->is_comm_self ) )
		{
			new_tmp->is_comm_self = 1;
		}
		else
		{
			new_tmp->is_comm_self = 0;
		}

		assume ( new_tmp != NULL );
		assume ( new_tmp->remote_comm != NULL );

		if ( rank == local_leader )
		{
			local_root = 1;
			tmp->has_zero = 1;
		}

		sctk_barrier ( origin_communicator );

		if ( ( rank != local_leader ) && ( tmp->has_zero == 1 ) )
		{
			local_root = 0;
		}

		sctk_nodebug ( "new id DUP INTER for rank %d, local_root %d, has_zero %d", rank, local_root, tmp->has_zero );
		new_tmp->id = sctk_communicator_get_new_id_from_intercomm ( local_root, rank, local_leader, remote_leader, origin_communicator, new_tmp );
		sctk_network_notify_new_communicator(new_tmp->id, new_tmp->nb_task);
		sctk_nodebug ( "comm %d duplicated =============> (%d - %d)", origin_communicator, new_tmp->id, new_tmp->id );
		sctk_get_internal_communicator ( new_tmp->id );
		assume ( new_tmp->id >= 0 );
		sctk_collectives_init_hook ( new_tmp->id );

		if ( tmp->new_comm != NULL )
			tmp->new_comm = NULL;

		assume ( new_tmp->id != origin_communicator );
		return new_tmp->id;
	}

	return SCTK_COMM_NULL;
}

/************************* FUNCTION ************************/
/**
 * This method creates a new intercommunicator.
 * @param local_comm communicator linked to the local group.
 * @param local_leader rank of the root task in the local group.
 * @param peer_comm communicator used to communicate between a designated process in the other communicator.
 * @param remote_leader rank of the root task in the remote group.
 * @param tag tag for the communications between leaders during the creation.
 * @return the identification number of the new intercommunicator.
**/
sctk_communicator_t sctk_create_intercommunicator ( const sctk_communicator_t local_comm, const int local_leader,
                                                    const sctk_communicator_t peer_comm, const int remote_leader, const int tag, const int first )
{
	sctk_status_t status;
	sctk_communicator_t comm;
	sctk_internal_communicator_t *tmp;
	sctk_internal_communicator_t *local_tmp;
	sctk_internal_communicator_t *remote_tmp;
	int local_root = 0, i, rank, grank, nb_task_involved, local_size, remote_leader_rank,
	    remote_size, remote_lleader, remote_rleader, rleader, lleader, local_id, remote_id;
	int *remote_task_list;
	/* get task id */

	rank = sctk_get_task_rank();
	sctk_barrier (local_comm);
	/* group rank */
	grank = sctk_get_rank ( local_comm, rank );
	sctk_nodebug ( "get grank %d from rank %d in comm %d", grank, rank, local_comm );
	/* get comm struct */
	tmp = sctk_get_internal_communicator ( local_comm );
	local_size = tmp->nb_task;
	lleader = local_leader;
	rleader = remote_leader;
	local_id = local_comm;
	
	/* Here we resolve the remote leader in te peer_comm */
	remote_leader_rank = sctk_get_comm_world_rank ( peer_comm, remote_leader );

	sctk_broadcast ((void*)&remote_leader,sizeof(int),local_leader,local_comm);
	
	sctk_nodebug ( "rank %d : sctk_intercomm_create, first = %d, local_comm %d, peer_comm %d, local_leader %d, remote_leader %d (%d)", rank, first, local_comm, peer_comm, local_leader, remote_leader, remote_leader_rank );

	//~ exchange local size
	if ( grank == local_leader )
	{
		sctk_sendrecv( &local_size, sizeof(int), remote_leader_rank, tag, &remote_size,  remote_leader_rank, SCTK_COMM_WORLD );
	}

	sctk_broadcast (&remote_size,sizeof(int),local_leader,local_comm);

	//~ exchange local tasks
	remote_task_list = sctk_malloc ( remote_size * sizeof ( int ) );

	if ( grank == local_leader )
	{
		int *task_list = sctk_malloc ( local_size * sizeof ( int ) );

		for ( i = 0; i < local_size; i++ )
		{
			task_list[i] = tmp->local_to_global[i];
		}
		
		/* Do a sendrecv with varying sizes */
		sctk_request_t task_sendreq, task_recvreq;
		sctk_message_isend( remote_leader_rank, task_list, local_size * sizeof(int), tag, SCTK_COMM_WORLD , &task_sendreq );
		sctk_message_irecv( remote_leader_rank, remote_task_list, remote_size * sizeof(int), tag, SCTK_COMM_WORLD , &task_recvreq );
		
		sctk_wait_message( &task_sendreq );
		sctk_wait_message( &task_recvreq );
	}

	sctk_broadcast (remote_task_list,(remote_size*sizeof(int)),local_leader,local_comm);

	//~ exchange leaders
	if ( grank == local_leader )
	{
		sctk_sendrecv( &lleader, sizeof(int), remote_leader_rank, tag, &remote_lleader,  remote_leader_rank, SCTK_COMM_WORLD );
	}

	sctk_broadcast (&remote_lleader,sizeof(int),local_leader,local_comm);

	if ( grank == local_leader )
	{
		sctk_sendrecv( &rleader, sizeof(int), remote_leader_rank, tag, &remote_rleader,  remote_leader_rank, SCTK_COMM_WORLD );
	}

	sctk_broadcast (&remote_rleader,sizeof(int),local_leader,local_comm);

	//~ exchange local comm ids
	if ( grank == local_leader )
	{
		sctk_sendrecv( &local_id, sizeof(int), remote_leader_rank, tag, &remote_id,  remote_leader_rank, SCTK_COMM_WORLD );
	}

	sctk_broadcast (&remote_id,sizeof(int),local_leader,local_comm);

	/* Fill the local structure */
	sctk_spinlock_lock ( & ( tmp->creation_lock ) );

	if ( tmp->new_comm == NULL )
	{
		int local_tasks = 0;
		int *local_to_global;
		int *global_to_local;
		int *task_to_process;
		sctk_process_ht_t *process = NULL;
		sctk_process_ht_t *tmp_process = NULL;
		sctk_process_ht_t *current_process = NULL;
		int *process_array;
		int process_nb = 0;

		local_root = 1;
		tmp->has_zero = 0;

		/* allocate new comm */
		local_tmp = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
		memset ( local_tmp, 0, sizeof ( sctk_internal_communicator_t ) );

		local_to_global = sctk_malloc ( local_size * sizeof ( int ) );
		global_to_local = sctk_malloc ( sctk_get_nb_task_total ( SCTK_COMM_WORLD ) * sizeof ( int ) );
		task_to_process = sctk_malloc ( local_size * sizeof ( int ) );

		/* fill new comm */
		for ( i = 0; i < local_size; i++ )
		{
			local_to_global[i] = tmp->local_to_global[i];
			global_to_local[local_to_global[i]] = i;
			task_to_process[i] = sctk_get_process_rank_from_task_rank ( local_to_global[i] );

			if ( task_to_process[i] == sctk_process_rank )
			{
				local_tasks++;
			}

			/* build list of processes */
			HASH_FIND_INT ( process, &task_to_process[i], tmp_process );

			if ( tmp_process == NULL )
			{
				tmp_process = sctk_malloc ( sizeof ( sctk_process_ht_t ) );
				tmp_process->process_id = task_to_process[i];
				HASH_ADD_INT ( process, process_id, tmp_process );
				process_nb ++;
			}
		}

		process_array = sctk_malloc ( process_nb * sizeof ( int ) );
		i = 0;
		HASH_ITER ( hh, process, current_process, tmp_process )
		{
			process_array[i++] = current_process->process_id;
			HASH_DEL ( process, current_process );
			free ( current_process );
		}
		tmp->new_comm = local_tmp;
		/* init new comm */
		sctk_communicator_init_intern_init_only ( local_size,
		                                          -1,
		                                          -1,
		                                          local_tasks,
		                                          local_to_global,
		                                          global_to_local,
		                                          task_to_process,
		                                          process_array,
		                                          process_nb,
		                                          tmp->new_comm );
		tmp->new_comm->is_inter_comm = 1;
		tmp->new_comm->local_leader = local_leader;
		tmp->new_comm->remote_leader = remote_leader;
		tmp->new_comm->peer_comm = peer_comm;
		tmp->new_comm->local_id = local_comm;
		tmp->new_comm->remote_id = remote_id;
		//~ --------------------------------------------------------

		/* Fill the remote structure */
		int  remote_local_tasks = 0;
		int *remote_local_to_global;
		int *remote_global_to_local;
		int *remote_task_to_process;
		sctk_process_ht_t *remote_process = NULL;
		sctk_process_ht_t *remote_tmp_process = NULL;
		sctk_process_ht_t *remote_current_process = NULL;
		int *remote_process_array;
		int remote_process_nb = 0;

		/* allocate new comm */
		remote_tmp = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
		memset ( remote_tmp, 0, sizeof ( sctk_internal_communicator_t ) );

		remote_local_to_global = sctk_malloc ( remote_size * sizeof ( int ) );
		remote_global_to_local = sctk_malloc ( sctk_get_nb_task_total ( SCTK_COMM_WORLD ) * sizeof ( int ) );
		remote_task_to_process = sctk_malloc ( remote_size * sizeof ( int ) );

		/* fill new comm */
		for ( i = 0; i < remote_size; i++ )
		{
			remote_local_to_global[i] = remote_task_list[i];
			remote_global_to_local[remote_local_to_global[i]] = i;
			remote_task_to_process[i] = sctk_get_process_rank_from_task_rank ( remote_local_to_global[i] );

			if ( remote_task_to_process[i] == sctk_process_rank )
			{
				remote_local_tasks++;
			}

			/* build list of processes */
			HASH_FIND_INT ( remote_process, &remote_task_to_process[i], remote_tmp_process );

			if ( remote_tmp_process == NULL )
			{
				remote_tmp_process = sctk_malloc ( sizeof ( sctk_process_ht_t ) );
				remote_tmp_process->process_id = remote_task_to_process[i];
				HASH_ADD_INT ( remote_process, process_id, remote_tmp_process );
				remote_process_nb ++;
			}
		}

		remote_process_array = sctk_malloc ( remote_process_nb * sizeof ( int ) );
		i = 0;
		HASH_ITER ( hh, remote_process, remote_current_process, remote_tmp_process )
		{
			remote_process_array[i++] = remote_current_process->process_id;
			HASH_DEL ( remote_process, remote_current_process );
			free ( remote_current_process );
		}

		tmp->remote_comm = remote_tmp;
		/* init new comm */
		sctk_communicator_init_intern_init_only ( remote_size,
		                                          -1,
		                                          -1,
		                                          remote_local_tasks,
		                                          remote_local_to_global,
		                                          remote_global_to_local,
		                                          remote_task_to_process,
		                                          remote_process_array,
		                                          remote_process_nb,
		                                          tmp->remote_comm );

		tmp->remote_comm->is_inter_comm = 1;
		tmp->remote_comm->new_comm = NULL;
		tmp->remote_comm->local_leader = remote_lleader;
		tmp->remote_comm->remote_leader = remote_rleader;
		tmp->remote_comm->peer_comm = peer_comm;
		tmp->remote_comm->local_id = remote_id;
		tmp->remote_comm->remote_id = local_comm;
	}

	sctk_spinlock_unlock ( & ( tmp->creation_lock ) );
	sctk_barrier(local_comm);

	remote_tmp = tmp->remote_comm;
	local_tmp = tmp->new_comm;

	if ( first )
	{
		local_tmp->remote_comm = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
		memset ( local_tmp->remote_comm, 0, sizeof ( sctk_internal_communicator_t ) );

		local_tmp->remote_comm = remote_tmp;
		assume ( local_tmp->remote_comm != NULL );
	}
	else
	{
		remote_tmp->remote_comm = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
		memset ( remote_tmp->remote_comm, 0, sizeof ( sctk_internal_communicator_t ) );

		remote_tmp->remote_comm = local_tmp;
		assume ( remote_tmp->remote_comm != NULL );
	}

	sctk_barrier(local_comm);

	if ( grank == local_leader )
	{
		local_root = 1;
		tmp->has_zero = 1;
	}

	sctk_barrier(local_comm);

	if ( ( grank != local_leader ) && ( tmp->has_zero == 1 ) )
	{
		local_root = 0;
	}

	sctk_nodebug ( "new INTERCOMM id for rank %d, grank %d, local_root %d, has_zero %d", rank, grank, local_root, tmp->has_zero );

	/* get new id for comm */
	if ( first )
	{
		assume ( local_tmp->remote_comm != NULL );
		comm = sctk_intercommunicator_get_new_id ( local_root, grank, local_leader, remote_leader, local_comm, peer_comm, local_tmp );
		sctk_nodebug("new INTERCOMM id for rank %d, grank %d, local_root %d, is inter %d, first %d", rank, grank, local_root, local_tmp->is_inter_comm, first);
	}
	else
	{
		assume ( remote_tmp->remote_comm != NULL );
		comm = sctk_intercommunicator_get_new_id ( local_root, grank, local_leader, remote_leader, local_comm, peer_comm, remote_tmp );
		sctk_nodebug("new INTERCOMM id for rank %d, grank %d, local_root %d, is inter %d, first %d", rank, grank, local_root, local_tmp->is_inter_comm, first);
	}

	sctk_nodebug ( "rank %d : intercomm created N° %d", rank, comm );
	/* Check if the communicator has been well created */
	sctk_get_internal_communicator ( comm );
	assume ( comm >= 0 );
	sctk_collectives_init_hook ( comm );
	sctk_network_notify_new_communicator(comm, local_size);

	assume ( comm != local_comm );

	tmp->new_comm = NULL;
	tmp->remote_comm = NULL;

	/*If not involved return SCTK_COMM_NULL*/
	for ( i = 0; i < local_size; i++ )
	{
		sctk_nodebug ( "task_list[%d] = %d, rank = %d", i, tmp->local_to_global[i], rank );

		if ( tmp->local_to_global[i] == rank )
		{
			sctk_nodebug ( "%d present in comm %d", rank, comm );
			return comm;
		}
	}

	return SCTK_COMM_NULL;
}

/************************* FUNCTION ************************/
/**
 * This method creates a new intercommunicator.
 * @param origin_communicator origin communicator.
 * @param nb_task_involved number of tasks involved.
 * @param task_list list of tasks involved.
 * @return the identification number of the new communicator.
**/
sctk_communicator_t sctk_create_communicator ( const sctk_communicator_t origin_communicator, const int nb_task_involved, const int *task_list )
{
	sctk_internal_communicator_t *tmp;
	sctk_internal_communicator_t *new_tmp;
	int local_root = 0;
	sctk_communicator_t comm;
	int i;
	int rank, grank;

	/* get task id */
	rank = sctk_get_task_rank();
	/* get group rank */
	grank = sctk_get_rank ( origin_communicator, rank );

	/* get comm struct */
	tmp = sctk_get_internal_communicator ( origin_communicator );
	sctk_spinlock_lock ( & ( tmp->creation_lock ) );

        if (tmp->new_comm == NULL) {
          int local_tasks = 0;
          int *local_to_global;
          int *global_to_local;
          int *task_to_process;
          sctk_process_ht_t *process = NULL;
          sctk_process_ht_t *tmp_process = NULL;
          sctk_process_ht_t *current_process = NULL;
          int *process_array;
          int process_nb = 0;

          local_root = 1;
          tmp->has_zero = 0;
          /* allocate new comm */
          new_tmp = sctk_malloc(sizeof(sctk_internal_communicator_t));
          memset(new_tmp, 0, sizeof(sctk_internal_communicator_t));

          local_to_global = sctk_calloc(nb_task_involved, sizeof(int));
          global_to_local =
              sctk_calloc(sctk_get_nb_task_total(SCTK_COMM_WORLD), sizeof(int));
          task_to_process = sctk_calloc(nb_task_involved, sizeof(int));

          /* fill new comm */
          for (i = 0; i < nb_task_involved; i++) {
            local_to_global[i] = task_list[i];
            global_to_local[local_to_global[i]] = i;
            task_to_process[i] =
                sctk_get_process_rank_from_task_rank(local_to_global[i]);

            if (task_to_process[i] == sctk_process_rank) {
              local_tasks++;
            }

            /* build list of processes */
            HASH_FIND_INT(process, &task_to_process[i], tmp_process);

            if (tmp_process == NULL) {
              tmp_process = sctk_malloc(sizeof(sctk_process_ht_t));
              tmp_process->process_id = task_to_process[i];
              HASH_ADD_INT(process, process_id, tmp_process);
              process_nb++;
            }
          }

          process_array = sctk_malloc(process_nb * sizeof(int));
          i = 0;
          HASH_ITER(hh, process, current_process, tmp_process) {
            process_array[i++] = current_process->process_id;
            HASH_DEL(process, current_process);
            sctk_free(current_process);
          }

          tmp->new_comm = new_tmp;
          /* init new comm */
          sctk_communicator_init_intern_init_only(
              nb_task_involved, -1, -1, local_tasks, local_to_global,
              global_to_local, task_to_process, process_array, process_nb,
              tmp->new_comm);

          tmp->new_comm->is_inter_comm = 0;
          tmp->new_comm->remote_leader = -1;
          tmp->new_comm->local_leader = 0;
          tmp->new_comm->new_comm = NULL;
          tmp->new_comm->remote_comm = NULL;
          tmp->new_comm->peer_comm = -1;
        }

        sctk_spinlock_unlock(&(tmp->creation_lock));
        sctk_barrier(origin_communicator);

        if (grank == 0) {
          local_root = 1;
          tmp->has_zero = 1;
        }

        sctk_barrier(origin_communicator);

        if ((grank != 0) && (tmp->has_zero == 1)) {
          local_root = 0;
        }

        new_tmp = tmp->new_comm;

        sctk_nodebug(
            "new id for rank %d, grank %d, local_root %d, has_zero %d, tmp %p",
            rank, grank, local_root, tmp->has_zero, tmp);
        /* get new id for comm */
        comm = sctk_communicator_get_new_id(local_root, grank,
                                            origin_communicator, new_tmp);

        /* Check if the communicator has been well created */
        sctk_get_internal_communicator(comm);
        assume(comm >= 0);
        sctk_collectives_init_hook(comm);

        tmp->new_comm = NULL;
        assume(new_tmp->id != origin_communicator);
        sctk_nodebug("new intracomm %d with size %d and local_comm %d",
                     new_tmp->id, new_tmp->nb_task, new_tmp->local_id);

	sctk_network_notify_new_communicator(new_tmp->id, new_tmp->nb_task);

        /*If not involved return SCTK_COMM_NULL*/
        for (i = 0; i < nb_task_involved; i++) {
          if (task_list[i] == rank) {
            return new_tmp->id;
          }
        }

        return SCTK_COMM_NULL;
}

/************************* FUNCTION ************************/
/**
 * This method creates a new communicator fropm an intercommunicator (for MPI_Intercomm_merge).
 * @param origin_communicator origin communicator.
 * @param nb_task_involved number of tasks involved.
 * @param task_list list of tasks involved.
 * @param is_inter_comm determine if it is an intercommunicator.
 * @return the identification number of the new communicator.
**/
sctk_communicator_t sctk_create_communicator_from_intercomm ( const sctk_communicator_t origin_communicator, const int nb_task_involved, const int *task_list, int first )
{
	sctk_internal_communicator_t *tmp;
	sctk_internal_communicator_t *new_tmp;
	int local_root = 0;
	sctk_communicator_t comm;
	int i, local_leader, remote_leader;
	int rank, grank;

	/* get task id */
	rank = sctk_get_task_rank();
	/* get group rank */
	grank = sctk_get_rank ( origin_communicator, rank );

	/* get comm struct */
	tmp = sctk_get_internal_communicator ( origin_communicator );

	if ( sctk_is_in_local_group ( origin_communicator ) )
	{
		local_leader = tmp->local_leader;
		remote_leader = tmp->remote_leader;
	}
	else
	{
		local_leader = tmp->remote_comm->local_leader;
		remote_leader = tmp->remote_comm->remote_leader;
	}

	sctk_spinlock_lock ( & ( tmp->creation_lock ) );

	if ( tmp->new_comm == NULL )
	{
		int local_tasks = 0;
		int *local_to_global;
		int *global_to_local;
		int *task_to_process;
		sctk_process_ht_t *process = NULL;
		sctk_process_ht_t *tmp_process = NULL;
		sctk_process_ht_t *current_process = NULL;
		int *process_array;
		int process_nb = 0;

		local_root = 1;
		tmp->has_zero = 0;
		/* allocate new comm */
		new_tmp = sctk_malloc ( sizeof ( sctk_internal_communicator_t ) );
		memset ( new_tmp, 0, sizeof ( sctk_internal_communicator_t ) );

		local_to_global = sctk_malloc ( nb_task_involved * sizeof ( int ) );
		global_to_local = sctk_malloc ( sctk_get_nb_task_total ( SCTK_COMM_WORLD ) * sizeof ( int ) );
		task_to_process = sctk_malloc ( nb_task_involved * sizeof ( int ) );

		//~ for(i = 0 ; i < sctk_get_nb_task_total(SCTK_COMM_WORLD) ; i++)
		//~ global_to_local[i] = -1;
		/* fill new comm */
		for ( i = 0; i < nb_task_involved; i++ )
		{
			local_to_global[i] = task_list[i];
			global_to_local[local_to_global[i]] = i;
			task_to_process[i] = sctk_get_process_rank_from_task_rank ( local_to_global[i] );

			if ( task_to_process[i] == sctk_process_rank )
			{
				local_tasks++;
			}

			/* build list of processes */
			HASH_FIND_INT ( process, &task_to_process[i], tmp_process );

			if ( tmp_process == NULL )
			{
				tmp_process = sctk_malloc ( sizeof ( sctk_process_ht_t ) );
				tmp_process->process_id = task_to_process[i];
				HASH_ADD_INT ( process, process_id, tmp_process );
				process_nb ++;
			}
		}

		process_array = sctk_malloc ( process_nb * sizeof ( int ) );
		i = 0;
		HASH_ITER ( hh, process, current_process, tmp_process )
		{
			process_array[i++] = current_process->process_id;
			HASH_DEL ( process, current_process );
			free ( current_process );
		}

		tmp->new_comm = new_tmp;
		/* init new comm */
		sctk_communicator_init_intern_init_only ( nb_task_involved,
		                                          -1,
		                                          -1,
		                                          local_tasks,
		                                          local_to_global,
		                                          global_to_local,
		                                          task_to_process,
		                                          process_array,
		                                          process_nb,
		                                          tmp->new_comm );
		tmp->new_comm->is_inter_comm = 0;
		tmp->new_comm->remote_leader = -1;
		tmp->new_comm->local_leader = 0;
		tmp->new_comm->new_comm = NULL;
		tmp->new_comm->remote_comm = NULL;
		tmp->new_comm->peer_comm = -1;
	}

	sctk_spinlock_unlock ( & ( tmp->creation_lock ) );

	sctk_barrier ( origin_communicator );

	if ( grank == local_leader )
	{
		local_root = 1;
		tmp->has_zero = 1;
	}

	sctk_barrier ( origin_communicator );

	if ( ( grank != local_leader ) && ( tmp->has_zero == 1 ) )
	{
		local_root = 0;
	}

	new_tmp = tmp->new_comm;

	sctk_nodebug ( "new id FROM INTERCOMM for rank %d, grank %d, local_root %d, has_zero %d, tmp %p", rank, grank, local_root, tmp->has_zero, tmp );
	/* get new id for comm */
	comm = sctk_communicator_get_new_id_from_intercomm ( local_root, grank, local_leader, remote_leader, origin_communicator, new_tmp );
	sctk_network_notify_new_communicator(comm, new_tmp->nb_task);
	sctk_nodebug ( "new comm from intercomm %d", comm );
	/* Check if the communicator has been well created */
	sctk_get_internal_communicator ( comm );
	assume ( comm >= 0 );
	sctk_collectives_init_hook ( comm );

	tmp->new_comm = NULL;
	assume ( new_tmp->id != origin_communicator );

	/*If not involved return SCTK_COMM_NULL*/
	for ( i = 0; i < nb_task_involved; i++ )
	{
		if ( task_list[i] == rank )
		{
			return new_tmp->id;
		}
	}

	return SCTK_COMM_NULL;
}

/************************* FUNCTION ************************/
/** 
 * This method creates a new intercommunicator from an intercommunicator
 * @param origin_communicator origin communicator.
 * @param nb_task_involved number of tasks involved in the local group.
 * @param task_list list of tasks involved in the local group.
 * @param local_comm local communicator for local group.
 * @return the identification number of the new communicator.
**/
sctk_communicator_t sctk_create_intercommunicator_from_intercommunicator (const sctk_communicator_t origin_communicator, const int nb_task_involved, const int *task_list, int remote_leader, int local_com)
{
	sctk_internal_communicator_t * tmp;
	sctk_communicator_t newintercomm;
	sctk_communicator_t peer_comm;
	sctk_status_t status;
	int local_comm;
	int local_leader = 0;
	int tag = 730;
	int first;
	int rank, grank;
	
	sctk_nodebug("sctk_create_intercommunicator_from_intercommunicator origin_comm %d, local_comm %d", origin_communicator, local_com);
	/* get task id */
	rank = sctk_get_task_rank();
	/* get group rank */
	grank = sctk_get_rank(origin_communicator, rank);
	local_comm = sctk_get_local_comm_id(origin_communicator);
	peer_comm = sctk_get_peer_comm(origin_communicator);
	
	/* get comm struct */
	tmp = sctk_get_internal_communicator(origin_communicator);
	
	if(sctk_is_in_local_group(origin_communicator))
		first = 1;
	else
		first = 0;
	
	sctk_nodebug("rank %d: newintercomm local_comm %d, local_leader %d, peer_comm %d, remote_leader %d, first %d", rank, local_com, local_leader, origin_communicator, remote_leader, first); 
	newintercomm = sctk_create_intercommunicator (local_com, local_leader, peer_comm, remote_leader, tag, first);
	
	return newintercomm;
}

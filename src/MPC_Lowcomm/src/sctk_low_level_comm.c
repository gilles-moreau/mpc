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
#include <mpc_config.h>


#include "sctk_low_level_comm.h"


#include <mpc_lowcomm.h>


#include <mpc_launch_pmi.h>

#include "alloc_mem.h"

#include "mpc_common_debug.h"

#include "sctk_checksum.h"

#include <mpc_launch.h>

#include <mpc_topology_device.h>

#include <mpc_common_rank.h>
#include <sctk_alloc.h>

#include <string.h>

/*Networks*/
#ifdef MPC_USE_INFINIBAND
#include <ibmpi.h>
#endif

#ifdef MPC_USE_OFI
#include <ofi.h>
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_portals.h>
#endif

#include <sctk_alloc.h>

#ifdef MPC_USE_DMTCP
#include "sctk_ft_iface.h"
#endif

#include "lowcomm_config.h"


/************************************************************************/
/* Config Helpers                                                       */
/************************************************************************/

/** \brief Get a pointer to the global network configuration
 *   \return the network config (static struct)
 */
static inline const struct _mpc_lowcomm_config_struct_networks *sctk_net_get_config()
{
	return _mpc_lowcomm_config_net_get();
}

/** \brief Get a pointer to a driver config
 *   \param name Name of the requested driver config
 *   \return The driver config or NULL
 */
struct _mpc_lowcomm_config_struct_net_driver_config *sctk_get_driver_config_by_name(char *name)
{
	return _mpc_lowcomm_conf_driver_unfolded_get(name);
}

/************************************************************************/
/* Network INIT                                                         */
/************************************************************************/

static inline int __unfold_rails(mpc_conf_config_type_t *cli_option)
{
	/* Now compute the actual total number of rails knowing that
	 * some rails might contain subrails */

	int k;
	int total_rail_nb = 0;

	for(k = 0; k < mpc_conf_config_type_count(cli_option); ++k)
	{
		mpc_conf_config_type_elem_t *erail = mpc_conf_config_type_nth(cli_option, k);

		/* Get the rail */
		struct _mpc_lowcomm_config_struct_net_rail *rail = _mpc_lowcomm_conf_rail_unfolded_get(mpc_conf_type_elem_get_as_string(erail) );

		if(!rail)
		{
			bad_parameter("Could not find a rail config named %s", rail);
		}

		/* Here we have two cases, first the rail device name is a regexp (begins with !)
		 * or the subrails are already defined with their device manually set  */

		/* Case with preset subrails */
		if(rail->subrails_size)
		{
			/* Count subrails */
			total_rail_nb += rail->subrails_size;
			snprintf(rail->topology, MPC_CONF_STRING_SIZE, "none");
		}
		else
		{
			if(strlen(rail->device) )
			{
				if(rail->device[0] == '!')
				{
					/* We need to rebuild subrails
					 * according to devices */
					int matching_rails = 0;

					/* +1 to skip the ! */
					mpc_topology_device_t **matching_device = mpc_topology_device_get_from_handle_regexp(rail->device + 1,
					                                                                                     &matching_rails);

					if(matching_rails == 0)
					{
						/* This rail does not have subrails as no device matched */
						rail->subrails_size = -1;
						//total_rail_nb++;
					}
					else
					{
						/* Now we build the subrail array
						* we duplicate the config of current rail
						* while just overriding device name with the
						* one we extracted from the device array */
						struct _mpc_lowcomm_config_struct_net_rail *subrails =
							sctk_malloc(sizeof(struct _mpc_lowcomm_config_struct_net_rail) * matching_rails);

						int i;

						for(i = 0; i < matching_rails; i++)
						{
							/* Copy Current rail (note that at this point the rail has no subrails ) */
							memcpy(&subrails[i], rail, sizeof(struct _mpc_lowcomm_config_struct_net_rail) );

							/* Update device name with matching device */
							snprintf(subrails[i].device, MPC_CONF_STRING_SIZE, "%s", matching_device[i]->name);

							/* Make sure that the topological rail has the highest priority
							* this is important during the on-demand election process
							* as we want the topological rail to make this choice */
							subrails[i].priority = rail->priority - 1;

							/* Make sure that subrails do not have a subrail */
							subrails[i].subrails_size = 0;
						}

						sctk_free(matching_device);

						/* Store the new subrail array in the rail */
						rail->subrails      = subrails;
						rail->subrails_size = matching_rails;

						/* Increment total rails by matching rails */
						total_rail_nb += matching_rails;
					}
				}
			}
		}

		/* Count this rail */
		total_rail_nb++;
	}

	return total_rail_nb;
}

void sctk_rail_init_driver(sctk_rail_info_t *rail, int driver_type)
{
	/* Switch on the driver to use */
	switch(driver_type)
	{
#ifdef MPC_USE_INFINIBAND
		case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
			sctk_network_init_mpi_ib(rail);
			break;
#endif
#ifdef MPC_USE_PORTALS
		case SCTK_RTCFG_net_driver_portals: /* PORTALS */
			sctk_network_init_ptl(rail);
			break;
#endif
#ifdef MPC_USE_OFI
		case SCTK_RTCFG_net_driver_ofi: /* OFI */
			sctk_network_init_ofi(rail);
			break;
#endif
		case SCTK_RTCFG_net_driver_topological:
			sctk_network_init_topological(rail);
			break;

		case SCTK_RTCFG_net_driver_tcp:
			sctk_network_init_tcp(rail);
			break;

		case SCTK_RTCFG_net_driver_shm:
			sctk_network_init_shm(rail);
			break;


		default:
			mpc_common_debug_fatal("No such network type");
			break;
	}
}

void sctk_net_init_task_level(int taskid, int vp)
{
	int i;

	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		if(rail->initialize_task)
		{
			rail->initialize_task(rail, taskid, vp);
		}
	}
}

void sctk_net_finalize_task_level(int taskid, int vp)
{
	int i;

	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		if(rail->finalize_task)
		{
			rail->finalize_task(rail, taskid, vp);
		}
	}
}

/********************************************************************/
/* Memory Allocator                                                 */
/********************************************************************/

#define IB_MEM_THRESHOLD_ALIGNED_SIZE    (256 * 1024) /* RDMA threshold */
#define IB_MEM_ALIGNMENT                 (4096)       /* Page size */

static inline size_t __mpc_memory_allocation_hook_ib(size_t size)
{
	if(size > IB_MEM_THRESHOLD_ALIGNED_SIZE)
	{
		return (size + (IB_MEM_ALIGNMENT - 1) ) & (~(IB_MEM_ALIGNMENT - 1) );
	}

	return 0;
}

#ifdef MPC_Allocator

static size_t __mpc_memory_allocation_hook(size_t size_origin)
{
	#ifdef MPC_USE_INFINIBAND
	if(sctk_network_is_ib_used() )
	{
		return __mpc_memory_allocation_hook_ib(size_origin);
	}
	#else
	UNUSED(size_origin);
	#endif
	return 0;
}

void __mpc_memory_free_hook(void *ptr, size_t size)
{
	//mpc_common_debug_error("FREE %p size %ld", ptr, size);

	if(mpc_lowcomm_allocmem_is_in_pool(ptr))
	{
		//mpc_common_debug_error("FREE pool");
		mpc_lowcomm_allocmem_pool_free_size(ptr, size);
	}

	#ifdef MPC_USE_INFINIBAND
	if(sctk_network_is_ib_used() )
	{
		return sctk_network_memory_free_hook_ib(ptr, size);
	}
	#else
	UNUSED(ptr);
	UNUSED(size);
	#endif
}

#endif

void sctk_net_init_driver(char *name)
{
#ifdef MPC_Allocator
	sctk_net_memory_allocation_hook_register(__mpc_memory_allocation_hook);
	sctk_net_memory_free_hook_register(__mpc_memory_free_hook);
#endif



	/* Retrieve default network from config */
	char *option_name = _mpc_lowcomm_config_net_get()->cli_default_network;

	/* If we have a network name from the command line (through sctk_launch.c)  */
	if(name != NULL)
	{
		option_name = name;
	}

	/* Here we retrieve the network configuration from the network list
	 * according to its name */

	mpc_common_nodebug("Run with driver %s", option_name);

	mpc_conf_config_type_t *cli_option = _mpc_lowcomm_conf_cli_get(option_name);

	if(cli_option == NULL)
	{
		/* We did not find this name in the network configurations */
		bad_parameter("No configuration found for the network '%s'. Please check you '-net=' argument"
		              " and your configuration file", option_name);
	}

	/* This call counts rails while also unfolding subrails */
	int total_rail_nb = __unfold_rails(cli_option);

	/* Allocate Rails Storage we need to do it at once as we are going to
	 * distribute pointers to rails to every modules therefore, they
	 * should not change during the whole execution */
	sctk_rail_allocate(total_rail_nb);

	if(255 < total_rail_nb)
	{
		mpc_common_debug_fatal("There cannot be more than 255 rails");

		/* If you want to remove this limation make sure that
		 * the rail ID is encoded with a larger type in
		 * struct mpc_lowcomm_ptp_ctrl_message_header_s */
	}

	int k;

	for(k = 0; k < mpc_conf_config_type_count(cli_option); k++)
	{
		mpc_conf_config_type_elem_t *erail = mpc_conf_config_type_nth(cli_option, k);
		char *rail_name = mpc_conf_type_elem_get_as_string(erail);

		/* For each RAIL */
		struct _mpc_lowcomm_config_struct_net_rail *rail_config_struct;
#ifndef MPC_USE_INFINIBAND
		if(strcmp(rail_name, "ib_mpi") == 0)
		{
			mpc_common_debug_warning("Network support %s not available switching to tcp_mpi", rail_name);
			rail_name = "tcpmpi";
		}
#endif
#ifndef MPC_USE_PORTALS
		if(strcmp(rail_name, "portals_mpi") == 0)
		{
			mpc_common_debug_warning("Network support %s not available switching to tcp_mpi", rail_name);
			rail_name = "tcpmpi";
		}
#endif
		rail_config_struct = _mpc_lowcomm_conf_rail_unfolded_get(rail_name);

		if(rail_config_struct == NULL)
		{
			mpc_common_debug_error("Rail with name '%s' not found in config!", rail_name);
			mpc_common_debug_abort();
		}

		/* Handle the case where no matching decice was found and flag as no_device */
		if( (rail_config_struct->subrails_size == -1) && (mpc_common_get_process_rank() == 0) )
		{
			mpc_common_debug_warning("Rail %s expect %s devices but none were found", rail_config_struct->name, rail_config_struct->device);
			mpc_common_debug_warning("MPC will fallback to TCP, consider changing your default network");
			rail_name = "tcpmpi";
			rail_config_struct = _mpc_lowcomm_conf_rail_unfolded_get("tcpmpi");
			assume(rail_config_struct != NULL);
		}

		/* For this rail retrieve the config */
		struct _mpc_lowcomm_config_struct_net_driver_config *driver_config = sctk_get_driver_config_by_name(rail_config_struct->config);

		if(driver_config == NULL)
		{
			/* This can only be accepted for topological rails */
			if(rail_config_struct->subrails_size == 0)
			{
				mpc_common_debug_error("Driver with name '%s' not found in config!", rail_config_struct->config);
				continue;
			}
		}

		/* Register and initalize new rail inside the rail array */
		sctk_rail_register(rail_config_struct, driver_config);
	}

	sctk_rail_commit();
	sctk_checksum_init();
}

/********************************************************************/
/* Hybrid MPI+X                                                     */
/********************************************************************/
static int is_mode_hybrid = 0;

int sctk_net_is_mode_hybrid()
{
	return is_mode_hybrid;
}

int sctk_net_set_mode_hybrid()
{
	is_mode_hybrid = 1;
	return 0;
}

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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

/********************  HEADERS  *********************/
#include <assert.h>
#include <string.h>
#include "sctk_debug.h"
#include "sctk_runtime_config.h"
#include "sctk_runtime_config_sources.h"
#include "sctk_runtime_config_mapper.h"
#include "sctk_runtime_config_printer.h"
#include "sctk_runtime_config_validation.h"
#include "sctk_runtime_config_struct_defaults.h"
#include "sctk_libxml_helper.h"

/*********************  GLOBAL  *********************/
/**
 * Global variable to store mpc runtime configuration loaded from XML.
**/
struct sctk_runtime_config __sctk_global_runtime_config__;
/** To know if aldready init. **/
bool __sctk_global_runtime_config_init__ = false;

/*********************  CONSTS  *********************/
/**
 * Declare the global symbol containing the meta description for mapping XML to C struct.
 * The content of this table is generated by XSLT files at build time.
**/
extern const struct sctk_runtime_config_entry_meta sctk_runtime_config_db[];

/*******************  FUNCTION  *********************/
void sctk_runtime_config_map_profile_entry(void * value, xmlNodePtr node,const char * entry_name,const char * entry_c_type)
{
	//vars
	xmlNodePtr entry;
	const struct sctk_runtime_config_entry_meta * entry_meta;

	//errors
	assert(value != NULL);
	assert(node != NULL);
	assert(xmlStrcmp(node->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0);

	//get the entry while taking in XML childs
	entry = sctk_libxml_find_child_node(node,BAD_CAST(entry_name));

	//if have entry, map
	if (entry != NULL)
	{
		//get meta description of entry
		entry_meta = sctk_runtime_config_get_meta_type(sctk_runtime_config_db, entry_c_type);

		//check error
		assume_m(entry_meta != NULL,"Invalid type name : %s.",entry_c_type);

		//map the struct
		sctk_runtime_config_map_struct(sctk_runtime_config_db,value,entry_meta,entry);
	}
}

/*******************  FUNCTION  *********************/
/**
 * Map the content of a profile XML node on MPC configuration structure.
 * @param config Define the configration structure to use.
 * @param node Define the XML node of the profile. Must be a &lt;profile&gt; balisis.
**/
void sctk_runtime_config_map_profile( struct sctk_runtime_config * config, xmlNodePtr node)
{
	//errors
	assert(config != NULL);
	assert(node != NULL);
	assert(xmlStrcmp(node->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0);

	sctk_runtime_config_map_profile_entry(&config->modules,node,SCTK_RUNTIME_CONFIG_XML_NODE_MODULES,"sctk_runtime_config_modules");
	sctk_runtime_config_map_profile_entry(&config->networks,node,SCTK_RUNTIME_CONFIG_XML_NODE_NETWORKS,"sctk_runtime_config_struct_networks");
}

/*******************  FUNCTION  *********************/
/**
 * Run over all configuration sources to map them onto the C configuration structure.
 * @param config Define the configuration structure to use.
 * @param config_sources Define the configuration source the use.
**/
void sctk_runtime_config_runtime_map_sources( struct sctk_runtime_config * config,struct sctk_runtime_config_sources * config_sources)
{
	//vars
	int i;

	//errors
	assert(config != NULL);
	assert(config_sources != NULL);

	//apply default
	memset(config,0,sizeof(*config));
	sctk_runtime_config_reset(config);

	//run over profiles
	for (i = 0 ; i < config_sources->cnt_profile_nodes ; ++i)
		sctk_runtime_config_map_profile(config,config_sources->profile_nodes[i]);
}


/*******************  FUNCTION  *********************/
/**
 * Display the configuration structure in standard output.
**/
void sctk_runtime_config_runtime_display(void)
{
	//error
	assert(__sctk_global_runtime_config_init__);

	//separator
	printf("====================== MPC CONFIG ======================\n");
	sctk_runtime_config_display_tree(sctk_runtime_config_db,"config","sctk_runtime_config",&__sctk_global_runtime_config__);
	printf("\n");
	printf("========================================================\n");
}


/*******************  FUNCTION  *********************/
/**
 * Function to cleanup the structure. Caution, be sure that it will not serve anymore.
**/
void sctk_runtime_config_runtime_cleanup(void)
{
	//ceanlup struct
	sctk_runtime_config_do_cleanup(&__sctk_global_runtime_config__);
}

/*******************  FUNCTION  *********************/
/**
 * Replacement for libxml handler but done nothing.
 * \TODO Improve by printing at least on MPI task 0, but not sure that we can get this informations
 * when MPC_Config is init.
 */
void sctk_runtime_config_libxml_silent_error_handler(void * ctx,const char * msg,...)
{

}

/*******************  FUNCTION  *********************/
/**
 * Provide our own error handling to avoid printing too much messages while running on
 * large clusters. This avoid to flood the terminal in case of error.
 * We count on mpcrun to pass the MPC_CONFIG_SILENT variable when using a large number po
 * nodes to disable libxml output in such case.
 */
void sctk_runtime_config_made_libxml_silent(void)
{
	//check env var MPC_CONFIG_SILENT.
	const char * silent = getenv("MPC_CONFIG_SILENT");

	if (silent == NULL)
	{
		//if not defined, nothing to do
		return;
	} else if (strcmp(silent,"1") == 0) {
		//ok, replace the error handler
		xmlSetGenericErrorFunc (NULL, sctk_runtime_config_libxml_silent_error_handler);
	}
}

/*******************  FUNCTION  *********************/
/**
 * Function used to initilized the MPC configuration structure. It load the XML files, parse them and
 * map the content on the global C structure.
 * CAUTION : This method is not thread safe before end of first execution.
**/
void sctk_runtime_config_init(void)
{
	//declare temp storage
	struct sctk_runtime_config_sources config_sources;
	const char * disable_config;

	//if already done do nothing
	if (__sctk_global_runtime_config_init__ == false)
	{
		disable_config = sctk_runtime_config_get_env_or_value("MPC_DISABLE_CONFIG","0");
		if (strcmp(disable_config,"1") == 0)
		{
			//reset the structure
			sctk_runtime_config_reset(&__sctk_global_runtime_config__);

			//validate
			sctk_runtime_config_validate(&__sctk_global_runtime_config__);

			//mark as init
			__sctk_global_runtime_config_init__ = true;
		} else {
			//init libxml (safer to manually call it in multi-thread environnement as not threadsafe)
			xmlInitParser();

			//made silent errors
			sctk_runtime_config_made_libxml_silent();

			//open
			sctk_runtime_config_sources_open(&config_sources);

			//map to c struct
			sctk_runtime_config_runtime_map_sources( &__sctk_global_runtime_config__,&config_sources);

			//close
			sctk_runtime_config_sources_close(&config_sources);

			//validate
			sctk_runtime_config_validate(&__sctk_global_runtime_config__);

			//mark as init
			__sctk_global_runtime_config_init__ = true;
		}
	}
}



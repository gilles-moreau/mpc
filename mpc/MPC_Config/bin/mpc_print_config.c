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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sctk_debug.h"
#include "sctk_runtime_config_mapper.h"
#include "sctk_runtime_config_walk.h"
#include "sctk_runtime_config.h"
#include "mpc_print_config_sh.h"
#include "mpc_print_config_xml.h"
#include <sctk_runtime_config_struct.h>

/**********************  ENUM  **********************/
/** Output mode availables to print the config. **/
enum output_mode
{
	/** Output an XML file which can be reuse as input configuration file. **/
	OUTPUT_MODE_XML,
	/** Print a more human readable text format. **/
	OUTPUT_MODE_TEXT,
	/** Display only the entries for launcher and format it in shell variables format. **/
	OUTPUT_MODE_LAUNCHER
};

/*********************  STRUCT  *********************/
/** Structure to store parsing status of all available options for the command. **/
struct command_options
{
	/** Setup the output mode (--xml, --launcher, --text). **/
	enum output_mode mode;
	/** Setup the system file to load (--system). **/
	char * system_file;
	/** Setup the user file to load (--user). **/
	char * user_file;
	/** Setup the application file to load (--app). **/
	char * app_file;
	/** Request the help of the command (--help). **/
	bool help;
	/** Disable the loading of config files (--nofile). **/
	bool nofile;
};

/*********************  CONSTS  *********************/
/**
 * Declare the global symbol containing the meta description for mapping XML to C struct.
 * The content of this table is generated by XSLT files at build time.
**/
extern const struct sctk_runtime_config_entry_meta sctk_runtime_config_db[];
/** Text for --help **/
static const char * cst_help_message = "\
mpc_print_config [--text|--xml|--launcher|--help]\n\
\n\
Options :\n\
  --text          : Display the current configuration in simple text format.\n\
  --xml           : Display the current configuration in MPC config XML format.\n\
  --launcher      : Display launcher options in bash variable format.\n\
  --system={file} : Override the system configuration file.\n\
  --user={file}   : Override the user configuration file.\n\
  --app={file}    : Override the application configuration file.\n\
  --nofile        : Only use the internal default values.\n\
\n\
You can olaso influence the loaded files with environnement variables :\n\
  - MPC_SYSTEM_CONFIG   : System configuration file (" SCTK_INSTALL_PREFIX "/share/mpc/config.xml)\n\
  - MPC_USER_CONFIG     : User configuration file (~/.mpc/config.xml)\n\
  - MPC_APP_CONFIG      : Application configuration file (disabled)\n\
  - MPC_DISABLE_CONFIG  : Disable loading of configuration files if setup to 1.\n";

/*******************  FUNCTION  *********************/
/**
 * Display the configuration for the launcher script (mpcrun) in sh format.
 * It will only display the section config.modules.launcher.
**/
void display_launcher(const struct sctk_runtime_config * config)
{
	display_tree_sh(sctk_runtime_config_db, "config_launcher","sctk_runtime_config_struct_launcher", (void*)&config->modules.launcher);
}


/*******************  FUNCTION  *********************/
/**
 * Standard display of the sturct to see the complete tee
**/
void display_all(const struct sctk_runtime_config * config)
{
	sctk_runtime_config_runtime_display();
}

/*******************  FUNCTION  *********************/
/**
 * Standard display of the sturct to see the complete tee
**/
void display_xml(const struct sctk_runtime_config * config)
{
	mpc_print_config_xml(sctk_runtime_config_db,"config","sctk_runtime_config",(void*)config);
}

/*******************  FUNCTION  *********************/
/**
 * Display help for command arguments.
**/
void display_help(void)
{
	puts(cst_help_message);
}

/*******************  FUNCTION  *********************/
void init_default_options(struct command_options * options)
{
	//errors
	assert(options != NULL);

	//default values
	options->help        = false;
	options->mode        = OUTPUT_MODE_TEXT;
	options->system_file = NULL;
	options->user_file   = NULL;
	options->app_file    = NULL;
	options->nofile      = false;
}

/*******************  FUNCTION  *********************/
/** Function to parse command line arguments. **/
bool parse_args(struct command_options * options,int argc, char ** argv)
{
	//vars
	int i;

	//errors
	assert(options != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	//setup default
	init_default_options(options);

	//read args
	for ( i = 1 ; i < argc ; i++)
	{
		if (argv[i][0] != '-' || argv[i][1] != '-')
		{
			//error
			fprintf(stderr,"Error : invalid argument %s\n",argv[i]);
			return false;
		} else {
			//find the good one
			if (strcmp(argv[i],"--help") == 0)
			{
				options->help = true;
			} else if (strcmp(argv[i],"--nofile") == 0) {
				options->nofile = true;
			} else if (strcmp(argv[i],"--text") == 0) {
				options->mode = OUTPUT_MODE_TEXT;
			} else if (strcmp(argv[i],"--xml") == 0) {
				options->mode = OUTPUT_MODE_XML;
			} else if (strcmp(argv[i],"--launcher") == 0) {
				options->mode = OUTPUT_MODE_LAUNCHER;
			} else if (strncmp(argv[i],"--system=",9) == 0) {
				options->system_file = strdup(argv[i]+9);
			} else if (strncmp(argv[i],"--user=",7) == 0) {
				options->user_file = strdup(argv[i]+7);
			} else if (strncmp(argv[i],"--app=",6) == 0) {
				options->app_file = strdup(argv[i]+6);
			} else {
				fprintf(stderr,"Error : invalid argument %s\n",argv[i]);
				return false;
			}
		}
	}

	return true;
}

/*******************  FUNCTION  *********************/
void cleanup_options(struct command_options * options)
{
	//errors
	assert(options != NULL);

	//free mem
	if (options->system_file != NULL)
		free(options->system_file);
	if (options->user_file != NULL)
		free(options->user_file);
	if (options->app_file != NULL)
		free(options->app_file);
}

/*******************  FUNCTION  *********************/
int load_and_print_mpc_config(const struct command_options * options)
{
	//vars
	int res = EXIT_SUCCESS;

	//overwrite files
	if (options->system_file != NULL)
		setenv("MPC_SYSTEM_CONFIG",options->system_file,1);
	if (options->user_file != NULL)
		setenv("MPC_USER_CONFIG",options->user_file,1);
	if (options->app_file != NULL)
		setenv("MPC_APP_CONFIG",options->app_file,1);
	if (options->nofile)
		setenv("MPC_DISABLE_CONFIG","1",1);

	//load the config
	const struct sctk_runtime_config * config = sctk_runtime_config_get();

	//print
	switch(options->mode)
	{
		case OUTPUT_MODE_TEXT:
			display_all(config);;
			break;
		case OUTPUT_MODE_XML:
			display_xml(config);
			break;
		case OUTPUT_MODE_LAUNCHER:
			display_launcher(config);
			printf("CONFIG_FILES_VALID=true\n");
			break;
		default:
			fprintf(stderr,"Error : invalide output mode : %d\n",options->mode);
			res = EXIT_FAILURE;
	}

	//final return
	return res;
}

/*******************  FUNCTION  *********************/
int main(int argc, char ** argv)
{
	//vars
	int res = EXIT_SUCCESS;
	struct command_options options;

	//parse options
	if (parse_args(&options,argc,argv))
	{
		if (options.help)
		{
			display_help();
		} else {
			res = load_and_print_mpc_config(&options);
		}
	} else {
		res = EXIT_FAILURE;
	}

	//cleanup
	cleanup_options(&options);

	//return final status
	return res;
}

/*******************  FUNCTION  *********************/
/**
 * This is a simply way to avoid link error and avoid to init mpc just to load the config.
**/
int mpc_user_main__(void)
{
	return EXIT_FAILURE;	
}

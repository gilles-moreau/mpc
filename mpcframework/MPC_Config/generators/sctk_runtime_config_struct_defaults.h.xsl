<?xml version="1.0"?>
<!--
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - VALAT Sebastien sebastien.valat@cea.fr                           #
#   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     #
#                                                                      #
########################################################################
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:text>#include &lt;stdlib.h&gt;&#10;</xsl:text>
		<xsl:text>#include &lt;dlfcn.h&gt;&#10;</xsl:text>
		<xsl:text>#include "uthash.h"&#10;&#10;</xsl:text>
		<xsl:text>#ifndef SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H&#10;</xsl:text>
		<xsl:text>#define SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H&#10;</xsl:text>
		<xsl:call-template name="gen-var-decl"/>
		<xsl:call-template name="gen-forward-struct-decl"/>
		<xsl:call-template name="gen-reset-function"/>
		<xsl:text>void sctk_runtime_config_clean_hash_tables();&#10;</xsl:text>
		<xsl:text>void* sctk_runtime_config_get_symbol();&#10;</xsl:text>
		<xsl:text>&#10;#endif /* SCTK_RUNTIME_CONFIG_STRUCT_DEFAULTS_H */&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-var-decl">
		<xsl:text>&#10;/******************************** VARIABLES *********************************/&#10;</xsl:text>
		<xsl:text>void * sctk_handler;&#10;</xsl:text>
		
		<xsl:text>&#10;struct enum_value {&#10;</xsl:text>
		<xsl:text>&#9;char name[50];&#10;</xsl:text>
		<xsl:text>&#9;int value;&#10;</xsl:text>
		<xsl:text>&#9;UT_hash_handle hh;&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
		
		<xsl:text>&#10;struct enum_type {&#10;</xsl:text>
		<xsl:text>&#9;char name[50];&#10;</xsl:text>
		<xsl:text>&#9;struct enum_value * values;&#10;</xsl:text>
		<xsl:text>&#9;UT_hash_handle hh;&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
		<xsl:text>&#10;struct enum_type * enums_types;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-forward-struct-decl">
		<xsl:text>&#10;/******************************** STRUCTURE *********************************/&#10;</xsl:text>
		<xsl:text>/* forward declaration functions */&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config_funcptr;&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-reset-function">
		<xsl:text>&#10;/********************************* FUNCTION *********************************/&#10;</xsl:text>
		<xsl:text>/* reset functions */&#10;</xsl:text>
		<xsl:apply-templates select="config/usertypes"/>
		<xsl:text>void sctk_runtime_config_reset(struct sctk_runtime_config * config);&#10;</xsl:text>
		<xsl:text>void sctk_runtime_config_reset_struct_default_if_needed(char * structname, void * ptr );&#10;</xsl:text>
		<xsl:text>void * sctk_runtime_config_get_union_value_offset(char * unionname, void * ptr );&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select="struct|union"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct|union">
		<xsl:call-template name="gen-reset-func-name"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-reset-func-name">
		<xsl:value-of select="concat('void sctk_runtime_config_struct_init_',@name,'(void * struct_ptr);&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="handler">
		<xsl:value-of select="concat('void ',.,'(struct sctk_runtime_config * config);&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-mpc-header">
<xsl:text>/* ############################# MPC License ############################## */
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
/* #   - AUTOMATIC GENERATION                                             # */
/* #                                                                      # */
/* ######################################################################## */

</xsl:text>
	</xsl:template>
	
</xsl:stylesheet>

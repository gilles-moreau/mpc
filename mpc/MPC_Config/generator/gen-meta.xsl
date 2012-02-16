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
#                                                                      #
########################################################################
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:text>#include "sctk_config_struct.h"&#10;</xsl:text>
		<xsl:text>#include "src/sctk_config_mapper.h"&#10;</xsl:text>
		<xsl:call-template name="gen-meta"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-meta">
		<xsl:text>&#10;/*********************  CONSTS  *********************/&#10;</xsl:text>
		<xsl:text>const struct sctk_config_entry_meta sctk_config_db[] = {&#10;</xsl:text>
		<xsl:call-template name="profile-meta"/>
		<xsl:call-template name="modules-meta"/>
		<xsl:apply-templates select="config"/>
		<xsl:call-template name="end-marker"/>
		<xsl:text>};&#10;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select="usertypes"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select="struct"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:text>&#09;//struct&#10;</xsl:text>
		<xsl:text>&#09;{"sctk_config_module_</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_config_module_</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>) , NULL , sctk_config_module_init_</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>},&#10;</xsl:text>
		<xsl:apply-templates select="param"/>
		<xsl:apply-templates select="array"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:text>&#09;{"</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>"     , SCTK_CONFIG_META_TYPE_PARAM  , </xsl:text>
		<xsl:value-of select="concat('sctk_config_get_offset_of_member(struct sctk_config_module_',../name,',',name,')')"/>
		<xsl:text>  , sizeof(</xsl:text>
		<xsl:call-template name="gen-type-name"/>
		<xsl:text>) , "</xsl:text>
		<xsl:call-template name="gen-type-name2"/>
		<xsl:text>" , </xsl:text>
		<xsl:call-template name="gen-init-name"/>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:text>&#09;{"</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>"     , SCTK_CONFIG_META_TYPE_ARRAY  , </xsl:text>
		<xsl:value-of select="concat('sctk_config_get_offset_of_member(struct sctk_config_module_',../name,',',name,')')"/>
		<xsl:text> , sizeof(</xsl:text>
		<xsl:call-template name="gen-type-name"/>
		<xsl:text>) , "</xsl:text>
		<xsl:call-template name="gen-type-name2"/>
		<xsl:text>" , "</xsl:text>
		<xsl:value-of select="entry-name"/>
		<xsl:text>"},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-init-name">
		<xsl:choose>
			<xsl:when test="type = 'int'">NULL</xsl:when>
			<xsl:when test="type = 'bool'">NULL</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="concat('sctk_config_module_init_',type)"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-type-name">
		<xsl:choose>
			<xsl:when test="type = 'int'">int</xsl:when>
			<xsl:when test="type = 'bool'">bool</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="gen-user-type-name">
					<xsl:with-param name="type"><xsl:value-of select='type'/></xsl:with-param>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-type-name2">
		<xsl:choose>
			<xsl:when test="type = 'int'">int</xsl:when>
			<xsl:when test="type = 'bool'">bool</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="gen-user-type-name2">
					<xsl:with-param name="type"><xsl:value-of select='type'/></xsl:with-param>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-user-type-name">
		<xsl:param name="type"/>
		<xsl:for-each select="//struct[name = $type]">
			<xsl:value-of select="concat('struct sctk_config_module_',$type)"/>
		</xsl:for-each>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-user-type-name2">
		<xsl:param name="type"/>
		<xsl:for-each select="//struct[name = $type]">
			<xsl:value-of select="concat('sctk_config_module_',$type)"/>
		</xsl:for-each>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="end-marker">
		<xsl:text>&#09;//end marker&#10;</xsl:text>
		<xsl:text>&#09;{NULL , SCTK_CONFIG_META_TYPE_END , 0 , 0 , NULL,  NULL}&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="profile-meta">
		<xsl:text>&#09;//sctk_config&#10;</xsl:text>
		<xsl:text>&#09;{"sctk_config" , SCTK_CONFIG_META_TYPE_STRUCT , 0  , sizeof(struct sctk_config) , NULL , sctk_config_reset},&#10;</xsl:text>
		<xsl:text>&#09;{"modules"     , SCTK_CONFIG_META_TYPE_PARAM  , 0  , sizeof(struct sctk_config_modules) , "sctk_config_modules" , NULL},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="modules-meta">
		<xsl:text>&#09;//sctk_config_modules&#10;</xsl:text>
		<xsl:text>&#09;{"sctk_config_modules" , SCTK_CONFIG_META_TYPE_STRUCT , 0 , sizeof(struct sctk_config) , NULL , NULL},&#10;</xsl:text>
		<xsl:for-each select="config">
			<xsl:apply-templates select="modules"/>
		</xsl:for-each>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="modules">
		<xsl:apply-templates select="module"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="module">
		<xsl:text>&#09;{"</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>"     , SCTK_CONFIG_META_TYPE_PARAM , </xsl:text>
		<xsl:value-of select="concat('sctk_config_get_offset_of_member(struct sctk_config_modules',../name,',',name,')')"/>
		<xsl:text>  , sizeof(struct sctk_config_module_</xsl:text>
		<xsl:value-of select="type"/>
		<xsl:text>) , "sctk_config_module_</xsl:text>
		<xsl:value-of select="type"/>
		<xsl:text>" , sctk_config_module_init_</xsl:text>
		<xsl:value-of select="type"/>
		<xsl:text>},&#10;</xsl:text>
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

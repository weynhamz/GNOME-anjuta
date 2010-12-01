/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-project-private.h
 *
 * Copyright (C) 2009  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _AM_PROJECT_PRIVATE_H_
#define _AM_PROJECT_PRIVATE_H_

#include "am-project.h"
#include "command-queue.h"

G_BEGIN_DECLS

typedef enum {
	AM_PROPERTY_IN_CONFIGURE = 1 << 0,
	AM_PROPERTY_IN_MAKEFILE = 1 << 1
} AmpPropertyFlag;


struct _AmpProperty {
	AnjutaProjectProperty base;
	gint token_type;
	gint position;
	AmpPropertyFlag flags;
	AnjutaToken *token;
	const gchar *suffix;
};

struct _AmpProject {
	AnjutaProjectNode base;

	/* GFile corresponding to root configure */
	GFile *configure;
	AnjutaTokenFile *configure_file;
	AnjutaToken *configure_token;

	/* File monitor */
	GFileMonitor *monitor;
	
	AnjutaToken			*ac_init;
	AnjutaToken			*args;

	/* Project file list */
	GList	   *files;

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	GHashTable		*configs;		/* Config file from configure_file */
	
	/* Keep list style */
	AnjutaTokenStyle *ac_space_list;
	AnjutaTokenStyle *am_space_list;
	AnjutaTokenStyle *arg_list;

	/* Command queue */
	PmCommandQueue *queue;
};

typedef struct _AmpNodeInfo AmpNodeInfo;

struct _AmpNodeInfo {
	AnjutaProjectNodeInfo base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};


#define ANJUTA_TYPE_AM_MODULE_NODE			(anjuta_am_module_node_get_type ())
#define ANJUTA_AM_MODULE_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_MODULE_NODE, AnjutaAmModuleNode))

GType anjuta_am_module_node_get_type (void) G_GNUC_CONST;

struct _AnjutaAmModuleNode {
	AnjutaProjectNode base;
	AnjutaToken *module;
};


#define ANJUTA_TYPE_AM_PACKAGE_NODE			(anjuta_am_package_node_get_type ())
#define ANJUTA_AM_PACKAGE_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_PACKAGE_NODE, AnjutaAmPackageNode))

GType anjuta_am_package_node_get_type (void) G_GNUC_CONST;

struct _AnjutaAmPackageNode {
	AnjutaProjectNode base;
	gchar *version;
	AnjutaToken *token;
};


#define ANJUTA_TYPE_AM_GROUP_NODE			(anjuta_am_group_node_get_type ())
#define ANJUTA_AM_GROUP_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_GROUP_NODE, AnjutaAmGroupNode))

GType anjuta_am_group_node_get_type (void) G_GNUC_CONST;

typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} AmpGroupTokenCategory;

struct _AnjutaAmGroupNode {
	AnjutaProjectNode base;
	gboolean dist_only;										/* TRUE if the group is distributed but not built */
	GFile *makefile;												/* GFile corresponding to group makefile */
	AnjutaTokenFile *tfile;										/* Corresponding Makefile */
	GList *tokens[AM_GROUP_TOKEN_LAST];			/* List of token used by this group */
	AnjutaToken *make_token;
	GHashTable *variables;
	GFileMonitor *monitor;									/* File monitor */
};



#define ANJUTA_TYPE_AM_TARGET_NODE			(anjuta_am_target_node_get_type ())
#define ANJUTA_AM_TARGET_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_TARGET_NODE, AnjutaAmTargetNode))

GType anjuta_am_target_node_get_type (void) G_GNUC_CONST;

typedef enum {
	AM_TARGET_TOKEN_TARGET,
	AM_TARGET_TOKEN_SOURCES,
	AM_TARGET_TOKEN_LAST
} AmpTargetTokenCategory;

struct _AnjutaAmTargetNode {
	AnjutaProjectNode base;
	gchar *install;
	gint flags;
	GList* tokens;	
};



#define ANJUTA_TYPE_AM_SOURCE_NODE			(anjuta_am_source_node_get_type ())
#define ANJUTA_AM_SOURCE_NODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_SOURCE_NODE, AnjutaAmSourceNode))

GType anjuta_am_source_node_get_type (void) G_GNUC_CONST;

struct _AnjutaAmSourceNode {
	AnjutaProjectNode base;
	AnjutaToken* token;	
};



G_END_DECLS

#endif /* _AM_PROJECT_PRIVATE_H_ */

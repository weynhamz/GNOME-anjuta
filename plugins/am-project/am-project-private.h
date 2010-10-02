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

G_BEGIN_DECLS

struct _AmpProperty {
	AnjutaProjectProperty base;
	gint token_type;
	gint position;
	AnjutaToken *token;
};

struct _AmpProject {
	GObject         parent;

	/* uri of the project; this can be a full uri, even though we
	 * can only work with native local files */
	AnjutaProjectNode			*root;

	/* project data */
	AnjutaTokenFile		*configure_file;		/* configure.in file */
	AnjutaToken			*configure_token;
	
	//AmpProperty			*property;
	GList				*properties;
	AnjutaToken			*ac_init;
	AnjutaToken			*args;
	
	AnjutaAmGroupNode              *root_node;         	/* tree containing project data;
								 * each GNode's data is a
								 * AmpNode, and the root of
								 * the tree is the root group. */

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	GHashTable		*files;
	GHashTable		*configs;		/* Config file from configure_file */
	
	GHashTable	*modules;
	
	/* project files monitors */
	GHashTable         *monitors;

	/* Keep list style */
	AnjutaTokenStyle *ac_space_list;
	AnjutaTokenStyle *am_space_list;
	AnjutaTokenStyle *arg_list;
};

typedef struct _AmpNodeInfo AmpNodeInfo;

struct _AmpNodeInfo {
	AnjutaProjectNodeInfo base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};

#define ANJUTA_TYPE_AM_ROOT_NODE				(anjuta_am_root_node_get_type ())
#define ANJUTA_AM_ROOT_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_ROOT_NODE, AnjutaAmRootNode))

GType anjuta_am_root_node_get_type (void) G_GNUC_CONST;

struct _AnjutaAmRootNode {
	AnjutaProjectNode base;
	AnjutaTokenFile *configure_file;					/* Corresponding configure file */
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
	GObject *project;											/* Project used by file monitor */
};



#define ANJUTA_TYPE_AM_TARGET_NODE			(anjuta_am_target_node_get_type ())
#define ANJUTA_AM_TARGET_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_TARGET_NODE, AnjutaAmTargetNode))

GType anjuta_am_target_node_get_type (void) G_GNUC_CONST;

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

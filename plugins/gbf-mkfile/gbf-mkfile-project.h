/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf-mkfile-project.h
 *
 * Copyright (C) 2005  Eric Greveson
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
 *
 * Author: Eric Greveson
 * Based on the Autotools GBF backend (libgbf-am) by
 *   JP Rosevear
 *   Dave Camp
 *   Naba Kumar
 *   Gustavo Giráldez
 */

#ifndef _GBF_MKFILE_PROJECT_H_
#define _GBF_MKFILE_PROJECT_H_

#include <glib-object.h>
#include <libanjuta/gbf-project.h>
#include "gbf-mkfile-config.h"

G_BEGIN_DECLS

#define GBF_TYPE_MKFILE_PROJECT		(gbf_mkfile_project_get_type (NULL))
#define GBF_MKFILE_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GBF_TYPE_MKFILE_PROJECT, GbfMkfileProject))
#define GBF_MKFILE_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GBF_TYPE_MKFILE_PROJECT, GbfMkfileProjectClass))
#define GBF_IS_MKFILE_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GBF_TYPE_MKFILE_PROJECT))
#define GBF_IS_MKFILE_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GBF_TYPE_MKFILE_PROJECT))

typedef struct _GbfMkfileProject        GbfMkfileProject;
typedef struct _GbfMkfileProjectClass   GbfMkfileProjectClass;
typedef struct _GbfMkfileBuildCallback  GbfMkfileBuildCallback;
typedef struct _GbfMkfileNode           GbfMkfileNode;

typedef enum {
	GBF_MKFILE_NODE_GROUP,
	GBF_MKFILE_NODE_TARGET,
	GBF_MKFILE_NODE_SOURCE
} GbfMkfileNodeType;
	
struct _GbfMkfileNode {
	GbfMkfileNodeType       type;
	gchar                  *id;        /* unique id among nodes of the same type */
	gchar                  *name;      /* user visible string */
	GbfMkfileConfigMapping *config;
	gchar                  *uri;       /* groups: path to Makefile.am;
					       targets: NULL;
					       sources: file uri */
	gchar                  *detail;    /* groups: NULL;
					       targets: target type;
					       sources: NULL or target dependency for built sources */
};

struct _GbfMkfileProject {
	GbfProject          parent;

	/* uri of the project; this can be a full uri, even though we
	 * can only work with native local files */
	gchar              *project_root_uri;

	/* project data */
	gchar                  *project_file;      /* configure.in uri */
	GbfMkfileConfigMapping *project_config;    /* project configuration
						    * (i.e. from configure.in) */
	GNode                  *root_node;         /* tree containing project data;
	                        * each GNode's data is a
						    * GbfAmNode, and the root of
						    * the tree is the root group. */

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable         *groups;
	GHashTable         *targets;
	GHashTable         *sources;
	
	/* project files monitors */
	GHashTable         *monitors;

	/* operations queue */
	GQueue             *queue_ops;
	guint               queue_handler_tag;

	/* build callbacks */
	GList              *callbacks;

	/* build config */
	gchar	 	   *make_command;
	gchar 		   *configure_command;
	gchar 		   *autogen_command;
	gchar 		   *install_prefix;
};

struct _GbfMkfileProjectClass {
	GbfProjectClass parent_class;
};

/* convenient shortcut macro the get the GbfMkfileNode from a GNode */
#define GBF_MKFILE_NODE(g_node)  ((g_node) != NULL ? (GbfMkfileNode *)((g_node)->data) : NULL)

GType         gbf_mkfile_project_get_type (GTypeModule *module);
GbfProject   *gbf_mkfile_project_new      (void);

/* FIXME: The config infrastructure should probably be made part of GbfProject
 * so that other backend implementations could use them directly and we don't
 * have to create separate configuration widgets. But then different back end
 * implementations could have significantly different config management
 * warranting separate implementations.
 */
/* These functions returns a copy of the config. It should be free with
 * gbf_mkfile_config_value_free() when no longer required
 */
GbfMkfileConfigMapping *gbf_mkfile_project_get_config (GbfMkfileProject *project, GError **error);
GbfMkfileConfigMapping *gbf_mkfile_project_get_group_config (GbfMkfileProject *project, const gchar *group_id, GError **error);
GbfMkfileConfigMapping *gbf_mkfile_project_get_target_config (GbfMkfileProject *project, const gchar *target_id, GError **error);

void gbf_mkfile_project_set_config (GbfMkfileProject *project, GbfMkfileConfigMapping *new_config, GError **error);
void gbf_mkfile_project_set_group_config (GbfMkfileProject *project, const gchar *group_id, GbfMkfileConfigMapping *new_config, GError **error);
void gbf_mkfile_project_set_target_config (GbfMkfileProject *project, const gchar *target_id, GbfMkfileConfigMapping *new_config, GError **error);

G_END_DECLS

#endif /* _GBF_MKFILE_PROJECT_H_ */

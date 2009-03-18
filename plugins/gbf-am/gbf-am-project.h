/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf-am-project.h
 *
 * Copyright (C) 2000  JP Rosevear
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: JP Rosevear
 */

#ifndef _GBF_AM_PROJECT_H_
#define _GBF_AM_PROJECT_H_

#include <glib-object.h>
#include <libanjuta/gbf-project.h>
#include <libanjuta/anjuta-plugin.h>
#include "gbf-am-config.h"

G_BEGIN_DECLS

#define GBF_TYPE_AM_PROJECT		(gbf_am_project_get_type (NULL))
#define GBF_AM_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GBF_TYPE_AM_PROJECT, GbfAmProject))
#define GBF_AM_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GBF_TYPE_AM_PROJECT, GbfAmProjectClass))
#define GBF_IS_AM_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GBF_TYPE_AM_PROJECT))
#define GBF_IS_AM_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GBF_TYPE_AM_PROJECT))

typedef struct _GbfAmProject        GbfAmProject;
typedef struct _GbfAmProjectClass   GbfAmProjectClass;
typedef struct _GbfAmBuildCallback  GbfAmBuildCallback;
typedef struct _GbfAmNode           GbfAmNode;

typedef enum {
	GBF_AM_NODE_GROUP,
	GBF_AM_NODE_TARGET,
	GBF_AM_NODE_SOURCE
} GbfAmNodeType;
	
struct _GbfAmNode {
	GbfAmNodeType       type;
	gchar              *id;        /* unique id among nodes of the same type */
	gchar              *name;      /* user visible string */
	GbfAmConfigMapping *config;
	gchar              *uri;       /* groups: path to Makefile.am;
					  targets: NULL;
					  sources: file uri */
	gchar              *detail;    /* groups: NULL;
					  targets: target type;
					  sources: NULL or target dependency for built sources */
};

struct _GbfAmProject {
	GbfProject          parent;

	/* uri of the project; this can be a full uri, even though we
	 * can only work with native local files */
	gchar              *project_root_uri;

	/* project data */
	gchar              *project_file;      /* configure.in uri */
	GbfAmConfigMapping *project_config;    /* project configuration
						* (i.e. from configure.in) */
	GNode              *root_node;         /* tree containing project data;
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

struct _GbfAmProjectClass {
	GbfProjectClass parent_class;
};

/* convenient shortcut macro the get the GbfAmNode from a GNode */
#define GBF_AM_NODE(g_node)  ((g_node) != NULL ? (GbfAmNode *)((g_node)->data) : NULL)

GType         gbf_am_project_get_type (GTypeModule *plugin);
GbfProject   *gbf_am_project_new      (void);

/* FIXME: The config infrastructure should probably be made part of GbfProject
 * so that other backend implementations could use them directly and we don't
 * have to create separate configuration widgets. But then different back end
 * implementations could have significantly different config management
 * warranting separate implementations.
 */
/* These functions returns a copy of the config. It should be free with
 * gbf_am_config_value_free() when no longer required
 */
GbfAmConfigMapping *gbf_am_project_get_config (GbfAmProject *project, GError **error);
GbfAmConfigMapping *gbf_am_project_get_group_config (GbfAmProject *project, const gchar *group_id, GError **error);
GbfAmConfigMapping *gbf_am_project_get_target_config (GbfAmProject *project, const gchar *target_id, GError **error);

void gbf_am_project_set_config (GbfAmProject *project, GbfAmConfigMapping *new_config, GError **error);
void gbf_am_project_set_group_config (GbfAmProject *project, const gchar *group_id, GbfAmConfigMapping *new_config, GError **error);
void gbf_am_project_set_target_config (GbfAmProject *project, const gchar *target_id, GbfAmConfigMapping *new_config, GError **error);

G_END_DECLS

#endif /* _GBF_AM_PROJECT_H_ */

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* project.h
 *
 * Copyright (C) 2010  Sébastien Granjoux
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
 */

#ifndef _PROJECT_H_
#define _PROJECT_H_

#include <glib.h>
#include <glib-object.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-project.h>
#include <libanjuta/interfaces/ianjuta-project.h>

#include "project-model.h"
#include "tree-data.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_PM_PROJECT             (anjuta_pm_project_get_type ())
#define ANJUTA_PM_PROJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PM_PROJECT, AnjutaPmProject))
#define ANJUTA_PM_PROJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PM_PROJECT, AnjutaPmProjectClass))
#define ANJUTA_IS_PM_PROJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PM_PROJECT))
#define ANJUTA_IS_PM_PROJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PM_PROJECT))
#define ANJUTA_PM_PROJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PM_PROJECT, AnjutaPmProjectClass))

typedef struct _AnjutaPmProjectClass AnjutaPmProjectClass;
//typedef struct _AnjutaPmProject AnjutaPmProject;

struct _AnjutaPmProjectClass
{
	GObjectClass parent_class;

	void (*updated) (GError *error);
	void (*loaded) (GError *error);
};

struct _AnjutaPmProject
{
	GObject parent_instance;

	AnjutaPlugin *plugin;
	
	IAnjutaProject *project;
	GbfProjectModel *model;

	AnjutaProjectNode *root;
	
	guint incomplete_node;
	
	/* project properties dialog */
	GtkWidget *properties_dialog;
	
	/* Thread queue */
	GQueue *job_queue;
	GAsyncQueue *work_queue;
	GAsyncQueue *done_queue;
	GThread *worker;
	guint idle_func;
	gboolean stopping;
	guint busy;
};

GType anjuta_pm_project_get_type (void) G_GNUC_CONST;

AnjutaPmProject* anjuta_pm_project_new (AnjutaPlugin *plugin);
void anjuta_pm_project_free (AnjutaPmProject* project);

gboolean anjuta_pm_project_load (AnjutaPmProject *project, GFile *file, GError **error);
gboolean anjuta_pm_project_unload (AnjutaPmProject *project, GError **error);
gboolean anjuta_pm_project_refresh (AnjutaPmProject *project, GError **error);

gint anjuta_pm_project_get_capabilities (AnjutaPmProject *project);
GList *anjuta_pm_project_get_node_info (AnjutaPmProject *project);

GList *anjuta_pm_project_get_packages (AnjutaPmProject *project);

AnjutaProjectNode *anjuta_pm_project_add_group (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, GError **error);
AnjutaProjectNode *anjuta_pm_project_add_target (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, AnjutaProjectNodeType type, GError **error);
AnjutaProjectNode *anjuta_pm_project_add_source (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, GError **error);
AnjutaProjectNode *anjuta_pm_project_get_root (AnjutaPmProject *project);
gboolean anjuta_pm_project_remove (AnjutaPmProject *project, AnjutaProjectNode *node, GError **error);
gboolean anjuta_pm_project_remove_data (AnjutaPmProject *project, GbfTreeData *data, GError **error);

gboolean anjuta_pm_project_is_open (AnjutaPmProject *project);

IAnjutaProject *anjuta_pm_project_get_project (AnjutaPmProject *project);
GbfProjectModel *anjuta_pm_project_get_model (AnjutaPmProject *project);

AnjutaProjectNode *anjuta_pm_project_get_node (AnjutaPmProject *project, GbfTreeData *data);
AnjutaProjectNode *anjuta_pm_project_get_node_from_file (AnjutaPmProject *project, AnjutaProjectNodeType type, GFile *file);

gboolean anjuta_pm_project_show_properties_dialog (AnjutaPmProject *project, GbfTreeData *data);

G_END_DECLS

#endif /* _PROJECT_H_ */

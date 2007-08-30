/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _PROJECT_MANAGER_PLUGIN_H_
#define _PROJECT_MANAGER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include <gbf/gbf-project.h>
#include <gbf/gbf-project-model.h>
#include <gbf/gbf-project-view.h>

extern GType project_manager_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_PROJECT_MANAGER         (project_manager_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_PROJECT_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_PROJECT_MANAGER, ProjectManagerPlugin))
#define ANJUTA_PLUGIN_PROJECT_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_PROJECT_MANAGER, ProjectManagerPluginClass))
#define ANJUTA_IS_PLUGIN_PROJECT_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_PROJECT_MANAGER))
#define ANJUTA_IS_PLUGIN_PROJECT_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_PROJECT_MANAGER))
#define ANJUTA_PLUGIN_PROJECT_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_PROJECT_MANAGER, ProjectManagerPluginClass))

typedef struct _ProjectManagerPlugin ProjectManagerPlugin;
typedef struct _ProjectManagerPluginClass ProjectManagerPluginClass;

struct _ProjectManagerPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	GbfProject *project;
	GtkWidget *view;
	GbfProjectModel *model;
	GtkWidget *scrolledwindow;
	
	GtkActionGroup *pm_action_group;
	GtkActionGroup *popup_action_group;
	gint merge_id;
	
	gint fm_watch_id;
	gint editor_watch_id;
	
	gchar *fm_current_uri;
	gchar *current_editor_uri;
	gchar *project_root_uri;
	gchar *project_uri;
	
	/* Update state recording */
	GList *pre_update_sources;
	GList *pre_update_targets;
	GList *pre_update_groups;
	
	/* Session flag */
	gboolean session_by_me;
};

struct _ProjectManagerPluginClass{
	AnjutaPluginClass parent_class;
};

#endif

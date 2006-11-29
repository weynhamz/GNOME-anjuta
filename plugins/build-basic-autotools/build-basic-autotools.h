/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    build-basic-autotools.h
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __BUILD_BASIC_AUTOTOOLS_H__
#define __BUILD_BASIC_AUTOTOOLS_H__

#include <libanjuta/anjuta-plugin.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-build-basic-autotools-plugin.glade"

extern GType basic_autotools_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_BASIC_AUTOTOOLS         (basic_autotools_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_BASIC_AUTOTOOLS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_BASIC_AUTOTOOLS, BasicAutotoolsPlugin))
#define ANJUTA_PLUGIN_BASIC_AUTOTOOLS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_BASIC_AUTOTOOLS, BasicAutotoolsPluginClass))
#define ANJUTA_IS_PLUGIN_BASIC_AUTOTOOLS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_BASIC_AUTOTOOLS))
#define ANJUTA_IS_PLUGIN_BASIC_AUTOTOOLS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_BASIC_AUTOTOOLS))
#define ANJUTA_PLUGIN_BASIC_AUTOTOOLS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_BASIC_AUTOTOOLS, BasicAutotoolsPluginClass))

typedef struct _BasicAutotoolsPlugin BasicAutotoolsPlugin;
typedef struct _BasicAutotoolsPluginClass BasicAutotoolsPluginClass;

struct _BasicAutotoolsPlugin{
	AnjutaPlugin parent;
	
	/* Build contexts pool */
	GList *contexts_pool;

	/* Editors in which indicators have been updated */
	GHashTable *indicators_updated_editors;
	
	/* Watch IDs */
	gint fm_watch_id;
	gint pm_watch_id;
	gint project_watch_id;
	gint editor_watch_id;
	
	/* Watched values */
	gchar *fm_current_filename;
	gchar *pm_current_filename;
	gchar *project_root_dir;
	gchar *current_editor_filename;
	IAnjutaEditor *current_editor;
	
	/* UI */
	gint build_merge_id;
	GtkActionGroup *build_action_group;
	GtkActionGroup *build_popup_action_group;
	
	/* Build parameters */
	gchar *configure_args;
	
	/* Execution parameters */
	gchar *program_args;
	gboolean run_in_terminal;
};

struct _BasicAutotoolsPluginClass{
	AnjutaPluginClass parent_class;
};

#endif

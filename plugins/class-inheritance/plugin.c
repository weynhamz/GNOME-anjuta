/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <glib/gi18n.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/anjuta-debug.h>
#include <gnome.h>
#include "plugin.h"
#include "class-inherit.h"

#define ICON_FILE "anjuta-class-inheritance-plugin-48.png"

static gpointer parent_class;

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaClassInheritance *ci_plugin;
	const gchar *root_uri;

	ci_plugin = ANJUTA_PLUGIN_CLASS_INHERITANCE (plugin);
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = anjuta_util_get_local_path_from_uri (root_uri);
		if (root_dir)
		{	
			ci_plugin->top_dir = g_strdup(root_dir);
		}
		else
			ci_plugin->top_dir = NULL;
		g_free (root_dir);
	}
	else
		ci_plugin->top_dir = NULL;
	
	/* let's update the graph */
	class_inheritance_update_graph (ci_plugin);
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaClassInheritance *ci_plugin;
	ci_plugin = ANJUTA_PLUGIN_CLASS_INHERITANCE (plugin);
	
	/* clean up the canvas */
	class_inheritance_clean_canvas (ci_plugin);

	/* destroy and re-initialize the hashtable */
	class_inheritance_gtree_clear (ci_plugin);
	
	if (ci_plugin->top_dir)
		g_free(ci_plugin->top_dir);
	ci_plugin->top_dir = NULL;
}

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON (PACKAGE_PIXMAPS_DIR"/"ICON_FILE,
				   "class-inheritance-plugin-icon");
	END_REGISTER_ICON;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaClassInheritance *class_inheritance;
	static gboolean initialized = FALSE;
	
	DEBUG_PRINT ("%s", "AnjutaClassInheritance: Activating plugin ...");
	
	register_stock_icons (plugin);
	
	class_inheritance = ANJUTA_PLUGIN_CLASS_INHERITANCE (plugin);
	
	class_inheritance_base_gui_init (class_inheritance);
	
	anjuta_shell_add_widget (plugin->shell, class_inheritance->widget,
							 "AnjutaClassInheritance", _("Inheritance Graph"),
							 "class-inheritance-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	class_inheritance->top_dir = NULL;
	
	/* set up project directory watch */
	class_inheritance->root_watch_id = anjuta_plugin_add_watch (plugin,
									IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
									project_root_added,
									project_root_removed, NULL);

	initialized	= TRUE;	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("%s", "AnjutaClassInheritance: Dectivating plugin ...");
	AnjutaClassInheritance* class_inheritance;
	class_inheritance = ANJUTA_PLUGIN_CLASS_INHERITANCE (plugin);

	/* clean up the canvas [e.g. destroys it's elements */
	class_inheritance_clean_canvas (class_inheritance);
	
	/* destroy the nodestatus gtree */
	if (class_inheritance->expansion_node_list) {
		g_tree_destroy (class_inheritance->expansion_node_list);
		class_inheritance->expansion_node_list = NULL;
	}
	
	/* Container holds the last ref to this widget so it will be destroyed as
	 * soon as removed. No need to separately destroy it. */
	/* In most cases, only toplevel widgets (windows) require explicit 
	 * destruction, because when you destroy a toplevel its children will 
	 * be destroyed as well. */
	anjuta_shell_remove_widget (plugin->shell,
								class_inheritance->widget,
								NULL);

	/* Remove watches */
	anjuta_plugin_remove_watch (plugin,
								class_inheritance->root_watch_id,
								TRUE);
	return TRUE;
}

static void
class_inheritance_finalize (GObject *obj)
{
	AnjutaClassInheritance *ci_plugin;
	ci_plugin = ANJUTA_PLUGIN_CLASS_INHERITANCE (obj);
	
	if (ci_plugin->top_dir)
		g_free (ci_plugin->top_dir);
	
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
class_inheritance_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
class_inheritance_instance_init (GObject *obj)
{
	AnjutaClassInheritance *plugin = ANJUTA_PLUGIN_CLASS_INHERITANCE (obj);

	plugin->uiid = 0;

	plugin->widget = NULL;
	plugin->graph = NULL;
	plugin->gvc = NULL;
	plugin->expansion_node_list = NULL;
}

static void
class_inheritance_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->finalize = class_inheritance_finalize;
	klass->dispose = class_inheritance_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (AnjutaClassInheritance, class_inheritance);
ANJUTA_SIMPLE_PLUGIN (AnjutaClassInheritance, class_inheritance);

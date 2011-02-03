/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* project.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "project.h"
#include "project-marshal.h"
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include "project-model.h"
#include "project-view.h"
#include "dialogs.h"


/* Types
 *---------------------------------------------------------------------------*/

/* Signal
 *---------------------------------------------------------------------------*/

enum
{
	LOADED,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

static void
on_pm_project_load_incomplete (AnjutaProjectNode *node, AnjutaPmProject *project)
{
	gint state = anjuta_project_node_get_state (node);

	/* Get capabilities for all existing node */
	project->node_capabilities |= state;
	
	if ((state & ANJUTA_PROJECT_INCOMPLETE) && !(state & ANJUTA_PROJECT_LOADING))
	{
		//g_message ("incomplete node %s", anjuta_project_node_get_name (node));
		project->incomplete_node++;
		anjuta_project_node_set_state (node, ANJUTA_PROJECT_LOADING);
		//g_message ("load incomplete %p", node);
		ianjuta_project_load_node (project->project, node, NULL);
	}
}

static gboolean
pm_command_load_complete (AnjutaPmProject *project, AnjutaProjectNode *node, GError *error)
{
	gboolean complete = FALSE;

	//g_message ("pm_command_load_complete %p", node);
	if (error == NULL)
	{
		if (project->root == node)
		{
			project->incomplete_node = 0;
		}
		
		// Check for incompletely loaded object and load them
		if (anjuta_project_node_get_state (node) & ANJUTA_PROJECT_INCOMPLETE)
		{
			project->incomplete_node--;
			//g_message ("remaining node %d", project->incomplete_node);
		}
		anjuta_project_node_clear_state (node, ANJUTA_PROJECT_LOADING | ANJUTA_PROJECT_INCOMPLETE);
		anjuta_project_node_foreach (node, G_POST_ORDER, (AnjutaProjectNodeForeachFunc)on_pm_project_load_incomplete, project);

		complete = !project->loaded && (project->incomplete_node == 0);
		if (complete) project->loaded = TRUE;
	}

	//g_message ("pm_command_load_complete %d is loaded %d", complete, ianjuta_project_is_loaded (project->project, NULL));
	g_signal_emit (G_OBJECT (project), signals[LOADED], 0, node, complete, error);

	
	return TRUE;
}

static void
on_file_changed (IAnjutaProject *sender, AnjutaProjectNode *node, AnjutaPmProject *project)
{
	ianjuta_project_load_node (project->project, node, NULL);
}

static void
on_node_loaded (IAnjutaProject *sender, AnjutaProjectNode *node, GError *error, AnjutaPmProject *project)
{
	pm_command_load_complete (project, node, error);
}

static void
on_node_changed (IAnjutaProject *sender, AnjutaProjectNode *node, GError *error, AnjutaPmProject *project)
{
	ianjuta_project_save_node (project->project, node, NULL);
}

/* Public functions
 *---------------------------------------------------------------------------*/


gboolean 
anjuta_pm_project_load (AnjutaPmProject *project, GFile *file, GError **error)
{
	AnjutaPluginManager *plugin_manager;
	GList *desc;
	IAnjutaProjectBackend *backend;
	gint found = 0;
	GValue value = {0, };
	
	g_return_val_if_fail (file != NULL, FALSE);

	anjuta_pm_project_unload (project, NULL);
	
	DEBUG_PRINT ("loading gbf backend…\n");
	plugin_manager = anjuta_shell_get_plugin_manager (project->plugin->shell, NULL);

	if (!anjuta_plugin_manager_is_active_plugin (plugin_manager, "IAnjutaProjectBackend"))
	{
		GList *descs = NULL;
		
		descs = anjuta_plugin_manager_query (plugin_manager,
											 "Anjuta Plugin",
											 "Interfaces",
											 "IAnjutaProjectBackend",
											 NULL);
		backend = NULL;
		for (desc = g_list_first (descs); desc != NULL; desc = g_list_next (desc)) {
			AnjutaPluginDescription *backend_desc;
			gchar *location = NULL;
			IAnjutaProjectBackend *plugin;
			gint backend_val;
				
			backend_desc = (AnjutaPluginDescription *)desc->data;
			anjuta_plugin_description_get_string (backend_desc, "Anjuta Plugin", "Location", &location);
			plugin = (IAnjutaProjectBackend *)anjuta_plugin_manager_get_plugin_by_id (plugin_manager, location);
			g_free (location);

			backend_val = ianjuta_project_backend_probe (plugin, file, NULL);
			if (backend_val > found)
			{
				/* Backend found */;
				backend = plugin;
				found = backend_val;
			}
		}
		g_list_free (descs);
	}
	else
	{
		/* A backend is already loaded, use it */
		backend = IANJUTA_PROJECT_BACKEND (anjuta_shell_get_object (project->plugin->shell,
                                        "IAnjutaProjectBackend", NULL));

		g_object_ref (backend);
	}
	
	if (!backend)
	{
		/* FIXME: Set err */
		g_warning ("no backend available for this project\n");
		
		return FALSE;
	}
	
	DEBUG_PRINT ("%s", "Creating new gbf project\n");
	project->project = ianjuta_project_backend_new_project (backend, file, NULL);
	if (!project->project)
	{
		/* FIXME: Set err */
		g_warning ("project creation failed\n");
		
		return FALSE;
	}

	g_signal_connect (G_OBJECT (project->project),
						"file-changed",
						G_CALLBACK (on_file_changed),
						project);
	g_signal_connect (G_OBJECT (project->project),
						"node-loaded",
						G_CALLBACK (on_node_loaded),
						project);
	g_signal_connect (G_OBJECT (project->project),
						"node-changed",
						G_CALLBACK (on_node_changed),
						project);
	
	project->root = ianjuta_project_get_root (project->project, NULL);

	g_value_init (&value, G_TYPE_OBJECT);
	g_value_set_object (&value, project->project);
	anjuta_shell_add_value (project->plugin->shell,
	                        IANJUTA_PROJECT_MANAGER_CURRENT_PROJECT,
	                        &value, NULL);
	g_value_unset(&value);
	
	ianjuta_project_load_node (project->project, project->root, NULL);
	
	return TRUE;
}

gboolean 
anjuta_pm_project_unload (AnjutaPmProject *project, GError **error)
{
	/* Remove value from Anjuta shell */
	anjuta_shell_remove_value (project->plugin->shell,
	                           IANJUTA_PROJECT_MANAGER_CURRENT_PROJECT,
	                           NULL);
	
	if (project->project) g_object_unref (project->project);
	project->project = NULL;
	project->loaded = FALSE;
	project->node_capabilities = 0;
	
	/* Remove project properties dialogs */
	if (project->properties_dialog != NULL) gtk_widget_destroy (project->properties_dialog);
	project->properties_dialog = NULL;
	
	return TRUE;
}

gboolean
anjuta_pm_project_refresh (AnjutaPmProject *project, GError **error)
{
	ianjuta_project_load_node (project->project, project->root, NULL);	

	return TRUE;
}

gint
anjuta_pm_project_get_capabilities (AnjutaPmProject *project)
{
	gint caps = 0;

	if (project->project != NULL)
	{
		const GList *item;

		for (item = anjuta_pm_project_get_node_info (project); item != NULL; item = g_list_next (item))
		{
			AnjutaProjectNodeInfo *info = (AnjutaProjectNodeInfo *)item->data;

			switch (info->type & ANJUTA_PROJECT_TYPE_MASK)
			{
			case ANJUTA_PROJECT_GROUP:
				caps |= ANJUTA_PROJECT_CAN_ADD_GROUP;
				break;
			case ANJUTA_PROJECT_TARGET:
				caps |= ANJUTA_PROJECT_CAN_ADD_TARGET;
				break;
			case ANJUTA_PROJECT_SOURCE:
				caps |= ANJUTA_PROJECT_CAN_ADD_SOURCE;
				break;
			case ANJUTA_PROJECT_MODULE:
				caps |= ANJUTA_PROJECT_CAN_ADD_MODULE;
				break;
			case ANJUTA_PROJECT_PACKAGE:
				caps |= ANJUTA_PROJECT_CAN_ADD_PACKAGE;
				break;
			default:
				break;
			}
		}
	}

	/* Make sure that at least one node can do it */
	caps &= project->node_capabilities;
	
	return caps;
}

const GList *
anjuta_pm_project_get_node_info (AnjutaPmProject *project)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_get_node_info (project->project, NULL);
}

AnjutaProjectNode *
anjuta_pm_project_get_root (AnjutaPmProject *project)
{
	return project->root;
}

GList *
anjuta_pm_project_get_packages (AnjutaPmProject *project)
{
	AnjutaProjectNode *module;
	GHashTable *all;
	GList *packages;

	g_return_val_if_fail (project != NULL, NULL);
	
	all = g_hash_table_new (g_str_hash, g_str_equal);
	
	for (module = anjuta_project_node_first_child (project->root); module != NULL; module = anjuta_project_node_next_sibling (module))
	{
		if (anjuta_project_node_get_node_type(module) == ANJUTA_PROJECT_MODULE)
		{
			AnjutaProjectNode *package;

			for (package = anjuta_project_node_first_child (module); package != NULL; package = anjuta_project_node_next_sibling (package))
			{
				if (anjuta_project_node_get_node_type (package) == ANJUTA_PROJECT_PACKAGE)
				{
					g_hash_table_replace (all, (gpointer)anjuta_project_node_get_name (package), NULL);
				}
			}
		}
	}
	
	packages = g_hash_table_get_keys (all);
	g_hash_table_destroy (all);
	
	return packages;
}

AnjutaProjectNode *
anjuta_pm_project_add_group (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;
	
	g_return_val_if_fail (project->project != NULL, NULL);
	
	node = ianjuta_project_add_node_before (project->project, parent, sibling, ANJUTA_PROJECT_GROUP, NULL, name, NULL);

	return node;
}

AnjutaProjectNode *
anjuta_pm_project_add_target (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, AnjutaProjectNodeType type, GError **error)
{
	AnjutaProjectNode *node;
	
	g_return_val_if_fail (project->project != NULL, NULL);

	node = ianjuta_project_add_node_before (project->project, parent, sibling, ANJUTA_PROJECT_TARGET | type, NULL, name, NULL);

	return node;
}

AnjutaProjectNode *
anjuta_pm_project_add_source (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;
	gchar *scheme;
	GFile *file = NULL;

	g_return_val_if_fail (project->project != NULL, NULL);

	scheme = g_uri_parse_scheme (name);
	if (scheme != NULL)
	{
		g_free (scheme);
		file = g_file_new_for_uri (name);
	}
	
	node = ianjuta_project_add_node_before (project->project, parent, sibling, ANJUTA_PROJECT_SOURCE, file, file == NULL ? name : NULL, NULL);

	return node;
}

gboolean
anjuta_pm_project_remove (AnjutaPmProject *project, AnjutaProjectNode *node, GError **error)
{
	ianjuta_project_remove_node (project->project, node, NULL);

	return TRUE;
}

gboolean
anjuta_pm_project_is_open (AnjutaPmProject *project)
{
	return project->project != NULL;
}

IAnjutaProject *
anjuta_pm_project_get_project (AnjutaPmProject *project)
{
	return project->project;
}

static gboolean
find_module (AnjutaProjectNode *node, gpointer data)
{
	gboolean found = FALSE;
	
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_MODULE)
	{
		const gchar *name = anjuta_project_node_get_name (node);

		found = g_strcmp0 (name, (const gchar *)data) == 0;
	}

	return found;
}

AnjutaProjectNode *
anjuta_pm_project_get_module (AnjutaPmProject *project, const gchar *name)
{
	AnjutaProjectNode *root;
	AnjutaProjectNode *module;

	root = ianjuta_project_get_root (project->project, NULL);

	module = anjuta_project_node_children_traverse (root, find_module, (gpointer)name);

	return module;
}

/* Implement GObject
 *---------------------------------------------------------------------------*/

G_DEFINE_TYPE (AnjutaPmProject, anjuta_pm_project, G_TYPE_OBJECT);

static void
anjuta_pm_project_init (AnjutaPmProject *project)
{
	project->plugin = NULL;
	project->loaded = FALSE;

	project->properties_dialog = NULL;
}

static void
anjuta_pm_project_finalize (GObject *object)
{
	G_OBJECT_CLASS (anjuta_pm_project_parent_class)->finalize (object);
}

static void
anjuta_pm_project_class_init (AnjutaPmProjectClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_pm_project_finalize;


 	/*Change both signal to use marshal_VOID__POINTER_BOXED
	adding a AnjutaProjectNode pointer corresponding to the
	 loaded node => done
	 Such marshal doesn't exist as glib marshal, so look in the
	 symbol db plugin how to add new marshal => done
	 ToDo :
	 This new argument can be used in the plugin object in
	 order to add corresponding shortcut when the project
	 is loaded and a new node is loaded.
	 The plugin should probably get the GFile from the
	 AnjutaProjectNode object and then use a function
	 in project-view.c to create the corresponding shortcut*/
	
	signals[LOADED] = g_signal_new ("loaded",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (AnjutaPmProjectClass, loaded),
		NULL, NULL,
		pm_cclosure_marshal_VOID__POINTER_BOOLEAN_BOXED,
		G_TYPE_NONE,
		3,
	    G_TYPE_POINTER,
	    G_TYPE_BOOLEAN,
		G_TYPE_ERROR);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AnjutaPmProject*
anjuta_pm_project_new (AnjutaPlugin *plugin)
{
	AnjutaPmProject *project;

	project = g_object_new (ANJUTA_TYPE_PM_PROJECT, NULL);
	project->plugin = plugin;

	return project;
}

void
anjuta_pm_project_free (AnjutaPmProject* project)
{
	g_object_unref (project);
}

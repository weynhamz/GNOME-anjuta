/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  plugin.c
 *  Copyright (C) 2005 Massimo Cora'
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-vcs.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include <plugins/project-wizard/autogen.h>

#include "plugin.h"
/* #include "class_gen.h" */

#include "window.h"

#define ICON_FILE "class_logo.xpm"

static gpointer parent_class;

static void
project_root_added (AnjutaPlugin *plugin,
                    G_GNUC_UNUSED const gchar *name,
                    const GValue *value,
					G_GNUC_UNUSED gpointer user_data)
{
	AnjutaClassGenPlugin *cg_plugin;
	const gchar *root_uri;

	cg_plugin = ANJUTA_PLUGIN_CLASS_GEN (plugin);
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = anjuta_util_get_local_path_from_uri (root_uri);
		if (root_dir)
			cg_plugin->top_dir = g_strdup(root_dir);
		else
			cg_plugin->top_dir = NULL;
		g_free (root_dir);
	}
	else
		cg_plugin->top_dir = NULL;
}

static void
project_root_removed (AnjutaPlugin *plugin,
                      G_GNUC_UNUSED const gchar *name,
					  G_GNUC_UNUSED gpointer user_data)
{
	AnjutaClassGenPlugin *cg_plugin;
	cg_plugin = ANJUTA_PLUGIN_CLASS_GEN (plugin);
	
	if (cg_plugin->top_dir)
		g_free(cg_plugin->top_dir);
	cg_plugin->top_dir = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaClassGenPlugin *cg_plugin;
	
	DEBUG_PRINT ("%s", "AnjutaClassGenPlugin: Activating ClassGen plugin...");
	cg_plugin = ANJUTA_PLUGIN_CLASS_GEN (plugin);
	cg_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	g_return_val_if_fail (cg_plugin->prefs != NULL, FALSE);
	
	cg_plugin->top_dir = NULL;

	/* Check if autogen is present */
	if(!npw_check_autogen())
	{
		anjuta_util_dialog_error(
			NULL,
			_("Could not find autogen version 5, please install the "
			  "autogen package. You can get it from "
			  "http://autogen.sourceforge.net"));

		return FALSE;
	}

	/* set up project directory watch */
	cg_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
									IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
									project_root_added,
									project_root_removed, NULL);

	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaClassGenPlugin *cg_plugin;
	cg_plugin = ANJUTA_PLUGIN_CLASS_GEN (plugin);
	DEBUG_PRINT ("%s", "AnjutaClassGenPlugin: Deactivating ClassGen plugin ...");
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, cg_plugin->root_watch_id, TRUE);
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	AnjutaClassGenPlugin *cg_plugin;
	cg_plugin = ANJUTA_PLUGIN_CLASS_GEN (obj);
	g_free (cg_plugin->top_dir);

	if(cg_plugin->window != NULL)
		g_object_unref(G_OBJECT(cg_plugin->window));
	if(cg_plugin->generator != NULL)
		g_object_unref(G_OBJECT(cg_plugin->generator));

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
class_gen_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static void
class_gen_plugin_instance_init (GObject *obj)
{
	AnjutaClassGenPlugin *plugin = ANJUTA_PLUGIN_CLASS_GEN (obj);
	plugin->root_watch_id = 0;
	plugin->top_dir = NULL;
	plugin->window = NULL;
	plugin->generator = NULL;
}

static gboolean
cg_plugin_add_to_project (AnjutaClassGenPlugin *plugin,
                          const gchar *header_file,
                          const gchar *source_file,
                          gchar **new_header_file,
                          gchar **new_source_file)
{
	IAnjutaProjectManager *manager;
	GList *filenames;
	GList *added_files;
	GList *node;
	gchar *dirname;
	gchar *curdir;
	gboolean result;

	manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
	                                      IAnjutaProjectManager, NULL);
       
	if (manager == NULL) return FALSE;

	curdir = g_get_current_dir ();

	filenames = NULL;
	filenames = g_list_append (filenames, g_path_get_basename (header_file));
	filenames = g_list_append (filenames, g_path_get_basename (source_file));
	dirname = g_path_get_dirname (source_file);

	if (dirname != NULL && strcmp (dirname, ".") != 0)
	{
		added_files = ianjuta_project_manager_add_sources (manager, filenames,
														   dirname, NULL);
	}
	else
	{
		added_files = ianjuta_project_manager_add_sources (manager, filenames,
														   curdir, NULL);
	}

	if (g_list_length (added_files) != 2)
	{
		for (node = added_files; node != NULL; node = g_list_next (node))
			g_free (node->data);

		result = FALSE;
	}
	else
	{
		GFile *file;

		file = g_file_new_for_uri ((gchar *)added_files->data);
		*new_header_file = g_file_get_path(file);
		g_object_unref (file);
		file = g_file_new_for_uri ((gchar *)g_list_next (added_files)->data);
		*new_source_file = g_file_get_path(file);
		g_object_unref (file);
		
		result = TRUE;
	}

	g_free (curdir);
	g_free (dirname);
	g_list_foreach (added_files, (GFunc)g_free, NULL);
	g_list_free (added_files);
	g_list_free (filenames);

	return result;
}

static void
cg_plugin_add_to_repository (AnjutaClassGenPlugin *plugin,
                             const gchar *header_file,
                             const gchar *source_file)
{
	IAnjutaVcs *vcs;
	vcs = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
	                                  IAnjutaVcs, NULL);

	if(vcs != NULL)
	{
		GList* files = NULL;
		files = g_list_append (files, g_file_new_for_path (header_file));
		files = g_list_append (files, g_file_new_for_path (source_file));		
		ianjuta_vcs_add (vcs, files, NULL, NULL);
		g_list_foreach (files, (GFunc) g_object_unref, NULL);
		g_list_free (files);
	}
}

static void
cg_plugin_generator_error_cb (G_GNUC_UNUSED CgGenerator *generator,
                              GError *error,
                              gpointer user_data)
{
	AnjutaClassGenPlugin *plugin;
	plugin = (AnjutaClassGenPlugin *) user_data;

	anjuta_util_dialog_error (
		GTK_WINDOW (cg_window_get_dialog (plugin->window)),
		_("Failed to execute autogen: %s"), error->message);

	gtk_widget_set_sensitive (
		GTK_WIDGET (cg_window_get_dialog (plugin->window)), TRUE);
}

static gboolean
cg_plugin_load (AnjutaClassGenPlugin *plugin,
                CgGenerator *generator,
                const gchar *file,
                GError **error)
{
	IAnjutaDocumentManager *docman;
	IAnjutaEditor *editor;
	gchar *name;
	gchar *contents;
	gboolean result;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);	

	if(g_file_get_contents(file, &contents, NULL, error) == FALSE)
		return FALSE;

	name = g_path_get_basename (file);

	result = FALSE;
	/* The content argument seems not to work */
	editor = ianjuta_document_manager_add_buffer (docman, name, "", error);

	if(editor != NULL)
	{
		ianjuta_editor_append(editor, contents, -1, error);
		if(!error || *error == NULL)
			result = TRUE;
	}
	
	g_free(contents);
	g_free(name);
	
	return result;
}

static void
cg_plugin_generator_created_cb (CgGenerator *generator,
                                gpointer user_data)
{
	AnjutaClassGenPlugin *plugin;
	const gchar *header_file;
	const gchar *source_file;
	IAnjutaFileLoader *loader;

	plugin = (AnjutaClassGenPlugin *) user_data;
	header_file = cg_generator_get_header_destination (generator);
	source_file = cg_generator_get_source_destination (generator);

	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
	                                     IAnjutaFileLoader, NULL);

	if (cg_window_get_add_to_repository (plugin->window))
	{
		cg_plugin_add_to_repository (plugin, header_file, source_file);
	}

	if (cg_window_get_add_to_project (plugin->window))
	{
		GFile* header = g_file_new_for_path (header_file);
		GFile* source = g_file_new_for_path (source_file);
		ianjuta_file_loader_load (loader, header, FALSE, NULL);
		ianjuta_file_loader_load (loader, source, FALSE, NULL);
		g_object_unref (header);
		g_object_unref (source);
	}
	else
	{
		/* We do not just use ianjuta_file_leader_load here to ensure that
		 * the new documents are flagged as changed and no path is
		 * already set. */
		cg_plugin_load (plugin, generator, header_file, NULL);
		cg_plugin_load (plugin, generator, source_file, NULL);
	}

	g_object_unref (G_OBJECT (plugin->window));
	plugin->window = NULL;
}

static void
cg_plugin_window_response_cb (G_GNUC_UNUSED GtkDialog *dialog,
                              gint response_id,
                              gpointer user_data)
{
	AnjutaClassGenPlugin *plugin;
	IAnjutaProjectManager *manager;
	NPWValueHeap *values;
	NPWValue *value;
	GError *error;
	gchar *name;

	gchar *header_file;
	gchar *source_file;
	gboolean result;

	plugin = (AnjutaClassGenPlugin *) user_data;
	error = NULL;

	if (response_id == GTK_RESPONSE_ACCEPT)
	{
    	if (cg_window_get_add_to_project (plugin->window))
	    {
		    result = cg_plugin_add_to_project (
		    	plugin, cg_window_get_header_file (plugin->window),
				cg_window_get_source_file (plugin->window),
				&header_file, &source_file);
		}
		else
		{
		    header_file = g_build_filename (g_get_tmp_dir (),
		    	cg_window_get_header_file (plugin->window), NULL);
		    source_file = g_build_filename (g_get_tmp_dir (),
		    	cg_window_get_source_file (plugin->window), NULL);

		    result = TRUE;
		}
    	
		if(result == TRUE)
		{
			values = cg_window_create_value_heap (plugin->window);

			manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
			                                      IAnjutaProjectManager, NULL);

			if (manager != NULL && plugin->top_dir != NULL)
			{
				/* Use basename of the project's root URI as project name. */
				name = g_path_get_basename (plugin->top_dir);
				value = npw_value_heap_find_value (values, "ProjectName");
				npw_value_heap_set_value (values, value, name, NPW_VALID_VALUE);
				g_free (name);
			}
			else
			{
				name = g_path_get_basename (cg_window_get_source_file(
				                            plugin->window));
				value = npw_value_heap_find_value (values, "ProjectName");
				npw_value_heap_set_value (values, value, name, NPW_VALID_VALUE);
				g_free (name);
			}

    		plugin->generator = cg_generator_new (
				cg_window_get_header_template(plugin->window),
				cg_window_get_source_template(plugin->window),
				header_file,
				source_file);

			if (cg_generator_run (plugin->generator, values, &error) == FALSE)
			{
				anjuta_util_dialog_error (
					GTK_WINDOW (cg_window_get_dialog (plugin->window)),
					_("Failed to execute autogen: %s"), error->message);

				g_object_unref (G_OBJECT (plugin->generator));
				g_error_free (error);
			}
			else
			{
				g_signal_connect (G_OBJECT (plugin->generator), "error",
				                  G_CALLBACK (cg_plugin_generator_error_cb),
				                  plugin);

				g_signal_connect (G_OBJECT (plugin->generator), "created",
				                 G_CALLBACK (cg_plugin_generator_created_cb),
								 plugin);

				gtk_widget_set_sensitive (
					GTK_WIDGET (cg_window_get_dialog (plugin->window)), FALSE);
			}

			npw_value_heap_free (values);
			g_free (header_file);
			g_free (source_file);
		}
	}
	else
	{
		g_object_unref (G_OBJECT (plugin->window));
		plugin->window = NULL;
	}
}

static void
iwizard_activate (IAnjutaWizard *wiz, G_GNUC_UNUSED GError **err)
{
	/* IAnjutaProjectManager *pm; */
	AnjutaClassGenPlugin *cg_plugin;
	gchar *user_name;
	gchar *user_email;
	IAnjutaProjectManagerCapabilities caps =
		IANJUTA_PROJECT_MANAGER_CAN_ADD_NONE;
	
	cg_plugin = ANJUTA_PLUGIN_CLASS_GEN (wiz);

	if (cg_plugin->window != NULL)
		g_object_unref (G_OBJECT (cg_plugin->window));

	cg_plugin->window = cg_window_new ();

	user_name = anjuta_preferences_get (cg_plugin->prefs,
	                                    "anjuta.user.name");
	user_email = anjuta_preferences_get (cg_plugin->prefs,
	                                     "anjuta.user.email");
	
	if (user_name != NULL)
		cg_window_set_author (cg_plugin->window, user_name);

	if (user_email != NULL)
		cg_window_set_email (cg_plugin->window, user_email);

	g_free(user_name);
	g_free(user_email);

	/* Check whether we have a loaded project and it can add sources */
	if (cg_plugin->top_dir)
	{
		IAnjutaProjectManager *manager =
			anjuta_shell_get_interface (ANJUTA_PLUGIN (wiz)->shell,
										IAnjutaProjectManager, NULL);
       if (manager)
			caps = ianjuta_project_manager_get_capabilities (manager, NULL);
	}

	if((caps & IANJUTA_PROJECT_MANAGER_CAN_ADD_SOURCE) == FALSE)
	{
		cg_window_set_add_to_project (cg_plugin->window, FALSE);
		cg_window_enable_add_to_project (cg_plugin->window, FALSE);
	}

	/* TODO: Check whether the project is in version control, and enable
	 * "add to repository" button respectively. */

	g_signal_connect (G_OBJECT (cg_window_get_dialog(cg_plugin->window)),
	                 "response", G_CALLBACK (cg_plugin_window_response_cb),
					 cg_plugin);

	gtk_widget_show (GTK_WIDGET (cg_window_get_dialog (cg_plugin->window)));
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (AnjutaClassGenPlugin, class_gen_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaClassGenPlugin, class_gen_plugin);

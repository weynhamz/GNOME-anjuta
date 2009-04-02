/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar, 2005 Johannes Schmid

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

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include <libanjuta/interfaces/ianjuta-vcs.h>
#include <libanjuta/gbf-project.h>
#include <libanjuta/anjuta-async-notify.h>

#include "plugin.h"
#include "project-import-dialog.h"

#define ICON_FILE "anjuta-project-import-plugin-48.png"

#define AM_PROJECT_FILE PACKAGE_DATA_DIR"/project/terminal/project.anjuta"
#define MKFILE_PROJECT_FILE PACKAGE_DATA_DIR"/project/mkfile/project.anjuta"

static gpointer parent_class;

static gboolean
project_import_generate_file (AnjutaProjectImportPlugin* import_plugin, ProjectImportDialog *import_dialog,
                              GFile *project_file)
{
	/* Of course we could do some more intelligent stuff here
	and check which plugins are really needed but for now we just
	take a default project file. */
	
	GFile* source_file;
	
	if (!strcmp (import_plugin->backend_id, "automake"))
		source_file = g_file_new_for_path (AM_PROJECT_FILE);
	else if (!strcmp (import_plugin->backend_id, "make"))
		source_file = g_file_new_for_path (MKFILE_PROJECT_FILE);
	else
	{
		/* We shouldn't get here, unless someone has upgraded their GBF */
		/* but not Anjuta.                                              */
		
		/* show the dialog since it may be hidden */
		gtk_widget_show (GTK_WIDGET (import_dialog));

		anjuta_util_dialog_error (GTK_WINDOW (import_dialog),
		                          _("Generation of project file failed. Cannot "
		                            "find an appropriate project template to "
		                            "use. Please make sure your version of "
		                            "Anjuta is up to date."));
		return FALSE;
	}
	
	GError* error = NULL;

	if (!g_file_copy (source_file, project_file, 
			G_FILE_COPY_NONE,
			NULL,
			NULL,
			NULL,
			&error))
	{
		if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_EXISTS)
		{
			gchar *prjfile = g_file_get_parse_name (project_file);
			if (anjuta_util_dialog_boolean_question (GTK_WINDOW (ANJUTA_PLUGIN(import_plugin)->shell),
					_("A file named \"%s\" already exists. "
					  "Do you want to replace it?"), prjfile))
			{
				g_error_free (error);
				error = NULL;
				g_file_copy (source_file, project_file,
						G_FILE_COPY_OVERWRITE,
						NULL,
						NULL,
						NULL,
						&error);
			}
			g_free (prjfile);
		}
	}

	g_object_unref (source_file);

	if (!error)
	{
		time_t ctime = time(NULL);
		GFileInfo* file_info = g_file_info_new();
		g_file_info_set_attribute_uint64(file_info, 
				"time::modified",
				ctime);
		g_file_info_set_attribute_uint64(file_info, 
				"time::created",
				ctime);
		g_file_info_set_attribute_uint64(file_info, 
				"time::access",
				ctime);
		g_file_set_attributes_from_info (project_file, 
				file_info,
				G_FILE_QUERY_INFO_NONE,
				NULL, NULL);

		g_object_unref (G_OBJECT(file_info));;
	}
	else
	{
		gchar *prjfile;

		prjfile = g_file_get_parse_name (project_file);
		
		/* show the dialog since it may be hidden */
		gtk_widget_show (GTK_WIDGET (import_dialog));
		
		anjuta_util_dialog_error (GTK_WINDOW (import_dialog),
				_("A file named \"%s\" cannot be written: %s.  "
				  "Check if you have write access to the project directory."),
				  prjfile, error->message);

		g_free (prjfile);
		
		return FALSE;

	}

	return TRUE;
}

static gboolean
project_import_import_project (AnjutaProjectImportPlugin *import_plugin, ProjectImportDialog *import_dialog,
                               GFile *source_dir)
{
	AnjutaPluginManager *plugin_manager;
	GList *descs = NULL;
	GList *desc;
	AnjutaPluginDescription *backend;
	gchar *name, *project_file_name;
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN(import_plugin)->shell, NULL);
	descs = anjuta_plugin_manager_query (plugin_manager,
										 "Anjuta Plugin",
										 "Interfaces",
										 "IAnjutaProjectBackend",
										 NULL);	
	for (desc = g_list_first (descs); desc != NULL; desc = g_list_next (desc)) {
		IAnjutaProjectBackend *plugin;
		gchar *location = NULL;
		GbfProject* proj;	
		
		backend = (AnjutaPluginDescription *)desc->data;
		anjuta_plugin_description_get_string (backend, "Anjuta Plugin", "Location", &location);
		plugin = (IAnjutaProjectBackend *)anjuta_plugin_manager_get_plugin_by_id (plugin_manager, location);
		g_free (location);

		/* Probe the backend to find out if the project directory is OK */
		/* If probe() returns TRUE then we have a valid backend */
		
		proj= ianjuta_project_backend_new_project (plugin, NULL);
		if (proj)
		{
			gchar *path;

			path = g_file_get_path (source_dir);
			if (gbf_project_probe (proj, path, NULL))
			{
				/* This is a valid backend for this root directory */
				/* FIXME: Possibility of more than one valid backend? */
				break;
			}
			g_object_unref (proj);
			g_free (path);
		}
		plugin = NULL;
		backend = NULL;
	}
	g_list_free (descs);
	
	if (!backend)
	{
		gchar *path = project_import_dialog_get_name (import_dialog);

		/* show the dialog since it may be hidden */
		gtk_widget_show (GTK_WIDGET (import_dialog));

		anjuta_util_dialog_error (GTK_WINDOW (import_dialog),
		                          _("Could not find a valid project backend for the "
		                            "directory given (%s). Please select a different "
		                            "directory, or try upgrading to a newer version of "
		                            "Anjuta."), path);

		g_free (path);

		return FALSE;
	}

	if (!anjuta_plugin_description_get_string (backend, "Project", "Supported-Project-Types", &import_plugin->backend_id))
	{
		import_plugin->backend_id = g_strdup ("unknown");
	}

	name = project_import_dialog_get_name (import_dialog);
	project_file_name = g_strconcat (name, ".", "anjuta", NULL);
	GFile *project_file = g_file_get_child (source_dir, project_file_name);

	g_free (name);
	g_free (project_file_name);
	
	IAnjutaFileLoader* loader;
	
	if (!project_import_generate_file (import_plugin, import_dialog, project_file))
	{
		g_object_unref (project_file);
		return FALSE;
	}
	
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (import_plugin)->shell,
	                                     IAnjutaFileLoader, NULL);
	if (!loader)
	{
		g_warning("No IAnjutaFileLoader interface! Cannot open project file!");
		g_object_unref (project_file);
		return TRUE;
	}
	
	ianjuta_file_loader_load (loader, project_file, FALSE, NULL);

	g_object_unref (project_file);
	
	return TRUE;
}

typedef struct
{
	AnjutaProjectImportPlugin *import_plugin;
	ProjectImportDialog *import_dialog;
	GFile *checkout_dir;
} CheckoutData;

static void
checkout_finished (AnjutaAsyncNotify *async_notify, gpointer user_data)
{
	CheckoutData *ch = (CheckoutData *)user_data;
	GError *err;

	err = NULL;
	anjuta_async_notify_get_error (async_notify, &err);
	if (err)
	{
		gchar *vcs_uri;

		/* show the dialog since it's hidden */
		gtk_widget_show (GTK_WIDGET (ch->import_dialog));
		
		vcs_uri = project_import_dialog_get_vcs_uri (ch->import_dialog);
		anjuta_util_dialog_error (GTK_WINDOW (ch->import_dialog),
		                          _("Couldn't checkout the supplied uri "
		                          "\"%s\", the error returned was: \"%s\""),
		                          vcs_uri, err->message);

		g_free (vcs_uri);
		g_error_free (err);

		goto cleanup;
	}

	project_import_import_project (ch->import_plugin, ch->import_dialog, ch->checkout_dir);

cleanup:
	g_object_unref (ch->checkout_dir);
	g_slice_free (CheckoutData, ch);
}

static void
project_import_checkout_project (AnjutaProjectImportPlugin *import_plugin,
                                 ProjectImportDialog *import_dialog)
{
	CheckoutData *ch_data;
	AnjutaAsyncNotify *async_notify;
	gchar *vcs_uri, *plugin_id, *name;
	GFile *dest_dir, *checkout_dir;
	AnjutaPluginManager *plugin_manager;
	IAnjutaVcs *ivcs;
	GError *err;

	name = project_import_dialog_get_name (import_dialog);
	dest_dir = project_import_dialog_get_dest_dir (import_dialog);
	checkout_dir = g_file_get_child (dest_dir, name);

	g_object_unref (dest_dir);
	g_free (name);
	
	ch_data = g_slice_new (CheckoutData);
	ch_data->import_plugin = import_plugin;
	ch_data->import_dialog = import_dialog;
	ch_data->checkout_dir = checkout_dir;
	
	async_notify = anjuta_async_notify_new ();
	g_signal_connect (async_notify, "finished", G_CALLBACK (checkout_finished),
	                  ch_data);

	vcs_uri = project_import_dialog_get_vcs_uri (import_dialog);
	plugin_id = project_import_dialog_get_vcs_id (import_dialog);

	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (import_plugin)->shell, NULL);
	ivcs = IANJUTA_VCS (anjuta_plugin_manager_get_plugin_by_id (plugin_manager, plugin_id));

	err = NULL;
	ianjuta_vcs_checkout (ivcs, vcs_uri, checkout_dir, NULL, async_notify, &err);
	if (err)
	{
		anjuta_util_dialog_error (GTK_WINDOW (import_dialog),
		                          _("Couldn't checkout the supplied uri "
		                            "\"%s\", the error returned was: \"%s\""),
		                          vcs_uri, err->message);

		g_error_free (err);
		
		goto cleanup;
	}

	/* hide the import dialog */
	gtk_widget_hide (GTK_WIDGET (import_dialog));

cleanup:
	g_free (vcs_uri);
	g_free (plugin_id);
}

static void
import_dialog_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	AnjutaProjectImportPlugin *import_plugin = (AnjutaProjectImportPlugin *)user_data;
	ProjectImportDialog *import_dialog = PROJECT_IMPORT_DIALOG (dialog);
	
	switch (response_id)
	{
		case GTK_RESPONSE_ACCEPT:
		{
			GFile *source_dir;

			source_dir = project_import_dialog_get_source_dir (import_dialog);
			if (source_dir)
			{
				if (project_import_import_project (import_plugin, import_dialog, source_dir))
					gtk_widget_destroy (GTK_WIDGET (import_dialog));
				
				g_object_unref (source_dir);
			}
			else
				project_import_checkout_project (import_plugin, import_dialog);

			break;

		}
		case GTK_RESPONSE_REJECT:
			gtk_widget_destroy (GTK_WIDGET (dialog));
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProjectImportPlugin *iplugin;
	
	DEBUG_PRINT ("%s", "AnjutaProjectImportPlugin: Activating Project Import Plugin ...");
	
	iplugin = ANJUTA_PLUGIN_PROJECT_IMPORT (plugin);
	iplugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProjectImportPlugin *iplugin;
	iplugin = ANJUTA_PLUGIN_PROJECT_IMPORT (plugin);
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
	AnjutaProjectImportPlugin *import_plugin = (AnjutaProjectImportPlugin *)obj;

	if (import_plugin->backend_id)
		g_free (import_plugin->backend_id);
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
project_import_plugin_instance_init (GObject *obj)
{
	// AnjutaFileWizardPlugin *plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (obj);
}

static void
project_import_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	AnjutaProjectImportPlugin* plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (wiz);
	AnjutaPluginManager *plugin_manager;
	ProjectImportDialog* pi;

	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell, NULL);

	pi = project_import_dialog_new(plugin_manager, NULL, NULL);
	g_signal_connect (pi, "response", G_CALLBACK (import_dialog_response), plugin);

	gtk_widget_show (GTK_WIDGET (pi));
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **err)
{
	gchar *ext, *project_name;
	GFile *dir;
	ProjectImportDialog* pi;
	AnjutaProjectImportPlugin* plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (ifile);
	gchar* uri = g_file_get_uri (file);
	AnjutaPluginManager *plugin_manager;
	
	g_return_if_fail (uri != NULL && strlen (uri) > 0);
	
	dir = g_file_get_parent (file);
	project_name = g_path_get_basename (uri);
	ext = strrchr (project_name, '.');
	if (ext)
		*ext = '\0';

	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	pi = project_import_dialog_new (plugin_manager, project_name, dir);
	g_signal_connect (pi, "response", G_CALLBACK (import_dialog_response), plugin);

	gtk_widget_show (GTK_WIDGET (pi));
	
	g_object_unref (dir);
	g_free (project_name);
	g_free (uri);
}

static GFile*
ifile_get_file (IAnjutaFile *file, GError **err)
{
	g_warning ("Unsupported operation");
	return NULL;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

ANJUTA_PLUGIN_BEGIN (AnjutaProjectImportPlugin, project_import_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaProjectImportPlugin, project_import_plugin);

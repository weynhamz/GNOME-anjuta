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
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include <libanjuta/interfaces/ianjuta-vcs.h>
#include <libanjuta/anjuta-async-notify.h>

#include "plugin.h"
#include "project-import-dialog.h"

#define ICON_FILE "anjuta-project-import-plugin-48.png"

#define AM_PROJECT_FILE PACKAGE_DATA_DIR"/templates/terminal/project.anjuta"
#define MKFILE_PROJECT_FILE PACKAGE_DATA_DIR"/templates/mkfile/project.anjuta"
#define DIRECTORY_PROJECT_FILE PACKAGE_DATA_DIR"/templates/directory/project.anjuta"

static gpointer parent_class;

static gboolean
project_import_generate_file (AnjutaPluginDescription *backend, ProjectImportDialog *import_dialog,
                              GFile *project_file)
{
	/* Of course we could do some more intelligent stuff here
	 * and check which plugins are really needed */
	
	GFile* source_file = NULL;
	gchar *backend_id = NULL;
	GError* error = NULL;

	if (!anjuta_plugin_description_get_string (backend, "Project", "Supported-Project-Types", &backend_id))
	{
		if (!strcmp (backend_id, "automake"))
			source_file = g_file_new_for_path (AM_PROJECT_FILE);
		else if (!strcmp (backend_id, "make"))
			source_file = g_file_new_for_path (MKFILE_PROJECT_FILE);
		else if (!strcmp (backend_id, "directory"))
			source_file = g_file_new_for_path (DIRECTORY_PROJECT_FILE);
	}
	g_free (backend_id);
	
	if (source_file != NULL)
	{
		/* Use a default project file */
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
				if (anjuta_util_dialog_boolean_question (GTK_WINDOW (import_dialog), FALSE,
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
	}
	else
	{
		/* For unknown project backend we use the directory project file and
		 * replace the backend plugin with the right one */

		gchar *content;
		gsize length;

		source_file = g_file_new_for_path (DIRECTORY_PROJECT_FILE);
		if (g_file_load_contents (source_file, NULL, &content, &length, NULL, &error))
		{
			GString *buffer;
			const gchar *pos;
			const gchar *plugin;
			const gchar *end_plugin;
			gssize len;

			buffer = g_string_new_len (content, length);
			pos = buffer->str;
			len = buffer->len;
			for (;;)
			{
				plugin = g_strstr_len (pos, len, "<plugin ");
				if (plugin == NULL) break;
				
				end_plugin = g_strstr_len (plugin, len - (plugin - pos), "</plugin>");
				if (end_plugin == NULL) break;
				
				if (g_strstr_len (plugin, end_plugin - plugin, "\"IAnjutaProjectBackend\"") != NULL) break;

				pos = end_plugin + 9;
				len -= (end_plugin + 9 - pos);
			}

			if ((plugin == NULL) || (end_plugin == NULL))
			{
				g_set_error (&error, ianjuta_project_backend_error_quark(),0, "Unable to find backend plugin");
			}
			else
			{
				/* Replace directory backend with right one */
				GString *str;
				GFileOutputStream *stream;
				gchar *name = NULL;
				gchar *plugin_id = NULL;

				anjuta_plugin_description_get_string (backend, "Anjuta Plugin", "Name", &name);
				anjuta_plugin_description_get_string (backend, "Anjuta Plugin", "Location", &plugin_id);

				str = g_string_new (NULL);
				g_string_printf (str, "<plugin name= \"%s\"\n"
				                 "            mandatory=\"yes\">\n"
				                 "         <require group=\"Anjuta Plugin\"\n"
				                 "                  attribute=\"Location\"\n"
				                 "                  value=\"%s\"/>\n"
				                 "         <require group=\"Anjuta Plugin\"\n"
				                 "                  attribute=\"Interfaces\"\n"
				                 "                  value=\"IAnjutaProjectBackend\"/>\n"
				                 "    ", name, plugin_id);
					
				g_string_erase (buffer, plugin - buffer->str, end_plugin - plugin);
				g_string_insert_len (buffer, plugin - buffer->str, str->str, str->len);

				g_string_free (str, TRUE);

				stream = g_file_create (project_file, G_FILE_CREATE_NONE, NULL, &error);
				if (stream == NULL && error->domain == G_IO_ERROR && error->code == G_IO_ERROR_EXISTS)
				{
					gchar *prjfile = g_file_get_parse_name (project_file);
					if (anjuta_util_dialog_boolean_question (GTK_WINDOW (import_dialog), FALSE,
							_("A file named \"%s\" already exists. "
							  "Do you want to replace it?"), prjfile))
					{
						g_error_free (error);
						error = NULL;
						stream = g_file_replace (project_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error);
					}
					g_free (prjfile);
				}
					
				if (stream != NULL)
				{
					gsize written;
					
					g_output_stream_write_all (G_OUTPUT_STREAM (stream), buffer->str, buffer->len, &written, NULL, &error);
					g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, NULL);
				}
			}

			g_string_free (buffer, TRUE);
			g_free (content);
		}
	}
	g_object_unref (source_file);

	if (error)
	{
		gchar *prjfile;

		prjfile = g_file_get_parse_name (project_file);
		
		/* show the dialog since it may be hidden */
		gtk_widget_show (GTK_WIDGET (import_dialog));
		
		anjuta_util_dialog_error (GTK_WINDOW (import_dialog),
				_("A file named \"%s\" cannot be written: %s. "
				  "Check if you have write access to the project directory."),
				  prjfile, error->message);
		g_free (prjfile);
		g_error_free (error);
		
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

	/* Search for all valid project backend */
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN(import_plugin)->shell, NULL);
	descs = anjuta_plugin_manager_query (plugin_manager,
										 "Anjuta Plugin",
										 "Interfaces",
										 "IAnjutaProjectBackend",
										 NULL);	
	for (desc = g_list_first (descs); desc != NULL;) {
		IAnjutaProjectBackend *plugin;
		gchar *location = NULL;
		GList *next;
		
		backend = (AnjutaPluginDescription *)desc->data;
		anjuta_plugin_description_get_string (backend, "Anjuta Plugin", "Location", &location);
		plugin = (IAnjutaProjectBackend *)anjuta_plugin_manager_get_plugin_by_id (plugin_manager, location);
		g_free (location);

		next = g_list_next (desc);
		
		/* Probe the project directory to find if the backend can handle it */
		if (ianjuta_project_backend_probe (plugin, source_dir, NULL) <= 0)
		{
			/* Remove invalid backend */
			descs = g_list_delete_link (descs, desc);
		}

		desc = next;
	}

	if (descs == NULL)
	{
		backend = NULL;
	}
	else if (g_list_next (descs) == NULL)
	{
		backend =  (AnjutaPluginDescription *)descs->data;
	}
	else
	{
		/* Several backend are possible, ask the user to select one */
		gchar *path = project_import_dialog_get_name (import_dialog);
		gchar* message = g_strdup_printf (_("Please select a project backend to open %s."), path);
		
		g_free (path);
		
        backend = anjuta_plugin_manager_select (plugin_manager,
		    _("Open With"),
		    message,
		    descs);
		g_free (message);
	}
	g_list_free (descs);

	if (backend == NULL)
	{
		gchar *path = project_import_dialog_get_name (import_dialog);

		/* show the dialog since it may be hidden */
		gtk_widget_show (GTK_WIDGET (import_dialog));

		anjuta_util_dialog_error (GTK_WINDOW (import_dialog),
		                          _("Could not find a valid project backend for the "
		                            "given directory (%s). Please select a different "
		                            "directory, or try upgrading to a newer version of "
		                            "Anjuta."), path);

		g_free (path);

		return FALSE;
	}


	name = project_import_dialog_get_name (import_dialog);
	project_file_name = g_strconcat (name, ".", "anjuta", NULL);
	GFile *project_file = g_file_get_child (source_dir, project_file_name);

	g_free (name);
	g_free (project_file_name);
	
	IAnjutaFileLoader* loader;
	
	if (!project_import_generate_file (backend, import_dialog, project_file))
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
		                          _("Couldn't check out the supplied URI "
		                          "\"%s\". The error returned was: \"%s\""),
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
		                          _("Couldn't check out the supplied URI "
		                            "\"%s\". The error returned was: \"%s\""),
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
	AnjutaProjectImportPlugin* plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (ifile);
	gchar *mime_type;

	g_return_if_fail (G_IS_FILE (file));

	mime_type = anjuta_util_get_file_mime_type (file);
	if (g_strcmp0 (mime_type,"application/x-anjuta-old") == 0)
	{
		/* Automatically import old Anjuta project */
		gchar *ext, *project_name;
		GFile *dir;
		ProjectImportDialog* pi;
		AnjutaPluginManager *plugin_manager;
		
		dir = g_file_get_parent (file);
		project_name = g_file_get_basename (file);
		ext = strrchr (project_name, '.');
		if (ext)
			*ext = '\0';

		plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
		pi = project_import_dialog_new (plugin_manager, project_name, dir);
		g_signal_connect (pi, "response", G_CALLBACK (import_dialog_response), plugin);

		gtk_widget_show (GTK_WIDGET (pi));
	
		g_object_unref (dir);
		g_free (project_name);
	}
	else if (g_strcmp0 (mime_type,"inode/directory") == 0)
	{
		GFileEnumerator *dir;

		dir = g_file_enumerate_children (file,
		                                 G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
		                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
		                                 NULL,
		                                 NULL);
		if (dir)
		{
			/* Look for anjuta project file in the directory */
			GFileInfo *info;

			for (info = g_file_enumerator_next_file (dir, NULL, NULL); info != NULL; info = g_file_enumerator_next_file (dir, NULL, NULL))
			{
				gchar *mime_type = anjuta_util_get_file_info_mime_type (info);
				
				if (g_strcmp0 (mime_type, "application/x-anjuta") == 0)
				{
					/* Open the first anjuta project file */
					IAnjutaFileLoader *loader;

					loader = anjuta_shell_get_interface(ANJUTA_PLUGIN (plugin)->shell, IAnjutaFileLoader, NULL);
					if (loader != NULL)
					{
						GFile* project_file = g_file_get_child (file, g_file_info_get_name(info));
						ianjuta_file_loader_load(loader, project_file, FALSE, NULL);
						g_object_unref (project_file);
					}
					g_free (mime_type);
					g_object_unref (info);
					break;
				}
				g_free (mime_type);
				g_object_unref (info);
			}
			if (info == NULL)
			{
				/* Else import the directory */
				ProjectImportDialog* pi;
				AnjutaPluginManager *plugin_manager;
				gchar *basename;

				plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell, NULL);

				basename = g_file_get_basename (file);
				pi = project_import_dialog_new (plugin_manager, basename, file);
				g_free (basename);
				g_signal_connect (pi, "response", G_CALLBACK (import_dialog_response), plugin);

				gtk_widget_show (GTK_WIDGET (pi));
			}
			g_object_unref (dir);
		}
	}
	g_free (mime_type);
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

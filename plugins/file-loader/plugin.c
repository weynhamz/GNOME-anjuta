/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

#include <config.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/plugins.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-loader-plugin.ui"

gpointer parent_class;

static void
on_open_response_ok (GtkDialog* dialog, gint id,
					 AnjutaFileLoaderPlugin *plugin)
{
	gchar *uri;
	gchar *entry_filename = NULL;
	int i;
	GSList * list;
	int elements;

	if (id != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}
	
	list = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
	elements = g_slist_length(list);
	for (i = 0; i < elements; i++)
	{
		uri = g_strdup(g_slist_nth_data(list,i));
		if (!uri)
			return;
		ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin), uri, FALSE, NULL);
		g_free (uri);
	}

	if (entry_filename)
	{
		g_slist_remove(list, entry_filename);
		g_free(entry_filename);
	}
}

static GtkWidget*
create_file_open_dialog_gui(GtkWindow* parent, AnjutaFileLoaderPlugin* plugin)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Open file"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_OPEN,
									GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect(G_OBJECT(dialog), "response", 
					G_CALLBACK(on_open_response_ok), plugin);
	g_signal_connect_swapped (GTK_OBJECT (dialog), 
                             "response", 
                             G_CALLBACK (gtk_widget_destroy),
                             GTK_OBJECT (dialog));
	return dialog;
}

static void
on_open_activate (GtkAction *action, AnjutaFileLoaderPlugin *plugin)
{
	GtkWidget *dlg;
	
	dlg = create_file_open_dialog_gui (NULL, plugin);
	gtk_widget_show (dlg);
}

static void
on_new_activate (GtkAction *action, AnjutaFileLoaderPlugin *loader)
{
	GSList *plugin_descs = NULL;
	GObject *plugin = NULL;	
	
	plugin_descs = anjuta_plugins_query (ANJUTA_PLUGIN(loader)->shell,
										 "Anjuta Plugin",
										 "Interfaces", "IAnjutaWizard",
										 NULL);
	if (g_slist_length (plugin_descs) > 0)
	{
		plugin = anjuta_plugins_select (ANJUTA_PLUGIN(loader)->shell,
								"New",
								"Please select a wizard to create a new component.",
								plugin_descs);
		if (plugin)
			ianjuta_wizard_activate (IANJUTA_WIZARD (plugin), NULL);
	}
	else
	{
		anjuta_util_dialog_error (NULL,
		"No Wizard plugins found. Please make sure you have them installed.");
	}
	g_slist_free (plugin_descs);
}

static GtkActionEntry actions_file[] = {
  { "ActionFileNew", GTK_STOCK_NEW, N_("_New ..."), "<control>n",
	N_("New file, project and project components."),
    G_CALLBACK (on_new_activate)},
  { "ActionFileOpen", GTK_STOCK_OPEN, N_("_Open ..."), "<control>o",
	N_("Open file"), G_CALLBACK (on_open_activate)},
};

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaFileLoaderPlugin *loader_plugin;
	
	loader_plugin = (AnjutaFileLoaderPlugin*)plugin;
	
	g_message ("AnjutaFileLoaderPlugin: Activating File Loader plugin ...");
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	loader_plugin->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupLoader",
											_("File Loader"),
											actions_file,
											G_N_ELEMENTS (actions_file),
											plugin);
	loader_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaFileLoaderPlugin *loader_plugin;
	
	loader_plugin = (AnjutaFileLoaderPlugin*)plugin;
	
	g_message ("AnjutaFileLoaderPlugin: Deactivating File Loader plugin ...");
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, loader_plugin->uiid);
	anjuta_ui_remove_action_group (ui, loader_plugin->action_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
}

static void
anjuta_file_loader_plugin_instance_init (GObject *obj)
{
}

static void
anjuta_file_loader_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static GObject*
iloader_load (IAnjutaFileLoader *loader, const gchar *filename,
			  gboolean read_only, GError **err)
{
	gchar *mime_type;
	GSList *plugin_descs = NULL;
	GObject *plugin = NULL;	
	
	mime_type = gnome_vfs_get_mime_type (filename);
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	plugin_descs = anjuta_plugins_query (ANJUTA_PLUGIN(loader)->shell,
										 "Anjuta Plugin",
										 "Interfaces", "IAnjutaFile",
										 "File Loader",
										 "SupportedMimeTypes", mime_type,
										 NULL);
	
	if (g_slist_length (plugin_descs) > 1)
	{
		plugin = anjuta_plugins_select (ANJUTA_PLUGIN(loader)->shell,
								"Open With",
								"Please select a plugin to open this file.",
								plugin_descs);
	}
	else if (g_slist_length (plugin_descs) == 1)
	{
		gchar *location = NULL;
		
		AnjutaPluginDescription *desc = plugin_descs->data;
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &location);
		g_return_val_if_fail (location != NULL, NULL);
		plugin =
			anjuta_plugins_get_plugin_by_location (ANJUTA_PLUGIN(loader)->shell,
												   location);
	}
	else
	{
		anjuta_util_dialog_error (NULL,
						"No plugin capabale of opening mime type \"%s\" found",
						mime_type);
	}
	if (plugin)
		ianjuta_file_open (IANJUTA_FILE(plugin), filename, NULL);
	return plugin;
}

static void
iloader_iface_init(IAnjutaLoaderIface *iface)
{
	iface->find_plugins = NULL;
}

static void
iloader_file_iface_init(IAnjutaFileLoaderIface *iface)
{
	iface->load = iloader_load;
}

ANJUTA_PLUGIN_BEGIN (AnjutaFileLoaderPlugin, anjuta_file_loader_plugin);
ANJUTA_INTERFACE (iloader, IANJUTA_TYPE_LOADER);
ANJUTA_INTERFACE (iloader_file, IANJUTA_TYPE_FILE_LOADER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaFileLoaderPlugin, anjuta_file_loader_plugin);

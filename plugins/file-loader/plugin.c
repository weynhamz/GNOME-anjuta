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
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
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

static GtkActionEntry actions_file[] = {
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
											actions_file, 1,
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

static void
iloader_load (IAnjutaFileLoader *loader, const gchar *filename,
			  gboolean read_only, GError **err)
{
	GSList *plugin_descs = NULL;
	gchar *icon_filename;
	gchar *mime_type;
	gchar *icon_path = NULL;
	
	mime_type = gnome_vfs_get_file_mime_type (filename, NULL, FALSE);
	g_return_if_fail (mime_type != NULL);
	
	plugin_descs = anjuta_plugins_query (ANJUTA_PLUGIN(loader)->shell,
										 "Anjuta Plugin",
										 "Interfaces", "IAnjutaFile",
										 "File Loader",
										 "SupportedMimeTypes", mime_type,
										 NULL);
	
	if (g_slist_length (plugin_descs) > 1) {
		GdkPixbuf *icon_pixbuf;
		GtkTreeModel *model;
		GtkWidget *view;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
#if 0			
		model = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING,
									G_TYPE_POINTER);
		view = gtk_tree_view_new_with_model (model);
		
		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_title (column, _("Available Plugins"));
	
		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", PIXBUF_COLUMN);
	
		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (column, renderer, TRUE);
		gtk_tree_view_column_add_attribute (column, renderer, "text", PLUGIN_COLUMN);
	
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		gtk_tree_view_set_expander_column (GTK_TREE_VIEW (view), column);
	
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Rev"), renderer,
								   "text", REV_COLUMN,
								   NULL);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (fv->tree), column);

		
		AnjutaPluginDescription *desc =
			(AnjutaPluginDescription*)plugin_descs->data;
		if (anjuta_plugin_description_get_string (desc,
												  "Anjuta Plugin",
												  "Icon",
												  &icon_filename)) {
			icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
									 icon_filename, NULL);
			g_message ("Icon: %s", icon_path);
			icon_pixbuf = 
				gdk_pixbuf_new_from_file (icon_path, NULL);
			g_free (icon_path);
			if (icon_pixbuf) {
				e_splash_add_icon (splash, icon_pixbuf);
				max_icons++;
				g_object_unref (icon_pixbuf);
			}
			// while (gtk_events_pending ())
			//	gtk_main_iteration ();
		} else {
			g_warning ("Plugin does not define Icon");
		}
		g_slist_free (plugin_descs);
#endif
	}
	else if (g_slist_length (plugin_descs) == 1)
	{
		GObject *plugin = NULL;	
		gchar *location = NULL;
		
		AnjutaPluginDescription *desc = plugin_descs->data;
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &location);
		g_return_if_fail (location != NULL);
		// anjuta_util_dialog_info (NULL, "Plugin %s found", location);
		plugin =
			anjuta_plugins_get_plugin_by_location (ANJUTA_PLUGIN(loader)->shell,
												   location);
		
		ianjuta_file_open (IANJUTA_FILE(plugin), filename, NULL);
	}
	else
	{
		anjuta_util_dialog_error (NULL,
						"No plugin capabale of opening mime type \"%s\" found",
						mime_type);
	}
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

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Naba Kumar

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

#include <glib/gi18n.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "plugin.h"
#include "dnd.h"
#include "anjuta-recent-chooser-menu.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-loader-plugin.ui"

static gpointer parent_class;

static int
sort_wizards(gconstpointer wizard1, gconstpointer wizard2)
{
	gchar* name1, *name2;
	AnjutaPluginDescription* desc1 = (AnjutaPluginDescription*) wizard1;
	AnjutaPluginDescription* desc2 = (AnjutaPluginDescription*) wizard2;
	
	if ((anjuta_plugin_description_get_locale_string (desc1, "Wizard",
													  "Title", &name1) ||
			anjuta_plugin_description_get_locale_string (desc1, "Anjuta Plugin",
														 "Name", &name1)) &&
		(anjuta_plugin_description_get_locale_string (desc2, "Wizard",
													  "Title", &name2) ||
			anjuta_plugin_description_get_locale_string (desc2, "Anjuta Plugin",
														 "Name", &name2)))
	{
		return strcmp(name1, name2);
	}
	else
		return 0;
}

/* The add argument is here to remember that recent menu items should be removed
 * when the destination does not exist anymore */
static void
update_recent_file (AnjutaFileLoaderPlugin *plugin, const gchar *uri,
				 const gchar *mime, gboolean add)
{
	
	if (add)
	{
		GtkRecentData *recent_data;
	
		recent_data = g_slice_new (GtkRecentData);

		recent_data->display_name = NULL;
		recent_data->description = NULL;
		recent_data->mime_type = (gchar *)mime;
		recent_data->app_name = (gchar *) g_get_application_name ();
		recent_data->app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);
		recent_data->groups = NULL;
		recent_data->is_private = FALSE;
	
		if (!gtk_recent_manager_add_full (plugin->recent_manager, uri, recent_data))
		{
      		g_warning ("Unable to add '%s' to the list of recently used documents", uri);
		}

		g_free (recent_data->app_exec);
		g_slice_free (GtkRecentData, recent_data);
	}
	else
	{
		gtk_recent_manager_remove_item (plugin->recent_manager, uri, NULL);
	}

}

static void
launch_application_failure (AnjutaFileLoaderPlugin *plugin,
							const gchar *uri,
							const gchar *errmsg)
{
	GtkWidget *parent;
	gchar *basename;
	
	parent =
		gtk_widget_get_toplevel (GTK_WIDGET(ANJUTA_PLUGIN (plugin)->shell));
	basename = g_path_get_basename (uri);
	anjuta_util_dialog_error (GTK_WINDOW (parent),
							  _("Can not open \"%s\".\n\n%s"),
							  basename, errmsg);
	g_free (basename);
}

static GList *
get_available_plugins_for_mime (AnjutaPlugin* plugin,
							   const gchar *mime_type)
{
	AnjutaPluginManager *plugin_manager;
	GList *plugin_descs = NULL;
	
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	plugin_manager = anjuta_shell_get_plugin_manager (plugin->shell,
													  NULL);
	
	/* Check an exact match */
	plugin_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaFile",
												"File Loader",
												"SupportedMimeTypes",
												mime_type,
												NULL);
	
	/* Check for plugins supporting one supertype */
	if (plugin_descs == NULL)
	{
		GList *node;
		GList *loader_descs = NULL;	
		
		loader_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaFile",
												NULL);
		for (node = g_list_first (loader_descs); node != NULL; node = g_list_next (node))
		{
			gchar *value;
			
			if (anjuta_plugin_description_get_string ((AnjutaPluginDescription *)node->data,
													  "File Loader", "SupportedMimeTypes", &value))
			{
				gchar **split_value;
				
				split_value = g_strsplit (value, ",", -1);
				g_free (value);
				if (split_value)
				{
					gchar **mime;
					
					for (mime = split_value; *mime != NULL; mime++)
					{
						/* The following line is working on unix only where
						 * content and mime type are the same. Normally the
						 * mime type has to be converted to a content type.
						 * But it is a recent (glib 2.18) function, I think we can
						 * wait a bit to fix this */
						if (g_content_type_is_a (mime_type, *mime))
						{
							gchar *loc;
							anjuta_plugin_description_get_string ((AnjutaPluginDescription *)node->data,
													  "Anjuta Plugin", "Location", &loc);
							
							plugin_descs = g_list_prepend (plugin_descs, node->data);
							break;
						}
					}
				}
				g_strfreev (split_value);
			}
		}
		g_list_free (loader_descs);
		plugin_descs = g_list_reverse (plugin_descs);
	}
	
	return plugin_descs;
}

static void
open_with_dialog (AnjutaFileLoaderPlugin *plugin, const gchar *uri,
				  const gchar *mime_type)
{
	GList *plugin_descs, *snode;
	GList *mime_apps, *node;
	GtkWidget *menu, *menuitem;
	GAppInfo *mime_app;
	
	GtkWidget *dialog, *parent, *hbox, *label;
	GtkWidget *options;
	gchar *message;
	gchar *basename;
	AnjutaPluginManager *plugin_manager;
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell,
													  NULL);
	basename = g_path_get_basename (uri);
	message = g_strdup_printf (_("<b>Cannot open \"%s\"</b>.\n\n"
								 "There is no plugin, default action, or application "
								 "configured to handle this file type.\n"
								 "\n"
								 "Mime type: %s\n"
								 "\n"
								 "You may choose to try opening it with the following "
								 "plugins or applications."),
							   basename, mime_type);
	g_free (basename);

	parent =
		gtk_widget_get_toplevel (GTK_WIDGET(ANJUTA_PLUGIN (plugin)->shell));
	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (parent),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_OK_CANCEL, message);
	g_free (message);
	
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), hbox,
						FALSE, FALSE, 5);
	label = gtk_label_new (_("Open with:"));
	options = gtk_option_menu_new ();
	gtk_box_pack_end (GTK_BOX(hbox), options, FALSE, FALSE, 10);
	gtk_box_pack_end (GTK_BOX(hbox), label, FALSE, FALSE, 10);
	
	menu = gtk_menu_new ();
	
	/* Document manager plugin */
	menuitem = gtk_menu_item_new_with_label (_("Document Manager"));
	gtk_menu_append (menu, menuitem);
	
	/* Open with plugins menu items */
	plugin_descs = get_available_plugins_for_mime (ANJUTA_PLUGIN (plugin), mime_type);
	snode = plugin_descs;
	while (snode)
	{
		gchar *name;
		AnjutaPluginDescription *desc;
		
		desc = (AnjutaPluginDescription *)(snode->data);
		
		name = NULL;
		
		anjuta_plugin_description_get_locale_string (desc, "File Loader",
													 "Title", &name);
		
		if (!name)
		{
			anjuta_plugin_description_get_locale_string (desc, "Anjuta Plugin",
														 "Name", &name);
		}
		if (!name)
		{
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &name);
		}
		menuitem = gtk_menu_item_new_with_label (name);
		gtk_menu_append (menu, menuitem);
		g_free (name);
		snode = g_list_next (snode);
	}
	
	/* Open with application menu items */
	mime_apps = g_app_info_get_all_for_type (mime_type);
	if (mime_apps)
	{
		/* Separator */
		menuitem = gtk_menu_item_new ();
		gtk_menu_append (menu, menuitem);
	}	
	node = mime_apps;
	while (node)
	{
		mime_app = (GAppInfo *)(node->data);
		if (g_app_info_should_show (mime_app))
		{
			menuitem = gtk_menu_item_new_with_label (
					g_app_info_get_name (mime_app));
			gtk_menu_append (menu, menuitem);
		}
		node = g_list_next (node);
	}
	
	gtk_option_menu_set_menu (GTK_OPTION_MENU (options), menu);
	gtk_widget_show_all (hbox);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		gint option;
		
		option = gtk_option_menu_get_history(GTK_OPTION_MENU (options));
		if (option == 0)
		{
			IAnjutaDocumentManager *docman;
			docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												 IAnjutaDocumentManager,
												 NULL);
			if (docman)
			{
				GFile* file = g_file_new_for_uri (uri);
				ianjuta_file_open (IANJUTA_FILE (docman), file, NULL);
				g_object_unref (file);
			}
			else
			{
				g_warning ("No document manager plugin!!");
			}
		}
		else if (option < (g_list_length (plugin_descs) + 1))
		{
			AnjutaPluginDescription *desc;
			gchar *location = NULL;
			
			option--;
			desc = g_list_nth_data (plugin_descs, option);
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &location);
			g_assert (location != NULL);
			if (location != NULL)
			{
				GObject *loaded_plugin;
				
				loaded_plugin =
					anjuta_plugin_manager_get_plugin_by_id (plugin_manager,
															location);
				if (loaded_plugin)
				{
					GFile* file = g_file_new_for_uri (uri);
					ianjuta_file_open (IANJUTA_FILE (loaded_plugin), file, NULL);
					update_recent_file (plugin, uri, mime_type, TRUE);
					g_object_unref (file);
				}
				else
				{
					anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
											  "Failed to activate plugin: %s",
											  location);
				}
				g_free (location);
			}
		}
		else
		{
			GList *uris = NULL;
			GError *error = NULL;
			
			option -= (g_list_length (plugin_descs) + 2);
			mime_app = g_list_nth_data (mime_apps, option);
			uris = g_list_prepend (uris, (gpointer)uri);
			g_app_info_launch_uris(mime_app, uris, NULL, &error);
			if (error)
			{
				launch_application_failure (plugin, uri, error->message);
				g_error_free (error);
			}
			update_recent_file (plugin, uri, mime_type, error == NULL);
			g_list_free (uris);
		}
	}
	g_list_foreach (mime_apps, (GFunc) g_object_unref, NULL);
	g_list_free (mime_apps);
	if (plugin_descs)
		g_list_free (plugin_descs);
	gtk_widget_destroy (dialog);
}

static void
open_file (AnjutaFileLoaderPlugin *plugin, const gchar *uri)
{
	gchar *dirname;
	gchar *path;
	GFile* file;
	
	file = g_file_new_for_uri (uri);
	path = g_file_get_path (file);
	dirname = g_path_get_dirname (uri);
	chdir (dirname);
	g_free (path);
	g_free (dirname);
	
	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin),
							  file, FALSE, NULL);
	g_object_unref (file);
}

typedef struct
{
	AnjutaFileLoaderPlugin *plugin;
	const gchar *uri;
} RecentIdleOpenData;

static gboolean
on_open_recent_file_idle (gpointer data)
{
	RecentIdleOpenData *rdata;
	
	rdata = (RecentIdleOpenData*)data;
	open_file (rdata->plugin, rdata->uri);
	g_free (rdata);
	return FALSE;
}

static gboolean
on_open_recent_file (GtkRecentChooser *chooser, AnjutaFileLoaderPlugin *plugin)
{
	const gchar *uri;
	gboolean ret = TRUE;
	RecentIdleOpenData *rdata;
	
	uri = gtk_recent_chooser_get_current_uri (chooser);
	rdata = g_new0 (RecentIdleOpenData, 1);
	rdata->plugin = plugin;
	rdata->uri = uri;
	g_idle_add (on_open_recent_file_idle, rdata);

	return ret;
}

static void
on_open_response_ok (GtkDialog* dialog, gint id,
					 AnjutaFileLoaderPlugin *plugin)
{
	GSList *list, *node;

	if (id != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}
	
	list = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
	node = list;
	while (node)
	{
		if (node->data)
			open_file (plugin, (gchar *)node->data);
		g_free (node->data);
		node = g_slist_next (node);
	}
	g_slist_free (list);
}

static void
setup_file_filters (GtkFileChooser *fc)
{
	GtkFileFilter *filter;
	
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (fc, filter);
	
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Anjuta Projects"));
	gtk_file_filter_add_pattern (filter, "*.anjuta");
	gtk_file_filter_add_pattern (filter, "*.prj");
	gtk_file_chooser_add_filter (fc, filter);
	
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("C/C++ source files"));
	gtk_file_filter_add_pattern (filter, "*.c");
	gtk_file_filter_add_pattern (filter, "*.cc");
	gtk_file_filter_add_pattern (filter, "*.cpp");
	gtk_file_filter_add_pattern (filter, "*.cxx");
	gtk_file_filter_add_pattern (filter, "*.c++");
	gtk_file_filter_add_pattern (filter, "*.h");
	gtk_file_filter_add_pattern (filter, "*.hh");
	gtk_file_filter_add_pattern (filter, "*.hpp");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("C# source files"));
	gtk_file_filter_add_pattern (filter, "*.cs");
	gtk_file_filter_add_pattern (filter, "*.h");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Java source files"));
	gtk_file_filter_add_pattern (filter, "*.java");
	gtk_file_filter_add_pattern (filter, "*.js");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Pascal source files"));
	gtk_file_filter_add_pattern (filter, "*.pas");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("PHP source files"));
	gtk_file_filter_add_pattern (filter, "*.php");
	gtk_file_filter_add_pattern (filter, "*.php?");
	gtk_file_filter_add_pattern (filter, "*.phtml");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Perl source files"));
	gtk_file_filter_add_pattern (filter, "*.pl");
	gtk_file_filter_add_pattern (filter, "*.pm");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Python source files"));
	gtk_file_filter_add_pattern (filter, "*.py");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Hyper text markup files"));
	gtk_file_filter_add_pattern (filter, "*.htm");
	gtk_file_filter_add_pattern (filter, "*.html");
	gtk_file_filter_add_pattern (filter, "*.xhtml");
	gtk_file_filter_add_pattern (filter, "*.dhtml");
	gtk_file_filter_add_pattern (filter, "*.cs");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Shell scripts files"));
	gtk_file_filter_add_pattern (filter, "*.sh");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Makefiles"));
	gtk_file_filter_add_pattern (filter, "Makefile*");
	gtk_file_filter_add_pattern (filter, "makefile*");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Lua files"));
	gtk_file_filter_add_pattern (filter, "*.lua");
	gtk_file_chooser_add_filter (fc, filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Diff files"));
	gtk_file_filter_add_pattern (filter, "*.diff");
	gtk_file_filter_add_pattern (filter, "*.patch");
	gtk_file_filter_add_pattern (filter, "*.cvsdiff");
	gtk_file_chooser_add_filter (fc, filter);
}

static GtkWidget*
create_file_open_dialog_gui(GtkWindow* parent, AnjutaFileLoaderPlugin* plugin)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Open file"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_OPEN,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);
	gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_window_set_destroy_with_parent (GTK_WINDOW(dialog), TRUE);
	
	setup_file_filters (GTK_FILE_CHOOSER (dialog));
	
	g_signal_connect(G_OBJECT(dialog), "response", 
					G_CALLBACK(on_open_response_ok), plugin);
	g_signal_connect_swapped (GTK_OBJECT (dialog), 
                             "response", 
                             G_CALLBACK (gtk_widget_destroy),
                             GTK_OBJECT (dialog));
	return dialog;
}

static void
on_new_clicked (GtkToolButton *button, AnjutaFileLoaderPlugin *plugin)
{
	AnjutaShell* shell = ANJUTA_PLUGIN (plugin)->shell;
	IAnjutaDocumentManager *docman = anjuta_shell_get_interface (shell, 
																 IAnjutaDocumentManager,
																 NULL);
	if (docman)
		ianjuta_document_manager_add_buffer (docman, NULL, NULL, NULL);
}

static void
on_open_clicked (GtkToolButton *button, AnjutaFileLoaderPlugin *plugin)
{
	GtkWidget *dlg;
	
	dlg =
		create_file_open_dialog_gui (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									 plugin);
	gtk_widget_show (dlg);
}

static void
on_new_activate (GtkAction *action, AnjutaFileLoaderPlugin *plugin)
{
	AnjutaShell* shell = ANJUTA_PLUGIN (plugin)->shell;
	IAnjutaDocumentManager *docman = anjuta_shell_get_interface (shell, 
																 IAnjutaDocumentManager,
																 NULL);
	if (docman)
		ianjuta_document_manager_add_buffer (docman, NULL, NULL, NULL);
}

static void
on_open_activate (GtkAction *action, AnjutaFileLoaderPlugin *plugin)
{
	GtkWidget *dlg;
	
	dlg =
		create_file_open_dialog_gui (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									 plugin);
	gtk_widget_show (dlg);
}

static void
on_activate_wizard (GtkMenuItem *menuitem,
					AnjutaFileLoaderPlugin *loader)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaPluginDescription *desc;
	
	desc = g_object_get_data (G_OBJECT (menuitem), "__plugin_desc");
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (loader)->shell,
													  NULL);
	if (desc)
	{
		gchar *id;
		GObject *plugin;
		
		if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &id))
		{
			plugin =
				anjuta_plugin_manager_get_plugin_by_id (plugin_manager, id);
			ianjuta_wizard_activate (IANJUTA_WIZARD (plugin), NULL);
		}
	}
}

static GtkWidget*
on_create_submenu (gpointer user_data)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaFileLoaderPlugin *loader;
	GList *node;
	gint count;
	GtkWidget *submenu = NULL;
	GList *plugin_descs = NULL;
	
	loader = ANJUTA_PLUGIN_FILE_LOADER (user_data);
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (loader)->shell,
													  NULL);
	submenu = gtk_menu_new ();
	gtk_widget_show (submenu);
	
	plugin_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaWizard",
												NULL);
	plugin_descs = g_list_sort(plugin_descs, sort_wizards);
	node = plugin_descs;
	count = 0;
	while (node)
	{
		AnjutaPluginDescription *desc;
		GtkWidget *menuitem;
		GtkWidget *icon;
		gchar *str, *icon_path, *name;
		
		desc = node->data;
		
		icon = NULL;
		name = NULL;
		if (anjuta_plugin_description_get_locale_string (desc, "Wizard",
														 "Title", &str) ||
			anjuta_plugin_description_get_locale_string (desc, "Anjuta Plugin",
														 "Name", &str))
		{
			count++;
			if (count < 10)
				name = g_strdup_printf ("_%d. %s", count, N_(str));
			else
				name = g_strdup_printf ("%d. %s", count, N_(str));
			g_free (str);
		}
		if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Icon", &str))
		{
			GdkPixbuf *pixbuf, *scaled_pixbuf;
			gint height, width;
			
			gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (submenu),
											   GTK_ICON_SIZE_MENU,
											   &width, &height);
			icon_path = g_build_filename (PACKAGE_PIXMAPS_DIR, str, NULL);
			pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
			if (pixbuf)
			{
				scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height,
														 GDK_INTERP_BILINEAR);
				icon = gtk_image_new_from_pixbuf (scaled_pixbuf);
                g_object_unref (pixbuf);
				g_object_unref (scaled_pixbuf);
			}
			else
				icon = gtk_image_new ();
			
			gtk_widget_show (icon);
			g_free (icon_path);
			g_free (str);
		}
		if (name)
		{
			menuitem = gtk_image_menu_item_new_with_mnemonic (name);
			g_free(name);
			gtk_widget_show (menuitem);
			g_object_set_data (G_OBJECT (menuitem), "__plugin_desc", desc);
			g_signal_connect (G_OBJECT (menuitem), "activate",
							  G_CALLBACK (on_activate_wizard),
							  loader);
			if (icon)
				gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem),
											   icon);
			gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menuitem);
		}
		node = g_list_next (node);
	}
	g_list_free (plugin_descs);
	return submenu;
}

static void
open_uri_with (AnjutaFileLoaderPlugin *plugin, GtkMenuItem *menuitem,
				const gchar *uri)
{
	GAppInfo *app;
	AnjutaPluginDescription *desc;
	const gchar *mime_type;
	
	/* Open with plugin */
	desc = (AnjutaPluginDescription*) g_object_get_data (G_OBJECT (menuitem),
														 "desc");
	mime_type = (const gchar*) g_object_get_data (G_OBJECT (menuitem),
														 "mime_type");
	if (desc)
	{
		AnjutaPluginManager *plugin_manager;
		gchar *location = NULL;

		plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell,
	 														  NULL);
		
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &location);
		g_assert (location != NULL);
		if (location != NULL)
		{
			GObject *loaded_plugin;
			
			loaded_plugin =
				anjuta_plugin_manager_get_plugin_by_id (plugin_manager,
														location);
			if (loaded_plugin)
			{
				GFile* file = g_file_new_for_uri (uri);
				GError *error = NULL;
				
				ianjuta_file_open (IANJUTA_FILE (loaded_plugin), file, &error);
				g_object_unref (file);
				update_recent_file (plugin, uri, mime_type, error == NULL);
				g_free (error);
			}
			else
			{
				anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
										  _("Failed to activate plugin: %s"),
										  location);
			}
			g_free (location);
		}
	}
	else
	{
		/* Open with application */
		app = (GAppInfo *) g_object_get_data (G_OBJECT (menuitem),
															 "app");
		if (app)
		{
			GList *uris = NULL;
			GError *error = NULL;
		
			uris = g_list_prepend (uris, (gpointer)uri);
			g_app_info_launch_uris (app, uris, NULL, &error);
			g_list_free (uris);
			if (error)
			{
				launch_application_failure (plugin, uri, error->message);
				g_error_free (error);
			}
			update_recent_file (plugin, uri, mime_type, error == NULL);
		}
	}
}

static void
fm_open (GtkAction *action, AnjutaFileLoaderPlugin *plugin)
{
	if (plugin->fm_current_uri)
		open_file (plugin, plugin->fm_current_uri);
}

static void
fm_open_with (GtkMenuItem *menuitem, AnjutaFileLoaderPlugin *plugin)
{
	if (plugin->fm_current_uri)
		open_uri_with (plugin, menuitem, plugin->fm_current_uri);
}

static void
pm_open (GtkAction *action, AnjutaFileLoaderPlugin *plugin)
{
	if (plugin->pm_current_uri)
		open_file (plugin, plugin->pm_current_uri);
}

static void
pm_open_with (GtkMenuItem *menuitem, AnjutaFileLoaderPlugin *plugin)
{
	if (plugin->pm_current_uri)
		open_uri_with (plugin, menuitem, plugin->pm_current_uri);
}

static GtkActionEntry actions_file[] = {
	{ 
		"ActionFileNew", 
		GTK_STOCK_NEW,
		N_("_New"), 
		"<control>n",
		N_("New empty file"),
		G_CALLBACK (on_new_activate)
	},
	{
		"ActionFileOpen",
		GTK_STOCK_OPEN,
		N_("_Open..."),
		"<control>o",
		N_("Open file"),
		G_CALLBACK (on_open_activate)
	}
};

static GtkActionEntry popup_actions_file[] = {
	{
		"ActionPopupOpen",
		GTK_STOCK_OPEN,
		N_("_Open"), NULL,
		N_("Open file"),
		G_CALLBACK (fm_open)
	},
	{
		"ActionPopupOpenWith",
		NULL,
		N_("Open _With"), NULL,
		N_("Open with"), NULL
	},
	{
		"ActionPopupPMOpen",
		GTK_STOCK_OPEN,
		N_("_Open"), NULL,
		N_("Open file"),
		G_CALLBACK (pm_open)
	},
	{
		"ActionPopupPMOpenWith",
		NULL,
		N_("Open _With"), NULL,
		N_("Open with"), NULL
	}
};

static gboolean
create_open_with_submenu (AnjutaFileLoaderPlugin *plugin, GtkWidget *parentmenu,
						  const gchar *uri, GCallback callback,
						  gpointer callback_data)
{
	GList *mime_apps;
	GList *plugin_descs;
	GList *node;	
	GtkWidget *menu, *menuitem;
	gchar *mime_type;
	GFile *file;
	
	g_return_val_if_fail (GTK_IS_MENU_ITEM (parentmenu), FALSE);
	
	menu = gtk_menu_new ();
	gtk_widget_show (menu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (parentmenu), menu);
	
	file = g_file_new_for_uri (uri);
	mime_type = anjuta_util_get_file_mime_type (file);
	g_object_unref (file);
	if (mime_type == NULL)
		return FALSE;
	
	/* Open with plugins menu items */
	plugin_descs = get_available_plugins_for_mime (ANJUTA_PLUGIN (plugin), mime_type);
	for (node = plugin_descs; node != NULL; node = g_list_next (node))
	{
		gchar *name;
		AnjutaPluginDescription *desc;
		
		desc = (AnjutaPluginDescription *)(node->data);
		name = NULL;
		anjuta_plugin_description_get_locale_string (desc, "File Loader",
													 "Title", &name);
		if (!name)
		{
			anjuta_plugin_description_get_locale_string (desc, "Anjuta Plugin",
														 "Name", &name);
		}
		if (!name)
		{
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &name);
		}
		menuitem = gtk_menu_item_new_with_label (name);
		g_object_set_data (G_OBJECT (menuitem), "desc", (gpointer)(desc));
		g_object_set_data (G_OBJECT (menuitem), "mime_type", mime_type);			
		g_signal_connect (G_OBJECT (menuitem), "activate",
						  G_CALLBACK (callback), callback_data);
		gtk_menu_append (menu, menuitem);
		g_free (name);
	}
	g_list_free (plugin_descs);
	
	/* Open with applications */
	mime_apps = g_app_info_get_all_for_type (mime_type);
	if (plugin_descs && mime_apps)
	{
		menuitem = gtk_menu_item_new ();
		gtk_menu_append (menu, menuitem);
	}
	
	for (node = mime_apps; node != NULL; node = g_list_next (node))
	{
		GAppInfo *mime_app;
		
		mime_app = (GAppInfo *)(node->data);
		if (g_app_info_should_show (mime_app))
		{
			menuitem = gtk_menu_item_new_with_label ( g_app_info_get_name (mime_app));
			g_object_set_data_full (G_OBJECT (menuitem), "app", mime_app, g_object_unref);			
			g_object_set_data (G_OBJECT (menuitem), "mime_type", mime_type);			
			g_signal_connect (G_OBJECT (menuitem), "activate",
							  G_CALLBACK (callback), callback_data);
			gtk_menu_append (menu, menuitem);
		}
		else
		{
			g_object_unref (mime_app);
		}
	}
	g_list_free (mime_apps);

	gtk_widget_show_all (menu);

	if ((mime_apps != NULL) || (plugin_descs != NULL))
	{
		g_object_set_data_full (G_OBJECT (menu), "mime_type", (gpointer)mime_type, g_free);
		
		return TRUE;
	}
	else
	{
		g_free (mime_type);
		
		return FALSE;
	}
}
						  
static void
value_added_fm_current_file (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	gchar *uri;
	AnjutaFileLoaderPlugin *fl_plugin;
	GtkAction *action;
	GtkWidget *parentmenu;
	GFile* file = G_FILE (g_value_get_object (value));
	
	uri = g_file_get_uri (file);
	g_return_if_fail (name != NULL);

	fl_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader", "ActionPopupOpen");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader",
								   "ActionPopupOpenWith");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	
	if (fl_plugin->fm_current_uri)
		g_free (fl_plugin->fm_current_uri);
	fl_plugin->fm_current_uri = g_strdup (uri);
	
	parentmenu =
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/PopupFileManager/PlaceholderPopupFileOpen/OpenWith");
	if (!create_open_with_submenu (fl_plugin, parentmenu, uri,
								   G_CALLBACK (fm_open_with), plugin))
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
	g_free (uri);
}

static void
value_removed_fm_current_file (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	AnjutaFileLoaderPlugin *fl_plugin;

	fl_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	
	if (fl_plugin->fm_current_uri)
		g_free (fl_plugin->fm_current_uri);
	fl_plugin->fm_current_uri = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader", "ActionPopupOpen");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader",
								   "ActionPopupOpenWith");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
value_added_pm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	const gchar *uri;
	AnjutaFileLoaderPlugin *fl_plugin;
	GtkAction *action;
	GtkWidget *parentmenu;
	
	uri = g_value_get_string (value);
	g_return_if_fail (name != NULL);
	
	fl_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader", "ActionPopupPMOpen");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader",
								   "ActionPopupPMOpenWith");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	
	if (fl_plugin->pm_current_uri)
		g_free (fl_plugin->pm_current_uri);
	fl_plugin->pm_current_uri = g_strdup (uri);
	
	parentmenu =
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/PopupProjectManager/PlaceholderPopupProjectOpen/OpenWith");
	if (!create_open_with_submenu (fl_plugin, parentmenu, uri,
								   G_CALLBACK (pm_open_with), plugin))
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
value_removed_pm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	AnjutaFileLoaderPlugin *fl_plugin;

	fl_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	
	if (fl_plugin->pm_current_uri)
		g_free (fl_plugin->pm_current_uri);
	fl_plugin->pm_current_uri = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader", "ActionPopupPMOpen");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupLoader",
								   "ActionPopupPMOpenWith");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
dnd_dropped (const gchar *uri, gpointer plugin)
{
	GFile* file = g_file_new_for_uri (uri);
	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin), file, FALSE, NULL);
	g_object_unref (file);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 AnjutaFileLoaderPlugin *plugin)
{
	GList *files, *node;
				
	/* We want to load the files first before other session loads */
	if (phase != ANJUTA_SESSION_PHASE_FIRST)
		return;
	
	files = anjuta_session_get_string_list (session, "File Loader", "Files");
	if (!files)
		return;
		
	/* Open all files except project files */
	for (node = g_list_first (files); node != NULL; node = g_list_next (node))
	{
		gchar *uri = node->data;
		
		if (uri)
		{
			gchar *fragment;
			
			fragment = strchr (uri, '#');
			if (fragment)
				*fragment = '\0';

			if (!anjuta_util_is_project_file (uri))
			{
				GFile* file = g_file_new_for_uri (uri);
				GObject *loader = ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin),
											  file, FALSE, NULL);
				if (fragment)
				{
					if (IANJUTA_IS_DOCUMENT_MANAGER (loader))
					{
						ianjuta_document_manager_goto_file_line (IANJUTA_DOCUMENT_MANAGER (loader), file, atoi(fragment + 1), NULL);
					}
				}
				g_object_unref (file);
			}
		}
		g_free (uri);
	}
	g_list_free (files);
}

static void
setup_recent_chooser_menu (GtkRecentChooser* recent_menu, AnjutaFileLoaderPlugin* plugin)
{
	GtkRecentFilter *filter;

	gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER (recent_menu), TRUE);
	gtk_recent_chooser_set_show_icons (GTK_RECENT_CHOOSER (recent_menu), TRUE);
	gtk_recent_chooser_set_show_tips (GTK_RECENT_CHOOSER (recent_menu), TRUE);
	gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (recent_menu), GTK_RECENT_SORT_MRU);
	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (recent_menu), 20);

	filter = gtk_recent_filter_new ();
	gtk_recent_filter_add_application (filter, g_get_application_name ());
	gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (recent_menu), filter);

	g_signal_connect (recent_menu, "item-activated",
					  G_CALLBACK (on_open_recent_file), plugin);	
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkAction *action, *saction;
	AnjutaUI *ui;
	AnjutaFileLoaderPlugin *loader_plugin;
	GtkActionGroup *group;
	GtkWidget *widget;
	GtkWidget* recent_menu;
	GtkWidget* toolbar_menu;
	
	loader_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	
	DEBUG_PRINT ("%s", "AnjutaFileLoaderPlugin: Activating File Loader plugin...");
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Recent manager */
	loader_plugin->recent_manager = gtk_recent_manager_get_default();
	
	/* Add action group */
	loader_plugin->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupLoader",
											_("File Loader"),
											actions_file,
											G_N_ELEMENTS (actions_file),
											GETTEXT_PACKAGE, TRUE, plugin);
	loader_plugin->popup_action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupPopupLoader",
											_("File Loader"),
											popup_actions_file,
											G_N_ELEMENTS (popup_actions_file),
											GETTEXT_PACKAGE, FALSE, plugin);
	saction = gtk_recent_action_new ("ActionFileWizard", _("New"),
							  _("New file, project and project components."), NULL);
	g_object_set (saction, "stock-id", GTK_STOCK_NEW, NULL);
	gtk_action_group_add_action (loader_plugin->action_group,
								 GTK_ACTION (saction));
	
	/* Set short labels */
	action = anjuta_ui_get_action (ui, "ActionGroupLoader", "ActionFileOpen");
	g_object_set (G_OBJECT (action), "short-label", _("Open"),
				  "is-important", TRUE, NULL);

	group = gtk_action_group_new ("ActionGroupLoaderRecent");
	action = gtk_recent_action_new ("ActionFileOpenRecent", _("Open _Recent"),
												_("Open recent file"), NULL);
	g_object_set (action, "stock-id", GTK_STOCK_OPEN, NULL);
	setup_recent_chooser_menu (GTK_RECENT_CHOOSER (action), loader_plugin);
	
	gtk_action_group_add_action (group, action);
	anjuta_ui_add_action_group (ui, "ActionGroupLoaderRecent",
								N_("Open recent files"), group, FALSE);
	loader_plugin->recent_group = group;
	
	/* Add UI */
	loader_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* Adding submenus */
	recent_menu = anjuta_recent_chooser_menu_new_for_manager (loader_plugin->recent_manager);
	setup_recent_chooser_menu (GTK_RECENT_CHOOSER (recent_menu), loader_plugin);
	widget = gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/MenuMain/MenuFile/PlaceholderFileMenus/OpenRecent");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), 
							   recent_menu);
	
	widget = gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/MenuMain/MenuFile/PlaceholderFileMenus/Wizard");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), on_create_submenu(loader_plugin));
	
	widget = gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/ToolbarMain/PlaceholderFileToolbar/New");
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (widget), on_create_submenu(loader_plugin));
	g_signal_connect (widget, "clicked", G_CALLBACK (on_new_clicked), loader_plugin);
	
	widget = gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/ToolbarMain/PlaceholderFileToolbar/Open");
	toolbar_menu = anjuta_recent_chooser_menu_new_for_manager (loader_plugin->recent_manager);
	setup_recent_chooser_menu (GTK_RECENT_CHOOSER (toolbar_menu), loader_plugin);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (widget),
								   toolbar_menu);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (widget), _("Open"));
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (widget), _("Open a file"));
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (widget), _("Open recent file"));
	g_signal_connect (widget, "clicked", G_CALLBACK (on_open_clicked), loader_plugin);

	/* Install drag n drop handler */
	dnd_drop_init (GTK_WIDGET (plugin->shell), dnd_dropped, plugin,
				   "text/plain", "text/html", "text/source", "application-x/anjuta",
				   NULL);
	
	/* Add watches */
	loader_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, IANJUTA_FILE_MANAGER_SELECTED_FILE,
								 value_added_fm_current_file,
								 value_removed_fm_current_file, NULL);
	loader_plugin->pm_watch_id = 
		anjuta_plugin_add_watch (plugin, IANJUTA_PROJECT_MANAGER_CURRENT_URI,
								 value_added_pm_current_uri,
								 value_removed_pm_current_uri, NULL);
	
	/* Connect to session */
	g_signal_connect (G_OBJECT (plugin->shell), "load_session",
					  G_CALLBACK (on_session_load), plugin);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaFileLoaderPlugin *loader_plugin;
	
	loader_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	
	DEBUG_PRINT ("%s", "AnjutaFileLoaderPlugin: Deactivating File Loader plugin...");
	
	/* Disconnect session */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_load), plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, loader_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, loader_plugin->pm_watch_id, TRUE);
	/* Uninstall dnd */
	dnd_drop_finalize (GTK_WIDGET (plugin->shell), plugin);
	/* Remove UI */
	anjuta_ui_unmerge (ui, loader_plugin->uiid);
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, loader_plugin->action_group);
	anjuta_ui_remove_action_group (ui, loader_plugin->popup_action_group);
	anjuta_ui_remove_action_group (ui, loader_plugin->recent_group);
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
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
anjuta_file_loader_plugin_instance_init (GObject *obj)
{
	AnjutaFileLoaderPlugin *plugin = ANJUTA_PLUGIN_FILE_LOADER (obj);
	
	plugin->fm_current_uri = NULL;
	plugin->pm_current_uri = NULL;
}

static void
anjuta_file_loader_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static GObject*
iloader_load (IAnjutaFileLoader *loader, GFile* file,
			  gboolean read_only, GError **err)
{
	gchar *mime_type;
	AnjutaStatus *status;
	AnjutaPluginManager *plugin_manager;
	GList *plugin_descs = NULL;
	GObject *plugin = NULL;	
	gchar *uri = g_file_get_uri (file);
	
	g_return_val_if_fail (uri != NULL, NULL);

	mime_type = anjuta_util_get_file_mime_type (file);
	
	if (mime_type == NULL)
	{
		update_recent_file (ANJUTA_PLUGIN_FILE_LOADER (loader), uri, NULL, FALSE);	
		
		if (err == NULL)
			launch_application_failure (ANJUTA_PLUGIN_FILE_LOADER (loader), uri, _("File not found"));
		
		g_set_error (err, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, _("File not found"));
		
		g_free (uri);
		return NULL;
	}
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (loader)->shell,
													  NULL);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (loader)->shell, NULL);
	anjuta_status_busy_push (status);
	
	DEBUG_PRINT ("Opening URI: %s", uri);
	
	plugin_descs = get_available_plugins_for_mime (ANJUTA_PLUGIN (loader), mime_type);
	
	if (g_list_length (plugin_descs) > 1)
	{
		gchar* basename = g_path_get_basename (uri);
		/* %s is name of file that will be opened */ 
		gchar* message = g_strdup_printf (_("Please select a plugin to open <b>%s</b>."), 
										 basename);
		plugin =
			anjuta_plugin_manager_select_and_activate (plugin_manager,
													   _("Open With"),
													   message,
													   plugin_descs);
		g_free (basename);
		g_free (message);
	}
	else if (g_list_length (plugin_descs) == 1)
	{
		gchar *location = NULL;
		
		AnjutaPluginDescription *desc = plugin_descs->data;
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &location);
		g_return_val_if_fail (location != NULL, NULL);
		plugin =
			anjuta_plugin_manager_get_plugin_by_id (plugin_manager,
													location);
		g_free (location);
	}
	else
	{
		GAppInfo *appinfo;
		GList *uris = NULL;

		uris = g_list_prepend (uris, (gpointer)uri);

		appinfo = g_app_info_get_default_for_type (mime_type, TRUE);
		if (appinfo)
		{
			GError *error = NULL;
			if (!g_app_info_launch_uris (appinfo, uris, NULL, &error))
			{
				open_with_dialog (ANJUTA_PLUGIN_FILE_LOADER (loader), uri, mime_type);
			}
			else
			{
				update_recent_file (ANJUTA_PLUGIN_FILE_LOADER (loader), uri, mime_type, error == NULL);
			}				
			g_object_unref (G_OBJECT (appinfo));
		}
		g_list_free (uris);
	}
	
	if (plugin)
	{
		GError *error = NULL;
		
		ianjuta_file_open (IANJUTA_FILE(plugin), file, &error);
		if (error != NULL)
		{
			
			if (err == NULL)
				launch_application_failure (ANJUTA_PLUGIN_FILE_LOADER (loader), uri, error->message);
			g_propagate_error (err, error);
		}
		update_recent_file (ANJUTA_PLUGIN_FILE_LOADER (loader), uri, mime_type, error == NULL);
	}
	
	if (plugin_descs)
		g_list_free (plugin_descs);
	
	g_free (mime_type);
	g_free (uri);
	anjuta_status_busy_pop (status);
		
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
ANJUTA_PLUGIN_ADD_INTERFACE (iloader, IANJUTA_TYPE_LOADER);
ANJUTA_PLUGIN_ADD_INTERFACE (iloader_file, IANJUTA_TYPE_FILE_LOADER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaFileLoaderPlugin, anjuta_file_loader_plugin);

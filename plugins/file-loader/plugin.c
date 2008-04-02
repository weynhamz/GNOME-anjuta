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
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include <glib/gi18n.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libegg/menu/egg-submenu-action.h>
#include <libegg/menu/egg-recent-action.h>

#include "plugin.h"
#include "dnd.h"

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

static void
set_recent_file (AnjutaFileLoaderPlugin *plugin, const gchar *uri,
				 const gchar *mime)
{
	EggRecentItem *recent_item;
	DEBUG_PRINT ("Adding recent item of mimi-type: %s", mime);
	recent_item = egg_recent_item_new ();
	egg_recent_item_set_uri (recent_item, uri);
	egg_recent_item_set_mime_type (recent_item, mime);
	if (strcmp (mime, "application/x-anjuta") == 0)
	{
		egg_recent_item_add_group (recent_item, "anjuta-projects");
		egg_recent_model_add_full (plugin->recent_files_model_top,
								   recent_item);
	}
	else
	{
		egg_recent_item_add_group (recent_item, "anjuta-files");
		egg_recent_model_add_full (plugin->recent_files_model_bottom,
								   recent_item);
	}
}

static void
launch_application_failure (AnjutaFileLoaderPlugin *plugin,
							const gchar *uri,
							GnomeVFSResult result)
{
	const gchar *errmsg;
	GtkWidget *parent;
	
	errmsg = gnome_vfs_result_to_string (result);
	parent =
		gtk_widget_get_toplevel (GTK_WIDGET(ANJUTA_PLUGIN (plugin)->shell));
	anjuta_util_dialog_error (GTK_WINDOW (parent),
							  "Can not open \"%s\".\n\n%s",
							  g_basename (uri), errmsg);
}

static GList *
get_available_plugins_for_mime (AnjutaFileLoaderPlugin* plugin,
							   const gchar *mime_type)
{
	AnjutaPluginManager *plugin_manager;
	GList *plugin_descs = NULL;
	
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN(plugin)->shell,
													  NULL);
	plugin_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaFile",
												"File Loader",
												"SupportedMimeTypes",
												mime_type,
												NULL);
	if (!plugin_descs);
	{
		gchar* supertype = gnome_vfs_get_supertype_from_mime_type (mime_type);
		plugin_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaFile",
												"File Loader",
												"SupportedMimeTypes",
												supertype,
												NULL);
		g_free (supertype);
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
	GnomeVFSMimeApplication *mime_app;
	
	GtkWidget *dialog, *parent, *hbox, *label;
	GtkWidget *options;
	gchar *message;
	AnjutaPluginManager *plugin_manager;
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell,
													  NULL);
	message = g_strdup_printf (_("<b>Cannot open \"%s\"</b>.\n\n"
								 "There is no plugin, default action, or application "
								 "configured to handle this file type.\n"
								 "\n"
								 "Mime type: %s\n"
								 "\n"
								 "You may choose to try opening it with the following "
								 "plugins or applications."),
							   g_basename(uri), mime_type);
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
	plugin_descs = get_available_plugins_for_mime (plugin, mime_type);
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
	mime_apps = gnome_vfs_mime_get_all_applications (mime_type);
	if (mime_apps)
	{
		/* Separator */
		menuitem = gtk_menu_item_new ();
		gtk_menu_append (menu, menuitem);
	}	
	node = mime_apps;
	while (node)
	{
		mime_app = (GnomeVFSMimeApplication *)(node->data);
		menuitem = gtk_menu_item_new_with_label (mime_app->name);
		gtk_menu_append (menu, menuitem);
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
				ianjuta_file_open (IANJUTA_FILE (docman), uri, NULL);
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
					ianjuta_file_open (IANJUTA_FILE (loaded_plugin), uri, NULL);
					set_recent_file (plugin, uri, mime_type);
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
			GnomeVFSResult res;
			
			option -= (g_list_length (plugin_descs) + 2);
			mime_app = g_list_nth_data (mime_apps, option);
			uris = g_list_prepend (uris, (gpointer)uri);
			res = gnome_vfs_mime_application_launch (mime_app, uris);
			if (res != GNOME_VFS_OK)
				launch_application_failure (plugin, uri, res);
			else
				set_recent_file (plugin, uri, mime_type);
			g_list_free (uris);
		}
	}
	gnome_vfs_mime_application_list_free (mime_apps);
	if (plugin_descs)
		g_list_free (plugin_descs);
	gtk_widget_destroy (dialog);
}

static gboolean
launch_in_default_application (AnjutaFileLoaderPlugin *plugin,
							   const gchar *mime_type, const gchar *uri)
{
	GnomeVFSMimeAction *action;
	GList *uris = NULL;
	gboolean ret;
	
	uris = g_list_prepend (uris, (gpointer)uri);
	
	ret = TRUE;
	action = gnome_vfs_mime_get_default_action (mime_type);
	if (!action || gnome_vfs_mime_action_launch (action, uris) != GNOME_VFS_OK)
	{
		GnomeVFSMimeApplication *app;
		GnomeVFSResult res;
		app = gnome_vfs_mime_get_default_application (mime_type);
		if (!app ||
			(res =
				gnome_vfs_mime_application_launch (app, uris)) != GNOME_VFS_OK)
		{
			open_with_dialog (plugin, uri, mime_type);
		}
		if (app)
			gnome_vfs_mime_application_free (app);
	}
	if (action)
		gnome_vfs_mime_action_free (action);
	g_list_free (uris);
	return ret;
}

static void
open_file (AnjutaFileLoaderPlugin *plugin, const gchar *uri)
{
	GnomeVFSURI *vfs_uri;	
	gchar *dirname;
	
	vfs_uri = gnome_vfs_uri_new (uri);
	dirname = gnome_vfs_uri_extract_dirname (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);
	chdir (dirname);
	g_free (dirname);
	
	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin),
							  uri, FALSE, NULL);
}

typedef struct
{
	AnjutaFileLoaderPlugin *plugin;
	const gchar *uri;
} RecentIdelOpenData;

static gboolean
on_open_recent_file_idle (gpointer data)
{
	RecentIdelOpenData *rdata;
	
	rdata = (RecentIdelOpenData*)data;
	open_file (rdata->plugin, rdata->uri);
	g_free (rdata);
	return FALSE;
}

static gboolean
on_open_recent_file (EggRecentAction *action, AnjutaFileLoaderPlugin *plugin)
{
	const gchar *uri;
	GnomeVFSURI *vfs_uri;
	gboolean ret = TRUE;
	RecentIdelOpenData *rdata;
	
	uri = egg_recent_action_get_selected_uri (action);
	vfs_uri = gnome_vfs_uri_new (uri);
	rdata = g_new0 (RecentIdelOpenData, 1);
	rdata->plugin = plugin;
	rdata->uri = uri;
	g_idle_add (on_open_recent_file_idle, rdata);
	gnome_vfs_uri_unref (vfs_uri);

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

/*
static void
on_recent_files_tooltip (GtkTooltips *tooltips, GtkWidget *menu_item,
						 EggRecentItem *item, gpointer user_data)
{
	char *uri, *tip;

	uri = egg_recent_item_get_uri_for_display (item);
	tip = g_strdup_printf ("Open '%s'", uri);

	gtk_tooltips_set_tip (tooltips, menu_item, tip, NULL);

	g_free (uri);
}
*/

static void
open_file_with (AnjutaFileLoaderPlugin *plugin, GtkMenuItem *menuitem,
				const gchar *uri)
{
	GList *mime_apps;
	GnomeVFSMimeApplication *mime_app;
	gchar *mime_type;
	gint idx;
	AnjutaPluginDescription *desc;
	AnjutaPluginManager *plugin_manager;
	
	idx = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "index"));
	desc = (AnjutaPluginDescription*) g_object_get_data (G_OBJECT (menuitem),
														 "desc");
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell,
													  NULL);
	mime_type = anjuta_util_get_uri_mime_type (uri);
	mime_apps = gnome_vfs_mime_get_all_applications (mime_type);
	
	/* Open with plugin */
	if (desc)
	{
		gchar *location = NULL;
		
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
				ianjuta_file_open (IANJUTA_FILE (loaded_plugin), uri, NULL);
				set_recent_file (plugin, uri, mime_type);
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
		GList *uris = NULL;
		GnomeVFSResult res;
		
		mime_app = g_list_nth_data (mime_apps, idx);
		uris = g_list_prepend (uris, (gpointer)uri);
		res = gnome_vfs_mime_application_launch (mime_app, uris);
		if (res != GNOME_VFS_OK)
			launch_application_failure (plugin, uri, res);
		else
			set_recent_file (plugin, uri, mime_type);
		g_list_free (uris);
	}
	gnome_vfs_mime_application_list_free (mime_apps);
	g_free (mime_type);
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
		open_file_with (plugin, menuitem, plugin->fm_current_uri);
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
		open_file_with (plugin, menuitem, plugin->pm_current_uri);
}

static GtkActionEntry actions_file[] = {
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
	GList *mime_apps, *node;
	GnomeVFSMimeApplication *mime_app;
	GList *plugin_descs, *snode;
	GtkWidget *menu, *menuitem;
	gchar *mime_type;
	gint idx;
	gboolean ret;
	
	g_return_val_if_fail (GTK_IS_MENU_ITEM (parentmenu), FALSE);
	
	menu = gtk_menu_new ();
	gtk_widget_show (menu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (parentmenu), menu);
	
	mime_type = anjuta_util_get_uri_mime_type (uri);
	if (mime_type == NULL)
		return FALSE;
	
	idx = 0;
	
	/* Open with plugins menu items */
	plugin_descs = get_available_plugins_for_mime (plugin, mime_type);
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
		g_object_set_data (G_OBJECT (menuitem), "index", GINT_TO_POINTER (idx));
		g_object_set_data (G_OBJECT (menuitem), "desc", (gpointer)(desc));
		g_signal_connect (G_OBJECT (menuitem), "activate",
						  G_CALLBACK (callback), callback_data);
		gtk_menu_append (menu, menuitem);
		g_free (name);
		snode = g_list_next (snode);
		idx++;
	}
	
	/* Open with applications */
	mime_apps = gnome_vfs_mime_get_all_applications (mime_type);
	if (idx > 0 && mime_apps)
	{
		menuitem = gtk_menu_item_new ();
		gtk_menu_append (menu, menuitem);
		idx++;
	}
	g_free (mime_type);
	idx = 0;
	node = mime_apps;
	while (node)
	{
		mime_app = (GnomeVFSMimeApplication *)(node->data);
		menuitem = gtk_menu_item_new_with_label (mime_app->name);
		g_object_set_data (G_OBJECT (menuitem), "index", GINT_TO_POINTER (idx));
		g_signal_connect (G_OBJECT (menuitem), "activate",
						  G_CALLBACK (callback), callback_data);
		gtk_menu_append (menu, menuitem);
		node = g_list_next (node);
		idx++;
	}
	gtk_widget_show_all (menu);
	
	if (mime_apps == NULL && plugin_descs == NULL)
		ret = FALSE;
	else
		ret = TRUE;
	
	gnome_vfs_mime_application_list_free (mime_apps);
	if (plugin_descs)
		g_list_free (plugin_descs);
	
	return ret;
}
						  
static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
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
}

static void
value_removed_fm_current_uri (AnjutaPlugin *plugin,
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
	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin), uri, FALSE, NULL);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 AnjutaFileLoaderPlugin *plugin)
{
	AnjutaStatus *status;
	const gchar *uri;
	gchar *mime_type;
	GList *files, *node;
	gint i;
				
	/* We want to load the files first before other session loads */
	if (phase != ANJUTA_SESSION_PHASE_FIRST)
		return;
	
	files = anjuta_session_get_string_list (session, "File Loader", "Files");
	if (!files)
		return;
	
	status = anjuta_shell_get_status (shell, NULL);
	anjuta_status_progress_add_ticks (status, g_list_length(files));
	
	/* Open project files first and then regular files */
	for (i = 0; i < 2; i++)
	{
		node = files;
		while (node)
		{
			uri = node->data;
			if (uri)
			{
				gchar *label, *filename;
				
				mime_type = anjuta_util_get_uri_mime_type (uri);
				
				filename = g_path_get_basename (uri);
				if (strchr (filename, '#'))
					*(strchr (filename, '#')) = '\0';
				
				label = g_strconcat ("Loaded: ", filename, NULL);
				
				if (i == 0 && mime_type &&
					strcmp (mime_type, "application/x-anjuta") == 0)
				{
					/* Project files first */
					/* FIXME: Ignore project files for now */
					/*
					ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin),
											  uri, FALSE, NULL);
					*/
					anjuta_status_progress_tick (status, NULL, label);
				}
				else if (i != 0 &&
						 (!mime_type ||
						  strcmp (mime_type, "application/x-anjuta") != 0))
				{
					/* Then rest of the files */
					ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin),
											  uri, FALSE, NULL);
					anjuta_status_progress_tick (status, NULL, label);
				}
				g_free (filename);
				g_free (label);
				g_free (mime_type);
			}
			node = g_list_next (node);
		}
	}
	if (files)
	{
		g_list_foreach (files, (GFunc)g_free, NULL);
		g_list_free (files);
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	EggSubmenuAction *saction;
	GtkAction *action;
	AnjutaUI *ui;
	AnjutaFileLoaderPlugin *loader_plugin;
	// GtkWidget *recent_menu;
	// EggRecentViewGtk *recent_view;
	GtkActionGroup *group;
	
	loader_plugin = ANJUTA_PLUGIN_FILE_LOADER (plugin);
	
	DEBUG_PRINT ("AnjutaFileLoaderPlugin: Activating File Loader plugin...");
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
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
	saction = g_object_new (EGG_TYPE_SUBMENU_ACTION,
							"name", "ActionFileWizard",
							"label", _("New"),
							"tooltip", _("New file, project and project components."),
							NULL);
	egg_submenu_action_set_menu_factory (saction,
										 on_create_submenu, plugin);
	gtk_action_group_add_action (loader_plugin->action_group,
								 GTK_ACTION (saction));
	
	/* Set short labels */
	action = anjuta_ui_get_action (ui, "ActionGroupLoader", "ActionFileOpen");
	g_object_set (G_OBJECT (action), "short-label", _("Open"),
				  "is-important", TRUE, NULL);

	group = gtk_action_group_new ("ActionGroupLoaderRecent");
	action = g_object_new (EGG_TYPE_RECENT_ACTION,
						   "name", "ActionFileOpenRecent",
						   "label", _("Open _Recent"),
						   "tooltip", _("Open recent file"),
							NULL);
	egg_recent_action_add_model (EGG_RECENT_ACTION (action),
								 loader_plugin->recent_files_model_top);
	egg_recent_action_add_model (EGG_RECENT_ACTION (action),
								 loader_plugin->recent_files_model_bottom);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_open_recent_file), plugin);
	gtk_action_group_add_action (group, action);
	anjuta_ui_add_action_group (ui, "ActionGroupLoaderRecent",
								N_("Open recent files"), group, FALSE);
	loader_plugin->recent_group = group;
	
	/* Add UI */
	loader_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* Install drag n drop handler */
	dnd_drop_init (GTK_WIDGET (plugin->shell), dnd_dropped, plugin,
				   "text/plain", "text/html", "text/source", "application-x/anjuta",
				   NULL);
	
	/* Add watches */
	loader_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	loader_plugin->pm_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_manager_current_uri",
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
	
	DEBUG_PRINT ("AnjutaFileLoaderPlugin: Deactivating File Loader plugin...");
	
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
	AnjutaFileLoaderPlugin *plugin = ANJUTA_PLUGIN_FILE_LOADER (obj);
	if (plugin->recent_files_model_top)
	{
		g_object_unref (plugin->recent_files_model_top);
		plugin->recent_files_model_top = NULL;
	}
	if (plugin->recent_files_model_bottom)
	{
		g_object_unref (plugin->recent_files_model_bottom);
		plugin->recent_files_model_bottom = NULL;
	}
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
	
	plugin->recent_files_model_top =
		egg_recent_model_new (EGG_RECENT_MODEL_SORT_MRU);
	egg_recent_model_set_limit (plugin->recent_files_model_top, 5);
	egg_recent_model_set_filter_groups (plugin->recent_files_model_top,
										"anjuta-projects", NULL);
	plugin->recent_files_model_bottom =
		egg_recent_model_new (EGG_RECENT_MODEL_SORT_MRU);
	egg_recent_model_set_limit (plugin->recent_files_model_bottom, 15);
	egg_recent_model_set_filter_groups (plugin->recent_files_model_bottom,
										"anjuta-files", NULL);
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
iloader_load (IAnjutaFileLoader *loader, const gchar *uri,
			  gboolean read_only, GError **err)
{
	gchar *mime_type;
	gchar *new_uri;
	GnomeVFSURI* vfs_uri;
	AnjutaStatus *status;
	AnjutaPluginManager *plugin_manager;
	GList *plugin_descs = NULL;
	GObject *plugin = NULL;	
	
	g_return_val_if_fail (uri != NULL, NULL);
	vfs_uri = gnome_vfs_uri_new (uri);
	
	if (!gnome_vfs_uri_exists (vfs_uri))
	{
		launch_application_failure (ANJUTA_PLUGIN_FILE_LOADER (loader),
									uri, GNOME_VFS_ERROR_NOT_FOUND);
		gnome_vfs_uri_unref (vfs_uri);
		return NULL;
	}
	
	new_uri = gnome_vfs_uri_to_string (vfs_uri,
									   GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER);
	mime_type = anjuta_util_get_uri_mime_type (new_uri);
	gnome_vfs_uri_unref (vfs_uri);
	
	if (mime_type == NULL)
	{
		launch_application_failure (ANJUTA_PLUGIN_FILE_LOADER (loader),
									new_uri, GNOME_VFS_ERROR_NOT_FOUND);
		g_free (new_uri);
		return NULL;
	}
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (loader)->shell,
													  NULL);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (loader)->shell, NULL);
	anjuta_status_busy_push (status);
	
	DEBUG_PRINT ("Opening URI: %s", uri);
	
	plugin_descs = anjuta_plugin_manager_query (plugin_manager,
												"Anjuta Plugin",
												"Interfaces", "IAnjutaFile",
												"File Loader",
												"SupportedMimeTypes",
												mime_type,
												NULL);
	
	if (g_list_length (plugin_descs) > 1)
	{
		plugin =
			anjuta_plugin_manager_select_and_activate (plugin_manager,
													   "Open With",
								"Please select a plugin to open this file.",
													   plugin_descs);
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
		launch_in_default_application (ANJUTA_PLUGIN_FILE_LOADER (loader), mime_type, uri);
	}
	if (plugin)
		ianjuta_file_open (IANJUTA_FILE(plugin), uri, NULL);
	
	set_recent_file (ANJUTA_PLUGIN_FILE_LOADER (loader), new_uri, mime_type);
	
	if (plugin_descs)
		g_list_free (plugin_descs);
	g_free (mime_type);
	g_free (new_uri);
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

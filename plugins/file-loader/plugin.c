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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/plugins.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include <libegg/menu/egg-recent-action.h>

#include "plugin.h"
#include "dnd.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-loader-plugin.ui"

static gpointer parent_class;

static gboolean
path_has_extension (const gchar *path, const gchar *ext)
{
	if (strlen (path) <= strlen (ext))
		return FALSE;
	if ((path[strlen (path) - strlen (ext) - 1] == '.') &&
		(strcmp (&path[strlen (path) - strlen (ext)], ext) == 0))
		return TRUE;
	return FALSE;
}

static gchar *
get_uri_mime_type (const gchar *uri)
{
	GnomeVFSURI *vfs_uri;
	const gchar *path;
	gchar *mime_type;
	
	vfs_uri = gnome_vfs_uri_new (uri);
	path = gnome_vfs_uri_get_path (vfs_uri);
	
	/* If Anjuta is not installed in system gnome prefix, the mime types 
	 * may not have been correctly registed. In that case, we use the
	 * following mime detection
	 */
	if (path_has_extension (path, "anjuta") ||
		path_has_extension (path, "prj"))
	{
		mime_type = g_strdup ("application/x-anjuta");
	}
	else
	{
		mime_type = gnome_vfs_get_mime_type (uri);
	}
	gnome_vfs_uri_unref (vfs_uri);
	return mime_type;
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

static GSList *
get_available_plugins_for_mime (AnjutaFileLoaderPlugin* plugin,
							   const gchar *mime_type)
{
	GSList *plugin_descs = NULL;
	
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	plugin_descs = anjuta_plugins_query (ANJUTA_PLUGIN(plugin)->shell,
										 "Anjuta Plugin",
										 "Interfaces", "IAnjutaFile",
										 "File Loader",
										 "SupportedMimeTypes", mime_type,
										 NULL);
	return plugin_descs;
}

static void
open_with_dialog (AnjutaFileLoaderPlugin *plugin, const gchar *uri,
				  const gchar *mime_type)
{
	GSList *plugin_descs, *snode;
	GList *mime_apps, *node;
	GtkWidget *menu, *menuitem;
	GnomeVFSMimeApplication *mime_app;
	
	GtkWidget *dialog, *parent, *hbox, *label;
	GtkWidget *options;
	gchar *message;
	
	message = g_strdup_printf (_("<b>Can not open \"%s\"</b>.\n\nThere is no suitable plugin capabale of handling this file. Nor any default action or application for this file has been configured in your desktop.\n\nMime type: %s.\n\nYou may choose to try opening it with following plugins or applications."), g_basename(uri), mime_type);
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
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Name", &name);
		if (!name)
		{
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &name);
		}
		menuitem = gtk_menu_item_new_with_label (name);
		gtk_menu_append (menu, menuitem);
		g_free (name);
		snode = g_slist_next (snode);
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
		else if (option < (g_slist_length (plugin_descs) + 1))
		{
			AnjutaPluginDescription *desc;
			gchar *location = NULL;
			
			option--;
			desc = g_slist_nth_data (plugin_descs, option);
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &location);
			g_assert (location != NULL);
			if (location != NULL)
			{
				GObject *loaded_plugin;
				
				loaded_plugin =
					anjuta_plugins_get_plugin_by_location (ANJUTA_PLUGIN(plugin)->shell,
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
			
			option -= (g_slist_length (plugin_descs) + 2);
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
		g_slist_free (plugin_descs);
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
									GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
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
fm_open (GtkAction *action, AnjutaFileLoaderPlugin *plugin)
{
	if (plugin->fm_current_uri)
		open_file (plugin, plugin->fm_current_uri);
}

static void
fm_open_with (GtkMenuItem *menuitem, AnjutaFileLoaderPlugin *plugin)
{
	// GSList *plugin_descs;
	GList *mime_apps;
	GnomeVFSMimeApplication *mime_app;
	gchar *mime_type;
	gint idx;
	const gchar *uri;
	AnjutaPluginDescription *desc;
	
	uri = plugin->fm_current_uri;
	idx = (gint) g_object_get_data (G_OBJECT (menuitem), "index");
	desc = (AnjutaPluginDescription*) g_object_get_data (G_OBJECT (menuitem),
														 "desc");
	mime_type = get_uri_mime_type (uri);
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
				anjuta_plugins_get_plugin_by_location (ANJUTA_PLUGIN(plugin)->shell,
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

static GtkActionEntry actions_file[] = {
	{
		"ActionFileNew",
		GTK_STOCK_NEW,
		N_("_New ..."),
		"<control>n",
		N_("New file, project and project components."),
		G_CALLBACK (on_new_activate)
	},
	{
		"ActionFileOpen",
		GTK_STOCK_OPEN,
		N_("_Open ..."),
		"<control>o",
		N_("Open file"),
		G_CALLBACK (on_open_activate)
	},
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
	}
};

static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	const gchar *uri;
	AnjutaFileLoaderPlugin *fl_plugin;
	GtkAction *action;
	
	GList *mime_apps, *node;
	GnomeVFSMimeApplication *mime_app;
	
	GSList *plugin_descs, *snode;
	
	GtkWidget *menu, *menuitem, *parentmenu;
	gchar *mime_type;
	gint idx;
	
	uri = g_value_get_string (value);
	g_return_if_fail (name != NULL);

	fl_plugin = (AnjutaFileLoaderPlugin*) plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupLoader", "ActionPopupOpen");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupLoader",
								   "ActionPopupOpenWith");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	
	if (fl_plugin->fm_current_uri)
		g_free (fl_plugin->fm_current_uri);
	fl_plugin->fm_current_uri = g_strdup (uri);
	
	mime_type = get_uri_mime_type (uri);
	menu = gtk_menu_new ();
	
	idx = 0;
	
	/* Open with plugins menu items */
	plugin_descs = get_available_plugins_for_mime (fl_plugin, mime_type);
	snode = plugin_descs;
	while (snode)
	{
		gchar *name;
		AnjutaPluginDescription *desc;
		
		desc = (AnjutaPluginDescription *)(snode->data);
		name = NULL;
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Name", &name);
		if (!name)
		{
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &name);
		}
		menuitem = gtk_menu_item_new_with_label (name);
		g_object_set_data (G_OBJECT (menuitem), "index", (gpointer)(idx));
		g_object_set_data (G_OBJECT (menuitem), "desc", (gpointer)(desc));
		g_signal_connect (G_OBJECT (menuitem), "activate",
						  G_CALLBACK (fm_open_with), plugin);
		gtk_menu_append (menu, menuitem);
		g_free (name);
		snode = g_slist_next (snode);
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
		g_object_set_data (G_OBJECT (menuitem), "index", (gpointer)(idx));
		g_signal_connect (G_OBJECT (menuitem), "activate",
						  G_CALLBACK (fm_open_with), plugin);
		gtk_menu_append (menu, menuitem);
		node = g_list_next (node);
		idx++;
	}
	gtk_widget_show_all (menu);
	parentmenu =
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
					"/PopupFileManager/PlaceholderPopupFileOpen/OpenWith");
	g_assert (GTK_IS_MENU_ITEM (parentmenu));
	if (parentmenu)
	{
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (parentmenu), menu);
		if (mime_apps == NULL && plugin_descs == NULL)
		{
			g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		}
	}
	gnome_vfs_mime_application_list_free (mime_apps);
	if (plugin_descs)
		g_slist_free (plugin_descs);
}

static void
value_removed_fm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	AnjutaFileLoaderPlugin *fl_plugin;

	fl_plugin = (AnjutaFileLoaderPlugin*)plugin;
	
	if (fl_plugin->fm_current_uri)
		g_free (fl_plugin->fm_current_uri);
	fl_plugin->fm_current_uri = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupLoader", "ActionPopupOpen");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupLoader",
								   "ActionPopupOpenWith");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
dnd_dropped (const gchar *uri, gpointer plugin)
{
	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin), uri, FALSE, NULL);
}

static void
on_session_load (AnjutaShell *shell, GQueue *commandline_queue,
				 AnjutaFileLoaderPlugin *plugin)
{
	const gchar *uri;
	while ((uri = g_queue_pop_head (commandline_queue)))
	{
		ianjuta_file_loader_load (IANJUTA_FILE_LOADER (plugin), uri, FALSE, NULL);
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkAction *action;
	AnjutaUI *ui;
	AnjutaFileLoaderPlugin *loader_plugin;
	// GtkWidget *recent_menu;
	// EggRecentViewGtk *recent_view;
	GtkActionGroup *group;
	
	loader_plugin = (AnjutaFileLoaderPlugin*)plugin;
	
	DEBUG_PRINT ("AnjutaFileLoaderPlugin: Activating File Loader plugin ...");
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add action group */
	loader_plugin->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupLoader",
											_("File Loader"),
											actions_file,
											G_N_ELEMENTS (actions_file),
											plugin);
	action = anjuta_ui_get_action (ui, "ActionGroupLoader", "ActionFileNew");
	g_object_set (G_OBJECT (action), "short-label", _("New"), NULL);
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
								N_("Open recent files"), group);
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
	
	loader_plugin = (AnjutaFileLoaderPlugin*)plugin;
	
	DEBUG_PRINT ("AnjutaFileLoaderPlugin: Deactivating File Loader plugin ...");
	
	/* Disconnect session */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_load), plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, loader_plugin->fm_watch_id, TRUE);
	/* Uninstall dnd */
	dnd_drop_finalize (GTK_WIDGET (plugin->shell), plugin);
	/* Remove UI */
	anjuta_ui_unmerge (ui, loader_plugin->uiid);
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, loader_plugin->action_group);
	anjuta_ui_remove_action_group (ui, loader_plugin->recent_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	AnjutaFileLoaderPlugin *plugin = (AnjutaFileLoaderPlugin*)obj;
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
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_file_loader_plugin_instance_init (GObject *obj)
{
	AnjutaFileLoaderPlugin *plugin = (AnjutaFileLoaderPlugin*)obj;
	plugin->fm_current_uri = NULL;
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
}

static GObject*
iloader_load (IAnjutaFileLoader *loader, const gchar *uri,
			  gboolean read_only, GError **err)
{
	gchar *mime_type;
	gchar *new_uri;
	GnomeVFSURI* vfs_uri;
	AnjutaStatus *status;
	GSList *plugin_descs = NULL;
	GObject *plugin = NULL;	
	
	g_return_val_if_fail (uri != NULL, NULL);
	vfs_uri = gnome_vfs_uri_new (uri);
	new_uri = gnome_vfs_uri_to_string (vfs_uri,
									   GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER);
	mime_type = get_uri_mime_type (new_uri);
	gnome_vfs_uri_unref (vfs_uri);
	
	if (mime_type == NULL)
	{
		launch_application_failure ((AnjutaFileLoaderPlugin*)(ANJUTA_PLUGIN (loader)),
									new_uri, GNOME_VFS_ERROR_NOT_FOUND);
		g_free (new_uri);
		return NULL;
	}
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (loader)->shell, NULL);
	anjuta_status_busy_push (status);
	
	DEBUG_PRINT ("Opening URI: %s", uri);
	
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
		g_free (location);
	}
	else
	{
		launch_in_default_application ((AnjutaFileLoaderPlugin*)loader, mime_type, uri);
	}
	if (plugin)
		ianjuta_file_open (IANJUTA_FILE(plugin), uri, NULL);
	
	set_recent_file ((AnjutaFileLoaderPlugin*)loader, new_uri, mime_type);
	
	if (plugin_descs)
		g_slist_free (plugin_descs);
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

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2008 Ignacio Casal Quinteiro

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
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include <gtk/gtk.h>

#include "plugin.h"

static gpointer parent_class;

#define STARTER_VBOX "starter_vbox"
#define RECENT_VBOX "recent_vbox"
#define IMPORT_IMAGE "import_image"
#define NEW_IMAGE "new_image"

#define NEW_PROJECT_PIXMAP "anjuta-project-wizard-plugin-48.png"
#define IMPORT_PROJECT_PIXMAP "anjuta-project-import-plugin-48.png"

#define PROJECT_WIZARD_ID "anjuta-project-wizard:NPWPlugin"
#define PROJECT_IMPORT_ID "anjuta-project-import:AnjutaProjectImportPlugin"

#define RECENT_LIMIT 3

void
on_new_project_clicked (GtkButton *button, gpointer user_data);
void
on_import_project_clicked (GtkButton *button, gpointer user_data);

static void
on_recent_project_clicked (GtkButton *button, StarterPlugin *plugin)
{
	GFile *file;
	IAnjutaFileLoader *loader = 
		anjuta_shell_get_interface (anjuta_plugin_get_shell (ANJUTA_PLUGIN (plugin)),
		                            IAnjutaFileLoader,
		                            NULL);
	
	file = g_object_get_data (G_OBJECT (button), "file");

	ianjuta_file_loader_load (IANJUTA_FILE_LOADER (loader),
							  file, FALSE, NULL);
}

void
on_new_project_clicked (GtkButton *button, gpointer user_data)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN (user_data);
	AnjutaPluginManager* plugin_manager = 
		anjuta_shell_get_plugin_manager (anjuta_plugin_get_shell (plugin),
		                                 NULL);
	
	GObject* wizard = 
		anjuta_plugin_manager_get_plugin_by_id (plugin_manager, PROJECT_WIZARD_ID);

	if (wizard)
		ianjuta_wizard_activate (IANJUTA_WIZARD (wizard), NULL);
}

void
on_import_project_clicked (GtkButton *button, gpointer user_data)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN (user_data);
	AnjutaPluginManager* plugin_manager = 
		anjuta_shell_get_plugin_manager (anjuta_plugin_get_shell (plugin),
		                                 NULL);
	GObject* wizard = 
		anjuta_plugin_manager_get_plugin_by_id (plugin_manager, PROJECT_IMPORT_ID);

	if (wizard)
		ianjuta_wizard_activate (IANJUTA_WIZARD (wizard), NULL);
}

static void
build_recent_projects (GtkWidget *box, StarterPlugin* plugin)
{
	GtkRecentManager *manager;
	GList *items, *list;
	gint i = 0;
	
	manager = gtk_recent_manager_get_default ();

	items = gtk_recent_manager_get_items (manager);
	items = g_list_reverse (items);

	list = items;
	while (i < RECENT_LIMIT && list != NULL)
	{
		if (strcmp (gtk_recent_info_get_mime_type (list->data), "application/x-anjuta") == 0)
		{
			GtkWidget *button;
			GtkWidget *label;
			GtkWidget *button_box;
			GFile *file;
			GFileInfo* info;
			gchar *uri;
			GIcon* icon;

			button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
			button = gtk_button_new ();
			gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
			gtk_widget_set_halign (button, GTK_ALIGN_START);

			label = gtk_label_new (gtk_recent_info_get_display_name (list->data));
			gtk_box_pack_end (GTK_BOX (button_box), label, FALSE, FALSE, 0);
			
			file = g_file_new_for_uri (gtk_recent_info_get_uri (list->data));

			if (!g_file_query_exists (file, NULL))
			{
				list = g_list_next (list);
				continue;
			}

			info = g_file_query_info (file,
			                          G_FILE_ATTRIBUTE_STANDARD_ICON,
			                          G_FILE_QUERY_INFO_NONE,
			                          NULL, NULL);
			if (info)
			{
				icon = g_file_info_get_icon (info);
				if (icon)
				{
					GtkWidget* image = gtk_image_new_from_gicon (icon,
						                                         GTK_ICON_SIZE_BUTTON);
					gtk_box_pack_start (GTK_BOX (button_box), image, FALSE, FALSE, 0);
				}
				g_object_unref (info);
			}
			gtk_container_add (GTK_CONTAINER (button), button_box);
			gtk_widget_show_all (button);
			
			gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

			g_object_set_data_full (G_OBJECT (button), "file", file,
			                        (GDestroyNotify)g_object_unref);
			uri = gtk_recent_info_get_uri_display (list->data);
			if (uri)
			{
				gchar *tip;

				tip = g_strdup_printf (_("Open '%s'"), uri);
				gtk_widget_set_tooltip_text (button, tip);

				g_free (tip);
				g_free (uri);
			}

			g_signal_connect (button, "clicked",
							  G_CALLBACK (on_recent_project_clicked),
							  plugin);
			i++;
		}
		list = g_list_next (list);
	}

	g_list_free_full(items, (GDestroyNotify)gtk_recent_info_unref);
}

static GtkWidget*
create_starter_widget (StarterPlugin* plugin)
{
	GError* error = NULL;
	GtkWidget* starter_vbox;
	GtkWidget* recent_vbox;
	GtkBuilder* builder = gtk_builder_new ();
	GtkWidget* image;
	
	if (!gtk_builder_add_from_resource (builder,
	                                    "/org/gnome/anjuta/ui/starter.ui",
	                                    &error))
	{
		DEBUG_PRINT ("Could load starter ui! %s", error->message);
		g_error_free (error);
		return NULL;
	}
	starter_vbox = GTK_WIDGET (gtk_builder_get_object (builder, STARTER_VBOX));
	recent_vbox = GTK_WIDGET (gtk_builder_get_object (builder, RECENT_VBOX));

	build_recent_projects (recent_vbox, plugin);

	g_object_ref (starter_vbox);
	gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (starter_vbox)),
	                      starter_vbox);

	image = GTK_WIDGET (gtk_builder_get_object (builder, NEW_IMAGE));
	gtk_image_set_from_file (GTK_IMAGE (image),
	                         PACKAGE_PIXMAPS_DIR"/"NEW_PROJECT_PIXMAP);
	image = GTK_WIDGET (gtk_builder_get_object (builder, IMPORT_IMAGE));
	gtk_image_set_from_file (GTK_IMAGE (image),
	                         PACKAGE_PIXMAPS_DIR"/"IMPORT_PROJECT_PIXMAP);	
	
	gtk_builder_connect_signals (builder, plugin);

	return starter_vbox;
}

/* Remove the starter plugin once a document was opened */
static void
on_value_added_current_project (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	GObject* project = g_value_get_object (value);
	AnjutaShell* shell = ANJUTA_PLUGIN(plugin)->shell;
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);
	
	if (project)
	{
		if (splugin->starter)
		{
			DEBUG_PRINT ("Hiding starter");
			anjuta_shell_remove_widget (shell, splugin->starter, NULL);
		}
		splugin->starter = NULL;
	}
}


/* Remove the starter plugin once a document was opened */
static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	GObject* doc = g_value_get_object (value);
	AnjutaShell* shell = ANJUTA_PLUGIN(plugin)->shell;
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);
	
	if (doc)
	{
		if (splugin->starter)
		{
			DEBUG_PRINT ("Hiding starter");
			anjuta_shell_remove_widget (shell, splugin->starter, NULL);
		}
		splugin->starter = NULL;
	}
}

static void
on_value_removed (AnjutaPlugin *plugin, const gchar *name,
                  gpointer data)
{
	AnjutaShell* shell = anjuta_plugin_get_shell (plugin);
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);
	IAnjutaDocumentManager* docman = anjuta_shell_get_interface (shell,
	                                                             IAnjutaDocumentManager,
	                                                             NULL);
	IAnjutaProjectManager* pm = anjuta_shell_get_interface (shell,
	                                                        IAnjutaProjectManager,
	                                                        NULL);
	
	if (!(docman && ianjuta_document_manager_get_doc_widgets (docman, NULL)) &&
	    !(pm && ianjuta_project_manager_get_current_project (pm, NULL)))
	{
		DEBUG_PRINT ("Showing starter");
		splugin->starter = create_starter_widget (splugin);
		anjuta_shell_add_widget (shell, splugin->starter,
		                         "AnjutaStarter",
		                         _("Start"),
		                         GTK_STOCK_ABOUT,
		                         ANJUTA_SHELL_PLACEMENT_CENTER,
		                         NULL);
		anjuta_shell_present_widget (shell, splugin->starter, NULL);
		g_object_unref (splugin->starter);
	}
}

static void
on_session_load (AnjutaShell* shell,
                 AnjutaSessionPhase phase,
                 AnjutaSession* session,
                 StarterPlugin* plugin)
{
	if (phase == ANJUTA_SESSION_PHASE_END)
	{
		if (!plugin->starter)
			on_value_removed (ANJUTA_PLUGIN (plugin), NULL, plugin);
		if (plugin->starter)
		{
			anjuta_shell_maximize_widget (shell,
			                              "AnjutaStarter",
			                              NULL);
		}
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);

	DEBUG_PRINT ("StarterPlugin: Activating document manager plugin...");
	
	splugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 on_value_added_current_editor,
								 on_value_removed,
								 NULL);
	splugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin,
								  IANJUTA_PROJECT_MANAGER_CURRENT_PROJECT,
								 on_value_added_current_project,
								 on_value_removed,
								 NULL);
	on_value_removed (plugin, NULL, splugin);

	g_signal_connect (anjuta_plugin_get_shell (plugin),
	                  "load-session",
	                  G_CALLBACK (on_session_load),
	                  plugin);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);
	DEBUG_PRINT ("StarterPlugin: Deactivating starter plugin...");
	if (splugin->starter)
		anjuta_shell_remove_widget (anjuta_plugin_get_shell (plugin),
		                            splugin->starter, NULL);

	anjuta_plugin_remove_watch (plugin, splugin->editor_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, splugin->project_watch_id, FALSE);	
	
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
starter_plugin_instance_init (GObject *obj)
{
}

static void
starter_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

ANJUTA_PLUGIN_BOILERPLATE (StarterPlugin, starter_plugin);
ANJUTA_SIMPLE_PLUGIN (StarterPlugin, starter_plugin);

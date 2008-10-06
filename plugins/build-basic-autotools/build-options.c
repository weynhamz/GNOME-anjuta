/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    build-options.c
    Copyright (C) 2008 Sébastien Granjoux

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

#include "build-options.h"

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <glade/glade-xml.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-shell.h>
#include <string.h>

/* Constants
 *---------------------------------------------------------------------------*/

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-build-basic-autotools-plugin.glade"

#define CONFIGURE_DIALOG "configure_dialog"
#define RUN_AUTOGEN_CHECK "force_autogen_check"
#define CONFIGURATION_COMBO "configuration_combo_entry"
#define BUILD_DIR_CHOOSER "build_dir_chooser"
#define CONFIGURE_ARGS_ENTRY "configure_args_entry"
#define OK_BUTTON "ok_button"

#define GTK_FILE_CHOOSER_CREATE_DIRECTORY_QUARK (build_gtk_file_chooser_create_directory_get_quark ())


/* Type defintions
 *---------------------------------------------------------------------------*/

typedef struct _BuildConfigureDialog BuildConfigureDialog;

struct _BuildConfigureDialog
{
 	GtkWidget *win;
	
	GtkWidget *combo;
	GtkWidget *autogen;
	GtkWidget *build_dir_chooser;
	GtkWidget *args;
	GtkWidget *ok;
	
	BuildConfigurationList *config_list;
	
	const gchar *project_uri;
};

typedef struct _BuildMissingDirectory BuildMissingDirectory;

struct _BuildMissingDirectory
{
	gsize exist;
	gchar uri[1];
};


/* Create directory at run time for GtkFileChooserButton
 *---------------------------------------------------------------------------*/

/* Create a directories, including parents if necessary, return 
 * */

static GFile*
build_make_directories (GFile *file,
							   GCancellable *cancellable,
							   GError **error)
{
	GError *path_error = NULL;
	GList *children = NULL;
	GFile *parent;
	
	parent = g_file_get_parent(file);

	for (;;)
	{
		if (g_file_make_directory (file, NULL, &path_error))
		{
			/* Making child directory succeed */
			if (children == NULL)
			{
				/* All directories have been created */
				return parent;
			}
			else
			{
				/* Get next child directory */
				g_object_unref (file);
				file = (GFile *)children->data;
				children = g_list_delete_link (children, children);
			}
		}
		else if (path_error->code == G_IO_ERROR_NOT_FOUND)
		{
			g_clear_error (&path_error);
			children = g_list_prepend (children, file);
			file = parent;
			parent = g_file_get_parent(file);
		}
		else
		{
			g_object_unref (parent);
			g_propagate_error (error, path_error);
			
			return NULL;
		}
	}				
}

static GQuark
build_gtk_file_chooser_create_directory_get_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("gtk-file-chooser-create-directory");
  
  return quark;
}

/* Remove created directory */
static void
on_build_missing_directory_destroyed (BuildMissingDirectory* dir)
{
	/* Remove previously created directories */
	GFile* created_dir;
	GFile* existing_dir;
		
	created_dir = g_file_new_for_uri (dir->uri);
	dir->uri[dir->exist] = '\0';
	existing_dir = g_file_new_for_uri (dir->uri);

	for (;!g_file_equal (created_dir, existing_dir);)
	{
		GFile *parent_dir;
			
		if (!g_file_delete (created_dir, NULL, NULL)) break;
		parent_dir = g_file_get_parent (created_dir);
		g_object_unref (created_dir);
		created_dir = parent_dir;
	}
	g_object_unref (created_dir);
	g_object_unref (existing_dir);
	g_free (dir);
}

/* If the folder is missing created it before setting it */

static gboolean
build_gtk_file_chooser_create_and_set_uri (GtkFileChooser *chooser, const gchar *uri)
{
	GFile *dir;
	GError *error = NULL;
	GFile *parent;
	
	dir = g_file_new_for_uri (uri);
	parent = build_make_directories (dir, NULL, &error);
	if (parent != NULL)
	{
		BuildMissingDirectory* dir;
		gchar *last;
		gsize len;

		len = strlen (uri);
		dir = (BuildMissingDirectory *)g_new (char, sizeof (BuildMissingDirectory) + len);
		
		memcpy (dir->uri, uri, len + 1);
		last = g_file_get_uri (parent);
		dir->exist = strlen (last);
		
		g_object_set_qdata_full (G_OBJECT (chooser),
								 GTK_FILE_CHOOSER_CREATE_DIRECTORY_QUARK,
								 dir,
								 (GDestroyNotify)on_build_missing_directory_destroyed);		
	}
	else
	{
		g_object_set_qdata (G_OBJECT (chooser), 
							GTK_FILE_CHOOSER_CREATE_DIRECTORY_QUARK,
							NULL);
		g_error_free (error);
	}
	g_object_unref (dir);

	gtk_file_chooser_set_current_folder_uri (chooser, uri);
	return gtk_file_chooser_set_uri (chooser, uri);
}

/* Do not delete the automatically created folder */
static void
build_gtk_file_chooser_keep_folder (GtkFileChooser *chooser, const char *uri)
{
	BuildMissingDirectory* dir;

	dir = g_object_steal_qdata (G_OBJECT (chooser), GTK_FILE_CHOOSER_CREATE_DIRECTORY_QUARK);	
	if (dir != NULL)
	{
		GFile *created_dir;
		GFile *needed_dir;
	
		needed_dir = g_file_new_for_uri (uri);
		created_dir = g_file_new_for_uri (dir->uri);
		if (!g_file_equal (created_dir, needed_dir))
		{
			/* Need to delete created directory */
			on_build_missing_directory_destroyed (dir);
		}
		else
		{
			g_free (dir);
		}
		g_object_unref (created_dir);
		g_object_unref (needed_dir);
	}
}


/* Helper functions
 *---------------------------------------------------------------------------*/

static void
on_select_configuration (GtkComboBox *widget, gpointer user_data)
{
	BuildConfigureDialog *dlg = (BuildConfigureDialog *)user_data;
	gchar *name;
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dlg->combo), &iter))
	{
		gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (dlg->combo)), &iter, 1, &name, -1);			
	}
	else
	{
		name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dlg->combo));
	}
	
	if (*name == '\0')
	{
		/* Configuration name is mandatory disable Ok button */
		gtk_widget_set_sensitive (dlg->ok, FALSE);
	}
	else
	{
		BuildConfiguration *cfg;
		gchar *uri;
		
		gtk_widget_set_sensitive (dlg->ok, TRUE);
	
		cfg = build_configuration_list_select (dlg->config_list, name);
		
		if (cfg != NULL)
		{
			const gchar *args;

			args = build_configuration_get_args (cfg); 
			gtk_entry_set_text (GTK_ENTRY (dlg->args), args == NULL ? "" : args);
		
			uri = build_configuration_list_get_build_uri (dlg->config_list, cfg);
			build_gtk_file_chooser_create_and_set_uri (GTK_FILE_CHOOSER (dlg->build_dir_chooser), uri);
			g_free (uri);
		}
	}
	g_free (name);
}

static void 
fill_dialog (BuildConfigureDialog *dlg)
{
	GtkListStore* store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	BuildConfiguration *cfg;

	gtk_combo_box_set_model (GTK_COMBO_BOX(dlg->combo), GTK_TREE_MODEL(store));
	gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (dlg->combo), 0);
	
	for (cfg = build_configuration_list_get_first (dlg->config_list); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		GtkTreeIter iter;
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, build_configuration_get_translated_name (cfg), 1, build_configuration_get_name (cfg), -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (dlg->combo),
							  build_configuration_list_get_position (dlg->config_list,
																	 build_configuration_list_get_selected (dlg->config_list)));
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
build_dialog_configure (GtkWindow* parent, const gchar *project_root_uri, BuildConfigurationList *config_list, gboolean *run_autogen)
{
	GladeXML* gxml;
	BuildConfigureDialog dlg;
	BuildConfiguration *cfg = NULL;
	gint response;
	
	/* Get all dialog widgets */
	gxml = glade_xml_new (GLADE_FILE, CONFIGURE_DIALOG, NULL);
	dlg.win = glade_xml_get_widget (gxml, CONFIGURE_DIALOG);
	dlg.combo = glade_xml_get_widget(gxml, CONFIGURATION_COMBO);
	dlg.autogen = glade_xml_get_widget(gxml, RUN_AUTOGEN_CHECK);
	dlg.build_dir_chooser = glade_xml_get_widget(gxml, BUILD_DIR_CHOOSER);
	dlg.args = glade_xml_get_widget(gxml, CONFIGURE_ARGS_ENTRY);
	dlg.ok = glade_xml_get_widget(gxml, OK_BUTTON);
	g_object_unref (gxml);
	
	dlg.config_list = config_list;
	dlg.project_uri = project_root_uri;

	/* Set run autogen option */	
	if (*run_autogen) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg.autogen), TRUE);

	g_signal_connect (dlg.combo, "changed", G_CALLBACK (on_select_configuration), &dlg);
	
	fill_dialog(&dlg);
	
	response = gtk_dialog_run (GTK_DIALOG (dlg.win));
	
	if (response == GTK_RESPONSE_OK)
	{
		gchar *name;
		gchar *uri;
		const gchar *args;
		GtkTreeIter iter;

		*run_autogen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg.autogen));
		
		if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dlg.combo), &iter))
		{
			gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (dlg.combo)), &iter, 1, &name, -1);			
		}
		else
		{
			name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dlg.combo));
		}
		cfg = build_configuration_list_create (config_list, name);
		g_free (name);
		
		args = gtk_entry_get_text (GTK_ENTRY (dlg.args));
		build_configuration_set_args (cfg, args);
		
		uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dlg.build_dir_chooser));
		build_configuration_list_set_build_uri (dlg.config_list, cfg, uri);
		build_gtk_file_chooser_keep_folder (GTK_FILE_CHOOSER (dlg.build_dir_chooser), uri);
		g_free (uri);
	}
	gtk_widget_destroy (GTK_WIDGET(dlg.win));

	return cfg != NULL;
}


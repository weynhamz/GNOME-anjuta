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
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-environment-editor.h>
#include <string.h>

/* Constants
 *---------------------------------------------------------------------------*/

#define BUILDER_FILE PACKAGE_DATA_DIR "/glade/anjuta-build-basic-autotools-plugin.ui"

#define CONFIGURE_DIALOG "configure_dialog"
#define RUN_AUTOGEN_CHECK "force_autogen_check"
#define CONFIGURATION_COMBO "configuration_combo_entry"
#define BUILD_DIR_BUTTON "build_dir_button"
#define BUILD_DIR_LABEL "build_dir_label"
#define CONFIGURE_ARGS_ENTRY "configure_args_entry"
#define ENVIRONMENT_EDITOR "environment_editor"
#define OK_BUTTON "ok_button"


/* Type defintions
 *---------------------------------------------------------------------------*/

typedef struct _BuildConfigureDialog BuildConfigureDialog;

struct _BuildConfigureDialog
{
 	GtkWidget *win;

	GtkWidget *combo;
	GtkWidget *autogen;
	GtkWidget *build_dir_button;
	GtkWidget *build_dir_label;
	GtkWidget *args;
	GtkWidget *env_editor;
	GtkWidget *ok;

	
	BuildConfigurationList *config_list;

	const gchar *project_uri;
	GFile *build_file;
};

/* Create directory at run time for GtkFileChooserButton
 *---------------------------------------------------------------------------*/

static void
on_build_dir_button_clicked (GtkButton *button, gpointer user_data)
{
	BuildConfigureDialog *dlg = (BuildConfigureDialog *)user_data;
	GtkWidget *chooser;
	GFile *file = NULL;

	chooser = gtk_file_chooser_dialog_new (_("Select a build directory inside the project directory"),
	                                       GTK_WINDOW (dlg->win),
	                                       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	                                       NULL);

	if (dlg->build_file != NULL)
	{
		if (g_file_make_directory_with_parents (dlg->build_file, NULL, NULL)) file = g_object_ref (dlg->build_file);
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser), dlg->build_file, NULL);
	}
	else
	{
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (chooser), dlg->project_uri);
	}

	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *basename;

		if (dlg->build_file) g_object_unref (dlg->build_file);
		dlg->build_file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
		
		basename = g_file_get_basename (dlg->build_file);
		gtk_label_set_text (GTK_LABEL (dlg->build_dir_label), basename);
		g_free (basename);
	}
	if (file != NULL)
	{
		while ((file != NULL) && g_file_delete (file, NULL, NULL))
		{
			GFile *child = file;
			file = g_file_get_parent (child);
			g_object_unref (child);
		}
		g_object_unref (file);
	}
	gtk_widget_destroy (chooser);
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
		GtkWidget* entry = gtk_bin_get_child (GTK_BIN (dlg->combo));
		name = g_strdup(gtk_entry_get_text (GTK_ENTRY (entry)));
	}

	if (*name == '\0')
	{
		/* Configuration name is mandatory disable Ok button */
		gtk_widget_set_sensitive (dlg->ok, FALSE);
	}
	else
	{
		BuildConfiguration *cfg;

		gtk_widget_set_sensitive (dlg->ok, TRUE);

		cfg = build_configuration_list_get (dlg->config_list, name);

		if (cfg != NULL)
		{
			const gchar *args;
			GList *item;
			gchar *basename;

			args = build_configuration_get_args (cfg);
			gtk_entry_set_text (GTK_ENTRY (dlg->args), args == NULL ? "" : args);

			if (dlg->build_file != NULL) g_object_unref (dlg->build_file);
			dlg->build_file = build_configuration_list_get_build_file (dlg->config_list, cfg);
			basename = g_file_get_basename (dlg->build_file);
			gtk_label_set_text (GTK_LABEL (dlg->build_dir_label), basename);
			g_free (basename);

			anjuta_environment_editor_reset (ANJUTA_ENVIRONMENT_EDITOR (dlg->env_editor));
			for (item = build_configuration_get_variables (cfg); item != NULL; item = g_list_next (item))
			{
				anjuta_environment_editor_set_variable (ANJUTA_ENVIRONMENT_EDITOR (dlg->env_editor), (gchar *)item->data);
			}
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
	gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (dlg->combo), 0);

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
	GtkBuilder* bxml;
	BuildConfigureDialog dlg;
	BuildConfiguration *cfg = NULL;
	gint response;

	/* Get all dialog widgets */
	bxml = anjuta_util_builder_new (BUILDER_FILE, NULL);
	if (bxml == NULL) return FALSE;
	anjuta_util_builder_get_objects (bxml,
	    CONFIGURE_DIALOG, &dlg.win,
	    CONFIGURATION_COMBO, &dlg.combo,
	    RUN_AUTOGEN_CHECK, &dlg.autogen,
	    BUILD_DIR_BUTTON, &dlg.build_dir_button,
	    BUILD_DIR_LABEL, &dlg.build_dir_label,
	    CONFIGURE_ARGS_ENTRY, &dlg.args,
	    ENVIRONMENT_EDITOR, &dlg.env_editor,
	    OK_BUTTON, &dlg.ok,
	    NULL);
	g_object_unref (bxml);

	dlg.config_list = config_list;
	dlg.project_uri = project_root_uri;
	dlg.build_file = NULL;

	/* Set run autogen option */
	if (*run_autogen) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg.autogen), TRUE);

	g_signal_connect (dlg.combo, "changed", G_CALLBACK (on_select_configuration), &dlg);
	g_signal_connect (dlg.build_dir_button, "clicked", G_CALLBACK (on_build_dir_button_clicked), &dlg);

	fill_dialog(&dlg);

	response = gtk_dialog_run (GTK_DIALOG (dlg.win));

	if (response == GTK_RESPONSE_OK)
	{
		gchar *name;
		const gchar *args;
		GtkTreeIter iter;
		gchar **mod_var;

		*run_autogen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg.autogen));

		if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dlg.combo), &iter))
		{
			gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (dlg.combo)), &iter, 1, &name, -1);
		}
		else
		{
			GtkWidget* entry = gtk_bin_get_child (GTK_BIN (dlg.combo));
			name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
		}
		cfg = build_configuration_list_create (config_list, name);
		g_free (name);

		args = gtk_entry_get_text (GTK_ENTRY (dlg.args));
		build_configuration_set_args (cfg, args);

		if (dlg.build_file != NULL)
		{
			gchar *uri = g_file_get_uri (dlg.build_file);
			build_configuration_list_set_build_uri (dlg.config_list, cfg, uri);
			g_free (uri);
		}

		build_configuration_clear_variables (cfg);
		mod_var = anjuta_environment_editor_get_modified_variables (ANJUTA_ENVIRONMENT_EDITOR (dlg.env_editor));
		if ((mod_var != NULL) && (*mod_var != NULL))
		{
			gchar **var;
			/* Invert list */
			for (var = mod_var; *var != NULL; var++);
			do
			{
				var--;
				build_configuration_set_variable (cfg, *var);
			}
			while (var != mod_var);
		}
		g_strfreev (mod_var);
	}
	if (dlg.build_file != NULL) g_object_unref (dlg.build_file);
	gtk_widget_destroy (GTK_WIDGET(dlg.win));
	
	return cfg != NULL;
}


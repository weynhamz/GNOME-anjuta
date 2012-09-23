/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    parameters.c
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

/*
 * Display and run program parameters dialog
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "parameters.h"

#include "utils.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-environment-editor.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

/*---------------------------------------------------------------------------*/

#define UI_FILE PACKAGE_DATA_DIR "/ui/anjuta-run-program.xml"
#define BUILDER_FILE PACKAGE_DATA_DIR "/glade/anjuta-run-program.ui"

#define PARAMETERS_DIALOG "parameters_dialog"
#define TERMINAL_CHECK_BUTTON "parameter_run_in_term_check"
#define PARAMETER_COMBO "parameter_combo"
#define TARGET_COMBO "target_combo"
#define ENVIRONMENT_EDITOR "environment_editor"
#define DIR_CHOOSER "working_dir_chooser"
#define TARGET_BUTTON "target_button"

/* Type defintions
 *---------------------------------------------------------------------------*/

typedef struct _RunDialog RunDialog;

struct _RunDialog
{
 	GtkWidget *win;

	/* Child widgets */
	GtkToggleButton *term;
	GtkComboBox *args;
	GtkComboBox *target;
	GtkFileChooser *dirs;
	AnjutaEnvironmentEditor *vars;

	/* Plugin */
	RunProgramPlugin *plugin;
};

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
on_add_string_in_model (gpointer data, gpointer user_data)
{
	GtkListStore* model = (GtkListStore *)user_data;
	GtkTreeIter iter;

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, (const gchar *)data, -1);
}

static void
on_add_file_in_model (gpointer data, gpointer user_data)
{
	GtkListStore* model = (GtkListStore *)user_data;
	GtkTreeIter iter;
	gchar *local;

	local = g_file_get_path ((GFile *)data);
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, local, -1);
	g_free (local);
}

static void
on_add_directory_in_chooser (gpointer data, gpointer user_data)
{
	GtkFileChooser* chooser = (GtkFileChooser *)user_data;
	gchar *local;

	local = g_file_get_path ((GFile *)data);
	gtk_file_chooser_add_shortcut_folder (chooser, local, NULL);
	g_free (local);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static gint
compare_file (GFile *file_a, GFile *file_b)
{
	return g_file_equal (file_a, file_b) ? 0 : 1;
}

static void
save_dialog_data (RunDialog* dlg)
{
	gchar *arg;
	const gchar *filename;
	GFile *file;
	GList *find;
	RunProgramPlugin *plugin = dlg->plugin;

	/* Save arguments */
	arg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dlg->args)))));
	arg = g_strstrip (arg);
	if (arg != NULL)
	{
		/* Remove empty string in list, allow it only as first item */
		if ((plugin->recent_args != NULL) &&
			(*(gchar *)(plugin->recent_args->data) == '\0')) plugin->recent_args = g_list_delete_link (plugin->recent_args, plugin->recent_args);
		find = g_list_find_custom(plugin->recent_args, arg, (GCompareFunc)strcmp);
		if (find)
		{
			g_free (find->data);
			plugin->recent_args = g_list_delete_link (plugin->recent_args, find);
		}
		plugin->recent_args = g_list_prepend (plugin->recent_args, arg);
	}

	/* Save target */
	filename = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dlg->target))));
	if ((filename != NULL) && (*filename != '\0'))
	{
		file = g_file_new_for_path (filename);
		find = g_list_find_custom (plugin->recent_target, file, (GCompareFunc)compare_file);
		if (find)
		{
			g_object_unref (G_OBJECT (find->data));
			plugin->recent_target = g_list_delete_link (plugin->recent_target, find);
		}
		plugin->recent_target = g_list_prepend (plugin->recent_target, file);
	}

	/* Save working dir */
	file = gtk_file_chooser_get_file (dlg->dirs);
	if (file != NULL)
	{
		find = g_list_find_custom(plugin->recent_dirs, file, (GCompareFunc)compare_file);
		if (find)
		{
			g_object_unref (G_OBJECT (find->data));
			plugin->recent_dirs = g_list_delete_link (plugin->recent_dirs, find);
		}
		plugin->recent_dirs = g_list_prepend (plugin->recent_dirs, file);
	}

	/* Save all environment variables */
	g_strfreev (plugin->environment_vars);
	plugin->environment_vars = anjuta_environment_editor_get_modified_variables (dlg->vars);

	plugin->run_in_terminal = gtk_toggle_button_get_active (dlg->term);

	run_plugin_update_shell_value (plugin);
}

static void
on_select_target (RunDialog* dlg)
{
	GtkWidget *sel_dlg = gtk_file_chooser_dialog_new (
			_("Load Target to run"), GTK_WINDOW (dlg->win),
			 GTK_FILE_CHOOSER_ACTION_OPEN,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(sel_dlg), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (sel_dlg), TRUE);

	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (sel_dlg), filter);

	if (gtk_dialog_run (GTK_DIALOG (sel_dlg)) == GTK_RESPONSE_ACCEPT)
	{
		gchar* filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sel_dlg));

		gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dlg->target))), filename);
		g_free (filename);
	}
	gtk_widget_destroy (GTK_WIDGET (sel_dlg));
}

static RunDialog*
run_dialog_init (RunDialog *dlg, RunProgramPlugin *plugin)
{
	GtkBuilder *bxml;
	GtkWindow *parent;
	GtkWidget *child;
	GtkTreeModel* model;
	GObject *button;
	GValue value = {0,};
	const gchar *project_root_uri;
	GError* error = NULL;
	gchar **variable;

	parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	bxml = gtk_builder_new ();

	if (!gtk_builder_add_from_file (bxml, BUILDER_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	dlg->plugin = plugin;

	dlg->win = GTK_WIDGET (gtk_builder_get_object (bxml, PARAMETERS_DIALOG));
	dlg->term = GTK_TOGGLE_BUTTON (gtk_builder_get_object (bxml, TERMINAL_CHECK_BUTTON));
	dlg->args = GTK_COMBO_BOX (gtk_builder_get_object (bxml, PARAMETER_COMBO));
	dlg->target = GTK_COMBO_BOX (gtk_builder_get_object (bxml, TARGET_COMBO));
	dlg->vars = ANJUTA_ENVIRONMENT_EDITOR (gtk_builder_get_object (bxml, ENVIRONMENT_EDITOR));
	dlg->dirs = GTK_FILE_CHOOSER (gtk_builder_get_object (bxml, DIR_CHOOSER));

	/* Connect signals */
	button = gtk_builder_get_object (bxml, TARGET_BUTTON);
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (on_select_target), dlg);

	g_object_unref (bxml);

	/* Fill parameter combo box */
	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (dlg->args, model);
	gtk_combo_box_set_entry_text_column( GTK_COMBO_BOX(dlg->args), 0);
	g_list_foreach (plugin->recent_args, on_add_string_in_model, model);
	if (plugin->recent_args != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dlg->args))), (const char *)plugin->recent_args->data);
	}
	g_object_unref (model);

	/* Fill working directory list */
	g_list_foreach (plugin->recent_dirs, on_add_directory_in_chooser, dlg->dirs);
	if (plugin->recent_dirs != NULL) gtk_file_chooser_set_file (dlg->dirs, (GFile *)plugin->recent_dirs->data, NULL);

	/* Fill target combo box */
	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (dlg->target, model);
	gtk_combo_box_set_entry_text_column( GTK_COMBO_BOX (dlg->target), 0);
	g_list_foreach (plugin->recent_target, on_add_file_in_model, model);

    anjuta_shell_get_value (ANJUTA_PLUGIN (plugin)->shell, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI, &value, NULL);
	project_root_uri = G_VALUE_HOLDS_STRING (&value) ? g_value_get_string (&value) : NULL;
	if (project_root_uri != NULL)
	{
		/* One project loaded, get all executable target */
		IAnjutaProjectManager *pm;
		GList *exec_targets = NULL;

		pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
		if (pm != NULL)
		{
			exec_targets = ianjuta_project_manager_get_elements (pm,
							 ANJUTA_PROJECT_EXECUTABLE,
							 NULL);
		}
		if (exec_targets != NULL)
		{
			GList *node;

			for (node = exec_targets; node; node = g_list_next (node))
			{
				GList *target;
				GFile *file = (GFile *)node->data;
				for (target = plugin->recent_target; target; target = g_list_next (target))
				{
					if (g_file_equal ((GFile *)target->data, file)) break;
				}
				if (target == NULL)
				{
					on_add_file_in_model (file, model);
				}
				g_object_unref (G_OBJECT (file));
			}
			g_list_free (exec_targets);
		}

		/* Set a default value for current dir, allowing to find glade file in
		 * new project created by the wizard without any change */
		if (plugin->recent_dirs == NULL) gtk_file_chooser_set_uri (dlg->dirs, project_root_uri);
	}

	child = gtk_bin_get_child (GTK_BIN (dlg->target));

	if (plugin->recent_target != NULL)
	{
		gchar *local;

		local = g_file_get_path ((GFile *)plugin->recent_target->data);
		gtk_entry_set_text (GTK_ENTRY (child), local);
		g_free (local);
	}
	else
	{
		GtkTreeIter iter;

		if (gtk_tree_model_get_iter_first (model, &iter) &&
			!gtk_tree_model_iter_next (model, &iter))
		{
			gchar *default_target;

			gtk_tree_model_get_iter_first (model, &iter);
			gtk_tree_model_get (model, &iter,
								0, &default_target,
								-1);
			gtk_entry_set_text (GTK_ENTRY (child), default_target);
			g_free (default_target);
		}
	}
	g_object_unref (model);

	/* Set stored user modified environment variables */
	if (plugin->environment_vars)
	{
		for (variable = plugin->environment_vars; *variable; ++variable)
			anjuta_environment_editor_set_variable (dlg->vars, *variable);
	}

	/* Set terminal option */
	if (plugin->run_in_terminal) gtk_toggle_button_set_active (dlg->term, TRUE);

	gtk_window_set_transient_for (GTK_WINDOW (dlg->win), parent);

	return dlg;
}

static gint
run_parameters_dialog_or_try_execute (RunProgramPlugin *plugin, gboolean try_run)
{
	RunDialog dlg;
	gint response;

	run_dialog_init (&dlg, plugin);
	const char *target = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dlg.target))));

	if (try_run && target && *target)
	{
		save_dialog_data (&dlg);
		return GTK_RESPONSE_APPLY;
	}
	else
	{
		response = gtk_dialog_run (GTK_DIALOG (dlg.win));
		if (response == GTK_RESPONSE_APPLY)
		{
			save_dialog_data (&dlg);
		}
		gtk_widget_destroy (dlg.win);
	}

	return response;
}

/* Public functions
 *---------------------------------------------------------------------------*/

gint
run_parameters_dialog_run (RunProgramPlugin *plugin)
{
	return run_parameters_dialog_or_try_execute (plugin, FALSE);
}

gint
run_parameters_dialog_or_execute (RunProgramPlugin *plugin)
{
	return run_parameters_dialog_or_try_execute (plugin, TRUE);
}

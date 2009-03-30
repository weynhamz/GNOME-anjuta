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
#include <libanjuta/interfaces/ianjuta-project-manager.h>

/*---------------------------------------------------------------------------*/

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-run-program.ui"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-run-program.glade"

#define PARAMETERS_DIALOG "parameters_dialog"
#define TERMINAL_CHECK_BUTTON "parameter_run_in_term_check"
#define PARAMETER_COMBO "parameter_combo"
#define TARGET_COMBO "target_combo"
#define VAR_TREEVIEW "environment_treeview"
#define DIR_CHOOSER "working_dir_chooser"
#define TARGET_BUTTON "target_button"
#define ADD_VAR_BUTTON "add_button"
#define REMOVE_VAR_BUTTON "remove_button"
#define EDIT_VAR_BUTTON "edit_button"

enum {
	ENV_NAME_COLUMN = 0,
	ENV_VALUE_COLUMN,
	ENV_DEFAULT_VALUE_COLUMN,
	ENV_COLOR_COLUMN,
	ENV_N_COLUMNS
};

#define ENV_USER_COLOR	"black"
#define ENV_DEFAULT_COLOR "gray"

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
	GtkTreeView *vars;
	GtkTreeModel* model;
	GtkWidget *remove_button;
	GtkWidget *edit_button;
		
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
	gtk_list_store_set (model, &iter, ENV_NAME_COLUMN, (const gchar *)data, -1);
}

static void
on_add_uri_in_model (gpointer data, gpointer user_data)
{
	GtkListStore* model = (GtkListStore *)user_data;
	GtkTreeIter iter;
	gchar *local;
	
	local = anjuta_util_get_local_path_from_uri ((const char *)data);
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, ENV_NAME_COLUMN, local, -1);
	g_free (local);
}

static void
on_add_directory_in_chooser (gpointer data, gpointer user_data)
{
	GtkFileChooser* chooser = (GtkFileChooser *)user_data;
	gchar *local;

	local = anjuta_util_get_local_path_from_uri ((const char *)data);
	gtk_file_chooser_add_shortcut_folder (chooser, (const gchar *)local, NULL);
	g_free (local);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
load_environment_variables (RunProgramPlugin *plugin, GtkListStore *store)
{
	GtkTreeIter iter;
	gchar **var;
	gchar **list;
	
	/* Load current environment variables */
	list = g_listenv();
	var = list;
	if (var)
	{
		for (; *var != NULL; var++)
		{
			const gchar *value = g_getenv (*var);
			gtk_list_store_prepend (store, &iter);
			gtk_list_store_set (store, &iter,
								ENV_NAME_COLUMN, *var,
								ENV_VALUE_COLUMN, value,
								ENV_DEFAULT_VALUE_COLUMN, value,
								ENV_COLOR_COLUMN, ENV_DEFAULT_COLOR,
								-1);
		}
	}
	g_strfreev (list);
	
	/* Load user environment variables */
	var = plugin->environment_vars;
	if (var)
	{
		for (; *var != NULL; var++)
		{
			gchar ** value;
			
			value = g_strsplit (*var, "=", 2);
			if (value)
			{
				if (run_plugin_gtk_tree_model_find_string (GTK_TREE_MODEL (store), 
													NULL, &iter, ENV_NAME_COLUMN,
													value[0]))
				{
					gtk_list_store_set (store, &iter,
									ENV_VALUE_COLUMN, value[1],
									ENV_COLOR_COLUMN, ENV_USER_COLOR,
									-1);
				}
				else
				{
					gtk_list_store_prepend (store, &iter);
					gtk_list_store_set (store, &iter,
										ENV_NAME_COLUMN, value[0],
										ENV_VALUE_COLUMN, value[1],
										ENV_DEFAULT_VALUE_COLUMN, NULL,
										ENV_COLOR_COLUMN, ENV_USER_COLOR,
										-1);
				}
				g_strfreev (value);
			}
		}
	}
}

static void
save_environment_variables (RunProgramPlugin *plugin, GtkTreeModel *model)
{
	gchar **vars;
	gboolean valid;
	GtkTreeIter iter;
	
	/* Remove previous variables */
	g_strfreev (plugin->environment_vars);
	
	/* Allocated a too big array: able to save all environment variables
	 * while we need to save only variables modified by user but it
	 * shouldn't be that big anyway and checking exactly which variable
	 * need to be saved will take more time */
	vars = g_new (gchar *,
							gtk_tree_model_iter_n_children (GTK_TREE_MODEL (model), NULL) + 1);
	plugin->environment_vars = vars;
	
	for (valid = gtk_tree_model_get_iter_first (model, &iter); valid; valid = gtk_tree_model_iter_next (model, &iter))
	{
		gchar *name;
		gchar *value;
		gchar *color;
		
		gtk_tree_model_get (model, &iter,
							ENV_NAME_COLUMN, &name,
							ENV_VALUE_COLUMN, &value,
							ENV_COLOR_COLUMN, &color,
							-1);
		
		/* Save only variables modified by user */
		if (strcmp(color, ENV_USER_COLOR) == 0)
		{
			*vars = g_strconcat(name, "=", value, NULL);
			vars++;
		}
		g_free (name);
		g_free (value);
		g_free (color);
	}
	*vars = NULL;
}

static void
save_dialog_data (RunDialog* dlg)
{
	const gchar *arg;
	const gchar *filename;
	gchar *uri;
	GList *find;
	GtkTreeModel* model;
	RunProgramPlugin *plugin = dlg->plugin;

	/* Save arguments */
	arg = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (dlg->args)->child));
	if (arg != NULL)
	{
		find = g_list_find_custom(plugin->recent_args, arg, (GCompareFunc)strcmp);
		if (find)
		{
			g_free (find->data);
			plugin->recent_args = g_list_delete_link (plugin->recent_args, find);
		}
		plugin->recent_args = g_list_prepend (plugin->recent_args, g_strdup (arg));
	}	

	/* Save target */
	filename = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (dlg->target)->child));
	if ((filename != NULL) && (*filename != '\0'))
	{
		GFile *file;

		file = g_file_new_for_path (filename);
		uri = g_file_get_uri (file);
		g_object_unref (file);

		if (uri != NULL)
		{
			find = g_list_find_custom (plugin->recent_target, uri, (GCompareFunc)strcmp);
			if (find)
			{
				g_free (find->data);
				plugin->recent_target = g_list_delete_link (plugin->recent_target, find);
			}
			plugin->recent_target = g_list_prepend (plugin->recent_target, uri);
		}
	}

	/* Save working dir */
	uri = gtk_file_chooser_get_uri (dlg->dirs);
	if (uri != NULL)
	{
		find = g_list_find_custom(plugin->recent_dirs, uri, (GCompareFunc)strcmp);
		if (find)
		{
			g_free (find->data);
			plugin->recent_dirs = g_list_delete_link (plugin->recent_dirs, find);
		}
		plugin->recent_dirs = g_list_prepend (plugin->recent_dirs, uri);
	}
			
	/* Save all environment variables */
	model = gtk_tree_view_get_model (dlg->vars);
	save_environment_variables (plugin, model);
			
	plugin->run_in_terminal = gtk_toggle_button_get_active (dlg->term);

	run_plugin_update_shell_value (plugin);
}

static void
on_select_target (RunDialog* dlg)
{
	GtkWidget *sel_dlg = gtk_file_chooser_dialog_new (
			_("Load Target to run"), GTK_WINDOW (dlg->win),
			 GTK_FILE_CHOOSER_ACTION_OPEN,
			 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(sel_dlg), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (sel_dlg), TRUE);

	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (sel_dlg), filter);

	if (gtk_dialog_run (GTK_DIALOG (sel_dlg)) == GTK_RESPONSE_ACCEPT)
	{
		gchar* filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sel_dlg));

		gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dlg->target)->child), filename);
		g_free (filename);
	}		
	gtk_widget_destroy (GTK_WIDGET (sel_dlg));
}

static void
on_environment_selection_changed (GtkTreeSelection *selection, RunDialog *dlg)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean selected;

	if (selection == NULL)
	{
		selection = gtk_tree_view_get_selection (dlg->vars);
	}
	
	selected = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (selected)
	{
		gchar *color;
		gchar *value;
		gboolean restore;
		
		gtk_tree_model_get (model, &iter,
							ENV_DEFAULT_VALUE_COLUMN, &value,
							ENV_COLOR_COLUMN, &color,
							-1);
		
		restore = (strcmp (color, ENV_USER_COLOR) == 0) && (value != NULL);
		gtk_button_set_label (GTK_BUTTON (dlg->remove_button), restore ? GTK_STOCK_REVERT_TO_SAVED : GTK_STOCK_DELETE);
		g_free (color);
		g_free (value);
	}
	gtk_widget_set_sensitive (dlg->remove_button, selected);
	gtk_widget_set_sensitive (dlg->edit_button, selected);
}

static void
on_environment_add_button (GtkButton *button, GtkTreeView *view)
{
	GtkTreeIter iter;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkTreePath *path;
	GtkTreeSelection* sel;
		
	model = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	
	sel = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (sel, NULL, &iter))
	{
		GtkTreeIter niter;
		gtk_list_store_insert_after	(model, &niter, &iter);
		iter = niter;
	}
	else
	{
		gtk_list_store_prepend (model, &iter);
	}
	
	gtk_list_store_set (model, &iter, ENV_NAME_COLUMN, "",
								ENV_VALUE_COLUMN, "",
								ENV_DEFAULT_VALUE_COLUMN, NULL,
								ENV_COLOR_COLUMN, ENV_USER_COLOR,
								-1);
		
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
	column = gtk_tree_view_get_column (view, ENV_NAME_COLUMN);
	gtk_tree_view_scroll_to_cell (view, path, column, FALSE, 0, 0);
	gtk_tree_view_set_cursor (view, path, column ,TRUE);
	gtk_tree_path_free (path);
}

static void
on_environment_edit_button (GtkButton *button, GtkTreeView *view)
{
	GtkTreeIter iter;
	GtkTreeSelection* sel;

	sel = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (sel, NULL, &iter))
	{
		GtkListStore *model;
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		
		model = GTK_LIST_STORE (gtk_tree_view_get_model (view));
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
		column = gtk_tree_view_get_column (view, ENV_VALUE_COLUMN);
		gtk_tree_view_scroll_to_cell (view, path, column, FALSE, 0, 0);
		gtk_tree_view_set_cursor (view, path, column ,TRUE);
		gtk_tree_path_free (path);
	}
}

static void
on_environment_remove_button (GtkButton *button, RunDialog *dlg)
{
	GtkTreeIter iter;
	GtkTreeSelection* sel;
	GtkTreeView *view = dlg->vars;

	sel = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (sel, NULL, &iter))
	{
		GtkListStore *model;
		GtkTreeViewColumn *column;
		GtkTreePath *path;
		gchar *color;
		
		model = GTK_LIST_STORE (gtk_tree_view_get_model (view));

		/* Display variable */
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
		column = gtk_tree_view_get_column (view, ENV_NAME_COLUMN);
		gtk_tree_view_scroll_to_cell (view, path, column, FALSE, 0, 0);
		gtk_tree_path_free (path);
		
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
							ENV_COLOR_COLUMN, &color,
							-1);
		if (strcmp(color, ENV_USER_COLOR) == 0)
		{
			/* Remove an user variable */
			gchar *value;
			
			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
								ENV_DEFAULT_VALUE_COLUMN, &value,
								-1);
			
			if (value != NULL)
			{
				/* Restore default environment variable */
				gtk_list_store_set (model, &iter, ENV_VALUE_COLUMN, value,
									ENV_COLOR_COLUMN, ENV_DEFAULT_COLOR,
									-1);
			}
			else
			{
				gtk_list_store_remove (model, &iter);
			}
			g_free (value);
		}				
		else
		{
			/* Replace value with an empty one */
			gtk_list_store_set (model, &iter, ENV_VALUE_COLUMN, NULL,
								ENV_COLOR_COLUMN, ENV_USER_COLOR,
								-1);
		}
		on_environment_selection_changed (sel, dlg);
	}
}

static void
on_environment_variable_edited (GtkCellRendererText *cell,
						  gchar *path,
                          gchar *text,
                          RunDialog *dlg)
{
	GtkTreeIter iter;
	GtkTreeIter niter;
	GtkListStore *model;
	gboolean valid;
	GtkTreeView *view = dlg->vars;
	
	text = g_strstrip (text);
	
	model = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	
	valid = gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (model), &iter, path);
	if (valid)
	{
		gchar *name;
		gchar *value;
		gchar *def_value;
			
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
							ENV_NAME_COLUMN, &name,
							ENV_VALUE_COLUMN, &value,
							ENV_DEFAULT_VALUE_COLUMN, &def_value,
							-1);
		
		if (strcmp(name, text) != 0)
		{
			GtkTreeSelection* sel;
			GtkTreePath *path;
			GtkTreeViewColumn *column;
			
			if (def_value != NULL)
			{
				/* Remove current variable */
				gtk_list_store_set (model, &iter, ENV_VALUE_COLUMN, NULL,
									ENV_COLOR_COLUMN, ENV_USER_COLOR,
									-1);
			}
			
			/* Search variable with new name */
			if (run_plugin_gtk_tree_model_find_string (GTK_TREE_MODEL (model), 
												NULL, &niter, ENV_NAME_COLUMN,
												text))
			{
					if (def_value == NULL)
					{
						gtk_list_store_remove (model, &iter);
					}
					gtk_list_store_set (model, &niter, 
										ENV_VALUE_COLUMN, value,
										ENV_COLOR_COLUMN, ENV_USER_COLOR,
										-1);
			}
			else
			{
				if (def_value != NULL)
				{
					gtk_list_store_insert_after	(model, &niter, &iter);
					gtk_list_store_set (model, &niter, ENV_NAME_COLUMN, text,
										ENV_VALUE_COLUMN, value,
										ENV_DEFAULT_VALUE_COLUMN, NULL,
										ENV_COLOR_COLUMN, ENV_USER_COLOR,
										-1);
				}
				else
				{
					gtk_list_store_set (model, &iter, ENV_NAME_COLUMN, text,
										-1);
					niter = iter;
				}
			}
			sel = gtk_tree_view_get_selection (view);
			gtk_tree_selection_select_iter (sel, &niter);
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &niter);
			column = gtk_tree_view_get_column (view, ENV_VALUE_COLUMN);
			gtk_tree_view_scroll_to_cell (view, path, column, FALSE, 0, 0);
			gtk_tree_view_set_cursor (view, path, column ,TRUE);
			gtk_tree_path_free (path);
		}
		g_free (name);
		g_free (def_value);
		g_free (value);
	}
}

static void
on_environment_value_edited (GtkCellRendererText *cell,
						  gchar *path,
                          gchar *text,
                          RunDialog *dlg)
{
	GtkTreeIter iter;
	GtkListStore *model;
	GtkTreeView *view = dlg->vars;

	text = g_strstrip (text);
	
	model = GTK_LIST_STORE (gtk_tree_view_get_model (view));	
	if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (model), &iter, path))
	{
		gtk_list_store_set (model, &iter, ENV_VALUE_COLUMN, text, 
										ENV_COLOR_COLUMN, ENV_USER_COLOR,
										-1);
		on_environment_selection_changed (NULL, dlg);
	}
}

static RunDialog*
run_dialog_init (RunDialog *dlg, RunProgramPlugin *plugin)
{
	GladeXML *gxml;
	GtkWindow *parent;
	GtkCellRenderer *renderer;	
	GtkTreeModel* model;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GObject *button;
	GValue value = {0,};
	const gchar *project_root_uri;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	gxml = glade_xml_new (GLADE_FILE, PARAMETERS_DIALOG, NULL);
	if (gxml == NULL)
	{
		anjuta_util_dialog_error(parent, _("Missing file %s"), GLADE_FILE);
		return NULL;
	}
	
	dlg->plugin = plugin;
		
	dlg->win = glade_xml_get_widget (gxml, PARAMETERS_DIALOG);
	dlg->term = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, TERMINAL_CHECK_BUTTON));
	dlg->args = GTK_COMBO_BOX (glade_xml_get_widget (gxml, PARAMETER_COMBO));
	dlg->target = GTK_COMBO_BOX (glade_xml_get_widget (gxml, TARGET_COMBO));
	dlg->vars = GTK_TREE_VIEW (glade_xml_get_widget (gxml, VAR_TREEVIEW));
	dlg->dirs = GTK_FILE_CHOOSER (glade_xml_get_widget (gxml, DIR_CHOOSER));
	dlg->remove_button = glade_xml_get_widget (gxml, REMOVE_VAR_BUTTON);
	dlg->edit_button = glade_xml_get_widget (gxml, EDIT_VAR_BUTTON);

	/* Connect signals */	
	button = G_OBJECT (glade_xml_get_widget (gxml, TARGET_BUTTON));
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (on_select_target), dlg);
	button = G_OBJECT (glade_xml_get_widget (gxml, ADD_VAR_BUTTON));
	g_signal_connect (button, "clicked", G_CALLBACK (on_environment_add_button), dlg->vars);
	button = G_OBJECT (glade_xml_get_widget (gxml, EDIT_VAR_BUTTON));
	g_signal_connect (button, "clicked", G_CALLBACK (on_environment_edit_button), dlg->vars);
	button = G_OBJECT (glade_xml_get_widget (gxml, REMOVE_VAR_BUTTON));
	g_signal_connect (button, "clicked", G_CALLBACK (on_environment_remove_button), dlg);
	selection = gtk_tree_view_get_selection (dlg->vars);
	g_signal_connect (selection, "changed", G_CALLBACK (on_environment_selection_changed), dlg);
	
	g_object_unref (gxml);

	/* Fill parameter combo box */
	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (dlg->args, model);
	gtk_combo_box_entry_set_text_column( GTK_COMBO_BOX_ENTRY(dlg->args), 0);
	g_list_foreach (plugin->recent_args, on_add_string_in_model, model);
	if (plugin->recent_args != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dlg->args)->child), (const char *)plugin->recent_args->data);
	}
	g_object_unref (model);

	/* Fill working directory list */
	g_list_foreach (plugin->recent_dirs, on_add_directory_in_chooser, dlg->dirs);
	if (plugin->recent_dirs != NULL) gtk_file_chooser_set_uri (dlg->dirs, (const char *)plugin->recent_dirs->data);
	
	/* Fill target combo box */
	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (dlg->target, model);
	gtk_combo_box_entry_set_text_column( GTK_COMBO_BOX_ENTRY(dlg->target), 0);
	g_list_foreach (plugin->recent_target, on_add_uri_in_model, model);

    anjuta_shell_get_value (ANJUTA_PLUGIN (plugin)->shell, "project_root_uri", &value, NULL);
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
			exec_targets = ianjuta_project_manager_get_targets (pm,
							 IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE,
							 NULL);
		}
		if (exec_targets != NULL)
		{
			GList *node;

			for (node = exec_targets; node; node = g_list_next (node))
			{
				GList *target;
				for (target = plugin->recent_target; target; target = g_list_next (target))
				{
					if (strcmp ((const gchar *)target->data, node->data) == 0) break;
				}
				if (target == NULL)
				{
					on_add_uri_in_model (node->data, model);
				}
				g_free (node->data);
			}
			g_list_free (exec_targets);
		}

		/* Set a default value for current dir, allowing to find glade file in
		 * new project created by the wizard without any change */
		if (plugin->recent_dirs == NULL) gtk_file_chooser_set_uri (dlg->dirs, project_root_uri);
	}
	
	if (plugin->recent_target != NULL)
	{
		gchar *local;
		
		local = anjuta_util_get_local_path_from_uri ((const char *)plugin->recent_target->data);
		gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dlg->target)->child), local);
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
			gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dlg->target)->child), default_target);
			g_free (default_target);
		}
	}
	g_object_unref (model);
	
	/* Fill environment variable list */
	model = GTK_TREE_MODEL (gtk_list_store_new (ENV_N_COLUMNS,
												G_TYPE_STRING,
												G_TYPE_STRING,
												G_TYPE_STRING,
												G_TYPE_STRING,
												G_TYPE_BOOLEAN));
	gtk_tree_view_set_model (dlg->vars, model);
	load_environment_variables (plugin, GTK_LIST_STORE (model));
	g_object_unref (model);
	
	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect(renderer, "edited", (GCallback) on_environment_variable_edited, dlg);
	g_object_set(renderer, "editable", TRUE, NULL);	
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
													   "text", ENV_NAME_COLUMN,
													   "foreground", ENV_COLOR_COLUMN,
													   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (dlg->vars, column);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", (GCallback) on_environment_value_edited, dlg);
	column = gtk_tree_view_column_new_with_attributes (_("Value"), renderer,
													   "text", ENV_VALUE_COLUMN,
													   "foreground", ENV_COLOR_COLUMN,
													   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (dlg->vars, column);
	
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
	const char *target = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (dlg.target)->child));
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

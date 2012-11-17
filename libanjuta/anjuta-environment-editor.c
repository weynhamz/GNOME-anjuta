/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-environment-editor.c
 * Copyright (C) 2011 Sebastien Granjoux
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "anjuta-environment-editor.h"
#include <glib/gi18n.h>

enum
{
	CHANGED,
	LAST_SIGNAL
};

enum {
	ENV_NAME_COLUMN = 0,
	ENV_VALUE_COLUMN,
	ENV_DEFAULT_VALUE_COLUMN,
	ENV_COLOR_COLUMN,
	ENV_N_COLUMNS
};

#define ENV_USER_COLOR	"black"
#define ENV_DEFAULT_COLOR "gray"


struct _AnjutaEnvironmentEditor
{
	GtkBin parent;

	gchar **variables;

	GtkTreeModel *model;
	
	GtkTreeView *treeview;
	GtkWidget *edit_button;
	GtkWidget *remove_button;
};


G_DEFINE_TYPE (AnjutaEnvironmentEditor, anjuta_environment_editor, GTK_TYPE_BIN);


/* Helpers functions
 *---------------------------------------------------------------------------*/

#include <string.h>

static gboolean
anjuta_gtk_tree_model_find_string (GtkTreeModel *model, GtkTreeIter *parent,
		GtkTreeIter *iter, guint col, const gchar *value)

{
	gboolean valid;
	gboolean found = FALSE;

	g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	if (!parent)
	{
		valid = gtk_tree_model_get_iter_first (model, iter);
	}
	else 
	{
		valid = gtk_tree_model_iter_children (model, iter, parent);
	}

	while (valid)
	{
		gchar *mvalue;
		
		gtk_tree_model_get (model, iter, col, &mvalue, -1);
		found = (mvalue != NULL) && (strcmp (mvalue, value) == 0);
		g_free (mvalue);
		if (found) break;						   

		if (gtk_tree_model_iter_has_child (model, iter))
		{
			GtkTreeIter citer;
			
			found = anjuta_gtk_tree_model_find_string (model, iter,
														   &citer, col, value);
			if (found)
			{
				*iter = citer;
				break;
			}
		}
		valid = gtk_tree_model_iter_next (model, iter);
	}

	return found;
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
load_environment_variables (AnjutaEnvironmentEditor *editor, GtkListStore *store)
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
	var = editor->variables;
	if (var)
	{
		for (; *var != NULL; var++)
		{
			gchar ** value;
			
			value = g_strsplit (*var, "=", 2);
			if (value)
			{
				if (anjuta_gtk_tree_model_find_string (GTK_TREE_MODEL (store), 
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


/* Implement GtkWidget functions
 *---------------------------------------------------------------------------*/

static void
anjuta_environment_editor_size_allocate (GtkWidget     *widget,
                                         GtkAllocation *allocation)
{
	GtkBin *bin;
	GtkWidget *child;

	bin = GTK_BIN (widget);
	child = gtk_bin_get_child (bin);
	if ((child != NULL) && gtk_widget_get_visible (child))
	{
		gtk_widget_set_allocation (widget, allocation);
		gtk_widget_size_allocate (child, allocation);
	}
}

static void
anjuta_environment_editor_get_preferred_width (GtkWidget *widget,
                               gint      *minimum_size,
                               gint      *natural_size)
{
	GtkBin *bin;
	GtkWidget *child;

	bin = GTK_BIN (widget);
	child = gtk_bin_get_child (bin);
	gtk_widget_get_preferred_width (child, minimum_size, natural_size);
}

static void
anjuta_environment_editor_get_preferred_height (GtkWidget *widget,
                                gint      *minimum_size,
                                gint      *natural_size)
{
	GtkBin *bin;
	GtkWidget *child;

	bin = GTK_BIN (widget);
	child = gtk_bin_get_child (bin);
	gtk_widget_get_preferred_height (child, minimum_size, natural_size);
}

static void
on_environment_selection_changed (GtkTreeSelection *selection, AnjutaEnvironmentEditor *editor)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean selected;

	if (selection == NULL)
	{
		selection = gtk_tree_view_get_selection (editor->treeview);
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
		gtk_button_set_label (GTK_BUTTON (editor->remove_button), restore ? GTK_STOCK_REVERT_TO_SAVED : GTK_STOCK_DELETE);
		g_free (color);
		g_free (value);
	}
	gtk_widget_set_sensitive (editor->remove_button, selected);
	gtk_widget_set_sensitive (editor->edit_button, selected);
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
on_environment_remove_button (GtkButton *button, AnjutaEnvironmentEditor *editor)
{
	GtkTreeIter iter;
	GtkTreeSelection* sel;
	GtkTreeView *view = editor->treeview;

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
		on_environment_selection_changed (sel, editor);
	}
}

static gboolean
move_to_environment_value (gpointer data)
{
	GtkTreeView *view = GTK_TREE_VIEW (data);
	GtkTreeSelection* sel;
	GtkTreeModel *model;
	GtkTreeIter iter;	
	GtkTreeViewColumn *column;

	sel = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		GtkTreePath *path;
		
		path = gtk_tree_model_get_path (model, &iter);
		column = gtk_tree_view_get_column (view, ENV_VALUE_COLUMN);
		gtk_tree_view_set_cursor (view, path, column, TRUE);
		gtk_tree_path_free (path);
	}

	return FALSE;
}

static void
on_environment_variable_edited (GtkCellRendererText *cell,
						  gchar *path,
                          gchar *text,
                          AnjutaEnvironmentEditor *editor)
{
	GtkTreeIter iter;
	GtkTreeIter niter;
	GtkListStore *model;
	gboolean valid;
	GtkTreeView *view = editor->treeview;
	
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
			
			if (def_value != NULL)
			{
				/* Remove current variable */
				gtk_list_store_set (model, &iter, ENV_VALUE_COLUMN, NULL,
									ENV_COLOR_COLUMN, ENV_USER_COLOR,
									-1);
			}
			
			/* Search variable with new name */
			if (anjuta_gtk_tree_model_find_string (GTK_TREE_MODEL (model), 
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
				}
			}
			g_idle_add (move_to_environment_value, view);
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
                          AnjutaEnvironmentEditor *editor)
{
	GtkTreeIter iter;
	GtkListStore *model;
	GtkTreeView *view = editor->treeview;

	text = g_strstrip (text);
	
	model = GTK_LIST_STORE (gtk_tree_view_get_model (view));	
	if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (model), &iter, path))
	{
		gtk_list_store_set (model, &iter, ENV_VALUE_COLUMN, text, 
										ENV_COLOR_COLUMN, ENV_USER_COLOR,
										-1);
		on_environment_selection_changed (NULL, editor);
	}
}

/* Implement GtkWidget functions
 *---------------------------------------------------------------------------*/

static void
anjuta_environment_editor_dispose (GObject *object)
{
	AnjutaEnvironmentEditor* editor = ANJUTA_ENVIRONMENT_EDITOR (object);

	if (editor->model != NULL) g_object_unref (editor->model);

	G_OBJECT_CLASS (anjuta_environment_editor_parent_class)->dispose (object);
}

static void
anjuta_environment_editor_finalize (GObject *object)
{
	AnjutaEnvironmentEditor* editor = ANJUTA_ENVIRONMENT_EDITOR (object);
	
	g_strfreev (editor->variables);
	editor->variables = NULL;

	G_OBJECT_CLASS (anjuta_environment_editor_parent_class)->finalize (object);
}

static void
anjuta_environment_editor_init (AnjutaEnvironmentEditor *editor)
{
	GtkWidget *expander;
	GtkWidget *hbox;
	GtkWidget *scrolled;
	GtkWidget *treeview;
	GtkWidget *vbutton;
	GtkWidget *add_button;
	GtkWidget *edit_button;
	GtkWidget *remove_button;
	GtkTreeModel *model;
	GtkTreeViewColumn* column;
	GtkCellRenderer* renderer;
	GtkTreeSelection *selection;

	editor->variables = NULL;
	
	gtk_widget_set_has_window (GTK_WIDGET (editor), FALSE);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (editor), FALSE);

	/* Create all needed widgets */
	expander = gtk_expander_new (_("Environment Variables:"));
	gtk_container_add (GTK_CONTAINER (editor), expander);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);
	gtk_container_add (GTK_CONTAINER (expander), hbox);
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_OUT);
 	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	gtk_box_pack_start (GTK_BOX (hbox), scrolled, TRUE, TRUE, 0);
	treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolled), treeview);
	editor->treeview = GTK_TREE_VIEW (treeview);

	vbutton = gtk_button_box_new (GTK_ORIENTATION_VERTICAL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (vbutton), GTK_BUTTONBOX_START);
	gtk_box_set_spacing (GTK_BOX (vbutton), 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbutton, FALSE, FALSE, 0);
	add_button = gtk_button_new_from_stock (GTK_STOCK_NEW);
	gtk_container_add (GTK_CONTAINER (vbutton), add_button);
	edit_button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_container_add (GTK_CONTAINER (vbutton), edit_button);
	editor->edit_button = edit_button;
	remove_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
	gtk_container_add (GTK_CONTAINER (vbutton), remove_button);
	editor->remove_button = remove_button;
	
	gtk_widget_show_all (GTK_WIDGET (editor));

	/* Fill environment variable list */
	model = GTK_TREE_MODEL (gtk_list_store_new (ENV_N_COLUMNS,
												G_TYPE_STRING,
												G_TYPE_STRING,
												G_TYPE_STRING,
												G_TYPE_STRING,
												G_TYPE_BOOLEAN));
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	load_environment_variables (editor, GTK_LIST_STORE (model));
	editor->model = g_object_ref (model);
	
	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect(renderer, "edited", (GCallback) on_environment_variable_edited, editor);
	g_object_set(renderer, "editable", TRUE, NULL);	
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
													   "text", ENV_NAME_COLUMN,
													   "foreground", ENV_COLOR_COLUMN,
													   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", (GCallback) on_environment_value_edited, editor);
	column = gtk_tree_view_column_new_with_attributes (_("Value"), renderer,
													   "text", ENV_VALUE_COLUMN,
													   "foreground", ENV_COLOR_COLUMN,
													   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Connect signals */	
	g_signal_connect (G_OBJECT (add_button), "clicked", G_CALLBACK (on_environment_add_button), editor->treeview);
	g_signal_connect (G_OBJECT (edit_button), "clicked", G_CALLBACK (on_environment_edit_button), editor->treeview);
	g_signal_connect (G_OBJECT (remove_button), "clicked", G_CALLBACK (on_environment_remove_button), editor);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (selection, "changed", G_CALLBACK (on_environment_selection_changed), editor);

	on_environment_selection_changed (NULL, editor);
}

static void
anjuta_environment_editor_class_init (AnjutaEnvironmentEditorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

 	
	/**
	 * AnjutaEnvironmentEditor::changed:
	 * @widget: the AnjutaEnvironmentEditor that received the signal
	 *
	 * The ::changed signal is emitted when an environment variable
	 * is changed (include deleted variables)
	 */
	g_signal_new ("changed",
	              G_OBJECT_CLASS_TYPE (klass),
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (AnjutaEnvironmentEditorClass, changed),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE, 0);

	object_class->dispose = anjuta_environment_editor_dispose;
	object_class->finalize = anjuta_environment_editor_finalize;
	
	widget_class->size_allocate = anjuta_environment_editor_size_allocate;
	widget_class->get_preferred_width = anjuta_environment_editor_get_preferred_width;
	widget_class->get_preferred_height = anjuta_environment_editor_get_preferred_height;
}

/* Public functions
 *---------------------------------------------------------------------------*/

/*
 * anjuta_environment_editor_new:
 * 
 * Returns: A new AnjutaEnvironmentEditor widget
 */
GtkWidget*
anjuta_environment_editor_new (void)
{
	return GTK_WIDGET (g_object_new (ANJUTA_TYPE_ENVIRONMENT_EDITOR, NULL));
}

/*
 * anjuta_environment_editor_set_variable:
 * @editor: A AnjutaEnvironmentEditor widget
 * @variable: Environment variable name and value
 *
 * Set environment variable.
 */
void
anjuta_environment_editor_set_variable (AnjutaEnvironmentEditor *editor, const gchar *variable)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean valid;
	gchar *name;
	const gchar *equal;
	guint len;
	
	model = editor->model;

	/* Check is variable is already existing */
	equal = strchr (variable, '=');
	len = equal != NULL ? equal - variable : 0;
		
	for (valid = gtk_tree_model_get_iter_first (model, &iter); valid; valid = gtk_tree_model_iter_next (model, &iter))
	{
		gtk_tree_model_get (editor->model, &iter,
							ENV_NAME_COLUMN, &name,
							-1);
		if (((len == 0) && (strcmp (name, variable) == 0)) ||
		    ((len != 0) && (strncmp (name, variable, len) == 0) && (name[len] == '\0')))
		{
			break;
		}
		g_free (name);
		name = NULL;
	}

	if (!valid) gtk_list_store_insert_after (GTK_LIST_STORE (model), &iter, NULL);

	if (name == NULL)
	{
		name = g_strdup (variable);
		if (len != 0) name[len] = '\0';
	}
	if (equal != NULL)
	{
		equal++;
	}
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, ENV_NAME_COLUMN, name,
								ENV_VALUE_COLUMN, equal,
								ENV_COLOR_COLUMN, ENV_USER_COLOR,
								-1);
	g_free (name);
}

/*
 * anjuta_environment_editor_get_all_variables:
 * @editor: A AnjutaEnvironmentEditor widget
 *
 * Returns: A list of all environment variables in a string array.
 */
gchar** 
anjuta_environment_editor_get_all_variables (AnjutaEnvironmentEditor *editor)
{
	gchar **all_var;
	gchar *var;
	gboolean valid;
	GtkTreeIter iter;
	
	all_var = g_new (gchar *, gtk_tree_model_iter_n_children (editor->model, NULL) + 1);
	var = *all_var;
	
	for (valid = gtk_tree_model_get_iter_first (editor->model, &iter); valid; valid = gtk_tree_model_iter_next (editor->model, &iter))
	{
		gchar *name;
		gchar *value;
		gchar *color;
		
		gtk_tree_model_get (editor->model, &iter,
							ENV_NAME_COLUMN, &name,
							ENV_VALUE_COLUMN, &value,
							ENV_COLOR_COLUMN, &color,
							-1);
		
		var = g_strconcat(name, "=", value, NULL);
		var++;
		g_free (name);
		g_free (value);
		g_free (color);
	}
	var = NULL;

	return all_var;
}

/*
 * anjuta_environment_editor_get_modified_variables:
 * @editor: A AnjutaEnvironmentEditor widget
 *
 * Returns: A list of modified environment variables in a string array.
 */
gchar**
anjuta_environment_editor_get_modified_variables (AnjutaEnvironmentEditor *editor)
{
	gchar **mod_var;
	gchar **var;
	gboolean valid;
	GtkTreeIter iter;
	
	/* Allocated a too big array: able to save all environment variables
	 * while we need to save only variables modified by user but it
	 * shouldn't be that big anyway and checking exactly which variable
	 * need to be saved will take more time */
	mod_var = g_new (gchar *, gtk_tree_model_iter_n_children (editor->model, NULL) + 1);
	var = mod_var;
	
	for (valid = gtk_tree_model_get_iter_first (editor->model, &iter); valid; valid = gtk_tree_model_iter_next (editor->model, &iter))
	{
		gchar *name;
		gchar *value;
		gchar *color;
		
		gtk_tree_model_get (editor->model, &iter,
							ENV_NAME_COLUMN, &name,
							ENV_VALUE_COLUMN, &value,
							ENV_COLOR_COLUMN, &color,
							-1);

		/* Save only variables modified by user */
		if (strcmp(color, ENV_USER_COLOR) == 0)
		{
			*var = g_strconcat(name, "=", value, NULL);
			var++;
		}
		g_free (name);
		g_free (value);
		g_free (color);
	}
	*var = NULL;

	return mod_var;
}

/*
 * anjuta_environment_editor_reset:
 * @editor: A AnjutaEnvironmentEditor widget
 *
 * Remove all variables modified by the user
 */
void
anjuta_environment_editor_reset (AnjutaEnvironmentEditor *editor)
{
	gtk_list_store_clear (GTK_LIST_STORE (editor->model));
	load_environment_variables (editor, GTK_LIST_STORE (editor->model));
}

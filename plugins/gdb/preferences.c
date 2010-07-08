/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* preferences.c
 *
 * Copyright (C) 2010  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "preferences.h"


#include <libanjuta/anjuta-utils.h>


#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/anjuta-gdb.ui"

#define BUILD_PREFS_DIALOG "preferences_dialog_build"
#define GDB_PREFS_ROOT "gdb_preferences_container"
#define GDB_PRINTER_TREEVIEW "printers_treeview"
#define GDB_PRINTER_REMOVE_BUTTON "remove_button"


#define ICON_FILE "anjuta-gdb.plugin.png"

#define GDB_SECTION "Gdb"
#define GDB_PRINTER_KEY "PrettyPrinter"


/* column of the printer list view */
enum {
	GDB_PP_ACTIVE_COLUMN,
	GDB_PP_FILENAME_COLUMN,
	GDB_PP_REGISTER_COLUMN,
	GDB_PP_N_COLUMNS
};

/* Node types
 *---------------------------------------------------------------------------*/

typedef struct
{
	GtkTreeView *treeview;
	GtkListStore *model;
	GtkWidget *remove_button;
	GList **list;
} PreferenceDialog;
 
 
/* Private functions
 *---------------------------------------------------------------------------*/

static gboolean
gdb_append_missing_register_function (GString *msg, GtkTreeModel *model, GtkTreeIter *iter)
{
	gboolean active;
	gchar *path;
	gchar *function;
	gboolean missing;

	gtk_tree_model_get (model, iter,
						GDB_PP_ACTIVE_COLUMN, &active,
						GDB_PP_FILENAME_COLUMN, &path,
						GDB_PP_REGISTER_COLUMN, &function, -1);
	if (function != NULL) function = g_strstrip (function);

	missing = active && ((function == NULL) || (*function == '\0'));
	if (missing)
	{
		g_string_append (msg, path);
		g_string_append (msg, "\n");
		gtk_list_store_set (GTK_LIST_STORE (model), iter, GDB_PP_ACTIVE_COLUMN, FALSE, -1);
	}
	g_free (path);
	g_free (function);
	
	return missing;
}
 
static void
gdb_check_register_function (PreferenceDialog *dlg, GtkTreeIter *iter)
{
	GString *list;

	list = g_string_new (NULL);
	if (iter == NULL)
	{
		GtkTreeIter iter;
		gboolean valid;
		
		for (valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dlg->model), &iter);
			valid; valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (dlg->model), &iter))
		{
			gdb_append_missing_register_function (list, GTK_TREE_MODEL (dlg->model), &iter);
		}
	}
	else
	{
		gdb_append_missing_register_function (list, GTK_TREE_MODEL (dlg->model), iter);
	}

	if (list->len > 0)
	{
		gchar *msg;

		msg = g_strdup_printf(_("The following pretty printers, without a register functions, have been disabled:\n %s"), list->str);
		anjuta_util_dialog_warning (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (dlg->treeview))), msg);
		g_free (msg);
		g_string_free (list, TRUE);
	}
}

static gchar *
gdb_find_register_function (const gchar *path)
{
	GFile *file;
	gchar *function = NULL;
	gchar *content = NULL;

	file = g_file_new_for_path (path);

	if (g_file_load_contents (file, NULL, &content, NULL, NULL, NULL))
	{
		GRegex *regex;
		GMatchInfo *match;

		regex = g_regex_new ("^def\\s+(register\w*)\\s*\\(\\w+\\)\\s*:", G_REGEX_CASELESS | G_REGEX_MULTILINE, 0, NULL);
		if (g_regex_match (regex, content, 0, &match))
		{
			function = g_match_info_fetch (match, 1);
			g_match_info_free (match);
		}
		g_regex_unref (regex);
		g_free (content);
	}

	g_object_unref (file);

	return function;
}

static gboolean
on_add_printer_in_list (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	GList** list = (GList **)user_data;
	gchar *filepath;
	gchar *function;
	gboolean active;
	GdbPrettyPrinter *printer;

	gtk_tree_model_get (model, iter, GDB_PP_ACTIVE_COLUMN, &active,
						GDB_PP_FILENAME_COLUMN, &filepath,
						GDB_PP_REGISTER_COLUMN, &function,
						-1);
	printer = g_slice_new0 (GdbPrettyPrinter);
	printer->enable = active;
	printer->path = filepath;
	printer->function = function;
	*list = g_list_prepend (*list, printer);

	return FALSE;
}

/* Call backs
 *---------------------------------------------------------------------------*/

 /* Gtk builder callbacks */
void gdb_on_printer_add (GtkButton *button, gpointer user_data);
void gdb_on_printer_remove (GtkButton *button, gpointer user_data);
void gdb_on_destroy_preferences (GtkObject *object, gpointer user_data);

void
gdb_on_destroy_preferences (GtkObject *object, gpointer user_data)
{
	PreferenceDialog *dlg = (PreferenceDialog *)user_data;
	GList *new_list;

	/* Free previous list and replace with new one */
	g_list_foreach (*(dlg->list), (GFunc)gdb_pretty_printer_free, NULL);
	g_list_free (*(dlg->list));
	*(dlg->list) = NULL;

	/* Replace with new one */
	new_list = NULL;
	gtk_tree_model_foreach (GTK_TREE_MODEL (dlg->model), on_add_printer_in_list, &new_list);
	new_list = g_list_reverse (new_list);
	*(dlg->list) = new_list;

	g_free (dlg);
}

void
gdb_on_printer_add (GtkButton *button, gpointer user_data)
{
	PreferenceDialog *dlg = (PreferenceDialog *)user_data;
	GtkWidget *chooser;
	GtkFileFilter *filter;
	
	chooser = gtk_file_chooser_dialog_new (_("Select a pretty printer file"),
					GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
	filter = gtk_file_filter_new ();
	gtk_file_filter_add_mime_type (filter, "text/x-python");
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), TRUE);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser),filter);

	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
	{
		GSList *filenames;
		GSList *item;
		
		filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (chooser));

		for (item = filenames; item != NULL; item = g_slist_next (item))
		{
			GtkTreeIter iter;
			gchar *path = (gchar *)item->data;
			gchar *function;
			
			function = gdb_find_register_function (path);
			
			gtk_list_store_append (dlg->model, &iter);
			gtk_list_store_set (dlg->model, &iter, GDB_PP_ACTIVE_COLUMN, TRUE,
												GDB_PP_FILENAME_COLUMN, path,
												GDB_PP_REGISTER_COLUMN, function,
												-1);
			g_free (path);
			g_free (function);
			
			gdb_check_register_function (dlg, &iter);
		}
		g_slist_free (filenames);
	}
	gtk_widget_destroy (chooser);
}

void
gdb_on_printer_remove (GtkButton *button, gpointer user_data)
{
	PreferenceDialog *dlg = (PreferenceDialog *)user_data;
	GtkTreeIter iter;
	GtkTreeSelection* sel;

	sel = gtk_tree_view_get_selection (dlg->treeview);
	if (gtk_tree_selection_get_selected (sel, NULL, &iter))
	{
		gtk_list_store_remove (dlg->model, &iter);
	}
}

static void
gdb_on_printer_activate (GtkCellRendererToggle *cell_renderer, const gchar *path, gpointer user_data)
{
	PreferenceDialog *dlg = (PreferenceDialog *)user_data;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (dlg->model), &iter, path))
	{
		gboolean enable;
		
		gtk_tree_model_get (GTK_TREE_MODEL (dlg->model), &iter, GDB_PP_ACTIVE_COLUMN, &enable, -1);
		enable = !enable;
		gtk_list_store_set (dlg->model, &iter, GDB_PP_ACTIVE_COLUMN, enable, -1);
		
		if (enable) gdb_check_register_function (dlg, &iter);
	}
}

static void
gdb_on_printer_function_changed (GtkCellRendererText *renderer,
								gchar		*path,
								gchar		*new_text,
								gpointer	user_data)
{
	PreferenceDialog *dlg = (PreferenceDialog *)user_data;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (dlg->model), &iter, path))
	{
		gchar *function = g_strstrip (new_text);
		gtk_list_store_set (dlg->model, &iter, GDB_PP_REGISTER_COLUMN, function, -1);
	}
}

static void
gdb_on_printer_selection_changed (GtkTreeSelection *selection, gpointer user_data)
{
	PreferenceDialog *dlg = (PreferenceDialog *)user_data;
	GtkTreeIter iter;
	GtkTreeSelection* sel;
	gboolean selected;

	sel = gtk_tree_view_get_selection (dlg->treeview);
	selected = gtk_tree_selection_get_selected (sel, NULL, &iter);
	gtk_widget_set_sensitive(dlg->remove_button, selected);
}
 
/* Public functions
 *---------------------------------------------------------------------------*/

void
gdb_merge_preferences (AnjutaPreferences* prefs, GList **list)
{
	GtkBuilder *bxml;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	PreferenceDialog *dlg;
	GList *item;

	g_return_if_fail (list != NULL);
	
	/* Create the preferences page */
	bxml =  anjuta_util_builder_new (BUILDER_FILE, NULL);
	if (!bxml) return;

	dlg = g_new0 (PreferenceDialog, 1);
	
	/* Get widgets */
	anjuta_util_builder_get_objects (bxml,
		GDB_PRINTER_TREEVIEW, &dlg->treeview,
		GDB_PRINTER_REMOVE_BUTTON, &dlg->remove_button,
		NULL);

	/* Create tree view */	
	dlg->model = gtk_list_store_new (GDB_PP_N_COLUMNS,
		G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (dlg->treeview, GTK_TREE_MODEL (dlg->model));
	g_object_unref (dlg->model);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (gdb_on_printer_activate), dlg);
	column = gtk_tree_view_column_new_with_attributes (_("Activate"), renderer,
		"active", GDB_PP_ACTIVE_COLUMN, NULL);
	gtk_tree_view_append_column (dlg->treeview, column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("File"), renderer,
		"text", GDB_PP_FILENAME_COLUMN, NULL);
	gtk_tree_view_append_column (dlg->treeview, column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", G_CALLBACK (gdb_on_printer_function_changed), dlg);
	column = gtk_tree_view_column_new_with_attributes (_("Register Function"), renderer,
		"text", GDB_PP_REGISTER_COLUMN, NULL);
	gtk_tree_view_append_column (dlg->treeview, column);

	/* Connect all signals */
	gtk_builder_connect_signals (bxml, dlg);
	selection = gtk_tree_view_get_selection (dlg->treeview);
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (gdb_on_printer_selection_changed), dlg);

	
	/* Fill tree view */
	dlg->list = list;
	for (item = g_list_first (*list); item != NULL; item = g_list_next (item))
	{
		GdbPrettyPrinter *printer = (GdbPrettyPrinter *)item->data;
		GtkTreeIter iter;
			
		gtk_list_store_append (dlg->model, &iter);
		gtk_list_store_set (dlg->model, &iter, GDB_PP_ACTIVE_COLUMN, printer->enable ? TRUE : FALSE,
											GDB_PP_FILENAME_COLUMN, printer->path,
											GDB_PP_REGISTER_COLUMN, printer->function,
											-1);
	}
	
	anjuta_preferences_add_from_builder (prefs, bxml, GDB_PREFS_ROOT, _("Gdb Debugger"),  ICON_FILE);

	g_object_unref (bxml);
}

void
gdb_unmerge_preferences (AnjutaPreferences* prefs)
{
	anjuta_preferences_remove_page(prefs, _("Gdb Debugger"));
}

GList *
gdb_load_pretty_printers (AnjutaSession *session)
{
	GList *session_list;
	GList *list = NULL;
	GList *item;

	session_list = anjuta_session_get_string_list (session, GDB_SECTION, GDB_PRINTER_KEY);
	for (item = g_list_first (session_list); item != NULL; item = g_list_next (item))
	{
		GdbPrettyPrinter *printer;
		gchar *name = (gchar *)item->data;
		gchar *ptr;
		
		printer = g_slice_new0 (GdbPrettyPrinter);
		ptr = strchr (name, ':');
		if (ptr != NULL)
		{
			if (*name == 'E') printer->enable = TRUE;
			name = ptr + 1;
		}
		ptr = strrchr (name, ':');
		if (ptr != NULL)
		{
			*ptr = '\0';
			printer->function = g_strdup (ptr + 1);
		}
		printer->path = g_strdup (name);
		
		list = g_list_prepend (list, printer);
	}
	
	g_list_foreach (session_list, (GFunc)g_free, NULL);
	g_list_free (session_list);
	
	return list;
}

gboolean
gdb_save_pretty_printers (AnjutaSession *session, GList *list)
{
	GList *session_list = NULL;
	GList *item;
	
	for (item = g_list_first (list); item != NULL; item = g_list_next (item))
	{
		GdbPrettyPrinter *printer = (GdbPrettyPrinter *)item->data;
		gchar *name;
		
		name = g_strconcat (printer->enable ? "E:" : "D:", printer->path, ":", printer->function == NULL ? "" : printer->function, NULL);

		session_list = g_list_prepend (session_list, name);
	}
	session_list = g_list_reverse (session_list);
	anjuta_session_set_string_list (session, GDB_SECTION, GDB_PRINTER_KEY, session_list);
	g_list_foreach (session_list, (GFunc)g_free, NULL);
	g_list_free (session_list);
	
	return FALSE;
}

void
gdb_pretty_printer_free (GdbPrettyPrinter *printer)
{
	g_free (printer->path);
	g_free (printer->function);
	g_slice_free (GdbPrettyPrinter, printer);
}

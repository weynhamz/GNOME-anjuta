/*
    src_paths.c
    Copyright (C) 2000  Naba Kumar <naba@gnome.org>

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include <glade/glade.h>

#include "anjuta.h"
#include "resources.h"
#include "properties.h"
#include "src_paths.h"
#include "debugger.h"

#define PROJECT_SRC_PATHS "project.src.paths"

struct _SrcPathsPriv
{
	PropsID props;
	GtkWidget *dialog;
	GtkWidget *clist;
	GtkWidget *entry;
};

enum
{
	PATHS_COLUMN,
	N_COLUMNS
};

static gchar *
get_from_treeview_as_string (GtkTreeView *view, const gchar *separator)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar* tmp;
	gchar* str = g_strdup ("");
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid)
	{
		gchar *text;
		gtk_tree_model_get (model, &iter, PATHS_COLUMN, &text, -1);
		tmp = g_strconcat (str, separator, text, NULL);
		g_free (str);
		str = tmp;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	return str;
}

static void
sync_to_props (SrcPaths *co)
{
	gchar *str =
		get_from_treeview_as_string (GTK_TREE_VIEW (co->priv->clist), " ");
	prop_set_with_key (co->priv->props, PROJECT_SRC_PATHS, str);
	g_free (str);
}

static gboolean
verify_new_entry (GtkTreeModel *model, const gchar *str, gint col)
{
	GtkTreeIter iter;
	gboolean valid;
	
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gchar *text;
		gtk_tree_model_get (model, &iter, col, &text, -1);
		if (strcmp(str, text) == 0)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	return TRUE;
}

static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, SrcPaths *co)
{
	sync_to_props (co);
	co->priv->dialog = NULL;
	if (debugger_is_active ())
	{
		debugger_update_src_dirs ();
	}
	return FALSE;
}

static void
on_selection_changed (GtkTreeSelection *sel, SrcPaths * co)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	const gchar *text;
	GtkEntry *entry = GTK_ENTRY (co->priv->entry);
	
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, PATHS_COLUMN, &text, -1);
		gtk_entry_set_text (entry, text);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (co->priv->dialog),
										   GTK_RESPONSE_CANCEL, TRUE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (co->priv->dialog),
										   GTK_RESPONSE_APPLY, TRUE);
	}
	else
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (co->priv->dialog),
										   GTK_RESPONSE_CANCEL, FALSE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (co->priv->dialog),
										   GTK_RESPONSE_APPLY, FALSE);
	}
}

static void
on_response (GtkDialog *dlg, gint res, SrcPaths *co)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeView *view;
	GtkEntry *entry;
	GtkWidget *win;
	gint valid;
	gchar *text, *str;
	
	g_return_if_fail (co);
	g_return_if_fail (co->priv->dialog);
	
	entry = GTK_ENTRY (co->priv->entry);
	view = GTK_TREE_VIEW (co->priv->clist);
	selection = gtk_tree_view_get_selection (view);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	
	switch (res)
	{
	case GTK_RESPONSE_OK: /* Add */
		str = g_strdup (gtk_entry_get_text (entry));
		text = g_strstrip(str);
		if (strlen (text) == 0)
		{
			g_free (str);
			return;
		}
		if (verify_new_entry (GTK_TREE_MODEL (model),
							  text, PATHS_COLUMN) == FALSE)
		{
			gtk_entry_set_text (entry, "");
			g_free (str);
			return;
		}
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							PATHS_COLUMN, text, -1);
		gtk_entry_set_text (entry, "");
		g_free (str);
		break;

	case GTK_RESPONSE_APPLY: /* Update */
		str = g_strdup (gtk_entry_get_text (entry));
		text = g_strstrip(str);
		if (strlen (text) == 0)
		{
			g_free (str);
			return;
		}
		if (verify_new_entry (GTK_TREE_MODEL (model),
							  text, PATHS_COLUMN) == FALSE)
		{
			gtk_entry_set_text (entry, "");
			g_free (str);
			return;
		}
		if (valid)
		{
			gtk_list_store_set (GTK_LIST_STORE (model),
								&iter, PATHS_COLUMN, text, -1);
			gtk_entry_set_text (entry, "");
		}
		g_free (str);
		break;

	case GTK_RESPONSE_CANCEL: /* Remove */
		if (valid)
		{
			gtk_list_store_remove (GTK_LIST_STORE(model), &iter);
			gtk_entry_set_text (GTK_ENTRY (entry), "");
		}
		break;
		
	case GTK_RESPONSE_REJECT: /* Clear */
		win = gtk_message_dialog_new (GTK_WINDOW (co->priv->dialog),
									  GTK_DIALOG_DESTROY_WITH_PARENT,
									  GTK_MESSAGE_QUESTION,
									  GTK_BUTTONS_NONE,
									  _("Do you want to clear the list?"));
		gtk_dialog_add_buttons (GTK_DIALOG (win),
								GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
								GTK_STOCK_CLEAR,	GTK_RESPONSE_YES,
								NULL);
		if (gtk_dialog_run (GTK_DIALOG (win)) == GTK_RESPONSE_YES)
			gtk_list_store_clear (GTK_LIST_STORE (model));
		gtk_widget_destroy (win);
		break;
	}
	sync_to_props (co);
}

static void
create_src_paths_gui (SrcPaths *co)
{
	GladeXML *gxml;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	
	if (co->priv->dialog)
		return;
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "source_paths_dialog", NULL);
	co->priv->dialog = glade_xml_get_widget (gxml, "source_paths_dialog");
	gtk_widget_show (co->priv->dialog);
	co->priv->clist = glade_xml_get_widget (gxml, "src_clist");
	co->priv->entry = glade_xml_get_widget (gxml, "src_entry");
	
	gtk_window_set_transient_for (GTK_WINDOW (co->priv->dialog),
								  GTK_WINDOW (app->widgets.window));
	
	gtk_dialog_set_response_sensitive (GTK_DIALOG (co->priv->dialog),
									   GTK_RESPONSE_CANCEL, FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (co->priv->dialog),
									   GTK_RESPONSE_APPLY, FALSE);
	
	/* Add tree model */
	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (co->priv->clist),
							 GTK_TREE_MODEL(store));
	
	/* Add column */	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Source Paths"),
													   renderer,
													   "text",
													   PATHS_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (co->priv->clist), column);
	g_object_unref (G_OBJECT(store));
	
	/* Connect signals */
	g_signal_connect (G_OBJECT (co->priv->dialog), "delete_event",
					  G_CALLBACK (on_delete_event), co);
	g_signal_connect (G_OBJECT (co->priv->dialog), "response",
					  G_CALLBACK (on_response), co);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (co->priv->clist));
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_selection_changed), co);

	g_object_unref (gxml);
}

SrcPaths *
src_paths_new (void)
{
	SrcPaths *co = g_new0 (SrcPaths, 1);
	co->priv = g_new0 (SrcPathsPriv, 1);
	co->priv->props = 0;
	co->priv->dialog = NULL;
	return co;
}

void
src_paths_destroy (SrcPaths * co)
{
	g_return_if_fail (co);
	g_free (co->priv);
	g_free (co);
}

gboolean
src_paths_save (SrcPaths * co, FILE * f)
{
	GList *list, *node;
	gchar *text;

	g_return_val_if_fail (co != NULL, FALSE);
	g_return_val_if_fail (f != NULL, FALSE);
	
	fprintf (f, PROJECT_SRC_PATHS"=");
	list = glist_from_data (co->priv->props, PROJECT_SRC_PATHS);
	node = list;
	while (node)
	{
		text = node->data;
		if (node->next)
			fprintf (f, "%s \\\n", text);
		else
			fprintf (f, "%s\n", text);
		node = g_list_next (node);
	}
	fprintf (f, "\n");
	glist_strings_free (list);
	return TRUE;
}

gboolean
src_paths_load (SrcPaths *co, PropsID props)
{
	g_return_val_if_fail (co != NULL, FALSE);
	co->priv->props = props;
	return TRUE;
}

GList *
src_paths_get_paths (SrcPaths * co)
{
	return glist_from_data (co->priv->props, PROJECT_SRC_PATHS);
}

void
src_paths_show (SrcPaths * co)
{
	GList *list, *node;
	
	g_return_if_fail (co != NULL);
	create_src_paths_gui (co);

	list = glist_from_data (co->priv->props, PROJECT_SRC_PATHS);
	node = list;
	while (node)
	{
		if (node->data)
		{
			GtkTreeIter iter;
			GtkTreeModel *model;
			gchar *path = node->data;
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (co->priv->clist));
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
								PATHS_COLUMN, path, -1);
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);
	gtk_entry_set_text (GTK_ENTRY (co->priv->entry), "");
}

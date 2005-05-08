/* 
 * watch_cbs.c Copyright (C) 2000 Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
/* TODO #include "anjuta.h" */
#include "watch_cbs.h"
#include "watch_gui.h"
#include "utilities.h"
#include "info.h"

extern gchar *eval_entry_history;
extern gchar *expr_watch_entry_history;

static void add_watch_entry (GtkEntry * ent, ExprWatch* ew);
static void change_watch_entry (GtkEntry * ent, ExprWatch* ew);

gboolean
on_watch_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GdkEventButton *bevent;
	ExprWatch *ew = user_data;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	if (((GdkEventButton *) event)->button == 3) {
		bevent = (GdkEventButton *) event;
		bevent->button = 1;
		expr_watch_update_controls (ew);
		gtk_menu_popup (GTK_MENU (ew->widgets.menu), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
		return TRUE;
	}
	return FALSE;
}

void
on_watch_add_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	ExprWatch* ew = (ExprWatch*) user_data;
	GtkWidget *dialog = create_watch_add_dialog (ew);
	gtk_widget_show (dialog);
}


void
on_watch_remove_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	ExprWatch* ew = (ExprWatch*) user_data;

	view = GTK_TREE_VIEW (ew->widgets.clist);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		g_warning("Error getting selection\n");
		return;
	}	
	
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	expr_watch_update_controls (ew);
}

void
on_watch_clear_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	ExprWatch* ew = (ExprWatch*) user_data;
	
	expr_watch_clear (ew);
}

void
on_watch_change_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *dialog;
	ExprWatch* ew = (ExprWatch*) user_data;

	dialog = create_watch_change_dialog (ew);
	gtk_widget_show (dialog);
}

void
on_watch_update_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	ExprWatch* ew = (ExprWatch*) user_data;
	
	expr_watch_cmd_queqe (ew);
}

void
on_ew_add_ok_clicked (GtkButton * button, gpointer user_data)
{
	ExprWatch* ew = (ExprWatch*) g_object_get_data (G_OBJECT(user_data),
													"user_data");
	add_watch_entry ((GtkEntry *) user_data, ew);
}

void
on_ew_entry_activate (GtkWidget * wid, gpointer user_data)
{
	on_ew_add_ok_clicked (NULL, wid);
	gtk_widget_destroy (GTK_WIDGET (user_data));
}

void
on_ew_entry_change_activate (GtkWidget * wid, gpointer user_data)
{
	on_ew_change_ok_clicked (NULL, wid);
	gtk_widget_destroy (GTK_WIDGET (user_data));
}

void
on_ew_change_ok_clicked (GtkButton * wid, gpointer user_data)
{
	ExprWatch* ew =(ExprWatch*)g_object_get_data(G_OBJECT(user_data),"user_data");	
	change_watch_entry ((GtkEntry *) user_data, ew);
}

void
on_eval_entry_activate (GtkWidget * wid, gpointer user_data)
{
	on_eval_ok_clicked (NULL, wid);
	gtk_widget_destroy (GTK_WIDGET (user_data));
}

void
on_eval_add_watch (GtkButton * button, gpointer user_data)
{
	ExprWatch* ew = (ExprWatch*) g_object_get_data (G_OBJECT(user_data),
													"user_data");
	add_watch_entry ((GtkEntry *) user_data, ew);
}

static void
eval_output_arrived (Debugger *debugger, const GDBMIValue *mi_result,
					 const GList * lines, gpointer data)
{
	GList *list;
	const gchar *tmp1, *tmp2;
	gchar *tmp3 = NULL;

	list = gdb_util_remove_blank_lines (lines);
	if (g_list_length ((GList*)list) < 1)
	{
		g_list_free (list);
		return;
	}
	if (data)
	{
		tmp1 = list->data;
		tmp2 = strchr (tmp1, '=');
		if (tmp2)
		{
			tmp3 = g_strconcat (data, " ", tmp2, NULL);
			list = g_list_remove (list, tmp1);
			list = g_list_prepend (list, tmp3);
		}
		g_free (data);
	}
	gdb_info_show_list (NULL, (GList*)lines, 0, 0);
	g_list_free (list);
	if (tmp3)
		g_free (tmp3);
}

void
on_eval_ok_clicked (GtkButton * button, gpointer user_data)
{
	GtkEntry *ent;
	const gchar *buff1;
	ExprWatch* ew = (ExprWatch*) g_object_get_data (G_OBJECT(user_data),
													"user_data");	

	ent = (GtkEntry *) user_data;
	buff1 = gtk_entry_get_text (ent);
	if (strlen (buff1) == 0)
		return;
	if (eval_entry_history)
		g_free (eval_entry_history);
	eval_entry_history = g_strdup (buff1);

	expr_watch_evaluate_expression (ew, buff1, eval_output_arrived,
									g_strdup (buff1));
}

void
expr_watch_update (Debugger *debugger, const GDBMIValue *mi_result,
				   const GList *lines, gpointer data)
{
	gchar *ptr, *tmp;
	GdkColor *color;
	gchar not_def[] = N_("< Not defined in current context >");
	GdkColor red = { 16, -1, 0, 0 };
	GdkColor blue = { 16, 0, 0, -1 };
	GList *list, *node;
	GtkTreeModel* model;
	GtkTreeIter iter;
	
	struct watch_cb_data* cb_data = (struct watch_cb_data*)data;

	list = gdb_util_remove_blank_lines (lines);
	if (g_list_length (list) < 1)
	{
		tmp = _(not_def);
		color = &red;
	}
	else
	{
		tmp = strchr ((gchar *) list->data, '=');
		color = &blue;
	}
	if (tmp == NULL)
	{
		tmp = _(not_def);
		color = &red;
	}
	ptr = g_strdup (tmp);
	if (list)
		node = list->next;
	else
		node = NULL;
	while (node)
	{
		tmp = ptr;
		ptr = g_strconcat (tmp, (gchar *) node->data, NULL);
		g_free (tmp);
		node = node->next;
	}
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(cb_data->ew->widgets.clist));
	if (gtk_tree_model_get_iter (model, &iter, cb_data->path))
	{
		gtk_list_store_set(GTK_LIST_STORE(model), 
						   &iter, WATCH_VALUE_COLUMN, ptr, -1);
	}
	else
	{
		g_warning ("Invalid watch model path");
	}
	expr_watch_update_controls (cb_data->ew);
	
	gtk_tree_path_free (cb_data->path);
	g_free (ptr);
	if (list)
		g_list_free (list);
	g_free(cb_data);
}

/* ************ private methods ***************/

static void
add_watch_entry (GtkEntry * ent, ExprWatch* ew)
{
	const gchar *row;
	gchar *buff;
	GtkTreeModel *model;
	GtkTreeIter iter;
	struct watch_cb_data *cb_data;

	if (GTK_IS_ENTRY (ent) == FALSE)
		return;
	row = gtk_entry_get_text (ent);
	if (row == NULL || strlen (row) <= 0)
		return;

	if (expr_watch_entry_history)
		g_free (expr_watch_entry_history);
	expr_watch_entry_history = g_strdup (row);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ew->widgets.clist));	

	/* add a watch entry */
	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter, 
						WATCH_VARIABLE_COLUMN, row,
						WATCH_VALUE_COLUMN, "", -1);

	/* send command to gdb to get the initial value */
	buff = g_strconcat ("print ", row, NULL);
	
	cb_data = g_new0 (struct watch_cb_data, 1);
	cb_data->ew = ew;
	cb_data->path = gtk_tree_model_get_path (model, &iter);
	
	debugger_command (ew->debugger, buff, FALSE, expr_watch_update, cb_data);

	/* TODO	an_message_manager_show_pane (app->messages, MESSAGE_WATCHES); */
	g_free (buff);
}

static void
change_watch_entry (GtkEntry * ent, ExprWatch* ew)
{
	gchar *row, *buff;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	struct watch_cb_data *cb_data;

	if (GTK_IS_ENTRY (ent) == FALSE)
		return;

	row = (gchar *) gtk_entry_get_text (ent);
	if (strlen (row) == 0)
		return;

	view = GTK_TREE_VIEW (ew->widgets.clist);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	/* get iterator to the currently selected line */
	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		g_warning("Error getting watch tree selection\n");
		return;
	}

	if (expr_watch_entry_history)
		g_free (expr_watch_entry_history);
	
	expr_watch_entry_history = g_strdup (row);

	/* change the variable */
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						WATCH_VARIABLE_COLUMN, g_strdup (row),
						WATCH_VALUE_COLUMN, "", -1);

	/* send command to gdb to get the initial value */	
	buff = g_strconcat ("print ", row, NULL);
	
	cb_data = g_new (struct watch_cb_data, 1);
	cb_data->ew = ew;
	cb_data->path = gtk_tree_model_get_path (model, &iter);
	
	debugger_command (ew->debugger, buff, FALSE, expr_watch_update, cb_data);
	g_free (buff);
}

/* 
 * watch.c Copyright (C) 2000 Kh. Naba Kumar Singh
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include <libanjuta/resources.h>

#include "debugger.h"
#include "anjuta_info.h"
#include "utilities.h"
#include "watch.h"
#include "watch_gui.h"

ExprWatch *
expr_watch_new ()
{
	ExprWatch *ew;

	ew = g_new0 (ExprWatch, 1);
	create_expr_watch_gui (ew);
	return ew;
}

void
expr_watch_clear (ExprWatch * ew)
{
	GtkTreeModel* model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(ew->widgets.clist));

	gtk_list_store_clear(GTK_LIST_STORE(model));
	expr_watch_update_controls (ew);
}

static gboolean
get_watch_expr (GtkTreeModel *model, GtkTreePath* path, GtkTreeIter* iter, gpointer pdata)
{

	ExprWatch* ew = pdata;
	gchar* variable = NULL;
	gchar* buff = NULL;
	struct watch_cb_data* cb_data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, WATCH_VARIABLE_COLUMN, &variable, -1);	

	if (variable)
	{
		/* allocate data structure for cb function */
		cb_data = g_new(struct watch_cb_data,1);
		cb_data->ew = ew;
		cb_data->iter = gtk_tree_iter_copy(iter);
		
		buff = g_strconcat ("print ", (gchar *)variable, NULL);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, expr_watch_update, cb_data);
		g_free(buff);

		return FALSE;
	}
	else
	{
		g_warning("Error getting variable\n");
		return TRUE;
	}
}

void
expr_watch_cmd_queqe (ExprWatch * ew)
{
	GtkTreeModel* model;
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(ew->widgets.clist));
	
	/* for each watched expression, send a print command using the debugger */
	gtk_tree_model_foreach(model,get_watch_expr,ew);			
}


void
expr_watch_update_controls (ExprWatch * ew)
{
	gboolean A, R/*, C, S*/;

	A = debugger_is_active ();
	R = debugger_is_ready ();
	/*C = (g_list_length (ew->exprs) > 0);
	S = (ew->current_index >= 0);*/
	gtk_widget_set_sensitive (ew->widgets.menu_add, A && R);
	gtk_widget_set_sensitive (ew->widgets.menu_update, A && R /*&& C*/);
	gtk_widget_set_sensitive (ew->widgets.menu_remove, A && R /*&& C && S*/);
	gtk_widget_set_sensitive (ew->widgets.menu_clear, A && R /*&& C*/);
	gtk_widget_set_sensitive (ew->widgets.menu_toggle, FALSE);
	gtk_widget_set_sensitive (ew->widgets.menu_change, A && R /*&& C && S*/);
}

void
eval_output_arrived (GList * lines, gpointer data)
{
	GList *list;

	list = lines;
	if (g_list_length (list) < 1)
		return;
	if (data)
	{
		gchar *tmp1, *tmp2, *tmp3;

		tmp1 = list->data;
		tmp2 = strchr (tmp1, '=');
		if (tmp2)
		{
			tmp3 = g_strconcat (data, " ", tmp2, NULL);
			list = g_list_remove (list, tmp1);
			if (tmp1)
				g_free (tmp1);
			list = g_list_prepend (list, tmp3);
		}
		g_free (data);
	}
	anjuta_info_show_list (lines, 0, 0);
}


void
expr_watch_destroy (ExprWatch * ew)
{
	if (ew)
	{
		expr_watch_clear (ew);
		gtk_widget_unref (ew->widgets.clist);
		gtk_widget_unref (ew->widgets.menu_add);
		gtk_widget_unref (ew->widgets.menu_remove);
		gtk_widget_unref (ew->widgets.menu_clear);
		gtk_widget_unref (ew->widgets.menu_update);
		gtk_widget_unref (ew->widgets.menu_toggle);
		gtk_widget_unref (ew->widgets.menu);
		g_free (ew);
	}
}

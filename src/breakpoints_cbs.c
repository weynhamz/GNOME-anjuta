/*
    breakpoints_cbs.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#include <gnome.h>


#include "anjuta.h"
#include "breakpoints.h"
#include "debugger.h"

#include "breakpoints_cbs.h"

static void bk_close (gpointer data);

void
on_bk_remove_clicked (GtkWidget *button,
		      gpointer   data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	GtkTreeModel *model;
	GtkTreeIter iter;

	bd = (BreakpointsDBase *) data;

	if (bd->breakpoints == NULL)
		return;


	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->widgets.treeview));
	gtk_tree_model_iter_nth_child (model, &iter, NULL, bd->current_index);
	gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);

	bi = g_list_nth_data (bd->breakpoints, bd->current_index);
	debugger_delete_breakpoint (bi->id);
	bd->breakpoints = g_list_remove (bd->breakpoints, bi);
	breakpoint_item_destroy (bi);
}

void
on_bk_jumpto_clicked (GtkWidget *button,
		      gpointer   data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;

	bd = (BreakpointsDBase *) data;

	if (bd->breakpoints == NULL)
		return;

	bi = g_list_nth_data (bd->breakpoints, bd->current_index);

	if (!bd->is_docked)
#warning "G2 port: THOUROUGLY VERIFY ME"
		gtk_widget_hide (bd->widgets.window);
	else
		breakpoints_dbase_hide (bd);

	anjuta_goto_file_line (bi->file, bi->line);
}

void
on_bk_properties_clicked (GtkWidget *button,
			  gpointer   data)
{
	BreakpointsDBase *bd;
	GtkWidget *dialog;

	bd = (BreakpointsDBase *) data;

	if (bd->breakpoints == NULL)
		return;

	/* We don't ref the dialog so it's slower but all fields are erased then. */
	dialog = glade_xml_get_widget (bd->gxml_prop, "breakpoint_properties_dialog");

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		if (bd->edit_index >= 0)
			return;

		bd->edit_index = bd->current_index;

		on_bk_item_add_ok_clicked (button, data);
	}
}

void
on_bk_add_clicked (GtkWidget *button,
		   gpointer   data)
{
	GtkWidget *dialog;
	BreakpointsDBase *bd;

	bd = (BreakpointsDBase *) data;

	/* We don't ref the dialog so it's slower but all fields are erased then. */
	dialog = glade_xml_get_widget (bd->gxml_prop, "breakpoint_properties_dialog");

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
		on_bk_item_add_ok_clicked (button, data);
}

void
on_bk_removeall_clicked (GtkWidget *button,
			 gpointer   data)
{
	BreakpointsDBase *bd;
	GtkWidget *dialog;

	bd = (BreakpointsDBase *) data;

	if (bd->breakpoints == NULL)
		return;

	dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Are you sure you want to delete all the breakpoints ?"),
					 NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES) {
		debugger_delete_all_breakpoints ();
		breakpoints_dbase_clear (debugger.breakpoints_dbase);
	} else
		bk_close (data);
}

void
on_bk_enableall_clicked (GtkWidget *button,
			 gpointer   data)
{
#warning "G2 port: change the active state of the enabled column"
	debugger_enable_all_breakpoints ();
}

void
on_bk_disableall_clicked (GtkWidget *button,
			  gpointer   data)
{
#warning "G2 port: change the active state of the enabled column"
	debugger_disable_all_breakpoints ();
}

gboolean
on_bk_treeview_event (GtkWidget *widget,
		      GdkEvent  *event,
		      gpointer   data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	BreakpointsDBase *bd;

	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	if (!event)
		return FALSE;

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return FALSE;

	bd = (BreakpointsDBase *) data;
	bd->current_index = gtk_tree_model_iter_n_children (model, &iter);

	breakpoints_dbase_update_controls (bd);
}

#warning "G2 port: verify the behaviour w/ ANJUTA_1_0_0"
static void
bk_close (gpointer data)
{
	BreakpointsDBase *db = (BreakpointsDBase *) data;

	if (NULL != db && FALSE == db->is_docked)
		gtk_widget_hide (db->widgets.window);
	else
		breakpoints_dbase_hide (db);
}

/*************************************************************************************/

void
on_bk_item_add_ok_clicked (GtkWidget *button, gpointer data)
{
	BreakpointsDBase *bd;
	GtkWidget *location_entry;
	GtkWidget *condition_entry;
	GtkWidget *pass_entry;
	gchar *buff;

	bd = (BreakpointsDBase *) data;

	if (bd->breakpoints == NULL)
		return;

	location_entry = glade_xml_get_widget (bd->gxml_prop, "breakpoint_location_entry");
	condition_entry = glade_xml_get_widget (bd->gxml_prop, "breakpoint_condition_entry");
	pass_entry = glade_xml_get_widget (bd->gxml_prop, "breakpoint_pass_entry");

	if (strlen (gtk_entry_get_text (GTK_ENTRY (location_entry))) > 0)
	{
		const gchar *loc_text;
		const gchar *cond_text;
		const gchar *pass_text;

		loc_text = gtk_entry_get_text (GTK_ENTRY (location_entry));
		cond_text = gtk_entry_get_text (GTK_ENTRY (condition_entry));
		pass_text = gtk_entry_get_text (GTK_ENTRY (pass_entry));

		if (strlen (cond_text) != 0)
			buff = g_strdup_printf ("break %s if %s", loc_text, cond_text);
		else
			buff = g_strdup_printf ("break %s", loc_text);

		debugger_put_cmd_in_queqe (buff,
					   DB_CMD_ALL,
					   bk_item_add_mesg_arrived,
					   data);
		g_free (buff);

		debugger_execute_cmd_in_queqe ();
	}
	else
	{
		anjuta_error (_("You must give a valid location to set the breakpoint."));
	}
}

void
bk_item_add_mesg_arrived (GList * lines, gpointer data)
{
	struct BkItemData *bid;
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	gint index;
	GList *outputs;

	bid = (struct BkItemData *) data;
	outputs = remove_blank_lines (lines);
	if (outputs == NULL)
		goto down_label;
	if (g_list_length (outputs) == 1)
	{
		gint id, count, line;
		gchar file[256];
		gchar *buff;
		
		count =	sscanf ((char *) g_list_nth_data (outputs, 0),
			"Breakpoint %d at %*s file %s line %d", &id, file, &line);
		if (count != 3)
		{
			debugger_dialog_message (outputs, data);
			goto down_label;
		}
		/* Removing the extra comma at the end ot the filename */
		if (strlen(file))
			file[strlen(file)-1] = '\0';

		anjuta_goto_file_line_mark (file, line, FALSE);
		if (bid == NULL)
			return;
		if (strlen (bid->pass_text) > 0)
		{
			buff =
				g_strdup_printf ("ignore %d %s", id,
						 bid->pass_text);
			debugger_put_cmd_in_queqe (buff, DB_CMD_ALL,
						   pass_item_add_mesg_arrived,
						   NULL);
			g_free (buff);
		}

		bd = bid->bd;
		index = bd->edit_index;

		if (index >= 0) {
			GtkTreeModel *model;
			GtkTreeIter iter;

			model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->widgets.treeview));
			gtk_tree_model_iter_nth_child (model, &iter, NULL, index);
			gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);

			bi = g_list_nth_data (bd->breakpoints, index);

			debugger_delete_breakpoint (bi->id);
			bd->edit_index = -1;
			bd->breakpoints = g_list_remove (bd->breakpoints, bi);
			breakpoint_item_destroy (bi);
		}
		else
		{
			debugger_put_cmd_in_queqe ("info breakpoints",
						   DB_CMD_NONE,
						   breakpoints_dbase_update,
						   debugger.
						   breakpoints_dbase);
		}
		debugger_execute_cmd_in_queqe ();
	}
	else
	{
		debugger_dialog_message (outputs, data);
	}

      down_label:

	if (bid) {
		if (bid->loc_text)
			g_free (bid->loc_text);
		if (bid->cond_text)
			g_free (bid->cond_text);
		if (bid->pass_text)
			g_free (bid->pass_text);
		g_free (bid);
	}

	if (outputs)
		g_list_free (outputs);

	return;
}

void
pass_item_add_mesg_arrived (GList * lines, gpointer data)
{
	GList *outputs;

	outputs = remove_blank_lines (lines);

	if (outputs == NULL)
		return;

	if (g_list_length (outputs) != 1) {
		GtkWidget *dialog;
		gchar *msg, *tmp;
		gint i;

		msg = g_strdup ((gchar *) g_list_nth_data (outputs, 0));

		for (i = 1; i < g_list_length (outputs); i++)
		{
			tmp = msg;
			msg =
				g_strconcat (tmp, "\n",
					     (gchar *)
					     g_list_nth_data (outputs, i),
					     NULL);
			g_free (tmp);
		}


		dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_CLOSE,
						 msg,
						 NULL);

		gtk_dialog_run (GTK_DIALOG (dialog));

		g_free (msg);
		g_list_free (outputs);

		return;
	}

	if (strstr ((gchar *) outputs->data, "Will ignore next") == NULL) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_CLOSE,
						 (gchar *) g_list_nth_data (outputs, 0),
						 NULL);

		gtk_dialog_run (GTK_DIALOG (dialog));

		g_list_free (outputs);

		return;
	}
}

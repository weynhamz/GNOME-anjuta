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
#include "messagebox.h"
#include "breakpoints.h"
#include "debugger.h"

#include "breakpoints_cbs.h"

gboolean
on_breakpoints_close (GtkWidget * w, gpointer data)
{
	BreakpointsDBase *bd = data;
	breakpoints_dbase_hide (bd);
	return FALSE;
}

void
on_bk_view_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;

	bd = (BreakpointsDBase *) user_data;
	if (bd->breakpoints == NULL)
		return;
	bi = g_list_nth_data (bd->breakpoints, bd->current_index);
	if (FALSE == bd->is_docked)
		gnome_dialog_close(GNOME_DIALOG(bd->widgets.window));
	else
		breakpoints_dbase_hide (bd);
	anjuta_goto_file_line (bi->file, bi->line);
}

void
on_bk_edit_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *bd;
	GtkWidget *dialog;

	bd = (BreakpointsDBase *) user_data;
	if (bd->breakpoints == NULL)
		return;
	dialog = create_bk_edit_dialog (bd);
	if (dialog)
		gtk_widget_show (dialog);
}

void
on_bk_delete_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	gint index;

	bd = (BreakpointsDBase *) user_data;
	if (bd->breakpoints == NULL)
		return;
	index = bd->current_index;
	gtk_clist_remove (GTK_CLIST (bd->widgets.clist), index);
	bi = g_list_nth_data (bd->breakpoints, index);
	debugger_delete_breakpoint (bi->id);
	bd->breakpoints = g_list_remove (bd->breakpoints, bi);
	breakpoint_item_destroy (bi);
}

void
on_bk_delete_all_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *bd;
	bd = (BreakpointsDBase *) user_data;
	if (bd->breakpoints == NULL)
		return;

	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to delete all breakpoints?"),
		     GNOME_STOCK_BUTTON_YES,
		     GNOME_STOCK_BUTTON_NO,
		     on_bk_delete_all_confirm_yes_clicked, NULL, NULL);
}

void
on_bk_add_breakpoint_clicked (GtkButton * button, gpointer user_data)
{
	GtkWidget *dialog;
	dialog = create_bk_add_dialog ((BreakpointsDBase *) user_data);
	gtk_widget_show (dialog);
}

void
on_bk_toggle_enable_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;

	bd = (BreakpointsDBase *) user_data;
	if (bd->breakpoints == NULL)
		return;
	bi = g_list_nth_data (bd->breakpoints, bd->current_index);
	if (bi->enable)
	{
		bi->enable = FALSE;
		gtk_clist_set_text (GTK_CLIST (bd->widgets.clist),
				    bd->current_index, 0, _("NO"));
		debugger_disable_breakpoint (bi->id);
	}
	else
	{
		bi->enable = TRUE;
		gtk_clist_set_text (GTK_CLIST (bd->widgets.clist),
				    bd->current_index, 0, _("YES"));
		debugger_enable_breakpoint (bi->id);
	}
}

void
on_bk_enable_all_clicked (GtkButton * button, gpointer user_data)
{
	debugger_enable_all_breakpoints ();
}

void
on_bk_disable_all_clicked (GtkButton * button, gpointer user_data)
{
	debugger_disable_all_breakpoints ();
}

void
on_bk_clist_select_row (GtkCList * clist,
			gint row,
			gint column, GdkEvent * event, gpointer user_data)
{
	BreakpointsDBase *bd;
	bd = (BreakpointsDBase *) user_data;
	bd->current_index = row;
	breakpoints_dbase_update_controls (bd);
}

void
on_bk_clist_unselect_row (GtkCList * clist,
			  gint row,
			  gint column, GdkEvent * event, gpointer user_data)
{
	BreakpointsDBase *bd;
	bd = (BreakpointsDBase *) user_data;
	bd->current_index = -1;
	breakpoints_dbase_update_controls (bd);
}

void
on_bk_help_clicked (GtkButton * button, gpointer user_data)
{

}

void
on_bk_close_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *db = user_data;
	if (NULL != db && FALSE == db->is_docked)
		gnome_dialog_close(GNOME_DIALOG(db->widgets.window));
	else
		breakpoints_dbase_hide (db);
}

void
on_bk_delete_all_confirm_yes_clicked (GtkButton * button, gpointer data)
{
	debugger_delete_all_breakpoints ();
	breakpoints_dbase_clear (debugger.breakpoints_dbase);
}

/*************************************************************************************/
gboolean
on_bk_item_add_delete_event (GtkWidget * w, GdkEvent * event,
			     gpointer user_data)
{
	struct BkItemData *bid;
	bid = (struct BkItemData *) user_data;

	if (!bid)
		return FALSE;
	gtk_widget_unref (bid->dialog);
	gtk_widget_unref (bid->loc);
	gtk_widget_unref (bid->cond);
	gtk_widget_unref (bid->pass);
	g_free (bid);
	return FALSE;
}

void
on_bk_item_add_help_clicked (GtkButton * button, gpointer user_data)
{

}

void
on_bk_item_add_cancel_clicked (GtkButton * button, gpointer user_data)
{
	struct BkItemData *bid;
	bid = (struct BkItemData *) user_data;
	if (!bid)
		return;
	gtk_widget_unref (bid->dialog);
	gtk_widget_unref (bid->loc);
	gtk_widget_unref (bid->cond);
	gtk_widget_unref (bid->pass);
	gtk_widget_destroy (bid->dialog);
	g_free (bid);
}

void
on_bk_item_add_ok_clicked (GtkButton * button, gpointer user_data)
{
	struct BkItemData *bid;
	gchar *buff;

	bid = (struct BkItemData *) user_data;
	if (bid == NULL)
		return;
	if (strlen (gtk_entry_get_text (GTK_ENTRY (bid->loc))) > 0)
	{
		bid->loc_text =
			g_strdup (gtk_entry_get_text (GTK_ENTRY (bid->loc)));
		bid->cond_text =
			g_strdup (gtk_entry_get_text (GTK_ENTRY (bid->cond)));
		bid->pass_text =
			g_strdup (gtk_entry_get_text (GTK_ENTRY (bid->pass)));

		if (bid->bd->loc_history)
			g_free (bid->bd->loc_history);
		bid->bd->loc_history = g_strdup (bid->loc_text);
		if (bid->bd->cond_history)
			g_free (bid->bd->cond_history);
		bid->bd->cond_history = g_strdup (bid->cond_text);
		if (bid->bd->pass_history)
			g_free (bid->bd->pass_history);
		bid->bd->pass_history = g_strdup (bid->pass_text);

		if (strlen (bid->cond_text) != 0)
			buff =
				g_strdup_printf ("break %s if %s",
						 bid->loc_text,
						 bid->cond_text);
		else
			buff = g_strdup_printf ("break %s", bid->loc_text);
		debugger_put_cmd_in_queqe (buff, DB_CMD_ALL,
					   bk_item_add_mesg_arrived,
					   user_data);
		g_free (buff);
		gtk_widget_unref (bid->dialog);
		gtk_widget_unref (bid->loc);
		gtk_widget_unref (bid->cond);
		gtk_widget_unref (bid->pass);
		gtk_widget_destroy (bid->dialog);
		debugger_execute_cmd_in_queqe ();
	}
	else
	{
		anjuta_error (_("You must give a valid location to set the breakpoint."));
	}
}

void
bk_item_add_mesg_arrived (GList * lines, gpointer user_data)
{
	struct BkItemData *bid;
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	gint index;
	GList *outputs;

	bid = (struct BkItemData *) user_data;
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
			debugger_dialog_message (outputs, user_data);
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
		if (index >= 0)
		{
			gtk_clist_remove (GTK_CLIST (bd->widgets.clist),
					  index);
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
		debugger_dialog_message (outputs, user_data);
	}

      down_label:

	if (bid)
	{
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
pass_item_add_mesg_arrived (GList * lines, gpointer user_data)
{
	GList *outputs;

	outputs = remove_blank_lines (lines);
	if (outputs == NULL)
		return;
	if (g_list_length (outputs) != 1)
	{
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
		messagebox (GNOME_MESSAGE_BOX_INFO, msg);
		g_free (msg);
		g_list_free (outputs);
		return;
	}
	if (strstr ((gchar *) outputs->data, "Will ignore next") == NULL)
	{
		messagebox (GNOME_MESSAGE_BOX_INFO,
			    (gchar *) g_list_nth_data (outputs, 0));
		g_list_free (outputs);
		return;
	}
}

/*************************************************************************************/

void
on_bk_item_edit_help_clicked (GtkButton * button, gpointer user_data)
{

}


void
on_bk_item_edit_ok_clicked (GtkButton * button, gpointer user_data)
{
	BreakpointsDBase *bd = ((struct BkItemData *) user_data)->bd;
	if (bd->edit_index >= 0)
		return;
	bd->edit_index = bd->current_index;
	on_bk_item_add_ok_clicked (button, user_data);
}

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
#include "debugger.h"
#include "watch_cbs.h"

extern gchar *eval_entry_history;
extern gchar *expr_watch_entry_history;

static void add_watch_entry (GtkEntry * ent);
static void change_watch_entry (GtkEntry * ent);

gint
on_watch_delete_event (GtkWidget * w, GdkEvent * event, gpointer data)
{
	ExprWatch *ew = debugger.watch;

	expr_watch_hide (ew);
	return TRUE;
}

gboolean
on_watch_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GdkEventButton *bevent;
	ExprWatch *ew = user_data;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	if (((GdkEventButton *) event)->button != 3)
		return FALSE;
	bevent = (GdkEventButton *) event;
	bevent->button = 1;
	expr_watch_update_controls (debugger.watch);
	gtk_menu_popup (GTK_MENU (ew->widgets.menu), NULL, NULL, NULL, NULL,
					bevent->button, bevent->time);
	return TRUE;
}

void
on_watch_clist_select_row (GtkCList * clist, gint row, gint column,
						   GdkEvent * event, gpointer user_data)
{
	ExprWatch *ew;

	ew = debugger.watch;
	ew->current_index = row;

	/* Attempt for double-click to work as expected... */
	if ((event->type == GDK_2BUTTON_PRESS) &&
		(((GdkEventButton *) event)->button == 1))
	{
		if ((g_list_length (ew->exprs) < 1) || (ew->current_index < 0))
			on_watch_add_activate (NULL, NULL);
		else
			on_watch_change_activate (NULL, NULL);
	}
}

void
on_watch_clist_unselect_row (GtkCList * clist, gint row, gint column,
							 GdkEvent * event, gpointer user_data)
{
	ExprWatch *ew;

	ew = debugger.watch;
	ew->current_index = -1;
}

void
on_watch_add_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *dialog =
		create_watch_add_dialog (GTK_WINDOW (debugger.watch->widgets.window));
	gtk_widget_show (dialog);
}

void
on_watch_remove_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *exp;

	if (g_list_length (debugger.watch->exprs) < 1)
		return;
	exp =
		g_list_nth_data (debugger.watch->exprs, debugger.watch->current_index);
	debugger.watch->exprs = g_list_remove (debugger.watch->exprs, exp);
	gtk_clist_remove (GTK_CLIST (debugger.watch->widgets.clist),
					  debugger.watch->current_index);
	expr_watch_update_controls (debugger.watch);
}

void
on_watch_clear_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	expr_watch_clear (debugger.watch);
}

void
on_watch_toggle_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}

void
on_watch_change_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *dialog;

	printf ("In on_watch_change_activate...\n");
	if (g_list_length (debugger.watch->exprs) < 1)
		return;
	expr_watch_entry_history =
		g_list_nth_data (debugger.watch->exprs, debugger.watch->current_index);
	dialog =
		create_watch_change_dialog (GTK_WINDOW
									(debugger.watch->widgets.window));
	gtk_widget_show (dialog);
}

void
on_watch_update_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	expr_watch_cmd_queqe (debugger.watch);
	debugger_execute_cmd_in_queqe ();
}

void
on_watch_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}

/*****************************************************************************/
void
on_ew_add_help_clicked (GtkButton * button, gpointer user_data)
{

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
on_ew_change_help_clicked (GtkButton * wid, gpointer user_data)
{

}

void
on_ew_change_ok_clicked (GtkButton * wid, gpointer user_data)
{
	change_watch_entry ((GtkEntry *) user_data);
}

static void
change_watch_entry (GtkEntry * ent)
{
	gchar *row[2], *exp;

	// gchar *buff;
	gint idx;					// To forcefully get the idx stuff right.

	if (GTK_IS_ENTRY (ent) == FALSE)
		return;
	if (g_list_length (debugger.watch->exprs) < 1)
		return;
	row[0] = (gchar *) gtk_entry_get_text (ent);
	if (strlen (row[0]) == 0)
		return;

	if (expr_watch_entry_history)
		g_free (expr_watch_entry_history);
	expr_watch_entry_history = g_strdup (row[0]);

	idx = debugger.watch->current_index;

	// internal debugger-list stuff:
	exp = g_list_nth_data (debugger.watch->exprs, idx);
	debugger.watch->exprs = g_list_remove (debugger.watch->exprs, exp);
	debugger.watch->exprs =
		g_list_insert (debugger.watch->exprs, g_strdup (row[0]), idx);

	// Visible watch-list stuff:
	row[1] = g_strdup ("");
	gtk_clist_remove (GTK_CLIST (debugger.watch->widgets.clist), idx);
	gtk_clist_insert (GTK_CLIST (debugger.watch->widgets.clist), idx, row);
	// Make watch-screen update:
	/* buff = g_strconcat ("print ", row[0], NULL); debugger_put_cmd_in_queqe
	 * (buff, DB_CMD_NONE, expr_watch_update, debugger.watch); g_free (buff);
	 */
	expr_watch_cmd_queqe (debugger.watch);
	g_free (row[1]);
	debugger_execute_cmd_in_queqe ();
}

static void
add_watch_entry (GtkEntry * ent)
{
	gchar *row[2], *buff;

	if (GTK_IS_ENTRY (ent) == FALSE)
		return;
	row[0] = (gchar *) gtk_entry_get_text (ent);
	if (strlen (row[0]) == 0)
		return;

	if (expr_watch_entry_history)
		g_free (expr_watch_entry_history);
	expr_watch_entry_history = g_strdup (row[0]);

	debugger.watch->exprs =
		g_list_append (debugger.watch->exprs, g_strdup (row[0]));
	row[1] = g_strdup ("");
	gtk_clist_append (GTK_CLIST (debugger.watch->widgets.clist), row);
	debugger.watch->count = g_list_length (debugger.watch->exprs) - 1;
	buff = g_strconcat ("print ", row[0], NULL);
	debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, expr_watch_update,
							   debugger.watch);
	g_free (buff);
	g_free (row[1]);
	debugger_execute_cmd_in_queqe ();
}

void
on_ew_add_ok_clicked (GtkButton * button, gpointer user_data)
{
	add_watch_entry ((GtkEntry *) user_data);
}

/*****************************************************************************/
void
on_eval_help_clicked (GtkButton * button, gpointer user_data)
{

}

void
on_eval_entry_activate (GtkWidget * wid, gpointer user_data)
{
	on_eval_ok_clicked (NULL, wid);
	gtk_widget_destroy (GTK_WIDGET (user_data));
}

void
on_eval_ok_clicked (GtkButton * button, gpointer user_data)
{
	GtkEntry *ent;
	const gchar *buff1;
	gchar *buff2;

	ent = (GtkEntry *) user_data;
	buff1 = gtk_entry_get_text (ent);
	if (strlen (buff1) == 0)
		return;
	if (eval_entry_history)
		g_free (eval_entry_history);
	eval_entry_history = g_strdup (buff1);

	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set verbose off", DB_CMD_NONE, NULL, NULL);
	buff2 = g_strconcat ("print ", buff1, NULL);
	debugger_put_cmd_in_queqe (buff2, DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
							   eval_output_arrived, g_strdup (buff1));
	debugger_put_cmd_in_queqe ("set verbose on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL, NULL);
	g_free (buff2);
	debugger_execute_cmd_in_queqe ();
}

void
on_eval_add_watch (GtkButton * button, gpointer user_data)
{
	add_watch_entry ((GtkEntry *) user_data);
}

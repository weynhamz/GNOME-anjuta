/*
    stack_trace_cbs.c
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
#include "debugger.h"
#include "stack_trace_cbs.h"

extern GdkPixmap *pointer_pix;
extern GdkBitmap *pointer_pix_mask;

extern GdkPixmap *blank_pix;
extern GdkBitmap *blank_pix_mask;

gint
on_stack_trace_window_delete_event (GtkWidget * w,
				    GdkEvent * event, gpointer data)
{
	StackTrace *cr = data;
	stack_trace_hide (cr);
	return TRUE;
}

gboolean
on_stack_trace_event (GtkWidget * widget,
		      GdkEvent * event, gpointer user_data)
{
	StackTrace *st = user_data;
	if (event->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent = (GdkEventButton *) event;
		if (bevent->button != 3)
			return FALSE;
		bevent->button = 1;
		stack_trace_update_controls (st);
		gtk_menu_popup (GTK_MENU (st->widgets.menu), NULL,
				NULL, NULL, NULL,
				bevent->button, bevent->time);
		return TRUE;
	}
	else
		return FALSE;
}

void
on_stack_trace_clist_select_row (GtkCList * clist,
				 gint row,
				 gint column,
				 GdkEvent * event, gpointer user_data)
{
	StackTrace *cr;
	cr = (StackTrace *) user_data;
	if (cr)
	{
		cr->current_index = row;
		if (event == NULL)
			return;
		if (event->type == GDK_2BUTTON_PRESS)
		{
			if (((GdkEventButton *) event)->button == 1)
				on_stack_frame_set_activate (NULL, NULL);
		}
	}
}

void
on_stack_trace_clist_unselect_row (GtkCList * clist,
				   gint row,
				   gint column,
				   GdkEvent * event, gpointer user_data)
{
	StackTrace *cr;
	cr = (StackTrace *) user_data;
	cr->current_index = -1;
}

void
on_stack_frame_set_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace *cr;
	gchar *cmd;
	cr = debugger.stack;
	if (cr->current_index < 0)
		return;

	cmd = g_strdup_printf ("frame %d", cr->current_index);
	debugger_put_cmd_in_queqe (cmd, 0, NULL, NULL);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info all-registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);

	debugger_execute_cmd_in_queqe ();
	g_free (cmd);
	gtk_clist_set_pixmap (GTK_CLIST (cr->widgets.clist), cr->current_frame,
			      0, blank_pix, blank_pix_mask);
	gtk_clist_set_pixmap (GTK_CLIST (cr->widgets.clist), cr->current_index,
			      0, pointer_pix, pointer_pix_mask);
	cr->current_frame = cr->current_index;
}

void
on_stack_frame_info_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("info frame", 0, debugger_dialog_message,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}


void
on_stack_update_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);
	debugger_execute_cmd_in_queqe ();
}

void
on_stack_view_src_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}

void
on_stack_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}

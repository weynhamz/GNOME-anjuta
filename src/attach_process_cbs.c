/*
    attach_process_cbs.h
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

#include "attach_process.h"
#include "attach_process_cbs.h"
#include "debugger.h"

gboolean
on_attach_process_close(GtkWidget* win, gpointer data)
{
	AttachProcess* ap = data;
	attach_process_hide(ap);
	return FALSE;
}

void
on_attach_process_clist_select_row(GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	AttachProcess* ap = user_data;
	gchar* text;
	gtk_clist_get_text(GTK_CLIST(clist), row, 0, &text);
	ap->pid = atoi(text);
}

void
on_attach_process_update_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;
	attach_process_update(ap);
}

void
on_attach_process_attach_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;
    gnome_dialog_close(GNOME_DIALOG(ap->widgets.window));
	if(ap->pid < 0) return;
	debugger_attach_process(ap->pid);
}

void
on_attach_process_cancel_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;
	if (NULL != ap)
        gnome_dialog_close(GNOME_DIALOG(ap->widgets.window));
}

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

extern int PID_COLUMN;

gboolean
on_attach_process_close(GtkWidget* win, gpointer data)
{
	AttachProcess* ap = data;

	attach_process_hide (ap);

	return FALSE;
}

void
on_attach_process_tv_event (GtkWidget *w,
			    GdkEvent  *event,
			    gpointer   data)
{
	AttachProcess *ap = data;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar* text;

	g_return_if_fail (GTK_IS_TREE_VIEW (w));

	view = GTK_TREE_VIEW (w);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return ;

	gtk_tree_mode_get (model, &iter,
			   PID_COLUMN, &text,
			   -1);

	ap->pid = atoi(text);
}

void
on_attach_process_update_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;

	attach_process_update (ap);
}

void
on_attach_process_attach_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;

	gtk_widget_hide (ap->widgets.window);

	if (ap->pid < 0)
		return ;

	debugger_attach_process (ap->pid);
}

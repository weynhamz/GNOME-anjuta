/*
    message-manager-dock.c
    Copyright (C) 2000, 2001  Kh. Naba Kumar Singh, Johannes Schmid

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

#include "anjuta.h"
#include "message-manager-dock.h"

static gboolean on_delete_event (GtkWidget * widget, GdkEvent * event,
				 gpointer data);

void
amm_undock (GtkWidget * amm, GtkWidget ** window)
{
	g_return_if_fail (amm != NULL);

	if (amm->parent != NULL)
		amm_hide_docked ();

	gtk_window_add_accel_group (GTK_WINDOW (*window), app->accel_group);
	gtk_signal_connect (GTK_OBJECT (*window), "delete_event",
			    GTK_SIGNAL_FUNC (on_delete_event), amm);

	gtk_widget_ref (amm);
	gtk_container_remove (GTK_CONTAINER (app->widgets.mesg_win_container),
			      amm);
	gtk_container_add (GTK_CONTAINER (*window), amm);
	gtk_widget_unref (amm);
}

void
amm_dock (GtkWidget * amm, GtkWidget ** window)
{
	g_return_if_fail (amm != NULL);

	gtk_widget_hide (amm);
	gtk_widget_ref (amm);
	if (*window != NULL)
	{
		gtk_container_remove (GTK_CONTAINER (*window), amm);
		gtk_widget_destroy (*window);
	}
	gtk_container_add (GTK_CONTAINER (app->widgets.mesg_win_container),
			   amm);
	gtk_widget_unref (amm);
	gtk_widget_show (app->widgets.mesg_win_container);
}


/*
	Here we handle juggling the user interface to hide/show the message pane.
	
	The use of gtk_widget_reparent() here is very important.   See the comments
	in project_dbase.c for more info.
*/

void
amm_hide_docked (void)
{
	gtk_widget_hide (app->widgets.mesg_win_container);

	gtk_widget_reparent (app->widgets.hpaned_client, app->widgets.client_area);
	gtk_container_remove (GTK_CONTAINER (app->widgets.client_area), app->widgets.vpaned);

	app->widgets.the_client = app->widgets.hpaned_client;
}

void
amm_show_docked (void)
{
	gtk_container_add (GTK_CONTAINER (app->widgets.client_area), app->widgets.vpaned);
	gtk_widget_reparent (app->widgets.hpaned_client, app->widgets.vpaned);

	gtk_widget_show_all (app->widgets.mesg_win_container);

	app->widgets.the_client = app->widgets.vpaned;
}


static gboolean
on_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (data));
	return TRUE;
}

// Function must be here because I cannot include anjuta.h in c++ files
Preferences *
get_preferences ()
{
	return app->preferences;
}

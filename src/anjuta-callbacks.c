/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-callbacks.c
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
#include <signal.h>
#include <string.h>

#include <gnome.h>

#include <libanjuta/resources.h>

#include "anjuta-app.h"
#include "anjuta-callbacks.h"

#if 0

static void
build_msg_save_real (void)
{
	gchar *filename;
	FILE *msgfile; 
	
	filename = fileselection_get_filename (app->save_as_build_msg_sel);
	msgfile = fopen(filename, "w");
	if (! msgfile)
	{
		anjuta_error (_("Could not open file for writing"));
		return;
	}
	
	an_message_manager_save_build(app->messages, msgfile);
			
	fclose(msgfile);

	return;
}

void
on_build_msg_save_ok_clicked(GtkButton * button, gpointer user_data)
{
	gchar *filename;

	filename = fileselection_get_filename (app->save_as_build_msg_sel);
	if (file_is_regular (filename))
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new (GTK_WINDOW (app),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE,
										 _("The file '%s' already exists.\n"
										 "Do you want to replace it with the one you are saving?."),
										 filename);
		gtk_dialog_add_button (GTK_DIALOG (dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_dialog_add_button (GTK_DIALOG (dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			build_msg_save_real ();
		gtk_widget_destroy (dialog);
	}
	else
		build_msg_save_real ();
	g_free (filename);
}

void
on_build_msg_save_cancel_clicked(GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->save_as_build_msg_sel);
	closing_state = FALSE;
}

void
on_anjuta_progress_cancel (gpointer data)
{
	app->in_progress = FALSE;
	app->progress_cancel_cb (data);
	return;
}

gdouble
on_anjuta_progress_cb (gpointer data)
{
	return (app->progress_value);
}
#endif

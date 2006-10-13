/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
  * mainmenu_callbacks.c
  * Copyright (C) 2003  Naba Kumar  <naba@gnome.org>
  * 
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  * 
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <sys/wait.h>
#include <errno.h>

#include <gnome.h>

#include <libgnomeui/gnome-window-icon.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/plugins.h>

#include "anjuta-app.h"
// #include "help.h"
#include "about.h"
#include "action-callbacks.h"
#include "anjuta.h"

void
on_exit1_activate (GtkAction * action, AnjutaApp *app)
{
	GdkEvent *event = gdk_event_new (GDK_DELETE);

	event->any.window = g_object_ref (GTK_WIDGET(app)->window);
	event->any.send_event = TRUE;
  
	gtk_main_do_event (event);
	gdk_event_free (event);
}

void
on_fullscreen_toggle (GtkAction *action, AnjutaApp *app)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gtk_window_fullscreen (GTK_WINDOW(app));
	else
		gtk_window_unfullscreen (GTK_WINDOW(app));
}

void
on_layout_lock_toggle (GtkAction *action, AnjutaApp *app)
{
	if (app->layout_manager)
		g_object_set (app->layout_manager->master, "locked",
					  gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)),
					  NULL);
}

void
on_reset_layout_activate(GtkAction *action, AnjutaApp *app)
{
	anjuta_app_layout_reset (app);
}

void
on_set_preferences1_activate (GtkAction * action, AnjutaApp *app)
{
	gtk_widget_show (GTK_WIDGET (app->preferences));
}

void
on_set_default_preferences1_activate (GtkAction * action,
				      AnjutaApp *app)
{
	anjuta_preferences_reset_defaults (ANJUTA_PREFERENCES (app->preferences));
}

void
on_customize_shortcuts_activate(GtkAction *action, AnjutaApp *app)
{
	GtkWidget *win, *accel_editor;
	
	accel_editor = anjuta_ui_get_accel_editor (ANJUTA_UI (app->ui));
	win = gtk_dialog_new_with_buttons (_("Anjuta Plugins"), GTK_WINDOW (app),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CLOSE, GTK_STOCK_CANCEL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG(win)->vbox), accel_editor);
	gtk_window_set_default_size (GTK_WINDOW (win), 500, 400);
	gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);
}

void
on_show_plugins_activate (GtkAction *action, AnjutaApp *app)
{
	GtkWidget *win, *plg;
	
	plg = anjuta_plugins_get_installed_dialog (ANJUTA_SHELL (app));
	win = gtk_dialog_new_with_buttons (_("Anjuta Plugins"), GTK_WINDOW (app),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CLOSE, GTK_STOCK_CANCEL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG(win)->vbox), plg);
	gtk_window_set_default_size (GTK_WINDOW(win), 500, 550);
	gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);
}

static void
help_activate (const gchar *item)
{
	if (gnome_help_display (item, NULL, NULL) == FALSE)
	{
	  anjuta_util_dialog_error (NULL, _("Unable to display help. Please make sure Anjuta documentation package is install. It can be downloaded from http://anjuta.org"));	
	}
}

void
on_help_manual_activate (GtkAction *action, gpointer data)
{
	help_activate ("anjuta-manual.xml");
}

void
on_help_tutorial_activate (GtkAction *action, gpointer data)
{
	help_activate ("anjuta-tutorial.xml");
}

void
on_help_advanced_tutorial_activate (GtkAction *action, gpointer data)
{
	help_activate ("anjuta-advanced-tutorial.xml");
}

void
on_help_faqs_activate (GtkAction *action, gpointer data)
{
	help_activate ("anjuta-faqs.xml");
}

void
on_url_home_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://www.anjuta.org");
}

void
on_url_bugs_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("http://bugzilla.gnome.org/simple-bug-guide.cgi");
}

void
on_url_faqs_activate (GtkAction * action, gpointer user_data)
{
	anjuta_res_url_show("mailto:anjuta-list@lists.sourceforge.net");
}

void
on_about_activate (GtkAction * action, gpointer user_data)
{
	GtkWidget *about_dlg = about_box_new ();

	g_signal_connect_swapped(about_dlg, "response",
		G_CALLBACK(gtk_widget_destroy), about_dlg);

	gtk_widget_show (about_dlg);
}

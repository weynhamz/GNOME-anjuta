/*
 * configurer.c Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include <gnome.h>

#include "anjuta.h"
#include "text_editor.h"
#include "messagebox.h"
#include "utilities.h"
#include "launcher.h"
#include "configurer.h"
#include "resources.h"

static GtkWidget* create_configurer_dialog(Configurer* c);

static void on_configurer_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data);
static void on_configurer_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data);
static void conf_mesg_arrived(gchar *mesg);
static void conf_terminated(int status, time_t t);

Configurer *
configurer_new (PropsID props)
{
	Configurer *c = malloc (sizeof (Configurer));
	if (c)
	{
		c->props = props;
	}
	return c;
}

void
configurer_destroy (Configurer * c)
{
	if (c) g_free (c);
}

void
configurer_show (Configurer * c)
{
	gtk_widget_show (create_configurer_dialog (c));
}

static GtkWidget *
create_configurer_dialog (Configurer * c)
{
	GtkWidget *dialog2;
	GtkWidget *dialog_vbox2;
	GtkWidget *frame2;
	GtkWidget *vbox2;
	GtkWidget *frame3;
	GtkWidget *combo2;
	GtkWidget *combo_entry2;
	GtkWidget *dialog_action_area2;
	GtkWidget *button4;
	GtkWidget *button6;
	gchar* options;

	dialog2 = gnome_dialog_new (_("Configure"), NULL);
	gtk_widget_set_usize (dialog2, 400, -2);
	gtk_window_set_position (GTK_WINDOW (dialog2), GTK_WIN_POS_CENTER);
	gtk_window_set_policy (GTK_WINDOW (dialog2), FALSE, FALSE, FALSE);
	gnome_dialog_set_close (GNOME_DIALOG (dialog2), TRUE);

	dialog_vbox2 = GNOME_DIALOG (dialog2)->vbox;
	gtk_widget_show (dialog_vbox2);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (dialog_vbox2), frame2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame2), vbox2);

	frame3 = gtk_frame_new (_("Configure Options (if any)"));
	gtk_widget_show (frame3);
	gtk_box_pack_start (GTK_BOX (vbox2), frame3, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 5);

	combo2 = gtk_combo_new ();
	gtk_widget_show (combo2);
	gtk_container_add (GTK_CONTAINER (frame3), combo2);
	gtk_container_set_border_width (GTK_CONTAINER (combo2), 5);

	combo_entry2 = GTK_COMBO (combo2)->entry;
	gtk_widget_show (combo_entry2);

	dialog_action_area2 = GNOME_DIALOG (dialog2)->action_area;
	gtk_widget_show (dialog_action_area2);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area2),
				   GTK_BUTTONBOX_EDGE);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area2), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog2),
				    GNOME_STOCK_BUTTON_OK);
	button4 = g_list_last (GNOME_DIALOG (dialog2)->buttons)->data;
	gtk_widget_show (button4);
	GTK_WIDGET_SET_FLAGS (button4, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog2),
				    GNOME_STOCK_BUTTON_CANCEL);
	button6 = g_list_last (GNOME_DIALOG (dialog2)->buttons)->data;
	gtk_widget_show (button6);
	GTK_WIDGET_UNSET_FLAGS (button6, GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS (button6, GTK_CAN_DEFAULT);

	options = prop_get (c->props, "project.configure.options");
	if (options)
	{
		gtk_entry_set_text (GTK_ENTRY (combo_entry2), options);
		g_free (options);
	}

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (dialog2));

	gtk_signal_connect (GTK_OBJECT (combo_entry2), "changed",
			    GTK_SIGNAL_FUNC (on_configurer_entry_changed), c);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_configurer_ok_clicked), c);

	return dialog2;
}

static void
on_configurer_entry_changed (GtkEditable * editable, gpointer user_data)
{
	Configurer *c = user_data;
	gchar *options;
	options = gtk_entry_get_text (GTK_ENTRY (editable));
	if (options)
		prop_set_with_key (c->props, "project.configure.options",
				   options);
	else
		prop_set_with_key (c->props, "project.configure.options", "");
}

static void
on_configurer_ok_clicked (GtkButton * button, gpointer user_data)
{
	Configurer *cof;
	gchar *tmp, *options;
	cof = user_data;

	g_return_if_fail (cof != NULL);
	g_return_if_fail (app->project_dbase->project_is_open);
	
	chdir (app->project_dbase->top_proj_dir);
	if (file_is_executable ("./configure") == FALSE)
	{
		anjuta_error (_
			      ("Project does not have executable configure script.\n"
			       "You need to autogen the project first."));
		return;
	}
	options = prop_get (cof->props, "project.configure.options");
	if (options)
	{
		tmp = g_strconcat ("./configure ", options, NULL);
		g_free (options);
	}
	else
	{
		tmp = g_strdup ("./configure ");
	}
	if (launcher_execute (tmp, conf_mesg_arrived, conf_mesg_arrived, conf_terminated) == FALSE)
	{
		anjuta_error ("Project configuration failed.");
		g_free (tmp);
		return;
	}
	g_free (tmp);
	anjuta_update_app_status (TRUE, _("Configure"));
	messages_clear (app->messages, MESSAGE_BUILD);
	messages_append (app->messages, _("Configuring the project ....\n"), MESSAGE_BUILD);
	messages_show (app->messages, MESSAGE_BUILD);
}

static void
conf_mesg_arrived (gchar * mesg)
{
	messages_append (app->messages, mesg, MESSAGE_BUILD);
}

static void
conf_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		messages_append (app->messages,
				 _
				 ("Configure completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		anjuta_warning (_("Configure completed ... unsuccessful"));
	}
	else
	{
		messages_append (app->messages,
				 _
				 ("Configure completed...............Successful\n"),
				 MESSAGE_BUILD);
		anjuta_status (_("Configure completed ... successful"));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (gint) time);
	messages_append (app->messages, buff1, MESSAGE_BUILD);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}

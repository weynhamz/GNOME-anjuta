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
#include "message-manager.h"
#include "text_editor.h"
#include "utilities.h"
#include "launcher.h"
#include "configurer.h"
#include "resources.h"

static GtkWidget* create_configurer_dialog(Configurer* c);
static void on_configurer_ok_clicked (GtkButton *button, gpointer data);
static void on_configurer_entry_changed (GtkEditable *editable, gpointer data);
static void on_configurer_environment_changed (GtkEditable * editable, gpointer user_data);

static void conf_mesg_arrived(gchar *mesg);
static void conf_terminated(int status, time_t t);

Configurer *
configurer_new (PropsID props)
{
	Configurer *c = malloc (sizeof (Configurer));
	if (c) {
		c->props = props;
		c->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "configurer_dialog", NULL);
		gtk_widget_hide (glade_xml_get_widget (c->gxml, "configurer_dialog"));
		glade_xml_signal_autoconnect (c->gxml);
	}
	return c;
}

void
configurer_destroy (Configurer * c)
{
	if (c) {
		g_object_unref (c->gxml);
		g_free (c);
	}
}

void
configurer_show (Configurer * c)
{
	gtk_widget_show (create_configurer_dialog (c));
}

static GtkWidget *
create_configurer_dialog (Configurer * c)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *ok_button;
	gchar *options;
	
	dialog = glade_xml_get_widget (c->gxml, "configurer_dialog");
	ok_button = glade_xml_get_widget (c->gxml, "configurer_ok_button");
	g_signal_connect (G_OBJECT (ok_button), "clicked",
			    G_CALLBACK (on_configurer_ok_clicked), c);
	
	entry = glade_xml_get_widget (c->gxml, "configurer_entry");
	options = prop_get (c->props, "project.configure.options");
	if (options)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), options);
		g_free (options);
	}
	g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (on_configurer_entry_changed), c);
	
	entry = glade_xml_get_widget (c->gxml, "configurer_environment_entry");
	options = prop_get (c->props, "project.configure.environment");
	if (options)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), options);
		g_free (options);
	}
	g_signal_connect (G_OBJECT (entry), "changed",
			    G_CALLBACK (on_configurer_environment_changed), c);

	/* gtk_accel_group_attach (app->accel_group, GTK_OBJECT (dialog)); */
	return dialog;
}

static void
on_configurer_entry_changed (GtkEditable * editable, gpointer user_data)
{
	Configurer *c = (Configurer *) user_data;
	const gchar *options;
	options = gtk_entry_get_text (GTK_ENTRY (editable));
	if (options)
		prop_set_with_key (c->props, "project.configure.options",
				   options);
	else
		prop_set_with_key (c->props, "project.configure.options", "");
}

static void
on_configurer_environment_changed (GtkEditable * editable, gpointer user_data)
{
	Configurer *c = (Configurer *) user_data;
	const gchar *options;
	options = gtk_entry_get_text (GTK_ENTRY (editable));
	if (options)
		prop_set_with_key (c->props, "project.configure.environment",
				   options);
	else
		prop_set_with_key (c->props, "project.configure.environment", "");
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
			      ("Project does not have an executable configure script.\n"
			       "Auto generate the Project first."));
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
	options = prop_get(cof->props, "project.configure.environment");
	if (options)
	{
		gchar *tmp1 = g_strdup_printf("sh -c '%s %s'", options, tmp);
		g_free(tmp);
		tmp = tmp1;
	}
#ifdef DEBUG
	g_message("Executing '%s'\n", tmp);
#endif
	if (launcher_execute (tmp, conf_mesg_arrived,
		conf_mesg_arrived, conf_terminated) == FALSE)
	{
		anjuta_error ("Project configuration failed.");
		g_free (tmp);
		return;
	}
	g_free (tmp);
	anjuta_update_app_status (TRUE, _("Configure"));
	anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
	anjuta_message_manager_append (app->messages, _("Configuring the Project ....\n"), MESSAGE_BUILD);
	anjuta_message_manager_show (app->messages, MESSAGE_BUILD);
}

static void
conf_mesg_arrived (gchar * mesg)
{
	anjuta_message_manager_append (app->messages, mesg, MESSAGE_BUILD);
}

static void
conf_terminated (int status, time_t time)
{
	gchar *buff1;

	if (status)
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Configure completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		anjuta_warning (_("Configure completed ... unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Configure completed...............Successful\n"),
				 MESSAGE_BUILD);
		anjuta_status (_("Configure completed ... successful"));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (gint) time);
	anjuta_message_manager_append (app->messages, buff1, MESSAGE_BUILD);
	if (anjuta_preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}

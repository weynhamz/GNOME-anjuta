/*
 * start-with.c Copyright (C) 2003  Naba Kumar
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

#include <gtk/gtkdialog.h>
#include <glade/glade.h>
#include "anjuta.h"
#include "start-with.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta.glade"

static void
on_application_wizard_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET (data);
	gtk_signal_emit_by_name (GTK_OBJECT (app->widgets.menubar.file.new_project),
							 "activate", NULL);
	gtk_widget_destroy (dialog);
}

static void
on_import_wizard_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET (data);
	gtk_signal_emit_by_name (GTK_OBJECT (app->widgets.menubar.file.import_project),
							 "activate", NULL);
	gtk_widget_destroy (dialog);
}

static void
on_open_project_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET (data);
	gtk_signal_emit_by_name (GTK_OBJECT (app->widgets.menubar.file.open_project),
							 "activate", NULL);
	gtk_widget_destroy (dialog);
}

static void
on_open_last_project_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog;	
	/*  Do not allow a second click  */
	gchar *prj_filename;

	dialog  = GTK_WIDGET (data);
	gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
	prj_filename = anjuta_preferences_get (app->preferences,
												  "anjuta.last.open.project");
	project_dbase_load_project (app->project_dbase, prj_filename, TRUE);
	g_free (prj_filename);
	gtk_widget_destroy (dialog);
}

static void
on_open_file_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET (data);
	gtk_signal_emit_by_name (GTK_OBJECT (app->widgets.menubar.file.open_file),
							 "activate", NULL);
	gtk_widget_destroy (dialog);
}

static void
on_new_file_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET (data);
	gtk_signal_emit_by_name (GTK_OBJECT (app->widgets.menubar.file.new_file),
							 "activate", NULL);
	gtk_widget_destroy (dialog);
}

static void
on_do_not_show_again_toggled (GtkToggleButton *button, AnjutaPreferences *p)
{
	gboolean state;
	state = gtk_toggle_button_get_active (button);
	anjuta_preferences_set_int (p, "donotshow.start.with.dialog", state);
}

static void
on_dialog_response (GtkWidget *dialog, gint response, gpointer data)
{
	gtk_widget_destroy (dialog);
}

void
start_with_dialog_show (GtkWindow *parent, AnjutaPreferences *pref,
						gboolean force)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *button;
	gboolean do_not_show;
	gboolean reload_last_project;
	gchar *last_project;
	
	do_not_show = anjuta_preferences_get_int (pref, "donotshow.start.with.dialog");
	reload_last_project = anjuta_preferences_get_int (pref, "reload.last.project");
	last_project = anjuta_preferences_get (pref, "anjuta.last.open.project");

	/* Return if the dialog is not to be shown */
	if (!force && do_not_show)
		return;
	
	/* Return if preference for loading last project is set */
	if (!force && reload_last_project &&
		last_project && file_is_readable (last_project))
		return;
	
	gxml = glade_xml_new (GLADE_FILE, "start_with_dialog", NULL);
	g_return_if_fail (gxml != NULL);
	
	dialog = glade_xml_get_widget (gxml, "start_with_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (on_dialog_response), pref);
	
	button = glade_xml_get_widget (gxml, "application_wizard_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_application_wizard_clicked), dialog);
	
	button = glade_xml_get_widget (gxml, "import_wizard_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_import_wizard_clicked), dialog);
	
	button = glade_xml_get_widget (gxml, "open_project_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_open_project_clicked), dialog);
	
	button = glade_xml_get_widget (gxml, "open_last_project_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_open_last_project_clicked), dialog);
	
	if (last_project &&	file_is_readable (last_project))
		gtk_widget_set_sensitive (button, TRUE);
	else
		gtk_widget_set_sensitive (button, FALSE);
	
	button = glade_xml_get_widget (gxml, "open_file_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_open_file_clicked), dialog);
	
	button = glade_xml_get_widget (gxml, "new_file_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_new_file_clicked), dialog);
	
	button = glade_xml_get_widget (gxml, "do_not_show_button");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), do_not_show);
	g_signal_connect (G_OBJECT (button), "toggled",
					  G_CALLBACK (on_do_not_show_again_toggled), pref);
	
	g_object_unref (gxml);
	g_free (last_project);
	gtk_widget_show (dialog);
}

void
start_with_dialog_save_yourself (AnjutaPreferences *pref, FILE *fp)
{
	gint state = anjuta_preferences_get_int (pref, "donotshow.start.with.dialog");
	fprintf (fp, "donotshow.start.with.dialog=%d\n", state);
}

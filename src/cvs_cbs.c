/*  cvs_gui.c (c) Johannes Schmid 2002
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "cvs_cbs.h"
#include "cvs.h"
#include "anjuta.h"

#include <time.h>

/*
	Called when the user clicks apply on the settings dialog.
	Saves the values to CVS structure.
	Return value: TRUE, every value was set correctly,
				  FALSE, some value was missing.
	A warning dialog is printed if no type was selected.
*/

gboolean
on_cvs_settings_apply (GtkWidget * button, CVSSettingsGUI * gui)
{
	guint i;
	gchar *type;
	CVS *cvs = app->cvs;

	g_return_val_if_fail (cvs != NULL, FALSE);
	g_return_val_if_fail (gui != NULL, FALSE);

	type = g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO
							(gui->
							 combo_server_type)->
							entry)));
	for (i = 0; i < 4; i++)
	{
		if (strcmp (server_types[i], type) == 0)
		{
			cvs_set_server_type (cvs, i);
			break;
		}
	}
	g_free (type);
	if (i == 4)
	{
		gnome_ok_dialog (_("You need to select a server type!"));
		return TRUE;
	}


	cvs_set_server (cvs,
			gtk_entry_get_text (GTK_ENTRY (gui->entry_server)));
	cvs_set_directory (cvs,
			   gtk_entry_get_text (GTK_ENTRY
					       (gui->entry_server_dir)));
	cvs_set_username (cvs,
			  gtk_entry_get_text (GTK_ENTRY
					      (gui->entry_username)));
	cvs_set_passwd (cvs,
			gtk_entry_get_text (GTK_ENTRY (gui->entry_passwd)));
	cvs_set_compression (cvs,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (gui->
								spin_compression)));

	return FALSE;
}

/* 
	This is called if the users clicks the ok button of the settings
	dialog. It tries to set the values corrently. If the users did
	not set a neccessary value the dialog stays on the screen. Otherwise
	it is destroyed
*/

void
on_cvs_settings_ok (GtkWidget * button, CVSSettingsGUI * gui)
{
	gboolean destroy = on_cvs_settings_apply (button, gui);

	if (destroy)
	{
		gtk_widget_hide (gui->dialog);
		gtk_widget_destroy (gui->dialog);
		g_free (gui);
	}
}

/* 
	This is called when the users clicks the cancel button of 
	the settings dialog. Hides and destroys the dialog
*/

void
on_cvs_settings_cancel (GtkWidget * button, CVSSettingsGUI * gui)
{
	gtk_widget_hide (gui->dialog);
	gtk_widget_destroy (gui->dialog);
	g_free (gui);
}

/* 
	This is called if one of the entries of the settings dialog
	is changed. It set the modified bit of the propertybox.
	If server type == LOCAL then all entries which have no use
	in LOCAL mode are hidden.
*/

void
on_entry_changed (GtkWidget * entry, CVSSettingsGUI * gui)
{
	gchar *type;
	gboolean sensitive;

	g_return_if_fail (gui != NULL);

	type = strdup (gtk_entry_get_text
		       (GTK_ENTRY
			(GTK_COMBO (gui->combo_server_type)->entry)));

	// TRUE = !0 => do not hide, FALSE = 0 => hide
	sensitive = strcmp (type, server_types[CVS_LOCAL]);

	gtk_widget_set_sensitive (gui->entry_server, sensitive);
	gtk_widget_set_sensitive (gui->entry_username, sensitive);
	gtk_widget_set_sensitive (gui->entry_passwd, sensitive);
	gtk_widget_set_sensitive (gui->spin_compression, sensitive);

	g_free (type);
}

/*
	Called when the user presses ok in the cvs file dialog.
	Starts the appropriate cvs action to start by checking the type
	of the dialog. Destroys the dialog afterwards.
	If entry is empty nothing happens.
*/

void
on_cvs_ok (GtkWidget * button, CVSFileGUI * gui)
{
	gchar *filename;
	gchar *branch;
	gchar *message = NULL;
	gboolean is_dir = FALSE;

	GtkWidget *gtk_entry_file;
	GtkWidget *gtk_entry_branch;

	g_return_if_fail (gui != NULL);

	gtk_entry_file =
		gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY
					    (gui->entry_file));
	gtk_entry_branch =
		gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_branch));

	filename = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_entry_file)));
	branch = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_entry_branch)));
	message =
		g_strdup (gtk_editable_get_chars
			  (GTK_EDITABLE (gui->text_message), 0,
			   gtk_text_get_length (GTK_TEXT
				(gui->text_message))));

	if (strlen (filename) == 0)
		return;

	is_dir = file_is_directory(filename);
	
	switch (gui->type)
	{
	case CVS_ACTION_UPDATE:
		cvs_update (app->cvs, filename, branch, is_dir);
		break;
	case CVS_ACTION_COMMIT:
		cvs_commit (app->cvs, filename, branch, message, is_dir);
		break;
	case CVS_ACTION_STATUS:
		cvs_status (app->cvs, filename, is_dir);
		break;
	case CVS_ACTION_LOG:
		cvs_log (app->cvs, filename, is_dir);
		break;
	case CVS_ACTION_ADD:
		cvs_add_file (app->cvs, filename, message);
		break;
	case CVS_ACTION_REMOVE:
		cvs_remove_file(app->cvs, filename);
		break;
	default:
		g_warning ("Invalid dialog type %d !", gui->type);
		return;
	}

	g_free (filename);
	g_free (branch);
	g_free (message);
	
	/* Destroy dialog */
	on_cvs_cancel (button, gui);
}

/*
	Called when the user presses cancel in the cvs file dialog.
	Destroys the dialog.
*/

void
on_cvs_cancel (GtkWidget * button, CVSFileGUI * gui)
{
	g_return_if_fail (gui != NULL);

	gtk_widget_hide (gui->dialog);
	gtk_widget_destroy (gui->dialog);
	g_free (gui);
}

/*
	Called when the user presses the "Diff" button in the File Diff
	Dialog. Starts cvs diff if file != "" and closes and destroys
	the dialog.
*/

void on_cvs_diff_ok (GtkWidget* button, CVSFileDiffGUI * gui)
{
	gchar* filename;
	gchar* revision;
	time_t time;
	gboolean unified;
	
	GtkWidget* gtk_file_entry;
	GtkWidget* gtk_rev_entry;
	
	g_return_if_fail(gui != NULL);
	
	gtk_file_entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(gui->entry_file));
	gtk_rev_entry = gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_rev));
	
	filename = gtk_entry_get_text(GTK_ENTRY(gtk_file_entry));
	revision = gtk_entry_get_text(GTK_ENTRY(gtk_rev_entry));
	time = gnome_date_edit_get_date(GNOME_DATE_EDIT(gui->entry_date));
	unified = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->check_unified));
	
	if (strlen(filename) > 0)
	{
		gboolean is_dir;
		is_dir = file_is_directory(filename);
		cvs_diff(app->cvs, filename, revision, time, unified, is_dir);
	}
	on_cvs_diff_cancel(button, gui);
}

/*
	Closes the dialog and destroys it.
*/

void on_cvs_diff_cancel (GtkWidget* button, CVSFileDiffGUI * gui)
{
	g_return_if_fail (gui != NULL);
	
	gtk_widget_hide(gui->dialog);
	gtk_widget_destroy(gui->dialog);
	
	g_free(gui);
}


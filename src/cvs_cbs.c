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

static ServerType get_server_type (GtkEntry* entry);

void on_cvs_login_ok (GtkWidget* button, CVSLoginGUI* gui)
{
	ServerType stype = 0;
	gchar* server;
	gchar* user;
	gchar* dir;
	
	g_return_if_fail (gui != NULL);
	
	stype = get_server_type (GTK_ENTRY (GTK_COMBO
							(gui->
							 combo_type)->
							entry));
	
	if (stype == CVS_LOCAL)
	{
		gnome_ok_dialog (_("You do not need to login to a local server"));
		return;
	}
	server = gtk_entry_get_text( 
			GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY (gui->entry_server))));
	user = gtk_entry_get_text( 
			GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY (gui->entry_user))));
	dir = gtk_entry_get_text( 
			GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY (gui->entry_dir))));
	
	cvs_login (app->cvs, stype, server, dir, user);
	on_cvs_login_cancel (button, gui);
}

void on_cvs_login_cancel (GtkWidget* button, CVSLoginGUI* gui)
{
	gtk_widget_hide (gui->dialog);
	gtk_widget_destroy (gui->dialog);
	g_free (gui);
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

	filename = gtk_editable_get_chars (GTK_EDITABLE (gtk_entry_file), 0, -1);
	branch = gtk_editable_get_chars (GTK_EDITABLE (gtk_entry_branch), 0, -1);
	message = gtk_editable_get_chars (GTK_EDITABLE (gui->text_message), 0, -1);

	if (strlen (filename) == 0)
	{
		g_free(filename);
		g_free(branch);
		g_free(message);
		return;
	}

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
		break;
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
	gboolean state;
	
	GtkWidget* gtk_file_entry;
	GtkWidget* gtk_rev_entry;
	
	g_return_if_fail(gui != NULL);
	
	gtk_file_entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(gui->entry_file));
	gtk_rev_entry = gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_rev));
	
	filename = gtk_entry_get_text(GTK_ENTRY(gtk_file_entry));
	revision = gtk_entry_get_text(GTK_ENTRY(gtk_rev_entry));
	time = gnome_date_edit_get_date(GNOME_DATE_EDIT(gui->entry_date));
	state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->check_date));
	cvs_set_diff_use_date(app->cvs, state);
	if (strlen(filename) > 0)
	{
		gboolean is_dir;
		is_dir = file_is_directory(filename);
		cvs_diff(app->cvs, filename, revision, time, is_dir);
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

void on_cvs_import_ok (GtkWidget* button, CVSImportGUI * gui)
{
	ServerType type;
	gchar* server;
	gchar* user;
	gchar* dir;
	gchar* module;
	gchar* release;
	gchar* vendor;
	gchar* message;
	
	g_return_if_fail (gui != NULL);
	
	type = get_server_type (GTK_ENTRY (GTK_COMBO (gui->combo_type)->entry));
	
	server = gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_server))));
	dir = gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_dir))));
	user = gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_user))));
	module = gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_module))));
	vendor = gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_vendor))));
	release = gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY (gui->entry_release))));
	message = g_strdup (gtk_editable_get_chars
			  (GTK_EDITABLE (gui->text_message), 0,
			   gtk_text_get_length (GTK_TEXT
				(gui->text_message))));
	
	cvs_import_project (app->cvs, type, server, dir, user, module, 
						vendor, release, message);
	on_cvs_import_cancel(button, gui);
}

void on_cvs_import_cancel (GtkWidget* button, CVSImportGUI * gui)
{
	gtk_widget_hide (gui->dialog);
	gtk_widget_destroy (gui->dialog);
	g_free (gui);
}

void on_cvs_type_combo_changed (GtkWidget* entry, CVSImportGUI* gui)
{
	ServerType stype;
	
	g_return_if_fail (gui != NULL);
	
	stype = get_server_type (GTK_ENTRY(entry));
	switch (stype)
	{
		case CVS_LOCAL:
			gtk_widget_set_sensitive (gui->entry_user, FALSE);
			gtk_widget_set_sensitive (gui->entry_server, FALSE);
			break;
		case CVS_PASSWORD:
		case CVS_EXT:
		case CVS_SERVER:
		default:
			gtk_widget_set_sensitive (gui->entry_user, TRUE);
			gtk_widget_set_sensitive (gui->entry_server, TRUE);
	}
}

void on_cvs_diff_use_date_toggled (GtkToggleButton* b, CVSFileDiffGUI* gui)
{
	gboolean state;
	state = gtk_toggle_button_get_active(b);
	gtk_widget_set_sensitive(gui->entry_date, state);
}

/*
	Determines the server type selected in a GtkCombo according
	to the server_types array 
*/

static ServerType get_server_type (GtkEntry* entry)
{
	ServerType cur_type;
	gchar* type;
	
	type = g_strdup (gtk_entry_get_text (entry));
	for (cur_type = 0; cur_type < 4; cur_type++)
	{
		if (strcmp (server_types[cur_type], type) == 0)
		{
			g_free (type);
			return cur_type;
		}
	}
	
	// Not found
	g_free(type);
	return CVS_END;
}


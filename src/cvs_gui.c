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

#include <gnome.h>
#include <time.h>

#include "cvs_gui.h"
#include "cvs_cbs.h"
#include "anjuta.h"

/* Lables for server types */
gchar *server_types[4] =
	{ _("Local"), _("Password"), _("Extern(rsh)"), _("Server") };

/* Utility:
	Get the filename of the current open file from aneditor.
	Do not free return string!
*/

static gchar* get_cur_filename()
{
	TextEditor *te = anjuta_get_current_text_editor ();
	if (te == NULL || te->full_filename == NULL)
		return "";
	else
		return te->full_filename;
}
		

/* 
	Creates and show a dialog where server, username... for CVS 
	can be set.
*/

void
create_cvs_settings_dialog (CVS * cvs)
{
	CVSSettingsGUI *gui;
	GtkWidget *server_page_label;
	GtkWidget *table;

	GtkWidget *label_server_type;
	GList *string_list;
	int i;

	GtkWidget *label_server;
	GtkWidget *label_server_dir;
	GtkWidget *label_username;
	GtkWidget *label_passwd;
	GtkWidget *label_compression;
	GtkAdjustment *adj;

	gchar *server = cvs_get_server (cvs);
	gchar *server_dir = cvs_get_directory (cvs);
	ServerType server_type = cvs_get_server_type (cvs);
	gchar *username = cvs_get_username (cvs);
	gchar *passwd = cvs_get_passwd (cvs);
	guint compression = cvs_get_compression (cvs);
	string_list = g_list_alloc ();
	for (i = 0; i < 4; i++)
	{
		string_list = g_list_append (string_list, server_types[i]);
	}

	gui = g_new0 (CVSSettingsGUI, 1);

	gui->dialog = gnome_property_box_new ();
	gtk_window_set_wmclass (GTK_WINDOW (gui->dialog), "cvs-settings",
				"anjuta");
	gtk_window_set_title (GTK_WINDOW (gui->dialog), _("CVS Settings"));

	table = gtk_table_new (8, 2, TRUE);
	gtk_widget_show (table);

	label_server = gtk_label_new (_("CVS server: "));
	gui->entry_server = gtk_entry_new ();
	gtk_widget_show (label_server);
	gtk_widget_show (gui->entry_server);
	gtk_entry_set_text (GTK_ENTRY (gui->entry_server), server);
	gtk_signal_connect (GTK_OBJECT (gui->entry_server), "changed",
			    GTK_SIGNAL_FUNC (on_entry_changed), gui);
	gtk_table_attach_defaults (GTK_TABLE (table), label_server, 0, 1, 1,
				   2);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_server, 1, 2,
				   1, 2);

	label_server_dir = gtk_label_new (_("Directory on the server: "));
	gui->entry_server_dir = gtk_entry_new ();
	gtk_widget_show (label_server_dir);
	gtk_widget_show (gui->entry_server_dir);
	gtk_entry_set_text (GTK_ENTRY (gui->entry_server_dir), server_dir);
	gtk_signal_connect (GTK_OBJECT (gui->entry_server_dir), "changed",
			    GTK_SIGNAL_FUNC (on_entry_changed), gui);
	gtk_table_attach_defaults (GTK_TABLE (table), label_server_dir, 0, 1,
				   2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_server_dir,
				   1, 2, 2, 3);

	label_username = gtk_label_new (_("Username: "));
	gui->entry_username = gtk_entry_new ();
	gtk_widget_show (label_username);
	gtk_widget_show (gui->entry_username);
	gtk_entry_set_text (GTK_ENTRY (gui->entry_username), username);
	gtk_signal_connect (GTK_OBJECT (gui->entry_username), "changed",
			    GTK_SIGNAL_FUNC (on_entry_changed), gui);
	gtk_table_attach_defaults (GTK_TABLE (table), label_username, 0, 1, 4,
				   5);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_username, 1,
				   2, 4, 5);

	label_passwd = gtk_label_new (_("Password: "));
	gui->entry_passwd = gtk_entry_new ();
	gtk_widget_show (label_passwd);
	gtk_widget_show (gui->entry_passwd);
	gtk_entry_set_visibility (GTK_ENTRY (gui->entry_passwd), FALSE);
	gtk_entry_set_text (GTK_ENTRY (gui->entry_passwd), passwd);
	gtk_signal_connect (GTK_OBJECT (gui->entry_passwd), "changed",
			    GTK_SIGNAL_FUNC (on_entry_changed), gui);
	gtk_table_attach_defaults (GTK_TABLE (table), label_passwd, 0, 1, 5,
				   6);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_passwd, 1, 2,
				   5, 6);

	label_compression = gtk_label_new (_("Compression:"));
	adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 9, 1, 10, 0));
	gui->spin_compression = gtk_spin_button_new (adj, 1, 0);
	gtk_widget_show (label_compression);
	gtk_widget_show (gui->spin_compression);
	gtk_spin_button_set_value
		(GTK_SPIN_BUTTON (gui->spin_compression), compression);
	gtk_signal_connect (GTK_OBJECT (gui->spin_compression), "changed",
			    GTK_SIGNAL_FUNC (on_entry_changed), gui);
	gtk_table_attach_defaults (GTK_TABLE (table), label_compression, 0, 1,
				   7, 8);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->spin_compression,
				   1, 2, 7, 8);

	label_server_type = gtk_label_new (_("Select Server type:"));
	gui->combo_server_type = gtk_combo_new ();
	gtk_widget_show (label_server_type);
	gtk_widget_show (gui->combo_server_type);
	gtk_combo_set_popdown_strings (GTK_COMBO (gui->combo_server_type),
				       string_list);
	gtk_combo_set_value_in_list (GTK_COMBO (gui->combo_server_type),
				     TRUE, FALSE);
	gtk_signal_connect (GTK_OBJECT
			    (GTK_COMBO (gui->combo_server_type)->entry),
			    "changed", GTK_SIGNAL_FUNC (on_entry_changed),
			    gui);
	gtk_entry_set_text (GTK_ENTRY
			    (GTK_COMBO (gui->combo_server_type)->entry),
			    server_types[server_type]);
	gtk_table_attach_defaults (GTK_TABLE (table), label_server_type, 0, 1,
				   0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->combo_server_type,
				   1, 2, 0, 1);

	gtk_signal_connect (GTK_OBJECT
			    (GNOME_PROPERTY_BOX (gui->dialog)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (on_cvs_settings_ok),
			    gui);
	gtk_signal_connect (GTK_OBJECT
			    (GNOME_PROPERTY_BOX (gui->dialog)->apply_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (on_cvs_settings_apply), gui);
	gtk_signal_connect (GTK_OBJECT
			    (GNOME_PROPERTY_BOX (gui->dialog)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (on_cvs_settings_cancel), gui);

	server_page_label = gtk_label_new ("CVS");
	gnome_property_box_append_page
		(GNOME_PROPERTY_BOX (gui->dialog), table, server_page_label);

	gtk_widget_show (gui->dialog);
}

/* 
	Create a dialog in which a filename can be specified.
	If the user clicks the Update/Commit/Status button cvs is invoked.
	Default for the filename is the current open file.
*/

void
create_cvs_gui (CVS * cvs, int dialog_type, gchar* filename, gboolean bypass_dialog)
{
	gchar *title;
	gchar *button_label;

	GtkWidget *label_file;
	GtkWidget *label_branch;
	GtkWidget *label_msg;
	GtkWidget *table;
	GtkWidget *gtkentry;

	CVSFileGUI *gui;

	g_return_if_fail (cvs != NULL);

	switch (dialog_type)
	{
	case CVS_ACTION_UPDATE:
		title = _("CVS: Update");
		button_label = _("Update");
		break;
	case CVS_ACTION_COMMIT:
		title = _("CVS: Commit");
		button_label = _("Commit");
		break;
	case CVS_ACTION_STATUS:
		title = _("CVS: Status");
		button_label = _("Show status");
		break;
	case CVS_ACTION_LOG:
		title = _("CVS: Get Log");
		button_label = _("Show log");
		break;
	case CVS_ACTION_ADD:
		title = _("CVS: Add file");
		button_label = _("Add");
		break;
	case CVS_ACTION_REMOVE:
		title = _("CVS: Remove file");
		button_label = _("Remove");
		break;
	default:
		return;
	}

	gui = g_new0 (CVSFileGUI, 1);
	gui->type = dialog_type;

	gui->dialog =
		gnome_dialog_new (title, button_label, _("Cancel"), NULL);
	gtk_window_set_wmclass (GTK_WINDOW (gui->dialog), "cvs-file",
				"anjuta");

	table = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table);

	label_file = gtk_label_new (_("File: "));
	if (gui->type != CVS_ACTION_COMMIT)
		label_branch = gtk_label_new (_("Branch: "));
	else
		label_branch = gtk_label_new (_("Revision: ")); 
	label_msg = gtk_label_new (_("Log message: "));
	gtk_widget_show (label_file);

	gui->entry_file = gnome_file_entry_new ("cvs-file", _("Select file"));
	gui->entry_branch = gnome_entry_new ("cvs-branch");
	gui->text_message = gtk_text_new (NULL, NULL);
	gtk_widget_show (gui->entry_file);
	gtk_widget_set_usize(gui->entry_file, 300, -1);

	if (gui->type == CVS_ACTION_UPDATE || gui->type == CVS_ACTION_COMMIT)
	{
		gtk_widget_show (label_branch);
		gtk_widget_show (gui->entry_branch);
	}

	if (gui->type == CVS_ACTION_COMMIT || gui->type == CVS_ACTION_ADD)
	{
		gtk_widget_show (label_msg);
		gtk_widget_show (gui->text_message);
		gtk_text_set_editable (GTK_TEXT (gui->text_message), TRUE);
	}

	gtk_table_attach_defaults (GTK_TABLE (table), label_file, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_file, 1, 2,
				   0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), label_branch, 0, 1, 1,
				   2);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_branch, 1, 2,
				   1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), label_msg, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->text_message, 1, 2,
				   2, 4);

	gtkentry =
		gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY
					    (gui->entry_file));
	
	if(filename == NULL) {
		gtk_entry_set_text (GTK_ENTRY (gtkentry), get_cur_filename());
	} else {
		gtk_entry_set_text (GTK_ENTRY (gtkentry), filename);
	}

	gui->action_button =
		g_list_first (GNOME_DIALOG (gui->dialog)->buttons)->data;
	gui->cancel_button =
		g_list_last (GNOME_DIALOG (gui->dialog)->buttons)->data;

	gtk_signal_connect (GTK_OBJECT (gui->action_button), "clicked",
			    GTK_SIGNAL_FUNC (on_cvs_ok), gui);
	gtk_signal_connect (GTK_OBJECT (gui->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (on_cvs_cancel), gui);

	gtk_box_pack_start_defaults (GTK_BOX
				     (GNOME_DIALOG (gui->dialog)->vbox),
				     table);
	
	if (bypass_dialog) {
		on_cvs_ok(gui->action_button, gui);
	} else {
		gtk_widget_show (gui->dialog);
	}
}

/*
	Create a window to where the user can choose options for
	a diff between the working copy of a file and the repositry
*/

void
create_cvs_diff_gui (CVS * cvs, gchar* filename, gboolean bypass_dialog)
{
	CVSFileDiffGUI *gui;

	GtkWidget *table;

	GtkWidget *label_file;
	GtkWidget *label_rev;
	GtkWidget *label_date;
	GtkWidget *gtkentry;

	gui = g_new0 (CVSFileDiffGUI, 1);

	gui->dialog =
		gnome_dialog_new (_("CVS: Diff file"), _("Diff"), _("Cancel"),
				  NULL);
	gui->diff_button =
		g_list_first (GNOME_DIALOG (gui->dialog)->buttons)->data;
	gui->cancel_button =
		g_list_last (GNOME_DIALOG (gui->dialog)->buttons)->data;

	gtk_signal_connect (GTK_OBJECT (gui->diff_button), "clicked",
			    GTK_SIGNAL_FUNC (on_cvs_diff_ok), gui);
	gtk_signal_connect (GTK_OBJECT (gui->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (on_cvs_diff_cancel), gui);

	table = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table);

	label_file = gtk_label_new (_("File: "));
	gui->entry_file = gnome_file_entry_new ("cvs-file", _("Select file"));
	gtkentry =
		gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY
					    (gui->entry_file));
	gtk_widget_set_usize(gtkentry, 300, -1);
	
	if(filename == NULL) {
		gtk_entry_set_text (GTK_ENTRY (gtkentry), get_cur_filename());
	} else {
		gtk_entry_set_text (GTK_ENTRY (gtkentry), filename);
	}

	gtk_widget_show (label_file);
	gtk_widget_show (gui->entry_file);
	gtk_table_attach_defaults (GTK_TABLE (table), label_file, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_file, 1, 2,
				   0, 1);

	label_date = gtk_label_new (_("Date to diff with: "));
	gui->entry_date = gnome_date_edit_new (time (NULL), TRUE, TRUE);
	gtk_widget_show (label_date);
	gtk_widget_show (gui->entry_date);
	gtk_table_attach_defaults (GTK_TABLE (table), label_date, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_date, 1, 2,
				   1, 2);

	label_rev = gtk_label_new (_("Revision to diff with: "));
	gui->entry_rev = gnome_entry_new ("cvs-revision");
	gtk_widget_show (label_rev);
	gtk_widget_show (gui->entry_rev);
	gtk_table_attach_defaults (GTK_TABLE (table), label_rev, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_rev, 1, 2, 2,
				   3);

	gui->check_unified =
		gtk_check_button_new_with_label (_("Create unified diff"));
	gtk_widget_show (gui->check_unified);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->check_unified), TRUE);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->check_unified, 0,
				   1, 3, 4);

	gtk_box_pack_start_defaults (GTK_BOX(GNOME_DIALOG (gui->dialog)->vbox),
			table);

	if (bypass_dialog) {
		on_cvs_diff_ok(gui->diff_button, gui);
	} else {
		gtk_widget_show (gui->dialog);
	}
}

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

	table = gtk_table_new (3, 2, FALSE);
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

	gtk_box_pack_start_defaults (GTK_BOX(GNOME_DIALOG (gui->dialog)->vbox),
			table);

	if (bypass_dialog) {
		on_cvs_diff_ok(gui->diff_button, gui);
	} else {
		gtk_widget_show (gui->dialog);
	}
}

void 
create_cvs_login_gui (CVS * cvs)
{
	GtkWidget* type_label;
	GtkWidget* user_label;
	GtkWidget* server_label;
	GtkWidget* dir_label;
	GtkWidget* table;
	GtkWidget* ok_button;
	GtkWidget* cancel_button;
	
	GList* strings;
	int i;
	CVSLoginGUI* gui;
	
	gui = g_new0(CVSLoginGUI, 1);
	
	gui->dialog = gnome_dialog_new (_("CVS Login"), _("Login"), _("Cancel"), NULL);
	
	gui->combo_type = gtk_combo_new();
	gui->entry_user = gnome_entry_new ("cvs-user");
	gui->entry_server = gnome_entry_new ("cvs-server");
	gui->entry_dir = gnome_entry_new ("cvs-server-dir");
	
	type_label = gtk_label_new (_("Server type: "));
	user_label = gtk_label_new (_("Username: "));
	server_label = gtk_label_new (_("Server: "));
	dir_label = gtk_label_new (_("Directory on the server: "));
	
	table = gtk_table_new (4, 2, TRUE);
	gtk_table_attach_defaults (GTK_TABLE (table), type_label, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), server_label, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), dir_label, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), user_label, 0, 1, 3, 4);
	
	gtk_table_attach_defaults (GTK_TABLE (table), gui->combo_type, 1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_server, 1, 2, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_dir, 1, 2, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), gui->entry_user, 1, 2, 3, 4);
	
	strings = g_list_alloc();
	for (i = 0; i < 4; i++)
	{
		strings = g_list_append (strings, server_types[i]);
	}
	
	gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (gui->combo_type)->entry), FALSE);
	gtk_combo_set_popdown_strings (GTK_COMBO (gui->combo_type), strings);
	gtk_combo_set_value_in_list (GTK_COMBO (gui->combo_type), TRUE, FALSE);
	
	gtk_box_pack_start_defaults (GTK_BOX (GNOME_DIALOG(gui->dialog)->vbox),
				table);
	
	ok_button =
		g_list_first (GNOME_DIALOG (gui->dialog)->buttons)->data;
	cancel_button =
		g_list_last (GNOME_DIALOG (gui->dialog)->buttons)->data;
	
	gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", 
			GTK_SIGNAL_FUNC (on_cvs_login_ok), gui);
	gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
			GTK_SIGNAL_FUNC (on_cvs_login_cancel), gui);
	
	gtk_widget_show_all (gui->dialog);
}

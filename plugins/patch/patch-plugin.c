/*  patch-plugin.c (C) 2002 Johannes Schmid
 *  					     2005 Massimo Cora'  [ported to the new plugin architetture]
 *
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
 
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>

#include "plugin.h"
#include "patch-plugin.h"

static void patch_level_changed (GtkAdjustment *adj);

static void on_ok_clicked (GtkButton *button, PatchPluginGUI* gui);
static void on_cancel_clicked (GtkButton *button, PatchPluginGUI* gui);

static void on_msg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar* line, gpointer data);
static void on_patch_terminated (AnjutaLauncher *launcher,
								 gint child_pid, gint status, gulong time_taken,
								 gpointer data);


gint patch_level = 0;


static void
patch_level_changed (GtkAdjustment *adj)
{
	patch_level = (gint) adj->value;
}

void
patch_show_gui (PatchPlugin *plugin)
{
	PatchPluginGUI *gui;
	GtkObject* adj;
	GtkWidget* dir_label = gtk_label_new (_("File/Dir to patch"));
	GtkWidget* file_label = gtk_label_new (_("Patch file"));
	GtkWidget* level_label = gtk_label_new (_("Patch level (-p)"));
	GtkWidget* table = gtk_table_new (3, 2, FALSE);

	gui = g_new0 (PatchPluginGUI, 1);
	
	/* setting plugin reference */
	gui->plugin = plugin;
	
	gui->dialog = gnome_dialog_new (_("Patch Plugin"), _("Cancel"), _("Patch"), NULL);
	gui->entry_patch_dir = gnome_file_entry_new ("patch-dir", 
	_("Selected directory to patch"));
	gui->entry_patch_file = gnome_file_entry_new ("patch-file",
			_("Selected patch file"));

	adj = gtk_adjustment_new (patch_level, 0, 10, 1, 1, 1);
	gtk_signal_connect (adj, "value_changed",
			    GTK_SIGNAL_FUNC (patch_level_changed), NULL);

	gui->hscale_patch_level = gtk_hscale_new (GTK_ADJUSTMENT (adj));
	gtk_scale_set_digits (GTK_SCALE (gui->hscale_patch_level), 0);
	
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 10);

	gtk_table_attach_defaults (GTK_TABLE(table), dir_label, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE(table), file_label, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE(table), level_label, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE(table), gui->entry_patch_dir, 1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE(table), gui->entry_patch_file, 1, 2, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE(table), gui->hscale_patch_level, 1, 2, 2, 3);
	
	gui->ok_button = g_list_last (GNOME_DIALOG (gui->dialog)->buttons)->data;
	gui->cancel_button = g_list_first (GNOME_DIALOG (gui->dialog)->buttons)->data;
	
	g_signal_connect (G_OBJECT (gui->ok_button), "clicked", 
			GTK_SIGNAL_FUNC(on_ok_clicked), gui);
	g_signal_connect (G_OBJECT (gui->cancel_button), "clicked", 
			GTK_SIGNAL_FUNC(on_cancel_clicked), gui);

	gtk_container_set_border_width (GTK_CONTAINER (GNOME_DIALOG (gui->dialog)->vbox), 5);
	gtk_box_pack_start_defaults (GTK_BOX (GNOME_DIALOG (gui->dialog)->vbox), table);

	gtk_widget_show_all (gui->dialog);
}


static void 
on_ok_clicked (GtkButton *button, PatchPluginGUI* gui)
{
	const gchar* directory;
	const gchar* patch_file;
	GString* command = g_string_new (NULL);
	gchar* message;
	IAnjutaMessageManager *mesg_manager;
	PatchPlugin *p_plugin;
	p_plugin = gui->plugin;
	
	g_return_if_fail (p_plugin != NULL);

	mesg_manager = anjuta_shell_get_interface 
		(ANJUTA_PLUGIN (p_plugin)->shell,	IAnjutaMessageManager, NULL);
	
	g_return_if_fail (mesg_manager != NULL);
	
	gui->mesg_view =
		ianjuta_message_manager_add_view (mesg_manager, _("Patch"), 
		ICON_FILE, NULL);
	
	ianjuta_message_manager_set_current_view (mesg_manager, gui->mesg_view, NULL);	
	
	directory = gtk_entry_get_text(GTK_ENTRY(
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(gui->entry_patch_dir))));
	patch_file = gtk_entry_get_text(GTK_ENTRY(
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(gui->entry_patch_file))));

	if (!file_is_directory (directory))
	{
		g_string_free (command, TRUE);
		gnome_ok_dialog (_("Please select the directory where the patch should be applied"));
		return;
	}
	
	g_string_sprintf (command, "patch -d %s -p %d -f -i %s",
			  directory, patch_level, patch_file);
	
	message = g_strdup_printf (_("Patching %s using %s\n"), 
			  directory, patch_file);

	ianjuta_message_view_append (gui->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 message, "", NULL);
	
	ianjuta_message_view_append (gui->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 _("Patching...\n"), "", NULL);

	g_signal_connect (p_plugin->launcher, "child-exited",
					  G_CALLBACK (on_patch_terminated), gui);
	
	if (!anjuta_launcher_is_busy (p_plugin->launcher))
		anjuta_launcher_execute (p_plugin->launcher, command->str,
								 (AnjutaLauncherOutputCallback)on_msg_arrived, gui);
	else
		gnome_ok_dialog (
			_("There are unfinished jobs, please wait until they are finished"));
	g_string_free(command, TRUE);
	on_cancel_clicked (GTK_BUTTON(gui->cancel_button), gui);

}

static void 
on_cancel_clicked (GtkButton *button, PatchPluginGUI* gui)
{
	gtk_widget_hide (gui->dialog);
	gtk_widget_destroy (gui->dialog);
}

static void 
on_msg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar* line, gpointer data)
{
	PatchPluginGUI *gui = (PatchPluginGUI*)data;
	
	g_return_if_fail (line != NULL);
	g_return_if_fail (gui != NULL);
	
	ianjuta_message_view_append (gui->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 line, "", NULL);
}

static void 
on_patch_terminated (AnjutaLauncher *launcher,
								 gint child_pid, gint status, gulong time_taken,
								 gpointer data)
{
	PatchPluginGUI *gui = (PatchPluginGUI*)data;
	
	g_return_if_fail (gui != NULL);
	g_return_if_fail (launcher != NULL);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (on_patch_terminated),
										  gui);

	if (status != 0)
	{
		ianjuta_message_view_append (gui->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_ERROR,
								 _("Patch failed.\nPlease review the failure messages.\n"
			"Examine and remove any rejected files.\n"), "", NULL);		
	}
	else
	{
		ianjuta_message_view_append (gui->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 _("Patch successful.\n"), "", NULL);
	}

	gui->mesg_view = NULL;
}

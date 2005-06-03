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

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/patch-plugin.glade"

static void patch_level_changed (GtkAdjustment *adj);

static void on_ok_clicked (GtkButton *button, PatchPlugin* p_plugin);
static void on_cancel_clicked (GtkButton *button, GtkDialog* dialog);

static void on_msg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar* line, gpointer data);
static void on_patch_terminated (AnjutaLauncher *launcher,
								 gint child_pid, gint status, gulong time_taken,
								 gpointer data);


static gint patch_level = 0;


static void
patch_level_changed (GtkAdjustment *adj)
{
	patch_level = (gint) adj->value;
}

void
patch_show_gui (PatchPlugin *plugin)
{
	GtkWidget* table;
	GtkWidget* patch_button;
	GtkWidget* cancel_button;
	GtkWidget* scale;
	GtkAdjustment* adj;
	
	GladeXML* gxml;
	
	gxml = glade_xml_new(GLADE_FILE, "patch_dialog", NULL);
	
	plugin->dialog = glade_xml_get_widget(gxml, "patch_dialog");
	
	table = glade_xml_get_widget(gxml, "patch_table");
	plugin->file_chooser = gtk_file_chooser_button_new(_("File/Directory to patch"),
											   GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	plugin->patch_chooser = gtk_file_chooser_button_new(_("Patch file"),
											   GTK_FILE_CHOOSER_ACTION_OPEN);
	
	gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(plugin->file_chooser),
											30);
	gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(plugin->patch_chooser),
											30);
	
	
	gtk_table_attach_defaults(GTK_TABLE(table), plugin->file_chooser, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), plugin->patch_chooser, 1, 2, 1, 2);
	
	scale = glade_xml_get_widget(gxml, "patch_level_scale");
	adj = gtk_range_get_adjustment(GTK_RANGE(scale));
	g_signal_connect (G_OBJECT(adj), "value_changed",
			    GTK_SIGNAL_FUNC (patch_level_changed), NULL);
		
	patch_button = glade_xml_get_widget(gxml, "patch_button");
	cancel_button = glade_xml_get_widget(gxml, "cancel_button");
	
	g_signal_connect (G_OBJECT (patch_button), "clicked", 
			G_CALLBACK(on_ok_clicked), plugin);
	g_signal_connect (G_OBJECT (cancel_button), "clicked", 
			G_CALLBACK(on_cancel_clicked), plugin->dialog);

	gtk_widget_show_all (plugin->dialog);
}


static void 
on_ok_clicked (GtkButton *button, PatchPlugin* p_plugin)
{
	const gchar* directory;
	const gchar* patch_file;
	GString* command = g_string_new (NULL);
	gchar* message;
	IAnjutaMessageManager *mesg_manager;
	
	g_return_if_fail (p_plugin != NULL);

	mesg_manager = anjuta_shell_get_interface 
		(ANJUTA_PLUGIN (p_plugin)->shell,	IAnjutaMessageManager, NULL);
	
	g_return_if_fail (mesg_manager != NULL);
	
	p_plugin->mesg_view =
		ianjuta_message_manager_add_view (mesg_manager, _("Patch"), 
		ICON_FILE, NULL);
	
	ianjuta_message_manager_set_current_view (mesg_manager, p_plugin->mesg_view, NULL);	
	
	directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p_plugin->file_chooser));
	patch_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p_plugin->patch_chooser));
	
	if (!g_file_test (directory, G_FILE_TEST_IS_DIR))
	{
		g_string_free (command, TRUE);
		gnome_ok_dialog (_("Please select the directory where the patch should be applied"));
		return;
	}
	
	g_string_sprintf (command, "patch -d %s -p %d -f -i %s",
			  directory, patch_level, patch_file);
	
	message = g_strdup_printf (_("Patching %s using %s\n"), 
			  directory, patch_file);

	ianjuta_message_view_append (p_plugin->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 message, "", NULL);
	
	ianjuta_message_view_append (p_plugin->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 _("Patching...\n"), "", NULL);

	g_signal_connect (p_plugin->launcher, "child-exited",
					  G_CALLBACK (on_patch_terminated), p_plugin);
	
	if (!anjuta_launcher_is_busy (p_plugin->launcher))
		anjuta_launcher_execute (p_plugin->launcher, command->str,
								 (AnjutaLauncherOutputCallback)on_msg_arrived, p_plugin);
	else
		gnome_ok_dialog (
			_("There are unfinished jobs, please wait until they are finished"));
	g_string_free(command, TRUE);
	
	gtk_widget_hide (p_plugin->dialog);
	gtk_widget_destroy (p_plugin->dialog);
}

static void 
on_cancel_clicked (GtkButton *button, GtkDialog* dialog)
{
	gtk_widget_hide (GTK_WIDGET(dialog));
	gtk_widget_destroy (GTK_WIDGET(dialog));
}

static void 
on_msg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar* line, gpointer data)
{	
	g_return_if_fail (line != NULL);	
	
	PatchPlugin* p_plugin = (PatchPlugin*)data;
	ianjuta_message_view_append (p_plugin->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 line, "", NULL);
}

static void 
on_patch_terminated (AnjutaLauncher *launcher,
								 gint child_pid, gint status, gulong time_taken,
								 gpointer data)
{
	g_return_if_fail (launcher != NULL);
	
	PatchPlugin* plugin = (PatchPlugin*)data;
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (on_patch_terminated),
										  plugin);

	if (status != 0)
	{
		ianjuta_message_view_append (plugin->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_ERROR,
								 _("Patch failed.\nPlease review the failure messages.\n"
			"Examine and remove any rejected files.\n"), "", NULL);		
	}
	else
	{
		ianjuta_message_view_append (plugin->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 _("Patch successful.\n"), "", NULL);
	}

	plugin->mesg_view = NULL;
}

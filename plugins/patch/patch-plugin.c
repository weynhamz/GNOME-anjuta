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
static void on_cancel_clicked (GtkButton *button, PatchPlugin* plugin);

static void on_msg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar* line, gpointer data);
static void on_msg_buffer (IAnjutaMessageView *view, const gchar *line,
					  PatchPlugin *plugin);

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
	GtkWidget* scale;
	GtkAdjustment* adj;
	
	GladeXML* gxml;
	
	gxml = glade_xml_new(GLADE_FILE, "patch_dialog", NULL);
	
	plugin->dialog = glade_xml_get_widget(gxml, "patch_dialog");
	plugin->output_label = glade_xml_get_widget(gxml, "output");
	
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
		
	plugin->patch_button = glade_xml_get_widget(gxml, "patch_button");
	plugin->cancel_button = glade_xml_get_widget(gxml, "cancel_button");
	plugin->dry_run_check = glade_xml_get_widget(gxml, "dryrun");
	
	g_signal_connect (G_OBJECT (plugin->patch_button), "clicked", 
			G_CALLBACK(on_ok_clicked), plugin);
	g_signal_connect (G_OBJECT (plugin->cancel_button), "clicked", 
			G_CALLBACK(on_cancel_clicked), plugin);
	
	plugin->executing = FALSE;
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

	g_signal_connect (G_OBJECT (p_plugin->mesg_view), "buffer-flushed",
						  G_CALLBACK (on_msg_buffer), p_plugin);

	directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p_plugin->file_chooser));
	patch_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p_plugin->patch_chooser));						
	
	if (!g_file_test (directory, G_FILE_TEST_IS_DIR))
	{
		g_string_free (command, TRUE);
		anjuta_util_dialog_error(GTK_WINDOW(p_plugin->dialog), 
			_("Please select the directory where the patch should be applied"));
		return;
	}
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p_plugin->dry_run_check)))
		g_string_sprintf (command, "patch --dry-run -d %s -p %d -f -i %s",
				  directory, patch_level, patch_file);
	else
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
	{
		anjuta_launcher_execute (p_plugin->launcher, command->str,
								 (AnjutaLauncherOutputCallback)on_msg_arrived, p_plugin);
		p_plugin->executing = TRUE;
		gtk_label_set_text(GTK_LABEL(p_plugin->output_label), "Patching...");
		gtk_widget_set_sensitive(p_plugin->patch_button, FALSE);
	}
	else
		anjuta_util_dialog_error(GTK_WINDOW(p_plugin->dialog),
			_("There are unfinished jobs, please wait until they are finished"));
	g_string_free(command, TRUE);
}

static void 
on_cancel_clicked (GtkButton *button, PatchPlugin* plugin)
{
	if (plugin->executing == TRUE)
	{
		anjuta_launcher_reset(plugin->launcher);
	}
	gtk_widget_hide (GTK_WIDGET(plugin->dialog));
	gtk_widget_destroy (GTK_WIDGET(plugin->dialog));
}

static void 
on_msg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar* line, gpointer data)
{	
	g_return_if_fail (line != NULL);	
	
	PatchPlugin* p_plugin = PATCH_PLUGIN (data);
	ianjuta_message_view_buffer_append (p_plugin->mesg_view, line, NULL);
}

static void 
on_patch_terminated (AnjutaLauncher *launcher,
								 gint child_pid, gint status, gulong time_taken,
								 gpointer data)
{
	g_return_if_fail (launcher != NULL);
	
	PatchPlugin* plugin = PATCH_PLUGIN (data);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (on_patch_terminated),
										  plugin);

	if (status != 0)
	{
		gtk_label_set_text(GTK_LABEL(plugin->output_label), 
			_("Patch failed.\nPlease review the failure messages.\n"
			"Examine and remove any rejected files.\n"));
		gtk_widget_set_sensitive(plugin->patch_button, TRUE);	
	}
	else
	{
		gtk_label_set_text(GTK_LABEL(plugin->output_label), "Patching complete");
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(plugin->dry_run_check)))
		{
			gtk_widget_hide (plugin->dialog);
			gtk_widget_destroy (plugin->dialog);
		}
		else
			gtk_widget_set_sensitive(plugin->patch_button, TRUE);
	}

	plugin->executing = FALSE;
	plugin->mesg_view = NULL;
}

static void
on_msg_buffer (IAnjutaMessageView *view, const gchar *line,
					  PatchPlugin *plugin)
{
	ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "", NULL);
}

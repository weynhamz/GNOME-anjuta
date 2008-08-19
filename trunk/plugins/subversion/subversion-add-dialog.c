/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "subversion-add-dialog.h"

static void
on_add_command_finished (AnjutaCommand *command, guint return_code, 
						 Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: File will be added on next "
							 "commit."), 5);
	
	report_errors (command, return_code);
	
	svn_add_command_destroy (SVN_ADD_COMMAND (command));
}

static void
on_subversion_add_response(GtkDialog* dialog, gint response, SubversionData* data)
{
	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			GtkWidget* force = glade_xml_get_widget(data->gxml, "subversion_force");
			GtkWidget* recurse = glade_xml_get_widget(data->gxml, "subversion_recurse");
			
			const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
			SvnAddCommand *add_command;
			
			if (!check_input (GTK_WIDGET (dialog), 
							  fileentry, _("Please enter a path.")))
			{
				break;
			}
			
			add_command = svn_add_command_new ((gchar *) filename,
											   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(force)),
											   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recurse)));
			
			g_signal_connect (G_OBJECT (add_command), "command-finished",
							  G_CALLBACK (on_add_command_finished),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (add_command));
			
			gtk_widget_destroy(GTK_WIDGET(dialog));
			subversion_data_free(data);
			break;
		}
		default:
		{
			gtk_widget_destroy (GTK_WIDGET(dialog));
			subversion_data_free(data);
		}
	}
}

static void
subversion_add_dialog(GtkAction* action, Subversion* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	SubversionData* data;
	gxml = glade_xml_new(GLADE_FILE, "subversion_add", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_add");
	fileentry = glade_xml_get_widget(gxml, "subversion_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	data = subversion_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_add_response), data);
	
	gtk_widget_show(dialog);
}

void 
on_menu_subversion_add (GtkAction* action, Subversion* plugin)
{
	subversion_add_dialog(action, plugin, plugin->current_editor_filename);
}

void
on_fm_subversion_add (GtkAction *action, Subversion *plugin)
{
	subversion_add_dialog (action, plugin, plugin->fm_current_filename);
}

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    cvs-actions.c
    Copyright (C) 2004 Johannes Schmid

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Note on GnomeVFS
	The CVS plugin currently does not use GnomeVFS for file access because this would
	be overkill in my opinion. CVS currently only support real files.
*/

#include "cvs-actions.h"
#include "cvs-execute.h"

#include "glade/glade.h"
#include <libgnomevfs/gnome-vfs.h>
#include "libgen.h"

#include "libanjuta/anjuta-preferences.h"

/* cvs /command independant options/ /command/ /command options/ /command_arguments/ */

static gchar* create_cvs_command(AnjutaPreferences* prefs,
								const gchar* action, 
								const gchar* command_options,
								const gchar* command_arguments)
{
	gchar* cvs;
	gchar* global_options = NULL;
	gboolean ignorerc;
	gint compression;
	gchar* command;
	/* command global_options action command_options command_arguments */
	gchar* CVS_FORMAT = "%s %s %s %s %s";
	
	g_return_val_if_fail (prefs != NULL, NULL);
	g_return_val_if_fail (action != NULL, NULL);
	g_return_val_if_fail (command_options != NULL, NULL);
	g_return_val_if_fail (command_arguments != NULL, NULL);
	
	cvs = anjuta_preferences_get(prefs, "cvs.path");
	compression = anjuta_preferences_get_int(prefs, "cvs.compression");
	ignorerc = anjuta_preferences_get_int(prefs, "cvs.ignorerc");
	if (compression && ignorerc)
		global_options = g_strdup_printf("-f -z%d", compression);
	else if (compression)
		global_options = g_strdup_printf("-z%d", compression);
	else /* if (ignorerc */
		global_options = g_strdup("-f");
	
	command = g_strdup_printf(CVS_FORMAT, cvs, global_options, action,  
								command_options, command_arguments);
	
	g_free (global_options);
	
	return command;
}
void on_cvs_add_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* binary;
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_add", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_add");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	binary = glade_xml_get_widget(gxml, "cvs_binary");

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gboolean is_binary;
		gchar* filename;
		gchar* command;
		gchar* command_options;
		
		is_binary = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(binary));
		filename = basename (gtk_entry_get_text(GTK_ENTRY(fileentry)));
		
		if (!strlen(filename))
			break;	
	
		if (is_binary)
			command_options = "-kb";
		else
			command_options = "";
		
		
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "add", command_options, filename);
		cvs_execute(plugin, command, dirname(filename));
		g_free(command);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_remove_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* binary;
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_remove", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_remove");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		
		filename = basename (gtk_entry_get_text(GTK_ENTRY(fileentry)));
		/* Did I say we do not use GnomeVFS... */
		if (gnome_vfs_unlink(gtk_entry_get_text(GTK_ENTRY(fileentry)))
			!= GNOME_VFS_OK)
		{
			anjuta_util_dialog_error
				(dialog,_("Unable to delete file"), NULL);
			break;
		}
		
		if (!strlen(filename))
			break;	
		
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "remove", "", filename);
		cvs_execute(plugin, command, dirname(filename));
		g_free(command);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_commit_activate (GtkAction* action, CVSPlugin* plugin)
{

}

void on_cvs_update_activate (GtkAction* action, CVSPlugin* plugin)
{

}

void on_cvs_diff_file_activate (GtkAction* action, CVSPlugin* plugin)
{

}

void on_cvs_diff_tree_activate (GtkAction* action, CVSPlugin* plugin)
{

}

void on_cvs_import_activate (GtkAction* action, CVSPlugin* plugin)
{

}

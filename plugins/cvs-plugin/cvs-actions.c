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
#include "libgen.h"

/* cvs /command independant options/ /command/ /command options/ /command_arguments/ */
static const gchar* cvs_command = "cvs %s %s %s %s";

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
		filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
		
		if (!strlen(filename))
			break;	
	
		if (is_binary)
			command_options = "-kb";
		else
			command_options = "";
		
		command = g_strdup_printf(cvs_command, "", "add", command_options, filename);
		g_message("Executing: %s", command);
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

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    subversion-actions.c
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


#include "subversion-actions.h"
#include "subversion-callbacks.h"

#include "glade/glade.h"
#include <libgnomevfs/gnome-vfs.h>
#include "libgen.h"

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>

static void init_whole_project(Subversion *plugin, GtkWidget* project)
{
	gboolean project_loaded = (plugin->project_root_dir != NULL);
	gtk_widget_set_sensitive(project, project_loaded);
}	

static void on_whole_project_toggled(GtkToggleButton* project, Subversion *plugin)
{
	GtkEntry* fileentry = g_object_get_data (G_OBJECT (project), "fileentry");
	if (gtk_toggle_button_get_active(project) && plugin->project_root_dir)
	{
		gtk_entry_set_text (fileentry, plugin->project_root_dir);
		gtk_widget_set_sensitive(GTK_WIDGET(fileentry), FALSE);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(fileentry), TRUE);	
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

static void
subversion_remove_dialog(GtkAction* action, Subversion* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	SubversionData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "subversion_remove", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_remove");
	fileentry = glade_xml_get_widget(gxml, "subversion_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);

	data = subversion_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_remove_response), data);
	
	gtk_widget_show(dialog);
	
}

static void
subversion_commit_dialog (GtkAction* action, Subversion* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	SubversionData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "subversion_commit", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_commit");
	fileentry = glade_xml_get_widget(gxml, "subversion_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "subversion_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	data = subversion_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_commit_response), data);
	
	gtk_widget_show(dialog);
	
}

static void
subversion_update_dialog (GtkAction* action, Subversion* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	SubversionData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "subversion_update", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_update");
	fileentry = glade_xml_get_widget(gxml, "subversion_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "subversion_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	data = subversion_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_update_response), data);
	
	gtk_widget_show(dialog);	
}

void on_menu_subversion_add (GtkAction* action, Subversion* plugin)
{
	subversion_add_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_subversion_remove (GtkAction* action, Subversion* plugin)
{
	subversion_remove_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_subversion_commit (GtkAction* action, Subversion* plugin)
{
	subversion_commit_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_subversion_update (GtkAction* action, Subversion* plugin)
{
	subversion_update_dialog(action, plugin, plugin->current_editor_filename);
}

void on_fm_subversion_commit (GtkAction* action, Subversion* plugin)
{
	subversion_commit_dialog(action, plugin, plugin->fm_current_filename);
}

void on_fm_subversion_update (GtkAction* action, Subversion* plugin)
{
	subversion_update_dialog(action, plugin, plugin->fm_current_filename);
}

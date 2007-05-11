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


#include "cvs-actions.h"
#include "cvs-execute.h"
#include "cvs-callbacks.h"

#include "glade/glade.h"
#include <libgnomevfs/gnome-vfs.h>
#include "libgen.h"

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

void cvs_add_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_remove_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_commit_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_update_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_diff_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_status_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_log_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);

static void on_server_type_changed(GtkComboBox* combo, GladeXML* gxml)
{
	GtkWidget* username;
	GtkWidget* password;
	
	username = glade_xml_get_widget(gxml, "cvs_username");
	password = glade_xml_get_widget(gxml, "cvs_password");
	
	switch (gtk_combo_box_get_active(combo))
	{
		case SERVER_LOCAL:
			gtk_widget_set_sensitive(username, FALSE);
			gtk_widget_set_sensitive(password, FALSE);
			break;
		case SERVER_EXTERN:
			gtk_widget_set_sensitive(username, TRUE);
			gtk_widget_set_sensitive(password, FALSE);
			break;
		case SERVER_PASSWORD:
			gtk_widget_set_sensitive(username, TRUE);
			gtk_widget_set_sensitive(password, TRUE);
			break;
		default:
			DEBUG_PRINT("Unknown CVS server type");
	}
}

static void on_diff_type_changed(GtkComboBox* combo, GtkWidget* unified_check)
{
	if (gtk_combo_box_get_active(combo) == DIFF_STANDARD)
	{
		gtk_widget_set_sensitive(unified_check, FALSE);
	}
	else if (gtk_combo_box_get_active(combo) == DIFF_PATCH)
	{
		gtk_widget_set_sensitive(unified_check, TRUE);
	}
	else
		gtk_combo_box_set_active(combo, DIFF_STANDARD);
}

static void init_whole_project(CVSPlugin *plugin, GtkWidget* project)
{
	gboolean project_loaded = (plugin->project_root_dir != NULL);
	gtk_widget_set_sensitive(project, project_loaded);
}	

static void on_whole_project_toggled(GtkToggleButton* project, CVSPlugin *plugin)
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

void cvs_add_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	CVSData* data;
	gxml = glade_xml_new(GLADE_FILE, "cvs_add", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_add");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_add_response), data);
	
	gtk_widget_show(dialog);
}

void cvs_remove_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	CVSData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_remove", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_remove");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);

	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_remove_response), data);
	
	gtk_widget_show(dialog);
	
}

void cvs_commit_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	CVSData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_commit", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_commit");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "cvs_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_commit_response), data);
	
	gtk_widget_show(dialog);
	
}

void cvs_update_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	CVSData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_update", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_update");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "cvs_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_update_response), data);
	
	gtk_widget_show(dialog);	
}

void cvs_diff_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* diff_type;
	GtkWidget* unified_diff;
	GtkWidget* project;
	CVSData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_diff", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_diff");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "cvs_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	diff_type = glade_xml_get_widget(gxml, "cvs_diff_type");
	unified_diff = glade_xml_get_widget(gxml, "cvs_unified");
	gtk_combo_box_set_active(GTK_COMBO_BOX(diff_type), DIFF_PATCH);
	g_signal_connect(G_OBJECT(diff_type), "changed", 
		G_CALLBACK(on_diff_type_changed), unified_diff);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_diff_response), data);
	
	gtk_widget_show(dialog);	
}

void cvs_status_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	CVSData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_status", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_status");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);

	project = glade_xml_get_widget(gxml, "cvs_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_status_response), data);
	
	gtk_widget_show(dialog);	

}

void cvs_log_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	CVSData* data;

	gxml = glade_xml_new(GLADE_FILE, "cvs_logdialog", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_logdialog");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "cvs_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_log_response), data);
	
	gtk_widget_show(dialog);	
}

void on_menu_cvs_add (GtkAction* action, CVSPlugin* plugin)
{
	cvs_add_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_cvs_remove (GtkAction* action, CVSPlugin* plugin)
{
	cvs_remove_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_cvs_commit (GtkAction* action, CVSPlugin* plugin)
{
	cvs_commit_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_cvs_update (GtkAction* action, CVSPlugin* plugin)
{
	cvs_update_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_cvs_diff (GtkAction* action, CVSPlugin* plugin)
{
	cvs_diff_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_cvs_status (GtkAction* action, CVSPlugin* plugin)
{
	cvs_status_dialog(action, plugin, plugin->current_editor_filename);
}

void on_menu_cvs_import (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* direntry;
	GtkWidget* typecombo;
	CVSData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_import", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_import");
	direntry = glade_xml_get_widget(gxml, "cvs_rootdir");
	typecombo = glade_xml_get_widget(gxml, "cvs_server_type");
	
	g_signal_connect (G_OBJECT(typecombo), "changed", 
		G_CALLBACK(on_server_type_changed), gxml);
	gtk_combo_box_set_active(GTK_COMBO_BOX(typecombo), SERVER_LOCAL);
	
	if (plugin->project_root_dir)
		gtk_entry_set_text(GTK_ENTRY(direntry), plugin->project_root_dir);
	
	data = cvs_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_import_response), data);
	
	gtk_widget_show(dialog);	
	
}

void on_menu_cvs_log (GtkAction* action, CVSPlugin* plugin)
{
	cvs_log_dialog(action, plugin, plugin->current_editor_filename);
}

void on_fm_cvs_commit (GtkAction* action, CVSPlugin* plugin)
{
	cvs_commit_dialog(action, plugin, plugin->fm_current_filename);
}

void on_fm_cvs_update (GtkAction* action, CVSPlugin* plugin)
{
	cvs_update_dialog(action, plugin, plugin->fm_current_filename);
}

void on_fm_cvs_diff (GtkAction* action, CVSPlugin* plugin)
{
	cvs_diff_dialog(action, plugin, plugin->fm_current_filename);
}

void on_fm_cvs_status (GtkAction* action, CVSPlugin* plugin)
{
	cvs_status_dialog(action, plugin, plugin->fm_current_filename);
}

void on_fm_cvs_log (GtkAction* action, CVSPlugin* plugin)
{
	cvs_log_dialog(action, plugin, plugin->fm_current_filename);
}

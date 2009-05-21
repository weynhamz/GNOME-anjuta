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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "cvs-actions.h"
#include "cvs-execute.h"
#include "cvs-callbacks.h"

#include "libgen.h"

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#define BROWSE_BUTTON_ADD_DIALOG	"browse_button_add_dialog"
#define BROWSE_BUTTON_REMOVE_DIALOG	"browse_button_remove_dialog"
#define BROWSE_BUTTON_COMMIT_DIALOG	"browse_button_commit_dialog"
#define BROWSE_BUTTON_UPDATE_DIALOG	"browse_button_update_dialog"
#define BROWSE_BUTTON_DIFF_DIALOG	"browse_button_diff_dialog"
#define BROWSE_BUTTON_STATUS_DIALOG	"browse_button_status_dialog"
#define BROWSE_BUTTON_LOG_DIALOG	"browse_button_log_dialog"

void cvs_add_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_remove_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_commit_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_update_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_diff_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_status_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_log_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);

static void on_server_type_changed(GtkComboBox* combo, GtkBuilder* bxml)
{
	GtkWidget* username;
	GtkWidget* password;
	
	username = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_username"));
	password = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_password"));
	
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
			DEBUG_PRINT("%s", "Unknown CVS server type");
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

static void
on_browse_button_clicked(GtkButton *button, GtkEntry *entry)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Open File",
					      NULL,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));

		gtk_entry_set_text(entry, filename);

		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

void cvs_add_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_add"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_add_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_ADD_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_add_response), data);
	
	gtk_widget_show(dialog);
}

void cvs_remove_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_remove"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_remove_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);

	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_REMOVE_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_remove_response), data);
	
	gtk_widget_show(dialog);
	
}

void cvs_commit_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_commit"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_commit_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_commit_project"));
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);

	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_COMMIT_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_commit_response), data);
	
	gtk_widget_show(dialog);
	
}

void cvs_update_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_update"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_update_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_update_project"));
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);

	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_UPDATE_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_update_response), data);
	
	gtk_widget_show(dialog);	
}

void cvs_diff_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* diff_type;
	GtkWidget* unified_diff;
	GtkWidget* project;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_diff"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_diff_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_diff_project"));
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);
	
	diff_type = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_diff_type"));
	unified_diff = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_unified"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(diff_type), DIFF_PATCH);
	g_signal_connect(G_OBJECT(diff_type), "changed", 
		G_CALLBACK(on_diff_type_changed), unified_diff);

	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_DIFF_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_diff_response), data);
	
	gtk_widget_show(dialog);	
}

void cvs_status_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_status"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_status_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);

	project = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_status_project"));
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);

	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_STATUS_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_cvs_status_response), data);
	
	gtk_widget_show(dialog);	
}

void cvs_log_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	GtkWidget* button;
	CVSData* data;
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_logdialog"));
	fileentry = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_logdialog_filename"));
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_logdialog_project"));
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project);

	button = GTK_WIDGET(gtk_builder_get_object(bxml, BROWSE_BUTTON_LOG_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_browse_button_clicked), fileentry);

	data = cvs_data_new(plugin, bxml);
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
	GtkBuilder* bxml;
	GtkWidget* dialog; 
	GtkFileChooser* dir;
	GtkWidget* typecombo;
	CVSData* data;
	
	GError* error = NULL;
	bxml = gtk_builder_new();
	if (!gtk_builder_add_from_file(bxml, GLADE_FILE, &error))
	{
		g_warning("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}
	
	dialog = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_import"));
	dir = GTK_FILE_CHOOSER (gtk_builder_get_object(bxml, "cvs_rootdir"));
	typecombo = GTK_WIDGET(gtk_builder_get_object(bxml, "cvs_server_type"));
	
	g_signal_connect (G_OBJECT(typecombo), "changed", 
		G_CALLBACK(on_server_type_changed), bxml);
	gtk_combo_box_set_active(GTK_COMBO_BOX(typecombo), SERVER_LOCAL);
	
	if (plugin->project_root_dir)
		gtk_file_chooser_set_filename(dir, plugin->project_root_dir);
	
	data = cvs_data_new(plugin, bxml);
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

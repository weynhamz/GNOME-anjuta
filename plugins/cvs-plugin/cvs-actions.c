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

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>

enum
{
	DIFF_STANDARD = 0,
	DIFF_PATCH = 1
};

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

static void add_option(GtkWidget* toggle_button, GString* options, const gchar* argument)
{
	g_return_if_fail (options != NULL);
	g_return_if_fail (toggle_button != NULL);
	g_return_if_fail (argument != NULL);
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)))
	{
			g_string_append(options, " ");
			g_string_append(options, argument);
	}
}

static gboolean is_directory(const gchar* filename)
{
	GnomeVFSFileInfo info;
	GnomeVFSResult result;

	result = gnome_vfs_get_file_info(filename, &info, GNOME_VFS_FILE_INFO_DEFAULT);
	if (result == GNOME_VFS_OK)
	{
		if (info.type == GNOME_VFS_FILE_TYPE_DIRECTORY)
			return TRUE;
		else
			return FALSE;
	}
	else
		return FALSE;
}

static void diff_type_changed(GtkComboBox* combo, GtkWidget* unified_check)
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

static void set_editor_filename (CVSPlugin* plugin, GtkWidget* entry)
{
	if (plugin->current_editor_filename)
	{
		gtk_entry_set_text(GTK_ENTRY(entry), plugin->current_editor_filename);
	}
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
	set_editor_filename(plugin, fileentry);
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
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		
		if (!strlen(filename))
			break;	
	
		if (is_binary)
			command_options = "-kb";
		else
			command_options = "";
		
		
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "add", command_options, basename(filename));
		cvs_execute(plugin, command, dirname(filename));
		g_free(command);
		g_free(filename);
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
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_remove", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_remove");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	set_editor_filename(plugin, fileentry);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		/* Did I say we do not use GnomeVFS... */
		if (gnome_vfs_unlink(gtk_entry_get_text(GTK_ENTRY(fileentry)))
			!= GNOME_VFS_OK)
		{
			anjuta_util_dialog_error
				(GTK_WINDOW(dialog),_("Unable to delete file"), NULL);
			break;
		}
		
		if (!strlen(filename))
			break;	
		
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "remove", "", basename(filename));
		cvs_execute(plugin, command, dirname(filename));
		g_free(command);
		g_free(filename);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_commit_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_commit", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_commit");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	set_editor_filename(plugin, fileentry);
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		gchar* log;
		gchar* rev;
		gchar* escaped_log;
		GString* options;
		GtkWidget* logtext;
		GtkWidget* revisionentry;
		GtkWidget* norecurse;
		GtkTextBuffer* textbuf;
		GtkTextIter iterbegin;
		GtkTextIter iterend;
		
		options = g_string_new("");
		
		logtext = glade_xml_get_widget(gxml, "cvs_log");
		textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(logtext));
		gtk_text_buffer_get_start_iter(textbuf, &iterbegin);
		gtk_text_buffer_get_end_iter(textbuf, &iterend) ;
		log = gtk_text_buffer_get_text(textbuf, &iterbegin, &iterend, FALSE);
/* #warning FIXME: Check for escape chars in log */
		/* Fixed. -naba*/
		escaped_log = anjuta_util_escape_quotes (log);
		g_string_append_printf(options, "-m '%s'", escaped_log);
		g_free (escaped_log);

		revisionentry = glade_xml_get_widget(gxml, "cvs_revision");
		rev = g_strdup(gtk_entry_get_text(GTK_ENTRY(revisionentry)));
		if (strlen(rev))
		{
			g_string_append_printf(options, " -r %s", rev);
		}
		g_free(rev);

		norecurse = glade_xml_get_widget(gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!strlen(filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "commit", options->str, basename(filename));
			cvs_execute(plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "commit", options->str, "");
			cvs_execute(plugin, command, filename);
		}
			
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_update_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* revisionentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_update", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_update");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	set_editor_filename(plugin, fileentry);
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		gchar* revision;
		
		GString* options;
		GtkWidget* createdir;
		GtkWidget* removedir;
		GtkWidget* norecurse;
		GtkWidget* removesticky;

		options = g_string_new("");

		norecurse = glade_xml_get_widget(gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		removedir = glade_xml_get_widget(gxml, "cvs_removedir");
		add_option(removedir, options, "-P");
		createdir = glade_xml_get_widget(gxml, "cvs_createdir");
		add_option(createdir, options, "-d");

		revisionentry = glade_xml_get_widget(gxml, "cvs_revision");
		revision = g_strdup(gtk_entry_get_text(GTK_ENTRY(revisionentry)));
		if (strlen(revision))
		{
			g_string_append_printf(options, " -r %s", revision);
		}
		else
		{
			removesticky = glade_xml_get_widget(gxml, "cvs_removesticky");
			add_option(removesticky, options, "-A");
		}
		g_free(revision);
		
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!strlen(filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "update", options->str, basename(filename));
			cvs_execute(plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "update", options->str, "");
			cvs_execute(plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_diff_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* diff_type;
	GtkWidget* unified_diff;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_diff", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_diff");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	set_editor_filename(plugin, fileentry);
	diff_type = glade_xml_get_widget(gxml, "cvs_diff_type");
	unified_diff = glade_xml_get_widget(gxml, "cvs_unified");
	gtk_combo_box_set_active(GTK_COMBO_BOX(diff_type), DIFF_PATCH);
	g_signal_connect(G_OBJECT(diff_type), "changed", 
		G_CALLBACK(diff_type_changed), unified_diff);
	
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* diff;
		gchar* command;
		gchar* rev;
		int diff_type_nr;
		GtkWidget* norecurse;
		GtkWidget* revisionentry;
		GString* options;

		options = g_string_new("");

		revisionentry = glade_xml_get_widget(gxml, "cvs_revision");
		rev = g_strdup(gtk_entry_get_text(GTK_ENTRY(revisionentry)));
		if (strlen(rev))
		{
			g_string_append_printf(options, " -r %s", rev);
		}
		g_free(rev);
		
		norecurse = glade_xml_get_widget(gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		
		diff_type_nr = gtk_combo_box_get_active(GTK_COMBO_BOX(diff_type));
		if (diff_type_nr == DIFF_PATCH)
		{
			add_option(unified_diff, options, "-u");
			/* FIXME: rdiff do not take -u in my cvs */
			/* diff = "rdiff"; */
			diff = "diff";
		}
		else
			diff = "diff";
		
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!strlen(filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), diff, options->str, basename(filename));
			cvs_execute_diff(plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), diff, options->str, "");
			cvs_execute_diff(plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_status_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_status", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_status");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	set_editor_filename(plugin, fileentry);
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		GtkWidget* norecurse;
		GtkWidget* verbose;
		
		GString* options;
		options = g_string_new("");

		norecurse = glade_xml_get_widget(gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
	
		verbose = glade_xml_get_widget(gxml, "cvs_verbose");
		add_option(verbose, options, "-v");

		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!strlen(filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "status", options->str, basename(filename));
			cvs_execute_status(plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "status", options->str, "");
			cvs_execute_status(plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

void on_cvs_import_activate (GtkAction* action, CVSPlugin* plugin)
{

}

void on_cvs_log_activate (GtkAction* action, CVSPlugin* plugin)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_log", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_log");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	set_editor_filename(plugin, fileentry);
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GString* options;

		options = g_string_new("");
		
		norecurse = glade_xml_get_widget(gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		
		verbose = glade_xml_get_widget(gxml, "cvs_verbose");
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verbose)))
		{
			g_string_append(options, " -h");
		}
		
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!strlen(filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "log", options->str, basename(filename));
			cvs_execute_log(plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
									 NULL), "log", options->str, "");
			cvs_execute_log(plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		break;
		}
	default:
		break;
	}
	gtk_widget_destroy (dialog);
}

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

#include "glade/glade.h"
#include <libgnomevfs/gnome-vfs.h>
#include "libgen.h"

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>

void cvs_add_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_remove_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_commit_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_update_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_diff_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_status_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);
void cvs_log_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename);

enum
{
	DIFF_STANDARD = 0,
	DIFF_PATCH = 1
};

enum
{
	SERVER_LOCAL = 0,
	SERVER_EXTERN = 1,
	SERVER_PASSWORD = 2,
};

static gchar* create_cvs_command_with_cvsroot(AnjutaPreferences* prefs,
								const gchar* action, 
								const gchar* command_options,
								const gchar* command_arguments,
								const gchar* cvsroot)
{
	gchar* cvs;
	gchar* global_options = NULL;
	gboolean ignorerc;
	gint compression;
	gchar* command;
	/* command global_options cvsroot action command_options command_arguments */
	gchar* CVS_FORMAT = "%s %s %s %s %s %s";
	
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
	if (cvsroot == NULL)
	{
		cvsroot = "";
	}
	command = g_strdup_printf(CVS_FORMAT, cvs, global_options, cvsroot, action,  
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

inline static gchar* create_cvs_command(AnjutaPreferences* prefs,
								const gchar* action, 
								const gchar* command_options,
								const gchar* command_arguments)
{
	return create_cvs_command_with_cvsroot(prefs, action, command_options,
		command_arguments, NULL);
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
			g_warning("Unknown CVS server type");
	}
}

static gchar* get_log_from_textview(GtkWidget* textview)
{
	gchar* log;
	GtkTextBuffer* textbuf;
	GtkTextIter iterbegin, iterend;
	gchar* escaped_log;
	
	textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_get_start_iter(textbuf, &iterbegin);
	gtk_text_buffer_get_end_iter(textbuf, &iterend) ;
	log = gtk_text_buffer_get_text(textbuf, &iterbegin, &iterend, FALSE);
/* #warning FIXME: Check for escape chars in log */
	/* Fixed. -naba*/
	escaped_log = anjuta_util_escape_quotes (log);
	log = g_strdup_printf("-m '%s'", escaped_log);
	g_free (escaped_log);
	return log;
}

void cvs_add_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* binary;
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_add", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_add");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
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

void cvs_remove_dialog(GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_remove", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_remove");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);

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

void cvs_commit_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_commit", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_commit");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		gchar* log;
		gchar* rev;
		GString* options;
		GtkWidget* logtext;
		GtkWidget* revisionentry;
		GtkWidget* norecurse;
		
		options = g_string_new("");
		
		logtext = glade_xml_get_widget(gxml, "cvs_log");
		log = get_log_from_textview(logtext);
		g_string_append(options, log);
		g_free(log);

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

void cvs_update_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* revisionentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_update", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_update");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
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

void cvs_diff_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
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
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	diff_type = glade_xml_get_widget(gxml, "cvs_diff_type");
	unified_diff = glade_xml_get_widget(gxml, "cvs_unified");
	gtk_combo_box_set_active(GTK_COMBO_BOX(diff_type), DIFF_PATCH);
	g_signal_connect(G_OBJECT(diff_type), "changed", 
		G_CALLBACK(on_diff_type_changed), unified_diff);
	
	
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

void cvs_status_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_status", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_status");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
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

void cvs_log_dialog (GtkAction* action, CVSPlugin* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_logdialog", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_logdialog");
	fileentry = glade_xml_get_widget(gxml, "cvs_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
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
	
	gint result;
	gxml = glade_xml_new(GLADE_FILE, "cvs_import", NULL);
	
	dialog = glade_xml_get_widget(gxml, "cvs_import");
	direntry = glade_xml_get_widget(gxml, "cvs_rootdir");
	typecombo = glade_xml_get_widget(gxml, "cvs_server_type");
	
	g_signal_connect (G_OBJECT(typecombo), "changed", 
		G_CALLBACK(on_server_type_changed), gxml);
	gtk_combo_box_set_active(GTK_COMBO_BOX(typecombo), SERVER_LOCAL);
	
	if (plugin->project_root_dir)
		gtk_entry_set_text(GTK_ENTRY(direntry), plugin->project_root_dir);
	
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
		case GTK_RESPONSE_OK:
		{
			GtkWidget* username;
			GtkWidget* password;
			GtkWidget* cvsroot_entry;
			GtkWidget* module_entry;
			GtkWidget* vendortag;
			GtkWidget* releasetag;
			GtkWidget* logtext;
			gchar* cvsroot = NULL;
			gchar* cvs_command;
			gchar* log;
			GString* options;

			username = glade_xml_get_widget(gxml, "cvs_username");
			password = glade_xml_get_widget(gxml, "cvs_password");
			cvsroot_entry = glade_xml_get_widget(gxml, "cvs_cvsroot");
			module_entry = glade_xml_get_widget(gxml, "cvs_module");
			vendortag = glade_xml_get_widget(gxml, "cvs_vendor");
			releasetag = glade_xml_get_widget(gxml, "cvs_release");
			logtext = glade_xml_get_widget(gxml, "cvs_log");
			
			switch (gtk_combo_box_get_active(GTK_COMBO_BOX(typecombo)))
			{
				case SERVER_LOCAL:
				{
					cvsroot = g_strdup_printf("-d %s", 
						gtk_entry_get_text(GTK_ENTRY(cvsroot_entry)));
					break;
				}
				case SERVER_EXTERN:
				{
					cvsroot = g_strdup_printf("-d:ext:%s@%s",
						gtk_entry_get_text(GTK_ENTRY(username)),
						gtk_entry_get_text(GTK_ENTRY(cvsroot_entry)));
					break;
				}
				case SERVER_PASSWORD:
				{
					cvsroot = g_strdup_printf("-d:pserver:%s:%s@%s",
						gtk_entry_get_text(GTK_ENTRY(username)),
						gtk_entry_get_text(GTK_ENTRY(password)),
						gtk_entry_get_text(GTK_ENTRY(cvsroot_entry)));
					break;
				}
				default:
					g_warning("Invalid cvs server type!");
			}
			log = get_log_from_textview(logtext);
			options = g_string_new(log);
			g_string_append_printf(options, " %s %s %s",
				gtk_entry_get_text(GTK_ENTRY(module_entry)), 
				gtk_entry_get_text(GTK_ENTRY(vendortag)),
				gtk_entry_get_text(GTK_ENTRY(releasetag)));
			cvs_command = create_cvs_command_with_cvsroot(
				anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell, NULL),
				"import", options->str, "", cvsroot);
			cvs_execute(plugin, cvs_command, 
				gtk_entry_get_text(GTK_ENTRY(direntry)));
			g_string_free(options, TRUE);
			g_free(log);
			g_free(cvsroot);
			g_free(cvs_command);
			break;
		}
		default:
			break;
	}
	gtk_widget_destroy(dialog);
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

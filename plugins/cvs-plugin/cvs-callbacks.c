/*
 * cvs-callbacks.c (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "cvs-callbacks.h"
#include "cvs-execute.h"

#include "glade/glade.h"
#include <libgnomevfs/gnome-vfs.h>
#include "libgen.h"

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


static gboolean check_filename(GtkDialog* dialog, const gchar* filename)
{
	if (!strlen(filename))
	{
		GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE, _("Please enter a filename!"));
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		return FALSE;
	}
	return TRUE;
}

CVSData* cvs_data_new(CVSPlugin* plugin, GladeXML* gxml)
{
	CVSData* data = g_new0(CVSData, 1);
	data->plugin = plugin;
	data->gxml = gxml;
	
	return data;
}

void cvs_data_free(CVSData* data)
{
	g_free(data);
}

void
on_cvs_add_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		GString* options = g_string_new("");
		GtkWidget* binary = glade_xml_get_widget(data->gxml, "cvs_binary");
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
	
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!check_filename(dialog, filename))
			break;
		add_option(binary, options, "-kb");
		
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "add", options->str, basename(filename));
		cvs_execute(data->plugin, command, dirname(filename));
		g_free(command);
		g_free(filename);
		gtk_widget_destroy(GTK_WIDGET(dialog));
		cvs_data_free(data);
		break;
	}
	default:
		gtk_widget_destroy (GTK_WIDGET(dialog));
		cvs_data_free(data);
	}
}

void
on_cvs_remove_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!check_filename(dialog, filename))
			break;
		
		if (gnome_vfs_unlink(gtk_entry_get_text(GTK_ENTRY(fileentry)))
			!= GNOME_VFS_OK)
		{
			anjuta_util_dialog_error
				(GTK_WINDOW(dialog),_("Unable to delete file"), NULL);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
	
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "remove", "", basename(filename));
		cvs_execute(data->plugin, command, dirname(filename));
		g_free(command);
		g_free(filename);
		cvs_data_free(data);
		break;
		}
	default:
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));
	}
}

void
on_cvs_update_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		gchar* revision;
		
		GString* options;
		GtkWidget* fileentry;
		GtkWidget* createdir;
		GtkWidget* removedir;
		GtkWidget* norecurse;
		GtkWidget* removesticky;
		GtkWidget* revisionentry;

		options = g_string_new("");

		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		removedir = glade_xml_get_widget(data->gxml, "cvs_removedir");
		add_option(removedir, options, "-P");
		createdir = glade_xml_get_widget(data->gxml, "cvs_createdir");
		add_option(createdir, options, "-d");

		revisionentry = glade_xml_get_widget(data->gxml, "cvs_revision");
		revision = g_strdup(gtk_entry_get_text(GTK_ENTRY(revisionentry)));
		if (strlen(revision))
		{
			g_string_append_printf(options, " -r %s", revision);
		}
		else
		{
			removesticky = glade_xml_get_widget(data->gxml, "cvs_removesticky");
			add_option(removesticky, options, "-A");
		}
		g_free(revision);
		
		fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!check_filename(dialog, filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "update", options->str, basename(filename));
			cvs_execute(data->plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "update", options->str, "");
			cvs_execute(data->plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		cvs_data_free(data);
		gtk_widget_destroy(GTK_WIDGET(dialog));
		break;
		}
	default:
		gtk_widget_destroy(GTK_WIDGET(dialog));
		cvs_data_free(data);
		break;
	}
}

void
on_cvs_commit_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
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
		GtkWidget* fileentry;
		
		options = g_string_new("");
		
		logtext = glade_xml_get_widget(data->gxml, "cvs_log");
		log = get_log_from_textview(logtext);
		if (!strlen(log))
		{
			gint result;
			GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
				GTK_BUTTONS_YES_NO, 
				_("Are you sure that you do not want a log message?"));
			result = gtk_dialog_run(GTK_DIALOG(dlg));
			if (result == GTK_RESPONSE_NO)
				break;
			gtk_widget_destroy(dlg);
		}
		g_string_append(options, log);
		g_free(log);

		revisionentry = glade_xml_get_widget(data->gxml, "cvs_revision");
		rev = g_strdup(gtk_entry_get_text(GTK_ENTRY(revisionentry)));
		if (strlen(rev))
		{
			g_string_append_printf(options, " -r %s", rev);
		}
		g_free(rev);

		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		
		fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!check_filename(dialog, filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "commit", options->str, basename(filename));
			cvs_execute(data->plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "commit", options->str, "");
			cvs_execute(data->plugin, command, filename);
		}
			
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		cvs_data_free(data);
		gtk_widget_destroy(GTK_WIDGET(dialog));
		break;
		}
	default:
		gtk_widget_destroy (GTK_WIDGET(dialog));
		cvs_data_free(data);
		break;
	}
}

void
on_cvs_diff_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
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
		GtkWidget* fileentry;
		GtkWidget* diff_type;
		GtkWidget* unified_diff;
		GString* options;

		options = g_string_new("");

		revisionentry = glade_xml_get_widget(data->gxml, "cvs_revision");
		rev = g_strdup(gtk_entry_get_text(GTK_ENTRY(revisionentry)));
		if (strlen(rev))
		{
			g_string_append_printf(options, " -r %s", rev);
		}
		g_free(rev);
		
		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		
		diff_type = glade_xml_get_widget(data->gxml, "cvs_diff_type");
		unified_diff = glade_xml_get_widget(data->gxml, "cvs_unified");
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
		
		fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!check_filename(dialog, filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), diff, options->str, basename(filename));
			cvs_execute_diff(data->plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), diff, options->str, "");
			cvs_execute_diff(data->plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));	
		break;
		}
	default:
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));	
		break;
	}
}

void
on_cvs_status_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GtkWidget* fileentry;
		
		GString* options;
		options = g_string_new("");

		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
	
		verbose = glade_xml_get_widget(data->gxml, "cvs_verbose");
		add_option(verbose, options, "-v");

		fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!check_filename(dialog, filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "status", options->str, basename(filename));
			cvs_execute_status(data->plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "status", options->str, "");
			cvs_execute_status(data->plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));
		break;
		}
	default:
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));
		break;
	}
}

void
on_cvs_log_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* filename;
		gchar* command;
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GtkWidget* fileentry;
		GString* options;

		options = g_string_new("");
		
		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		add_option(norecurse, options, "-l");
		
		verbose = glade_xml_get_widget(data->gxml, "cvs_verbose");
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verbose)))
		{
			g_string_append(options, " -h");
		}
		
		fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!strlen(filename))
			break;	
		
		if (!is_directory(filename))
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "log", options->str, basename(filename));
			cvs_execute_log(data->plugin, command, dirname(filename));
		}
		else
		{
			command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell,
									 NULL), "log", options->str, "");
			cvs_execute_log(data->plugin, command, filename);
		}
		g_free(command);
		g_free(filename);
		g_string_free(options, TRUE);
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));		
		break;
		}
	default:
		cvs_data_free(data);
		gtk_widget_destroy (GTK_WIDGET(dialog));
		break;
	}
}

void
on_cvs_import_response(GtkDialog* dialog, gint response, CVSData* data)
{
	switch (response)
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
			GtkWidget* typecombo;
			GtkWidget* direntry;
			gchar* cvsroot = NULL;
			gchar* cvs_command;
			gchar* log;
			GString* options;

			username = glade_xml_get_widget(data->gxml, "cvs_username");
			password = glade_xml_get_widget(data->gxml, "cvs_password");
			cvsroot_entry = glade_xml_get_widget(data->gxml, "cvs_cvsroot");
			module_entry = glade_xml_get_widget(data->gxml, "cvs_module");
			vendortag = glade_xml_get_widget(data->gxml, "cvs_vendor");
			releasetag = glade_xml_get_widget(data->gxml, "cvs_release");
			logtext = glade_xml_get_widget(data->gxml, "cvs_log");
			typecombo = glade_xml_get_widget(data->gxml, "cvs_server_type");
			direntry = glade_xml_get_widget(data->gxml, "cvs_rootdir");

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
				anjuta_shell_get_preferences (ANJUTA_PLUGIN(data->plugin)->shell, NULL),
				"import", options->str, "", cvsroot);
			cvs_execute(data->plugin, cvs_command, 
				gtk_entry_get_text(GTK_ENTRY(direntry)));
			g_string_free(options, TRUE);
			g_free(log);
			g_free(cvsroot);
			g_free(cvs_command);
			cvs_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
		default:
			cvs_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
	}
}

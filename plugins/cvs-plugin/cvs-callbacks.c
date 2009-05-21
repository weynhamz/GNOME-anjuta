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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "cvs-callbacks.h"
#include "cvs-execute.h"
#include "cvs-interface.h"

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
	return escaped_log;
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

static gboolean check_entry(GtkDialog* dialog, GtkWidget* entry,
	const gchar* stringname)
{
	if (!strlen(gtk_entry_get_text(GTK_ENTRY(entry))))
	{
		gchar* message = g_strdup_printf(_("Please fill field: %s"), stringname);
		GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE, "%s", message);
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		return FALSE;
	}
	return TRUE;
}

static gboolean 
is_busy (CVSPlugin* plugin, GtkDialog* dialog)
{
	if (plugin->executing_command)
	{
		GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE, 
			_("CVS command is running! Please wait until it is finished!"));
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		return TRUE;
	}
	return FALSE;
}

CVSData* cvs_data_new(CVSPlugin* plugin, GtkBuilder* bxml)
{
	CVSData* data = g_new0(CVSData, 1);
	data->plugin = plugin;
	data->bxml = bxml;
	
	return data;
}

void cvs_data_free(CVSData* data)
{
	g_free(data);
}

void
on_cvs_add_response(GtkDialog* dialog, gint response, CVSData* data)
{
	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		GtkWidget* binary = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_binary"));
		GtkWidget* fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_add_filename"));
	
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
		if (!check_filename(dialog, filename))
			break;
		
		anjuta_cvs_add(ANJUTA_PLUGIN(data->plugin), filename, 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(binary)), NULL); 
		
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
	GFile* file;

	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		GtkWidget* fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_remove_filename"));
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
		
		if (!check_filename(dialog, filename))
			break;

		file = g_file_new_for_uri(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		if (!g_file_delete(file, NULL, NULL))
		{
			anjuta_util_dialog_error
				(GTK_WINDOW(dialog),_("Unable to delete file"), NULL);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			cvs_data_free(data);
			break;
		}
		g_object_unref(G_OBJECT(file));
	
		anjuta_cvs_remove(ANJUTA_PLUGIN(data->plugin), filename, NULL);
		gtk_widget_destroy (GTK_WIDGET(dialog));
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
	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		const gchar* revision;
		
		GtkWidget* createdir;
		GtkWidget* removedir;
		GtkWidget* norecurse;
		GtkWidget* removesticky;
		GtkWidget* revisionentry;
		GtkWidget* fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_update_filename"));
		const gchar* filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		
		norecurse = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_update_norecurse"));
		removedir = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_removedir"));
		createdir = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_createdir"));
		revisionentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_update_revision"));
		revision = gtk_entry_get_text(GTK_ENTRY(revisionentry));
		removesticky = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_removesticky"));
				
		if (!check_filename(dialog, filename))
			break;	
		
		anjuta_cvs_update(ANJUTA_PLUGIN(data->plugin), filename, 
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(removedir)),		
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(createdir)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(removesticky)), 
			revision, NULL);
		
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
	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* log;
		const gchar* rev;
		GtkWidget* logtext;
		GtkWidget* revisionentry;
		GtkWidget* norecurse;
		GtkWidget* fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_commit_filename"));
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
				
		logtext = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_commit_log"));
		log = get_log_from_textview(logtext);
		if (!g_utf8_strlen(log, -1))
		{
			gint result;
			GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
				GTK_BUTTONS_YES_NO, 
				_("Are you sure that you want to pass an empty log message?"));
			result = gtk_dialog_run(GTK_DIALOG(dlg));
			if (result == GTK_RESPONSE_NO)
			{
				gtk_widget_hide(dlg);
				gtk_widget_destroy(dlg);
				break;
			}
			gtk_widget_destroy(dlg);
		}
		
		revisionentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_commit_revision"));
		rev = gtk_entry_get_text(GTK_ENTRY(revisionentry));

		norecurse = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_commit_norecurse"));
		
		if (!check_filename(dialog, filename))
			break;	
		
		anjuta_cvs_commit(ANJUTA_PLUGIN(data->plugin), filename, log, rev,
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)), NULL);		
		
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
	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		const gchar* rev;
		int diff_type_nr;
		gboolean unified = FALSE;
		gboolean patch_style = FALSE;
		
		GtkWidget* norecurse;
		GtkWidget* revisionentry;
		GtkWidget* diff_type;
		GtkWidget* unified_diff;
		
		GtkWidget* fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_diff_filename"));
		const gchar* filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		

		revisionentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_diff_revision"));
		rev = gtk_entry_get_text(GTK_ENTRY(revisionentry));
		norecurse = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_diff_norecurse"));
		
		diff_type = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_diff_type"));
		unified_diff = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_unified"));
		diff_type_nr = gtk_combo_box_get_active(GTK_COMBO_BOX(diff_type));
		if (diff_type_nr == DIFF_PATCH)
		{
			unified = TRUE;
			/* FIXME: rdiff do not take -u in my cvs */
			/* diff = "rdiff"; */
		}
		
		if (!check_filename(dialog, filename))
			break;	
		
		anjuta_cvs_diff(ANJUTA_PLUGIN(data->plugin), filename, rev, 
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)), patch_style,
			unified, NULL);
		
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
	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GtkWidget* fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_status_filename"));
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));

		norecurse = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_status_norecurse"));
		verbose = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_status_verbose"));

		
		if (!check_filename(dialog, filename))
			break;	
		
		anjuta_cvs_status(ANJUTA_PLUGIN(data->plugin), filename,
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verbose)), NULL);
		
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
	if (is_busy(data->plugin, dialog))
		return;
	
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		const gchar* filename;
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GtkWidget* fileentry;
		
		norecurse = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_logdialog_norecurse"));
		verbose = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_logdialog_verbose"));
		
		fileentry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_logdialog_filename"));
		filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
		if (!check_filename(dialog, filename))
			break;
		
		anjuta_cvs_log(ANJUTA_PLUGIN(data->plugin), filename,
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verbose)), NULL);
					
		
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
	gchar* dirname = NULL;
	if (is_busy(data->plugin, dialog))
		return;
	
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
			GtkFileChooser* dir;
			gchar* log;
			
			username = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_username"));
			password = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_password"));
			
			cvsroot_entry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_cvsroot"));
			if (!check_entry(dialog, cvsroot_entry, _("CVSROOT")))
				break;
			module_entry = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_module"));
			if (!check_entry(dialog, module_entry, _("Module")))
				break;
			vendortag = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_vendor"));
			if (!check_entry(dialog, vendortag, _("Vendor")))
				break;
			releasetag = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_release"));
			if (!check_entry(dialog, releasetag, _("Release")))
				break;
			typecombo = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_server_type"));
			dir = GTK_FILE_CHOOSER(gtk_builder_get_object(data->bxml, "cvs_rootdir"));
			dirname = gtk_file_chooser_get_filename (dir);
			if (!dirname)
				break;
			
			logtext = GTK_WIDGET(gtk_builder_get_object(data->bxml, "cvs_import_log"));
			log = get_log_from_textview(logtext);
			if (!strlen(log))
			{
				gint result;
				GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
				GTK_BUTTONS_YES_NO, 
				_("Are you sure that you do not want a log message?"));
				result = gtk_dialog_run(GTK_DIALOG(dlg));
				gtk_widget_destroy(dlg);
				if (result == GTK_RESPONSE_NO)
					break;
			}
			
			anjuta_cvs_import(ANJUTA_PLUGIN(data->plugin),
				dirname,
				gtk_entry_get_text(GTK_ENTRY(cvsroot_entry)),
				gtk_entry_get_text(GTK_ENTRY(module_entry)),
				gtk_entry_get_text(GTK_ENTRY(vendortag)),
				gtk_entry_get_text(GTK_ENTRY(releasetag)),
				log,
				gtk_combo_box_get_active(GTK_COMBO_BOX(typecombo)),
				gtk_entry_get_text(GTK_ENTRY(username)),
				gtk_entry_get_text(GTK_ENTRY(password)), NULL);
			
			cvs_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
		default:
			cvs_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
	}
	g_free(dirname);
}

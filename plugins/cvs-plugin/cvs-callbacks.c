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
#include "cvs-interface.h"
#include "glade/glade.h"
#include <libgnomevfs/gnome-vfs.h>

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
			GTK_BUTTONS_CLOSE, message);
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		return FALSE;
	}
	return TRUE;
	return FALSE;
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
		GtkWidget* binary = glade_xml_get_widget(data->gxml, "cvs_binary");
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
	
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
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
		
		if (!check_filename(dialog, filename))
			break;
		
		if (gnome_vfs_unlink(gtk_entry_get_text(GTK_ENTRY(fileentry)))
			!= GNOME_VFS_OK)
		{
			anjuta_util_dialog_error
				(GTK_WINDOW(dialog),_("Unable to delete file"), NULL);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			cvs_data_free(data);
			break;
		}
	
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
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		const gchar* filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		
		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		removedir = glade_xml_get_widget(data->gxml, "cvs_removedir");
		createdir = glade_xml_get_widget(data->gxml, "cvs_createdir");
		revisionentry = glade_xml_get_widget(data->gxml, "cvs_revision");
		revision = gtk_entry_get_text(GTK_ENTRY(revisionentry));
		removesticky = glade_xml_get_widget(data->gxml, "cvs_removesticky");
				
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
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		gchar* log;
		const gchar* rev;
		GtkWidget* logtext;
		GtkWidget* revisionentry;
		GtkWidget* norecurse;
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
				
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
		
		revisionentry = glade_xml_get_widget(data->gxml, "cvs_revision");
		rev = gtk_entry_get_text(GTK_ENTRY(revisionentry));

		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		
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
		
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		const gchar* filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		

		revisionentry = glade_xml_get_widget(data->gxml, "cvs_revision");
		rev = gtk_entry_get_text(GTK_ENTRY(revisionentry));
		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		
		diff_type = glade_xml_get_widget(data->gxml, "cvs_diff_type");
		unified_diff = glade_xml_get_widget(data->gxml, "cvs_unified");
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
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
		const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));

		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		verbose = glade_xml_get_widget(data->gxml, "cvs_verbose");

		
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
	switch (response)
	{
	case GTK_RESPONSE_OK:
	{
		const gchar* filename;
		GtkWidget* norecurse;
		GtkWidget* verbose;
		GtkWidget* fileentry;
		
		norecurse = glade_xml_get_widget(data->gxml, "cvs_norecurse");
		verbose = glade_xml_get_widget(data->gxml, "cvs_verbose");
		
		fileentry = glade_xml_get_widget(data->gxml, "cvs_filename");
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
			gchar* log;
			
			username = glade_xml_get_widget(data->gxml, "cvs_username");
			password = glade_xml_get_widget(data->gxml, "cvs_password");
			
			cvsroot_entry = glade_xml_get_widget(data->gxml, "cvs_cvsroot");
			if (!check_entry(dialog, cvsroot_entry, _("CVSROOT")))
				break;
			module_entry = glade_xml_get_widget(data->gxml, "cvs_module");
			if (!check_entry(dialog, module_entry, _("Module")))
				break;
			vendortag = glade_xml_get_widget(data->gxml, "cvs_vendor");
			if (!check_entry(dialog, vendortag, _("Vendor")))
				break;
			releasetag = glade_xml_get_widget(data->gxml, "cvs_release");
			if (!check_entry(dialog, releasetag, _("Release")))
				break;
			typecombo = glade_xml_get_widget(data->gxml, "cvs_server_type");
			direntry = glade_xml_get_widget(data->gxml, "cvs_rootdir");
			if (!check_entry(dialog, direntry, _("Directory")))
				break;
			
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
			
			anjuta_cvs_import(ANJUTA_PLUGIN(data->plugin),
				gtk_entry_get_text(GTK_ENTRY(direntry)),
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
}

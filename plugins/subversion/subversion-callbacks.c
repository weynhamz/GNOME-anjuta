/*
 * subversion-callbacks.c (c) 2005 Johannes Schmid
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

#include "subversion-callbacks.h"
#include "svn-backend.h"
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

static gboolean 
is_busy (GtkDialog* dialog, Subversion* plugin)
{
	if (svn_backend_busy(plugin->backend))
	{
		GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE, 
			_("Subversion command is running! Please wait until it is finished!"));
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		return TRUE;
	}
	return FALSE;
}

/* static gboolean check_entry(GtkDialog* dialog, GtkWidget* entry,
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
}*/

SubversionData* subversion_data_new(Subversion* plugin, GladeXML* gxml)
{
	SubversionData* data = g_new0(SubversionData, 1);
	data->plugin = plugin;
	data->gxml = gxml;
	
	return data;
}

void subversion_data_free(SubversionData* data)
{
	g_free(data);
}

void
on_subversion_add_response(GtkDialog* dialog, gint response, SubversionData* data)
{
	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			GtkWidget* force = glade_xml_get_widget(data->gxml, "subversion_force");
			GtkWidget* recurse = glade_xml_get_widget(data->gxml, "subversion_recurse");
			
			const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
			
			if (is_busy(dialog, data->plugin))
				return;
			
			if (!check_filename(dialog, filename))
				break;
			
			svn_backend_add(data->plugin->backend, filename, 
							gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(force)),
							gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recurse)));
			
			gtk_widget_destroy(GTK_WIDGET(dialog));
			subversion_data_free(data);
			break;
		}
		default:
		{
			gtk_widget_destroy (GTK_WIDGET(dialog));
			subversion_data_free(data);
		}
	}
}

void
on_subversion_remove_response(GtkDialog* dialog, gint response, SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			GtkWidget* force = glade_xml_get_widget(data->gxml, "subversion_force");
			const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
		
			if (is_busy(dialog, data->plugin))
				return;
			
			if (!check_filename(dialog, filename))
				break;
			
			
			svn_backend_remove(data->plugin->backend, filename, 
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(force)));
			subversion_data_free(data);
			gtk_widget_destroy (GTK_WIDGET(dialog));
			break;
		}
		default:
		{
			subversion_data_free(data);
			gtk_widget_destroy (GTK_WIDGET(dialog));
		}
	}
}

void
on_subversion_update_response(GtkDialog* dialog, gint response, SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			const gchar* revision;
		
			GtkWidget* norecurse;
			GtkWidget* revisionentry;
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			const gchar* filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
		
			if (is_busy(dialog, data->plugin))
					return;
			
			norecurse = glade_xml_get_widget(data->gxml, "subversion_norecurse");
			revisionentry = glade_xml_get_widget(data->gxml, "subversion_revision");
			revision = gtk_entry_get_text(GTK_ENTRY(revisionentry));
			
			if (!check_filename(dialog, filename))
				break;	
			
			svn_backend_update(data->plugin->backend, filename, revision,
							   !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)));
			
			subversion_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
		default:
		{
			gtk_widget_destroy(GTK_WIDGET(dialog));
			subversion_data_free(data);
			break;
		}
	}
}

void
on_subversion_commit_response(GtkDialog* dialog, gint response, SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			gchar* log;
			GtkWidget* logtext;
			GtkWidget* norecurse;
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
			
			if (is_busy(dialog, data->plugin))
				return;
			
			logtext = glade_xml_get_widget(data->gxml, "subversion_log");
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
			
			norecurse = glade_xml_get_widget(data->gxml, "subversion_norecurse");
			
			if (!check_filename(dialog, filename))
				break;	
			
			svn_backend_commit(data->plugin->backend, filename, log,
							   !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)));		
			
			subversion_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
		default:
		{
			gtk_widget_destroy (GTK_WIDGET(dialog));
			subversion_data_free(data);
			break;
		}
	}
}

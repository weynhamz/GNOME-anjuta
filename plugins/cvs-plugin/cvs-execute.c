/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    cvs-execute.c
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

#include "cvs-execute.h"
#include "plugin.h"

#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>

#include <glade/glade.h>
#include <pcre.h>

#define CVS_ICON ""
#define CVS_INFO_REGEXP "cvs update:."
#define CVS_ERR_REGEXP "^C ."


static GtkWidget* status_text;

static void
on_cvs_mesg_format (IAnjutaMessageView *view, const gchar *line,
					  AnjutaPlugin *plugin)
{
	IAnjutaMessageViewType type;
	pcre *info, *err;
	const gchar *err_buf;
	int err_ptr, output[16];
	
	g_return_if_fail (line != NULL);

	/* Compile the regexps for message types. */
	if (!(info = pcre_compile(CVS_INFO_REGEXP, 0, &err_buf, &err_ptr, NULL)))
	{
		g_free((gchar *) err_buf);
		return;
	}
	if (!(err = pcre_compile(CVS_ERR_REGEXP, 0, &err_buf, &err_ptr, NULL)))
	{
		g_free((gchar *) err_buf);
		return;
	}		
	
	/* Match against type regexps to find the message type. */
	if (pcre_exec(info, NULL, line, strlen(line), 0, 0, output, 16) >= 0)
	{
		type = IANJUTA_MESSAGE_VIEW_TYPE_INFO;
	}
	else if (pcre_exec(err, NULL, line, strlen(line), 0, 0, output, 16) >= 0)
	{
		type = IANJUTA_MESSAGE_VIEW_TYPE_ERROR;
	}
	else type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;

	ianjuta_message_view_append (view, type, line, "", NULL);
	
	pcre_free(info);
	pcre_free(err);
}

static void
on_cvs_mesg_parse (IAnjutaMessageView *view, const gchar *line,
					 AnjutaPlugin *plugin)
{
	/* FIXME: Parse the line and determine if there is filename to goto.
	   If there is, extract filename then open it.

	*/

#if 0
	gchar *filename;
	gint lineno;
	
	if ((filename = parse_filename (line)))
	{
		gchar *uri;
		IAnjutaFileLoader *loader;
		
		/* Go to file and line number */
		loader = anjuta_shell_get_interface (plugin->shell, IAnjutaFileLoader,
											 NULL);
		
		/* FIXME: Determine full file path */
		uri = g_strdup_printf ("file:///%s", filename);
		ianjuta_file_loader_load (loader, uri, FALSE, NULL);
		g_free (uri);
		g_free (filename);
	}
#endif
}

static void
on_cvs_terminated (AnjutaLauncher *launcher,
					 gint child_pid, gint status, gulong time_taken,
					 CVSPlugin *plugin)
{
	g_return_if_fail (plugin != NULL);
	
	g_signal_handlers_disconnect_by_func (plugin->mesg_view,
										  G_CALLBACK (on_cvs_mesg_format),
										  plugin);
	g_signal_handlers_disconnect_by_func (plugin->mesg_view,
										  G_CALLBACK (on_cvs_mesg_parse),
										  plugin);
	DEBUG_PRINT ("Shuting down cvs message view");
	
	if (status != 0)
	{
		ianjuta_message_view_append (plugin->mesg_view,
									 IANJUTA_MESSAGE_VIEW_TYPE_INFO,
			_("CVS command failed! - See above for details"), "", NULL);
	}
	else
	{
		gchar *mesg;
		mesg = g_strdup_printf (_("CVS command successful! - Time taken %ld secs."),
								time_taken);
		ianjuta_message_view_append (plugin->mesg_view,
									 IANJUTA_MESSAGE_VIEW_TYPE_INFO,
									 mesg, "", NULL);
		g_free (mesg);
	}

	/* We do not care about this view any longer, it will be freed when 
	the users closes it */
	plugin->mesg_view = NULL;
	plugin->executing_command = FALSE;
}

static void
on_cvs_message (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, gpointer user_data)
{
	CVSPlugin* plugin = (CVSPlugin*)user_data;
	ianjuta_message_view_buffer_append (plugin->mesg_view, mesg, NULL);
}

static void 
on_cvs_status(AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, gpointer user_data)
{
	CVSPlugin* plugin = (CVSPlugin*)user_data;
	switch(output_type)
	{
		case ANJUTA_LAUNCHER_OUTPUT_STDERR:
			ianjuta_message_view_buffer_append (plugin->mesg_view, mesg, NULL);
			break;
		default:
			{
				GtkTextBuffer* textbuf;
				GtkTextIter end;
				
				if (status_text)
				{
					textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(status_text));
					gtk_text_buffer_get_end_iter(textbuf, &end);
					
					gtk_text_buffer_insert(textbuf, &end, mesg, -1);
				}
			}
	}	
}

static void
on_cvs_diff(AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, gpointer user_data)
{
	g_return_if_fail (user_data != NULL);
	CVSPlugin* plugin = (CVSPlugin*)user_data;
	
	switch(output_type)
	{
		case ANJUTA_LAUNCHER_OUTPUT_STDERR:
			ianjuta_message_view_buffer_append (plugin->mesg_view, mesg, NULL);
			break;
		default:
			ianjuta_editor_insert(plugin->diff_editor, -1, mesg, -1, NULL);
	}
}

static gboolean
on_cvs_status_destroy(GtkWidget* window, GdkEvent* event, gpointer data)
{
	status_text = NULL;
	
	return FALSE; /* Do not block the event */
}

static void 
cvs_execute_common (CVSPlugin* plugin, const gchar* command, const gchar* dir,
					AnjutaLauncherOutputCallback output)
{
	IAnjutaMessageManager *mesg_manager;
	
	g_return_if_fail (command != NULL);
	g_return_if_fail (dir != NULL);	
	
	if (plugin->executing_command)
	{
		anjuta_util_dialog_error
			(NULL,_("CVS command is running - please wait until it finishes!"), NULL);
		return;
	}
		
	mesg_manager = anjuta_shell_get_interface 
		(ANJUTA_PLUGIN (plugin)->shell,	IAnjutaMessageManager, NULL);
	plugin->mesg_view =
		ianjuta_message_manager_add_view (mesg_manager, _("CVS"), 
		CVS_ICON, NULL);
	g_signal_connect (G_OBJECT (plugin->mesg_view), "buffer-flushed",
					  G_CALLBACK (on_cvs_mesg_format), plugin);
	g_signal_connect (G_OBJECT (plugin->mesg_view), "message-clicked",
					  G_CALLBACK (on_cvs_mesg_parse), plugin);

	if (plugin->launcher == NULL)
	{
		plugin->launcher = anjuta_launcher_new ();
		
		g_signal_connect (G_OBJECT (plugin->launcher), "child-exited",
						  G_CALLBACK (on_cvs_terminated), plugin);
	}
	chdir (dir);
	plugin->executing_command = TRUE;

	DEBUG_PRINT ("CVS Executing: %s", command);
	ianjuta_message_view_append (plugin->mesg_view,
								 IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 command, "", NULL);

	anjuta_launcher_execute (plugin->launcher, command,
							 output, plugin);
}

void 
cvs_execute(CVSPlugin* plugin, const gchar* command, const gchar* dir)
{
	cvs_execute_common(plugin, command, dir, on_cvs_message);	
}

void
cvs_execute_status(CVSPlugin* plugin, const gchar* command, const gchar* dir)
{
	GladeXML* gxml;
	GtkWidget* window;
	
	gxml = glade_xml_new(GLADE_FILE, "cvs_status_output", NULL);
	window = glade_xml_get_widget(gxml, "cvs_status_output");
	status_text = glade_xml_get_widget(gxml, "cvs_status_text");
	
	g_signal_connect(G_OBJECT(window), "delete-event", 
		G_CALLBACK(on_cvs_status_destroy), status_text);
	
	gtk_widget_show(window);
	cvs_execute_common(plugin, command, dir, on_cvs_status);		
}

void
cvs_execute_diff(CVSPlugin* plugin, const gchar* command, const gchar* dir)
{
	IAnjutaDocumentManager *docman;
	
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	plugin->diff_editor = ianjuta_document_manager_add_buffer(docman, _("cvs.diff"), "", NULL);


	cvs_execute_common(plugin, command, dir, on_cvs_diff);
}

void
cvs_execute_log(CVSPlugin* plugin, const gchar* command, const gchar* dir)
{
	IAnjutaDocumentManager *docman;
	
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	plugin->diff_editor = ianjuta_document_manager_add_buffer(docman, _("cvs.log"), "", NULL);


	cvs_execute_common(plugin, command, dir, on_cvs_diff);
}

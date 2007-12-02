/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    gdb_info.h
    Copyright (C) Naba Kumar  <naba@gnome.org>

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <libanjuta/resources.h>
#include <libgnome/gnome-i18n.h>

#include "utilities.h"
#include "info.h"

static GtkWidget *
create_dialog_with_textview (GtkWindow *parent, gint width, gint height)
{
	GtkWidget *dialog;
	GtkWidget *textview;
	GtkWidget *scrolledwindow;

	if (height < 250)
		height = 250;

	if (width < 400)
		width = 400;

	dialog = gtk_dialog_new_with_buttons (_("Information"), parent,
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
										  NULL);
	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, TRUE, FALSE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 250);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "infoless", "Anjuta");
	gtk_widget_show (dialog);
	/* Auto close */
	g_signal_connect (G_OBJECT (dialog), "response",
					 G_CALLBACK (gtk_widget_destroy), NULL);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
					   scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
										 GTK_SHADOW_IN);
	gtk_widget_show (scrolledwindow);
	
	textview = gtk_text_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_WORD);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), textview);
	gtk_widget_show (textview);

	gtk_widget_show (dialog);

	return textview;
}

static GtkWidget *
create_dialog_with_treeview (GtkWindow *parent, gint width, gint height)
{
	GtkWidget *dialog;
	GtkWidget *scrolledwindow;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *treeview;

	if (height < 250)
		height = 250;

	if (width < 400)
		width = 400;

	dialog = gtk_dialog_new_with_buttons (_("Information"), parent,
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
										  NULL);
	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, TRUE, FALSE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 250);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "infoless", "Anjuta");
	gtk_widget_show (dialog);
	/* Auto close */
	g_signal_connect (G_OBJECT (dialog), "response",
					 G_CALLBACK (gtk_widget_destroy), NULL);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
					   scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
										 GTK_SHADOW_IN);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));

	treeview = gtk_tree_view_new_with_model (model);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Lines"), renderer,
													   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);
	gtk_widget_show (treeview);
	g_object_unref (G_OBJECT (model));

	return treeview;
}

gboolean
gdb_info_show_file (GtkWindow *parent, const gchar *path,
					gint width, gint height)
{
	FILE *f;

	g_return_val_if_fail (path != NULL, FALSE);

	if (!g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
		return FALSE;

	if ((f = fopen (path, "r")) == NULL)
		return FALSE;

	if (!gdb_info_show_filestream (parent, f, width, height))
	{
		int errno_bak = errno;

		fclose (f);

		errno = errno_bak;

		return FALSE;
	}

	return (fclose (f) != 0) ? FALSE : TRUE;
}

gboolean
gdb_info_show_command (GtkWindow *parent, const gchar *command_line,
					   gint width, gint height)
{
	GError *err = NULL;
	gchar *std_output = NULL;
	gboolean ret;

	g_return_val_if_fail (command_line != NULL, FALSE);

	/* Note: we could use popen like gdb_info_show_file,
	 * but g_spawn is safier
	 */
	if (!g_spawn_command_line_sync (command_line, &std_output,
									NULL, NULL, &err))
	{
		g_warning (err->message);
		g_error_free (err);

		return FALSE;
	}

	if (!g_utf8_validate (std_output, strlen (std_output), NULL))
		g_warning ("Invalid UTF-8 data encountered reading output of command '%s'",
				   command_line);

	ret = gdb_info_show_string (parent, std_output, width, height);

	g_free (std_output);

	return ret;
}

gboolean
gdb_info_show_string (GtkWindow *parent, const gchar *s,
					  gint width, gint height)
{
	GtkWidget *textview;
	GtkTextBuffer *buffer;

	g_return_val_if_fail (s != NULL, FALSE);

	textview = create_dialog_with_textview (parent,
											width, height);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_set_text (buffer, s, strlen (s));

	return TRUE;
}

#define ANJUTA_INFO_TEXTVIEW_BUFSIZE 1024

gboolean
gdb_info_show_filestream (GtkWindow *parent, FILE *f,
						  gint  width, gint  height)
{
	GtkWidget *textview;
	GtkTextBuffer *buffer;
	gchar buf[ANJUTA_INFO_TEXTVIEW_BUFSIZE];

	g_return_val_if_fail (f != NULL, FALSE);

	textview = create_dialog_with_textview (parent, width, height);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

        errno = 0; /* Reset it to detect errors */

	while (1) {
		GtkTextIter iter;
		gchar *s;

		if ((s = fgets (buf, ANJUTA_INFO_TEXTVIEW_BUFSIZE, f)) == NULL)
			break;

		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_buffer_insert (buffer, &iter, buf, strlen (buf));
	}
	return errno ? FALSE : TRUE;
}

gboolean
gdb_info_show_fd (GtkWindow *parent, int file_descriptor,
				  gint width, gint height)
{
	FILE *f;

	if ((f = fdopen (file_descriptor, "r")) == NULL)
		return FALSE;

	if (!gdb_info_show_filestream (parent, f, width, height))
	{
		int errno_bak = errno;

		fclose (f);

		errno = errno_bak;

		return FALSE;
	}

	return (fclose (f) != 0) ? FALSE : TRUE;
}

void
gdb_info_show_list (GtkWindow *parent, const GList* list,
					gint   width, gint   height)
{
	GtkWidget *treeview;
	GtkTreeModel *model;

	g_return_if_fail (list != NULL);

	treeview = create_dialog_with_treeview (parent, width, height);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

	for (; list; list = g_list_next (list))
	{
		GtkTreeIter iter;
		gchar *tmp;

		tmp = gdb_util_remove_white_spaces (list->data);

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, tmp, -1);

		g_free (tmp);
	}
}

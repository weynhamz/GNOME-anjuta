/*
    anjuta_info.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>

#include "anjuta.h"
#include "resources.h"
#include "utilities.h"
#include "anjuta_info.h"

static GtkWidget *
create_anjuta_info_dialog_with_textview (gint width,
					 gint height)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *action_area;
	GtkWidget *textview;
	GtkWidget *close_button;

	if (height < 250)
		height = 250;

	if (width < 400)
		width = 400;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), _("Information"));

	vbox = GTK_DIALOG (dialog)->vbox;
	gtk_widget_show (vbox);

	action_area = GTK_DIALOG (dialog)->action_area;
	gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);
	gtk_widget_show (action_area);

	textview = gtk_text_view_new ();
	gtk_box_pack_start (GTK_BOX (vbox), textview, TRUE, TRUE, 0);
	gtk_widget_show (textview);

	close_button = gtk_button_new_from_stock ("gtk-close");
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), close_button, GTK_RESPONSE_CLOSE);
	GTK_WIDGET_SET_FLAGS (close_button, GTK_CAN_DEFAULT);
	gtk_widget_show (close_button);

	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, TRUE, FALSE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 250);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "infoless", "Anjuta");

	g_signal_connect (dialog, "delete_event", G_CALLBACK (gtk_widget_hide), NULL);
	gtk_widget_show (dialog);

	gtk_widget_ref (textview);

	return textview;
}

static GtkWidget *
create_anjuta_info_dialog_with_treeview (gint width,
					 gint height)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *action_area;
	GtkWidget *scrolledwindow;
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkWidget *close_button;

	if (height < 250)
		height = 250;

	if (width < 400)
		width = 400;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), _("Information"));

	vbox = GTK_DIALOG (dialog)->vbox;
	gtk_widget_show (vbox);

	action_area = GTK_DIALOG (dialog)->action_area;
	gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);
	gtk_widget_show (action_area);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolledwindow);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));

	treeview = gtk_tree_view_new_with_model (model);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);
	gtk_widget_show (treeview);

	g_object_unref (G_OBJECT (model));

	close_button = gtk_button_new_from_stock ("gtk-close");
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), close_button, GTK_RESPONSE_CLOSE);
	GTK_WIDGET_SET_FLAGS (close_button, GTK_CAN_DEFAULT);
	gtk_widget_show (close_button);

	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, TRUE, FALSE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 250);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "infoless", "Anjuta");

	g_signal_connect (dialog, "delete_event", G_CALLBACK (gtk_widget_hide), NULL);
	gtk_widget_show (dialog);

	gtk_widget_ref (treeview);

	return treeview;
}

gboolean
anjuta_info_show_file (const gchar *path,
		       gint         width,
		       gint         height)
{
	FILE *f;

	g_return_val_if_fail (path != NULL, FALSE);

	if (!g_file_exists (path))
		return FALSE;

	if ((f = fopen (path, "r")) == NULL)
		return FALSE;

	if (!anjuta_info_show_filestream (f, width, height)) {
		int errno_bak = errno;

		fclose (f);

		errno = errno_bak;

		return FALSE;
	}

	return (fclose (f) != 0) ? FALSE : TRUE;
}

gboolean
anjuta_info_show_command (const gchar *command_line,
			  gint         width,
			  gint         height)
{
	GtkWidget *textview;
	GError *err = NULL;
	gchar *std_output = NULL;
	gboolean ret;

	g_return_val_if_fail (command_line != NULL, FALSE);

	/* Note: we could use popen like anjuta_info_show_file, but g_spawn is safier */

	if (!g_spawn_command_line_sync (command_line, &std_output, NULL, NULL, &err)) {
		g_warning (err->message);
		g_error_free (err);

		return FALSE;
	}

	if (!g_utf8_validate (std_output, strlen (std_output), NULL))
		g_warning ("Invalid UTF-8 data encountered reading output of command '%s'", command_line);

	ret = anjuta_info_show_string (std_output, width, height);

	g_free (std_output);

	return ret;
}

gboolean
anjuta_info_show_string (const gchar *s,
			 gint         width,
			 gint         height)
{
	GtkWidget *textview;
	GtkTextBuffer *buffer;

	g_return_val_if_fail (s != NULL, FALSE);

	textview = create_anjuta_info_dialog_with_textview (width, height);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_set_text (buffer, s, strlen (s));

	gtk_widget_unref (textview);

	return TRUE;
}

#define ANJUTA_INFO_TEXTVIEW_BUFSIZE 1024

gboolean
anjuta_info_show_filestream (FILE *f,
			     gint  width,
			     gint  height)
{
	GtkWidget *textview;
	GtkTextBuffer *buffer;
	gchar buf[ANJUTA_INFO_TEXTVIEW_BUFSIZE];
	gboolean ret;

	g_return_val_if_fail (f != NULL, FALSE);

	textview = create_anjuta_info_dialog_with_textview (width, height);
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

	gtk_widget_unref (textview);

	return errno ? FALSE : TRUE;
}

gboolean
anjuta_info_show_fd (int  file_descriptor,
		     gint width,
		     gint height)
{
	FILE *f;
	gboolean ret;

	if ((f = fdopen (file_descriptor, "r")) == NULL)
		return FALSE;

	if (!anjuta_info_show_filestream (f, width, height)) {
		int errno_bak = errno;

		fclose (f);

		errno = errno_bak;

		return FALSE;
	}

	return (fclose (f) != 0) ? FALSE : TRUE;
}

void
anjuta_info_show_list (GList* list,
		       gint   width,
		       gint   height)
{
	GtkWidget *treeview;
	GtkTreeModel *model;

	g_return_if_fail (list != NULL);

	treeview = create_anjuta_info_dialog_with_treeview (width, height);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

	for (; list; list = g_list_next (list)) {
		GtkTreeIter iter;
		gchar *tmp;

		tmp = remove_white_spaces (list->data);

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, tmp, -1);

		g_free (tmp);
	}
	
	gtk_widget_unref (treeview);
}

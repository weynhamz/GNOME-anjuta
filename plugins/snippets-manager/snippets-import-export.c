/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-import-export.c
    Copyright (C) Dragos Dena 2010

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/


#include "snippets-import-export.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-utils.h>

static void
add_native_snippets_at_path (SnippetsDB *snippets_db,
                             const gchar *path)
{

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	if (path == NULL)
		return;
	/* TODO - import the snippets */
}

static void
add_other_snippets_at_path (SnippetsDB *snippets_db,
                            const gchar *path)
{

	/* TODO - import the snippets. Should try every format known until it matches */
}

void 
snippets_manager_import_snippets (SnippetsDB *snippets_db,
                                  AnjutaShell *anjuta_shell)
{
	GtkWidget *file_chooser = NULL;
	GtkFileFilter *native_filter = NULL, *other_filter = NULL, *cur_filter = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	file_chooser = gtk_file_chooser_dialog_new (_("Import Snippets"),
	                                            GTK_WINDOW (anjuta_shell),
	                                            GTK_FILE_CHOOSER_ACTION_OPEN,
	                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	                                            NULL);

	/* Set up the filters */
	native_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (native_filter, "Native format");
	gtk_file_filter_add_pattern (native_filter, "*.anjuta-snippets");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), native_filter);

	other_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (other_filter, "Other formats");
	gtk_file_filter_add_pattern (other_filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), other_filter);

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (file_chooser)),
		      *path = anjuta_util_get_local_path_from_uri (uri);
		
		cur_filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (file_chooser));
		if (!g_strcmp0 ("Native format", gtk_file_filter_get_name (cur_filter)))
			add_native_snippets_at_path (snippets_db, path);
		else
			add_other_snippets_at_path (snippets_db, path);

		g_free (path);
		g_free (uri);
	}
	
	gtk_widget_destroy (file_chooser);
}

void snippets_manager_export_snippets (SnippetsDB *snippets_db,
                                       AnjutaShell *anjuta_shell)
{

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

}
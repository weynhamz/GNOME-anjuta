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
#include "snippets-xml-parser.h"
#include "snippets-group.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-utils.h>


static void
add_or_update_snippet (SnippetsDB *snippets_db,
                       AnjutaSnippet *snippet,
                       const gchar *group_name)
{
	const gchar *trigger = NULL;
	GList *iter = NULL, *languages = NULL;
	gchar *cur_lang = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));

	/* Get the trigger-key and the languages for which the snippet is supported */
	trigger = snippet_get_trigger_key (snippet);
	languages = (GList *)snippet_get_languages (snippet);

	/* Check if each (trigger, language) tuple exists in the database and update it 
	   if yes, or add it if not. */
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_lang = (gchar *)iter->data;

		/* If there is already an entry for (trigger, cur_lang) we remove it so
		   we can update it. */
		if (snippets_db_get_snippet (snippets_db, trigger, cur_lang))
			snippets_db_remove_snippet (snippets_db, trigger, cur_lang, FALSE);

	}

	snippets_db_add_snippet (snippets_db, snippet, group_name);
}

static void
add_group_list_to_database (SnippetsDB *snippets_db,
                            GList *snippets_groups)
{
	GList *iter = NULL, *iter2 = NULL, *snippets = NULL;
	AnjutaSnippetsGroup *cur_group = NULL;
	const gchar *cur_group_name = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	if (snippets_groups == NULL)
		return;

	for (iter = g_list_first (snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		if (!ANJUTA_IS_SNIPPETS_GROUP (iter->data))
			continue;
		cur_group = ANJUTA_SNIPPETS_GROUP (iter->data);
		cur_group_name = snippets_group_get_name (cur_group);

		/* If there isn't a snippets group with the same name we just add it */		
		if (!snippets_db_has_snippets_group_name (snippets_db, cur_group_name))
		{
			snippets_db_add_snippets_group (snippets_db, cur_group, TRUE);
			continue;
		}

		/* If there is already a group with the same name, we add the snippets inside
		   the group */
		snippets = snippets_group_get_snippets_list (cur_group);

		for (iter2 = g_list_first (snippets); iter2 != NULL; iter2 = g_list_next (iter2))
		{
			if (!ANJUTA_IS_SNIPPET (iter2->data))
				continue;

			add_or_update_snippet (snippets_db, 
			                       ANJUTA_SNIPPET (iter2->data), 
			                       cur_group_name);
		}
	}
}

static void
add_native_snippets_at_path (SnippetsDB *snippets_db,
                             const gchar *path)
{
	GList *snippets_groups = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	if (path == NULL)
		return;

	/* Parse the snippets file */
	snippets_groups = snippets_manager_parse_snippets_xml_file (path, NATIVE_FORMAT);

	/* Add the snippets groups from the file to the database */
	add_group_list_to_database (snippets_db, snippets_groups);

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

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/***************************************************************************
 *            search_preferences.c
 *
 *  Sat Nov 27 18:26:50 2004
 *  Copyright  2004  Jean-Noel GUIHENEUF
 *  guiheneuf.jean-noel@wanadoo.fr
 ****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include "search-replace_backend.h"
#include "search-replace.h"
#include "search_preferences.h"


enum {PREF_DEFAULT_COLUMN, PREF_NAME_COLUMN, PREF_ACTIVE_COLUMN};

#define SEARCH_PREF_PATH "/apps/anjuta/search_preferences"
#define BASIC _("Basic Search")


static GSList *list_pref = NULL;
static gchar *default_pref = NULL;

static SearchReplace *sr = NULL;



static GSList *search_preferences_find_setting(gchar *name);
static gboolean on_search_preferences_clear_default_foreach (GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer data);
static gboolean on_search_preferences_setting_inactive (GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer data);
static void search_preferences_update_entry(gchar *name);
static void search_preferences_read_setting(gchar *name);
static void search_preferences_setting_by_default(void);
static void search_preferences_save_setting(gchar *name);
static void search_preferences_save_search_pref(gchar *name);
static GtkTreeModel* search_preferences_get_model(void);
static void search_preferences_add_treeview(gchar *name);
static void search_preferences_remove_setting(gchar *name);
static void search_preferences_activate_default(gchar *name);
static void search_preferences_active_selection_row(GtkTreeView *view);
static void on_search_preferences_colorize_setting (GtkTreeViewColumn *tree_column,
					GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data);
static void on_search_preferences_row_activated (GtkTreeView *view,
	                                 GtkTreePath *tree_path,
	                                 GtkTreeViewColumn *view_column,
	                                 GtkCellRenderer *renderer);		
static void on_search_preferences_treeview_enable_toggle (GtkCellRendererToggle *cell,
							 gchar			*path_str, gpointer		 data);
static gboolean search_preferences_name_is_valid(gchar *name);
void search_preferences_initialize_setting_treeview(GtkWidget *dialog);							 




static GSList* 
search_preferences_find_setting(gchar *name)
{
	GSList *list;

	for(list = list_pref; list; list=g_slist_next(list))
	{
		if (g_ascii_strcasecmp(name, list->data) == 0)
			return list;
	}	
	return NULL;
}


static gboolean
on_search_preferences_clear_default_foreach (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data)
{
	gchar *t_name;
	gint active;
	
	gtk_tree_model_get (model, iter, PREF_NAME_COLUMN, &t_name,
	                                 PREF_ACTIVE_COLUMN, &active, -1);
	if ((data != NULL) && (g_ascii_strcasecmp(t_name, (gchar*) data) == 0))
		gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
		                    PREF_DEFAULT_COLUMN, TRUE,
		                    PREF_ACTIVE_COLUMN, TRUE, -1);
	else
		gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
		                   PREF_DEFAULT_COLUMN, FALSE,
		                  -1);
	return FALSE;
}


static gboolean
on_search_preferences_setting_inactive (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data)
{
	gtk_tree_store_set (GTK_TREE_STORE (model), iter, PREF_ACTIVE_COLUMN, FALSE, -1);
	return FALSE;
}


static void
search_preferences_update_entry(gchar *name)
{
	GtkWidget *pref_entry;
	
	pref_entry = sr_get_gladewidget(SETTING_PREF_ENTRY)->widget;
	gtk_entry_set_text(GTK_ENTRY(pref_entry), name);
}


static void
search_preferences_read_setting(gchar *name)
 {
	GConfClient *client;
	
	client = gconf_client_get_default();

	sr->search.expr.regex = gconf_client_get_bool(client, 
	                        gconf_concat_dir_and_key(name, "regex"), NULL);
	sr->search.expr.greedy = gconf_client_get_bool(client, 
	                         gconf_concat_dir_and_key(name, "greedy"), NULL);
	sr->search.expr.match_case = gconf_client_get_bool(client, 
	                              gconf_concat_dir_and_key(name, "match_case"), NULL);
	sr->search.expr.whole_word = gconf_client_get_bool(client, 
	                             gconf_concat_dir_and_key(name, "whole_word"), NULL);
	sr->search.expr.whole_line = gconf_client_get_bool(client, 
	                             gconf_concat_dir_and_key(name, "whole_line"), NULL);
	sr->search.expr.word_start = gconf_client_get_bool(client, 
	                             gconf_concat_dir_and_key(name, "word_start"), NULL);
	sr->search.expr.no_limit = gconf_client_get_bool(client, 
	                           gconf_concat_dir_and_key(name, "no_limit"), NULL);
	sr->search.expr.actions_max = gconf_client_get_int(client, 
	                              gconf_concat_dir_and_key(name, "actions_max"), NULL);
	sr->search.range.type = gconf_client_get_int(client, 
	                        gconf_concat_dir_and_key(name, "type"), NULL);
	sr->search.range.direction = gconf_client_get_int(client, 
	                             gconf_concat_dir_and_key(name, "direction"), NULL);	
	sr->search.action = gconf_client_get_int(client, 
	                             gconf_concat_dir_and_key(name, "action"), NULL);
	sr->search.basic_search = gconf_client_get_bool(client, 
	                             gconf_concat_dir_and_key(name, "basic_search"), NULL);
								 
	search_update_dialog();
 }

 
static void
search_preferences_setting_by_default(void)
 {
	sr->search.expr.regex =FALSE;
	sr->search.expr.greedy = FALSE;
	sr->search.expr.match_case = FALSE;
	sr->search.expr.whole_word = FALSE;
	sr->search.expr.whole_line = FALSE;
	sr->search.expr.word_start = FALSE;
	sr->search.expr.no_limit =TRUE;
	sr->search.expr.actions_max = 200;
	sr->search.range.type = SR_BUFFER;
	sr->search.range.direction = SD_FORWARD;	
	sr->search.action = SA_SELECT;
	sr->search.basic_search = TRUE;
	
	search_update_dialog();
 }
 
 
static void
search_preferences_save_setting(gchar *name)
{
	GConfClient *client;
	gchar *path;
	
	search_replace_populate();

	client = gconf_client_get_default();
	path =  gconf_concat_dir_and_key(SEARCH_PREF_PATH, name);
	
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "regex"), 
	                      sr->search.expr.regex, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "greedy"),
	                      sr->search.expr.greedy, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "match_case"),
                          sr->search.expr.match_case, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "whole_word"), 
	                      sr->search.expr.whole_word, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "whole_line"), 
	                      sr->search.expr.whole_line, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "word_start"),
	                      sr->search.expr.word_start, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "no_limit"), 
	                      sr->search.expr.no_limit, NULL);
	gconf_client_set_int(client, gconf_concat_dir_and_key(path, "actions_max"), 
	                     sr->search.expr.actions_max, NULL);
	gconf_client_set_int(client, gconf_concat_dir_and_key(path, "type"), 
	                     sr->search.range.type, NULL);
	gconf_client_set_int(client, gconf_concat_dir_and_key(path, "direction"), 
	                     sr->search.range.direction, NULL);	
	gconf_client_set_int(client, gconf_concat_dir_and_key(path, "action"), 
	                     sr->search.action, NULL);
	gconf_client_set_bool(client, gconf_concat_dir_and_key(path, "basic_search"), 
	                     sr->search.basic_search, NULL);
}

 
static void
search_preferences_save_search_pref(gchar *name)
{
	GConfClient *client;
	gchar *path;
	
	client = gconf_client_get_default();
	gconf_client_set_list(client, gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
	                      "list_pref"), GCONF_VALUE_STRING, list_pref, NULL);

	path =  gconf_concat_dir_and_key(SEARCH_PREF_PATH, name);
	gconf_client_add_dir(client, path, GCONF_CLIENT_PRELOAD_NONE, NULL);
	
	search_preferences_save_setting(name);
}


static GtkTreeModel*
search_preferences_get_model(void)
{
	GtkTreeView *view;
	
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	return gtk_tree_view_get_model(view);
}


static void
search_preferences_add_treeview(gchar *name)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = search_preferences_get_model();
	gtk_tree_model_foreach (model, on_search_preferences_setting_inactive, NULL);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							PREF_DEFAULT_COLUMN, FALSE,
							PREF_NAME_COLUMN, name,
						    PREF_ACTIVE_COLUMN, TRUE, -1);
}


static void
search_preferences_remove_setting(gchar *name)
{
	GConfClient *client;
	
	client = gconf_client_get_default();
	gconf_client_set_list(client, gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
		                  "list_pref"), GCONF_VALUE_STRING, list_pref, NULL);
// FIXME : Remove Setting Directory
	gconf_client_remove_dir(client, gconf_concat_dir_and_key(SEARCH_PREF_PATH, name), NULL);
}



static void
search_preferences_activate_default(gchar *name)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	model = gtk_tree_view_get_model (view);
	
	gtk_tree_model_foreach (model, on_search_preferences_clear_default_foreach, name);
	
}

static void
search_preferences_active_selection_row(GtkTreeView *view)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name;
	
	selection = gtk_tree_view_get_selection (view);

	if (gtk_tree_selection_get_selected (selection, &model, &iter) == TRUE)
	{
		gtk_tree_model_foreach (model, on_search_preferences_setting_inactive, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							PREF_ACTIVE_COLUMN, TRUE,
							-1);
		gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name, -1);
		search_preferences_update_entry(name);
		
		if (g_ascii_strcasecmp (name, BASIC))
			search_preferences_read_setting(gconf_concat_dir_and_key(
				                            SEARCH_PREF_PATH, name));
		else
			search_preferences_setting_by_default();
	}	
}


static void
on_search_preferences_colorize_setting (GtkTreeViewColumn *tree_column,
					GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data)
{
	gboolean active;
	static const gchar *colors[] = {"black", "red"};
	GValue gvalue = {0, };
	
	gtk_tree_model_get (tree_model, iter, PREF_ACTIVE_COLUMN, &active, -1);
	g_value_init (&gvalue, G_TYPE_STRING);
	g_value_set_static_string (&gvalue, colors[active? 1 : 0]);
	g_object_set_property (G_OBJECT (cell), "foreground", &gvalue);
}


static void
on_search_preferences_row_activated (GtkTreeView *view,
	                                 GtkTreePath *tree_path,
	                                 GtkTreeViewColumn *view_column,
	                                 GtkCellRenderer *renderer)
{
	search_preferences_active_selection_row(view);	
}


static void
on_search_preferences_treeview_enable_toggle (GtkCellRendererToggle *cell,
							 gchar			*path_str,
							 gpointer		 data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gboolean state;
	gchar *name;
	GConfClient *client;

	
	path = gtk_tree_path_new_from_string (path_str);
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name,
	                                  PREF_DEFAULT_COLUMN, &state, -1);
	
		client = gconf_client_get_default();
	if (state)
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, PREF_DEFAULT_COLUMN, FALSE, -1);
		gconf_client_set_string(client, gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
		                        "search_pref_default"), "", NULL);
	}
	else
	{
		gconf_client_set_string(client, gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
		                        "search_pref_default"), name, NULL);
		
		gtk_tree_model_foreach (model, on_search_preferences_clear_default_foreach, NULL);	
	
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, PREF_DEFAULT_COLUMN, TRUE, -1);		
	}
}


static gboolean
search_preferences_name_is_valid(gchar *name)
{
	gint i;
	
	for(i=0; i<strlen(name); i++)
		if ((!g_ascii_isalnum(name[i]) && name[i]!='_'))
			return FALSE;
	return TRUE;
}


void
on_setting_pref_add_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *pref_entry;
	gchar *name;

	pref_entry = sr_get_gladewidget(SETTING_PREF_ENTRY)->widget;
	name = g_strstrip(gtk_editable_get_chars(GTK_EDITABLE(pref_entry), 0, -1));
	
	if (!name || strlen(name) < 1)
		return;

	if (!search_preferences_name_is_valid(name))
		return;
	if (search_preferences_find_setting(name))
		return;
		
	if (g_ascii_strcasecmp (name, BASIC))
	{
		list_pref = g_slist_append(list_pref, g_strdup(name));
		
		search_preferences_save_search_pref(name);
		
		search_preferences_add_treeview(name);
	}
	g_free(name);
}


void
on_setting_pref_remove_clicked(GtkButton *button, gpointer user_data)
{
	GtkTreeView *view;
	GtkTreeStore *store;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *name;
	GConfClient *client;
	
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	store = GTK_TREE_STORE (gtk_tree_view_get_model(view));
	selection = gtk_tree_view_get_selection (view);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name, -1);
		if (g_ascii_strcasecmp (name, BASIC))
		{
			gchar *path;

			client = gconf_client_get_default();
			path = gconf_client_get_string(client, gconf_concat_dir_and_key(SEARCH_PREF_PATH,
							      "search_pref_default"), NULL);
			gtk_tree_store_remove(store, &iter);
		
			list_pref = g_slist_remove(list_pref, search_preferences_find_setting(name)->data);

			search_preferences_remove_setting(name);

			if (!g_ascii_strcasecmp (name, path))
			{
				gconf_client_set_string(client, gconf_concat_dir_and_key(
					SEARCH_PREF_PATH, "search_pref_default"), "", NULL);
			}
			g_free(path);
			search_preferences_update_entry("");
		}
	}
}


void
on_setting_pref_modify_clicked(GtkButton *button, gpointer user_data)
{
	GtkTreeView *view;
	GtkTreeStore *store;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *name;

	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	store = GTK_TREE_STORE (gtk_tree_view_get_model(view));
	selection = gtk_tree_view_get_selection (view);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name, -1);
		if (g_ascii_strcasecmp (name, BASIC))
		{
			search_preferences_save_setting(name);
			search_preferences_update_entry("");
		}
	}		
}


void
search_preferences_initialize_setting_treeview(GtkWidget *dialog)
{
	GtkTreeView *view;
	GtkTreeStore *store;	
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
		
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	store = gtk_tree_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
					     GTK_SELECTION_SINGLE);

	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Default"),
														renderer,
								   						"active",
														PREF_DEFAULT_COLUMN,
														NULL);
	g_signal_connect (renderer, "toggled",
						  G_CALLBACK (on_search_preferences_treeview_enable_toggle), NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (view, column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
														renderer, 
														"text", 
														PREF_NAME_COLUMN, 
														NULL);
	g_signal_connect (view, "row-activated",
						  G_CALLBACK (on_search_preferences_row_activated), renderer);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
					on_search_preferences_colorize_setting, NULL, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (view, column);
}

void
search_preferences_init(void)
{
	GConfClient *client;
	GSList *list;
	GtkTreeModel *model;
	GtkTreeIter iter;

	sr = create_search_replace_instance(NULL);

	search_preferences_add_treeview(BASIC);
	
	client = gconf_client_get_default();
	gconf_client_add_dir(client, SEARCH_PREF_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	
	list_pref = gconf_client_get_list(client,gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
	                                  "list_pref"), GCONF_VALUE_STRING, NULL);
		
	for (list = list_pref; list != NULL; list = g_slist_next(list))
		search_preferences_add_treeview(list->data);
	
	default_pref = gconf_client_get_string(client,gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
	                                       "search_pref_default"), NULL);

	model = search_preferences_get_model();
	gtk_tree_model_foreach (model, on_search_preferences_setting_inactive, NULL);
	
	if (default_pref && (*default_pref != '\0') && g_ascii_strcasecmp (default_pref, BASIC))	
		search_preferences_read_setting(gconf_concat_dir_and_key(SEARCH_PREF_PATH, 
		                                default_pref));
	else
	{
		gtk_tree_model_get_iter_first(model, &iter);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						    PREF_ACTIVE_COLUMN, TRUE, -1);
		search_preferences_setting_by_default();
	}
	
	search_preferences_activate_default(default_pref);
	g_free(default_pref);
}

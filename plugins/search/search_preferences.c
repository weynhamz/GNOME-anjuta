/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/***************************************************************************
 *            search_preferences.c
 *
 *  Wed Jan 14 05:22:01 2004
 *  Copyright  2004  Jean-Noel Guiheneuf
 *  jnoel@lotuscompounds.com
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gnome.h>

// #include "anjuta.h"

#include "search-replace_backend.h"
#include "search-replace.h"
#include "search_preferences.h"


typedef struct _SearchSet
{
	gchar *name;
	gchar *search_str;
	gboolean regex;
	gboolean greedy;
	gboolean ignore_case;
	gboolean whole_word;
	gboolean whole_line;
	gboolean word_start;
	gboolean no_limit;
	gint actions_max;
	gint target;
	gint direction;
	gint action;
	gboolean basic_search;
} SearchSet;

typedef struct _SearchPrefSetting
{
	gint nb_ss;
	GList *setting;
	gchar *name_default;
} SearchPrefSetting;


enum {PREF_DEFAULT_COLUMN, PREF_NAME_COLUMN, PREF_ACTIVE_COLUMN};

#define MAX_SETTINGS 8

static SearchSet *search_preferences_find_setting(gchar *name);
static gboolean
on_search_preferences_clear_default_foreach (GtkTreeModel *model, GtkTreePath *path,
								             GtkTreeIter *iter, gpointer data);
static void
on_search_preferences_treeview_enable_toggle (GtkCellRendererToggle *cell,
							                  gchar *path_str,
							                  gpointer data);
static void
on_search_preferences_colorize_setting (GtkTreeViewColumn *tree_column,
					GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data);
static gboolean
on_search_preferences_setting_inactive (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data);
static void
search_preferences_update_sr(SearchSet *ss);
static void
search_preferences_update_search(gchar *name);
static SearchSet *
search_preferences_update_setting(SearchSet *ss);
static SearchSet *
search_preferences_add_setting(gchar *name);
static void
search_preferences_update_treeview(void);
static void
search_preferences_remove_setting(gchar *name);
static void
search_preferences_update_item(gchar *name);
static void
search_preferences_save_setting (FILE *stream, gint nb, GList *list);
static SearchSet *
search_preferences_load_setting (PropsID props, gint nb);
static void
search_preferences_clear_setting(void);
static void
search_preferences_activate_default(gchar *name);



static SearchPrefSetting *sps = NULL;

static SearchReplace *sr = NULL;
static GtkWidget *sr_dialog = NULL;


static SearchSet *
search_preferences_find_setting(gchar *name)
{
	GList *list;
	SearchSet *ss;
	
	for(list = sps->setting; list; list=g_list_next(list))
	{
		ss = list->data;
		if (g_ascii_strcasecmp(name, ss->name) == 0)
			return ss;
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
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
		                    PREF_DEFAULT_COLUMN, TRUE,
		                    PREF_ACTIVE_COLUMN, TRUE, -1);
	}
	else
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
		                   PREF_DEFAULT_COLUMN, FALSE, -1);

	}
	return FALSE;
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
	
	path = gtk_tree_path_new_from_string (path_str);
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name,
	                                  PREF_DEFAULT_COLUMN, &state, -1);
	if (state)
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, PREF_DEFAULT_COLUMN, FALSE, -1);
	else
	{
		if (sps->name_default != NULL)
			g_free(sps->name_default);
		sps->name_default = g_strdup(name);
		
		gtk_tree_model_foreach (model, on_search_preferences_clear_default_foreach, NULL);	
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, PREF_DEFAULT_COLUMN, TRUE, -1);		
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

static gboolean
on_search_preferences_setting_inactive (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data)
{
	gtk_tree_store_set (GTK_TREE_STORE (model), iter, PREF_ACTIVE_COLUMN, FALSE, -1);
	return FALSE;
}

static void
search_preferences_update_sr(SearchSet *ss)
{
	if (ss->search_str)
		sr->search.expr.search_str = g_strdup(ss->search_str);
	sr->search.expr.regex =	ss->regex;
	sr->search.expr.greedy = ss->greedy;
	sr->search.expr.ignore_case = ss->ignore_case;
	sr->search.expr.whole_word = ss->whole_word;
	sr->search.expr.whole_line = ss->whole_line;
	sr->search.expr.word_start = ss->word_start;
	sr->search.expr.no_limit = ss->no_limit;
	sr->search.expr.actions_max = ss->actions_max;
	sr->search.range.type = ss->target;
	sr->search.range.direction = ss->direction;
	sr->search.action = ss->action;
//	ss->basic_search = 0;
}

static void
search_preferences_update_search(gchar *name)
{
	SearchSet *ss;
	
	if ((ss =search_preferences_find_setting(name)) != NULL)
		search_preferences_update_sr(ss);
}


static SearchSet *
search_preferences_update_setting(SearchSet *ss)
{
	ss->search_str = sr->search.expr.search_str;
	ss->regex = sr->search.expr.regex;
	ss->greedy = sr->search.expr.greedy;
	ss->ignore_case = sr->search.expr.ignore_case;
	ss->whole_word = sr->search.expr.whole_word;
	ss->whole_line = sr->search.expr.whole_line;
	ss->word_start = sr->search.expr.word_start;
	ss->no_limit = sr->search.expr.no_limit;
	ss->actions_max = sr->search.expr.actions_max;
	ss->target = sr->search.range.type;
	ss->direction = sr->search.range.direction;
	ss->action = sr->search.action;
//	ss->basic_search = 0;
	
	return ss;
}


static SearchSet *
search_preferences_add_setting(gchar *name)
{
	SearchSet *ss;
	
	ss = g_new0(SearchSet, 1);
	ss->name = g_strdup(name);
	(sps->nb_ss)++;
	
	ss = search_preferences_update_setting(ss);
	return ss;
}

static void
search_preferences_add_treeview(gchar *name)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	model = gtk_tree_view_get_model(view);
	gtk_tree_model_foreach (model, on_search_preferences_setting_inactive, NULL);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							PREF_DEFAULT_COLUMN, FALSE,
							PREF_NAME_COLUMN, name,
						    PREF_ACTIVE_COLUMN, TRUE,
							-1);
}

static void
search_preferences_update_treeview(void)
{
	gint nb;
	GList *list;
	SearchSet *ss;
	
	list = sps->setting;
	for (nb=0; nb<sps->nb_ss; nb++)
	{
		ss = list->data;
		search_preferences_add_treeview(ss->name);
		list = g_list_next(list);
	}	
}


static void
search_preferences_remove_setting(gchar *name)
{
	SearchSet *ss;
	
	if ((ss =search_preferences_find_setting(name)) != NULL)
	{
		g_free(ss->name);
		g_free(ss);
		sps->setting = g_list_remove(sps->setting, ss);			
		(sps->nb_ss)--;
	}		
}	
	

static void
search_preferences_update_item(gchar *name)
{
	SearchSet *ss;
	
	if ((ss =search_preferences_find_setting(name)) != NULL)
		search_preferences_update_setting(ss);	
}


static void
search_preferences_save_setting (FILE *stream, gint nb, GList *list)
{
	SearchSet *ss;
	
	ss = list->data;
	fprintf (stream, "search.setting.name%d=%s\n", nb, ss->name);
	if (ss->search_str)
		fprintf (stream, "search.setting.search_str%d=%s\n", nb, ss->search_str);
	fprintf (stream, "search.setting.regex%d=%d\n", nb, ss->regex);
	fprintf (stream, "search.setting.greedy%d=%d\n", nb, ss->greedy);
	fprintf (stream, "search.setting.ignore_case%d=%d\n", nb, ss->ignore_case);
	fprintf (stream, "search.setting.whole_word%d=%d\n", nb, ss->whole_word);
	fprintf (stream, "search.setting.whole_line%d=%d\n", nb, ss->whole_line);
	fprintf (stream, "search.setting.word_start%d=%d\n", nb, ss->word_start);
	fprintf (stream, "search.setting.no_limit%d=%d\n", nb, ss->no_limit);
	fprintf (stream, "search.setting.actions_max%d=%d\n", nb, ss->actions_max);
	fprintf (stream, "search.setting.target%d=%d\n", nb, ss->target);
	fprintf (stream, "search.setting.direction%d=%d\n", nb, ss->direction);
	fprintf (stream, "search.setting.action%d=%d\n", nb, ss->action);
	fprintf (stream, "search.setting.basic_search%d=%d\n", nb, ss->basic_search);
}
	

static SearchSet *
search_preferences_load_setting (PropsID props, gint nb)
{	
	SearchSet *ss;
	
	ss = g_new0(SearchSet, 1);
	
	ss->name = prop_get(props, g_strdup_printf("search.setting.name%d",nb));
	ss->search_str = prop_get(props, g_strdup_printf("search.setting.search_str%d",nb));	
	ss->regex = prop_get_int(props, g_strdup_printf("search.setting.regex%d",nb), 0);
	ss->greedy = prop_get_int(props, g_strdup_printf("search.setting.greedy%d",nb), 0);
	ss->ignore_case = prop_get_int(props, g_strdup_printf("search.setting.ignore_case%d",nb), 0);
	ss->whole_word = prop_get_int(props, g_strdup_printf("search.setting.whole_word%d",nb), 0);
	ss->whole_line = prop_get_int(props, g_strdup_printf("search.setting.whole_line%d",nb), 0);
	ss->word_start = prop_get_int(props, g_strdup_printf("search.setting.word_start%d",nb), 0);
	ss->no_limit = prop_get_int(props, g_strdup_printf("search.setting.no_limit%d",nb), 0);
	ss->actions_max = prop_get_int(props, g_strdup_printf("search.setting.actions_max%d",nb), 0);
	ss->target = prop_get_int(props, g_strdup_printf("search.setting.target%d",nb), 0);
	ss->direction = prop_get_int(props, g_strdup_printf("search.setting.direction%d",nb), 0);
	ss->action = prop_get_int(props, g_strdup_printf("search.setting.action%d",nb), 0);
	ss->basic_search = prop_get_int(props, g_strdup_printf("search.setting.basic_search %d",nb), 0);

	return ss;
}

static void
search_preferences_clear_setting(void)
{
	GList *list;
	SearchSet *ss;
	
	for(list = sps->setting; list; list=g_list_next(list))
	{
		ss = list->data;
		g_free(ss->name);
		g_free(ss->search_str);
		g_free(ss);
		sps->setting = g_list_remove(sps->setting, list->data);
	}	
	g_free(sps->name_default);
	sps->setting = NULL;
	sps->nb_ss = 0;
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

gboolean
search_preferences_save_yourself (FILE *stream)
{
	gint nb;
	GList *list;

	fprintf (stream, "search.setting.nb=%d\n", sps->nb_ss);
	fprintf (stream, "search.setting.default=%s\n", sps->name_default);
	list = sps->setting;
	for (nb=0; nb<sps->nb_ss; nb++)
	{
		search_preferences_save_setting (stream, nb, list);
		list = g_list_next(list);
	}	
	return TRUE;
}


gboolean
search_preferences_load_yourself (PropsID props)
{
	gint nb;
	
	if (sps == NULL)
		sps = g_new0(SearchPrefSetting, 1);
	else
		search_preferences_clear_setting();
	sr = create_search_replace_instance(NULL);
	
	sps->nb_ss = prop_get_int(props, "search.setting.nb", 0);
	sps->name_default = prop_get(props, "search.setting.default");
	
	for (nb=0; nb<sps->nb_ss; nb++)
	{
		sps->setting = g_list_append(sps->setting, search_preferences_load_setting (props, nb));
	}
	
	return TRUE;
}



void
on_setting_pref_add_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *pref_entry;
	gchar *name;
	GtkWidget *dialog;
	
	pref_entry = sr_get_gladewidget(SETTING_PREF_ENTRY)->widget;
	name = g_strstrip(gtk_editable_get_chars(GTK_EDITABLE(pref_entry), 0, -1));
	
	if (!name || strlen(name) < 1)
		return;
	if (sps->nb_ss >= MAX_SETTINGS)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW (sr_dialog),
				GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		        _("The maximum number of settings has been reached."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}	
	if (search_preferences_find_setting(name))
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW (sr_dialog),
				GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		        _("This name is already used."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}	
	search_replace_populate();	
	
	sps->setting = g_list_append(sps->setting, search_preferences_add_setting(name));
	
	search_preferences_add_treeview(name);
	
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
	
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	store = GTK_TREE_STORE (gtk_tree_view_get_model(view));
	selection = gtk_tree_view_get_selection (view);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name, -1);
		gtk_tree_store_remove(store, &iter);
		search_preferences_remove_setting(name);
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
		search_replace_populate();	
		gtk_tree_model_get (model, &iter, PREF_NAME_COLUMN, &name, -1);
		search_preferences_update_item(name);
		
		gtk_tree_model_foreach (model, on_search_preferences_setting_inactive, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							PREF_ACTIVE_COLUMN, TRUE,
							-1);
	}
		
}


void
on_setting_pref_activate_clicked(GtkButton *button, gpointer user_data)
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
		search_preferences_update_search(name);
		search_update_dialog();
		
		gtk_tree_model_foreach (model, on_search_preferences_setting_inactive, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							PREF_ACTIVE_COLUMN, TRUE,
							-1);
	}
}

void
search_preferences_initialize_setting_treeview(GtkWidget *dialog)
{
	GtkTreeView *view;
	GtkTreeStore *store;
	
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	if (sps == NULL)
		sps = g_new0(SearchPrefSetting, 1);
	sr = create_search_replace_instance(NULL);
	sr_dialog = dialog;
	
	view = GTK_TREE_VIEW (sr_get_gladewidget(SETTING_PREF_TREEVIEW)->widget);
	store = gtk_tree_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
					     GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (store));

	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("Default",
														renderer,
								   						"active",
														PREF_DEFAULT_COLUMN,
														NULL);
	g_signal_connect (renderer, "toggled",
						  G_CALLBACK (on_search_preferences_treeview_enable_toggle), NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (view, column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name",
														renderer, 
														"text", 
														PREF_NAME_COLUMN, 
														NULL);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
					on_search_preferences_colorize_setting, NULL, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (view, column);
		
	search_preferences_update_treeview();
	gtk_tree_model_foreach (GTK_TREE_MODEL (store), on_search_preferences_setting_inactive, NULL);
	if (sps->name_default)
		search_preferences_activate_default(sps->name_default);
	
	search_preferences_update_search(sps->name_default);
}

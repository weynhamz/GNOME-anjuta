/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-tags-pane.h"

enum
{
	COL_SELECTED,
	COL_NAME
};

static GtkTargetEntry drag_targets[] =
{
	{
		"STRING",
		0,
		0
	}
};

struct _GitTagsPanePriv
{
	GtkBuilder *builder;
};


G_DEFINE_TYPE (GitTagsPane, git_tags_pane, GIT_TYPE_PANE);

static void
on_tag_list_command_started (AnjutaCommand *command, GitTagsPane *self)
{
	GtkTreeView *tags_view;
	GtkListStore *tags_list_model;

	tags_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                   "tags_view"));
	tags_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                          "tags_list_model"));

	gtk_tree_view_set_model (tags_view, NULL);
	gtk_list_store_clear (tags_list_model);
}

static void
on_tag_list_command_finished (AnjutaCommand *command, guint return_code, 
                              GitTagsPane *self)
{
	GtkTreeView *tags_view;
	GtkTreeModel *tags_list_model;

	tags_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                   "tags_view"));
	tags_list_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                          "tags_list_model"));

	gtk_tree_view_set_model (tags_view, tags_list_model);
}

static void
on_tag_list_command_data_arrived (AnjutaCommand *command, 
                                  GtkListStore *tags_list_model)
{
	GQueue *output;
	GtkTreeIter iter;
	gchar *name;

	output = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));

	while (g_queue_peek_head (output))
	{
		name = g_queue_pop_head (output);
		gtk_list_store_append (tags_list_model, &iter);

		gtk_list_store_set (tags_list_model, &iter, 
							COL_SELECTED, FALSE,
		                    COL_NAME, name, 
		                    -1);

		g_free (name);
	}
	
}

static void
on_selected_renderer_toggled (GtkCellRendererToggle *renderer, 
                              gchar *path, GtkTreeModel *tags_list_model)
{
	GtkTreeIter iter;
	gboolean selected;

	gtk_tree_model_get_iter_from_string (tags_list_model, &iter, path);
	gtk_tree_model_get (tags_list_model, &iter, 
	                    COL_SELECTED, &selected, 
	                    -1);

	selected = !selected;

	gtk_list_store_set (GTK_LIST_STORE (tags_list_model), &iter, 0, selected,
	                    -1);
}

static void
on_tags_list_view_drag_data_get (GtkWidget *tags_list_view, 
                                 GdkDragContext *drag_context,
                                 GtkSelectionData *data,
                                 guint info, guint time,
                                 gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *tags_list_model;
	gchar *name;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tags_list_view));

	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		gtk_tree_selection_get_selected (selection, &tags_list_model, 
		                                 &iter);

		gtk_tree_model_get (tags_list_model, &iter, COL_NAME, &name, -1);

		gtk_selection_data_set_text (data, name, -1);

		g_free (name);
	}
}

static gboolean
get_selected_tags (GtkTreeModel *tags_list_model, gchar *path, 
                   GtkTreeIter *iter, GList **list)
{
	gboolean selected;
	gchar *name;

	gtk_tree_model_get (tags_list_model, iter, COL_SELECTED, &selected, -1);

	if (selected)
	{
		gtk_tree_model_get (tags_list_model, iter, COL_NAME, &name, -1);

		*list = g_list_append (*list, name);
	}

	return FALSE;
}

static void
git_tags_pane_init (GitTagsPane *self)
{
	gchar *objects[] = {"tags_pane",
						"tags_list_model",
						NULL};
	GError *error = NULL;
	GtkTreeView *tags_view;
	GtkListStore *tags_list_model;
	GtkCellRenderer *selected_renderer;
	
	self->priv = g_new0 (GitTagsPanePriv, 1);
	self->priv->builder = gtk_builder_new ();
	

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	tags_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                       "tags_view"));
	tags_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                          "tags_list_model"));
	selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                               "selected_renderer"));

	/* DND */
	gtk_tree_view_enable_model_drag_source (tags_view,
	                                        GDK_BUTTON1_MASK,
	                                        drag_targets,
	                                        G_N_ELEMENTS (drag_targets),
	                                        GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (tags_view), "drag-data-get",
	                  G_CALLBACK (on_tags_list_view_drag_data_get),
	                  NULL);

	g_signal_connect (G_OBJECT (selected_renderer), "toggled",
	                  G_CALLBACK (on_selected_renderer_toggled),
	                  tags_list_model);
}

static void
git_tags_pane_finalize (GObject *object)
{
	GitTagsPane *self;

	self = GIT_TAGS_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_tags_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_tags_pane_get_widget (AnjutaDockPane *pane)
{
	GitTagsPane *self;

	self = GIT_TAGS_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "tags_pane"));
}

static void
git_tags_pane_class_init (GitTagsPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_tags_pane_finalize;
	pane_class->get_widget = git_tags_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_tags_pane_new (Git *plugin)
{
	GitTagsPane *self;
	GtkListStore *tags_list_model;
	
	self = g_object_new (GIT_TYPE_TAGS_PANE, "plugin", plugin, NULL);
	tags_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                          "tags_list_model"));

	g_signal_connect (G_OBJECT (plugin->tag_list_command), "command-started",
	                  G_CALLBACK (on_tag_list_command_started),
	                  self);

	g_signal_connect (G_OBJECT (plugin->tag_list_command), "command-finished",
	                  G_CALLBACK (on_tag_list_command_finished),
	                  self);

	g_signal_connect (G_OBJECT (plugin->tag_list_command), "data-arrived",
	                  G_CALLBACK (on_tag_list_command_data_arrived),
	                  tags_list_model);

	return ANJUTA_DOCK_PANE (self);
}

GList *
git_tags_pane_get_selected_tags (GitTagsPane *self)
{
	GtkTreeModel *tags_list_model;
	GList *list;

	tags_list_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                          "tags_list_model"));
	list = NULL;

	gtk_tree_model_foreach (tags_list_model, 
	                        (GtkTreeModelForeachFunc) get_selected_tags, &list);

	return list;
}
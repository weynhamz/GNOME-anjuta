/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf_project-tree.c
 *
 * Copyright (C) 2000  JP Rosevear
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: JP Rosevear
 *          Gustavo Giráldez <gustavo.giraldez@gmx.net>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-macros.h>
#include <gio/gio.h>
#include <string.h>

#include "gbf-tree-data.h"
#include "gbf-project-model.h"
#include "gbf-project-view.h"

#define ICON_SIZE 16

struct _GbfProjectViewPrivate {	

};

enum {
	URI_ACTIVATED,
	TARGET_SELECTED,
	GROUP_SELECTED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static void gbf_project_view_class_init    (GbfProjectViewClass *klass);
static void gbf_project_view_instance_init (GbfProjectView      *tree);
static void destroy                        (GtkObject           *object);

static void set_pixbuf                     (GtkTreeViewColumn   *tree_column,
					    GtkCellRenderer     *cell,
					    GtkTreeModel        *model,
					    GtkTreeIter         *iter,
					    gpointer             data);
static void set_text                       (GtkTreeViewColumn   *tree_column,
					    GtkCellRenderer     *cell,
					    GtkTreeModel        *model,
					    GtkTreeIter         *iter,
					    gpointer             data);


GNOME_CLASS_BOILERPLATE(GbfProjectView, gbf_project_view,
			GtkTreeView, GTK_TYPE_TREE_VIEW);


static void
row_activated (GtkTreeView       *tree_view,
	       GtkTreePath       *path,
	       GtkTreeViewColumn *column)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GbfTreeData *data;
	
	model = gtk_tree_view_get_model (tree_view);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    -1);

	if (data->uri) {
		g_signal_emit (G_OBJECT (tree_view), 
			       signals [URI_ACTIVATED], 0,
			       data->uri);
	}

	if (data->type == GBF_TREE_NODE_TARGET) {
		g_signal_emit (G_OBJECT (tree_view),
			       signals [TARGET_SELECTED], 0,
			       data->id);
	}
	
	if (data->type == GBF_TREE_NODE_GROUP) {
		g_signal_emit (G_OBJECT (tree_view),
			       signals [GROUP_SELECTED], 0,
			       data->id);
	}
	
	gbf_tree_data_free (data);
}

static void
destroy (GtkObject *object)
{
	GbfProjectView *tree;
	GbfProjectViewPrivate *priv;
	
	tree = GBF_PROJECT_VIEW (object);
	priv = tree->priv;
	
	if (priv) {
		g_free (priv);
		tree->priv = NULL;
	}
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static GdkPixbuf*
get_icon (const gchar* uri)
{
	const gchar** icon_names;
	GtkIconInfo* icon_info;
	GIcon* icon;
	GdkPixbuf* pixbuf = NULL;
	GFile* file;
	GFileInfo* file_info;
	GError *error = NULL;
	
	file = g_file_new_for_uri (uri);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_ICON,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       &error);

	if (!file_info)
	{
		g_warning (G_STRLOC ": Unable to query information for URI: %s: %s", uri, error->message);
		g_clear_error (&error);
		return NULL;
	}
	
	icon = g_file_info_get_icon(file_info);
	g_object_get (icon, "names", &icon_names, NULL);
	icon_info = gtk_icon_theme_choose_icon (gtk_icon_theme_get_default(),
						icon_names,
						ICON_SIZE,
						GTK_ICON_LOOKUP_GENERIC_FALLBACK);
	pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
	gtk_icon_info_free(icon_info);
	
	return pixbuf;
}

static void
set_pixbuf (GtkTreeViewColumn *tree_column,
	    GtkCellRenderer   *cell,
	    GtkTreeModel      *model,
	    GtkTreeIter       *iter,
	    gpointer           user_data)
{
	//GbfProjectView *view = GBF_PROJECT_VIEW (user_data);
	GbfTreeData *data = NULL;
	GdkPixbuf *pixbuf = NULL;
	
	gtk_tree_model_get (model, iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	g_return_if_fail (data != NULL);
	
	switch (data->type) {
		case GBF_TREE_NODE_TARGET_SOURCE:
		{
			pixbuf = get_icon (data->uri);
			break;
		}
		case GBF_TREE_NODE_GROUP:
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
							   GTK_STOCK_DIRECTORY,
							   ICON_SIZE,
							   GTK_ICON_LOOKUP_GENERIC_FALLBACK,
							   NULL);
			break;
		case GBF_TREE_NODE_TARGET: 
		{
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
							   GTK_STOCK_CONVERT,
							   ICON_SIZE,
							   GTK_ICON_LOOKUP_GENERIC_FALLBACK,
							   NULL);
			break;
		}
		default:
			pixbuf = NULL;
	}

	g_object_set (GTK_CELL_RENDERER (cell), "pixbuf", pixbuf, NULL);
	if (pixbuf)
		g_object_unref (pixbuf);

	gbf_tree_data_free (data);
}

static void
set_text (GtkTreeViewColumn *tree_column,
	  GtkCellRenderer   *cell,
	  GtkTreeModel      *model,
	  GtkTreeIter       *iter,
	  gpointer           user_data)
{
	GbfTreeData *data;
  
	gtk_tree_model_get (model, iter, 0, &data, -1);
	g_object_set (GTK_CELL_RENDERER (cell), "text", 
		      data->name, NULL);
	gbf_tree_data_free (data);
}

static gboolean
search_equal_func (GtkTreeModel *model, gint column,
		   const gchar *key, GtkTreeIter *iter,
		   gpointer user_data)
{
	GbfTreeData *data;
	gboolean ret = TRUE;
		     
	gtk_tree_model_get (model, iter, 0, &data, -1);
	if (strncmp (data->name, key, strlen (key)) == 0)
	    ret = FALSE;
	gbf_tree_data_free (data);
	return ret;
}

static gint
expose_event (GtkWidget *widget, GdkEventExpose *ev)
{
	GtkTreeModel *model;
	GtkTreeView *tree_view;
	gint event_handled = FALSE;
	
	event_handled = GNOME_CALL_PARENT_WITH_DEFAULT (
		GTK_WIDGET_CLASS, expose_event, (widget, ev), event_handled);

	tree_view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (tree_view);
	if (ev->window == gtk_tree_view_get_bin_window (tree_view) &&
	    model && GBF_IS_PROJECT_MODEL (model)) {
		GtkTreePath *root;
		GdkRectangle rect;
		
		/* paint an horizontal ruler to separate the project
		 * tree from the target shortcuts */
		
		root = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (model));
		if (root) {
			gtk_tree_view_get_background_area (
				tree_view, root, gtk_tree_view_get_column (tree_view, 0), &rect);
			gtk_paint_hline (widget->style,
					 ev->window,
					 GTK_WIDGET_STATE (widget),
					 &ev->area,
					 widget,
					 "",
					 rect.x, rect.x + rect.width,
					 rect.y);
			gtk_tree_path_free (root);
		}
	}
	
	return event_handled;
}

static void 
gbf_project_view_class_init (GbfProjectViewClass *klass)
{
	GObjectClass     *g_object_class;
	GtkObjectClass   *object_class;
	GtkWidgetClass   *widget_class;
	GtkTreeViewClass *tree_view_class;

	g_object_class = G_OBJECT_CLASS (klass);
	object_class = (GtkObjectClass *) klass;
	widget_class = GTK_WIDGET_CLASS (klass);
	tree_view_class = GTK_TREE_VIEW_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->destroy = destroy;
	widget_class->expose_event = expose_event;
	tree_view_class->row_activated = row_activated;

	signals [URI_ACTIVATED] = 
		g_signal_new ("uri_activated",
			      GBF_TYPE_PROJECT_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GbfProjectViewClass,
					       uri_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals [TARGET_SELECTED] = 
		g_signal_new ("target_selected",
			      GBF_TYPE_PROJECT_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GbfProjectViewClass,
					       target_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	signals [GROUP_SELECTED] = 
		g_signal_new ("group_selected",
			      GBF_TYPE_PROJECT_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GbfProjectViewClass,
					       group_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void 
gbf_project_view_instance_init (GbfProjectView *tree)
{
	GbfProjectViewPrivate *priv;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	static GtkTargetEntry row_targets[] = {
		{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, 0 }
	};

	priv = g_new0 (GbfProjectViewPrivate, 1);
	tree->priv = priv;
    
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (tree), 0);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (tree),
					     search_equal_func,
					     NULL, NULL);
	
	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (tree),
						GDK_BUTTON1_MASK,
						row_targets,
						G_N_ELEMENTS (row_targets),
						GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (tree),
					      row_targets,
					      G_N_ELEMENTS (row_targets),
					      GDK_ACTION_MOVE);
      
	/* set renderer for files column. */        
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, set_pixbuf, tree, NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, set_text, tree, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

GtkWidget *
gbf_project_view_new (void)
{
	return GTK_WIDGET (g_object_new (GBF_TYPE_PROJECT_VIEW, NULL));
}

GbfTreeData *
gbf_project_view_find_selected (GbfProjectView *view, GbfTreeNodeType type)
{
	GbfTreeData *data = NULL;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), NULL);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
				    -1);
		/* walk up the hierarchy searching for a node of the given type */
		while (NULL != data && data->type != type) {
			gbf_tree_data_free (data);
			data = NULL;

			if (!gtk_tree_model_iter_parent (model, &iter2, &iter))
				break;
			
			gtk_tree_model_get (model, &iter2,
					    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
					    -1);
			iter = iter2;
		}
	}

	return data;
}


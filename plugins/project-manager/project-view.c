/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* project-tree.c
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
#include <gio/gio.h>
#include <string.h>

#include "tree-data.h"
#include "project-model.h"
#include "project-view.h"
#include "project-marshal.h"

#define ICON_SIZE 16

enum {
	URI_ACTIVATED,
	TARGET_SELECTED,
	GROUP_SELECTED,
	NODE_LOADED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static void gbf_project_view_class_init    (GbfProjectViewClass *klass);
static void gbf_project_view_init          (GbfProjectView      *tree);
static void destroy                        (GtkWidget           *object);

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


G_DEFINE_TYPE(GbfProjectView, gbf_project_view, GTK_TYPE_TREE_VIEW);

static void
row_activated (GtkTreeView       *tree_view,
	       GtkTreePath       *path,
	       GtkTreeViewColumn *column)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GbfTreeData *data;
	gchar *uri;
	
	model = gtk_tree_view_get_model (tree_view);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    -1);

	uri = gbf_tree_data_get_uri (data);
	if (uri != NULL) {
		g_signal_emit (G_OBJECT (tree_view), 
			       signals [URI_ACTIVATED], 0,
			       uri);
	}

	if (data->type == GBF_TREE_NODE_TARGET) {
		g_signal_emit (G_OBJECT (tree_view),
			       signals [TARGET_SELECTED], 0,
			       uri);
	}
	
	if (data->type == GBF_TREE_NODE_GROUP) {
		g_signal_emit (G_OBJECT (tree_view),
			       signals [GROUP_SELECTED], 0,
			       uri);
	}
	g_free (uri);
}

static void
dispose (GObject *object)
{
	GbfProjectView *view;
	
	view = GBF_PROJECT_VIEW (object);
	if (view->model)
	{
		g_object_unref (G_OBJECT (view->model));
		view->model = NULL;
	}
	
	G_OBJECT_CLASS (gbf_project_view_parent_class)->dispose (object);
}

static void
destroy (GtkWidget *object)
{
	GbfProjectView *tree;
	
	tree = GBF_PROJECT_VIEW (object);
	
	if (GTK_WIDGET_CLASS (gbf_project_view_parent_class)->destroy)
		(* GTK_WIDGET_CLASS (gbf_project_view_parent_class)->destroy) (object);
}

static GdkPixbuf*
get_icon (GFile *file)
{
	const gchar** icon_names;
	GtkIconInfo* icon_info;
	GIcon* icon;
	GdkPixbuf* pixbuf = NULL;
	GFileInfo* file_info;
	GError *error = NULL;
	
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_ICON,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       &error);

	if (!file_info)
	{
		gchar *name = g_file_get_parse_name (file);
		
		g_warning (G_STRLOC ": Unable to query information for URI: %s: %s", name, error->message);
		g_free (name);
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
	g_object_unref (file_info);
	
	return pixbuf;
}

static void
set_pixbuf (GtkTreeViewColumn *tree_column,
	    GtkCellRenderer   *cell,
	    GtkTreeModel      *model,
	    GtkTreeIter       *iter,
	    gpointer           user_data)
{
	GbfTreeData *data = NULL;
	GdkPixbuf *pixbuf = NULL;
	
	gtk_tree_model_get (model, iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	g_return_if_fail (data != NULL);
	/* FIXME: segmentation fault with shortcut when corresponding
	 * data is removed before the shortcut, so data = NULL.
	 * Perhaps we can add a GtkTreeReference to the shortcut
	 * node to remove the shortcut when the node is destroyed */

	if ((data->type == GBF_TREE_NODE_SHORTCUT) && (data->shortcut != NULL))
 	{
		data = data->shortcut;
	}
	switch (data->type) {
		case GBF_TREE_NODE_SOURCE:
		{
			pixbuf = get_icon (data->source);
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
		case GBF_TREE_NODE_MODULE: 
		{
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
							   GTK_STOCK_DND_MULTIPLE,
							   ICON_SIZE,
							   GTK_ICON_LOOKUP_GENERIC_FALLBACK,
							   NULL);
			break;
		}
		case GBF_TREE_NODE_PACKAGE: 
		{
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
							   GTK_STOCK_DND,
							   ICON_SIZE,
							   GTK_ICON_LOOKUP_GENERIC_FALLBACK,
							   NULL);
			break;
		}
		default:
			/* Can reach this if type = GBF_TREE_NODE_SHORTCUT. It 
			 * means a shortcut with the original data removed */
			pixbuf = NULL;
	}

	g_object_set (GTK_CELL_RENDERER (cell), "pixbuf", pixbuf, NULL);
	if (pixbuf)
		g_object_unref (pixbuf);
}

static void
set_text (GtkTreeViewColumn *tree_column,
	  GtkCellRenderer   *cell,
	  GtkTreeModel      *model,
	  GtkTreeIter       *iter,
	  gpointer           user_data)
{
	GbfTreeData *data;
  
	gtk_tree_model_get (model, iter, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	/* data can be NULL just after gtk_tree_store_insert before
	calling gtk_tree_store_set */ 
	g_object_set (GTK_CELL_RENDERER (cell), "text", 
		      data == NULL ? "" : data->name, NULL);
}

static gboolean
search_equal_func (GtkTreeModel *model, gint column,
		   const gchar *key, GtkTreeIter *iter,
		   gpointer user_data)
{
	GbfTreeData *data;
	gboolean ret = TRUE;
		     
	gtk_tree_model_get (model, iter, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	if (strncmp (data->name, key, strlen (key)) == 0)
	    ret = FALSE;
	return ret;
}

static gboolean
draw (GtkWidget *widget, cairo_t *cr)
{
	GtkTreeModel *model;
	GtkTreeView *tree_view;
	gint event_handled = FALSE;

	if (GTK_WIDGET_CLASS (gbf_project_view_parent_class)->draw != NULL)
		GTK_WIDGET_CLASS (gbf_project_view_parent_class)->draw (widget, cr);

	tree_view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (tree_view);
	if (gtk_cairo_should_draw_window (cr, gtk_tree_view_get_bin_window (tree_view)) &&
	    model && GBF_IS_PROJECT_MODEL (model)) {
		GtkTreePath *root;
		GdkRectangle rect;
		
		/* paint an horizontal ruler to separate the project
		 * tree from the target shortcuts */
		
		root = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (model));
		if (root) {
			gtk_tree_view_get_background_area (
				tree_view, root, gtk_tree_view_get_column (tree_view, 0), &rect);
			gtk_paint_hline (gtk_widget_get_style (widget),
					 cr,
					 gtk_widget_get_state (widget),
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
	GtkWidgetClass   *widget_class;
	GtkTreeViewClass *tree_view_class;

	g_object_class = G_OBJECT_CLASS (klass);
	widget_class = GTK_WIDGET_CLASS (klass);
	tree_view_class = GTK_TREE_VIEW_CLASS (klass);

	g_object_class->dispose = dispose;
	widget_class->destroy = destroy;
	widget_class->draw = draw;
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
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals [GROUP_SELECTED] = 
		g_signal_new ("group_selected",
			      GBF_TYPE_PROJECT_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GbfProjectViewClass,
					       group_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[NODE_LOADED] =
		g_signal_new ("node-loaded",
				GBF_TYPE_PROJECT_VIEW,
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (GbfProjectViewClass,
				                 node_loaded),
				NULL, NULL,
				pm_cclosure_marshal_VOID__POINTER_BOOLEAN_BOXED,
				G_TYPE_NONE, 3,
				G_TYPE_POINTER,
				G_TYPE_BOOLEAN,
				G_TYPE_ERROR);
	
}

static void 
gbf_project_view_init (GbfProjectView *tree)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	static GtkTargetEntry row_targets[] = {
		{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, 0 }
	};
    
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

	/* Create model */
	tree->model = gbf_project_model_new (NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (tree->model));
}

GtkWidget *
gbf_project_view_new (void)
{
	return GTK_WIDGET (g_object_new (GBF_TYPE_PROJECT_VIEW, NULL));
}

AnjutaProjectNode *
gbf_project_view_find_selected (GbfProjectView *view, AnjutaProjectNodeType type)
{
	GtkTreeIter iter;
	AnjutaProjectNode *node = NULL;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), NULL);

	if (gbf_project_view_get_first_selected (view, &iter) != NULL)
	{
		GtkTreeModel *model;
		
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
		if (GTK_IS_TREE_MODEL_FILTER (model))
		{
			GtkTreeIter filter_iter = iter;
			gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &iter, &filter_iter);
			model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
		}			
		node = gbf_project_model_get_node (GBF_PROJECT_MODEL (model), &iter);
		
		/* walk up the hierarchy searching for a node of the given type */
		while ((node != NULL) && (type != ANJUTA_PROJECT_UNKNOWN) && (anjuta_project_node_get_node_type (node) != type))
		{
			node = anjuta_project_node_parent (node);
		}
	}
	
	return node;
}

GbfTreeData *
gbf_project_view_get_first_selected (GbfProjectView *view, GtkTreeIter* selected)
{
	GtkTreeSelection *selection;
	GbfTreeData *data = NULL;
	GtkTreeModel *model;
	GList *list;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), NULL);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	list = gtk_tree_selection_get_selected_rows(selection, &model);
	if (list != NULL)
	{
		GtkTreeIter iter;
		
		if (gtk_tree_model_get_iter (model, &iter, list->data))
		{
			if (selected) *selected = iter;

			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    	-1);
		}
		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
	}

	return data;
}

static void
on_each_get_data (GtkTreeModel *model,
			GtkTreePath *path,
                        GtkTreeIter *iter,
                        gpointer user_data)
{
	GList **selected = (GList **)user_data;
	GbfTreeData *data;

	/*if (GTK_IS_TREE_MODEL_FILTER (model))
	{
		GtkTreeIter child_iter;
			
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, iter);
		*iter = child_iter;
	}*/
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);

	*selected = g_list_prepend (*selected, data);
}

GList *
gbf_project_view_get_all_selected (GbfProjectView *view)
{
	GtkTreeSelection *selection;
	GList *selected = NULL;

	g_return_val_if_fail (view != NULL, FALSE);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_selected_foreach (selection, on_each_get_data, &selected);

	return g_list_reverse (selected);
}

static void
on_each_get_iter (GtkTreeModel *model,
			GtkTreePath *path,
                        GtkTreeIter *iter,
                        gpointer user_data)
{
	GList **selected = (GList **)user_data;
	
	*selected = g_list_prepend (*selected, gtk_tree_iter_copy (iter));
}

GList *
gbf_project_view_get_all_selected_iter (GbfProjectView *view)
{
	GtkTreeSelection *selection;
	GList *selected = NULL;

	g_return_val_if_fail (view != NULL, FALSE);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_selected_foreach (selection, on_each_get_iter, &selected);

	return g_list_reverse (selected);
}

/* Shorcuts functions
 *---------------------------------------------------------------------------*/

GList *
gbf_project_view_get_shortcut_list (GbfProjectView *view)
{
	GList *list = NULL;
	GtkTreeModel* model;
	gboolean valid;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
	if (model != NULL)
	{
		for (valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &iter, NULL);
			valid == TRUE;
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter))
		{
			GbfTreeData *data;
			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
				GBF_PROJECT_MODEL_COLUMN_DATA, &data,
				-1);

			if ((data->type == GBF_TREE_NODE_SHORTCUT) && (data->shortcut != NULL))
			{
				gchar *uri;
				GtkTreePath *path;
				gboolean expand;

				uri = gbf_tree_data_get_uri (data);
				path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
				expand = gtk_tree_view_row_expanded (GTK_TREE_VIEW (view), path);
				gtk_tree_path_free (path);

				if (uri != NULL)
				{
					list = g_list_prepend (list, g_strconcat (expand ? "E " : "C ", uri, NULL));
				}
			}
		}
		list = g_list_reverse (list);
	}
	
	return list;
}

void
gbf_project_view_remove_all_shortcut (GbfProjectView* view)
{
	GtkTreeModel* model;
	gboolean valid;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

	/* Remove all current shortcuts */
	for (valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &iter, NULL);
		valid == TRUE;)
	{
		GbfTreeData *data;
			
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
	    	-1);

		if (data->type == GBF_TREE_NODE_SHORTCUT)
		{
			valid = gbf_project_model_remove (GBF_PROJECT_MODEL (model), &iter);
		}
		else
		{
			/* No more shortcut */
			break;
		}
	}
}

void
gbf_project_view_set_shortcut_list (GbfProjectView *view, GList *shortcuts)
{
	GtkTreeModel* model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

	if (shortcuts != NULL)
	{
		gboolean valid;
		GtkTreeIter iter;
		
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &iter, NULL);

		if (valid)
		{
			GList *node;
			
			for (node = g_list_first (shortcuts); node != NULL; node = g_list_next (node))
			{
				GFile *file;
				GtkTreeIter shortcut;
				gboolean expand = FALSE;
				gchar *uri = (gchar *)node->data;

				if (strncmp (uri, "E ", 2) == 0)
				{
					expand = TRUE;
					uri += 2;
				}
				else if (strncmp (uri, "C ", 2) == 0)
				{
					expand = FALSE;
					uri += 2;
				}
				file = g_file_new_for_uri (uri);

				if (gbf_project_model_find_file  (GBF_PROJECT_MODEL (model), &shortcut, NULL, ANJUTA_PROJECT_UNKNOWN, file))
				{
					GbfTreeData *data;

					gtk_tree_model_get (GTK_TREE_MODEL (model), &shortcut, 
			    			GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    				-1);

					/* Avoid duplicated shortcuts */
					if (data->type != GBF_TREE_NODE_SHORTCUT)
					{
						gbf_project_model_add_shortcut (GBF_PROJECT_MODEL (model),
					    		&shortcut,
					    		&iter,
					    		data);

						if (expand)
						{
							GtkTreePath *path;

							path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &shortcut);
							gtk_tree_view_expand_row (GTK_TREE_VIEW (view), path, FALSE);
							gtk_tree_path_free (path);
						}
						/* Mark the shortcut as used */
						*uri = 'U';
					}
				}
				g_object_unref (file);
			}
		}
	}
	
	return;
}

AnjutaProjectNode *
gbf_project_view_get_node_from_iter (GbfProjectView *view, GtkTreeIter *iter)
{
	return gbf_project_model_get_node (view->model, iter);
}

AnjutaProjectNode *
gbf_project_view_get_node_from_file (GbfProjectView *view, AnjutaProjectNodeType type, GFile *file)
{
	GtkTreeIter iter;
	AnjutaProjectNode *node = NULL;
	
	if (gbf_project_model_find_file (view->model, &iter, NULL, type, file))
	{
		
		node = gbf_project_model_get_node (view->model, &iter);
	}

	return NULL;
}

gboolean
gbf_project_view_remove_data (GbfProjectView *view, GbfTreeData *data, GError **error)
{
	GtkTreeIter iter;
	
	if (gbf_project_model_find_tree_data (view->model, &iter, data))
	{
		gbf_project_model_remove (view->model, &iter);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void
on_node_loaded (AnjutaPmProject *sender, AnjutaProjectNode *node, gboolean complete, GError *error, GbfProjectView *view)
{
	if (error != NULL)
	{
		g_warning ("unable to load node");
		g_signal_emit (G_OBJECT (view), NODE_LOADED, 0, NULL, complete, error);
	}
	else
	{
		GtkTreeIter iter;
		gboolean found;
			
		found = gbf_project_model_find_node (view->model, &iter, NULL, node);
		gbf_project_model_update_tree (view->model, node, found ? &iter : NULL);
		if (!found)
		{
			gtk_tree_model_get_iter_first (GTK_TREE_MODEL (view->model), &iter);
		}

		g_signal_emit (G_OBJECT (view), signals[NODE_LOADED], 0, &iter, complete, NULL);
	}
}


void 
gbf_project_view_set_project (GbfProjectView *view, AnjutaPmProject *project)
{
	g_signal_connect (project, "loaded", G_CALLBACK (on_node_loaded), view);

	gbf_project_model_set_project (view->model, project);
}

gboolean
gbf_project_view_find_file (GbfProjectView *view, GtkTreeIter* iter, GFile *file, GbfTreeNodeType type)
{
	return gbf_project_model_find_file (view->model, iter, NULL, type, file);	
}

GbfProjectModel *
gbf_project_view_get_model (GbfProjectView *view)
{
	return view->model;
}

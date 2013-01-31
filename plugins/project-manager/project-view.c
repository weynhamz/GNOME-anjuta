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
#include "project-util.h"

#define ICON_SIZE 16

enum {
	NODE_SELECTED,
	NODE_LOADED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

/* Project model filter with drag and drop support
 *---------------------------------------------------------------------------*/

typedef struct {
  GtkTreeModelFilter parent;
} PmProjectModelFilter;

typedef struct {
  GtkTreeModelFilterClass parent_class;
} PmProjectModelFilterClass;

#define PM_TYPE_PROJECT_MODEL_FILTER	    (pm_project_model_filter_get_type ())
#define PM_PROJECT_MODEL_FILTER(obj)	    (G_TYPE_CHECK_INSTANCE_CAST ((obj), PM_TYPE_PROJECT_MODEL_FILTER, PmProjectModelFilter))


static void
pm_project_model_filter_class_init (PmProjectModelFilterClass *class)
{
}

static void
pm_project_model_filter_init (PmProjectModelFilter *model)
{
}

static gboolean
idrag_source_row_draggable (GtkTreeDragSource *drag_source, GtkTreePath *path)
{
	GtkTreeIter iter;
	GbfTreeData *data;
	gboolean retval = FALSE;

	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_source), &iter, path))
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (drag_source), &iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    -1);

	if (data->is_shortcut) {
		/* shortcuts can be moved */
		retval = TRUE;

	} else if (data->type == GBF_TREE_NODE_TARGET) {
		/* don't allow duplicate shortcuts */
		if (data->shortcut == NULL)
			retval = TRUE;
	}

	return retval;
}

static gboolean
idrag_source_drag_data_delete (GtkTreeDragSource *drag_source, GtkTreePath *path)
{
	return FALSE;
}

static gboolean
idrag_dest_drag_data_received (GtkTreeDragDest  *drag_dest,
		    GtkTreePath      *dest,
		    GtkSelectionData *selection_data)
{
	GtkTreeModel *src_model = NULL;
	GtkTreePath *src_path = NULL;
	GtkTreeModel *project_model;
	gboolean retval = FALSE;

	if (GTK_IS_TREE_MODEL_FILTER (drag_dest))
	{
		project_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (drag_dest));
	}
	else
	{
		project_model = GTK_TREE_MODEL (drag_dest);
	}
	g_return_val_if_fail (GBF_IS_PROJECT_MODEL (project_model), FALSE);

	if (gtk_tree_get_row_drag_data (selection_data,
					&src_model,
					&src_path) &&
	    src_model == GTK_TREE_MODEL (project_model)) {

		GtkTreeIter iter;
		GbfTreeData *data = NULL;

		if (gtk_tree_model_get_iter (src_model, &iter, src_path)) {
			gtk_tree_model_get (src_model, &iter,
					    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
					    -1);
			if (data != NULL)
			{
				GtkTreePath *child_path;

				child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (drag_dest), dest);
				if (data->type == GBF_TREE_NODE_SHORTCUT)
				{
					gbf_project_model_move_target_shortcut (GBF_PROJECT_MODEL (project_model),
					                                        &iter, data, child_path);
				}
				else
				{
					gbf_project_model_add_target_shortcut (GBF_PROJECT_MODEL (project_model),
						     	NULL, data, child_path, NULL);
				}
				gtk_tree_path_free (child_path);
				retval = TRUE;
			}
		}
	}

	if (src_path)
		gtk_tree_path_free (src_path);

	return retval;
}

static gboolean
idrag_dest_row_drop_possible (GtkTreeDragDest  *drag_dest,
		   GtkTreePath      *dest_path,
		   GtkSelectionData *selection_data)
{
	GtkTreeModel *src_model;
	GtkTreeModel *project_model;
	GtkTreePath *src_path;
	GtkTreeIter iter;
	gboolean retval = FALSE;

	//g_return_val_if_fail (GBF_IS_PROJECT_MODEL (drag_dest), FALSE);

	if (GTK_IS_TREE_MODEL_FILTER (drag_dest))
	{
		project_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (drag_dest));
	}
	else
	{
		project_model = GTK_TREE_MODEL (drag_dest);
	}

	if (!gtk_tree_get_row_drag_data (selection_data,
					 &src_model,
					 &src_path))
	{
		return FALSE;
	}


	if (gtk_tree_model_get_iter (src_model, &iter, src_path))
	{
		GbfTreeData *data = NULL;

		gtk_tree_model_get (src_model, &iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

		if (data != NULL)
		{
			/* can only drag to ourselves and only new toplevel nodes will
			 * be created */
			if (src_model == project_model &&
	    			gtk_tree_path_get_depth (dest_path) == 1)
			{
				if (data->type == GBF_TREE_NODE_SHORTCUT)
				{
					retval = TRUE;
				}
				else
				{
					GtkTreePath *root_path;
					GtkTreePath *child_path;

					root_path = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (project_model));
					child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (drag_dest), dest_path);
					retval = gtk_tree_path_compare (child_path, root_path) <= 0;
					gtk_tree_path_free (child_path);
					gtk_tree_path_free (root_path);
				}
			}
		}
	}
	gtk_tree_path_free (src_path);

	return retval;
}

static void
pm_project_model_filter_drag_source_iface_init (GtkTreeDragSourceIface *iface)
{
  iface->row_draggable = idrag_source_row_draggable;
  iface->drag_data_delete = idrag_source_drag_data_delete;
}

static void
pm_project_model_filter_drag_dest_iface_init (GtkTreeDragDestIface *iface)
{
  iface->drag_data_received = idrag_dest_drag_data_received;
  iface->row_drop_possible = idrag_dest_row_drop_possible;
}

static GType pm_project_model_filter_get_type (void);

G_DEFINE_TYPE_WITH_CODE (PmProjectModelFilter,
                         pm_project_model_filter,
                         GTK_TYPE_TREE_MODEL_FILTER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
                                                pm_project_model_filter_drag_source_iface_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST,
                                                pm_project_model_filter_drag_dest_iface_init));


static GtkTreeModel *
pm_project_model_filter_new (GtkTreeModel          *child_model,
                             GtkTreePath           *root)
{
  PmProjectModelFilter *model;

  model = g_object_new (PM_TYPE_PROJECT_MODEL_FILTER,
			"child-model", child_model,
			"virtual-root", root,
			NULL);

  return GTK_TREE_MODEL (model);
}





static void gbf_project_view_class_init    (GbfProjectViewClass *klass);
static void gbf_project_view_init          (GbfProjectView      *tree);
static void destroy                        (GtkWidget           *object);

static void set_pixbuf                     (GtkCellLayout   	*layout,
					    GtkCellRenderer     *cell,
					    GtkTreeModel        *model,
					    GtkTreeIter         *iter,
					    gpointer             user_data);
static void set_text                       (GtkCellLayout   	*layout,
					    GtkCellRenderer     *cell,
					    GtkTreeModel        *model,
					    GtkTreeIter         *iter,
					    gpointer             user_data);


G_DEFINE_TYPE(GbfProjectView, gbf_project_view, GTK_TYPE_TREE_VIEW);

static void
row_activated (GtkTreeView       *tree_view,
	       GtkTreePath       *path,
	       GtkTreeViewColumn *column)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GbfTreeData *data;
	AnjutaProjectNode *node;

	model = gtk_tree_view_get_model (tree_view);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    -1);

	node = gbf_tree_data_get_node (data);
	if (node)
	{
		switch (anjuta_project_node_get_node_type (node))
		{
		case ANJUTA_PROJECT_GROUP:
		case ANJUTA_PROJECT_ROOT:
		case ANJUTA_PROJECT_TARGET:
		case ANJUTA_PROJECT_MODULE:
		case ANJUTA_PROJECT_PACKAGE:
			if (!gtk_tree_view_row_expanded (tree_view, path))
			{
				gtk_tree_view_expand_row (tree_view, path, FALSE);
			}
			else
			{
				gtk_tree_view_collapse_row (tree_view, path);
			}
			break;
		default:
			g_signal_emit (G_OBJECT (tree_view),
			               signals [NODE_SELECTED], 0,
			               node);
			break;
		}
	}
}

static void on_node_loaded (AnjutaPmProject *sender, AnjutaProjectNode *node, gboolean complete, GError *error, GbfProjectView *view);

static void
dispose (GObject *object)
{
	GbfProjectView *view;

	view = GBF_PROJECT_VIEW (object);

	if (view->filter)
	{
		g_object_unref (G_OBJECT (view->filter));
		view->filter = NULL;
	}
	if (view->model)
	{
		AnjutaPmProject *old_project;

		old_project = gbf_project_model_get_project (view->model);
		if (old_project != NULL)
		{
			g_signal_handlers_disconnect_by_func (old_project, G_CALLBACK (on_node_loaded), view);
		}
		g_object_unref (G_OBJECT (view->model));
		view->model = NULL;
	}

	G_OBJECT_CLASS (gbf_project_view_parent_class)->dispose (object);
}

static void
destroy (GtkWidget *object)
{
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

	if (file_info != NULL)
	{
		icon = g_file_info_get_icon(file_info);
		g_object_get (icon, "names", &icon_names, NULL);
		icon_info = gtk_icon_theme_choose_icon (gtk_icon_theme_get_default(),
							icon_names,
							ICON_SIZE,
							GTK_ICON_LOOKUP_GENERIC_FALLBACK);
		if (icon_info != NULL)
		{
			pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
			gtk_icon_info_free(icon_info);
		}
		g_object_unref (file_info);
	}

	if (pixbuf == NULL)
	{
		pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
						   GTK_STOCK_MISSING_IMAGE,
						   ICON_SIZE,
						   GTK_ICON_LOOKUP_GENERIC_FALLBACK,
						   NULL);
	}

	return pixbuf;
}

static void
set_pixbuf (GtkCellLayout *layout,
	    GtkCellRenderer *cell,
	    GtkTreeModel *model,
	    GtkTreeIter *iter,
	    gpointer user_data)
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
		case GBF_TREE_NODE_ROOT:
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
							   GTK_STOCK_OPEN,
							   ICON_SIZE,
							   GTK_ICON_LOOKUP_GENERIC_FALLBACK,
							   NULL);
			break;
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
set_text (GtkCellLayout *layout,
	  GtkCellRenderer *cell,
	  GtkTreeModel *model,
	  GtkTreeIter *iter,
	  gpointer user_data)
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
	GtkTreeModel *view_model;
	GtkTreeModel *model = NULL;
	GtkTreeView *tree_view;
	gint event_handled = FALSE;

	if (GTK_WIDGET_CLASS (gbf_project_view_parent_class)->draw != NULL)
		GTK_WIDGET_CLASS (gbf_project_view_parent_class)->draw (widget, cr);

	tree_view = GTK_TREE_VIEW (widget);
	view_model = gtk_tree_view_get_model (tree_view);
	if (GTK_IS_TREE_MODEL_FILTER (view_model))
	{
		model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (view_model));
	}
	if (gtk_cairo_should_draw_window (cr, gtk_tree_view_get_bin_window (tree_view)) &&
	    model && GBF_IS_PROJECT_MODEL (model)) {
		GtkTreePath *root;
		GdkRectangle rect;

		/* paint an horizontal ruler to separate the project
		 * tree from the target shortcuts */

		root = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (model));
		if (root) {
			if (view_model != model)
			{
				/* Convert path */
				GtkTreePath *child_path;

				child_path = gtk_tree_model_filter_convert_child_path_to_path (GTK_TREE_MODEL_FILTER (view_model), root);
				gtk_tree_path_free (root);
				root = child_path;
			}
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

	signals [NODE_SELECTED] =
		g_signal_new ("node-selected",
			      GBF_TYPE_PROJECT_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GbfProjectViewClass,
					       node_selected),
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

static gboolean
is_project_node_visible (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data;

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);

	return (data != NULL) && (gbf_tree_data_get_node (data) != NULL);
}

static void
gbf_project_view_init (GbfProjectView *tree)
{
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
	pm_setup_project_renderer (GTK_CELL_LAYOUT (column));
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	/* Create model */
	tree->model = gbf_project_model_new (NULL);
	tree->filter = GTK_TREE_MODEL_FILTER (pm_project_model_filter_new (GTK_TREE_MODEL (tree->model), NULL));
	gtk_tree_model_filter_set_visible_func (tree->filter, is_project_node_visible, tree, NULL);

	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (tree->filter));
}

GtkWidget *
gbf_project_view_new (void)
{
	return GTK_WIDGET (g_object_new (GBF_TYPE_PROJECT_VIEW, NULL));
}

AnjutaProjectNode *
gbf_project_view_find_selected (GbfProjectView *view, AnjutaProjectNodeType type)
{
	AnjutaProjectNode *node = NULL;
	GbfTreeData *data;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), NULL);

	data = gbf_project_view_get_first_selected (view, NULL);
	if (data != NULL)
	{

		node = gbf_tree_data_get_node (data);

		/* walk up the hierarchy searching for a node of the given type */
		while ((node != NULL) && (type != ANJUTA_PROJECT_UNKNOWN) && (anjuta_project_node_get_node_type (node) != type))
		{
			node = anjuta_project_node_parent (node);
		}
	}

	return node;
}

AnjutaProjectNode *
gbf_project_view_find_selected_state (GtkTreeView *view,
                                      AnjutaProjectNodeState state)
{
	AnjutaProjectNode *node = NULL;
	GbfTreeData *data;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT_VIEW (view), NULL);

	data = gbf_project_view_get_first_selected (GBF_PROJECT_VIEW (view), NULL);
	if (data != NULL)
	{

		node = gbf_tree_data_get_node (data);

		/* walk up the hierarchy searching for a node of the given type */
		while ((node != NULL) && (state != 0) && !(anjuta_project_node_get_state (node) & state))
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
			if (selected)
			{
				if (GTK_IS_TREE_MODEL_FILTER (model))
				{
					GtkTreeIter child_iter;

					gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, &iter);
					*selected = child_iter;
				}
				else
				{
					*selected = iter;
				}
			}

			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    	-1);
		}
		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
	}

	return data;
}

void
gbf_project_view_set_cursor_to_iter (GbfProjectView *view,
                                     GtkTreeIter *selected)
{
	GtkTreeIter view_iter;

	if (pm_convert_project_iter_to_model_iter (GTK_TREE_MODEL (view->filter), &view_iter, selected))
	{
		GtkTreePath *path;

		path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->filter), &view_iter);
		if (path)
		{
			gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), path);

			gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL,
			                              TRUE, 0.5, 0.0);
			gtk_tree_path_free (path);
		}
	}
}


static void
on_each_get_data (GtkTreeModel *model,
			GtkTreePath *path,
                        GtkTreeIter *iter,
                        gpointer user_data)
{
	GList **selected = (GList **)user_data;
	GbfTreeData *data;

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

static void
gbf_project_view_update_shortcut (GbfProjectView *view, AnjutaProjectNode *parent)
{
	GtkTreeIter child;
	gboolean valid;

	/* Get all root node */
	valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (view->model), &child, NULL);

	while (valid)
	{
		GbfTreeData *data;
		AnjutaProjectNode* old_node = NULL;

		gtk_tree_model_get (GTK_TREE_MODEL (view->model), &child,
	  			 GBF_PROJECT_MODEL_COLUMN_DATA, &data,
	    			-1);

		/* Shortcuts are always at the beginning */
		if (data->type != GBF_TREE_NODE_SHORTCUT) break;

		old_node = gbf_tree_data_get_node (data);
		if (old_node == parent)
		{
			/* check children */
			gbf_project_view_update_tree (view, parent, &child);
		}
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (view->model), &child);
	}
}

static gint
compare_node_name (gconstpointer a, gconstpointer b)
{
	const AnjutaProjectNode *node = (const AnjutaProjectNode *)a;
	const gchar *name = (const gchar *)b;

	return g_strcmp0 (anjuta_project_node_get_name (node), name);
}

static GList *
list_visible_children (AnjutaProjectNode *parent)
{
 	AnjutaProjectNode *node;
	GList *list = NULL;

	for (node = anjuta_project_node_first_child (parent); node != NULL; node = anjuta_project_node_next_sibling (node))
	{
		if (anjuta_project_node_get_full_type (node) & ANJUTA_PROJECT_FRAME) continue;
		if (anjuta_project_node_get_node_type (node) != ANJUTA_PROJECT_OBJECT)
		{
			list = g_list_prepend (list, node);
		}
		else
		{
			/* object node are hidden, get their children instead */
			GList *children = list_visible_children (node);

			children = g_list_reverse (children);
			list = g_list_concat (children, list);
		}
	}
	list = g_list_reverse (list);

	return list;
}

void
gbf_project_view_update_tree (GbfProjectView *view, AnjutaProjectNode *parent, GtkTreeIter *iter)
{
	GtkTreeIter child;
	GList *node;
	GList *nodes;
	GbfTreeData *data = NULL;

	/* Get all new nodes */
	nodes = list_visible_children (parent);

	/* walk the tree nodes */
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (view->model), &child, iter))
	{
		gboolean valid = TRUE;

		while (valid) {
			data = NULL;
			AnjutaProjectNode* data_node = NULL;

			/* Look for current node */
			gtk_tree_model_get (GTK_TREE_MODEL (view->model), &child,
				GBF_PROJECT_MODEL_COLUMN_DATA, &data,
				-1);

			data_node = gbf_tree_data_get_node (data);

			/* Skip shortcuts */
			if (data->type == GBF_TREE_NODE_SHORTCUT)
			{
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (view->model), &child);
				continue;
			}

			if (data->type == GBF_TREE_NODE_UNKNOWN)
			{
				node = g_list_find_custom (nodes, data->name, compare_node_name);
				if (node != NULL)
				{
					GtkTreePath *path;
					GtkTreePath *child_path;
					GtkTreeModelFilter *filter;
					gboolean expanded;
					gboolean shortcut;

					expanded = data->expanded;
					shortcut = data->has_shortcut;
					data_node = (AnjutaProjectNode *)node->data;
					gbf_tree_data_free (data);
					data = gbf_tree_data_new_node (data_node);
					gtk_tree_store_set (GTK_TREE_STORE (view->model), &child,
								GBF_PROJECT_MODEL_COLUMN_DATA, data,
								-1);

					/* Node already exist, remove it from the list */
					nodes = g_list_delete_link (nodes, node);

					/* Update shortcut */
					gbf_project_view_update_shortcut (view, data_node);

					/* update recursively */
					gbf_project_view_update_tree (view, data_node, &child);

					if (shortcut)
					{
						gboolean expanded;
						GtkTreeIter iter;

						gbf_project_model_add_target_shortcut (view->model, &iter, data, NULL, &expanded);
						if (expanded)
						{
							/* Expand shortcut */
							filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
							path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->model), &iter);
							child_path = gtk_tree_model_filter_convert_child_path_to_path (filter, path);
							gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), child_path);
							gtk_tree_path_free (child_path);
							gtk_tree_path_free (path);
						}
					}
					data->expanded = expanded;
					if (expanded)
					{
						filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
						path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->model), &child);
						child_path = gtk_tree_model_filter_convert_child_path_to_path (filter, path);
						gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), child_path);
						expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (view), child_path);
						gtk_tree_path_free (child_path);
						gtk_tree_path_free (path);
					}
				}
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (view->model), &child);
			}
			else
			{
				node = g_list_find (nodes, data_node);
				if (node != NULL)
				{
					/* Node already exist, remove it from the list */
					nodes = g_list_delete_link (nodes, node);

					/* Update shortcut */
					gbf_project_view_update_shortcut (view, data_node);

					/* update recursively */
					gbf_project_view_update_tree (view, data_node, &child);

					valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (view->model), &child);
				}
				else
				{
					/* Node has been deleted */
					valid = gbf_project_model_remove (view->model, &child);
				}
			}
		}
	}

	/* add the remaining sources, targets and groups */
	for (node = nodes; node; node = node->next)
	{
		gbf_project_model_add_node (view->model, node->data, iter, 0);
	}

	g_list_free (nodes);

	/* Expand parent, needed if the parent hasn't any children when it was created */
	if (iter != NULL)
	{
		/* Check parent data */
		gtk_tree_model_get (GTK_TREE_MODEL (view->model), iter,
			GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			-1);
		if (data->expanded)
		{
			GtkTreePath *path;
			GtkTreePath *child_path;
			GtkTreeModelFilter *filter;

			filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->model), iter);
			child_path = gtk_tree_model_filter_convert_child_path_to_path (filter, path);
			gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), child_path);
			gtk_tree_path_free (child_path);
			gtk_tree_path_free (path);
			data->expanded = FALSE;
		}
	}
}

/* Shorcuts functions
 *---------------------------------------------------------------------------*/

void
gbf_project_view_sort_shortcuts (GbfProjectView *view)
{
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (view->model),
	                                      GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
	                                      GTK_SORT_ASCENDING);
	gbf_project_model_sort_shortcuts (view->model);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (view->model),
	                                      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
	                                      GTK_SORT_ASCENDING);

}

GList *
gbf_project_view_get_shortcut_list (GbfProjectView *view)
{
	GList *list = NULL;
	GtkTreeModel* model;
	gboolean valid;
	GtkTreeIter iter;

	model = GTK_TREE_MODEL (view->model);
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
				GtkTreeIter iter;

				if (gbf_project_model_find_tree_data (view->model, &iter, data->shortcut))
				{
					GString *str;
					GtkTreeIter child;

					str = g_string_new (NULL);
					do
					{
						GbfTreeData *data;

						child = iter;
						gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
								GBF_PROJECT_MODEL_COLUMN_DATA, &data,
								-1);

						if (data->node != NULL)
						{
							if (str->len != 0) g_string_prepend (str, "//");
							g_string_prepend (str, anjuta_project_node_get_name (data->node));
						}
					}
					while (gtk_tree_model_iter_parent (model, &iter, &child));
					list = g_list_prepend (list, str->str);
					g_string_free (str, FALSE);
				}
			}
		}
		list = g_list_reverse (list);
	}

	return list;
}

static void
save_expanded_node (GtkTreeView *view, GtkTreePath *path, gpointer user_data)
{
	GList **list = (GList **)user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

	if (gtk_tree_model_get_iter (model, &iter, path))
	{
		GString *str;
		GtkTreeIter child;

		str = g_string_new (NULL);
		do
		{
			GbfTreeData *data;

			child = iter;
			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
				GBF_PROJECT_MODEL_COLUMN_DATA, &data,
				-1);

			if (data->node != NULL)
			{
				if (str->len != 0) g_string_prepend (str, "//");
				g_string_prepend (str, anjuta_project_node_get_name (data->node));
			}
		}
		while (gtk_tree_model_iter_parent (model, &iter, &child));

		*list = g_list_prepend (*list, str->str);
		g_string_free (str, FALSE);
	}
}

GList *
gbf_project_view_get_expanded_list (GbfProjectView *view)
{
	GList *list = NULL;

	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (view), save_expanded_node, &list);
	list = g_list_reverse (list);

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
	GList *item;

	gbf_project_model_set_default_shortcut (view->model, shortcuts == NULL);

	for (item = g_list_first (shortcuts); item != NULL; item = g_list_next (item))
	{
		gchar *name = (gchar *)item->data;
		gchar *end;
		GtkTreeIter iter;
		GtkTreeIter *parent = NULL;

		do
		{
			end = strstr (name, "/" "/");   /* Avoid troubles with auto indent */
			if (end != NULL) *end = '\0';
			if (*name != '\0')
			{
				if (!gbf_project_model_find_child_name (view->model, &iter, parent, name))
				{
					GbfTreeData *data;

					/* Create proxy node */
					data = gbf_tree_data_new_proxy (name, FALSE);
					gtk_tree_store_append (GTK_TREE_STORE (view->model), &iter, parent);
					gtk_tree_store_set (GTK_TREE_STORE (view->model), &iter,
							    GBF_PROJECT_MODEL_COLUMN_DATA, data,
							    -1);
					if (end == NULL)
					{
						data->has_shortcut = TRUE;

						/* Create another proxy at root level to keep shortcut order */
						data = gbf_tree_data_new_proxy (name, FALSE);
						gtk_tree_store_append (GTK_TREE_STORE (view->model), &iter, NULL);
						gtk_tree_store_set (GTK_TREE_STORE (view->model), &iter,
								    GBF_PROJECT_MODEL_COLUMN_DATA, data,
								    -1);
					}
				}
				else
				{
					GbfTreeData *data;

					gtk_tree_model_get (GTK_TREE_MODEL (view->model), &iter,
			    			GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    				-1);
					if (end == NULL) data->has_shortcut = TRUE;
				}
				parent = &iter;
			}
			if (end != NULL)
			{
				*end = '/';
				name = end + 2;
			}
		}
		while (end != NULL);
	}

	return;
}

void
gbf_project_view_set_expanded_list (GbfProjectView *view, GList *expand)
{
	GList *item;

	for (item = g_list_first (expand); item != NULL; item = g_list_next (item))
	{
		gchar *name = (gchar *)item->data;
		gchar *end;
		GtkTreeIter iter;
		GtkTreeIter *parent = NULL;

		do
		{
			end = strstr (name, "/" "/");   /* Avoid troubles with auto indent */
			if (end != NULL) *end = '\0';
			if (*name != '\0')
			{
				if (!gbf_project_model_find_child_name (view->model, &iter, parent, name))
				{
					GbfTreeData *data;

					/* Create proxy node */
					data = gbf_tree_data_new_proxy (name, TRUE);
					gtk_tree_store_append (GTK_TREE_STORE (view->model), &iter, parent);
					gtk_tree_store_set (GTK_TREE_STORE (view->model), &iter,
							    GBF_PROJECT_MODEL_COLUMN_DATA, data,
							    -1);
				}
				else
				{
					GbfTreeData *data;

					gtk_tree_model_get (GTK_TREE_MODEL (view->model), &iter,
			    			GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    				-1);
					data->expanded = TRUE;
				}
				parent = &iter;
			}
			if (end != NULL)
			{
				*end = '/';
				name = end + 2;
			}
		}
		while (end != NULL);
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

	if (gbf_project_model_find_file (view->model, &iter, NULL, gbf_tree_node_type_from_project (type), file))
	{

		node = gbf_project_model_get_node (view->model, &iter);
	}

	return node;
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

		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (view->model),
		                                      GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
		                                      GTK_SORT_ASCENDING);

		found = gbf_project_model_find_node (view->model, &iter, NULL, node);
		if (!found)
		{
			if (anjuta_project_node_parent (node) != NULL)
			{
				g_critical ("Unable to find node %s", anjuta_project_node_get_name (node));
			}
			else
			{
				GtkTreePath *path;
				GtkTreePath *child_path;
				GtkTreeModelFilter *filter;

				if (!gbf_project_model_find_child_name (view->model, &iter, NULL, anjuta_project_node_get_name (node)))
				{
					gbf_project_model_add_node (view->model, node, NULL, 0);
					gtk_tree_model_get_iter_first (GTK_TREE_MODEL (view->model), &iter);
				}
				else
				{
					GbfTreeData *data;
					GbfTreeData *new_data;

					/* Replace with new node */
					gtk_tree_model_get (GTK_TREE_MODEL (view->model), &iter,
						GBF_PROJECT_MODEL_COLUMN_DATA, &data,
						-1);
					new_data = gbf_tree_data_new_node (node);
					gtk_tree_store_set (GTK_TREE_STORE (view->model), &iter,
								GBF_PROJECT_MODEL_COLUMN_DATA, new_data,
								-1);
					gbf_tree_data_free (data);
					gbf_project_view_update_tree (view, node, &iter);
				}

				/* Expand root node */
				path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->model), &iter);
				filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
				child_path = gtk_tree_model_filter_convert_child_path_to_path (filter, path);
				if (child_path != NULL) gtk_tree_view_expand_row (GTK_TREE_VIEW (view), child_path, FALSE);
				gtk_tree_path_free (child_path);
				gtk_tree_path_free (path);
			}
		}
		else
		{
			gbf_project_view_update_tree (view, node, &iter);
		}
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (view->model),
		                                      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
		                                      GTK_SORT_ASCENDING);

		g_signal_emit (G_OBJECT (view), signals[NODE_LOADED], 0, found ? &iter : NULL, complete, NULL);
	}

	if (complete)
	{
		// Add shortcut for all new primary targets
		gbf_project_model_set_default_shortcut (view->model, TRUE);
	}
}


void
gbf_project_view_set_project (GbfProjectView *view, AnjutaPmProject *project)
{
	AnjutaPmProject *old_project;

	old_project = gbf_project_model_get_project (view->model);
	if (old_project != NULL)
	{
		g_signal_handlers_disconnect_by_func (old_project, G_CALLBACK (on_node_loaded), view);
	}

	g_signal_connect (project, "loaded", G_CALLBACK (on_node_loaded), view);

	gbf_project_model_set_project (view->model, project);
}

void
gbf_project_view_set_parent_view (GbfProjectView *view,
                                  GbfProjectView *parent,
                                  GtkTreePath *root)
{

	if (view->model != NULL) g_object_unref (view->model);
	if (view->filter != NULL) g_object_unref (view->model);

	view->model = g_object_ref (parent->model);
	view->filter = GTK_TREE_MODEL_FILTER (pm_project_model_filter_new (GTK_TREE_MODEL (view->model), root));
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (view->filter));
}

void
gbf_project_view_set_visible_func (GbfProjectView *view,
                                   GtkTreeModelFilterVisibleFunc func,
                                   gpointer data,
                                   GDestroyNotify destroy)
{
	if (func == NULL)
	{
		gtk_tree_model_filter_set_visible_func (view->filter, is_project_node_visible, view, NULL);
	}
	else
	{
		gtk_tree_model_filter_set_visible_func (view->filter, func, data, destroy);
	}
	gtk_tree_model_filter_refilter (view->filter);
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

gboolean
gbf_project_view_get_project_root (GbfProjectView *view, GtkTreeIter *iter)
{
	GtkTreeModel *model;
	GtkTreeModel *view_model;
	GtkTreePath *path;
	gboolean ok = FALSE;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
	view_model = model;
	if (GTK_IS_TREE_MODEL_FILTER (model))
	{
		model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (view_model));
	}

	path = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (model));
	if (path)
	{
		ok = gtk_tree_model_get_iter (model, iter, path);
		gtk_tree_path_free (path);
	}

	return ok;
}

/* Public functions
 *---------------------------------------------------------------------------*/

GtkCellLayout *
pm_setup_project_renderer (GtkCellLayout *layout)
{
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (layout, renderer, FALSE);
	gtk_cell_layout_set_cell_data_func (layout, renderer, set_pixbuf, NULL, NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (layout, renderer, FALSE);
	gtk_cell_layout_set_cell_data_func (layout, renderer, set_text, NULL, NULL);

	return layout;
}

gboolean
pm_convert_project_iter_to_model_iter (GtkTreeModel *model,
                                       GtkTreeIter *model_iter,
                                       GtkTreeIter *project_iter)
{
	gboolean found = TRUE;

	g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);

	/* Check if we can find a direct correspondance */
	if ((project_iter == NULL) || !gtk_tree_model_filter_convert_child_iter_to_iter (
			GTK_TREE_MODEL_FILTER (model), model_iter, project_iter))
	{
		GtkTreeModel *project_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));

		found = FALSE;

		/* Check if it is a shortcut or a child of a shortcut */
		if (project_iter != NULL)
		{
			GbfTreeData *data;

			gtk_tree_model_get (project_model, project_iter,
			                    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			                    -1);

			if ((data != NULL) && (data->node != NULL))
			{
				/* Select the corresponding node */
				GtkTreePath *path;
				GtkTreeIter root;
				GtkTreeIter iter;
				gboolean valid = FALSE;

				path = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (project_model));
				if (path)
				{
					valid = gtk_tree_model_get_iter (project_model, &root, path);
					gtk_tree_path_free (path);
				}

				if (valid && gbf_project_model_find_node (GBF_PROJECT_MODEL (project_model), &iter, &root, data->node))
				{
					found = gtk_tree_model_filter_convert_child_iter_to_iter (
							GTK_TREE_MODEL_FILTER (model), model_iter, &iter);
				}
			}
		}

		/* Try to select root node */
		if (!found)
		{

			GtkTreePath *root_path;

			root_path = gbf_project_model_get_project_root (GBF_PROJECT_MODEL (project_model));
			if (root_path)
			{
				GtkTreePath *path;
				path = gtk_tree_model_filter_convert_child_path_to_path (
							GTK_TREE_MODEL_FILTER (model), root_path);
				if (path)
				{
					found = gtk_tree_model_get_iter	(model, model_iter, path);
					gtk_tree_path_free (path);
				}
				gtk_tree_path_free (root_path);
			}
		}

		/* Take the first node */
		if (!found)
		{
			found = gtk_tree_model_get_iter_first (model, model_iter);
		}
	}

	return found;
}


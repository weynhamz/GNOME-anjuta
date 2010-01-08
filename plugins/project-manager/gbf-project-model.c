/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * Copyright (C) 2002 Dave Camp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA. 
 * 
 * Authors: Dave Camp <dave@ximian.com>
 *          Gustavo Giráldez <gustavo.giraldez@gmx.net>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "gbf-project-util.h"
#include "gbf-project-model.h"


struct _GbfProjectModelPrivate {
	IAnjutaProject      *proj;
	gulong               project_updated_handler;
	
	GtkTreeRowReference *root_row;
	GList               *shortcuts;

};

enum {
	PROP_NONE,
	PROP_PROJECT
};


/* Function prototypes ------------- */

static void     gbf_project_model_class_init         (GbfProjectModelClass   *klass);
static void     gbf_project_model_instance_init      (GbfProjectModel        *tree);
static void     gbf_project_model_drag_source_init   (GtkTreeDragSourceIface *iface);
static void     gbf_project_model_drag_dest_init     (GtkTreeDragDestIface   *iface);

static void     load_project                         (GbfProjectModel        *model,
						      IAnjutaProject         *proj);
static void     insert_empty_node                    (GbfProjectModel        *model);
static void     unload_project                       (GbfProjectModel        *model);

static gint     default_sort_func                    (GtkTreeModel           *model,
						      GtkTreeIter            *iter_a,
						      GtkTreeIter            *iter_b,
						      gpointer                user_data);

/* DND functions */

static gboolean row_draggable                        (GtkTreeDragSource      *drag_source,
						      GtkTreePath            *path);

static gboolean drag_data_delete                     (GtkTreeDragSource      *drag_source,
						      GtkTreePath            *path);

static gboolean drag_data_received                   (GtkTreeDragDest        *drag_dest,
						      GtkTreePath            *dest,
						      GtkSelectionData       *selection_data);

static gboolean row_drop_possible                    (GtkTreeDragDest        *drag_dest,
						      GtkTreePath            *dest_path,
						      GtkSelectionData       *selection_data);


static GtkTreeStoreClass *parent_class = NULL;


/* Implementation ---------------- */

/* Helper functions */

static void
my_gtk_tree_model_foreach_child (GtkTreeModel *const model,
                           GtkTreeIter *const parent,
                           GtkTreeModelForeachFunc func,
                           gpointer user_data)
{
  GtkTreeIter iter;
  gboolean success = gtk_tree_model_iter_children(model, &iter, parent);
  while(success)
  {
    if(gtk_tree_model_iter_has_child(model, &iter))
        my_gtk_tree_model_foreach_child (model, &iter, func, NULL);

        success = (!func(model, NULL, &iter, user_data) &&
               gtk_tree_model_iter_next (model, &iter));
  }
}


/* Type & interfaces initialization */

static void
gbf_project_model_drag_source_init (GtkTreeDragSourceIface *iface)
{
	iface->row_draggable = row_draggable;
	iface->drag_data_delete = drag_data_delete;
}

static void
gbf_project_model_drag_dest_init (GtkTreeDragDestIface *iface)
{
	iface->drag_data_received = drag_data_received;
	iface->row_drop_possible = row_drop_possible;
}

static void
gbf_project_model_class_init_trampoline (gpointer klass,
					 gpointer data)
{
	parent_class = g_type_class_ref (GTK_TYPE_TREE_STORE);
	gbf_project_model_class_init (klass);
}

GType
gbf_project_model_get_type (void)
{
	static GType object_type = 0;
	if (object_type == 0) {
		static const GTypeInfo object_info = {
		    sizeof (GbfProjectModelClass),
		    NULL,		/* base_init */
		    NULL,		/* base_finalize */
		    gbf_project_model_class_init_trampoline,
		    NULL,		/* class_finalize */
		    NULL,               /* class_data */
		    sizeof (GbfProjectModel),
		    0,                  /* n_preallocs */
		    (GInstanceInitFunc) gbf_project_model_instance_init
		};
		static const GInterfaceInfo drag_source_info = {
			(GInterfaceInitFunc) gbf_project_model_drag_source_init,
			NULL,
			NULL
		};
		static const GInterfaceInfo drag_dest_info = {
			(GInterfaceInitFunc) gbf_project_model_drag_dest_init,
			NULL,
			NULL
		};

		object_type = g_type_register_static (
			GTK_TYPE_TREE_STORE, "GbfProjectModel",
			&object_info, 0);

		g_type_add_interface_static (object_type,
					     GTK_TYPE_TREE_DRAG_SOURCE,
					     &drag_source_info);

		g_type_add_interface_static (object_type,
					     GTK_TYPE_TREE_DRAG_DEST,
					     &drag_dest_info);
	}
	return object_type;
}

static void 
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
        GbfProjectModel *model = GBF_PROJECT_MODEL (object);
        
        switch (prop_id) {
        case PROP_PROJECT:
                g_value_set_pointer (value, model->priv->proj);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
        GbfProjectModel *model = GBF_PROJECT_MODEL (object);

        switch (prop_id) {
        case PROP_PROJECT:
		gbf_project_model_set_project (model, g_value_get_pointer (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }   
}

static void
dispose (GObject *obj)
{
	GbfProjectModel *model = GBF_PROJECT_MODEL (obj);

	if (model->priv->proj) {
		unload_project (model);
	}

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
finalize (GObject *obj)
{
	GbfProjectModel *model = GBF_PROJECT_MODEL (obj);

	g_free (model->priv);

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void 
gbf_project_model_class_init (GbfProjectModelClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->dispose = dispose;
	G_OBJECT_CLASS (klass)->finalize = finalize;
	G_OBJECT_CLASS (klass)->get_property = get_property;
	G_OBJECT_CLASS (klass)->set_property = set_property;

	g_object_class_install_property 
                (G_OBJECT_CLASS (klass), PROP_PROJECT,
                 g_param_spec_pointer ("project", 
                                       _("Project"),
                                       _("GbfProject Object"),
                                       G_PARAM_READWRITE));
}

static void
gbf_project_model_instance_init (GbfProjectModel *model)
{
	static GType types [GBF_PROJECT_MODEL_NUM_COLUMNS];

	types [GBF_PROJECT_MODEL_COLUMN_DATA] = G_TYPE_POINTER;

	gtk_tree_store_set_column_types (GTK_TREE_STORE (model),
					 GBF_PROJECT_MODEL_NUM_COLUMNS,
					 types);

	model->priv = g_new0 (GbfProjectModelPrivate, 1);

	/* sorting function */
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
						 default_sort_func,
						 NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);

	insert_empty_node (model);
}

/* Model data functions ------------ */

static gboolean
gbf_project_model_remove (GbfProjectModel *model, GtkTreeIter *iter)
{
	GtkTreeIter child;
	GbfTreeData *data;
	gboolean valid;

	/* Free all children */
	valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &child, iter);
	while (valid)
	{
		valid = gbf_project_model_remove (model, &child);
	}
	
	gtk_tree_model_get (model, iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);
	valid = gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
	if (data != NULL) gbf_tree_data_free (data);

	return valid;
}


static void
gbf_project_model_clear (GbfProjectModel *model)
{
	GtkTreeIter child;
	GbfTreeData *data;
	gboolean valid;

	valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &child, NULL);
	while (valid)
	{
		valid = gbf_project_model_remove (model, &child);
	}
}

static gint 
default_sort_func (GtkTreeModel *model,
		   GtkTreeIter  *iter_a,
		   GtkTreeIter  *iter_b,
		   gpointer      user_data)
{
	GbfTreeData *data_a, *data_b;
	gint retval = 0;
	
	gtk_tree_model_get (model, iter_a,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data_a,
			    -1);
	gtk_tree_model_get (model, iter_b,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data_b,
			    -1);

	if (data_a->is_shortcut && data_b->is_shortcut) {
		GtkTreeIter iter;
		gboolean valid;
		
		/* special case: the order of shortcuts is
		 * user customizable */
		for (valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);
		    valid == TRUE;
		    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter))
		{
			GbfTreeData *data;
			
			gtk_tree_model_get (model, &iter,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
				    -1);
			if (data == data_a) {
				/* a comes first */
				retval = -1;
				break;
			}
			else if (data == data_b) {
				/* b comes first */
				retval = 1;
				break;
			}
		}
		
	} else if (data_a->is_shortcut && !data_b->is_shortcut) {
		retval = -1;
	 
	} else if (!data_a->is_shortcut && data_b->is_shortcut) {
		retval = 1;
	 
	} else if (data_a->type == data_b->type) {
		retval = strcmp (data_a->name, data_b->name);

	} else {
		/* assume a->b and check for the opposite cases */
		retval = -1;
		if (data_a->type == GBF_TREE_NODE_TARGET &&
		    data_b->type == GBF_TREE_NODE_GROUP) {
			retval = 1;
		}
	}
	
	return retval;
}


static void 
add_source (GbfProjectModel    	      *model,
	    AnjutaProjectSource *source,
	    GtkTreeIter               *parent)
{
	GtkTreeIter iter;
	GbfTreeData *data;

	if ((!source) || (anjuta_project_node_get_type (source) != ANJUTA_PROJECT_SOURCE))
		return;
	
	data = gbf_tree_data_new_source (source);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
}

static void 
add_target_shortcut (GbfProjectModel *model,
		     GbfTreeData     *target,
		     GtkTreePath     *before_path)
{
	AnjutaProjectNode *node;
	GtkTreeIter iter, sibling;
	GtkTreePath *root_path;
	GbfTreeData *data;
	AnjutaProjectNode *parent;
	
	if (!target)
		return;

	root_path = gtk_tree_row_reference_get_path (model->priv->root_row);
	/* check before_path */
	if (!before_path ||
	    gtk_tree_path_get_depth (before_path) > 1 ||
	    gtk_tree_path_compare (before_path, root_path) > 0) {
		before_path = root_path;
	}
		
	/* get the tree iter for the row before which to insert the shortcut */
	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &sibling, before_path)) {
		gtk_tree_path_free (root_path);
		return;
	}

	if (target->type != GBF_TREE_NODE_SHORTCUT)
	{
		data = gbf_tree_data_new_shortcut (target);
	}
	else
	{
		data = target;
	}
	gtk_tree_store_insert_before (GTK_TREE_STORE (model), &iter, NULL, &sibling);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
	
	/* add sources */
	parent = gbf_tree_data_get_node (target, model->priv->proj);
	for (node = anjuta_project_node_first_child (parent); node; node = anjuta_project_node_next_sibling (node))
		add_source (model, node, &iter);

	gtk_tree_path_free (root_path);
}

static void 
move_target_shortcut (GbfProjectModel *model,
		     GtkTreeIter     *iter,
    		     GbfTreeData     *shortcut,
		     GtkTreePath     *before_path)
{
	AnjutaProjectNode *node;
	GtkTreeIter sibling;
	GtkTreePath *root_path;
	GtkTreePath *src_path;
	AnjutaProjectNode *parent;
	
	if (!shortcut)
		return;

	root_path = gtk_tree_row_reference_get_path (model->priv->root_row);
	/* check before_path */
	if (!before_path ||
	    gtk_tree_path_get_depth (before_path) > 1 ||
	    gtk_tree_path_compare (before_path, root_path) > 0) {
		before_path = root_path;
	}
		
	/* get the tree iter for the row before which to insert the shortcut */
	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &sibling, before_path)) {
		gtk_tree_path_free (root_path);
		return;
	}

	src_path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), iter);
	if (gtk_tree_path_compare (src_path, before_path) != 0)
	{
		gtk_tree_store_remove (GTK_TREE_STORE (model), iter);			
		gtk_tree_store_insert_before (GTK_TREE_STORE (model), iter, NULL, &sibling);
		gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
				    GBF_PROJECT_MODEL_COLUMN_DATA, shortcut,
				    -1);

		/* add sources */
		parent = gbf_tree_data_get_node (shortcut->shortcut, model->priv->proj);
		for (node = anjuta_project_node_first_child (parent); node; node = anjuta_project_node_next_sibling (node))
			add_source (model, node, iter);
	}

	gtk_tree_path_free (src_path);
	gtk_tree_path_free (root_path);

}

static void 
add_target (GbfProjectModel 		*model,
	    AnjutaProjectTarget   *target,
	    GtkTreeIter     	        *parent)
{
	AnjutaProjectNode *l;
	GtkTreeIter iter;	
	GbfTreeData *data;
	
	if ((!target) || (anjuta_project_node_get_type (target) != ANJUTA_PROJECT_TARGET))
		return;
	
	data = gbf_tree_data_new_target (target);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
	
	/* add sources */
	for (l = anjuta_project_node_first_child (target); l; l = anjuta_project_node_next_sibling (l))
	{
		add_source (model, l, &iter);
	}

	/* add a shortcut to the target if the target's type is a primary */
	/* FIXME: this shouldn't be here.  We would rather provide a
	 * set of public functions to add/remove shortcuts to save
	 * this information in the project metadata (when that's
	 * implemented) */
	switch (anjuta_project_target_type_class (anjuta_project_target_get_type (target)))
	{
		case ANJUTA_TARGET_SHAREDLIB:
		case ANJUTA_TARGET_STATICLIB:
		case ANJUTA_TARGET_EXECUTABLE:
		case ANJUTA_TARGET_PYTHON:
		case ANJUTA_TARGET_JAVA:
			add_target_shortcut (model, data, NULL);
			break;
		default:
			break;
	}
}

static void 
add_target_group (GbfProjectModel 	*model,
		  AnjutaProjectGroup	*group,
		  GtkTreeIter     	*parent)
{
	GtkTreeIter iter;
	AnjutaProjectNode *node;
	GbfTreeData *data;

	if ((!group) || (anjuta_project_node_get_type (group) != ANJUTA_PROJECT_GROUP))
		return;
	
	data = gbf_tree_data_new_group (group);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);

	/* create root reference */
	if (parent == NULL) {
		GtkTreePath *root_path;

		root_path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
		model->priv->root_row = gtk_tree_row_reference_new (
			GTK_TREE_MODEL (model), root_path);
		gtk_tree_path_free (root_path);
	}

	/* add groups ... */
	for (node = anjuta_project_node_first_child (group); node; node = anjuta_project_node_next_sibling (node))
		add_target_group (model, node, &iter);
	
	/* ... and targets */
	for (node = anjuta_project_node_first_child (group); node; node = anjuta_project_node_next_sibling (node))
		add_target (model, node, &iter);
	
	/* ... and sources */
	for (node = anjuta_project_node_first_child (group); node; node = anjuta_project_node_next_sibling (node))
		add_source (model, node, &iter);
}

static void
update_tree (GbfProjectModel *model, AnjutaProjectNode *parent, GtkTreeIter *iter)
{
	GtkTreeIter child;
	GList *node;
	GList *nodes;

	/* group can be NULL, but we iterate anyway to remove any
	 * shortcuts the old group could have had */
	
	/* Get all new nodes */
	nodes = gbf_project_util_all_child (parent, ANJUTA_PROJECT_UNKNOWN);

	/* walk the tree nodes */
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &child, iter)) {
		gboolean valid = TRUE;
		
		while (valid) {
			AnjutaProjectNode* data;
			GbfTreeData *tree_data = NULL;

			/* Get tree data */
			gtk_tree_model_get (GTK_TREE_MODEL (model), &child,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &tree_data,
			    -1);

			data = gbf_project_model_get_node (model, &child);

			if (data != NULL)
			{
				/* Remove from the new node list */
				node = g_list_find (nodes, data);
				if (node != NULL)
				{
					nodes = g_list_delete_link (nodes, node);
				}

				/* update recursively */
				update_tree (model, data, &child);
				
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &child);
			}
			else
			{
				/* update recursively */
				update_tree (model, data, &child);
				
				valid = gbf_project_model_remove (model, &child);
			}
		}
	}

	/* add the remaining sources, targets and groups */
	for (node = nodes; node; node = node->next)
	{
		switch (anjuta_project_node_get_type (node->data))
		{
		case ANJUTA_PROJECT_GROUP:
			add_target_group (model, node->data, iter);
			break;
		case ANJUTA_PROJECT_TARGET:
			add_target (model, node->data, iter);
			break;
		case ANJUTA_PROJECT_SOURCE:
			add_source (model, node->data, iter);
			break;
		default:
			break;
		}
	}
}

static void
project_updated_cb (IAnjutaProject *project, GbfProjectModel *model)
{
	update_tree (model, NULL, NULL);
}

static void
load_project (GbfProjectModel *model, IAnjutaProject *proj)
{
	model->priv->proj = proj;
	g_object_ref (proj);

	/* to get rid of the empty node */
	gbf_project_model_clear (model);

	add_target_group (model, ianjuta_project_get_root (proj, NULL), NULL);

	model->priv->project_updated_handler =
		g_signal_connect (model->priv->proj, "project-updated",
				  (GCallback) project_updated_cb, model);
}

static void 
insert_empty_node (GbfProjectModel *model)
{
	GtkTreeIter iter;
	GbfTreeData *empty_node;

	empty_node = gbf_tree_data_new_string (_("No project loaded"));

	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, empty_node,
			    -1);
}

static void
unload_project (GbfProjectModel *model)
{
	if (model->priv->proj) {
		gtk_tree_row_reference_free (model->priv->root_row);
		model->priv->root_row = NULL;

		gbf_project_model_clear (model);

		g_list_free (model->priv->shortcuts);
		model->priv->shortcuts = NULL;
		
		g_signal_handler_disconnect (model->priv->proj,
					     model->priv->project_updated_handler);
		model->priv->project_updated_handler = 0;
		g_object_unref (model->priv->proj);
		model->priv->proj = NULL;

		insert_empty_node (model);
	}
}

static gboolean 
recursive_find_id (GtkTreeModel   	*model,
		   GtkTreeIter     	*iter,
		   GbfTreeData  	*data)
{
	GtkTreeIter tmp;
	gboolean retval = FALSE;

	tmp = *iter;
	
	do {
		GtkTreeIter child;
		GbfTreeData *tmp_data;
		
		gtk_tree_model_get (model, &tmp,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &tmp_data, -1);
		if (tmp_data == data) {
			*iter = tmp;
			retval = TRUE;
		}
		
		if (gtk_tree_model_iter_children (model, &child, &tmp)) {
			if (recursive_find_id (model, &child, data)) {
				*iter = child;
				retval = TRUE;
			}
		}
				
	} while (!retval && gtk_tree_model_iter_next (model, &tmp));

	return retval;
}

gboolean 
gbf_project_model_find_id (GbfProjectModel 	*model,
			   GtkTreeIter     	*iter,
			   GbfTreeData  	*data)
{
	GtkTreePath *root;
	GtkTreeIter tmp_iter;
	gboolean retval = FALSE;
	
	root = gbf_project_model_get_project_root (model);
	if (!root)
		return FALSE;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &tmp_iter, root)) {
		if (recursive_find_id (GTK_TREE_MODEL (model), &tmp_iter, data)) {
			retval = TRUE;
			*iter = tmp_iter;
		}
	}
	gtk_tree_path_free (root);
	
	return retval;
}

static GbfTreeData *
recursive_find_group (GtkTreeModel   	*model,
		   GtkTreeIter     	*parent,
    		   GtkTreeIter		*found,
		   GFile		*file)
{
	GtkTreeIter iter;
	gboolean next;
	GbfTreeData *data = NULL;

	for (next = gtk_tree_model_iter_children (model, &iter, parent);
	    next;
	    next = gtk_tree_model_iter_next (model, &iter))
	{
		gtk_tree_model_get (model, &iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

		if (data->type == GBF_TREE_NODE_SHORTCUT)
		{
			data = data->shortcut;
		}
		if ((data->type == GBF_TREE_NODE_GROUP) && (data->group != NULL) && g_file_equal (data->group, file))
		{
			if (found != NULL) *found = iter;
			return data;
		}

		data = recursive_find_group (model, &iter, found, file);
		if (data != NULL) return data;
	}

	return NULL;
}

static GbfTreeData *
recursive_find_target (GtkTreeModel   	*model,
		   GtkTreeIter     	*parent,
    		   GtkTreeIter		*found,
		   const gchar	        *name)
{
	GtkTreeIter iter;
	gboolean next;
	GbfTreeData *data = NULL;

	for (next = gtk_tree_model_iter_children (model, &iter, parent);
	    next;
	    next = gtk_tree_model_iter_next (model, &iter))
	{
		gtk_tree_model_get (model, &iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

		if (data->type == GBF_TREE_NODE_SHORTCUT)
		{
			data = data->shortcut;
		}
		if ((data->type == GBF_TREE_NODE_TARGET) && (data->name != NULL) && (strcmp (data->name, name) == 0))
		{
			if (found != NULL) *found = iter;
			return data;
		}
		else
		{
			return NULL;
		}
	}

	return NULL;
}

static GbfTreeData *
recursive_find_source (GtkTreeModel   	*model,
		   GtkTreeIter     	*parent,
		   GtkTreeIter     	*found,
		   GFile		*file)
{
	GtkTreeIter iter;
	gboolean next;
	GbfTreeData *data = NULL;

	for (next = gtk_tree_model_iter_children (model, &iter, parent);
	    next;
	    next = gtk_tree_model_iter_next (model, &iter))
	{
		gtk_tree_model_get (model, &iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

		if (data->type == GBF_TREE_NODE_SHORTCUT)
		{
			data = data->shortcut;
		}
		if ((data->type == GBF_TREE_NODE_SOURCE) && (data->source != NULL) && g_file_equal (data->source, file))
		{
			if (found != NULL) *found = iter;
			return data;
		}
		
		data = recursive_find_group (model, &iter, found, file);
		if (data != NULL) return data;
	}

	return NULL;
}

GbfTreeData*
gbf_project_model_find_uri (GbfProjectModel     *model,
    			    const gchar         *uri,
    			    GbfTreeNodeType     type)
{
	GFile *file;
	GFile *group;
	gchar *name;
	GbfTreeData *data = NULL;
	GtkTreeIter iter;

	file = g_file_new_for_uri (uri);
	switch (type)
	{
	case GBF_TREE_NODE_SOURCE:
		data = recursive_find_source (GTK_TREE_MODEL (model), NULL, NULL, file);
		break;
	case GBF_TREE_NODE_GROUP:
		data = recursive_find_group (GTK_TREE_MODEL (model), NULL, NULL, file);
		break;
	case GBF_TREE_NODE_TARGET:
		group = g_file_get_parent (file);
		name = g_file_get_basename (file);
		if (recursive_find_group (GTK_TREE_MODEL (model), NULL, &iter, group))
		{
			data = recursive_find_target (GTK_TREE_MODEL (model), &iter, NULL, name);
		}
		g_free (name);
		g_object_unref (group);
		break;
	default:
		break;
	}
		
	g_object_unref (file);

	return data;
}

GbfProjectModel *
gbf_project_model_new (IAnjutaProject *project)
{
	return GBF_PROJECT_MODEL (g_object_new (GBF_TYPE_PROJECT_MODEL,
						"project", project,
						NULL));
}

void 
gbf_project_model_set_project (GbfProjectModel *model, IAnjutaProject *project)
{
	g_return_if_fail (model != NULL && GBF_IS_PROJECT_MODEL (model));
	g_return_if_fail (project == NULL || IANJUTA_IS_PROJECT (project));
	
	if (model->priv->proj)
		unload_project (model);
	
	if (project)
		load_project (model, project);
}

IAnjutaProject *
gbf_project_model_get_project (GbfProjectModel *model)
{
	g_return_val_if_fail (model != NULL && GBF_IS_PROJECT_MODEL (model), NULL);
	
	return model->priv->proj;
}

GtkTreePath *
gbf_project_model_get_project_root (GbfProjectModel *model)
{
	GtkTreePath *path = NULL;
	
	g_return_val_if_fail (GBF_IS_PROJECT_MODEL (model), NULL);

	if (model->priv->root_row)
		path = gtk_tree_row_reference_get_path (model->priv->root_row);
	
	return path;
}

AnjutaProjectNode *
gbf_project_model_get_node (GbfProjectModel *model,
                            GtkTreeIter     *iter)
{
	GbfTreeData *data = NULL;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
			    -1);

	return gbf_tree_data_get_node (data, model->priv->proj);
}

/* DND stuff ------------- */

static gboolean
row_draggable (GtkTreeDragSource *drag_source, GtkTreePath *path)
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
drag_data_delete (GtkTreeDragSource *drag_source, GtkTreePath *path)
{
	return FALSE;
}

static gboolean
drag_data_received (GtkTreeDragDest  *drag_dest,
		    GtkTreePath      *dest,
		    GtkSelectionData *selection_data)
{
	GtkTreeModel *src_model = NULL;
	GtkTreePath *src_path = NULL;
	gboolean retval = FALSE;

	g_return_val_if_fail (GBF_IS_PROJECT_MODEL (drag_dest), FALSE);

	if (gtk_tree_get_row_drag_data (selection_data,
					&src_model,
					&src_path) &&
	    src_model == GTK_TREE_MODEL (drag_dest)) {

		GtkTreeIter iter;
		GbfTreeData *data = NULL;
		
		if (gtk_tree_model_get_iter (src_model, &iter, src_path)) {
			gtk_tree_model_get (src_model, &iter,
					    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
					    -1);
			if (data != NULL)
			{
				if (data->type == GBF_TREE_NODE_SHORTCUT)
				{
					move_target_shortcut (GBF_PROJECT_MODEL (drag_dest),
					    		&iter, data, dest);
				}
				else
				{
					add_target_shortcut (GBF_PROJECT_MODEL (drag_dest),
						     	data, dest);
				}
				retval = TRUE;
			}
		}
	}

	if (src_path)
		gtk_tree_path_free (src_path);

	return retval;
}

static gboolean
row_drop_possible (GtkTreeDragDest  *drag_dest,
		   GtkTreePath      *dest_path,
		   GtkSelectionData *selection_data)
{
	GtkTreePath *root_path;
	GbfProjectModel *model;
	gboolean retval = FALSE;
	GtkTreeModel *src_model = NULL;
  
	g_return_val_if_fail (GBF_IS_PROJECT_MODEL (drag_dest), FALSE);

	model = GBF_PROJECT_MODEL (drag_dest);

	if (!gtk_tree_get_row_drag_data (selection_data,
					 &src_model,
					 NULL))
		return FALSE;
    
	/* can only drag to ourselves and only new toplevel nodes will
	 * be created */
	if (src_model == GTK_TREE_MODEL (drag_dest) &&
	    gtk_tree_path_get_depth (dest_path) == 1) {
		root_path = gtk_tree_row_reference_get_path (model->priv->root_row);
		if (gtk_tree_path_compare (dest_path, root_path) <= 0) {
			retval = TRUE;
		}
		gtk_tree_path_free (root_path);
	}

	return retval;
}

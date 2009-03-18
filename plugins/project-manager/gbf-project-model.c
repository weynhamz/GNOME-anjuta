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

#include "gbf-project-model.h"


struct _GbfProjectModelPrivate {
	GbfProject          *proj;
	gulong               project_updated_handler;
	
	GtkTreeRowReference *root_row;
	GList               *shortcuts;

	GbfTreeData         *empty_node;
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
						      GbfProject             *proj);
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

	if (model->priv->empty_node) {
		gbf_tree_data_free (model->priv->empty_node);
		model->priv->empty_node = NULL;
	}
	
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

	types [GBF_PROJECT_MODEL_COLUMN_DATA] = GBF_TYPE_TREE_DATA;

	gtk_tree_store_set_column_types (GTK_TREE_STORE (model),
					 GBF_PROJECT_MODEL_NUM_COLUMNS,
					 types);

	model->priv = g_new0 (GbfProjectModelPrivate, 1);

	model->priv->empty_node = gbf_tree_data_new_string (_("No project loaded"));
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
		GList *l;
		
		/* special case: the order of shortcuts is
		 * user customizable */
		for (l = GBF_PROJECT_MODEL (model)->priv->shortcuts; l; l = l->next) {
			if (!strcmp (l->data, data_a->id)) {
				/* a comes first */
				retval = -1;
				break;
			}
			else if (!strcmp (l->data, data_b->id)) {
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
	
	gbf_tree_data_free (data_a);
	gbf_tree_data_free (data_b);
	
	return retval;
}


static void 
add_source (GbfProjectModel *model,
	    const gchar     *source_id,
	    GtkTreeIter     *parent)
{
	GbfProjectTargetSource *source;
	GtkTreeIter iter;
	GbfTreeData *data;
	
	source = gbf_project_get_source (model->priv->proj, source_id, NULL);
	if (!source)
		return;
	
	data = gbf_tree_data_new_source (model->priv->proj, source);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
	gbf_tree_data_free (data);

	gbf_project_target_source_free (source);
}

static GtkTreePath *
find_shortcut (GbfProjectModel *model, const gchar *target_id)
{
	GList *l;
	gint i;
	
	for (l = model->priv->shortcuts, i = 0; l; l = l->next, i++) {
		if (!strcmp (target_id, l->data))
			return gtk_tree_path_new_from_indices (i, -1);
	}
	return NULL;
}

static void
remove_shortcut (GbfProjectModel *model, const gchar *target_id)
{
	GList *l;
	
	for (l = model->priv->shortcuts; l; l = l->next) {
		if (!strcmp (target_id, l->data)) {
			g_free (l->data);
			model->priv->shortcuts = g_list_delete_link (
				model->priv->shortcuts, l);
			break;
		}
	}
}

static void 
add_target_shortcut (GbfProjectModel *model,
		     const gchar     *target_id,
		     GtkTreePath     *before_path)
{
	GList *l;
	GtkTreeIter iter, sibling;
	GbfProjectTarget *target;
	GtkTreePath *root_path, *old_path;
	gint *path_indices, i;
	GbfTreeData *data;
	
	target = gbf_project_get_target (model->priv->proj, target_id, NULL);
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
	
	path_indices = gtk_tree_path_get_indices (before_path);
	i = path_indices [0];

	/* remove the old shortcut to make sorting actually work */
	old_path = find_shortcut (model, target_id);
	if (old_path) {
		remove_shortcut (model, target_id);
		if (gtk_tree_path_compare (old_path, before_path) < 0) {
			/* adjust shortcut insert position if the old
			 * index was before the new site */
			i--;
		}
		gtk_tree_path_free (old_path);
	}
			
	/* add entry to the shortcut list */
	model->priv->shortcuts = g_list_insert (model->priv->shortcuts,
						g_strdup (target->id), i);
	
	data = gbf_tree_data_new_target (model->priv->proj, target);
	data->is_shortcut = TRUE;
	gtk_tree_store_insert_before (GTK_TREE_STORE (model), &iter, NULL, &sibling);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
	gbf_tree_data_free (data);
	
	/* add sources */
	for (l = target->sources; l; l = l->next)
		add_source (model, l->data, &iter);

	gtk_tree_path_free (root_path);
	gbf_project_target_free (target);
}

static void 
add_target (GbfProjectModel *model,
	    const gchar     *target_id,
	    GtkTreeIter     *parent)
{
	GbfProjectTarget *target;
	GList *l;
	GtkTreeIter iter;	
	GbfTreeData *data;
	
	target = gbf_project_get_target (model->priv->proj, target_id, NULL);
	if (!target)
		return;
	
	data = gbf_tree_data_new_target (model->priv->proj, target);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
	gbf_tree_data_free (data);
	
	/* add sources */
	for (l = target->sources; l; l = l->next)
		add_source (model, l->data, &iter);

	/* add a shortcut to the target if the target's type is a primary */
	/* FIXME: this shouldn't be here.  We would rather provide a
	 * set of public functions to add/remove shortcuts to save
	 * this information in the project metadata (when that's
	 * implemented) */
	if (!strcmp (target->type, "program") ||
	    !strcmp (target->type, "shared_lib") ||
	    !strcmp (target->type, "static_lib") ||
	    !strcmp (target->type, "java") ||
	    !strcmp (target->type, "python")) {
		add_target_shortcut (model, target->id, NULL);
	}

	gbf_project_target_free (target);
}

static void
update_target (GbfProjectModel *model, const gchar *target_id, GtkTreeIter *iter)
{
	GtkTreeModel *tree_model;
	GbfProjectTarget *target;
	GtkTreeIter child;
	GList *node;
	
	tree_model = GTK_TREE_MODEL (model);
	target = gbf_project_get_target (model->priv->proj, target_id, NULL);
	if (!target)
		return;
	
	/* update target data here */
	
	/* walk the tree target */
	if (gtk_tree_model_iter_children (tree_model, &child, iter)) {
		GbfTreeData *data;
		gboolean valid = TRUE;
		
		while (valid) {
			gtk_tree_model_get (tree_model, &child,
					    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
					    -1);

			/* find the iterating id in the target's sources */
			if (data->id) {
				node = g_list_find_custom (target->sources,
							   data->id, (GCompareFunc) strcmp);
				if (node) {
					target->sources = g_list_delete_link (target->sources, node);
					valid = gtk_tree_model_iter_next (tree_model, &child);
				} else {
					valid = gtk_tree_store_remove (GTK_TREE_STORE (model), &child);
				}
				gbf_tree_data_free (data);
			}
		}
	}

	/* add the remaining sources */
	for (node = target->sources; node; node = node->next)
		add_source (model, node->data, iter);
	
	gbf_project_target_free (target);
}

static void 
add_target_group (GbfProjectModel *model,
		  const gchar     *group_id,
		  GtkTreeIter     *parent)
{
	GbfProjectGroup *group;
	GtkTreeIter iter;
	GList *l;
	GbfTreeData *data;

	group = gbf_project_get_group (model->priv->proj, group_id, NULL);
	if (!group)
		return;
	
	data = gbf_tree_data_new_group (model->priv->proj, group);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			    GBF_PROJECT_MODEL_COLUMN_DATA, data,
			    -1);
	gbf_tree_data_free (data);

	/* create root reference */
	if (parent == NULL) {
		GtkTreePath *root_path;

		root_path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
		model->priv->root_row = gtk_tree_row_reference_new (
			GTK_TREE_MODEL (model), root_path);
		gtk_tree_path_free (root_path);
	}

	/* add groups ... */
	for (l = group->groups; l; l = l->next)
		add_target_group (model, l->data, &iter);

	/* ... and targets */
	for (l = group->targets; l; l = l->next)
		add_target (model, l->data, &iter);

	gbf_project_group_free (group);
}

static void
update_group (GbfProjectModel *model, const gchar *group_id, GtkTreeIter *iter)
{
	GtkTreeModel *tree_model;
	GbfProjectGroup *group;
	GtkTreeIter child;
	GList *node;
	
	tree_model = GTK_TREE_MODEL (model);
	group = gbf_project_get_group (model->priv->proj, group_id, NULL);
	
	/* update group data. nothing to do here */

	/* walk the tree group */
	/* group can be NULL, but we iterate anyway to remove any
	 * shortcuts the old group could have had */
	if (gtk_tree_model_iter_children (tree_model, &child, iter)) {
		GbfTreeData *data;
		gboolean valid = TRUE;
		
		while (valid) {
			gboolean remove_child = FALSE;
			
			gtk_tree_model_get (tree_model, &child,
					    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
					    -1);

			/* find the iterating id in the group's children */
			if (data->type == GBF_TREE_NODE_GROUP) {
				/* update recursively */
				update_group (model, data->id, &child);
				if (group && (node = g_list_find_custom (group->groups, data->id,
									 (GCompareFunc) strcmp))) {
					group->groups = g_list_delete_link (group->groups, node);
				} else {
					remove_child = TRUE;
				}
				
			} else if (data->type == GBF_TREE_NODE_TARGET) {
				GtkTreePath *shortcut;

				if (group && (node = g_list_find_custom (group->targets,
									 data->id,
									 (GCompareFunc) strcmp))) {
					group->targets = g_list_delete_link (group->targets, node);
					/* update recursively */
					update_target (model, data->id, &child);
				} else {
					remove_child = TRUE;
				}

				/* remove or update the shortcut if it previously existed */
				shortcut = find_shortcut (model, data->id);
				if (shortcut) {
					GtkTreeIter tmp;
					
					if (remove_child)
						remove_shortcut (model, data->id);

					if (gtk_tree_model_get_iter (tree_model, &tmp, shortcut)) {
						if (remove_child)
							gtk_tree_store_remove (GTK_TREE_STORE (model), &tmp);
						else
							update_target (model, data->id, &tmp);
					}
					gtk_tree_path_free (shortcut);
				}
			}
			
			gbf_tree_data_free (data);
			if (remove_child)
				valid = gtk_tree_store_remove (GTK_TREE_STORE (model), &child);
			else
				valid = gtk_tree_model_iter_next (tree_model, &child);
		};
	}

	if (group) {
		/* add the remaining targets and groups */
		for (node = group->groups; node; node = node->next)
			add_target_group (model, node->data, iter);
		
		for (node = group->targets; node; node = node->next)
			add_target (model, node->data, iter);
	
		gbf_project_group_free (group);
	}
}

static void
project_updated_cb (GbfProject *project, GbfProjectModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_row_reference_get_path (model->priv->root_row);
	if (path && gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path))
		update_group (model, "/", &iter);
	else
		add_target_group (model, "/", NULL);
			
	if (path)
		gtk_tree_path_free (path);
}

static void
load_project (GbfProjectModel *model, GbfProject *proj)
{
	model->priv->proj = proj;
	g_object_ref (proj);

	/* to get rid of the empty node */
	gtk_tree_store_clear (GTK_TREE_STORE (model));

	add_target_group (model, "/", NULL);

	model->priv->project_updated_handler =
		g_signal_connect (model->priv->proj, "project-updated",
				  (GCallback) project_updated_cb, model);
}

static void 
insert_empty_node (GbfProjectModel *model)
{
	GtkTreeIter iter;

	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			    GBF_PROJECT_MODEL_COLUMN_DATA, model->priv->empty_node,
			    -1);
}

static void
unload_project (GbfProjectModel *model)
{
	if (model->priv->proj) {
		gtk_tree_row_reference_free (model->priv->root_row);
		model->priv->root_row = NULL;

		gtk_tree_store_clear (GTK_TREE_STORE (model));

		g_list_foreach (model->priv->shortcuts, (GFunc) g_free, NULL);
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
recursive_find_id (GtkTreeModel    *model,
		   GtkTreeIter     *iter,
		   GbfTreeNodeType  type,
		   const gchar     *id)
{
	GtkTreeIter tmp;
	GbfTreeData *data;
	gboolean retval = FALSE;

	tmp = *iter;
	
	do {
		GtkTreeIter child;
		
		gtk_tree_model_get (model, &tmp,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
		if (data->type == type && !strcmp (id, data->id)) {
			*iter = tmp;
			retval = TRUE;
		}
		gbf_tree_data_free (data);
		
		if (gtk_tree_model_iter_children (model, &child, &tmp)) {
			if (recursive_find_id (model, &child, type, id)) {
				*iter = child;
				retval = TRUE;
			}
		}
				
	} while (!retval && gtk_tree_model_iter_next (model, &tmp));

	return retval;
}

gboolean 
gbf_project_model_find_id (GbfProjectModel *model,
			   GtkTreeIter     *iter,
			   GbfTreeNodeType  type,
			   const gchar     *id)
{
	GtkTreePath *root;
	GtkTreeIter tmp_iter;
	gboolean retval = FALSE;
	
	root = gbf_project_model_get_project_root (model);
	if (!root)
		return FALSE;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &tmp_iter, root)) {
		if (recursive_find_id (GTK_TREE_MODEL (model), &tmp_iter, type, id)) {
			retval = TRUE;
			*iter = tmp_iter;
		}
	}
	gtk_tree_path_free (root);
	
	return retval;
}

GbfProjectModel *
gbf_project_model_new (GbfProject *project)
{
	return GBF_PROJECT_MODEL (g_object_new (GBF_TYPE_PROJECT_MODEL,
						"project", project,
						NULL));
}

void 
gbf_project_model_set_project (GbfProjectModel *model, GbfProject *project)
{
	g_return_if_fail (model != NULL && GBF_IS_PROJECT_MODEL (model));
	g_return_if_fail (project == NULL || GBF_IS_PROJECT (project));
	
	if (model->priv->proj)
		unload_project (model);
	
	if (project)
		load_project (model, project);
}

GbfProject *
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
		GtkTreePath *found;
		
		/* don't allow duplicate shortcuts */
		found = find_shortcut (GBF_PROJECT_MODEL (drag_source), data->id);
		if (!found)
			retval = TRUE;
		else
			gtk_tree_path_free (found);
	}
	gbf_tree_data_free (data);

	return retval;
}

static gboolean 
drag_data_delete (GtkTreeDragSource *drag_source, GtkTreePath *path)
{
	GtkTreeIter iter;
	gboolean retval = FALSE;
	
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_source),
				     &iter, path)) {
		GbfTreeData *data;

		gtk_tree_model_get (GTK_TREE_MODEL (drag_source), &iter,
				    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
				    -1);

		if (data->is_shortcut) {
			gtk_tree_store_remove (GTK_TREE_STORE (drag_source), &iter);
			retval = TRUE;
		}
		gbf_tree_data_free (data);
	}
	
	return retval;
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
			if (data && data->id && data->type == GBF_TREE_NODE_TARGET) {
				add_target_shortcut (GBF_PROJECT_MODEL (drag_dest),
						     data->id, dest);
				retval = TRUE;
			}
			gbf_tree_data_free (data);
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

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */

/* project-chooser.c
 *
 * Copyright (C) 2012  Sébastien Granjoux
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
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "project-chooser.h"

#include <glib/gi18n.h>

#include <libanjuta/interfaces/ianjuta-project-chooser.h>

#include "project-model.h"
#include "project-view.h"
#include "tree-data.h"
#include "plugin.h"


/* Types
 *---------------------------------------------------------------------------*/

struct _AnjutaPmChooserButtonPrivate
{
	AnjutaProjectNodeType child;
};

static void ianjuta_project_chooser_init (IAnjutaProjectChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnjutaPmChooserButton, anjuta_pm_chooser_button, ANJUTA_TYPE_TREE_COMBO_BOX,
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PROJECT_CHOOSER,
                                                ianjuta_project_chooser_init))

/* Helpers functions
 *---------------------------------------------------------------------------*/


/* Private functions
 *---------------------------------------------------------------------------*/

static gboolean
is_node_valid (GtkTreeModel *model, GtkTreeIter *iter, AnjutaPmChooserButton *button)
{
	GbfTreeData *data = NULL;
	gboolean valid = FALSE;

	gtk_tree_model_get (model, iter, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	if ((data != NULL) && (data->node != NULL))
	{
	  	AnjutaPmChooserButtonPrivate *priv;
		gint mask;

		priv = G_TYPE_INSTANCE_GET_PRIVATE (button,
		                                    ANJUTA_TYPE_PM_CHOOSER_BUTTON,
		                                    AnjutaPmChooserButtonPrivate);

		switch (priv->child)
		{
		case ANJUTA_PROJECT_ROOT:
			/* Allow all nodes */
			mask = -1;
			break;
		case ANJUTA_PROJECT_MODULE:
			/* Only module parent */
			mask = ANJUTA_PROJECT_CAN_ADD_MODULE;
			break;
		case ANJUTA_PROJECT_PACKAGE:
			/* Only package parent */
			mask = ANJUTA_PROJECT_CAN_ADD_PACKAGE;
			break;
		case ANJUTA_PROJECT_SOURCE:
			/* Only package parent */
			mask = ANJUTA_PROJECT_CAN_ADD_SOURCE;
			break;
		case ANJUTA_PROJECT_TARGET:
			/* Only package parent */
			mask = ANJUTA_PROJECT_CAN_ADD_TARGET;
			break;
		case ANJUTA_PROJECT_GROUP:
			/* Only group parent */
			mask = ANJUTA_PROJECT_CAN_ADD_GROUP;
			break;
		default:
			mask = 0;
			break;
		}

		valid = anjuta_project_node_get_state (ANJUTA_PROJECT_NODE (data->node)) & mask ? TRUE : FALSE;
	}

	return valid;
}

static void
setup_nodes_combo_box (AnjutaPmChooserButton      *view,
						GbfProjectModel	   *model,
                  	    GtkTreePath            *root,
						GtkTreeModelFilterVisibleFunc func,
						gpointer               data,
						GtkTreeIter            *selected)
{
	GtkTreeIter iter;

	g_return_if_fail (view != NULL);
	g_return_if_fail (model != NULL);

	if ((func != NULL) || (root != NULL))
	{
		GtkTreeModel *filter;

		filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (model), root);
		if (func != NULL)
		{
			gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), func, data, NULL);
		}
		anjuta_tree_combo_box_set_model (ANJUTA_TREE_COMBO_BOX (view), filter);
		g_object_unref (filter);
		if (pm_convert_project_iter_to_model_iter (filter, &iter, selected))
		{
			anjuta_tree_combo_box_set_active_iter (ANJUTA_TREE_COMBO_BOX (view), &iter);
		}
	}
	else
    {
		anjuta_tree_combo_box_set_model (ANJUTA_TREE_COMBO_BOX (view), GTK_TREE_MODEL (model));
		if (selected)
		{
			anjuta_tree_combo_box_set_active_iter (ANJUTA_TREE_COMBO_BOX (view), selected);
		}
	}
}

/* Callbacks functions
 *---------------------------------------------------------------------------*/

static gboolean
is_project_node_but_shortcut (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data;

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);

	return (data != NULL) && (data->shortcut == NULL) && (gbf_tree_data_get_node (data) != NULL);
}

static gboolean
is_project_module_node (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data;

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);
	if ((data != NULL) && (data->shortcut == NULL))
	{
		AnjutaProjectNode *node = gbf_tree_data_get_node (data);

		if ((node != NULL) && ((anjuta_project_node_get_node_type (node) & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_MODULE))
		{
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
is_project_target_or_group_node (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data;

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);
	if ((data != NULL) && (data->shortcut == NULL))
	{
		AnjutaProjectNode *node = gbf_tree_data_get_node (data);

		if (node != NULL)
		{
			AnjutaProjectNodeType type = anjuta_project_node_get_node_type (node) & ANJUTA_PROJECT_TYPE_MASK;

			return (type == ANJUTA_PROJECT_TARGET) || (type == ANJUTA_PROJECT_GROUP);
		}
	}

	return FALSE;
}

static gboolean
is_project_group_node (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data;

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);
	if ((data != NULL) && (data->shortcut == NULL))
	{
		AnjutaProjectNode *node = gbf_tree_data_get_node (data);

		if (node != NULL)
		{
			AnjutaProjectNodeType type = anjuta_project_node_get_node_type (node) & ANJUTA_PROJECT_TYPE_MASK;

			return type == ANJUTA_PROJECT_GROUP;
		}
	}

	return FALSE;
}


/* Public functions
 *---------------------------------------------------------------------------*/

GtkWidget *
anjuta_pm_chooser_button_new (void)
{
	return g_object_new (ANJUTA_TYPE_PM_CHOOSER_BUTTON, NULL);
}


/* Implement IAnjutaProjectChooser interface
 *---------------------------------------------------------------------------*/

typedef struct
{
	GtkTreeIter iter;
	gboolean found;
	AnjutaPmChooserButton *button;
}  SearchPacket;

static gboolean
anjuta_pm_chooser_is_node_valid (GtkTreeModel *model,
                                 GtkTreePath *path,
                                 GtkTreeIter *iter,
                                 gpointer data)
{
	SearchPacket *packet = (SearchPacket *)data;

	if (is_node_valid (model, iter, packet->button))
	{
		packet->iter = *iter;
		packet->found = TRUE;

		return TRUE;
	}

	return FALSE;
}

static gboolean
anjuta_pm_chooser_set_project_model (IAnjutaProjectChooser *iface, IAnjutaProjectManager *manager, AnjutaProjectNodeType child_type, GError **error)
{
	GtkTreeIter selected;
	gboolean found;
	GtkTreeModelFilterVisibleFunc func;
	const gchar *label;
	GbfProjectModel *model;

	ANJUTA_PM_CHOOSER_BUTTON (iface)->priv->child = child_type & ANJUTA_PROJECT_TYPE_MASK;

	switch (child_type & ANJUTA_PROJECT_TYPE_MASK)
	{
	case ANJUTA_PROJECT_ROOT:
		/* Display all nodes */
		func = is_project_node_but_shortcut;
		label = _("<Select any project node>");
		break;
	case ANJUTA_PROJECT_MODULE:
		/* Display all targets */
		func = is_project_target_or_group_node;
		label = _("<Select a target>");
		break;
	case ANJUTA_PROJECT_PACKAGE:
		/* Display all modules */
		func = is_project_module_node;
		label = _("<Select any module>");
		break;
	case ANJUTA_PROJECT_SOURCE:
		/* Display all targets */
		func = is_project_target_or_group_node;
		label = _("<Select a target>");
		break;
	case ANJUTA_PROJECT_TARGET:
		/* Display all targets */
		func = is_project_target_or_group_node;
		label = _("<Select a target or a folder>");
		break;
	case ANJUTA_PROJECT_GROUP:
		/* Display all groups */
		func = is_project_group_node;
		label = _("<Select a folder>");
		break;
	default:
		/* Invalid type */
		return FALSE;
	}

	anjuta_tree_combo_box_set_invalid_text (ANJUTA_TREE_COMBO_BOX (iface), label);
	anjuta_tree_combo_box_set_valid_function (ANJUTA_TREE_COMBO_BOX (iface), (GtkTreeModelFilterVisibleFunc)is_node_valid, iface, NULL);

	/* Find a default node */
	model = gbf_project_view_get_model(ANJUTA_PLUGIN_PROJECT_MANAGER (manager)->view);
	found = gbf_project_view_get_first_selected (ANJUTA_PLUGIN_PROJECT_MANAGER (manager)->view, &selected) != NULL;
	while (found && !is_node_valid (GTK_TREE_MODEL (model), &selected, ANJUTA_PM_CHOOSER_BUTTON (iface)))
	{
		GtkTreeIter iter;

		found = gtk_tree_model_iter_parent (GTK_TREE_MODEL (model), &iter, &selected);
		selected  = iter;
	}
	if (!found)
	{
		SearchPacket packet;

		packet.found = FALSE;
		packet.button = ANJUTA_PM_CHOOSER_BUTTON (iface);
		gtk_tree_model_foreach (GTK_TREE_MODEL (model), anjuta_pm_chooser_is_node_valid, &packet);

		if (packet.found)
		{
			found = packet.found;
			selected = packet.iter;
		}
	}

	/* Add combo node selection */
	setup_nodes_combo_box (ANJUTA_PM_CHOOSER_BUTTON (iface),
	                       model,
	                       NULL,
	                       func,
	                       NULL,
	                       found ? &selected : NULL);

	return TRUE;
}

static GFile *
anjuta_pm_chooser_get_selected (IAnjutaProjectChooser *iface, GError **error)
{
	GtkTreeIter iter;

	if (anjuta_tree_combo_box_get_active_iter (ANJUTA_TREE_COMBO_BOX (iface), &iter))
	{
		GtkTreeModel *model;

		model = anjuta_tree_combo_box_get_model (ANJUTA_TREE_COMBO_BOX (iface));

		if (is_node_valid (model, &iter, ANJUTA_PM_CHOOSER_BUTTON (iface)))
		{
			GbfTreeData *data;

			gtk_tree_model_get (model, &iter, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

			return anjuta_project_node_get_file (data->node);
		}
	}

	return NULL;
}

static void
ianjuta_project_chooser_init (IAnjutaProjectChooserIface *iface)
{
	iface->set_project_model = anjuta_pm_chooser_set_project_model;
	iface->get_selected = anjuta_pm_chooser_get_selected;
}

/* Implement GObject functions
 *---------------------------------------------------------------------------*/

static GObject *
anjuta_pm_chooser_button_constructor (GType                  type,
                                      guint                  n_construct_properties,
                                      GObjectConstructParam *construct_properties)
{
	GObject *object;

	object = G_OBJECT_CLASS (anjuta_pm_chooser_button_parent_class)->constructor
		(type, n_construct_properties, construct_properties);

	pm_setup_project_renderer (GTK_CELL_LAYOUT (object));

	return object;
}

static void
anjuta_pm_chooser_button_init (AnjutaPmChooserButton *button)
{
  	AnjutaPmChooserButtonPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE (button,
	                                    ANJUTA_TYPE_PM_CHOOSER_BUTTON,
	                                    AnjutaPmChooserButtonPrivate);
	button->priv = priv;
}

static void
anjuta_pm_chooser_button_class_init (AnjutaPmChooserButtonClass * klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	object_class->constructor = anjuta_pm_chooser_button_constructor;

	g_type_class_add_private (klass, sizeof (AnjutaPmChooserButtonPrivate));
}

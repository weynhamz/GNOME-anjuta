/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* gbf-tree-data.c
 *
 * Copyright (C) 2010  Sébastien Granjoux
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include "gbf-tree-data.h"

/**
 * The GbfTreeData object store all data needed by each element of the project
 * tree view. It has the same role than AnjutaProjectNode in the project
 * backend and normally there is one GbfTreeData for each AnjutaProjectNode.
 * 
 * The GbfTreeData objects do not have a pointer to their corresponding
 * AnjutaProjectNode because they do not have the same life time. By example
 * when a project is reloaded all AnjutaProjectNode are destroyed but the
 * GbfTreeData stay alive. They contain enough information to be able to
 * find the corresponding AnjutaProjectNode though, but it is a search so
 * it takes more time.
 *
 * Several GbfTreeData can correspond to the same AnjutaProjectNode because
 * the tree view can contain shortcuts on alreay existing  project node. Each
 * shortcut has its own GbfTreeData object containing the same data than the
 * root object.
 *
 * The GbfTreeData objects are not owned by the tree view which keeps only
 * pointer on them. When removing a element in the tree view, it is the
 * responsability of the routine to free all GbfTreeData objects before. On
 * the other hand, it must not be freed after getting one pointer from
 * the tree view.
 */ 

AnjutaProjectNode *
gbf_tree_data_get_node (GbfTreeData *data, IAnjutaProject *project)
{
	AnjutaProjectNode *node = NULL;
	
	if (data != NULL)
	{
		AnjutaProjectGroup *root = NULL;
		AnjutaProjectGroup *group = NULL;
		AnjutaProjectTarget *target = NULL;

		root = ianjuta_project_get_root (project, NULL);
		if ((root != NULL) && (data->group != NULL))
		{
			group = anjuta_project_group_get_node_from_file (root, data->group);
			node = group;
		}

		if ((group != NULL) && (data->target != NULL))
		{
			target = anjuta_project_target_get_node_from_name (group, data->target);
			node = target;
		}

		if (((group != NULL) || (target != NULL)) && (data->source != NULL))
		{
			node = anjuta_project_source_get_node_from_file (target != NULL ? target : group, data->source);
		}
	}

	return node;
}

gchar *
gbf_tree_data_get_uri (GbfTreeData *data)
{
	if (data->source != NULL)
	{
		return g_file_get_uri (data->source);
	}
	else if (data->target != NULL)
	{
		GFile *target;
		gchar *uri;

		target = g_file_get_child (data->group, data->target);
		uri = g_file_get_uri (target);
		g_object_unref (target);
		
		return uri;
	}
	else if (data->group != NULL)
	{
		return g_file_get_uri (data->group);
	}

	return NULL;
}

GbfTreeData *
gbf_tree_data_new_string (const gchar *string)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	
	data->type = GBF_TREE_NODE_STRING;
	data->name = g_strdup (string);
	
	return data;
}

GbfTreeData *
gbf_tree_data_new_shortcut (GbfTreeData *src)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);

	data->type = GBF_TREE_NODE_SHORTCUT;
	data->name = g_strdup (src->name);
	data->group = src->group == NULL ? NULL : g_object_ref (src->group);
	data->target = g_strdup (src->target);
	data->source = src->source == NULL ? NULL : g_object_ref (src->source);
	data->is_shortcut = TRUE;
	data->shortcut = src;
	src->shortcut = data;
	
	return data;
}

GbfTreeData *
gbf_tree_data_new_group (AnjutaProjectGroup *group)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	GFileInfo *ginfo;

	data->type = GBF_TREE_NODE_GROUP;
	
	ginfo = g_file_query_info (anjuta_project_group_get_directory (group),
	    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
	    G_FILE_QUERY_INFO_NONE,
	    NULL, NULL);
	if (ginfo)
	{
		data->name = g_strdup (g_file_info_get_display_name (ginfo));
	        g_object_unref(ginfo);
	}
	else
	{
		data->name = g_strdup ("?");
	}

	data->group = g_object_ref (anjuta_project_group_get_directory (group));
	
	return data;
}

GbfTreeData *
gbf_tree_data_new_target (AnjutaProjectTarget *target)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	AnjutaProjectGroup *group;
	
	data->type = GBF_TREE_NODE_TARGET;
	data->name = g_strdup (anjuta_project_target_get_name (target));

	group = (AnjutaProjectGroup *)anjuta_project_node_parent (target);
	data->group = g_object_ref (anjuta_project_group_get_directory (group));	
	data->target = g_strdup (anjuta_project_target_get_name (target));
	
	return data;
}

GbfTreeData *
gbf_tree_data_new_source (AnjutaProjectSource *source)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	GFileInfo *ginfo;
	AnjutaProjectNode *parent;
	
	data->type = GBF_TREE_NODE_SOURCE;

	data->source = g_object_ref (anjuta_project_source_get_file (source));
	
	ginfo = g_file_query_info (data->source,
	    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
	    G_FILE_QUERY_INFO_NONE,
	    NULL, NULL);
	if (ginfo)
	{
		data->name = g_strdup (g_file_info_get_display_name (ginfo));
	        g_object_unref(ginfo);
	}
	else
	{
		data->name = g_file_get_basename (data->source);
	}

	parent = anjuta_project_node_parent (source);
	if (anjuta_project_node_get_type (parent) == ANJUTA_PROJECT_GROUP)
	{
		data->group = g_object_ref (anjuta_project_group_get_directory (parent));
	}
	else
	{
		AnjutaProjectGroup *group;
		
		group = (AnjutaProjectGroup *)anjuta_project_node_parent (parent);
		data->group = g_object_ref (anjuta_project_group_get_directory (group));
		data->target = g_strdup (anjuta_project_target_get_name (parent));
	}

	return data;
}

void
gbf_tree_data_free (GbfTreeData *data)
{
	if (data)
	{
		g_free (data->name);
		if (data->group != NULL) g_object_unref (data->group);
		g_free (data->target);
		if (data->source != NULL) g_object_unref (data->source);
		if (data->shortcut) data->shortcut->shortcut = NULL;
		if (data->properties_dialog) gtk_widget_destroy (data->properties_dialog);
		g_slice_free (GbfTreeData, data);
	}
}

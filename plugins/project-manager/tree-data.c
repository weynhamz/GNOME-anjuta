/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* tree-data.c
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
#include "tree-data.h"

#include <string.h>

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

gchar *
gbf_tree_data_get_uri (GbfTreeData *data)
{
	return data->node ? g_file_get_uri (anjuta_project_node_get_file (data->node)) : NULL;
}

GFile *
gbf_tree_data_get_file (GbfTreeData *data)
{
	if (data->source != NULL)
	{
		return g_object_ref (g_file_get_uri (data->source));
	}
	else if (data->target != NULL)
	{
		GFile *target;

		target = g_file_get_child (data->group, data->target);

		return target;
	}
	else if (data->group != NULL)
	{
		return g_object_ref (g_file_get_uri (data->group));
	}

	return NULL;
}

gchar *
gbf_tree_data_get_path (GbfTreeData *data)
{
	return data->node ? g_file_get_path (anjuta_project_node_get_file (data->node)) : NULL;
}

const gchar *
gbf_tree_data_get_name (GbfTreeData *data)
{
	return data->node != NULL ? anjuta_project_node_get_name (data->node) : data->name;
}

AnjutaProjectNode *
gbf_tree_data_get_node (GbfTreeData *data)
{
	return data->node;
}

gboolean
gbf_tree_data_equal (GbfTreeData *data_a, GbfTreeData *data_b)
{
	gboolean equal;

	equal = data_a == data_b;
	if (!equal && (data_a != NULL) && (data_b != NULL))
	{
		equal = data_a->type == data_b->type;
		if (equal)
		{
			if ((data_a->group != NULL) && (data_b->group != NULL))
			{
				equal = g_file_equal (data_a->group, data_b->group);
			}

			if (equal)
			{
				if ((data_a->target != NULL) && (data_b->target != NULL))
				{
					equal = strcmp (data_a->target, data_b->target) == 0;
				}

				if (equal)
				{
					if ((data_a->source != NULL) && (data_b->source != NULL))
					{
						equal = g_file_equal (data_a->source, data_b->source);
					}
				}
			}
		}
		else if ((data_a->type == GBF_TREE_NODE_UNKNOWN) || (data_b->type == GBF_TREE_NODE_UNKNOWN))
		{
			equal = strcmp (data_b->name, data_a->name);
		}
	}

	return equal;
}

gboolean
gbf_tree_data_equal_file (GbfTreeData *data,  GbfTreeNodeType type, GFile *file)
{
	gboolean equal = FALSE;

	if (data != NULL)
	{
		AnjutaProjectNode *node = gbf_tree_data_get_node (data);

		if (node != NULL)
		{
			if ((type == GBF_TREE_NODE_UNKNOWN) || (type == data->type))
			{
				GFile* node_file = anjuta_project_node_get_file (node);
				if (node_file && g_file_equal (node_file, file))
				{
					equal = TRUE;
				}
			}
		}
	}

	return equal;
}

gboolean
gbf_tree_data_equal_name (GbfTreeData *data, const gchar *name)
{
	return g_strcmp0 (data->name, name) == 0;
}

GbfTreeNodeType
gbf_tree_node_type_from_project (AnjutaProjectNodeType type)
{
	GbfTreeNodeType tree_type;

	switch (type & ANJUTA_PROJECT_TYPE_MASK)
	{
	case ANJUTA_PROJECT_ROOT:
		tree_type = GBF_TREE_NODE_ROOT;
		break;
	case ANJUTA_PROJECT_GROUP:
		tree_type = GBF_TREE_NODE_GROUP;
		break;
	case ANJUTA_PROJECT_TARGET:
		tree_type = GBF_TREE_NODE_TARGET;
		break;
	case ANJUTA_PROJECT_SOURCE:
		tree_type = GBF_TREE_NODE_SOURCE;
		break;
	case ANJUTA_PROJECT_MODULE:
		tree_type = GBF_TREE_NODE_MODULE;
		break;
	case ANJUTA_PROJECT_PACKAGE:
		tree_type = GBF_TREE_NODE_PACKAGE;
		break;
	default:
		tree_type = GBF_TREE_NODE_UNKNOWN;
		break;
	}

	return tree_type;
}

void
gbf_tree_data_invalidate (GbfTreeData *data)
{
	data->type = GBF_TREE_NODE_INVALID;
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
	data->node = src->node;
	data->name = g_strdup (src->name);
	data->group = src->group == NULL ? NULL : g_object_ref (src->group);
	data->target = g_strdup (src->target);
	data->source = src->source == NULL ? NULL : g_object_ref (src->source);
	data->is_shortcut = TRUE;
	data->shortcut = src;
	//src->shortcut = data;

	return data;
}

GbfTreeData *
gbf_tree_data_new_proxy (const char *name, gboolean expanded)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);

	data->type = GBF_TREE_NODE_UNKNOWN;
	data->node = NULL;
	data->name = g_strdup (name);
	data->expanded = expanded;

	return data;
}

GbfTreeData *
gbf_tree_data_new_group (AnjutaProjectNode *group)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);

	data->type = anjuta_project_node_parent(group) == NULL ? GBF_TREE_NODE_ROOT : GBF_TREE_NODE_GROUP;
	data->node = group;

	data->name = g_strdup (anjuta_project_node_get_name (group));

	data->group = g_object_ref (anjuta_project_node_get_file (group));

	return data;
}

GbfTreeData *
gbf_tree_data_new_target (AnjutaProjectNode *target)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	AnjutaProjectNode *group;

	data->type = GBF_TREE_NODE_TARGET;
	data->node = target;
	data->name = g_strdup (anjuta_project_node_get_name (target));

	group = anjuta_project_node_parent (target);
	data->group = g_object_ref (anjuta_project_node_get_file (group));
	data->target = g_strdup (anjuta_project_node_get_name (target));

	return data;
}

GbfTreeData *
gbf_tree_data_new_object (AnjutaProjectNode *node)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	GFileInfo *ginfo;
	AnjutaProjectNode *parent;

	data->type = GBF_TREE_NODE_OBJECT;
	data->node = node;

	data->source = g_object_ref (anjuta_project_node_get_file (node));

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

	parent = anjuta_project_node_parent (node);
	if (anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_GROUP)
	{
		data->group = g_object_ref (anjuta_project_node_get_file (parent));
	}
	else if (anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_TARGET)
	{
		AnjutaProjectNode *group;

		group = anjuta_project_node_parent (parent);
		data->group = g_object_ref (anjuta_project_node_get_file (group));
		data->target = g_strdup (anjuta_project_node_get_name (parent));
	}

	return data;
}

GbfTreeData *
gbf_tree_data_new_source (AnjutaProjectNode *source)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	GFileInfo *ginfo;
	AnjutaProjectNode *parent;

	data->type = GBF_TREE_NODE_SOURCE;
	data->node = source;

	data->source = g_object_ref (anjuta_project_node_get_file (source));

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
	if (anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_GROUP)
	{
		data->group = g_object_ref (anjuta_project_node_get_file (parent));
	}
	else if (anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_TARGET)
	{
		AnjutaProjectNode *group;

		group = anjuta_project_node_parent (parent);
		data->group = g_object_ref (anjuta_project_node_get_file (group));
		data->target = g_strdup (anjuta_project_node_get_name (parent));
	}

	return data;
}

GbfTreeData *
gbf_tree_data_new_root (AnjutaProjectNode *root)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);

	data->type = GBF_TREE_NODE_ROOT;
	data->node = root;
	data->name = g_strdup (anjuta_project_node_get_name (root));

	return data;
}

GbfTreeData *
gbf_tree_data_new_module (AnjutaProjectNode *module)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);

	data->type = GBF_TREE_NODE_MODULE;
	data->node = module;
	data->name = g_strdup (anjuta_project_node_get_name (module));

	return data;
}

GbfTreeData *
gbf_tree_data_new_package (AnjutaProjectNode *package)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);

	data->type = GBF_TREE_NODE_PACKAGE;
	data->node = package;
	data->name = g_strdup (anjuta_project_node_get_name (package));

	return data;
}

GbfTreeData *
gbf_tree_data_new_node (AnjutaProjectNode *node)
{
	GbfTreeData *data = NULL;

	switch (anjuta_project_node_get_node_type (node))
	{
		case ANJUTA_PROJECT_GROUP:
			data = gbf_tree_data_new_group (node);
			break;
		case ANJUTA_PROJECT_TARGET:
			data = gbf_tree_data_new_target(node);
			break;
		case ANJUTA_PROJECT_OBJECT:
			data = gbf_tree_data_new_object (node);
			break;
		case ANJUTA_PROJECT_SOURCE:
			data = gbf_tree_data_new_source (node);
			break;
		case ANJUTA_PROJECT_MODULE:
			data = gbf_tree_data_new_module (node);
			break;
		case ANJUTA_PROJECT_PACKAGE:
			data = gbf_tree_data_new_package (node);
			break;
		case ANJUTA_PROJECT_ROOT:
			data = gbf_tree_data_new_root (node);
			break;
		default:
			break;
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
		//if (data->shortcut) data->shortcut->shortcut = NULL;
		if (data->properties_dialog) gtk_widget_destroy (data->properties_dialog);
		g_slice_free (GbfTreeData, data);
	}
}

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
	gchar *path;
	gchar *guri;
	gchar *suri;
	
	guri = data->group != NULL ? g_file_get_uri (data->group) : NULL;
	suri = data->source != NULL ? g_file_get_uri (data->source) : NULL;
	path = g_strconcat (guri, " ", data->target, " ", suri, NULL);
	g_free (suri);
	g_free (guri);

	return path;
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
		else
		{
			if (data_b->type == GBF_TREE_NODE_UNKNOWN)
			{
				GbfTreeNodeType type = data_a->type;

				data_a->type = data_b->type;
				data_b->type = type;
			}

			equal = data_a->type == GBF_TREE_NODE_UNKNOWN;
			if (equal)
			{
				if (data_b->source != NULL) 
				{
					equal = g_file_equal (data_a->group, data_b->source);
				}
				else if (data_b->target != NULL)
				{
					gchar *name;

					name = g_file_get_basename (data_a->group);
					equal = strcmp (name, data_b->target) == 0;
					g_free (name);
				}
				else if (data_b->group != NULL)
				{
					equal = g_file_equal (data_a->group, data_b->group);
				}
			}
		}
	}

	return equal;
}

GbfTreeData *
gbf_tree_data_new_for_path (const gchar *path)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	gchar **uris;

	uris = g_strsplit (path, " ", 3);

	if (uris != NULL)
	{
		if ((uris[0] != NULL) && (*uris[0] != '\0'))
		{
			data->group = g_file_new_for_uri (uris[0]);

			if ((uris[1] != NULL) && (*uris[1] != '\0'))
			{
				data->target = g_strdup (uris[1]);

				if ((uris[2] != NULL) && (*uris[2] != '\0'))
				{
					data->source = g_file_new_for_uri (uris[2]);
				}
			}
		}
	}
	
	if (data->source != NULL)
	{
		GFileInfo *ginfo;

		data->type = GBF_TREE_NODE_SOURCE;
		
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
	}
	else if (data->target != NULL)
	{
		data->type = GBF_TREE_NODE_TARGET;
		
		data->name = g_strdup (data->target);
	}
	else if (data->group != NULL)
	{
		GFileInfo *ginfo;

		data->type = GBF_TREE_NODE_GROUP;
		
		ginfo = g_file_query_info (data->group,
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
			data->name = g_file_get_basename (data->group);
		}
	}
	else
	{
		data->type = GBF_TREE_NODE_STRING;
		data->name = g_strdup ("?");
	}

	g_strfreev (uris);

	return data;
}

GbfTreeData *
gbf_tree_data_new_for_file (GFile *file, GbfTreeNodeType type)
{
	GbfTreeData *data = g_slice_new0 (GbfTreeData);
	GFileInfo *ginfo;

	data->type = type;

	switch (type)
	{
	case GBF_TREE_NODE_UNKNOWN:
	case GBF_TREE_NODE_SHORTCUT:
	case GBF_TREE_NODE_GROUP:
		data->group = g_object_ref (file);
		break;
	case GBF_TREE_NODE_TARGET:
		data->group = g_file_get_parent (file);
		data->target = g_file_get_basename (file);
		file = NULL;
		data->name = g_strdup (data->target);
		break;
	case GBF_TREE_NODE_SOURCE:
		data->source = g_object_ref (file);
		break;
	case GBF_TREE_NODE_STRING:
		data->name = g_file_get_parse_name (file);
		file = NULL;
		break;
	}

	if (file != NULL)
	{
		ginfo = g_file_query_info (file,
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
			data->name = g_file_get_basename (data->group);
		}
	}

	return data;
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
		data->name = g_file_get_basename (anjuta_project_group_get_directory (group));
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
	if (anjuta_project_node_type (parent) == ANJUTA_PROJECT_GROUP)
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

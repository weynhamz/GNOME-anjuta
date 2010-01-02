/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gbf-tree-data.c
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
 * Author: JP Rosevear
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include "gbf-tree-data.h"


GbfTreeData *
gbf_tree_data_new_string (const gchar *string)
{
	GbfTreeData *node = g_new0 (GbfTreeData, 1);
	
	node->type = GBF_TREE_NODE_STRING;
	node->name = g_strdup (string);
	
	return node;
}

GbfTreeData *
gbf_tree_data_new_group (IAnjutaProject *project, AnjutaProjectGroup *group)
{
	GbfTreeData *node = g_new0 (GbfTreeData, 1);
	GFileInfo *ginfo;

	node->type = GBF_TREE_NODE_GROUP;
	node->id = group;
	node->uri = g_file_get_uri (anjuta_project_group_get_directory (group));
	
	
	ginfo = g_file_query_info (anjuta_project_group_get_directory (group),
	    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
	    G_FILE_QUERY_INFO_NONE,
	    NULL, NULL);
	if (ginfo)
	{
		node->name = g_strdup (g_file_info_get_display_name (ginfo));
	        g_object_unref(ginfo);
	}
	else
	{
		node->name = g_strdup ("?");
	}

	return node;
}

GbfTreeData *
gbf_tree_data_new_target (IAnjutaProject *project, AnjutaProjectTarget *target)
{
	GbfTreeData *node = g_new0 (GbfTreeData, 1);
	AnjutaProjectGroup *group;
	
	node->type = GBF_TREE_NODE_TARGET;
	node->id = target;

	group = (AnjutaProjectGroup *)anjuta_project_node_parent (target);
	node->uri = g_file_get_uri (anjuta_project_group_get_directory (group));	
	
	node->name = g_strdup (anjuta_project_target_get_name (target));
	node->mime_type = g_strdup (anjuta_project_target_type_mime (anjuta_project_target_get_type (target)));
	
	return node;
}

GbfTreeData *
gbf_tree_data_new_source (IAnjutaProject *project, AnjutaProjectSource *source)
{
	GbfTreeData *node = g_new0 (GbfTreeData, 1);
	GFileInfo *ginfo;
	
	node->type = GBF_TREE_NODE_SOURCE;
	node->id = source;
	
	node->uri = g_file_get_uri (anjuta_project_source_get_file (source));

	ginfo = g_file_query_info (anjuta_project_source_get_file (source),
	    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
	    G_FILE_QUERY_INFO_NONE,
	    NULL, NULL);
	if (ginfo)
	{
		node->name = g_strdup (g_file_info_get_display_name (ginfo));
	        g_object_unref(ginfo);
	}
	else
	{
		node->name = g_file_get_basename (anjuta_project_source_get_file (source));
	}

	return node;
}

GbfTreeData *
gbf_tree_data_copy (GbfTreeData *src)
{
	GbfTreeData *node;

	node = g_new (GbfTreeData, 1);
	node->type = src->type;
	node->name = g_strdup (src->name);
	node->id = src->id;
	node->uri = g_strdup (src->uri);
	node->is_shortcut = src->is_shortcut;
	node->mime_type = g_strdup (src->mime_type);
	
	return node;
}

void
gbf_tree_data_free (GbfTreeData *node)
{
	if (node) {
		g_free (node->name);
		g_free (node->uri);
		g_free (node->mime_type);
		g_free (node);
	}
}

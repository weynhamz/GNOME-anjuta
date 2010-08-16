/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* tree-data.h
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

#ifndef _TREE_DATA_H_
#define _TREE_DATA_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-project.h>

G_BEGIN_DECLS

typedef struct _GbfTreeData GbfTreeData;

typedef enum {
	GBF_TREE_NODE_UNKNOWN,
	GBF_TREE_NODE_STRING,
	GBF_TREE_NODE_GROUP,
	GBF_TREE_NODE_TARGET,
	GBF_TREE_NODE_MODULE,
	GBF_TREE_NODE_PACKAGE,
	GBF_TREE_NODE_SOURCE,
	GBF_TREE_NODE_ROOT,
	GBF_TREE_NODE_SHORTCUT,
	GBF_TREE_NODE_INVALID
} GbfTreeNodeType;

struct _GbfTreeData
{
	GbfTreeNodeType	type;
	AnjutaProjectNode *node;
	gchar           *name;
	GFile			*group;
	gchar			*target;
	GFile			*source;
	gboolean		is_shortcut;
	GbfTreeData		*shortcut;
	GtkWidget		*properties_dialog;
};

gchar	      *gbf_tree_data_get_uri		    (GbfTreeData           *data);
GFile	      *gbf_tree_data_get_file		    (GbfTreeData           *data);
const gchar   *gbf_tree_data_get_name		    (GbfTreeData           *data);
AnjutaProjectNode *gbf_tree_data_get_node		(GbfTreeData           *data);
void 			gbf_tree_data_replace_node		(GbfTreeData           *data,
												 AnjutaProjectNode     *node);

gchar		  *gbf_tree_data_get_path		    (GbfTreeData           *data);

gboolean       gbf_tree_data_equal              (GbfTreeData           *data_a,
                                                 GbfTreeData           *data_b);
gboolean       gbf_tree_data_equal_file         (GbfTreeData           *data,
                                                 AnjutaProjectNodeType type,
                                                 GFile                 *file);

GbfTreeData   *gbf_tree_data_new_string         (const gchar           *string);
GbfTreeData   *gbf_tree_data_new_shortcut       (GbfTreeData		   *src);
GbfTreeData   *gbf_tree_data_new_group          (AnjutaProjectNode     *group);
GbfTreeData   *gbf_tree_data_new_target         (AnjutaProjectNode     *target);
GbfTreeData   *gbf_tree_data_new_source         (AnjutaProjectNode     *source);
GbfTreeData   *gbf_tree_data_new_module         (AnjutaProjectNode     *module);
GbfTreeData   *gbf_tree_data_new_package        (AnjutaProjectNode     *package);
void				gbf_tree_data_invalidate (GbfTreeData *data);
void           gbf_tree_data_free               (GbfTreeData           *data);


G_END_DECLS

#endif /* _TREE_DATA_H_ */

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gbf-tree-data.h
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

#ifndef _GBF_TREE_DATA_H_
#define _GBF_TREE_DATA_H_

#include <glib-object.h>
#include <libanjuta/gbf-project.h>

G_BEGIN_DECLS

#define GBF_TYPE_TREE_DATA			(gbf_tree_data_get_type ())

typedef struct _GbfTreeData GbfTreeData;

typedef enum {
	GBF_TREE_NODE_STRING,
	GBF_TREE_NODE_GROUP,
	GBF_TREE_NODE_TARGET,
	GBF_TREE_NODE_TARGET_SOURCE,
} GbfTreeNodeType;

struct _GbfTreeData
{
	GbfTreeNodeType  type;
	gchar           *name;
	gchar           *id;
	gchar           *uri;
	gboolean         is_shortcut;
	gchar           *mime_type;
};

GType          gbf_tree_data_get_type            (void);
GbfTreeData   *gbf_tree_data_new_string          (const gchar                  *string);
GbfTreeData   *gbf_tree_data_new_group           (GbfProject                   *project,
						  const GbfProjectGroup        *group);
GbfTreeData   *gbf_tree_data_new_target          (GbfProject                   *project,
						  const GbfProjectTarget       *target);
GbfTreeData   *gbf_tree_data_new_source          (GbfProject                   *project,
						  const GbfProjectTargetSource *source);
GbfTreeData   *gbf_tree_data_copy                (GbfTreeData                  *data);
void           gbf_tree_data_free                (GbfTreeData                  *data);


G_END_DECLS

#endif /* _GBF_TREE_DATA_H_ */

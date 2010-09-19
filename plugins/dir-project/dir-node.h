/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* dir-node.h
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _DIR_NODE_H_
#define _DIR_NODE_H_

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_DIR_ROOT_NODE				(anjuta_dir_root_node_get_type ())
#define ANJUTA_DIR_ROOT_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DIR_ROOT_NODE, AnjutaDirRootNode))

typedef struct _AnjutaDirRootNode AnjutaDirRootNode;
GType anjuta_dir_root_node_get_type (void) G_GNUC_CONST;


#define ANJUTA_TYPE_DIR_GROUP_NODE			(anjuta_dir_group_node_get_type ())
#define ANJUTA_DIR_GROUP_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DIR_GROUP_NODE, AnjutaDirGroupNode))

typedef struct _AnjutaDirGroupNode AnjutaDirGroupNode;
GType anjuta_dir_group_node_get_type (void) G_GNUC_CONST;



#define ANJUTA_TYPE_DIR_SOURCE_NODE			(anjuta_dir_source_node_get_type ())
#define ANJUTA_DIR_SOURCE_NODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DIR_SOURCE_NODE, AnjutaDirSourceNode))

typedef struct _AnjutaDirSourceNode AnjutaDirSourceNode;
GType anjuta_dir_source_node_get_type (void) G_GNUC_CONST;



AnjutaProjectNode* dir_root_node_new (GFile *file);

AnjutaProjectNode *dir_group_node_new (GFile *file, GObject *emitter);

AnjutaProjectNode *dir_source_node_new (GFile *file);


G_END_DECLS

#endif /* _DIR_NODE_H_ */

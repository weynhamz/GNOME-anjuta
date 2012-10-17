/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-node.h
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

#ifndef _AMP_NODE_H_
#define _AMP_NODE_H_

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

/* Type macros
 *---------------------------------------------------------------------------*/

#define AMP_TYPE_NODE                   (amp_node_get_type ())
#define AMP_NODE(object)                (G_TYPE_CHECK_INSTANCE_CAST ((object), AMP_TYPE_NODE, AmpNode))
#define AMP_NODE_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), AMP_TYPE_NODE, AmpNodeClass))
#define AMP_IS_NODE(object)             (G_TYPE_CHECK_INSTANCE_TYPE ((object), AMP_TYPE_NODE))
#define AMP_IS_NODE_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), AMP_TYPE_NODE))
#define AMP_NODE_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), AMP_TYPE_NODE, AmpNodeClass))


typedef struct _AmpNode                 AmpNode;
typedef struct _AmpNodeClass            AmpNodeClass;

typedef struct _AmpProject        AmpProject;

/**
 * AmpNode:
 *
 * An object representing a autotool project node, by example a directory, a target or a source file.
 */
struct _AmpNode
{
	AnjutaProjectNode   parent;
};

struct _AmpNodeClass
{
	AnjutaProjectNodeClass  parent_class;

	gboolean					(*load)					(AmpNode *node,
											                AmpNode *parent,
											                AmpProject *project,
											                GError **error);

	gboolean					(*save)					(AmpNode *node,
											                AmpNode *parent,
											                AmpProject *project,
											                GError **error);

	gboolean					(*update)				(AmpNode *node,
											                AmpNode *parent);

	AmpNode*				    (*copy)				    (AmpNode *node);

	gboolean					(*erase)				(AmpNode *node,
											                AmpNode *parent,
											                AmpProject *project,
											                GError **error);

	gboolean					(*write)				(AmpNode *node,
											                AmpNode *parent,
											                AmpProject *project,
											                GError **error);
};

GType amp_node_get_type (void) G_GNUC_CONST;


void				amp_set_error (GError **error, gint code, const gchar *message);


AnjutaProjectNode * amp_node_new_valid				(AnjutaProjectNode *parent,
				                                 AnjutaProjectNodeType type,
				                                 GFile *file,
				                                 const gchar *name,
				                                 GError **error);

AnjutaProjectNode * amp_node_new_valid		(AnjutaProjectNode *parent,
				                                 AnjutaProjectNodeType type,
				                                 GFile *file,
				                                 const gchar *name,
				                                 GError **error);

gboolean						amp_node_load					(AmpNode *node,
											                      AmpNode *parent,
											                      AmpProject *project,
											                      GError **error);

gboolean						amp_node_save					(AmpNode *node,
											                      AmpNode *parent,
											                      AmpProject *project,
											                      GError **error);

gboolean						amp_node_update					(AmpNode *node,
											                      AmpNode *new_node);

AmpNode*						amp_node_copy					(AmpNode *node);

gboolean						amp_node_write					(AmpNode *node,
											                       AmpNode *parent,
											                       AmpProject *project,
											                       GError **error);

gboolean						amp_node_erase					(AmpNode *node,
											                       AmpNode *parent,
											                       AmpProject *project,
											                       GError **error);

void amp_node_register (GTypeModule *module);

G_END_DECLS

#endif /* _AMP_NODE_H_ */

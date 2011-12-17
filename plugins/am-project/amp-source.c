/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-source.c
 *
 * Copyright (C) 2009  Sébastien Granjoux
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

#include "amp-source.h"

#include "amp-node.h"
#include "am-scanner.h"
#include "am-properties.h"
#include "am-writer.h"


#include <libanjuta/interfaces/ianjuta-project.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>

/* Types
 *---------------------------------------------------------------------------*/

struct _AmpSourceNode {
	AnjutaProjectNode base;
	AnjutaToken* token;
};




/* Source object
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_source_node_get_token (AmpSourceNode *node)
{
	return node->token;
}

void
amp_source_node_add_token (AmpSourceNode *node, AnjutaToken *token)
{
	node->token = token;
}

void
amp_source_node_update_node (AmpSourceNode *node, AmpSourceNode *new_node)
{
	node->token = new_node->token;
}

AnjutaProjectNode*
amp_source_node_new (GFile *file, AnjutaProjectNodeType type)
{
	AmpSourceNode *node = NULL;

	node = g_object_new (AMP_TYPE_SOURCE_NODE, NULL);
	node->base.file = g_object_ref (file);
	node->base.type = ANJUTA_PROJECT_SOURCE | type;

	return ANJUTA_PROJECT_NODE (node);
}

AnjutaProjectNode*
amp_source_node_new_valid (GFile *file, AnjutaProjectNodeType type, GError **error)
{
	return amp_source_node_new (file, type);
}

gboolean
amp_source_node_set_file (AmpSourceNode *source, GFile *new_file)
{
	g_object_unref (source->base.file);
	source->base.file = g_object_ref (new_file);

	return TRUE;
}

void
amp_source_node_free (AmpSourceNode *node)
{
	g_object_unref (G_OBJECT (node));
}

/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_source_node_update (AmpNode *node, AmpNode *new_node)
{
	amp_source_node_update_node (AMP_SOURCE_NODE (node), AMP_SOURCE_NODE (new_node));

	return TRUE;
}

static gboolean
amp_source_node_write (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_source_node_create_token (project, AMP_SOURCE_NODE (node), error);
}

static gboolean
amp_source_node_erase (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_source_node_delete_token (project, AMP_SOURCE_NODE (node), error);
}




/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AmpSourceNodeClass AmpSourceNodeClass;

struct _AmpSourceNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpSourceNode, amp_source_node, AMP_TYPE_NODE);

static void
amp_source_node_init (AmpSourceNode *node)
{
	node->base.type = ANJUTA_PROJECT_SOURCE;
	node->base.properties_info = amp_get_source_property_list();
	node->base.state = ANJUTA_PROJECT_CAN_REMOVE;
	node->token = NULL;
}

static void
amp_source_node_finalize (GObject *object)
{
	AmpSourceNode *node = AMP_SOURCE_NODE (object);

	g_list_foreach (node->base.properties, (GFunc)amp_property_free, NULL);
	G_OBJECT_CLASS (amp_source_node_parent_class)->finalize (object);
}

static void
amp_source_node_class_init (AmpSourceNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AmpNodeClass* node_class;

	object_class->finalize = amp_source_node_finalize;

	node_class = AMP_NODE_CLASS (klass);
	node_class->update = amp_source_node_update;
	node_class->write = amp_source_node_write;
	node_class->erase = amp_source_node_erase;
}

static void
amp_source_node_class_finalize (AmpSourceNodeClass *klass)
{
}

void
amp_source_node_register (GTypeModule *module)
{
	amp_source_node_register_type (module);
}


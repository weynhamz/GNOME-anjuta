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


#include <libanjuta/interfaces/ianjuta-project.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>

/* Types
 *---------------------------------------------------------------------------*/

struct _AnjutaAmSourceNode {
	AnjutaProjectNode base;
	AnjutaToken* token;	
};




/* Source object
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_source_get_token (AnjutaAmSourceNode *node)
{
	return node->token;
}

void
amp_source_add_token (AnjutaAmSourceNode *node, AnjutaToken *token)
{
	node->token = token;
}

void
amp_source_update_node (AnjutaAmSourceNode *node, AnjutaAmSourceNode *new_node)
{
	node->token = new_node->token;
}

AnjutaProjectNode*
amp_source_new (GFile *file, GError **error)
{
	AnjutaAmSourceNode *node = NULL;

	node = g_object_new (ANJUTA_TYPE_AM_SOURCE_NODE, NULL);
	node->base.file = g_object_ref (file);

	return ANJUTA_PROJECT_NODE (node);
}

gboolean
amp_source_set_file (AnjutaAmSourceNode *source, GFile *new_file)
{
	g_object_unref (source->base.file);
	source->base.file = g_object_ref (new_file);

	return TRUE;
}

void
amp_source_free (AnjutaAmSourceNode *node)
{
	g_object_unref (G_OBJECT (node));
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaAmSourceNodeClass AnjutaAmSourceNodeClass;

struct _AnjutaAmSourceNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AnjutaAmSourceNode, anjuta_am_source_node, AMP_TYPE_NODE);

static void
anjuta_am_source_node_init (AnjutaAmSourceNode *node)
{
	node->base.type = ANJUTA_PROJECT_SOURCE;
	node->base.native_properties = amp_get_source_property_list();
	node->base.state = ANJUTA_PROJECT_CAN_REMOVE;
	node->token = NULL;
}

static void
anjuta_am_source_node_finalize (GObject *object)
{
	AnjutaAmSourceNode *node = ANJUTA_AM_SOURCE_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	G_OBJECT_CLASS (anjuta_am_source_node_parent_class)->finalize (object);
}

static void
anjuta_am_source_node_class_init (AnjutaAmSourceNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_source_node_finalize;
}

static void
anjuta_am_source_node_class_finalize (AnjutaAmSourceNodeClass *klass)
{
}

void
anjuta_am_source_node_register (GTypeModule *module)
{
	anjuta_am_source_node_register_type (module);
}


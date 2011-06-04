/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-object.c
 *
 * Copyright (C) 2011  Sébastien Granjoux
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

#include "amp-object.h"

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

struct _AmpObjectNode {
	AnjutaProjectNode base;
};




/* Object object
 *---------------------------------------------------------------------------*/

AnjutaProjectNode*
amp_object_node_new (GFile *file)
{
	AmpObjectNode *node = NULL;

	node = g_object_new (AMP_TYPE_OBJECT_NODE, NULL);
	node->base.file = g_object_ref (file);

	return ANJUTA_PROJECT_NODE (node);
}

AnjutaProjectNode*
amp_object_node_new_valid (GFile *file, GError **error)
{
	return amp_object_node_new (file);
}

void
amp_object_node_free (AmpObjectNode *node)
{
	g_object_unref (G_OBJECT (node));
}

/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_object_node_update (AmpNode *node, AmpNode *new_node)
{

	return TRUE;
}

static gboolean
amp_object_node_write (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return TRUE;
}

static gboolean
amp_object_node_erase (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return TRUE;
}




/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AmpObjectNodeClass AmpObjectNodeClass;

struct _AmpObjectNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpObjectNode, amp_object_node, AMP_TYPE_NODE);

static void
amp_object_node_init (AmpObjectNode *node)
{
	node->base.type = ANJUTA_PROJECT_OBJECT;
	node->base.native_properties = NULL;
	node->base.state = 0;
}

static void
amp_object_node_finalize (GObject *object)
{
	AmpObjectNode *node = AMP_OBJECT_NODE (object);

	G_OBJECT_CLASS (amp_object_node_parent_class)->finalize (object);
}

static void
amp_object_node_class_init (AmpObjectNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AmpNodeClass* node_class;
	
	object_class->finalize = amp_object_node_finalize;

	node_class = AMP_NODE_CLASS (klass);
	node_class->update = amp_object_node_update;
	node_class->write = amp_object_node_write;
	node_class->erase = amp_object_node_erase;	
}

static void
amp_object_node_class_finalize (AmpObjectNodeClass *klass)
{
}

void
amp_object_node_register (GTypeModule *module)
{
	amp_object_node_register_type (module);
}


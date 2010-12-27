/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-root.c
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "amp-root.h"

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





/* Root object
 *---------------------------------------------------------------------------*/

gboolean
amp_root_node_set_file (AmpRootNode *root, GFile *new_file)
{
	if (root->base.file != NULL) g_object_unref (root->base.file);
	root->base.file = new_file != NULL ? g_object_ref (new_file) : NULL;

	return TRUE;
}

AnjutaProjectNode*
amp_root_node_new (GFile *file, GError **error)
{
	AmpRootNode *node = NULL;

	node = g_object_new (AMP_TYPE_ROOT_NODE, NULL);
	node->base.file = g_object_ref (file);

	return ANJUTA_PROJECT_NODE (node);
}

void
amp_root_node_free (AmpRootNode *node)
{
	g_object_unref (G_OBJECT (node));
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

G_DEFINE_DYNAMIC_TYPE (AmpRootNode, amp_root_node, AMP_TYPE_NODE);

static void
amp_root_node_init (AmpRootNode *node)
{
	node->base.type = ANJUTA_PROJECT_ROOT;
	node->base.native_properties = amp_get_project_property_list();
	node->base.state = ANJUTA_PROJECT_CAN_ADD_GROUP |
						ANJUTA_PROJECT_CAN_ADD_PACKAGE,
						ANJUTA_PROJECT_CAN_SAVE;
}

static void
amp_root_node_finalize (GObject *object)
{
	AmpRootNode *node = AMP_ROOT_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	G_OBJECT_CLASS (amp_root_node_parent_class)->finalize (object);
}

static void
amp_root_node_class_init (AmpRootNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = amp_root_node_finalize;
}

static void
amp_root_node_class_finalize (AmpRootNodeClass *klass)
{
}

void
amp_root_node_register (GTypeModule *module)
{
	amp_root_node_register_type (module);
}


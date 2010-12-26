/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-package.c
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

#include "amp-package.h"

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

struct _AmpPackageNode {
	AnjutaProjectNode base;
	gchar *version;
	AnjutaToken *token;
};



/* Package objects
 *---------------------------------------------------------------------------*/

AmpPackageNode*
amp_package_node_new (const gchar *name, GError **error)
{
	AmpPackageNode *node = NULL;

	node = g_object_new (AMP_TYPE_PACKAGE_NODE, NULL);
	node->base.name = g_strdup (name);

	return node;
}

void
amp_package_node_free (AmpPackageNode *node)
{
	g_object_unref (G_OBJECT (node));
}

void
amp_package_node_set_version (AmpPackageNode *node, const gchar *compare, const gchar *version)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail ((version == NULL) || (compare != NULL));

	g_free (node->version);
	node->version = version != NULL ? g_strconcat (compare, version, NULL) : NULL;
}

AnjutaToken *
amp_package_node_get_token (AmpPackageNode *node)
{
	return node->token;
}

void
amp_package_node_add_token (AmpPackageNode *node, AnjutaToken *token)
{
	node->token = token;
}

void
amp_package_node_update_node (AmpPackageNode *node, AmpPackageNode *new_node)
{
	g_return_if_fail (new_node != NULL);	

	node->token = new_node->token;
	g_free (node->version);
	node->version = new_node->version;
	new_node->version = NULL;
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AmpPackageNodeClass AmpPackageNodeClass;

struct _AmpPackageNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpPackageNode, amp_package_node, AMP_TYPE_NODE);

static void
amp_package_node_init (AmpPackageNode *node)
{
	node->base.type = ANJUTA_PROJECT_PACKAGE;
	node->base.native_properties = amp_get_package_property_list();
	node->base.state =  ANJUTA_PROJECT_CAN_REMOVE;
	node->version = NULL;
}

static void
amp_package_node_finalize (GObject *object)
{
	AmpPackageNode *node = AMP_PACKAGE_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	
	G_OBJECT_CLASS (amp_package_node_parent_class)->finalize (object);
}

static void
amp_package_node_class_init (AmpPackageNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = amp_package_node_finalize;
}

static void
amp_package_node_class_finalize (AmpPackageNodeClass *klass)
{
}

void
amp_package_node_register (GTypeModule *module)
{
	amp_package_node_register_type (module);
}

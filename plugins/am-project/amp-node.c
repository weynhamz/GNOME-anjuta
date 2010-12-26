/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-node.c
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

#include "amp-node.h"
#include "am-scanner.h"
#include "am-properties.h"


#include <libanjuta/interfaces/ianjuta-project.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>



/* GObjet implementation
 *---------------------------------------------------------------------------*/

G_DEFINE_DYNAMIC_TYPE (AmpNode, amp_node, ANJUTA_TYPE_PROJECT_NODE);

static void
amp_node_init (AmpNode *node)
{
}

static void
amp_node_finalize (GObject *object)
{
	AmpNode *node = AMP_NODE (object);

	//g_list_foreach (node->parent.custom_properties, (GFunc)amp_property_free, NULL);
	
	G_OBJECT_CLASS (amp_node_parent_class)->finalize (object);
}

static void
amp_node_class_init (AmpNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = amp_node_finalize;
}

static void
amp_node_class_finalize (AmpNodeClass *klass)
{
}


void
amp_node_register (GTypeModule *module)
{
	amp_node_register_type (module);
}

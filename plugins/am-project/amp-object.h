/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-object.h
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
 */

#ifndef _AMP_OBJECT_H_
#define _AMP_OBJECT_H_

#include "am-project-private.h"
#include "am-scanner.h"

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

/* Type macros
 *---------------------------------------------------------------------------*/

#define AMP_TYPE_OBJECT_NODE						(amp_object_node_get_type ())
#define AMP_OBJECT_NODE(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj), AMP_TYPE_OBJECT_NODE, AmpObjectNode))

GType amp_object_node_get_type (void) G_GNUC_CONST;



void amp_object_node_register (GTypeModule *module);

AnjutaProjectNode* amp_object_node_new (GFile *file, AnjutaProjectNodeType type);
AnjutaProjectNode* amp_object_node_new_valid (GFile *file, AnjutaProjectNodeType type, GError **error);

void amp_object_node_free (AmpObjectNode *node);


G_END_DECLS

#endif /* _AMP_OBJECT_H_ */

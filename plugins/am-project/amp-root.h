/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-root.h
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

#ifndef _AMP_ROOT_H_
#define _AMP_ROOT_H_

#include	"amp-node.h"
#include	"amp-group.h"

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

/* Type macros
 *---------------------------------------------------------------------------*/

#define AMP_TYPE_ROOT_NODE						(amp_root_node_get_type ())
#define AMP_ROOT_NODE(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj), AMP_TYPE_ROOT_NODE, AmpRootNode))


GType amp_root_node_get_type (void) G_GNUC_CONST;

typedef struct _AmpRootNode			AmpRootNode;
typedef struct _AmpRootNodeClass	AmpRootNodeClass;

struct _AmpRootNode {
	AmpGroupNode base;
};

struct _AmpRootNodeClass {
	AmpGroupNodeClass parent_class;
};

void amp_root_node_register (GTypeModule *module);

AnjutaProjectNode* amp_root_node_new (GFile *file);
AnjutaProjectNode* amp_root_node_new_valid (GFile *file, GError **error);
void amp_root_node_free (AmpRootNode *node);
gboolean amp_root_node_set_file (AmpRootNode *source, GFile *new_file);


G_END_DECLS

#endif /* _AMP_ROOT_H_ */

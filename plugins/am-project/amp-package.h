/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-package.h
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

#ifndef _AMP_PACKAGE_H_
#define _AMP_PACKAGE_H_

#include "am-project.h"

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

/* Type macros
 *---------------------------------------------------------------------------*/

#define ANJUTA_TYPE_AM_PACKAGE_NODE			(anjuta_am_package_node_get_type ())
#define ANJUTA_AM_PACKAGE_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_PACKAGE_NODE, AnjutaAmPackageNode))

GType anjuta_am_package_node_get_type (void) G_GNUC_CONST;


void anjuta_am_package_node_register (GTypeModule *module);

AnjutaAmPackageNode* amp_package_new (const gchar *name, GError **error);
void amp_package_free (AnjutaAmPackageNode *node);
void amp_package_set_version (AnjutaAmPackageNode *node, const gchar *compare, const gchar *version);
AnjutaToken *amp_package_get_token (AnjutaAmPackageNode *node);
void amp_package_add_token (AnjutaAmPackageNode *node, AnjutaToken *token);
void amp_package_update_node (AnjutaAmPackageNode *node, AnjutaAmPackageNode *new_node);

G_END_DECLS

#endif /* _AMP_PACKAGE_H_ */

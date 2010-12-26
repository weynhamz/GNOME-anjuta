/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-group.h
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

#ifndef _AMP_GROUP_H_
#define _AMP_GROUP_H_

#include "am-project-private.h"
#include "am-scanner.h"

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

/* Type macros
 *---------------------------------------------------------------------------*/

#define ANJUTA_TYPE_AM_GROUP_NODE			(anjuta_am_group_node_get_type ())
#define ANJUTA_AM_GROUP_NODE(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_AM_GROUP_NODE, AnjutaAmGroupNode))

GType anjuta_am_group_node_get_type (void) G_GNUC_CONST;

typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} AmpGroupTokenCategory;

typedef struct _AmpVariable AmpVariable;

struct _AmpVariable {
	gchar *name;
	AnjutaTokenType assign;
	AnjutaToken *value;
};

void anjuta_am_group_node_register (GTypeModule *module);

AmpVariable* amp_variable_new (gchar *name, AnjutaTokenType assign, AnjutaToken *value);


void amp_group_add_token (AnjutaAmGroupNode *group, AnjutaToken *token, AmpGroupTokenCategory category);
GList * amp_group_get_token (AnjutaAmGroupNode *group, AmpGroupTokenCategory category);
AnjutaToken* amp_group_get_first_token (AnjutaAmGroupNode *group, AmpGroupTokenCategory category);
void amp_group_set_dist_only (AnjutaAmGroupNode *group, gboolean dist_only);
AnjutaTokenFile* amp_group_set_makefile (AnjutaAmGroupNode *group, GFile *makefile, AmpProject *project);
AnjutaToken* amp_group_get_makefile_token (AnjutaAmGroupNode *group);
AnjutaTokenFile *amp_group_get_make_token_file (AnjutaAmGroupNode *group);
gchar *amp_group_get_makefile_name (AnjutaAmGroupNode *group);
gboolean amp_group_update_makefile (AnjutaAmGroupNode *group, AnjutaToken *token);
void amp_group_update_variable (AnjutaAmGroupNode *group, AnjutaToken *variable);
AnjutaToken* amp_group_get_variable_token (AnjutaAmGroupNode *group, AnjutaToken *variable);
AnjutaAmGroupNode* amp_group_new (GFile *file, gboolean dist_only, GError **error);
void amp_group_free (AnjutaAmGroupNode *node);
void amp_group_update_node (AnjutaAmGroupNode *node, AnjutaAmGroupNode *new_node);
gboolean amp_group_set_file (AnjutaAmGroupNode *group, GFile *new_file);


G_END_DECLS

#endif /* _AMP_GROUP_H_ */

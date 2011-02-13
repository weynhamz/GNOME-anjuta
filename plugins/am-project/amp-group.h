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

#define AMP_TYPE_GROUP_NODE					   (amp_group_node_get_type ())
#define AMP_GROUP_NODE(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), AMP_TYPE_GROUP_NODE, AmpGroupNode))

GType amp_group_node_get_type (void) G_GNUC_CONST;

typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} AmpGroupNodeTokenCategory;

typedef struct _AmpVariable AmpVariable;

struct _AmpVariable {
	gchar *name;
	AnjutaTokenType assign;
	AnjutaToken *value;
};

void amp_group_node_register (GTypeModule *module);

AmpVariable* amp_variable_new (gchar *name, AnjutaTokenType assign, AnjutaToken *value);


void amp_group_node_add_token (AmpGroupNode *group, AnjutaToken *token, AmpGroupNodeTokenCategory category);
GList * amp_group_node_get_token (AmpGroupNode *group, AmpGroupNodeTokenCategory category);
GList * amp_group_node_get_all_token (AmpGroupNode *group);
AnjutaToken* amp_group_node_get_first_token (AmpGroupNode *group, AmpGroupNodeTokenCategory category);
void amp_group_node_set_dist_only (AmpGroupNode *group, gboolean dist_only);
AnjutaTokenFile* amp_group_node_set_makefile (AmpGroupNode *group, GFile *makefile, AmpProject *project);
AnjutaToken* amp_group_node_get_makefile_token (AmpGroupNode *group);
AnjutaTokenFile *amp_group_node_get_make_token_file (AmpGroupNode *group);
gchar *amp_group_node_get_makefile_name (AmpGroupNode *group);
gboolean amp_group_node_update_makefile (AmpGroupNode *group, AnjutaToken *token);
void amp_group_node_update_variable (AmpGroupNode *group, AnjutaToken *variable);
AnjutaToken* amp_group_node_get_variable_token (AmpGroupNode *group, AnjutaToken *variable);
AmpGroupNode* amp_group_node_new (GFile *file, gboolean dist_only, GError **error);
void amp_group_node_free (AmpGroupNode *node);
void amp_group_node_update_node (AmpGroupNode *node, AmpGroupNode *new_node);
gboolean amp_group_node_set_file (AmpGroupNode *group, GFile *new_file);


G_END_DECLS

#endif /* _AMP_GROUP_H_ */

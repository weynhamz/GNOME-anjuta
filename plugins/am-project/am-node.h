/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-node.h
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

#ifndef _AM_NODE_H_
#define _AM_NODE_H_

#include "am-project-private.h"
#include "am-scanner.h"

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

typedef struct _AmpVariable AmpVariable;

struct _AmpVariable {
	gchar *name;
	AnjutaTokenType assign;
	AnjutaToken *value;
};

AmpVariable* amp_variable_new (gchar *name, AnjutaTokenType assign, AnjutaToken *value);


AnjutaAmRootNode* amp_root_new (GFile *file, GError **error);
void amp_root_free (AnjutaAmRootNode *node);
void amp_root_clear (AnjutaAmRootNode *node);
AnjutaTokenFile* amp_root_set_configure (AnjutaAmRootNode *node, GFile *configure);
gboolean amp_root_update_configure (AnjutaAmRootNode *group, AnjutaToken *token);
AnjutaToken* amp_root_get_configure_token (AnjutaAmRootNode *root);

AnjutaAmModuleNode* amp_module_new (const gchar *name, GError **error);
void amp_module_free (AnjutaAmModuleNode *node);
AnjutaToken *amp_module_get_token (AnjutaAmModuleNode *node);
void amp_module_add_token (AnjutaAmModuleNode *group, AnjutaToken *token);

AnjutaAmPackageNode* amp_package_new (const gchar *name, GError **error);
void amp_package_free (AnjutaAmPackageNode *node);
void amp_package_set_version (AnjutaAmPackageNode *node, const gchar *compare, const gchar *version);
AnjutaToken *amp_package_get_token (AnjutaAmPackageNode *node);
void amp_package_add_token (AnjutaAmPackageNode *node, AnjutaToken *token);


void amp_group_add_token (AnjutaAmGroupNode *group, AnjutaToken *token, AmpGroupTokenCategory category);
GList * amp_group_get_token (AnjutaAmGroupNode *group, AmpGroupTokenCategory category);
AnjutaToken* amp_group_get_first_token (AnjutaAmGroupNode *group, AmpGroupTokenCategory category);
void amp_group_set_dist_only (AnjutaAmGroupNode *group, gboolean dist_only);
AnjutaTokenFile* amp_group_set_makefile (AnjutaAmGroupNode *group, GFile *makefile, GObject* project);
AnjutaToken* amp_group_get_makefile_token (AnjutaAmGroupNode *group);
gchar *amp_group_get_makefile_name (AnjutaAmGroupNode *group);
gboolean amp_group_update_makefile (AnjutaAmGroupNode *group, AnjutaToken *token);
void amp_group_update_monitor (AnjutaAmGroupNode *node);
AnjutaAmGroupNode* amp_group_new (GFile *file, gboolean dist_only, GError **error);
void amp_group_free (AnjutaAmGroupNode *node);

typedef enum _AmpTargetFlag
{
	AM_TARGET_CHECK = 1 << 0,
	AM_TARGET_NOINST = 1 << 1,
	AM_TARGET_DIST = 1 << 2,
	AM_TARGET_NODIST = 1 << 3,
	AM_TARGET_NOBASE = 1 << 4,
	AM_TARGET_NOTRANS = 1 << 5,
	AM_TARGET_MAN = 1 << 6,
	AM_TARGET_MAN_SECTION = 31 << 7
} AmpTargetFlag;


void amp_target_add_token (AnjutaAmTargetNode *target, AmTokenType type, AnjutaToken *token);
GList * amp_target_get_token (AnjutaAmTargetNode *target, AmTokenType type);
AnjutaTokenType amp_target_get_first_token_type (AnjutaAmTargetNode *target);
AnjutaTokenType amp_target_get_next_token_type (AnjutaAmTargetNode *target, AnjutaTokenType type);
AnjutaAmTargetNode* amp_target_new (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags, GError **error);
void amp_target_free (AnjutaAmTargetNode *node);

AnjutaProjectNode* amp_source_new (GFile *file, GError **error);
void amp_source_free (AnjutaAmSourceNode *node);
AnjutaToken *amp_source_get_token (AnjutaAmSourceNode *node);
void amp_source_add_token (AnjutaAmSourceNode *node, AnjutaToken *token);

G_END_DECLS

#endif /* _DIR_NODE_H_ */

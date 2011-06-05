/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-target.h
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

#ifndef _AMP_TARGET_H_
#define _AMP_TARGET_H_

#include "am-project-private.h"
#include "am-scanner.h"

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>

G_BEGIN_DECLS

/* Type macros
 *---------------------------------------------------------------------------*/

#define AMP_TYPE_TARGET_NODE							(amp_target_node_get_type ())
#define AMP_TARGET_NODE(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), AMP_TYPE_TARGET_NODE, AmpTargetNode))

GType amp_target_node_get_type (void) G_GNUC_CONST;

typedef enum {
	AM_TARGET_TOKEN_TARGET,
	AM_TARGET_TOKEN_SOURCES,
	AM_TARGET_TOKEN_LAST
} AmpTargetNodeTokenCategory;

typedef enum _AmpTargetNodeFlag
{
	AM_TARGET_CHECK = 1 << 0,
	AM_TARGET_NOINST = 1 << 1,
	AM_TARGET_DIST = 1 << 2,
	AM_TARGET_NODIST = 1 << 3,
	AM_TARGET_NOBASE = 1 << 4,
	AM_TARGET_NOTRANS = 1 << 5,
	AM_TARGET_MAN = 1 << 6,
	AM_TARGET_MAN_SECTION = 31 << 7
} AmpTargetNodeFlag;

void amp_target_node_register (GTypeModule *module);

void amp_target_node_add_token (AmpTargetNode *target, AmTokenType type, AnjutaToken *token);
void amp_target_node_remove_token (AmpTargetNode *target, AnjutaToken *token);
GList * amp_target_node_get_token (AmpTargetNode *target, AmTokenType type);
GList * amp_target_node_get_all_token (AmpTargetNode *target);
void amp_target_node_set_type (AmpTargetNode *target, AmTokenType type);
AnjutaTokenType amp_target_node_get_first_token_type (AmpTargetNode *target);
AnjutaTokenType amp_target_node_get_next_token_type (AmpTargetNode *target, AnjutaTokenType type);
AmpTargetNode* amp_target_node_new (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags);
AmpTargetNode* amp_target_node_new_valid (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags, GError **error);
void amp_target_node_free (AmpTargetNode *node);
void amp_target_node_update_node (AmpTargetNode *node, AmpTargetNode *new_node);
void amp_target_changed (AmpTargetNode *node);

G_END_DECLS

#endif /* _AM_TARGET_H_ */

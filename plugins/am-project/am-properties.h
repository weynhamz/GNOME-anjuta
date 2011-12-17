/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-properties.h
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

#ifndef _AM_PROPERTIES_H_
#define _AM_PROPERTIES_H_

#include "am-project.h"

#include <glib.h>

G_BEGIN_DECLS

AnjutaProjectPropertyInfo *amp_property_info_new (AnjutaTokenType type, gint position);
void amp_property_info_free (AnjutaProjectPropertyInfo *prop);

AnjutaProjectProperty *amp_property_new (const gchar *name, AnjutaTokenType type, gint position, const gchar *value, AnjutaToken *token);
void amp_property_free (AnjutaProjectProperty *prop);

gboolean amp_node_property_load (AnjutaProjectNode *target, gint token_type, gint position, const gchar *value, AnjutaToken *token);
gboolean amp_node_property_add (AnjutaProjectNode *node, AnjutaProjectProperty *prop);
AnjutaProjectPropertyInfo *amp_node_get_property_info_from_token (AnjutaProjectNode *node, gint token, gint pos);
AnjutaProjectProperty *amp_node_get_property_from_token (AnjutaProjectNode *node, gint token, gint pos);
AnjutaProjectProperty *amp_node_property_set (AnjutaProjectNode *node, const gchar *id, const gchar* value);
AnjutaProjectProperty *amp_node_map_property_set (AnjutaProjectNode *node, const gchar *id, const gchar *name, const gchar* value);

gboolean amp_node_property_has_flags (AnjutaProjectNode *node, const gchar *id, const gchar *value);
AnjutaProjectProperty *amp_node_property_remove_flags (AnjutaProjectNode *node, const gchar *id, const gchar *value);
AnjutaProjectProperty *amp_node_property_add_flags (AnjutaProjectNode *node, const gchar *id, const gchar *value);

GList* amp_get_project_property_list (void);
GList* amp_get_group_property_list (void);
GList* amp_get_target_property_list (AnjutaProjectNodeType type);
GList* amp_get_source_property_list (void);
GList* amp_get_module_property_list (void);
GList* amp_get_package_property_list (void);

G_END_DECLS

#endif /* _AM_PROPERTIES_H_ */

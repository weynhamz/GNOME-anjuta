/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-project.h
 * Copyright (C) Sébastien Granjoux 2009 <seb.sfo@free.fr>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_PROJECT_H_
#define _ANJUTA_PROJECT_H_

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ANJUTA_IS_PROJECT_GROUP(obj) (((AnjutaProjectNodeData *)obj->data)->type == ANJUTA_PROJECT_GROUP)
#define ANJUTA_IS_PROJECT_TARGET(obj) (((AnjutaProjectNodeData *)obj->data)->type == ANJUTA_PROJECT_TARGET)
#define ANJUTA_IS_PROJECT_NODE(obj) (1)
#define ANJUTA_IS_PROJECT_PROPERTY(obj) (1)

typedef enum
{
	ANJUTA_PROJECT_UNKNOWN = 0,
	ANJUTA_PROJECT_SHAREDLIB,
	ANJUTA_PROJECT_STATICLIB,
	ANJUTA_PROJECT_BINARY,
	ANJUTA_PROJECT_PYTHON,
	ANJUTA_PROJECT_JAVA,
	ANJUTA_PROJECT_LISP,
	ANJUTA_PROJECT_HEADER,
	ANJUTA_PROJECT_MAN,
	ANJUTA_PROJECT_INFO,
	ANJUTA_PROJECT_GENERIC,
	ANJUTA_PROJECT_DATA,
	ANJUTA_PROJECT_EXTRA,
	ANJUTA_PROJECT_INTLTOOL,
	ANJUTA_PROJECT_CONFIGURE,
	ANJUTA_PROJECT_IDL,
	ANJUTA_PROJECT_MKENUMS,
	ANJUTA_PROJECT_GENMARSHAL,
	ANJUTA_PROJECT_SCRIPT,
	ANJUTA_PROJECT_PROXY = 1 << 14,
	ANJUTA_PROJECT_EXECUTABLE = 1 << 15,
	ANJUTA_PROJECT_ID_MASK = 0xFFF,
	ANJUTA_PROJECT_TYPE_MASK = 0xFFFF << 16,
	ANJUTA_PROJECT_ROOT = 1 << 16,
	ANJUTA_PROJECT_GROUP = 2 << 16,
	ANJUTA_PROJECT_TARGET = 3 << 16,
	ANJUTA_PROJECT_SOURCE = 4 << 16 ,
	ANJUTA_PROJECT_MODULE = 5 << 16,
	ANJUTA_PROJECT_PACKAGE = 6 << 16,
	ANJUTA_PROJECT_VARIABLE = 7 << 16
} AnjutaProjectNodeType;

typedef enum
{
	ANJUTA_PROJECT_OK = 0,
	ANJUTA_PROJECT_MODIFIED = 1 << 0,		/* Node has been modified */
	ANJUTA_PROJECT_INCOMPLETE = 1 << 1,	/* Node is not fully loaded */
	ANJUTA_PROJECT_LOADING = 1 << 2	    	/* Node is send to the worker thread */
} AnjutaProjectNodeState;

typedef enum
{
	ANJUTA_TARGET_UNKNOWN,
	ANJUTA_TARGET_SHAREDLIB,
	ANJUTA_TARGET_STATICLIB,
	ANJUTA_TARGET_EXECUTABLE,
	ANJUTA_TARGET_PYTHON,
	ANJUTA_TARGET_JAVA,
	ANJUTA_TARGET_LISP,
	ANJUTA_TARGET_HEADER,
	ANJUTA_TARGET_MAN,
	ANJUTA_TARGET_INFO,
	ANJUTA_TARGET_GENERIC,
	ANJUTA_TARGET_DATA,
	ANJUTA_TARGET_EXTRA,
	ANJUTA_TARGET_INTLTOOL,
	ANJUTA_TARGET_CONFIGURE,
	ANJUTA_TARGET_IDL,
	ANJUTA_TARGET_MKENUMS,
	ANJUTA_TARGET_GENMARSHAL
} AnjutaProjectTargetClass;

typedef struct 
{
	AnjutaProjectNodeType type;
	gchar *name;
	gchar *mime_type;
} AnjutaProjectNodeInfo;

typedef enum
{
	ANJUTA_PROJECT_PROPERTY_STRING = 1,
	ANJUTA_PROJECT_PROPERTY_BOOLEAN,
	ANJUTA_PROJECT_PROPERTY_LIST
} AnjutaProjectValueType;

typedef struct
{
	gchar *name;
	AnjutaProjectValueType type;
	gchar *value;
	GList *override;
} AnjutaProjectPropertyInfo;

typedef GList AnjutaProjectProperty;


typedef struct
{
	AnjutaProjectNodeType type;
	AnjutaProjectProperty *properties;
	GFile *file;
	gchar *name;
	AnjutaProjectNodeState state;
} AnjutaProjectNodeData;

#if 0
typedef struct {
	AnjutaProjectNodeData node;
	GFile *directory;
} AnjutaProjectGroupData;

typedef struct {
	AnjutaProjectNodeData node;
	gchar *name;
	AnjutaProjectTargetType type;
} AnjutaProjectTargetData;

typedef struct {
	AnjutaProjectNodeData node;
	GFile *file;
} AnjutaProjectSourceData;
#endif

typedef GNode AnjutaProjectNode;
#if 0
typedef GNode AnjutaProjectGroup;
typedef GNode AnjutaProjectTarget;
typedef GNode AnjutaProjectSource;
#endif

#define ANJUTA_PROJECT_NODE_DATA(node)  ((node) != NULL ? (AnjutaProjectNodeData *)((node)->data) : NULL)

typedef void (*AnjutaProjectNodeFunc) (AnjutaProjectNode *node, gpointer data);

AnjutaProjectProperty *anjuta_project_property_next (AnjutaProjectProperty *list);
AnjutaProjectProperty *anjuta_project_property_override (AnjutaProjectProperty *list, AnjutaProjectProperty *prop);
AnjutaProjectProperty *anjuta_project_property_next_item (AnjutaProjectProperty *item);
AnjutaProjectPropertyInfo *anjuta_project_property_get_info (AnjutaProjectProperty *list);
AnjutaProjectPropertyInfo *anjuta_project_property_lookup (AnjutaProjectProperty *list, AnjutaProjectProperty *prop);
AnjutaProjectProperty *anjuta_project_property_insert (AnjutaProjectProperty *list, AnjutaProjectProperty *prop, AnjutaProjectPropertyInfo *info);
AnjutaProjectProperty *anjuta_project_property_remove (AnjutaProjectProperty *list, AnjutaProjectProperty *prop);
void anjuta_project_property_foreach (AnjutaProjectProperty *list, GFunc func, gpointer user_data);


AnjutaProjectNode *anjuta_project_node_parent (AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_first_child (AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_last_child (AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_next_sibling (AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_prev_sibling (AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_nth_child (AnjutaProjectNode *node, guint n);

AnjutaProjectNode *anjuta_project_node_append (AnjutaProjectNode *parent, AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_prepend (AnjutaProjectNode *parent, AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_insert_before (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_node_insert_after (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node);

void anjuta_project_node_all_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data);
void anjuta_project_node_children_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data);

gboolean anjuta_project_node_set_state (AnjutaProjectNode *node, AnjutaProjectNodeState state);
gboolean anjuta_project_node_clear_state (AnjutaProjectNode *node, AnjutaProjectNodeState state);

AnjutaProjectNodeType anjuta_project_node_get_type (const AnjutaProjectNode *node);
AnjutaProjectNodeType anjuta_project_node_get_full_type (const AnjutaProjectNode *node);
AnjutaProjectNodeState anjuta_project_node_get_state (const AnjutaProjectNode *node);
gchar *anjuta_project_node_get_name (const AnjutaProjectNode *node);
gchar *anjuta_project_node_get_uri (AnjutaProjectNode *node);
GFile *anjuta_project_node_get_file (AnjutaProjectNode *node);

AnjutaProjectProperty *anjuta_project_node_first_property (AnjutaProjectNode *node);
AnjutaProjectProperty *anjuta_project_node_first_valid_property (AnjutaProjectNode *node);
AnjutaProjectProperty *anjuta_project_node_get_property (AnjutaProjectNode *node, AnjutaProjectProperty *property);
AnjutaProjectProperty *anjuta_project_node_insert_property (AnjutaProjectNode *node, AnjutaProjectProperty *frame, AnjutaProjectProperty *property);
AnjutaProjectProperty *anjuta_project_node_remove_property (AnjutaProjectNode *node, AnjutaProjectProperty *property);
//const gchar *anjuta_project_node_get_property_value (AnjutaProjectNode *node, AnjutaProjectProperty prop);

AnjutaProjectNode *anjuta_project_group_get_node_from_file (const AnjutaProjectNode *root, GFile *directory);
AnjutaProjectNode *anjuta_project_target_get_node_from_name (const AnjutaProjectNode *parent, const gchar *name);
AnjutaProjectNode *anjuta_project_source_get_node_from_file (const AnjutaProjectNode *parent, GFile *file);
AnjutaProjectNode *anjuta_project_group_get_node_from_uri (const AnjutaProjectNode *root, const gchar *uri);
AnjutaProjectNode *anjuta_project_source_get_node_from_uri (const AnjutaProjectNode *parent, const gchar *uri);

GFile *anjuta_project_group_get_directory (const AnjutaProjectNode *group);

const gchar *anjuta_project_target_get_name (const AnjutaProjectNode *target);

GFile *anjuta_project_source_get_file (const AnjutaProjectNode *source);

const gchar *anjuta_project_node_info_name (const AnjutaProjectNodeInfo *info);
const gchar *anjuta_project_node_info_mime (const AnjutaProjectNodeInfo *info);
AnjutaProjectNodeType anjuta_project_node_info_type (const AnjutaProjectNodeInfo *info);

AnjutaProjectNode *anjuta_project_proxy_new (AnjutaProjectNode *node);
AnjutaProjectNode *anjuta_project_proxy_unref (AnjutaProjectNode *node);

gboolean anjuta_project_node_is_proxy (AnjutaProjectNode *node);


G_END_DECLS

#endif

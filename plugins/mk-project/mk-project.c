/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* mk-project.c
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

#include "mk-project.h"
#include "mk-rule.h"

#include "mk-project-private.h"

#include <libanjuta/interfaces/ianjuta-project.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#include <string.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <glib.h>
#include "mk-scanner.h"


#define UNIMPLEMENTED  G_STMT_START { g_warning (G_STRLOC": unimplemented"); } G_STMT_END

/* Constant strings for parsing perl script error output */
#define ERROR_PREFIX      "ERROR("
#define WARNING_PREFIX    "WARNING("
#define MESSAGE_DELIMITER ": "

static const gchar *valid_makefiles[] = {"GNUmakefile", "makefile", "Makefile", NULL};

/* convenient shortcut macro the get the MkpNode from a GNode */
#define MKP_NODE_DATA(node)  ((node) != NULL ? (AnjutaProjectNodeData *)((node)->data) : NULL)
#define MKP_GROUP_DATA(node)  ((node) != NULL ? (MkpGroupData *)((node)->data) : NULL)
#define MKP_TARGET_DATA(node)  ((node) != NULL ? (MkpTargetData *)((node)->data) : NULL)
#define MKP_SOURCE_DATA(node)  ((node) != NULL ? (MkpSourceData *)((node)->data) : NULL)


struct _MkpVariable {
	gchar *name;
	AnjutaTokenType assign;
	AnjutaToken *value;
};

typedef struct _MkpRootData MkpRootData;

struct _MkpRootData {
	AnjutaProjectNodeData base;
};

typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} MkpGroupTokenCategory;

typedef struct _MkpGroupData MkpGroupData;

struct _MkpGroupData {
	AnjutaProjectNodeData base;		/* Common node data */
	gboolean dist_only;			/* TRUE if the group is distributed but not built */
	GFile *makefile;				/* GFile corresponding to group makefile */
	AnjutaTokenFile *tfile;		/* Corresponding Makefile */
	GList *tokens[AM_GROUP_TOKEN_LAST];					/* List of token used by this group */
};

typedef enum _MkpTargetFlag
{
	AM_TARGET_CHECK = 1 << 0,
	AM_TARGET_NOINST = 1 << 1,
	AM_TARGET_DIST = 1 << 2,
	AM_TARGET_NODIST = 1 << 3,
	AM_TARGET_NOBASE = 1 << 4,
	AM_TARGET_NOTRANS = 1 << 5,
	AM_TARGET_MAN = 1 << 6,
	AM_TARGET_MAN_SECTION = 31 << 7
} MkpTargetFlag;

typedef struct _MkpTargetData MkpTargetData;

struct _MkpTargetData {
	AnjutaProjectNodeData base;
	gchar *install;
	gint flags;
	GList* tokens;
};

typedef struct _MkpSourceData MkpSourceData;

struct _MkpSourceData {
	AnjutaProjectNodeData base;
	AnjutaToken* token;
};

typedef struct _MkpNodeInfo MkpNodeInfo;

struct _MkpNodeInfo {
	AnjutaProjectNodeInfo base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};

/* Target types
 *---------------------------------------------------------------------------*/

static MkpNodeInfo MkpNodeInformation[] = {
	{{ANJUTA_PROJECT_GROUP,
	N_("Group"),
	""},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_SOURCE,
	N_("Source"),
	""},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_TARGET,
	N_("Unknown"),
	"text/plain"},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_UNKNOWN,
	NULL,
	NULL},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL}
};


/* ----- Standard GObject types and variables ----- */

enum {
	PROP_0,
	PROP_PROJECT_DIR
};

static GObject *parent_class;

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
error_set (GError **error, gint code, const gchar *message)
{
        if (error != NULL) {
                if (*error != NULL) {
                        gchar *tmp;

                        /* error already created, just change the code
                         * and prepend the string */
                        (*error)->code = code;
                        tmp = (*error)->message;
                        (*error)->message = g_strconcat (message, "\n\n", tmp, NULL);
                        g_free (tmp);

                } else {
                        *error = g_error_new_literal (IANJUTA_PROJECT_ERROR,
                                                      code,
                                                      message);
                }
        }
}

/* Work even if file is not a descendant of parent */
static gchar*
get_relative_path (GFile *parent, GFile *file)
{
	gchar *relative;

	relative = g_file_get_relative_path (parent, file);
	if (relative == NULL)
	{
		if (g_file_equal (parent, file))
		{
			relative = g_strdup ("");
		}
		else
		{
			GFile *grand_parent = g_file_get_parent (parent);
			gint level;
			gchar *grand_relative;
			gchar *ptr;
			gsize len;
			
			
			for (level = 1;  !g_file_has_prefix (file, grand_parent); level++)
			{
				GFile *next = g_file_get_parent (grand_parent);
				
				g_object_unref (grand_parent);
				grand_parent = next;
			}

			grand_relative = g_file_get_relative_path (grand_parent, file);
			g_object_unref (grand_parent);

			len = strlen (grand_relative);
			relative = g_new (gchar, len + level * 3 + 1);
			ptr = relative;
			for (; level; level--)
			{
				memcpy(ptr, ".." G_DIR_SEPARATOR_S, 3);
				ptr += 3;
			}
			memcpy (ptr, grand_relative, len + 1);
			g_free (grand_relative);
		}
	}

	return relative;
}

static GFileType
file_type (GFile *file, const gchar *filename)
{
	GFile *child;
	GFileInfo *info;
	GFileType type;

	child = filename != NULL ? g_file_get_child (file, filename) : g_object_ref (file);

	//g_message ("check file %s", g_file_get_path (child));
	
	info = g_file_query_info (child,
	                          G_FILE_ATTRIBUTE_STANDARD_TYPE, 
	                          G_FILE_QUERY_INFO_NONE,
	                          NULL,
	                          NULL);
	if (info != NULL)
	{
		type = g_file_info_get_file_type (info);
		g_object_unref (info);
	}
	else
	{
		type = G_FILE_TYPE_UNKNOWN;
	}
	
	g_object_unref (child);
	
	return type;
}

/* Root objects
 *---------------------------------------------------------------------------*/

static AnjutaProjectNode*
mkp_root_new (GFile *file)
{
	MkpRootData *root = NULL;

	g_return_val_if_fail (file != NULL, NULL);
	
	root = g_slice_new0(MkpRootData); 
	root->base.type = ANJUTA_PROJECT_ROOT;
	root->base.properties = NULL;
	root->base.file = g_file_dup (file);

	return g_node_new (root);
}

static void
mkp_root_free (AnjutaProjectNode *node)
{
	AnjutaProjectNodeData *data = MKP_NODE_DATA (node);
	
	if (data->file != NULL) g_object_unref (data->file);
	g_free (data->name);
	g_slice_free (MkpRootData, (MkpRootData *)data);

	g_node_destroy (node);
}

/* Group objects
 *---------------------------------------------------------------------------*/

static void
mkp_group_add_token (MkpGroup *node, AnjutaToken *token, MkpGroupTokenCategory category)
{
    MkpGroupData *group;
	
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	group = MKP_GROUP_DATA (node);
	group->tokens[category] = g_list_prepend (group->tokens[category], token);
}

static GList *
mkp_group_get_token (MkpGroup *node, MkpGroupTokenCategory category)
{
    MkpGroupData *group;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	group = MKP_GROUP_DATA (node);
	return group->tokens[category];
}

static AnjutaTokenFile*
mkp_group_set_makefile (MkpGroup *node, GFile *makefile)
{
    MkpGroupData *group;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	group = MKP_GROUP_DATA (node);
	if (group->makefile != NULL) g_object_unref (group->makefile);
	if (group->tfile != NULL) anjuta_token_file_free (group->tfile);
	if (makefile != NULL)
	{
		group->makefile = g_object_ref (makefile);
		group->tfile = anjuta_token_file_new (makefile);
	}
	else
	{
		group->makefile = NULL;
		group->tfile = NULL;
	}

	return group->tfile;
}

static MkpGroup*
mkp_group_new (GFile *file)
{
    MkpGroupData *group = NULL;

	g_return_val_if_fail (file != NULL, NULL);
	
	group = g_slice_new0(MkpGroupData); 
	group->base.type = ANJUTA_PROJECT_GROUP;
	group->base.file = g_object_ref (file);

    return g_node_new (group);
}

static void
mkp_group_free (MkpGroup *node)
{
    MkpGroupData *group = (MkpGroupData *)node->data;
	gint i;
	
	if (group->base.file) g_object_unref (group->base.file);
	if (group->tfile) anjuta_token_file_free (group->tfile);
	if (group->makefile) g_object_unref (group->makefile);
	for (i = 0; i < AM_GROUP_TOKEN_LAST; i++)
	{
		if (group->tokens[i] != NULL) g_list_free (group->tokens[i]);
	}
    g_slice_free (MkpGroupData, group);

	g_node_destroy (node);
}

/* Target objects
 *---------------------------------------------------------------------------*/

void
mkp_target_add_token (MkpGroup *node, AnjutaToken *token)
{
    MkpTargetData *target;
	
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	target = MKP_TARGET_DATA (node);
	target->tokens = g_list_prepend (target->tokens, token);
}

static GList *
mkp_target_get_token (MkpGroup *node)
{
    MkpTargetData *target;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	target = MKP_TARGET_DATA (node);
	return target->tokens;
}


MkpTarget*
mkp_target_new (const gchar *name, AnjutaProjectNodeType type)
{
    MkpTargetData *target = NULL;

	target = g_slice_new0(MkpTargetData);
	target->base.type = type | ANJUTA_PROJECT_TARGET;
	target->base.name = g_strdup (name);

    return g_node_new (target);
}

void
mkp_target_free (MkpTarget *node)
{
    MkpTargetData *target = MKP_TARGET_DATA (node);
	
    g_free (target->base.name);
    g_free (target->install);
    g_slice_free (MkpTargetData, target);

	g_node_destroy (node);
}

/* Source objects
 *---------------------------------------------------------------------------*/

MkpSource*
mkp_source_new (GFile *file)
{
    MkpSourceData *source = NULL;

	source = g_slice_new0(MkpSourceData); 
	source->base.type = ANJUTA_PROJECT_SOURCE;
	source->base.file = g_object_ref (file);

    return g_node_new (source);
}

static void
mkp_source_free (MkpSource *node)
{
    MkpSourceData *source = MKP_SOURCE_DATA (node);
	
    g_object_unref (source->base.file);
    g_slice_free (MkpSourceData, source);

	g_node_destroy (node);
}

/*
 * File monitoring support --------------------------------
 * FIXME: review these
 */
static void
monitor_cb (GFileMonitor *monitor,
			GFile *file,
			GFile *other_file,
			GFileMonitorEvent event_type,
			gpointer data)
{
	MkpProject *project = data;

	g_return_if_fail (project != NULL && MKP_IS_PROJECT (project));

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* monitor will be removed here... is this safe? */
			//mkp_project_reload (project, NULL);
			g_signal_emit_by_name (G_OBJECT (project), "project-updated");
			break;
		default:
			break;
	}
}


static void
monitor_add (MkpProject *project, GFile *file)
{
	GFileMonitor *monitor = NULL;
	
	g_return_if_fail (project != NULL);
	g_return_if_fail (project->monitors != NULL);
	
	if (file == NULL)
		return;
	
	monitor = g_hash_table_lookup (project->monitors, file);
	if (!monitor) {
		gboolean exists;
		
		/* FIXME clarify if uri is uri, path or both */
		exists = g_file_query_exists (file, NULL);
		
		if (exists) {
			monitor = g_file_monitor_file (file, 
						       G_FILE_MONITOR_NONE,
						       NULL,
						       NULL);
			if (monitor != NULL)
			{
				g_signal_connect (G_OBJECT (monitor),
						  "changed",
						  G_CALLBACK (monitor_cb),
						  project);
				g_hash_table_insert (project->monitors,
						     g_object_ref (file),
						     monitor);
			}
		}
	}
}

static void
monitors_remove (MkpProject *project)
{
	g_return_if_fail (project != NULL);

	if (project->monitors)
		g_hash_table_destroy (project->monitors);
	project->monitors = NULL;
}

static void
group_hash_foreach_monitor (gpointer key,
			    gpointer value,
			    gpointer user_data)
{
	MkpGroup *group_node = value;
	MkpProject *project = user_data;

	monitor_add (project, MKP_GROUP_DATA(group_node)->base.file);
}

static void
monitors_setup (MkpProject *project)
{
	g_return_if_fail (project != NULL);

	monitors_remove (project);
	
	/* setup monitors hash */
	project->monitors = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
						   (GDestroyNotify) g_file_monitor_cancel);

	//monitor_add (project, anjuta_token_file_get_file (project->make_file));
	if (project->groups)
		g_hash_table_foreach (project->groups, group_hash_foreach_monitor, project);
}


/*
 * ---------------- Data structures managment
 */

static void
mkp_dump_node (AnjutaProjectNode *g_node)
{
	gchar *name = NULL;
	
	switch (MKP_NODE_DATA (g_node)->type) {
		case ANJUTA_PROJECT_GROUP:
			name = g_file_get_uri (MKP_GROUP_DATA (g_node)->base.file);
			DEBUG_PRINT ("GROUP: %s", name);
			break;
		case ANJUTA_PROJECT_TARGET:
			name = g_strdup (MKP_TARGET_DATA (g_node)->base.name);
			DEBUG_PRINT ("TARGET: %s", name);
			break;
		case ANJUTA_PROJECT_SOURCE:
			name = g_file_get_uri (MKP_SOURCE_DATA (g_node)->base.file);
			DEBUG_PRINT ("SOURCE: %s", name);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	g_free (name);
}

static void 
foreach_node_destroy (AnjutaProjectNode    *g_node,
		      gpointer  data)
{
	switch (MKP_NODE_DATA (g_node)->type) {
		case ANJUTA_PROJECT_GROUP:
			mkp_group_free (g_node);
			break;
		case ANJUTA_PROJECT_TARGET:
			mkp_target_free (g_node);
			break;
		case ANJUTA_PROJECT_SOURCE:
			mkp_source_free (g_node);
			break;
		case ANJUTA_PROJECT_ROOT:
			mkp_root_free (g_node);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
}

static void
project_node_destroy (MkpProject *project, AnjutaProjectNode *g_node)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (MKP_IS_PROJECT (project));
	
	if (g_node) {
		/* free each node's data first */
		anjuta_project_node_all_foreach (g_node,
				 foreach_node_destroy, project);

		/* now destroy the tree itself */
		//g_node_destroy (g_node);
	}
}

static AnjutaProjectNode *
project_node_new (MkpProject *project, AnjutaProjectNodeType type, GFile *file, const gchar *name)
{
	AnjutaProjectNode *node = NULL;
	
	switch (type & ANJUTA_PROJECT_TYPE_MASK) {
		case ANJUTA_PROJECT_GROUP:
			node = mkp_group_new (file);
			break;
		case ANJUTA_PROJECT_TARGET:
			node = mkp_target_new (name, 0);
			break;
		case ANJUTA_PROJECT_SOURCE:
			node = mkp_source_new (file);
			break;
		case ANJUTA_PROJECT_ROOT:
			node = mkp_root_new (file);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	if (node != NULL) MKP_NODE_DATA (node)->type = type;

	return node;
}

static void
find_target (AnjutaProjectNode *node, gpointer data)
{
	if (MKP_NODE_DATA (node)->type == ANJUTA_PROJECT_TARGET)
	{
		if (strcmp (MKP_TARGET_DATA (node)->base.name, *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(AnjutaProjectNode **)data = node;

			return;
		}
	}
}

static void
remove_make_file (gpointer data, GObject *object, gboolean is_last_ref)
{
	if (is_last_ref)
	{
		MkpProject *project = (MkpProject *)data;
		g_hash_table_remove (project->files, anjuta_token_file_get_file (ANJUTA_TOKEN_FILE (object)));
	}
}

static MkpGroup*
project_load_makefile (MkpProject *project, GFile *file, MkpGroup *parent, GError **error)
{
	MkpScanner *scanner;
	AnjutaToken *arg;
	AnjutaTokenFile *tfile;
	AnjutaToken *parse;
	gboolean ok;
	GError *err = NULL;

	/* Parse makefile */	
	DEBUG_PRINT ("Parse: %s", g_file_get_uri (file));
	tfile = mkp_group_set_makefile (parent, file);
	g_hash_table_insert (project->files, g_object_ref (file), g_object_ref (tfile));
//	g_object_add_toggle_ref (G_OBJECT (project->make_file), remove_make_file, project);
	arg = anjuta_token_file_load (tfile, NULL);
	scanner = mkp_scanner_new (project);
	parse = mkp_scanner_parse_token (scanner, arg, &err);
	ok = parse != NULL;
	mkp_scanner_free (scanner);
	if (!ok)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             	IANJUTA_PROJECT_ERROR_PROJECT_MALFORMED,
		    			err == NULL ? _("Unable to parse make file") : err->message);
		if (err != NULL) g_error_free (err);

		return NULL;
	}

	/* Load target */
	mkp_project_enumerate_targets (project, parent);

	return parent;
}

/* Project access functions
 *---------------------------------------------------------------------------*/

MkpGroup *
mkp_project_get_root (MkpProject *project)
{
	MkpGroup *g_node = NULL;
	
	if (project->root_file != NULL)
	{
		gchar *id = g_file_get_uri (project->root_file);
		g_node = (MkpGroup *)g_hash_table_lookup (project->groups, id);
		g_free (id);
	}

	return g_node;
}

GList *
mkp_project_list_variable (MkpProject *project)
{
	return g_hash_table_get_values (project->variables);
}

MkpVariable*
mkp_project_get_variable (MkpProject *project, const gchar *name)
{
	MkpVariable *var;
	
	var = (MkpVariable *)g_hash_table_lookup (project->groups, name);

	return var;
}

MkpGroup *
mkp_project_get_group (MkpProject *project, const gchar *id)
{
	return (MkpGroup *)g_hash_table_lookup (project->groups, id);
}

MkpTarget *
mkp_project_get_target (MkpProject *project, const gchar *id)
{
	MkpTarget **buffer;
	MkpTarget *target;
	gsize dummy;

	buffer = (MkpTarget **)g_base64_decode (id, &dummy);
	target = *buffer;
	g_free (buffer);

	return target;
}

MkpSource *
mkp_project_get_source (MkpProject *project, const gchar *id)
{
	MkpSource **buffer;
	MkpSource *source;
	gsize dummy;

	buffer = (MkpSource **)g_base64_decode (id, &dummy);
	source = *buffer;
	g_free (buffer);

	return source;
}

gchar *
mkp_project_get_node_id (MkpProject *project, const gchar *path)
{
	AnjutaProjectNode *node = NULL;

	if (path != NULL)
	{
		for (; *path != '\0';)
		{
			gchar *end;
			guint child = g_ascii_strtoull (path, &end, 10);

			if (end == path)
			{
				/* error */
				return NULL;
			}

			if (node == NULL)
			{
				if (child == 0) node = project->root_node;
			}
			else
			{
				node = anjuta_project_node_nth_child (node, child);
			}
			if (node == NULL)
			{
				/* no node */
				return NULL;
			}

			if (*end == '\0') break;
			path = end + 1;
		}
	}

	switch (MKP_NODE_DATA (node)->type)
	{
		case ANJUTA_PROJECT_GROUP:
			return g_file_get_uri (MKP_GROUP_DATA (node)->base.file);
		case ANJUTA_PROJECT_TARGET:
		case ANJUTA_PROJECT_SOURCE:
			return g_base64_encode ((guchar *)&node, sizeof (node));
		default:
			return NULL;
	}
}

gchar *
mkp_project_get_uri (MkpProject *project)
{
	g_return_val_if_fail (project != NULL, NULL);

	return project->root_file != NULL ? g_file_get_uri (project->root_file) : NULL;
}

GFile*
mkp_project_get_file (MkpProject *project)
{
	g_return_val_if_fail (project != NULL, NULL);

	return project->root_file;
}
	
GList *
mkp_project_get_node_info (MkpProject *project, GError **error)
{
	static GList *info_list = NULL;

	if (info_list == NULL)
	{
		MkpNodeInfo *node;
		
		for (node = MkpNodeInformation; node->base.type != 0; node++)
		{
			info_list = g_list_prepend (info_list, node);
		}

		info_list = g_list_reverse (info_list);
	}
	
	return info_list;
}

gboolean
mkp_project_get_token_location (MkpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token)
{
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	g_hash_table_iter_init (&iter, project->files);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		if (anjuta_token_file_get_token_location ((AnjutaTokenFile *)value, location, token))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/* Group access functions
 *---------------------------------------------------------------------------*/

GFile*
mkp_group_get_directory (MkpGroup *group)
{
	return MKP_GROUP_DATA (group)->base.file;
}

GFile*
mkp_group_get_makefile (MkpGroup *group)
{
	return MKP_GROUP_DATA (group)->makefile;
}

gchar *
mkp_group_get_id (MkpGroup *group)
{
	return g_file_get_uri (MKP_GROUP_DATA (group)->base.file);
}

/* Target access functions
 *---------------------------------------------------------------------------*/

const gchar *
mkp_target_get_name (MkpTarget *target)
{
	return MKP_TARGET_DATA (target)->base.name;
}

gchar *
mkp_target_get_id (MkpTarget *target)
{
	return g_base64_encode ((guchar *)&target, sizeof (target));
}

/* Source access functions
 *---------------------------------------------------------------------------*/

gchar *
mkp_source_get_id (MkpSource *source)
{
	return g_base64_encode ((guchar *)&source, sizeof (source));
}

GFile*
mkp_source_get_file (MkpSource *source)
{
	return MKP_SOURCE_DATA (source)->base.file;
}

/* Variable access functions
 *---------------------------------------------------------------------------*/

const gchar *
mkp_variable_get_name (MkpVariable *variable)
{
	return variable->name;
}

gchar *
mkp_variable_evaluate (MkpVariable *variable, MkpProject *project)
{
	return anjuta_token_evaluate (variable->value);
}

static MkpVariable*
mkp_variable_new (gchar *name, AnjutaTokenType assign, AnjutaToken *value)
{
    MkpVariable *variable = NULL;

	g_return_val_if_fail (name != NULL, NULL);
	
	variable = g_slice_new0(MkpVariable); 
	variable->name = g_strdup (name);
	variable->assign = assign;
	variable->value = value;

	return variable;
}

static void
mkp_variable_free (MkpVariable *variable)
{
	g_free (variable->name);
	
    g_slice_free (MkpVariable, variable);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
mkp_project_update_variable (MkpProject *project, AnjutaToken *variable)
{
	AnjutaToken *arg;
	char *name = NULL;
	MakeTokenType assign = 0;	
	AnjutaToken *value = NULL;

	fprintf(stdout, "update variable");
	anjuta_token_dump (variable);
	
	arg = anjuta_token_first_item (variable);
	name = g_strstrip (anjuta_token_evaluate (arg));
	arg = anjuta_token_next_item (arg);
	
	g_message ("new variable %s", name);
	switch (anjuta_token_get_type (arg))
	{
	case MK_TOKEN_EQUAL:
	case MK_TOKEN_IMMEDIATE_EQUAL:
	case MK_TOKEN_CONDITIONAL_EQUAL:
	case MK_TOKEN_APPEND:
		assign = anjuta_token_get_type (arg);
		break;
	default:
		break;
	}
	
	value = anjuta_token_next_item (arg);

	if (assign != 0)
	{
		MkpVariable *var;

		g_message ("assign %d name %s value %s\n", assign, name, anjuta_token_evaluate (value));
		var = (MkpVariable *)g_hash_table_lookup (project->variables, name);
		if (var != NULL)
		{
			var->assign = assign;
			var->value = value;
		}
		else
		{
			var = mkp_variable_new (name, assign, value);
			g_hash_table_insert (project->variables, var->name, var);
		}

	}

	g_message ("update variable %s", name);
	
	if (name) g_free (name);
}

AnjutaToken*
mkp_project_get_variable_token (MkpProject *project, AnjutaToken *variable)
{
	guint length;
	const gchar *string;
	gchar *name;
	MkpVariable *var;
		
	length = anjuta_token_get_length (variable);
	string = anjuta_token_get_string (variable);
	if (string[1] == '(')
	{
		name = g_strndup (string + 2, length - 3);
	}
	else
	{
		name = g_strndup (string + 1, 1);
	}
	var = g_hash_table_lookup (project->variables, name);
	g_free (name);

	return var != NULL ? var->value : NULL;
}

static AnjutaProjectNode *
mkp_project_load_root (MkpProject *project, AnjutaProjectNode *node, GError **error)
{
	GFile *root_file;
	GFile *make_file;
	const gchar **makefile;
	MkpGroup *group;

	/* Unload current project */
	root_file = g_object_ref (anjuta_project_node_get_file (node));
	mkp_project_unload (project);
	project->root_file = root_file;
	project->root_node = node;
	DEBUG_PRINT ("reload project %p root file %p", project, project->root_file);

	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_object_unref);
	project->variables = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)mkp_variable_free);

	/* Initialize rules data */
	mkp_project_init_rules (project);
	
	/* Initialize list styles */
	project->space_list = anjuta_token_style_new (NULL, " ", "\n", NULL, 0);
	project->arg_list = anjuta_token_style_new (NULL, ", ", ",\n ", ")", 0);

	/* Find make file */
	for (makefile = valid_makefiles; *makefile != NULL; makefile++)
	{
		if (file_type (root_file, *makefile) == G_FILE_TYPE_REGULAR)
		{
			make_file = g_file_get_child (root_file, *makefile);
			break;
		}
	}
	if (make_file == NULL)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));

		return NULL;
	}

	/* Create group */
	group = mkp_group_new (root_file);
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);
	anjuta_project_node_append (node, group);

	
	/* Parse make file */	
	project_load_makefile (project, make_file, group, error);
	g_object_unref (make_file);

	monitors_setup (project);
	
	
	return node;
}

AnjutaProjectNode *
dir_project_load_node (MkpProject *project, AnjutaProjectNode *node, GError **error) 
{
	switch (anjuta_project_node_get_type (node))
	{
	case ANJUTA_PROJECT_ROOT:
		return mkp_project_load_root (project, node, error);
	default:
		return NULL;
	}
}
 
gboolean
mkp_project_reload (MkpProject *project, GError **error) 
{
	GFile *root_file;
	GFile *make_file;
	const gchar **makefile;
	MkpGroup *group;
	gboolean ok = TRUE;

	/* Unload current project */
	root_file = g_object_ref (project->root_file);
	mkp_project_unload (project);
	project->root_file = root_file;
	DEBUG_PRINT ("reload project %p root file %p", project, project->root_file);

	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_object_unref);
	project->variables = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)mkp_variable_free);

	/* Initialize rules data */
	mkp_project_init_rules (project);
	
	/* Initialize list styles */
	project->space_list = anjuta_token_style_new (NULL, " ", "\n", NULL, 0);
	project->arg_list = anjuta_token_style_new (NULL, ", ", ",\n ", ")", 0);

	/* Find make file */
	for (makefile = valid_makefiles; *makefile != NULL; makefile++)
	{
		if (file_type (root_file, *makefile) == G_FILE_TYPE_REGULAR)
		{
			make_file = g_file_get_child (root_file, *makefile);
			break;
		}
	}
	if (make_file == NULL)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));

		return FALSE;
	}

	/* Create group */
	group = mkp_group_new (root_file);
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);
	project->root_node = group;

	
	/* Parse make file */	
	project_load_makefile (project, make_file, group, error);
	g_object_unref (make_file);

	monitors_setup (project);
	
	
	return ok;
}

gboolean
mkp_project_load (MkpProject  *project,
    GFile *directory,
	GError     **error)
{
	g_return_val_if_fail (directory != NULL, FALSE);

	project->root_node = mkp_root_new (directory);
	if (!mkp_project_load_root (project, project->root_node, error))
	{
		mkp_root_free (project->root_node);
		project->root_node = NULL;
	}

	return project->root_file != NULL;
}

void
mkp_project_unload (MkpProject *project)
{
	monitors_remove (project);
	
	/* project data */
	project_node_destroy (project, project->root_node);
	project->root_node = NULL;

	if (project->root_file) g_object_unref (project->root_file);
	project->root_file = NULL;

	/* shortcut hash tables */
	if (project->groups) g_hash_table_destroy (project->groups);
	project->groups = NULL;
	if (project->files) g_hash_table_destroy (project->files);
	project->files = NULL;
	if (project->variables) g_hash_table_destroy (project->variables);
	project->variables = NULL;

	mkp_project_free_rules (project);
	
	/* List styles */
	if (project->space_list) anjuta_token_style_free (project->space_list);
	if (project->arg_list) anjuta_token_style_free (project->arg_list);
}

gint
mkp_project_probe (GFile *directory,
	    GError     **error)
{
	gboolean probe;
	gboolean dir;
	
	dir = (file_type (directory, NULL) == G_FILE_TYPE_DIRECTORY);
	if (!dir)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));
	}
	
	probe =  dir;
	if (probe)
	{
		const gchar **makefile;

		/* Look for makefiles */
		probe = FALSE;
		for (makefile = valid_makefiles; *makefile != NULL; makefile++)
		{
			if (file_type (directory, *makefile) == G_FILE_TYPE_REGULAR)
			{
				probe = TRUE;
				break;
			}
		}
	}

	return probe ? IANJUTA_PROJECT_PROBE_MAKE_FILES : 0;
}

gboolean
mkp_project_save (MkpProject *project, GError **error)
{
	gpointer key;
	gpointer value;
	GHashTableIter iter;

	g_return_val_if_fail (project != NULL, FALSE);

	g_hash_table_iter_init (&iter, project->files);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		GError *error = NULL;
		AnjutaTokenFile *tfile = (AnjutaTokenFile *)value;
		;
		anjuta_token_file_save (tfile, &error);
	}

	return TRUE;
}

gboolean
mkp_project_move (MkpProject *project, const gchar *path)
{
	GFile	*old_root_file;
	GFile *new_file;
	gchar *relative;
	GHashTableIter iter;
	gpointer key;
	gpointer value;
	AnjutaTokenFile *tfile;
	GHashTable* old_hash;

	/* Change project root directory */
	old_root_file = project->root_file;
	project->root_file = g_file_new_for_path (path);

	/* Change project root directory in groups */
	old_hash = project->groups;
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	g_hash_table_iter_init (&iter, old_hash);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		MkpGroup *group = (MkpGroup *)value;
		
		relative = get_relative_path (old_root_file, MKP_GROUP_DATA (group)->base.file);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (MKP_GROUP_DATA (group)->base.file);
		MKP_GROUP_DATA (group)->base.file = new_file;

		g_hash_table_insert (project->groups, g_file_get_uri (new_file), group);
	}
	g_hash_table_destroy (old_hash);

	/* Change all files */
	old_hash = project->files;
	project->files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_object_unref);
	g_hash_table_iter_init (&iter, old_hash);
	while (g_hash_table_iter_next (&iter, &key, (gpointer *)&tfile))
	{
		relative = get_relative_path (old_root_file, anjuta_token_file_get_file (tfile));
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		anjuta_token_file_move (tfile, new_file);
		
		g_hash_table_insert (project->files, new_file, tfile);
		g_object_unref (key);
	}
	g_hash_table_steal_all (old_hash);
	g_hash_table_destroy (old_hash);

	g_object_unref (old_root_file);

	return TRUE;
}

MkpProject *
mkp_project_new (void)
{
	return MKP_PROJECT (g_object_new (MKP_TYPE_PROJECT, NULL));
}

/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static AnjutaProjectNode* 
iproject_add_group (IAnjutaProject *obj, AnjutaProjectNode *parent,  const gchar *name, GError **err)
{
	return NULL;
}

static AnjutaProjectNode* 
iproject_add_source (IAnjutaProject *obj, AnjutaProjectNode *parent,  GFile *file, GError **err)
{
	return NULL;
}

static AnjutaProjectNode* 
iproject_add_target (IAnjutaProject *obj, AnjutaProjectNode *parent,  const gchar *name,  AnjutaProjectNodeType type, GError **err)
{
	return NULL;
}

static GtkWidget* 
iproject_configure (IAnjutaProject *obj, GError **err)
{
	return NULL;
}

static guint 
iproject_get_capabilities (IAnjutaProject *obj, GError **err)
{
	return IANJUTA_PROJECT_CAN_ADD_NONE;
}

static GList* 
iproject_get_packages (IAnjutaProject *obj, GError **err)
{
	return NULL;
}

static AnjutaProjectNode* 
iproject_get_root (IAnjutaProject *obj, GError **err)
{
	return mkp_project_get_root (MKP_PROJECT (obj));
}

static GList* 
iproject_get_target_types (IAnjutaProject *obj, GError **err)
{
	return mkp_project_get_node_info (MKP_PROJECT (obj), err);
}

static gboolean
iproject_load (IAnjutaProject *obj, GFile *file, GError **err)
{
	return mkp_project_load (MKP_PROJECT (obj), file, err);
}

static gboolean
iproject_refresh (IAnjutaProject *obj, GError **err)
{
	return FALSE; //mkp_project_reload (MKP_PROJECT (obj), err);
}

static AnjutaProjectNode *
iproject_load_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	return dir_project_load_node (MKP_PROJECT (obj), node, err);
}

static AnjutaProjectNode *
iproject_save_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	return NULL;
}

static AnjutaProjectNode *
iproject_new_root_node (IAnjutaProject *obj, GFile *file, GError **err)
{
	return project_node_new (MKP_PROJECT (obj), ANJUTA_PROJECT_ROOT, file, NULL);
}

static AnjutaProjectNode *
iproject_add_file_node (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, GError **err)
{
	return NULL;
}

static AnjutaProjectNode *
iproject_add_name_node (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, const gchar *name, GError **err)
{
	return NULL;
}

static gboolean
iproject_remove_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	project_node_destroy (MKP_PROJECT (obj), node);

	return TRUE;
}

static AnjutaProjectProperty *
iproject_set_boolean_property (IAnjutaProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *property, gboolean value, GError **err )
{
	return NULL;
}

static AnjutaProjectProperty *
iproject_set_string_property (IAnjutaProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *property, const gchar *value, GError **err )
{
	return NULL;
}

static AnjutaProjectProperty *
iproject_set_list_property (IAnjutaProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *property, const gchar *name, const gchar *value, GError **err )
{
	return NULL;
}

static gboolean
iproject_remove_property (IAnjutaProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *property, GError **err)
{
	return FALSE;
}

static GList* 
iproject_get_node_info (IAnjutaProject *obj, GError **err)
{
	return mkp_project_get_node_info (MKP_PROJECT (obj), err);
}

static void
iproject_iface_init(IAnjutaProjectIface* iface)
{
	iface->add_group = iproject_add_group;
	iface->add_source = iproject_add_source;
	iface->add_target = iproject_add_target;
	iface->configure = iproject_configure;
	iface->get_capabilities = iproject_get_capabilities;
	iface->get_packages = iproject_get_packages;
	iface->get_root = iproject_get_root;
	iface->get_target_types = iproject_get_target_types;
	iface->load = iproject_load;
	iface->refresh = iproject_refresh;
	iface->load_node = iproject_load_node;
	iface->save_node = iproject_save_node;
	iface->new_root_node = iproject_new_root_node;
	iface->add_file_node = iproject_add_file_node;
	iface->add_name_node = iproject_add_name_node;
	iface->remove_node = iproject_remove_node;
	iface->set_boolean_property = iproject_set_boolean_property;
	iface->set_string_property = iproject_set_string_property;
	iface->set_list_property = iproject_set_list_property;
	iface->remove_property = iproject_remove_property;
	iface->get_node_info = iproject_get_node_info;
}

/* GObject implementation
 *---------------------------------------------------------------------------*/

static void
mkp_project_dispose (GObject *object)
{
	g_return_if_fail (MKP_IS_PROJECT (object));

	mkp_project_unload (MKP_PROJECT (object));

	G_OBJECT_CLASS (parent_class)->dispose (object);	
}

static void
mkp_project_instance_init (MkpProject *project)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (MKP_IS_PROJECT (project));
	
	/* project data */
	project->root_file = NULL;
	project->root_node = NULL;
	project->property = NULL;
	project->suffix = NULL;
	project->rules = NULL;

	project->space_list = NULL;
	project->arg_list = NULL;
}

static void
mkp_project_class_init (MkpProjectClass *klass)
{
	GObjectClass *object_class;
	
	parent_class = g_type_class_peek_parent (klass);

	object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = mkp_project_dispose;
}

ANJUTA_TYPE_BEGIN(MkpProject, mkp_project, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(iproject, IANJUTA_TYPE_PROJECT);
ANJUTA_TYPE_END;

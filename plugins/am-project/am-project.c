/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-project.c
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

#include "am-project.h"

#include "am-project-private.h"

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

#include "ac-scanner.h"
#include "ac-writer.h"
#include "am-scanner.h"
#include "am-dialogs.h"
//#include "am-config.h"
//#include "am-properties.h"


#define UNIMPLEMENTED  G_STMT_START { g_warning (G_STRLOC": unimplemented"); } G_STMT_END

/* Constant strings for parsing perl script error output */
#define ERROR_PREFIX      "ERROR("
#define WARNING_PREFIX    "WARNING("
#define MESSAGE_DELIMITER ": "

static const gchar *valid_am_makefiles[] = {"GNUmakefile.am", "makefile.am", "Makefile.am", NULL};

/* convenient shortcut macro the get the AnjutaProjectNode from a GNode */
#define AMP_NODE_DATA(node)  ((node) != NULL ? (AnjutaProjectNodeData *)((node)->data) : NULL)
#define AMP_GROUP_DATA(node)  ((node) != NULL ? (AmpGroupData *)((node)->data) : NULL)
#define AMP_TARGET_DATA(node)  ((node) != NULL ? (AmpTargetData *)((node)->data) : NULL)
#define AMP_SOURCE_DATA(node)  ((node) != NULL ? (AmpSourceData *)((node)->data) : NULL)

typedef struct _AmpPackage AmpPackage;

struct _AmpPackage {
    gchar *name;
    gchar *version;
};

typedef struct _AmpModule AmpModule;
	
struct _AmpModule {
    GList *packages;
    AnjutaToken *module;
};

typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} AmpGroupTokenCategory;

typedef struct _AmpGroupData AmpGroupData;

struct _AmpGroupData {
	AnjutaProjectGroupData base;		/* Common node data */
	gboolean dist_only;			/* TRUE if the group is distributed but not built */
	GFile *makefile;				/* GFile corresponding to group makefile */
	AnjutaTokenFile *tfile;		/* Corresponding Makefile */
	GList *tokens[AM_GROUP_TOKEN_LAST];					/* List of token used by this group */
};

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

typedef struct _AmpTargetData AmpTargetData;

struct _AmpTargetData {
	AnjutaProjectTargetData base;
	gchar *install;
	gint flags;
	GList* tokens;
};

typedef struct _AmpSourceData AmpSourceData;

struct _AmpSourceData {
	AnjutaProjectSourceData base;
	AnjutaToken* token;
};

typedef struct _AmpConfigFile AmpConfigFile;

struct _AmpConfigFile {
	GFile *file;
	AnjutaToken *token;
};

typedef struct _AmpTargetInformation AmpTargetInformation;

struct _AmpTargetInformation {
	AnjutaProjectTargetInformation base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};

/* Target types
 *---------------------------------------------------------------------------*/

static AmpTargetInformation AmpTargetTypes[] = {
	{{N_("Unknown"), ANJUTA_TARGET_UNKNOWN,
	"text/plain"},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{N_("Program"), ANJUTA_TARGET_EXECUTABLE,
	"application/x-executable"},
	AM_TOKEN__PROGRAMS,
	"_PROGRAMS",
	"bin"},
	
	{{N_("Shared Library"), ANJUTA_TARGET_SHAREDLIB,
	"application/x-sharedlib"},
	AM_TOKEN__LTLIBRARIES,
	"_LTLIBRARIES",
	"lib"},
	
	{{N_("Static Library"), ANJUTA_TARGET_STATICLIB,
	"application/x-archive"},
	AM_TOKEN__LIBRARIES,
	"_LIBRARIES",
	"lib"},
	
	{{N_("Header Files"), ANJUTA_TARGET_UNKNOWN,
	"text/x-chdr"},
	AM_TOKEN__HEADERS,
	"_HEADERS",
	"include"},
	
	{{N_("Man Documentation"), ANJUTA_TARGET_UNKNOWN,
	"text/x-troff-man"},
	AM_TOKEN__MANS,
	"_MANS",
	"man"},
	
	{{N_("Miscellaneous Data"), ANJUTA_TARGET_UNKNOWN,
	"application/octet-stream"},
	AM_TOKEN__DATA,
	"_DATA",
	"data"},
	
	{{N_("Script"), ANJUTA_TARGET_EXECUTABLE,
	"text/x-shellscript"},
	AM_TOKEN__SCRIPTS,
	"_SCRIPTS",
	"bin"},
	
	{{N_("Info Documentation"), ANJUTA_TARGET_UNKNOWN,
	"application/x-tex-info"},
	AM_TOKEN__TEXINFOS,
	"_TEXINFOS",
	"info"},
	
	{{N_("Java Module"), ANJUTA_TARGET_JAVA,
	"application/x-java"},
	AM_TOKEN__JAVA,
	"_JAVA",
	NULL},
	
	{{N_("Python Module"), ANJUTA_TARGET_PYTHON,
	"application/x-python"},
	AM_TOKEN__PYTHON,
	"_PYTHON",
	NULL},
	
	{{N_("Lisp Module"), ANJUTA_TARGET_UNKNOWN,
	"text/plain"},
	AM_TOKEN__LISP,
	"_LISP",
	"lisp"},
	
	{{NULL, ANJUTA_TARGET_UNKNOWN,
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

gboolean
remove_list_item (AnjutaToken *token, AnjutaTokenStyle *user_style)
{
	AnjutaTokenStyle *style;
	AnjutaToken *space;

	DEBUG_PRINT ("remove list item");

	style = user_style != NULL ? user_style : anjuta_token_style_new (NULL," ","\\n",NULL,0);
	anjuta_token_style_update (style, anjuta_token_parent (token));
	
	anjuta_token_remove (token);
	space = anjuta_token_next_sibling (token);
	if (space && (anjuta_token_get_type (space) == ANJUTA_TOKEN_SPACE) && (anjuta_token_next (space) != NULL))
	{
		/* Remove following space */
		anjuta_token_remove (space);
	}
	else
	{
		space = anjuta_token_previous_sibling (token);
		if (space && (anjuta_token_get_type (space) == ANJUTA_TOKEN_SPACE) && (anjuta_token_previous (space) != NULL))
		{
			anjuta_token_remove (space);
		}
	}
	
	anjuta_token_style_format (style, anjuta_token_parent (token));
	if (user_style == NULL) anjuta_token_style_free (style);
	
	return TRUE;
}

static gboolean
add_list_item (AnjutaToken *list, AnjutaToken *token, AnjutaTokenStyle *user_style)
{
	AnjutaTokenStyle *style;
	AnjutaToken *space;

	style = user_style != NULL ? user_style : anjuta_token_style_new (NULL," ","\\n",NULL,0);
	anjuta_token_style_update (style, anjuta_token_parent (list));
	
	space = anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " ");
	space = anjuta_token_insert_after (list, space);
	anjuta_token_insert_after (space, token);

	anjuta_token_style_format (style, anjuta_token_parent (list));
	if (user_style == NULL) anjuta_token_style_free (style);
	
	return TRUE;
}

/* Automake parsing function
 *---------------------------------------------------------------------------*/

/* Remove invalid character according to automake rules */
gchar*
canonicalize_automake_variable (gchar *name)
{
	gchar *canon_name = g_strdup (name);
	gchar *ptr;
	
	for (ptr = canon_name; *ptr != '\0'; ptr++)
	{
		if (!g_ascii_isalnum (*ptr) && (*ptr != '@'))
		{
			*ptr = '_';
		}
	}

	return canon_name;
}

gboolean
split_automake_variable (gchar *name, gint *flags, gchar **module, gchar **primary)
{
	GRegex *regex;
	GMatchInfo *match_info;
	gint start_pos;
	gint end_pos;

	regex = g_regex_new ("(nobase_|notrans_)?"
	                     "(dist_|nodist_)?"
	                     "(noinst_|check_|man_|man[0-9al]_)?"
	                     "(.*_)?"
	                     "([^_]+)",
	                     G_REGEX_ANCHORED,
	                     G_REGEX_MATCH_ANCHORED,
	                     NULL);

	if (!g_regex_match (regex, name, G_REGEX_MATCH_ANCHORED, &match_info)) return FALSE;

	if (flags)
	{
		*flags = 0;
		g_match_info_fetch_pos (match_info, 1, &start_pos, &end_pos);
		if (start_pos >= 0)
		{
			if (*(name + start_pos + 2) == 'b') *flags |= AM_TARGET_NOBASE;
			if (*(name + start_pos + 2) == 't') *flags |= AM_TARGET_NOTRANS;
		}

		g_match_info_fetch_pos (match_info, 2, &start_pos, &end_pos);
		if (start_pos >= 0)
		{
			if (*(name + start_pos) == 'd') *flags |= AM_TARGET_DIST;
			if (*(name + start_pos) == 'n') *flags |= AM_TARGET_NODIST;
		}

		g_match_info_fetch_pos (match_info, 3, &start_pos, &end_pos);
		if (start_pos >= 0)
		{
			if (*(name + start_pos) == 'n') *flags |= AM_TARGET_NOINST;
			if (*(name + start_pos) == 'c') *flags |= AM_TARGET_CHECK;
			if (*(name + start_pos) == 'm')
			{
				gchar section = *(name + end_pos - 1);
				*flags |= AM_TARGET_MAN;
				if (section != 'n') *flags |= (section & 0x1F) << 7;
			}
		}
	}

	if (module)
	{
		g_match_info_fetch_pos (match_info, 4, &start_pos, &end_pos);
		if (start_pos >= 0)
		{
			*module = name + start_pos;
			*(name + end_pos - 1) = '\0';
		}
		else
		{
			*module = NULL;
		}
	}

	if (primary)
	{
		g_match_info_fetch_pos (match_info, 5, &start_pos, &end_pos);
		if (start_pos >= 0)
		{
			*primary = name + start_pos;
		}
		else
		{
			*primary = NULL;
		}
	}

	g_regex_unref (regex);

	return TRUE;
}

gchar*
ac_init_default_tarname (const gchar *name)
{
	gchar *tarname;

	/* Remove GNU prefix */
	if (strncmp (name, "GNU ", 4) == 0) name += 4;

	tarname = g_ascii_strdown (name, -1);
	g_strcanon (tarname, "abcdefghijklmnopqrstuvwxyz0123456789", '-');
	
	return tarname;
}

/* Config file objects
 *---------------------------------------------------------------------------*/

static AmpConfigFile*
amp_config_file_new (const gchar *pathname, GFile *project_root, AnjutaToken *token)
{
	AmpConfigFile *config;

	g_return_val_if_fail ((pathname != NULL) && (project_root != NULL), NULL);

	config = g_slice_new0(AmpConfigFile);
	config->file = g_file_resolve_relative_path (project_root, pathname);
	config->token = token;

	return config;
}

static void
amp_config_file_free (AmpConfigFile *config)
{
	if (config)
	{
		g_object_unref (config->file);
		g_slice_free (AmpConfigFile, config);
	}
}

/* Package objects
 *---------------------------------------------------------------------------*/

void
amp_package_set_version (AmpPackage *package, const gchar *compare, const gchar *version)
{
	g_return_if_fail (package != NULL);
	g_return_if_fail ((version == NULL) || (compare != NULL));

	g_free (package->version);
	package->version = version != NULL ? g_strconcat (compare, version, NULL) : NULL;
}

static AmpPackage*
amp_package_new (const gchar *name)
{
	AmpPackage *package;

	g_return_val_if_fail (name != NULL, NULL);
	
	package = g_slice_new0(AmpPackage); 
	package->name = g_strdup (name);

	return package;
}

static void
amp_package_free (AmpPackage *package)
{
	if (package)
	{
		g_free (package->name);
		g_free (package->version);
		g_slice_free (AmpPackage, package);
	}
}

/* Module objects
 *---------------------------------------------------------------------------*/

static AmpModule*
amp_module_new (AnjutaToken *token)
{
	AmpModule *module;
	
	module = g_slice_new0(AmpModule); 
	module->module = token;

	return module;
}

static void
amp_module_free (AmpModule *module)
{
	if (module)
	{
		g_list_foreach (module->packages, (GFunc)amp_package_free, NULL);
		g_slice_free (AmpModule, module);
	}
}

static void
amp_project_new_module_hash (AmpProject *project)
{
	project->modules = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)amp_module_free);
}

static void
amp_project_free_module_hash (AmpProject *project)
{
	if (project->modules != NULL)
	{
		g_hash_table_destroy (project->modules);
		project->modules = NULL;
	}
}

/* Property objects
 *---------------------------------------------------------------------------*/

static AmpProperty*
amp_property_new (AnjutaToken *token)
{
	AmpProperty *prop;
	AnjutaToken *arg;
	
	prop = g_slice_new0(AmpProperty); 
	prop->ac_init = token;

	arg = anjuta_token_next_child (token);
	arg = anjuta_token_get_next_arg (arg, &prop->name);
	arg = anjuta_token_get_next_arg (arg, &prop->version);
	arg = anjuta_token_get_next_arg (arg, &prop->bug_report);
	arg = anjuta_token_get_next_arg (arg, &prop->tarname);
	arg = anjuta_token_get_next_arg (arg, &prop->url);
	
	return prop;
}

static void
amp_property_free (AmpProperty *prop)
{
	g_free (prop->name);
	g_free (prop->version);
	g_free (prop->bug_report);
	g_free (prop->tarname);
    g_slice_free (AmpProperty, prop);
}

/* Group objects
 *---------------------------------------------------------------------------*/

static void
amp_group_add_token (AmpGroup *node, AnjutaToken *token, AmpGroupTokenCategory category)
{
    AmpGroupData *group;
	
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	group = AMP_GROUP_DATA (node);
	group->tokens[category] = g_list_prepend (group->tokens[category], token);
}

static GList *
amp_group_get_token (AmpGroup *node, AmpGroupTokenCategory category)
{
    AmpGroupData *group;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	group = AMP_GROUP_DATA (node);
	return group->tokens[category];
}

static void
amp_group_set_dist_only (AmpGroup *node, gboolean dist_only)
{
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	AMP_GROUP_DATA (node)->dist_only = dist_only;
}

static AnjutaTokenFile*
amp_group_set_makefile (AmpGroup *node, GFile *makefile)
{
    AmpGroupData *group;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	group = AMP_GROUP_DATA (node);
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

static AmpGroup*
amp_group_new (GFile *file, gboolean dist_only)
{
    AmpGroupData *group = NULL;

	g_return_val_if_fail (file != NULL, NULL);
	
	group = g_slice_new0(AmpGroupData); 
	group->base.node.type = ANJUTA_PROJECT_GROUP;
	group->base.directory = g_object_ref (file);
	group->dist_only = dist_only;

    return g_node_new (group);
}

static void
amp_group_free (AmpGroup *node)
{
    AmpGroupData *group = (AmpGroupData *)node->data;
	gint i;
	
	if (group->base.directory) g_object_unref (group->base.directory);
	if (group->tfile) anjuta_token_file_free (group->tfile);
	if (group->makefile) g_object_unref (group->makefile);
	for (i = 0; i < AM_GROUP_TOKEN_LAST; i++)
	{
		if (group->tokens[i] != NULL) g_list_free (group->tokens[i]);
	}
    g_slice_free (AmpGroupData, group);

	g_node_destroy (node);
}

/* Target objects
 *---------------------------------------------------------------------------*/

static void
amp_target_add_token (AmpGroup *node, AnjutaToken *token)
{
    AmpTargetData *target;
	
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	target = AMP_TARGET_DATA (node);
	target->tokens = g_list_prepend (target->tokens, token);
}

static GList *
amp_target_get_token (AmpGroup *node)
{
    AmpTargetData *target;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	target = AMP_TARGET_DATA (node);
	return target->tokens;
}


static AmpTarget*
amp_target_new (const gchar *name, AnjutaProjectTargetType type, const gchar *install, gint flags)
{
    AmpTargetData *target = NULL;

	target = g_slice_new0(AmpTargetData); 
	target->base.node.type = ANJUTA_PROJECT_TARGET;
	target->base.name = g_strdup (name);
	target->base.type = type;
	target->install = g_strdup (install);
	target->flags = flags;

    return g_node_new (target);
}

static void
amp_target_free (AmpTarget *node)
{
    AmpTargetData *target = AMP_TARGET_DATA (node);
	
    g_free (target->base.name);
    g_free (target->install);
    g_slice_free (AmpTargetData, target);

	g_node_destroy (node);
}

/* Source objects
 *---------------------------------------------------------------------------*/

static AmpSource*
amp_source_new (GFile *file)
{
    AmpSourceData *source = NULL;

	source = g_slice_new0(AmpSourceData); 
	source->base.node.type = ANJUTA_PROJECT_SOURCE;
	source->base.file = g_object_ref (file);

    return g_node_new (source);
}

static void
amp_source_free (AmpSource *node)
{
    AmpSourceData *source = AMP_SOURCE_DATA (node);
	
    g_object_unref (source->base.file);
    g_slice_free (AmpSourceData, source);

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
	AmpProject *project = data;

	g_return_if_fail (project != NULL && AMP_IS_PROJECT (project));

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* monitor will be removed here... is this safe? */
			amp_project_reload (project, NULL);
			g_signal_emit_by_name (G_OBJECT (project), "project-updated");
			break;
		default:
			break;
	}
}


static void
monitor_add (AmpProject *project, GFile *file)
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
monitors_remove (AmpProject *project)
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
	AmpGroup *group_node = value;
	AmpProject *project = user_data;

	monitor_add (project, AMP_GROUP_DATA(group_node)->base.directory);
}

static void
monitors_setup (AmpProject *project)
{
	g_return_if_fail (project != NULL);

	monitors_remove (project);
	
	/* setup monitors hash */
	project->monitors = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
						   (GDestroyNotify) g_file_monitor_cancel);

	monitor_add (project, anjuta_token_file_get_file (project->configure_file));
	if (project->groups)
		g_hash_table_foreach (project->groups, group_hash_foreach_monitor, project);
}


/*
 * ---------------- Data structures managment
 */

static void
amp_dump_node (GNode *g_node)
{
	gchar *name = NULL;
	
	switch (AMP_NODE_DATA (g_node)->type) {
		case ANJUTA_PROJECT_GROUP:
			name = g_file_get_uri (AMP_GROUP_DATA (g_node)->base.directory);
			DEBUG_PRINT ("GROUP: %s", name);
			break;
		case ANJUTA_PROJECT_TARGET:
			name = g_strdup (AMP_TARGET_DATA (g_node)->base.name);
			DEBUG_PRINT ("TARGET: %s", name);
			break;
		case ANJUTA_PROJECT_SOURCE:
			name = g_file_get_uri (AMP_SOURCE_DATA (g_node)->base.file);
			DEBUG_PRINT ("SOURCE: %s", name);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	g_free (name);
}

static gboolean 
foreach_node_destroy (GNode    *g_node,
		      gpointer  data)
{
	switch (AMP_NODE_DATA (g_node)->type) {
		case ANJUTA_PROJECT_GROUP:
			//g_hash_table_remove (project->groups, g_file_get_uri (AMP_GROUP_NODE (g_node)->file));
			amp_group_free (g_node);
			break;
		case ANJUTA_PROJECT_TARGET:
			amp_target_free (g_node);
			break;
		case ANJUTA_PROJECT_SOURCE:
			//g_hash_table_remove (project->sources, AMP_NODE (g_node)->id);
			amp_source_free (g_node);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	

	return FALSE;
}

static void
project_node_destroy (AmpProject *project, GNode *g_node)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (AMP_IS_PROJECT (project));
	
	if (g_node) {
		/* free each node's data first */
		g_node_traverse (g_node,
				 G_POST_ORDER, G_TRAVERSE_ALL, -1,
				 foreach_node_destroy, project);

		/* now destroy the tree itself */
		//g_node_destroy (g_node);
	}
}

static gboolean
project_reload_property (AmpProject *project)
{
	AnjutaToken *ac_init_tok;
	AnjutaToken *sequence;
	AnjutaToken *init;

	ac_init_tok = anjuta_token_new_static (AC_TOKEN_AC_INIT, NULL);
	                                       
	sequence = anjuta_token_file_first (project->configure_file);
	if (anjuta_token_match (ac_init_tok, ANJUTA_SEARCH_INTO, sequence, &init))
	{
		g_message ("find ac_init");
		anjuta_token_style_update (project->arg_list, init);
		project->property = amp_property_new (init);
	}

	return TRUE;                                       
}

static void
project_reload_packages   (AmpProject *project)
{
	AnjutaToken *pkg_check_tok;
	AnjutaToken *sequence;
	AmpAcScanner *scanner = NULL;
	
	pkg_check_tok = anjuta_token_new_static (AC_TOKEN_PKG_CHECK_MODULES, "PKG_CHECK_MODULES(");
	
    sequence = anjuta_token_file_first (project->configure_file);
	for (;;)
	{
		AnjutaToken *module;
		AnjutaToken *arg;
		gchar *value;
		AmpModule *mod;
		AmpPackage *pack;
		gchar *compare;
		
		if (!anjuta_token_match (pkg_check_tok, ANJUTA_SEARCH_INTO, sequence, &module)) break;

		arg = anjuta_token_next_child (module);	/* Name */

		value = anjuta_token_evaluate (arg);
		mod = amp_module_new (arg);
		mod->packages = NULL;
		g_hash_table_insert (project->modules, value, mod);

		arg = anjuta_token_next_sibling (arg);	/* Separator */

		arg = anjuta_token_next_sibling (arg);	/* Package list */
		if (scanner == NULL) scanner = amp_ac_scanner_new ();
		amp_ac_scanner_parse_token (scanner, arg, AC_SPACE_LIST_STATE, NULL);
		
		pack = NULL;
		compare = NULL;
		for (arg = anjuta_token_next_child (arg); arg != NULL; arg = anjuta_token_next_sibling (arg))
		{
			switch (anjuta_token_get_type (arg))
			{
				case ANJUTA_TOKEN_START:
				case ANJUTA_TOKEN_NEXT:
				case ANJUTA_TOKEN_LAST:
				case ANJUTA_TOKEN_JUNK:
					continue;
				default:
					break;
			}
			
			value = anjuta_token_evaluate (arg);
			if (value == NULL) continue;		/* Empty value, a comment of a quote by example */

			if ((pack != NULL) && (compare != NULL))
			{
				amp_package_set_version (pack, compare, value);
				g_free (value);
				g_free (compare);
				pack = NULL;
				compare = NULL;
			}
			else if ((pack != NULL) && (anjuta_token_get_type (arg) == ANJUTA_TOKEN_OPERATOR))
			{
				compare = value;
			}
			else
			{
				pack = amp_package_new (value);
				mod->packages = g_list_prepend (mod->packages, pack);
				g_free (value);
				compare = NULL;
			}
		}
		mod->packages = g_list_reverse (mod->packages);

		sequence = anjuta_token_next_sibling (module);
	}
	anjuta_token_free (pkg_check_tok);
	if (scanner) amp_ac_scanner_free (scanner);
}

/* Add a GFile in the list for each makefile in the token list */
void
amp_project_add_config_files (AmpProject *project, AnjutaToken *list)
{
	AnjutaToken* arg;

	for (arg = anjuta_token_next_child (list); arg != NULL; arg = anjuta_token_next_sibling (arg))
	{
		gchar *value;
		AmpConfigFile *cfg;

		switch (anjuta_token_get_type (arg))
		{
			case ANJUTA_TOKEN_START:
			case ANJUTA_TOKEN_NEXT:
			case ANJUTA_TOKEN_LAST:
			case ANJUTA_TOKEN_JUNK:
				continue;
			default:
				break;
		}
			
		value = anjuta_token_evaluate (arg);
		if (value == NULL) continue;
		
		cfg = amp_config_file_new (value, project->root_file, arg);
		g_hash_table_insert (project->configs, cfg->file, cfg);
		g_free (value);
	}
}							   
                           
static gboolean
project_list_config_files (AmpProject *project)
{
	AnjutaToken *config_files_tok;
	AnjutaToken *sequence;
	AmpAcScanner *scanner = NULL;

	//g_message ("load config project %p root file %p", project, project->root_file);	
	/* Search the new AC_CONFIG_FILES macro */
	config_files_tok = anjuta_token_new_static (AC_TOKEN_AC_CONFIG_FILES, NULL);

    sequence = anjuta_token_file_first (project->configure_file);
	while (sequence != NULL)
	{
		AnjutaToken *arg;

		if (!anjuta_token_match (config_files_tok, ANJUTA_SEARCH_INTO, sequence, &sequence)) break;
		arg = anjuta_token_next_child (sequence);	/* List */
		if (scanner == NULL) scanner = amp_ac_scanner_new ();
		amp_ac_scanner_parse_token (scanner, arg, AC_SPACE_LIST_STATE, NULL);
		amp_project_add_config_files (project, arg);
		sequence = anjuta_token_next_sibling (sequence);
	}
	
	/* Search the old AC_OUTPUT macro */
    anjuta_token_free(config_files_tok);
    config_files_tok = anjuta_token_new_static (AC_TOKEN_OBSOLETE_AC_OUTPUT, NULL);
		
    sequence = anjuta_token_file_first (project->configure_file);
	while (sequence != NULL)
	{
		AnjutaToken *arg;

		if (!anjuta_token_match (config_files_tok, ANJUTA_SEARCH_INTO, sequence, &sequence)) break;
		arg = anjuta_token_next_child (sequence);	/* List */
		if (scanner == NULL) scanner = amp_ac_scanner_new ();
		amp_ac_scanner_parse_token (scanner, arg, AC_SPACE_LIST_STATE, NULL);
		amp_project_add_config_files (project, arg);
		sequence = anjuta_token_next_sibling (sequence);
	}
	
	if (scanner) amp_ac_scanner_free (scanner);
	anjuta_token_free (config_files_tok);

	return TRUE;
}

static void
find_target (GNode *node, gpointer data)
{
	if (AMP_NODE_DATA (node)->type == ANJUTA_PROJECT_TARGET)
	{
		if (strcmp (AMP_TARGET_DATA (node)->base.name, *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(GNode **)data = node;

			return;
		}
	}
}

static void
find_canonical_target (GNode *node, gpointer data)
{
	if (AMP_NODE_DATA (node)->type == ANJUTA_PROJECT_TARGET)
	{
		gchar *canon_name = canonicalize_automake_variable (AMP_TARGET_DATA (node)->base.name);	
		DEBUG_PRINT ("compare canon %s vs %s node %p", canon_name, *(gchar **)data, node);
		if (strcmp (canon_name, *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(GNode **)data = node;
			g_free (canon_name);

			return;
		}
		g_free (canon_name);
	}
}

static AnjutaToken*
project_load_target (AmpProject *project, AnjutaToken *start, GNode *parent, GHashTable *orphan_sources)
{
	AnjutaToken *arg;
	AnjutaProjectTargetType type = NULL;
	gchar *install;
	gchar *name;
	gint flags;
	AmpTargetInformation *targets = AmpTargetTypes; 

	type = (AnjutaProjectTargetType)targets;
	while (targets->base.name != NULL)
	{
		if (anjuta_token_get_type (start) == targets->token)
		{
			type = (AnjutaProjectTargetType)targets;
			break;
		}
		targets++;
	}

	name = anjuta_token_get_value (start);
	split_automake_variable (name, &flags, &install, NULL);
	g_free (name);

	amp_group_add_token (parent, start, AM_GROUP_TARGET);
	
	arg = anjuta_token_next_sibling (start);		/* Get variable data */
	if (arg == NULL) return NULL;
	if (anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) arg = anjuta_token_next_sibling (arg); /* Skip space */
	if (arg == NULL) return NULL;
	arg = anjuta_token_next_sibling (arg);			/* Skip equal */
	if (arg == NULL) return NULL;

	for (arg = anjuta_token_next_child (arg); arg != NULL; arg = anjuta_token_next_sibling (arg))
	{
		gchar *value;
		gchar *canon_id;
		AmpTarget *target;
		GList *sources;
		gchar *orig_key;
		gpointer find;

		if ((anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) || (anjuta_token_get_type (arg) == ANJUTA_TOKEN_COMMENT)) continue;
			
		value = anjuta_token_evaluate (arg);
		canon_id = canonicalize_automake_variable (value);		
		
		/* Check if target already exists */
		find = value;
		g_node_children_foreach (parent, G_TRAVERSE_ALL, find_target, &find);
		if ((gchar *)find != value)
		{
			/* Find target */
			g_free (canon_id);
			g_free (value);
			continue;
		}

		/* Create target */
		target = amp_target_new (value, type, install, flags);
		amp_target_add_token (target, arg);
		g_node_append (parent, target);
		DEBUG_PRINT ("create target %p name %s", target, value);

		/* Check if there are source availables */
		if (g_hash_table_lookup_extended (orphan_sources, canon_id, (gpointer *)&orig_key, (gpointer *)&sources))
		{
			GList *src;
			g_hash_table_steal (orphan_sources, canon_id);
			for (src = sources; src != NULL; src = g_list_next (src))
			{
				AmpSource *source = src->data;

				g_node_prepend (target, source);
			}
			g_free (orig_key);
			g_list_free (sources);
		}

		g_free (canon_id);
		g_free (value);
	}

	return NULL;
}

static AnjutaToken*
project_load_sources (AmpProject *project, AnjutaToken *start, GNode *parent, GHashTable *orphan_sources)
{
	AnjutaToken *arg;
	AmpGroupData *group = AMP_GROUP_DATA (parent);
	GFile *parent_file = g_object_ref (group->base.directory);
	gchar *target_id = NULL;
	GList *orphan = NULL;

	target_id = anjuta_token_get_value (start);
	if (target_id)
	{
		gchar *end = strrchr (target_id, '_');
		if (end)
		{
			*end = '\0';
		}
	}

	if (target_id)
	{
		gpointer find;
		
		find = target_id;
		DEBUG_PRINT ("search for canonical %s", target_id);
		g_node_children_foreach (parent, G_TRAVERSE_ALL, find_canonical_target, &find);
		parent = (gchar *)find != target_id ? (GNode *)find : NULL;

		arg = anjuta_token_next_sibling (start);		/* Get variable data */
		if (arg == NULL) return NULL;
		if (anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) arg = anjuta_token_next_sibling (arg); /* Skip space */
		if (arg == NULL) return NULL;
		arg = anjuta_token_next_sibling (arg);			/* Skip equal */
		if (arg == NULL) return NULL;

		for (arg = anjuta_token_next_child (arg); arg != NULL; arg = anjuta_token_next_sibling (arg))
		{
			gchar *value;
			AmpSource *source;
			GFile *src_file;
		
			if ((anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) || (anjuta_token_get_type (arg) == ANJUTA_TOKEN_COMMENT)) continue;
			
			value = anjuta_token_evaluate (arg);

			/* Create source */
			src_file = g_file_get_child (parent_file, value);
			source = amp_source_new (src_file);
			g_object_unref (src_file);
			AMP_SOURCE_DATA(source)->token = arg;

			if (parent == NULL)
			{
				/* Add in orphan list */
				orphan = g_list_prepend (orphan, source);
			}
			else
			{
				DEBUG_PRINT ("add target child %p", parent);
				/* Add as target child */
				g_node_append (parent, source);
			}

			g_free (value);
		}
		
		if (parent == NULL)
		{
			gchar *orig_key;
			GList *orig_sources;

			if (g_hash_table_lookup_extended (orphan_sources, target_id, (gpointer *)&orig_key, (gpointer *)&orig_sources))
			{
				g_hash_table_steal (orphan_sources, target_id);
				orphan = g_list_concat (orphan, orig_sources);	
				g_free (orig_key);
			}
			g_hash_table_insert (orphan_sources, target_id, orphan);
		}
		else
		{
			g_free (target_id);
		}
	}

	g_object_unref (parent_file);

	return NULL;
}

static AmpGroup* project_load_makefile (AmpProject *project, GFile *file, AmpGroup *parent, gboolean dist_only);

static void
project_load_subdirs (AmpProject *project, AnjutaToken *start, AmpGroup *parent, gboolean dist_only)
{
	AnjutaToken *arg;

	arg = anjuta_token_next_sibling (start);		/* Get variable data */
	if (arg == NULL) return;
	if (anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) arg = anjuta_token_next_sibling (arg); /* Skip space */
	if (arg == NULL) return;
	arg = anjuta_token_next_sibling (arg);			/* Skip equal */
	if (arg == NULL) return;

	for (arg = anjuta_token_next_child (arg); arg != NULL; arg = anjuta_token_next_sibling (arg))
	{
		gchar *value;
		
		if ((anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) || (anjuta_token_get_type (arg) == ANJUTA_TOKEN_COMMENT)) continue;
			
		value = anjuta_token_evaluate (arg);
		
		/* Skip ., it is a special case, used to defined build order */
		if (strcmp (value, ".") != 0)
		{
			GFile *subdir;
			gchar *group_id;
			AmpGroup *group;

			subdir = g_file_resolve_relative_path (AMP_GROUP_DATA (parent)->base.directory, value);
			
			/* Look for already existing group */
			group_id = g_file_get_uri (subdir);
			group = g_hash_table_lookup (project->groups, group_id);
			g_free (group_id);

			if (group != NULL)
			{
				/* Already existing group, mark for built if needed */
				if (!dist_only) amp_group_set_dist_only (group, FALSE);
			}
			else
			{
				/* Create new group */
				group = project_load_makefile (project, subdir, parent, dist_only);
			}
			amp_group_add_token (group, arg, dist_only ? AM_GROUP_TOKEN_DIST_SUBDIRS : AM_GROUP_TOKEN_SUBDIRS);
			g_object_unref (subdir);
		}
		g_free (value);
	}
}

static void
free_source_list (GList *source_list)
{
	g_list_foreach (source_list, (GFunc)amp_source_free, NULL);
	g_list_free (source_list);
}

static void
remove_config_file (gpointer data, GObject *object, gboolean is_last_ref)
{
	if (is_last_ref)
	{
		AmpProject *project = (AmpProject *)data;
		g_hash_table_remove (project->files, anjuta_token_file_get_file (ANJUTA_TOKEN_FILE (object)));
	}
}

static AmpGroup*
project_load_makefile (AmpProject *project, GFile *file, GNode *parent, gboolean dist_only)
{
	GHashTable *orphan_sources = NULL;
	const gchar **filename;
	AmpAmScanner *scanner;
	AmpGroup *group;
	AnjutaToken *significant_tok;
	AnjutaToken *arg;
	AnjutaTokenFile *tfile;
	GFile *makefile = NULL;
	gboolean found;

	/* Create group */
	group = amp_group_new (file, dist_only);
	g_hash_table_insert (project->groups, g_file_get_uri (file), group);
	if (parent == NULL)
	{
		project->root_node = group;
	}
	else
	{
		g_node_append (parent, group);
	}
		
	/* Find makefile name
	 * It has to be in the config_files list with .am extension */
	for (filename = valid_am_makefiles; *filename != NULL; filename++)
	{
		makefile = g_file_get_child (file, *filename);
		if (file_type (file, *filename) == G_FILE_TYPE_REGULAR)
		{
			gchar *final_filename = g_strdup (*filename);
			gchar *ptr;
			GFile *final_file;
			AmpConfigFile *config;

			ptr = g_strrstr (final_filename,".am");
			if (ptr != NULL) *ptr = '\0';
			final_file = g_file_get_child (file, final_filename);
			g_free (final_filename);
			
			config = g_hash_table_lookup (project->configs, final_file);
			g_object_unref (final_file);
			if (config != NULL)
			{
				amp_group_add_token (group, config->token, AM_GROUP_TOKEN_CONFIGURE);
				break;
			}
		}
		g_object_unref (makefile);
	}

	if (*filename == NULL)
	{
		/* Unable to find automake file */
		return group;
	}
	
	/* Parse makefile.am */	
	DEBUG_PRINT ("Parse: %s", g_file_get_uri (makefile));
	tfile = amp_group_set_makefile (group, makefile);
	g_hash_table_insert (project->files, makefile, tfile);
	g_object_add_toggle_ref (G_OBJECT (tfile), remove_config_file, project);
	scanner = amp_am_scanner_new ();
	amp_am_scanner_parse (scanner, tfile, NULL);
	amp_am_scanner_free (scanner);

	/* Find significant token */
	significant_tok = anjuta_token_new_static (ANJUTA_TOKEN_STATEMENT, NULL);
	
	arg = anjuta_token_file_first (AMP_GROUP_DATA (group)->tfile);
	//anjuta_token_old_dump_range (arg, NULL);

	/* Create hash table for sources list */
	orphan_sources = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)free_source_list);
	
	for (found = anjuta_token_match (significant_tok, ANJUTA_SEARCH_INTO, arg, &arg); found; found = anjuta_token_match (significant_tok, ANJUTA_SEARCH_INTO, anjuta_token_next_sibling (arg), &arg))
	{
		AnjutaToken *name = anjuta_token_next_child (arg);
		
		switch (anjuta_token_get_type (name))
		{
		case AM_TOKEN_SUBDIRS:
				project_load_subdirs (project, name, group, FALSE);
				break;
		case AM_TOKEN_DIST_SUBDIRS:
				project_load_subdirs (project, name, group, TRUE);
				break;
		case AM_TOKEN__DATA:
		case AM_TOKEN__HEADERS:
		case AM_TOKEN__LIBRARIES:
		case AM_TOKEN__LISP:
		case AM_TOKEN__LTLIBRARIES:
		case AM_TOKEN__MANS:
		case AM_TOKEN__PROGRAMS:
		case AM_TOKEN__PYTHON:
		case AM_TOKEN__JAVA:
		case AM_TOKEN__SCRIPTS:
		case AM_TOKEN__TEXINFOS:
				project_load_target (project, name, group, orphan_sources);
				break;
		case AM_TOKEN__SOURCES:
				project_load_sources (project, name, group, orphan_sources);
				break;
		}
	}

	/* Free unused sources files */
	g_hash_table_destroy (orphan_sources);

	return group;
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
amp_project_reload (AmpProject *project, GError **error) 
{
	AmpAcScanner *scanner;
	GFile *root_file;
	GFile *configure_file;
	gboolean ok;
	GError *err = NULL;

	/* Unload current project */
	root_file = g_object_ref (project->root_file);
	amp_project_unload (project);
	project->root_file = root_file;
	DEBUG_PRINT ("reload project %p root file %p", project, project->root_file);

	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_object_unref);
	project->configs = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, NULL, amp_config_file_free);

	/* Initialize list styles */
	project->space_list = anjuta_token_style_new (NULL, " ", "\\n", NULL, 0);
	project->arg_list = anjuta_token_style_new (NULL, ", ", ",\\n ", ")", 0);
	
	/* Find configure file */
	if (file_type (root_file, "configure.ac") == G_FILE_TYPE_REGULAR)
	{
		configure_file = g_file_get_child (root_file, "configure.ac");
	}
	else if (file_type (root_file, "configure.in") == G_FILE_TYPE_REGULAR)
	{
		configure_file = g_file_get_child (root_file, "configure.in");
	}
	else
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));

		return FALSE;
	}
	
	/* Parse configure */	
	project->configure_file = anjuta_token_file_new (configure_file);
	g_hash_table_insert (project->files, configure_file, project->configure_file);
	g_object_add_toggle_ref (G_OBJECT (project->configure_file), remove_config_file, project);
	scanner = amp_ac_scanner_new ();
	ok = amp_ac_scanner_parse (scanner, project->configure_file, &err);
	amp_ac_scanner_free (scanner);
	if (!ok)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             	IANJUTA_PROJECT_ERROR_PROJECT_MALFORMED,
		    			err == NULL ? _("Unable to parse project file") : err->message);
		if (err != NULL) g_error_free (err);

		return FALSE;
	}
		     
	monitors_setup (project);

	project_reload_property (project);
	
	amp_project_new_module_hash (project);
	project_reload_packages (project);
	
	/* Load all makefiles recursively */
	project_list_config_files (project);
	if (project_load_makefile (project, project->root_file, NULL, FALSE) == NULL)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));

		ok = FALSE;
	}
	
	return ok;
}

gboolean
amp_project_load (AmpProject  *project,
    GFile *directory,
	GError     **error)
{
	g_return_val_if_fail (directory != NULL, FALSE);

	project->root_file = g_object_ref (directory);
	if (!amp_project_reload (project, error))
	{
		g_object_unref (project->root_file);
		project->root_file = NULL;
	}

	return project->root_file != NULL;
}

void
amp_project_unload (AmpProject *project)
{
	monitors_remove (project);
	
	/* project data */
	project_node_destroy (project, project->root_node);
	project->root_node = NULL;

	if (project->configure_file)	g_object_unref (G_OBJECT (project->configure_file));
	project->configure_file = NULL;

	if (project->root_file) g_object_unref (project->root_file);
	project->root_file = NULL;

	if (project->property) amp_property_free (project->property);
	project->property = NULL;
	
	/* shortcut hash tables */
	if (project->groups) g_hash_table_destroy (project->groups);
	if (project->files) g_hash_table_destroy (project->files);
	if (project->configs) g_hash_table_destroy (project->configs);
	project->groups = NULL;
	project->files = NULL;
	project->configs = NULL;

	/* List styles */
	if (project->space_list) anjuta_token_style_free (project->space_list);
	if (project->arg_list) anjuta_token_style_free (project->arg_list);
	
	amp_project_free_module_hash (project);
}

gint
amp_project_probe (GFile *file,
	    GError     **error)
{
	gint probe;
	gboolean dir;

	dir = (file_type (file, NULL) == G_FILE_TYPE_DIRECTORY);
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
		for (makefile = valid_am_makefiles; *makefile != NULL; makefile++)
		{
			if (file_type (file, *makefile) == G_FILE_TYPE_REGULAR)
			{
				probe = TRUE;
				break;
			}
		}

		if (probe)
		{
			probe = ((file_type (file, "configure.ac") == G_FILE_TYPE_REGULAR) ||
			 				(file_type (file, "configure.in") == G_FILE_TYPE_REGULAR));
		}
	}

	return probe ? IANJUTA_PROJECT_PROBE_PROJECT_FILES : 0;
}

AmpGroup* 
amp_project_add_group (AmpProject  *project,
		AmpGroup *parent,
		const gchar *name,
		GError     **error)
{
	AmpGroup *last;
	AmpGroup *child;
	GFile *directory;
	GFile *makefile;
	AnjutaToken* token;
	AnjutaToken* prev_token;
	GList *token_list;
	gchar *basename;
	gchar *uri;
	AnjutaTokenFile* tfile;
	
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (parent != NULL, NULL);

	/* Validate group name */
	if (!name || strlen (name) <= 0)
	{
		error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			   _("Please specify group name"));
		return NULL;
	}
	{
		gboolean failed = FALSE;
		const gchar *ptr = name;
		while (*ptr) {
			if (!isalnum (*ptr) && *ptr != '.' && *ptr != '-' &&
			    *ptr != '_')
				failed = TRUE;
			ptr++;
		}
		if (failed) {
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Group name can only contain alphanumeric, '_', '-' or '.' characters"));
			return NULL;
		}
	}

	/* Check that the new group doesn't already exist */
	directory = g_file_get_child (AMP_GROUP_DATA (parent)->base.directory, name);
	uri = g_file_get_uri (directory);
	if (g_hash_table_lookup (project->groups, uri) != NULL)
	{
		g_free (uri);
		error_set (error, IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			_("Group already exists"));
		return NULL;
	}
	
	/* Add group node in project tree */
	child = amp_group_new (directory, FALSE);
	g_hash_table_insert (project->groups, uri, child);
	g_object_unref (directory);
	g_node_append (parent, child);

	/* Create directory */
	g_file_make_directory (directory, NULL, NULL);

	/* Create Makefile.am */
	basename = g_file_get_basename (AMP_GROUP_DATA (parent)->makefile);
	if (basename != NULL)
	{
		makefile = g_file_get_child (directory, basename);
		g_free (basename);
	}
	else
	{
		makefile = g_file_get_child (directory, "Makefile.am");
	}
	g_file_replace_contents (makefile, "", 0, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, NULL);
	tfile = amp_group_set_makefile (child, makefile);
	g_hash_table_insert (project->files, makefile, tfile);
	g_object_add_toggle_ref (G_OBJECT (tfile), remove_config_file, project);

	/* Add in configure */
	token_list= amp_group_get_token (parent, AM_GROUP_TOKEN_CONFIGURE);
	if (token_list != NULL)
	{
		gchar *relative_make;
		gchar *ext;
		AnjutaTokenStyle *style;
		
		prev_token = (AnjutaToken *)token_list->data;

		relative_make = g_file_get_relative_path (project->root_file, makefile);
		ext = relative_make + strlen (relative_make) - 3;
		if (strcmp (ext, ".am") == 0)
		{
			*ext = '\0';
		}
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED,  relative_make);
		g_free (relative_make);
		
		style = anjuta_token_style_new (NULL," ","\\n",NULL,0);
		add_list_item (prev_token, token, style);
		anjuta_token_style_free (style);
	}
	
	/* Add in Makefile.am */
	for (last = g_node_prev_sibling (child); (last != NULL) && (AMP_NODE_DATA (last)->type != ANJUTA_PROJECT_GROUP); last = g_node_prev_sibling (last));
	if (last == NULL)
	{
		AnjutaToken *prev_token;
		gint space = 0;
		AnjutaToken *list;

		/* Skip comment and one space at the beginning */
		for (prev_token = anjuta_token_next_child (anjuta_token_file_first (AMP_GROUP_DATA (parent)->tfile)); prev_token != NULL; prev_token = anjuta_token_next_sibling (prev_token))
		{
			switch (anjuta_token_get_type (prev_token))
			{
				case ANJUTA_TOKEN_COMMENT:
					space = 0;
					continue;
				case ANJUTA_TOKEN_SPACE:
					if (space == 0)
					{
						space = 1;
						continue;
					}
					break;
				default:
					break;
			}
			break;
		}

		if (prev_token == NULL)
		{
			prev_token = anjuta_token_next_child (anjuta_token_file_first (AMP_GROUP_DATA (parent)->tfile));
			if (prev_token)
			{
				prev_token = anjuta_token_file_last (AMP_GROUP_DATA (parent)->tfile);
			}
		}

		token = anjuta_token_new_static (ANJUTA_TOKEN_STATEMENT | ANJUTA_TOKEN_ADDED, NULL);
		if (prev_token == NULL)
		{
			prev_token = anjuta_token_insert_child (anjuta_token_file_first (AMP_GROUP_DATA (parent)->tfile), token);
		}
		else
		{
			prev_token = anjuta_token_insert_after (prev_token, token);
		}
		list = prev_token;
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (AM_TOKEN_SUBDIRS | ANJUTA_TOKEN_ADDED, "SUBDIRS"));
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_OPERATOR | ANJUTA_TOKEN_ADDED, "="));
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL));
		token = prev_token;
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name));
		anjuta_token_merge (token, prev_token);
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, "\n"));
		anjuta_token_merge (list, prev_token);
	}
	else
	{
		token_list = amp_group_get_token (parent, AM_GROUP_TOKEN_SUBDIRS);
		if (token_list != NULL)
		{
			prev_token = (AnjutaToken *)token_list->data;
		
			token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);
			add_list_item (prev_token, token, NULL);
		}
	}
	amp_group_add_token (child, token, AM_GROUP_TOKEN_SUBDIRS);

	return child;
}

void 
amp_project_remove_group (AmpProject  *project,
		   AmpGroup *group,
		   GError     **error)
{
	GList *token_list;

	if (AMP_NODE_DATA (group)->type != ANJUTA_PROJECT_GROUP) return;
	
	for (token_list = amp_group_get_token (group, AM_GROUP_TOKEN_CONFIGURE); token_list != NULL; token_list = g_list_next (token_list))
	{
		remove_list_item ((AnjutaToken *)token_list->data, NULL);
	}
	for (token_list = amp_group_get_token (group, AM_GROUP_TOKEN_SUBDIRS); token_list != NULL; token_list = g_list_next (token_list))
	{
		remove_list_item ((AnjutaToken *)token_list->data, NULL);
	}
	for (token_list = amp_group_get_token (group, AM_GROUP_TOKEN_DIST_SUBDIRS); token_list != NULL; token_list = g_list_next (token_list))
	{
		remove_list_item ((AnjutaToken *)token_list->data, NULL);
	}

	amp_group_free (group);
}

AmpTarget*
amp_project_add_target (AmpProject  *project,
		 AmpGroup *parent,
		 const gchar *name,
		 AnjutaProjectTargetType type,
		 GError     **error)
{
	AmpTarget *child;
	AnjutaToken* token;
	AnjutaToken* prev_token;
	AnjutaToken *list;
	gchar *targetname;
	gchar *find;
	GList *last;
	
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (parent != NULL, NULL);

	/* Validate target name */
	if (!name || strlen (name) <= 0)
	{
		error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			   _("Please specify target name"));
		return NULL;
	}
	{
		gboolean failed = FALSE;
		const gchar *ptr = name;
		while (*ptr) {
			if (!isalnum (*ptr) && *ptr != '.' && *ptr != '-' &&
			    *ptr != '_')
				failed = TRUE;
			ptr++;
		}
		if (failed) {
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Target name can only contain alphanumeric, '_', '-' or '.' characters"));
			return NULL;
		}
	}
	if (type->base == ANJUTA_TARGET_SHAREDLIB) {
		if (strlen (name) < 7 ||
		    strncmp (name, "lib", strlen("lib")) != 0 ||
		    strcmp (&name[strlen(name) - 3], ".la") != 0) {
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Shared library target name must be of the form 'libxxx.la'"));
			return NULL;
		}
	}
	else if (type->base == ANJUTA_TARGET_STATICLIB) {
		if (strlen (name) < 6 ||
		    strncmp (name, "lib", strlen("lib")) != 0 ||
		    strcmp (&name[strlen(name) - 2], ".a") != 0) {
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Static library target name must be of the form 'libxxx.a'"));
			return NULL;
		}
	}
	
	/* Check that the new target doesn't already exist */
	find = (gchar *)name;
	g_node_children_foreach (parent, G_TRAVERSE_ALL, find_target, &find);
	if ((gchar *)find != name)
	{
		error_set (error, IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			_("Target already exists"));

		return NULL;
	}
	
	/* Add target node in project tree */
	child = amp_target_new (name, type, "", 0);
	g_node_append (parent, child);

	/* Add in Makefile.am */
	targetname = g_strconcat (((AmpTargetInformation *)type)->install, ((AmpTargetInformation *)type)->prefix, NULL);

	for (last = amp_group_get_token (parent, AM_GROUP_TARGET); last != NULL; last = g_list_next (last))
	{
		gchar *value = anjuta_token_evaluate ((AnjutaToken *)last->data);
		
		if ((value != NULL) && (strcmp (targetname, value) == 0))
		{
			g_free (value);
			break;
		}
		g_free (value);
	}

	token = anjuta_token_new_string (((AmpTargetInformation *)type)->token, targetname);
	g_free (targetname);

	if (last == NULL)
	{
		prev_token = anjuta_token_next_child (anjuta_token_file_first (AMP_GROUP_DATA (parent)->tfile));
		if (prev_token != NULL)
		{
			/* Add at the end of the file */
			while (anjuta_token_next_sibling (prev_token) != NULL)
			{
				prev_token = anjuta_token_next_sibling (prev_token);
			}
		}

		list = anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, "\n");
		if (prev_token == NULL)
		{
			prev_token = anjuta_token_insert_child (anjuta_token_file_first (AMP_GROUP_DATA (parent)->tfile), list);
		}
		else
		{
			prev_token = anjuta_token_insert_after (prev_token, list);
		}

		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_static (ANJUTA_TOKEN_STATEMENT | ANJUTA_TOKEN_ADDED, NULL));
		list = prev_token;
		prev_token = anjuta_token_insert_after (prev_token, token);
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_OPERATOR | ANJUTA_TOKEN_ADDED, "="));
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL));
		token = prev_token;
		prev_token = anjuta_token_insert_after (prev_token, anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		anjuta_token_merge (token, prev_token);
		anjuta_token_merge (list, token);
		anjuta_token_insert_after (token, anjuta_token_new_string (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, "\n"));
		token = prev_token;
	}
	else
	{
		for (token = (AnjutaToken *)last->data; anjuta_token_get_type (token) != ANJUTA_TOKEN_LIST; token = anjuta_token_next_sibling (token));

		if (anjuta_token_next_child (token) == NULL)
		{
			token = anjuta_token_insert_child (token, anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		}
		else
		{
			token = anjuta_token_next_child (token);
		}

		for (; anjuta_token_next_sibling (token) != NULL; token = anjuta_token_next_sibling (token));
	}

	if (anjuta_token_get_type (token) != ANJUTA_TOKEN_SPACE)
	{
		token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
	}
	token = anjuta_token_insert_after (token, anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name));
	amp_target_add_token (child, token);

	return child;
}

void 
amp_project_remove_target (AmpProject  *project,
		    AmpTarget *target,
		    GError     **error)
{
	GList *token_list;

	if (AMP_NODE_DATA (target)->type != ANJUTA_PROJECT_TARGET) return;
	
	for (token_list = amp_target_get_token (target); token_list != NULL; token_list = g_list_next (token_list))
	{
		anjuta_token_remove ((AnjutaToken *)token_list->data);
	}

	amp_target_free (target);
}

AmpSource* 
amp_project_add_source (AmpProject  *project,
		 AmpTarget *target,
		 GFile *file,
		 GError     **error)
{
	AmpGroup *group;
	AmpSource *last;
	AmpSource *source;
	AnjutaToken* token;
	gchar *relative_name;
	
	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (target != NULL, NULL);
	
	if (AMP_NODE_DATA (target)->type != ANJUTA_PROJECT_TARGET) return NULL;

	group = (AmpGroup *)(target->parent);
	relative_name = g_file_get_relative_path (AMP_GROUP_DATA (group)->base.directory, file);

	/* Add token */
	last = g_node_last_child (target);
	if (last == NULL)
	{
		/* First child */
		AnjutaToken *tok;
		AnjutaToken *close_tok;
		AnjutaToken *eol_tok;
		gchar *target_var;
		gchar *canon_name;

		/* Search where the target is declared */
		tok = (AnjutaToken *)amp_target_get_token (target)->data;
		close_tok = anjuta_token_new_static (ANJUTA_TOKEN_CLOSE, NULL);
		eol_tok = anjuta_token_new_static (ANJUTA_TOKEN_EOL, NULL);
		//anjuta_token_match (close_tok, ANJUTA_SEARCH_OVER, tok, &tok);
		anjuta_token_match (eol_tok, ANJUTA_SEARCH_OVER, tok, &tok);
		anjuta_token_free (close_tok);
		anjuta_token_free (eol_tok);

		/* Add a _SOURCES variable just after */
		canon_name = canonicalize_automake_variable (AMP_TARGET_DATA (target)->base.name);
		target_var = g_strconcat (canon_name,  "_SOURCES", NULL);
		g_free (canon_name);
		tok = anjuta_token_insert_after (tok, anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, target_var));
		g_free (target_var);
		tok = anjuta_token_insert_after (tok, anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		tok = anjuta_token_insert_after (tok, anjuta_token_new_static (ANJUTA_TOKEN_OPERATOR | ANJUTA_TOKEN_ADDED, "="));
		tok = anjuta_token_insert_after (tok, anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " "));
		token = anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " ");
		tok = anjuta_token_insert_after (tok, token);
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, relative_name);
		tok = anjuta_token_insert_after (tok, token);
		tok = anjuta_token_insert_after (tok, anjuta_token_new_static (ANJUTA_TOKEN_EOL | ANJUTA_TOKEN_ADDED, "\n"));
	}
	else
	{
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, relative_name);
		add_list_item (AMP_SOURCE_DATA (last)->token, token, NULL);
	}
	g_free (relative_name);
	
	/* Add source node in project tree */
	source = amp_source_new (file);
	AMP_SOURCE_DATA(source)->token = token;
	g_node_append (target, source);

	return source;
}

void 
amp_project_remove_source (AmpProject  *project,
		    AmpSource *source,
		    GError     **error)
{
	amp_dump_node (source);
	if (AMP_NODE_DATA (source)->type != ANJUTA_PROJECT_SOURCE) return;
	
	remove_list_item (AMP_SOURCE_DATA (source)->token, NULL);

	amp_source_free (source);
}

GList *
amp_project_get_config_modules   (AmpProject *project, GError **error)
{
	return project->modules == NULL ? NULL : g_hash_table_get_keys (project->modules);
}

GList *
amp_project_get_config_packages  (AmpProject *project,
			   const gchar* module,
			   GError **error)
{
	AmpModule *mod;
	GList *packages = NULL;

	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (module != NULL, NULL);

	mod = g_hash_table_lookup (project->modules, module);

	if (mod != NULL)
	{
		GList *node;

		for (node = mod->packages; node != NULL; node = g_list_next (node))
		{
			packages = g_list_prepend (packages, ((AmpPackage *)node->data)->name);
		}

		packages = g_list_reverse (packages);
	}

	return packages;
}

GList *
amp_project_get_target_types (AmpProject *project, GError **error)
{
	AmpTargetInformation *targets = AmpTargetTypes; 
	GList *types = NULL;

	while (targets->base.name != NULL)
	{
		types = g_list_prepend (types, targets);
		targets++;
	}
	types = g_list_reverse (types);

	return types;
}


/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
amp_project_save (AmpProject *project, GError **error)
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
amp_project_move (AmpProject *project, const gchar *path)
{
	GFile	*old_root_file;
	GFile *new_file;
	gchar *relative;
	GHashTableIter iter;
	gpointer key;
	gpointer value;
	AnjutaTokenFile *tfile;
	AmpConfigFile *cfg;
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
		AmpGroup *group = (AmpGroup *)value;
		
		relative = get_relative_path (old_root_file, AMP_GROUP_DATA (group)->base.directory);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (AMP_GROUP_DATA (group)->base.directory);
		AMP_GROUP_DATA (group)->base.directory = new_file;

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

	/* Change all configs */
	old_hash = project->configs;
	project->configs = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, NULL, amp_config_file_free);
	g_hash_table_iter_init (&iter, old_hash);
	while (g_hash_table_iter_next (&iter, &key, (gpointer *)&cfg))
	{
		relative = get_relative_path (old_root_file, cfg->file);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (cfg->file);
		cfg->file = new_file;
		
		g_hash_table_insert (project->configs, new_file, cfg);
	}
	g_hash_table_steal_all (old_hash);
	g_hash_table_destroy (old_hash);

	
	g_object_unref (old_root_file);

	return TRUE;
}

AmpProject *
amp_project_new (void)
{
	return AMP_PROJECT (g_object_new (AMP_TYPE_PROJECT, NULL));
}

/* Project access functions
 *---------------------------------------------------------------------------*/

AmpGroup *
amp_project_get_root (AmpProject *project)
{
	AmpGroup *g_node = NULL;
	
	if (project->root_file != NULL)
	{
		gchar *id = g_file_get_uri (project->root_file);
		g_node = (AmpGroup *)g_hash_table_lookup (project->groups, id);
		g_free (id);
	}

	return g_node;
}

AmpGroup *
amp_project_get_group (AmpProject *project, const gchar *id)
{
	return (AmpGroup *)g_hash_table_lookup (project->groups, id);
}

AmpTarget *
amp_project_get_target (AmpProject *project, const gchar *id)
{
	AmpTarget **buffer;
	AmpTarget *target;
	gsize dummy;

	buffer = (AmpTarget **)g_base64_decode (id, &dummy);
	target = *buffer;
	g_free (buffer);

	return target;
}

AmpSource *
amp_project_get_source (AmpProject *project, const gchar *id)
{
	AmpSource **buffer;
	AmpSource *source;
	gsize dummy;

	buffer = (AmpSource **)g_base64_decode (id, &dummy);
	source = *buffer;
	g_free (buffer);

	return source;
}

gchar *
amp_project_get_node_id (AmpProject *project, const gchar *path)
{
	GNode *node = NULL;

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
				node = g_node_nth_child (node, child);
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

	switch (AMP_NODE_DATA (node)->type)
	{
		case ANJUTA_PROJECT_GROUP:
			return g_file_get_uri (AMP_GROUP_DATA (node)->base.directory);
		case ANJUTA_PROJECT_TARGET:
		case ANJUTA_PROJECT_SOURCE:
			return g_base64_encode ((guchar *)&node, sizeof (node));
		default:
			return NULL;
	}
}

gchar *
amp_project_get_uri (AmpProject *project)
{
	g_return_val_if_fail (project != NULL, NULL);

	return project->root_file != NULL ? g_file_get_uri (project->root_file) : NULL;
}

GFile*
amp_project_get_file (AmpProject *project)
{
	g_return_val_if_fail (project != NULL, NULL);

	return project->root_file;
}
	
gchar *
amp_project_get_property (AmpProject *project, AmpPropertyType type)
{
	const gchar *value = NULL;

	if (project->property != NULL)
	{
		switch (type)
		{
			case AMP_PROPERTY_NAME:
				value = project->property->name;
				break;
			case AMP_PROPERTY_VERSION:
				value = project->property->version;
				break;
			case AMP_PROPERTY_BUG_REPORT:
				value = project->property->bug_report;
				break;
			case AMP_PROPERTY_TARNAME:
				value = project->property->tarname;
				if (value == NULL) return ac_init_default_tarname (project->property->name);
				break;
			case AMP_PROPERTY_URL:
				value = project->property->url;
				break;
		}
	}

	return value == NULL ? NULL : g_strdup (value);
}

gboolean
amp_project_set_property (AmpProject *project, AmpPropertyType type, const gchar *value)
{
	if (project->property != NULL)
	{
		switch (type)
		{
			case AMP_PROPERTY_NAME:
				project->property->name = g_strdup (value);
				break;
			case AMP_PROPERTY_VERSION:
				project->property->version = g_strdup (value);
				break;
			case AMP_PROPERTY_BUG_REPORT:
				project->property->bug_report = g_strdup (value);
				break;
			case AMP_PROPERTY_TARNAME:
				project->property->tarname = g_strdup (value);
				break;
			case AMP_PROPERTY_URL:
				project->property->url = g_strdup (value);
				break;
		}
		return amp_project_update_property (project, type);
	}
	
	return TRUE;
}

/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static AnjutaProjectGroup* 
iproject_add_group (IAnjutaProject *obj, AnjutaProjectGroup *parent,  const gchar *name, GError **err)
{
	return amp_project_add_group (AMP_PROJECT (obj), AMP_GROUP (parent), name, err);
}

static AnjutaProjectSource* 
iproject_add_source (IAnjutaProject *obj, AnjutaProjectGroup *parent,  GFile *file, GError **err)
{
	return amp_project_add_source (AMP_PROJECT (obj), AMP_TARGET (parent), file, err);
}

static AnjutaProjectTarget* 
iproject_add_target (IAnjutaProject *obj, AnjutaProjectGroup *parent,  const gchar *name,  AnjutaProjectTargetType type, GError **err)
{
	return amp_project_add_target (AMP_PROJECT (obj), AMP_GROUP (parent), name, type, err);
}

static GtkWidget* 
iproject_configure (IAnjutaProject *obj, GError **err)
{
	return amp_configure_project_dialog (AMP_PROJECT (obj), err);
}

static guint 
iproject_get_capabilities (IAnjutaProject *obj, GError **err)
{
	return (IANJUTA_PROJECT_CAN_ADD_GROUP |
		IANJUTA_PROJECT_CAN_ADD_TARGET |
		IANJUTA_PROJECT_CAN_ADD_SOURCE);
}

static GList* 
iproject_get_packages (IAnjutaProject *obj, GError **err)
{
	GList *modules;
	GList *packages;
	GList* node;
	GHashTable *all = g_hash_table_new (g_str_hash, g_str_equal);
	
	modules = amp_project_get_config_modules (AMP_PROJECT (obj), NULL);
	for (node = modules; node != NULL; node = g_list_next (node))
	{
		GList *pack;
		
		packages = amp_project_get_config_packages (AMP_PROJECT (obj), (const gchar *)node->data, NULL);
		for (pack = packages; pack != NULL; pack = g_list_next (pack))
		{
			g_hash_table_replace (all, pack->data, NULL);
		}
	    g_list_free (packages);
	}
    g_list_free (modules);

	packages = g_hash_table_get_keys (all);
	g_hash_table_destroy (all);
	
	return packages;
}

static AnjutaProjectGroup* 
iproject_get_root (IAnjutaProject *obj, GError **err)
{
	return amp_project_get_root (AMP_PROJECT (obj));
}

static GList* 
iproject_get_target_types (IAnjutaProject *obj, GError **err)
{
	return amp_project_get_target_types (AMP_PROJECT (obj), err);
}

static gboolean
iproject_load (IAnjutaProject *obj, GFile *file, GError **err)
{
	return amp_project_load (AMP_PROJECT (obj), file, err);
}

static gboolean
iproject_refresh (IAnjutaProject *obj, GError **err)
{
	return amp_project_reload (AMP_PROJECT (obj), err);
}

static gboolean
iproject_remove_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	return TRUE;
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
	iface->remove_node = iproject_remove_node;
}

/* Group access functions
 *---------------------------------------------------------------------------*/

GFile*
amp_group_get_directory (AmpGroup *group)
{
	return AMP_GROUP_DATA (group)->base.directory;
}

GFile*
amp_group_get_makefile (AmpGroup *group)
{
	return AMP_GROUP_DATA (group)->makefile;
}

gchar *
amp_group_get_id (AmpGroup *group)
{
	return g_file_get_uri (AMP_GROUP_DATA (group)->base.directory);
}

/* Target access functions
 *---------------------------------------------------------------------------*/

const gchar *
amp_target_get_name (AmpTarget *target)
{
	return AMP_TARGET_DATA (target)->base.name;
}

AnjutaProjectTargetType
amp_target_get_type (AmpTarget *target)
{
	return AMP_TARGET_DATA (target)->base.type;
}

gchar *
amp_target_get_id (AmpTarget *target)
{
	return g_base64_encode ((guchar *)&target, sizeof (target));
}

/* Source access functions
 *---------------------------------------------------------------------------*/

gchar *
amp_source_get_id (AmpSource *source)
{
	return g_base64_encode ((guchar *)&source, sizeof (source));
}

GFile*
amp_source_get_file (AmpSource *source)
{
	return AMP_SOURCE_DATA (source)->base.file;
}

/* GbfProject implementation
 *---------------------------------------------------------------------------*/

static void
amp_project_dispose (GObject *object)
{
	g_return_if_fail (AMP_IS_PROJECT (object));

	amp_project_unload (AMP_PROJECT (object));

	G_OBJECT_CLASS (parent_class)->dispose (object);	
}

static void
amp_project_instance_init (AmpProject *project)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (AMP_IS_PROJECT (project));
	
	/* project data */
	project->root_file = NULL;
	project->configure_file = NULL;
	project->root_node = NULL;
	project->property = NULL;

	project->space_list = NULL;
	project->arg_list = NULL;
}

static void
amp_project_class_init (AmpProjectClass *klass)
{
	GObjectClass *object_class;
	
	parent_class = g_type_class_peek_parent (klass);

	object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = amp_project_dispose;
}

ANJUTA_TYPE_BEGIN(AmpProject, amp_project, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(iproject, IANJUTA_TYPE_PROJECT);
ANJUTA_TYPE_END;
//GBF_BACKEND_BOILERPLATE (AmpProject, amp_project);

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
#include "am-writer.h"
//#include "am-config.h"
#include "am-properties.h"


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
#define AMP_PACKAGE_DATA(node)  ((node) != NULL ? (AmpPackageData *)((node)->data) : NULL)
#define AMP_MODULE_DATA(node)  ((node) != NULL ? (AmpModuleData *)((node)->data) : NULL)

#define STR_REPLACE(target, source) \
	{ g_free (target); target = source == NULL ? NULL : g_strdup (source);}


/*typedef struct _AmpPackage AmpPackage;

typedef struct _AmpModule AmpModule;*/
	
typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} AmpGroupTokenCategory;

typedef struct _AmpGroupData AmpGroupData;

struct _AmpGroupData {
	AnjutaProjectNodeData base;		/* Common node data */
	gboolean dist_only;			/* TRUE if the group is distributed but not built */
	GFile *makefile;				/* GFile corresponding to group makefile */
	AnjutaTokenFile *tfile;		/* Corresponding Makefile */
	GList *tokens[AM_GROUP_TOKEN_LAST];					/* List of token used by this group */
	AnjutaToken *make_token;
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
	AnjutaProjectNodeData base;
	gchar *install;
	gint flags;
	GList* tokens;
};

typedef struct _AmpSourceData AmpSourceData;

struct _AmpSourceData {
	AnjutaProjectNodeData base;
	AnjutaToken* token;
};

typedef struct _AmpModuleData AmpModuleData;

struct _AmpModuleData {
	AnjutaProjectNodeData base;
	AnjutaToken *module;
};

typedef struct _AmpPackageData AmpPackageData;

struct _AmpPackageData {
	AnjutaProjectNodeData base;
	gchar *version;
};

typedef struct _AmpConfigFile AmpConfigFile;

struct _AmpConfigFile {
	GFile *file;
	AnjutaToken *token;
};

typedef struct _AmpTargetInformation AmpTargetInformation;

struct _AmpTargetInformation {
	AnjutaProjectTargetInfo base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};

struct _AmpTargetPropertyBuffer {
	GList *sources;
	GList *properties;
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

/* Properties
 *---------------------------------------------------------------------------*/




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

/* Automake parsing function
 *---------------------------------------------------------------------------*/

/* Remove invalid character according to automake rules */
static gchar*
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

static gboolean
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

static gchar*
ac_init_default_tarname (const gchar *name)
{
	gchar *tarname;

	if (name == NULL) return NULL;
	
	/* Remove GNU prefix */
	if (strncmp (name, "GNU ", 4) == 0) name += 4;

	tarname = g_ascii_strdown (name, -1);
	g_strcanon (tarname, "abcdefghijklmnopqrstuvwxyz0123456789", '-');
	
	return tarname;
}

/* Properties buffer objects
 *---------------------------------------------------------------------------*/

AmpTargetPropertyBuffer*
amp_target_property_buffer_new (void)
{
	AmpTargetPropertyBuffer* buffer;

	buffer = g_new0 (AmpTargetPropertyBuffer, 1);

	return buffer;
}

void
amp_target_property_buffer_free (AmpTargetPropertyBuffer *buffer)
{
	g_list_foreach (buffer->sources, (GFunc)amp_source_free, NULL);
	g_list_free (buffer->sources);
	g_list_foreach (buffer->properties, (GFunc)amp_property_free, NULL);
	g_list_free (buffer->properties);
	g_free (buffer);
}

void
amp_target_property_buffer_add_source (AmpTargetPropertyBuffer *buffer, AmpSource *source)
{
	buffer->sources = g_list_prepend (buffer->sources, source);
}

void
amp_target_property_buffer_add_property (AmpTargetPropertyBuffer *buffer, AnjutaProjectPropertyInfo *prop)
{
	buffer->properties = g_list_prepend (buffer->properties, prop);
}

GList *
amp_target_property_buffer_steal_sources (AmpTargetPropertyBuffer *buffer)
{
	GList *list = buffer->sources;

	buffer->sources = NULL;

	return list;
}

GList *
amp_target_property_buffer_steal_properties (AmpTargetPropertyBuffer *buffer)
{
	GList *list = buffer->properties;

	buffer->properties = NULL;

	return list;
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

static void
amp_package_set_version (AmpPackage *node, const gchar *compare, const gchar *version)
{
	AmpPackageData *package= AMP_PACKAGE_DATA (node);
	
	g_return_if_fail (package != NULL);
	g_return_if_fail ((version == NULL) || (compare != NULL));

	g_free (package->version);
	package->version = version != NULL ? g_strconcat (compare, version, NULL) : NULL;
}

static AmpPackage*
amp_package_new (const gchar *name)
{
    AmpPackageData *package = NULL;

	g_return_val_if_fail (name != NULL, NULL);
	
	package = g_slice_new0(AmpPackageData); 
	package->base.type = ANJUTA_PROJECT_PACKAGE;
	package->base.properties = amp_get_package_property_list();
	package->base.name = g_strdup (name);

	return g_node_new (package);
}

static void
amp_package_free (AmpPackage *node)
{
	AmpPackageData *package = AMP_PACKAGE_DATA (node);
	
	if (package->base.file) g_object_unref (package->base.file);
	g_free (package->base.name);
	anjuta_project_property_foreach (package->base.properties, (GFunc)amp_property_free, NULL);
    g_slice_free (AmpPackageData, package);

	g_node_destroy (node);
}

/* Module objects
 *---------------------------------------------------------------------------*/

static AmpModule*
amp_module_new (AnjutaToken *token)
{
	AmpModuleData *module;
	
	module = g_slice_new0(AmpModuleData); 
	module->base.type = ANJUTA_PROJECT_MODULE;
	module->base.properties = amp_get_module_property_list();
	module->base.name = anjuta_token_evaluate (token);
	module->module = token;

	return g_node_new (module);
}

static void
amp_module_free (AmpModule *node)
{
	AmpModuleData *module = AMP_MODULE_DATA (node);
	
	if (module->base.file) g_object_unref (module->base.file);
	g_free (module->base.name);

	g_slice_free (AmpModuleData, module);

	g_node_destroy (node);
}

static void
amp_project_new_module_hash (AmpProject *project)
{
	project->modules = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
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

static AnjutaToken*
amp_group_get_first_token (AmpGroup *node, AmpGroupTokenCategory category)
{
	GList *list;
	
	list = amp_group_get_token (node, category);
	if (list == NULL) return NULL;

	return (AnjutaToken *)list->data;
}

static void
amp_group_set_dist_only (AmpGroup *node, gboolean dist_only)
{
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	AMP_GROUP_DATA (node)->dist_only = dist_only;
}

static AnjutaTokenFile*
amp_group_set_makefile (AmpGroup *node, GFile *makefile, AmpProject* project)
{
    AmpGroupData *group;
	
	g_return_val_if_fail ((node != NULL) && (node->data != NULL), NULL); 

 	group = AMP_GROUP_DATA (node);
	if (group->makefile != NULL) g_object_unref (group->makefile);
	if (group->tfile != NULL) anjuta_token_file_free (group->tfile);
	if (makefile != NULL)
	{
		AnjutaToken *token;
		AmpAmScanner *scanner;
		
		group->makefile = g_object_ref (makefile);
		group->tfile = anjuta_token_file_new (makefile);

		token = anjuta_token_file_load (group->tfile, NULL);
			
		scanner = amp_am_scanner_new (project, node);
		group->make_token = amp_am_scanner_parse_token (scanner, token, makefile, NULL);
		amp_am_scanner_free (scanner);
	}
	else
	{
		group->makefile = NULL;
		group->tfile = NULL;
		group->make_token = NULL;
	}

	return group->tfile;
}

static AmpGroup*
amp_group_new (GFile *file, gboolean dist_only)
{
    AmpGroupData *group = NULL;

	g_return_val_if_fail (file != NULL, NULL);
	
	group = g_slice_new0(AmpGroupData); 
	group->base.type = ANJUTA_PROJECT_GROUP;
	group->base.properties = amp_get_group_property_list();
	group->base.file = g_object_ref (file);
	group->dist_only = dist_only;

    return g_node_new (group);
}

static void
amp_group_free (AmpGroup *node)
{
    AmpGroupData *group = (AmpGroupData *)node->data;
	gint i;
	
	if (group->base.file) g_object_unref (group->base.file);
	anjuta_project_property_foreach (group->base.properties, (GFunc)amp_property_free, NULL);
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
amp_target_add_token (AmpTarget *node, AnjutaToken *token)
{
    AmpTargetData *target;
	
	g_return_if_fail ((node != NULL) && (node->data != NULL)); 

 	target = AMP_TARGET_DATA (node);
	target->tokens = g_list_prepend (target->tokens, token);
}

static GList *
amp_target_get_token (AmpTarget *node)
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
	target->base.type = ANJUTA_PROJECT_TARGET;
	target->base.properties = amp_get_target_property_list(type);
	target->base.name = g_strdup (name);
	target->base.target_type = type;
	target->install = g_strdup (install);
	target->flags = flags;

    return g_node_new (target);
}

static void
amp_target_free (AmpTarget *node)
{
    AmpTargetData *target = AMP_TARGET_DATA (node);
	
    g_free (target->base.name);
	anjuta_project_property_foreach (target->base.properties, (GFunc)amp_property_free, NULL);
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
	source->base.type = ANJUTA_PROJECT_SOURCE;
	source->base.properties = amp_get_source_property_list();
	source->base.file = g_object_ref (file);

    return g_node_new (source);
}

void
amp_source_free (AmpSource *node)
{
    AmpSourceData *source = AMP_SOURCE_DATA (node);
	
    g_object_unref (source->base.file);
	anjuta_project_property_foreach (source->base.properties, (GFunc)amp_property_free, NULL);
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

	monitor_add (project, AMP_GROUP_DATA(group_node)->base.file);
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
amp_dump_node (AnjutaProjectNode *g_node)
{
	gchar *name = NULL;
	
	switch (AMP_NODE_DATA (g_node)->type) {
		case ANJUTA_PROJECT_GROUP:
			name = g_file_get_uri (AMP_GROUP_DATA (g_node)->base.file);
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

static void
foreach_node_destroy (AnjutaProjectNode    *g_node,
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
		case ANJUTA_PROJECT_MODULE:
			amp_module_free (g_node);
			break;
		case ANJUTA_PROJECT_PACKAGE:
			amp_package_free (g_node);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
}

static void
project_node_destroy (AmpProject *project, AnjutaProjectNode *g_node)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (AMP_IS_PROJECT (project));
	
	if (g_node) {
		/* free each node's data first */
		anjuta_project_node_all_foreach (g_node,
				 foreach_node_destroy, project);
		
		/* now destroy the tree itself */
		//g_node_destroy (g_node);
	}
}

void
amp_project_load_properties (AmpProject *project, AnjutaToken *macro, AnjutaToken *args)
{
	AnjutaProjectPropertyItem *list;
	
	//fprintf (stdout, "property list:\n");
	//anjuta_token_dump (args);

	project->ac_init = macro;
	project->args = args;
	
	for (list = anjuta_project_property_first (project->properties); list != NULL; list = anjuta_project_property_next (list))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)anjuta_project_property_get_info (list);

		if (info->position >= 0)
		{
			AnjutaProjectPropertyInfo *prop;
			AnjutaToken *arg;

			prop = anjuta_project_property_lookup (project->properties, list);
			if (prop == NULL)
			{
				prop = (AnjutaProjectPropertyInfo *)amp_property_new (NULL, info->token_type, info->position, NULL, macro);
				
				project->properties = anjuta_project_property_insert (project->properties, list, prop);
			}
	
			arg = anjuta_token_nth_word (args, info->position);
			if ((prop->value != NULL) && (prop->value != info->base.value)) g_free (prop->value);
			prop->value = anjuta_token_evaluate (arg);
		}
	}
}

void
amp_project_load_module (AmpProject *project, AnjutaToken *module_token)
{
	AmpAcScanner *scanner = NULL;

	if (module_token != NULL)
	{
		AnjutaToken *arg;
		AnjutaToken *list;
		AnjutaToken *item;
		gchar *value;
		AmpModule *module;
		AmpPackage *package;
		gchar *compare;


		/* Module name */
		arg = anjuta_token_first_item (module_token);
		value = anjuta_token_evaluate (arg);
		module = amp_module_new (arg);
		anjuta_project_node_append (project->root_node, module);
		if (value != NULL) g_hash_table_insert (project->modules, value, module);

		/* Package list */
		arg = anjuta_token_next_word (arg);
		scanner = amp_ac_scanner_new (project);
		list = amp_ac_scanner_parse_token (scanner, arg, AC_SPACE_LIST_STATE, NULL);
		anjuta_token_free_children (arg);
		list = anjuta_token_delete_parent (list);
		anjuta_token_prepend_items (arg, list);
		amp_ac_scanner_free (scanner);
		
		package = NULL;
		compare = NULL;
		for (item = anjuta_token_first_word (arg); item != NULL; item = anjuta_token_next_word (item))
		{
			value = anjuta_token_evaluate (item);
			if (value == NULL) continue;		/* Empty value, a comment of a quote by example */
			if (*value == '\0')
			{
				g_free (value);
				continue;
			}
			
			if ((package != NULL) && (compare != NULL))
			{
				amp_package_set_version (package, compare, value);
				g_free (value);
				g_free (compare);
				package = NULL;
				compare = NULL;
			}
			else if ((package != NULL) && (anjuta_token_get_type (item) == ANJUTA_TOKEN_OPERATOR))
			{
				compare = value;
			}
			else
			{
				package = amp_package_new (value);
				anjuta_project_node_append (module, package);
				g_free (value);
				compare = NULL;
			}
		}
	}
}

void
amp_project_load_config (AmpProject *project, AnjutaToken *arg_list)
{
	AmpAcScanner *scanner = NULL;

	if (arg_list != NULL)
	{
		AnjutaToken *arg;
		AnjutaToken *list;
		AnjutaToken *item;

		/* File list */
		scanner = amp_ac_scanner_new (project);
		fprintf (stdout, "\nParse list\n");
		
		arg = anjuta_token_first_item (arg_list);
		list = amp_ac_scanner_parse_token (scanner, arg, AC_SPACE_LIST_STATE, NULL);
		anjuta_token_free_children (arg);
		list = anjuta_token_delete_parent (list);
		anjuta_token_prepend_items (arg, list);
		amp_ac_scanner_free (scanner);
		
		for (item = anjuta_token_first_word (arg); item != NULL; item = anjuta_token_next_word (item))
		{
			gchar *value;
			AmpConfigFile *cfg;

			value = anjuta_token_evaluate (item);
			if (value == NULL) continue;
		
			cfg = amp_config_file_new (value, project->root_file, item);
			g_hash_table_insert (project->configs, cfg->file, cfg);
			g_free (value);
		}
	}
}

static void
find_target (AnjutaProjectNode *node, gpointer data)
{
	if (AMP_NODE_DATA (node)->type == ANJUTA_PROJECT_TARGET)
	{
		if (strcmp (AMP_TARGET_DATA (node)->base.name, *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(AnjutaProjectNode **)data = node;

			return;
		}
	}
}

static void
find_canonical_target (AnjutaProjectNode *node, gpointer data)
{
	if (AMP_NODE_DATA (node)->type == ANJUTA_PROJECT_TARGET)
	{
		gchar *canon_name = canonicalize_automake_variable (AMP_TARGET_DATA (node)->base.name);	
		DEBUG_PRINT ("compare canon %s vs %s node %p", canon_name, *(gchar **)data, node);
		if (strcmp (canon_name, *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(AnjutaProjectNode **)data = node;
			g_free (canon_name);

			return;
		}
		g_free (canon_name);
	}
}

static AnjutaToken*
project_load_target (AmpProject *project, AnjutaToken *name, AnjutaTokenType token_type, AnjutaToken *list, AnjutaProjectNode *parent, GHashTable *orphan_properties)
{
	AnjutaToken *arg;
	AnjutaProjectTargetType type = NULL;
	gchar *install;
	gchar *value;
	gint flags;
	AmpTargetInformation *targets = AmpTargetTypes; 

	type = (AnjutaProjectTargetType)targets;
	while (targets->base.name != NULL)
	{
		if (token_type == targets->token)
		{
			type = (AnjutaProjectTargetType)targets;
			break;
		}
		targets++;
	}

	value = anjuta_token_evaluate (name);
	split_automake_variable (value, &flags, &install, NULL);

	amp_group_add_token (parent, name, AM_GROUP_TARGET);
	
	for (arg = anjuta_token_first_word (list); arg != NULL; arg = anjuta_token_next_word (arg))
	{
		gchar *value;
		gchar *canon_id;
		AmpTarget *target;
		AmpTargetPropertyBuffer *buffer;
		gchar *orig_key;
		gpointer find;

		value = anjuta_token_evaluate (arg);
		canon_id = canonicalize_automake_variable (value);		
		
		/* Check if target already exists */
		find = value;
		anjuta_project_node_children_foreach (parent, find_target, &find);
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
		anjuta_project_node_append (parent, target);
		DEBUG_PRINT ("create target %p name %s", target, value);

		/* Check if there are sources or properties availables */
		if (g_hash_table_lookup_extended (orphan_properties, canon_id, (gpointer *)&orig_key, (gpointer *)&buffer))
		{
			GList *sources;
			GList *src;

			g_hash_table_steal (orphan_properties, canon_id);
			sources = amp_target_property_buffer_steal_sources (buffer);
			for (src = sources; src != NULL; src = g_list_next (src))
			{
				AmpSource *source = src->data;

				anjuta_project_node_prepend (target, source);
			}
			g_free (orig_key);
			g_list_free (sources);

			amp_target_property_buffer_free (buffer);
		}

		/* Set target properties */
		if (flags & AM_TARGET_NOBASE) 
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 0, "1", arg);
		if (flags & AM_TARGET_NOTRANS) 
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 1, "1", arg);
		if (flags & AM_TARGET_DIST) 
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 2, "1", arg);
		if (flags & AM_TARGET_NODIST) 
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 2, "0", arg);
		if (flags & AM_TARGET_NOINST) 
		{
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 3, "1", arg);
		}
		else
		{
			gchar *instdir = g_strconcat ("$(", install, "dir)", NULL);
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 6, instdir, arg);
			g_free (instdir);
		}
		
		if (flags & AM_TARGET_CHECK) 
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 4, "1", arg);
		if (flags & AM_TARGET_MAN)
		{
			gchar section[] = "0";

			section[0] += (flags >> 7) & 0x1F;
			amp_node_property_set (target, AM_TOKEN__PROGRAMS, 4, section, arg);
		}
		
		g_free (canon_id);
		g_free (value);
	}
	g_free (value);

	return NULL;
}

static AnjutaToken*
project_load_sources (AmpProject *project, AnjutaToken *name, AnjutaToken *list, AnjutaProjectNode *parent, GHashTable *orphan_properties)
{
	AnjutaToken *arg;
	AmpGroupData *group = AMP_GROUP_DATA (parent);
	GFile *parent_file = g_object_ref (group->base.file);
	gchar *target_id = NULL;
	AmpTargetPropertyBuffer *orphan = NULL;

	target_id = anjuta_token_evaluate (name);
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
		anjuta_project_node_children_foreach (parent, find_canonical_target, &find);
		parent = (gchar *)find != target_id ? (AnjutaProjectNode *)find : NULL;

		/* Get orphan buffer if there is no target */
		if (parent == NULL)
		{
			gchar *orig_key;
			if (g_hash_table_lookup_extended (orphan_properties, target_id, (gpointer *)&orig_key, (gpointer *)&orphan))
			{
				g_hash_table_steal (orphan_properties, target_id);
				g_free (orig_key);
			}
			else
			{
				orphan = amp_target_property_buffer_new ();
			}
		}

		for (arg = anjuta_token_first_word (list); arg != NULL; arg = anjuta_token_next_word (arg))
		{
			gchar *value;
			AmpSource *source;
			GFile *src_file;
		
			value = anjuta_token_evaluate (arg);

			/* Create source */
			src_file = g_file_get_child (parent_file, value);
			source = amp_source_new (src_file);
			g_object_unref (src_file);
			AMP_SOURCE_DATA(source)->token = arg;

			if (orphan != NULL)
			{
				amp_target_property_buffer_add_source (orphan, source);
			}
			else
			{
				DEBUG_PRINT ("add target child %p", parent);
				/* Add as target child */
				anjuta_project_node_append (parent, source);
			}

			g_free (value);
		}

		if (orphan != NULL)
		{
			g_hash_table_insert (orphan_properties, target_id, orphan);
		}
		else
		{
			g_free (target_id);
		}
	}

	g_object_unref (parent_file);

	return NULL;
}

static AnjutaToken*
project_load_data (AmpProject *project, AnjutaToken *name, AnjutaToken *list, AnjutaProjectNode *parent, GHashTable *orphan_properties)
{
	AnjutaProjectTargetType type = NULL;
	gchar *install;
	AnjutaProjectNode *target;
	gchar *target_id;
	gpointer find;
	gint flags;
	AmpTargetInformation *targets = AmpTargetTypes; 
	AnjutaToken *arg;

	type = (AnjutaProjectTargetType)targets;
	while (targets->base.name != NULL)
	{
		if (anjuta_token_get_type (name) == targets->token)
		{
			type = (AnjutaProjectTargetType)targets;
			break;
		}
		targets++;
	}

	target_id = anjuta_token_evaluate (name);
	split_automake_variable (target_id, &flags, &install, NULL);
	/*if (target_id)
	{
		gchar *end = strrchr (target_id, '_');
		if (end)
		{
			*end = '\0';
		}
	}*/
	
	amp_group_add_token (parent, name, AM_GROUP_TARGET);

	/* Check if target already exists */
	find = target_id;
	anjuta_project_node_children_foreach (parent, find_target, &find);
	if ((gchar *)find == target_id)
	{
		/* Create target */
		target = amp_target_new (target_id, type, install, flags);
		amp_target_add_token (target, arg);
		anjuta_project_node_append (parent, target);
		DEBUG_PRINT ("create target %p name %s", target, target_id);
	}
	else
	{
		target = (AnjutaProjectNode *)find;
	}
	g_free (target_id);

	if (target)
	{
		GFile *parent_file = g_object_ref (AMP_GROUP_DATA (parent)->base.file);
		
		for (arg = anjuta_token_first_word (list); arg != NULL; arg = anjuta_token_next_word (arg))
		{
			gchar *value;
			AmpSource *source;
			GFile *src_file;
		
			value = anjuta_token_evaluate (arg);

			/* Create source */
			src_file = g_file_get_child (parent_file, value);
			source = amp_source_new (src_file);
			g_object_unref (src_file);
			AMP_SOURCE_DATA(source)->token = arg;

			/* Add as target child */
			DEBUG_PRINT ("add target child %p", target);
			anjuta_project_node_append (target, source);

			g_free (value);
		}
		g_object_unref (parent_file);
	}

	return NULL;
}

static AnjutaToken*
project_load_target_properties (AmpProject *project, AnjutaToken *name, AnjutaTokenType type, AnjutaToken *list, AnjutaProjectNode *parent, GHashTable *orphan_properties)
{
	AmpGroupData *group = AMP_GROUP_DATA (parent);
	gchar *target_id = NULL;
	
	target_id = anjuta_token_evaluate (name);
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
		gchar *value;
		AnjutaProjectPropertyInfo *prop;
		AmpTargetPropertyBuffer *orphan = NULL;
		
		find = target_id;
		DEBUG_PRINT ("search for canonical %s", target_id);
		anjuta_project_node_children_foreach (parent, find_canonical_target, &find);
		parent = (gchar *)find != target_id ? (AnjutaProjectNode *)find : NULL;

		/* Get orphan buffer if there is no target */
		if (parent == NULL)
		{
			gchar *orig_key;
			if (g_hash_table_lookup_extended (orphan_properties, target_id, (gpointer *)&orig_key, (gpointer *)&orphan))
			{
				g_hash_table_steal (orphan_properties, target_id);
				g_free (orig_key);
			}
			else
			{
				orphan = amp_target_property_buffer_new ();
			}
		}

		/* Create property */
		value = anjuta_token_evaluate (list);
		prop = amp_property_new (NULL, type, 0, value, list);

		if (parent == NULL)
		{
			gchar *orig_key;
			AmpTargetPropertyBuffer *orphan = NULL;
			
			if (g_hash_table_lookup_extended (orphan_properties, target_id, (gpointer *)&orig_key, (gpointer *)&orphan))
			{
				g_hash_table_steal (orphan_properties, target_id);
				g_free (orig_key);
			}
			else
			{
				orphan = amp_target_property_buffer_new ();
			}
			amp_target_property_buffer_add_property (orphan, prop);
			g_hash_table_insert (orphan_properties, target_id, orphan);
		}
		else
		{
			amp_node_property_add (parent, prop);
			g_free (target_id);
		}
		g_free (value);
	}

	return NULL;
}

static AnjutaToken*
project_load_group_properties (AmpProject *project, AnjutaToken *token, AnjutaTokenType type, AnjutaToken *list, AnjutaProjectNode *parent)
{
	AmpGroupData *group = AMP_GROUP_DATA (parent);
	gchar *value;
	gchar *name;
	AnjutaProjectPropertyInfo *prop;
		
	/* Create property */
	name = anjuta_token_evaluate (token);
	value = anjuta_token_evaluate (list);

	prop = amp_property_new (name, type, 0, value, list);

	amp_node_property_add (parent, prop);
	g_free (value);
	g_free (name);

	return NULL;
}

static AmpGroup* project_load_makefile (AmpProject *project, GFile *file, AmpGroup *parent, gboolean dist_only);

static void
project_load_subdirs (AmpProject *project, AnjutaToken *list, AmpGroup *parent, gboolean dist_only)
{
	AnjutaToken *arg;

	for (arg = anjuta_token_first_word (list); arg != NULL; arg = anjuta_token_next_word (arg))
	{
		gchar *value;
		
		value = anjuta_token_evaluate (arg);
		
		/* Skip ., it is a special case, used to defined build order */
		if (strcmp (value, ".") != 0)
		{
			GFile *subdir;
			gchar *group_id;
			AmpGroup *group;

			subdir = g_file_resolve_relative_path (AMP_GROUP_DATA (parent)->base.file, value);
			
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
remove_config_file (gpointer data, GObject *object, gboolean is_last_ref)
{
	if (is_last_ref)
	{
		AmpProject *project = (AmpProject *)data;
		g_hash_table_remove (project->files, anjuta_token_file_get_file (ANJUTA_TOKEN_FILE (object)));
	}
}

static AmpGroup*
project_load_makefile (AmpProject *project, GFile *file, AnjutaProjectNode *parent, gboolean dist_only)
{
	const gchar **filename;
	AmpGroup *group;
	AnjutaTokenFile *tfile;
	GFile *makefile = NULL;

	/* Create group */
	if (parent != NULL)
	{
		group = amp_group_new (file, dist_only);
		g_hash_table_insert (project->groups, g_file_get_uri (file), group);
		anjuta_project_node_append (parent, group);
	}
	else
	{
		group = project->root_node;
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
				//g_message ("add group =%s= token %p group %p", *filename, config->token, anjuta_token_list (config->token));
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
	tfile = amp_group_set_makefile (group, makefile, project);
	g_hash_table_insert (project->files, makefile, tfile);
	g_object_add_toggle_ref (G_OBJECT (tfile), remove_config_file, project);
	
	return group;
}

void
amp_project_set_am_variable (AmpProject* project, AmpGroup* group, AnjutaTokenType variable, AnjutaToken *name, AnjutaToken *list, GHashTable *orphan_properties)
{
	
	switch (variable)
	{
	case AM_TOKEN_SUBDIRS:
		project_load_subdirs (project, list, group, FALSE);
		break;
	case AM_TOKEN_DIST_SUBDIRS:
		project_load_subdirs (project, list, group, TRUE);
		break;
	case AM_TOKEN__DATA:
		project_load_data (project, name, list, group, orphan_properties);
		break;
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
		project_load_target (project, name, variable, list, group, orphan_properties);
		break;
	case AM_TOKEN__SOURCES:
		project_load_sources (project, name, list, group, orphan_properties);
		break;
	case AM_TOKEN_DIR:
	case AM_TOKEN__LDFLAGS:
	case AM_TOKEN__CPPFLAGS:
	case AM_TOKEN__CFLAGS:
	case AM_TOKEN__CXXFLAGS:
	case AM_TOKEN__JAVACFLAGS:
	case AM_TOKEN__FCFLAGS:
	case AM_TOKEN__OBJCFLAGS:
	case AM_TOKEN__LFLAGS:
	case AM_TOKEN__YFLAGS:
		project_load_group_properties (project, name, variable, list, group);
		break;
	case AM_TOKEN_TARGET_LDFLAGS:
	case AM_TOKEN_TARGET_CPPFLAGS:
	case AM_TOKEN_TARGET_CFLAGS:
	case AM_TOKEN_TARGET_CXXFLAGS:
	case AM_TOKEN_TARGET_JAVACFLAGS:
	case AM_TOKEN_TARGET_FCFLAGS:
	case AM_TOKEN_TARGET_OBJCFLAGS:
	case AM_TOKEN_TARGET_LFLAGS:
	case AM_TOKEN_TARGET_YFLAGS:
	case AM_TOKEN_TARGET_DEPENDENCIES:
		project_load_target_properties (project, name, variable, list, group, orphan_properties);
		break;
	default:
		break;
	}
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
amp_project_reload (AmpProject *project, GError **error) 
{
	AmpAcScanner *scanner;
	AnjutaToken *arg;
	GFile *root_file;
	GFile *configure_file;
	gboolean ok = TRUE;
	GError *err = NULL;

	/* Unload current project */
	root_file = g_object_ref (project->root_file);
	amp_project_unload (project);
	project->root_file = root_file;
	DEBUG_PRINT ("reload project %p root file %p", project, project->root_file);

	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_object_unref);
	project->configs = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, NULL, (GDestroyNotify)amp_config_file_free);
	amp_project_new_module_hash (project);

	/* Initialize list styles */
	project->ac_space_list = anjuta_token_style_new (NULL, " ", "\n", NULL, 0);
	project->am_space_list = anjuta_token_style_new (NULL, " ", " \\\n", NULL, 0);
	project->arg_list = anjuta_token_style_new (NULL, ", ", ", ", ")", 0);
	
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
	
	/* Create root node */
	project->root_node = amp_group_new (project->root_file, FALSE);
	g_hash_table_insert (project->groups, g_file_get_uri (project->root_file), project->root_node);
	
	/* Parse configure */	
	project->configure_file = anjuta_token_file_new (configure_file);
	g_hash_table_insert (project->files, configure_file, project->configure_file);
	g_object_add_toggle_ref (G_OBJECT (project->configure_file), remove_config_file, project);
	arg = anjuta_token_file_load (project->configure_file, NULL);
	//fprintf (stdout, "AC file before parsing\n");
	//anjuta_token_dump (arg);
	//fprintf (stdout, "\n");
	scanner = amp_ac_scanner_new (project);
	project->configure_token = amp_ac_scanner_parse_token (scanner, arg, 0, &err);
	//fprintf (stdout, "AC file after parsing\n");
	//anjuta_token_check (arg);
	//anjuta_token_dump (project->configure_token);
	//fprintf (stdout, "\n");
	amp_ac_scanner_free (scanner);
	if (project->configure_token == NULL)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             	IANJUTA_PROJECT_ERROR_PROJECT_MALFORMED,
		    			err == NULL ? _("Unable to parse project file") : err->message);
		if (err != NULL) g_error_free (err);

		return FALSE;
	}
		     
	monitors_setup (project);

	/* Load all makefiles recursively */
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
	if (project->configure_token) anjuta_token_free (project->configure_token);

	if (project->root_file) g_object_unref (project->root_file);
	project->root_file = NULL;

	anjuta_project_property_foreach (project->properties, (GFunc)amp_property_free, NULL);
	project->properties = amp_get_project_property_list ();
	
	/* shortcut hash tables */
	if (project->groups) g_hash_table_destroy (project->groups);
	if (project->files) g_hash_table_destroy (project->files);
	if (project->configs) g_hash_table_destroy (project->configs);
	project->groups = NULL;
	project->files = NULL;
	project->configs = NULL;

	/* List styles */
	if (project->am_space_list) anjuta_token_style_free (project->am_space_list);
	if (project->ac_space_list) anjuta_token_style_free (project->ac_space_list);
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

gboolean
amp_project_get_token_location (AmpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token)
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

AmpGroup*
amp_project_add_sibling_group (AmpProject  *project,
    	AmpGroup *parent,
    	const gchar *name,
    	gboolean after,
    	AmpGroup *sibling,
    	GError **error)
{
	AmpGroup *last;
	AmpGroup *child;
	GFile *directory;
	GFile *makefile;
	AnjutaToken *list;
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
	directory = g_file_get_child (AMP_GROUP_DATA (parent)->base.file, name);
	uri = g_file_get_uri (directory);
	if (g_hash_table_lookup (project->groups, uri) != NULL)
	{
		g_free (uri);
		error_set (error, IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			_("Group already exists"));
		return NULL;
	}

	/* If a sibling is used, check that the parent is right */
	if ((sibling != NULL) && (parent != anjuta_project_node_parent (sibling)))
	{
		g_free (uri);
		error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			_("Sibling group has not the same parent"));
		return NULL;
	}
	
	/* Add group node in project tree */
	child = amp_group_new (directory, FALSE);
	g_hash_table_insert (project->groups, uri, child);
	g_object_unref (directory);
	if (after)
	{
		anjuta_project_node_insert_after (parent, sibling, child);
	}
	else
	{
		anjuta_project_node_insert_before (parent, sibling, child);
	}

	/* Create directory */
	g_file_make_directory (directory, NULL, NULL);

	/* Create Makefile.am */
	basename = AMP_GROUP_DATA (parent)->makefile != NULL ? g_file_get_basename (AMP_GROUP_DATA (parent)->makefile) : NULL;
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
	tfile = amp_group_set_makefile (child, makefile, project);
	g_hash_table_insert (project->files, makefile, tfile);
	g_object_add_toggle_ref (G_OBJECT (tfile), remove_config_file, project);

	if (sibling == NULL)
	{
		/* Find a sibling before */
		for (last = anjuta_project_node_prev_sibling (child); (last != NULL) && (AMP_NODE_DATA (last)->type != ANJUTA_PROJECT_GROUP); last = anjuta_project_node_prev_sibling (last));
		if (last != NULL)
		{
			sibling = last;
			after = TRUE;
		}
		else
		{
			/* Find a sibling after */
			for (last = anjuta_project_node_next_sibling (child); (last != NULL) && (AMP_NODE_DATA (last)->type != ANJUTA_PROJECT_GROUP); last = anjuta_project_node_next_sibling (last));
			if (last != NULL)
			{
				sibling = last;
				after = FALSE;
			}
		}
	}
	
	/* Add in configure */
	list = NULL;
	if (sibling) list = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_CONFIGURE);
	if (list == NULL) list= amp_group_get_first_token (parent, AM_GROUP_TOKEN_CONFIGURE);
	if (list != NULL) list = anjuta_token_list (list);
	if (list == NULL)
	{
		list = amp_project_write_config_list (project);
		list = anjuta_token_next (list);
	}
	if (list != NULL)
	{
		gchar *relative_make;
		gchar *ext;
		AnjutaToken *prev = NULL;

		if (sibling)
		{
			prev = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_CONFIGURE);
			/*if ((prev != NULL) && after)
			{
				prev = anjuta_token_next_word (prev);
			}*/
		}
		//prev_token = (AnjutaToken *)token_list->data;

		relative_make = g_file_get_relative_path (project->root_file, makefile);
		ext = relative_make + strlen (relative_make) - 3;
		if (strcmp (ext, ".am") == 0)
		{
			*ext = '\0';
		}
		//token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED,  relative_make);
		amp_project_write_config_file (project, list, after, prev, relative_make);
		g_free (relative_make);
		
		//style = anjuta_token_style_new (NULL," ","\n",NULL,0);
		//anjuta_token_add_word (prev_token, token, style);
		//anjuta_token_style_free (style);
	}
	
	/* Add in Makefile.am */
	if (sibling == NULL)
	{
		AnjutaToken *pos;
		static gint eol_type[] = {ANJUTA_TOKEN_EOL, ANJUTA_TOKEN_SPACE, ANJUTA_TOKEN_COMMENT, 0};
	
		pos = anjuta_token_find_type (AMP_GROUP_DATA (parent)->make_token, ANJUTA_TOKEN_SEARCH_NOT, eol_type);
		if (pos == NULL)
		{
			pos = anjuta_token_prepend_child (AMP_GROUP_DATA (parent)->make_token, anjuta_token_new_static (ANJUTA_TOKEN_SPACE, "\n"));
		}

		list = anjuta_token_insert_token_list (FALSE, pos,
		    	ANJUTA_TOKEN_SPACE, "\n");
		list = anjuta_token_insert_token_list (FALSE, list,
	    		AM_TOKEN_SUBDIRS, "SUBDIRS",
		    	ANJUTA_TOKEN_SPACE, " ",
		    	ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_LAST, NULL,
	    		NULL);
		list = anjuta_token_next (anjuta_token_next ( anjuta_token_next (list)));
	}
	else
	{
		AnjutaToken *prev;
		
		prev = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_SUBDIRS);
		list = anjuta_token_list (prev);
	}

	if (list != NULL)
	{
		AnjutaToken *token;
		AnjutaToken *prev;

		if (sibling)
		{
			prev = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_SUBDIRS);
		}		
		
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);
		if (after)
		{
			anjuta_token_insert_word_after (list, prev, token);
		}
		else
		{
			anjuta_token_insert_word_before (list, prev, token);
		}
	
		anjuta_token_style_format (project->am_space_list, list);
		anjuta_token_file_update (AMP_GROUP_DATA (parent)->tfile, token);
		
		amp_group_add_token (child, token, AM_GROUP_TOKEN_SUBDIRS);
	}

	return child;
}


AmpGroup* 
amp_project_add_group (AmpProject  *project,
		AmpGroup *parent,
		const gchar *name,
		GError     **error)
{
	return amp_project_add_sibling_group (project, parent, name, TRUE, NULL, error);
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
		anjuta_token_remove_word ((AnjutaToken *)token_list->data, NULL);
	}
	for (token_list = amp_group_get_token (group, AM_GROUP_TOKEN_SUBDIRS); token_list != NULL; token_list = g_list_next (token_list))
	{
		anjuta_token_remove_word ((AnjutaToken *)token_list->data, NULL);
	}
	for (token_list = amp_group_get_token (group, AM_GROUP_TOKEN_DIST_SUBDIRS); token_list != NULL; token_list = g_list_next (token_list))
	{
		anjuta_token_remove_word ((AnjutaToken *)token_list->data, NULL);
	}

	amp_group_free (group);
}

AmpTarget* 
amp_project_add_sibling_target (AmpProject  *project, AmpGroup *parent, const gchar *name, AnjutaProjectTargetType type, gboolean after, AmpTarget *sibling, GError **error)
{
	AmpTarget *child;
	AnjutaToken* token;
	AnjutaToken *args;
	AnjutaToken *var;
	AnjutaToken *prev;
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

	/* If a sibling is used, check that the parent is right */
	if ((sibling != NULL) && (parent != anjuta_project_node_parent (sibling)))
	{
		error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			_("Sibling target has not the same parent"));
		return NULL;
	}
	
	/* Check that the new target doesn't already exist */
	find = (gchar *)name;
	anjuta_project_node_children_foreach (parent, find_target, &find);
	if ((gchar *)find != name)
	{
		error_set (error, IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			_("Target already exists"));

		return NULL;
	}
	
	/* Add target node in project tree */
	child = amp_target_new (name, type, "", 0);
	if (after)
	{
		anjuta_project_node_insert_after (parent, sibling, child);
	}
	else
	{
		anjuta_project_node_insert_before (parent, sibling, child);
	}
	//anjuta_project_node_append (parent, child);

	/* Add in Makefile.am */
	targetname = g_strconcat (((AmpTargetInformation *)type)->install, ((AmpTargetInformation *)type)->prefix, NULL);

	// Get token corresponding to sibling and check if the target are compatible
	args = NULL;
	var = NULL;
	if (sibling != NULL)
	{
		last = amp_target_get_token (sibling);

		if (last != NULL) 
		{
			AnjutaToken *token = (AnjutaToken *)last->data;

			token = anjuta_token_list (token);
			if (token != NULL)
			{
				token = anjuta_token_list (token);
				var = token;
				if (token != NULL)
				{
					token = anjuta_token_first_item (token);
					if (token != NULL)
					{
						gchar *value;
						
						value = anjuta_token_evaluate (token);
		
						if ((value != NULL) && (strcmp (targetname, value) == 0))
						{
							g_free (value);
							prev = (AnjutaToken *)last->data;
							args = anjuta_token_last_item (anjuta_token_list (prev));
						}
					}
				}
			}	
		}
	}

	if (args == NULL)
	{
		for (last = amp_group_get_token (parent, AM_GROUP_TARGET); last != NULL; last = g_list_next (last))
		{
			gchar *value = anjuta_token_evaluate ((AnjutaToken *)last->data);
		
			if ((value != NULL) && (strcmp (targetname, value) == 0))
			{
				g_free (value);
				args = anjuta_token_last_item (anjuta_token_list ((AnjutaToken *)last->data));
				break;
			}
			g_free (value);
		}
	}


	if (args == NULL)
	{
		args = amp_project_write_target (AMP_GROUP_DATA (parent)->make_token, ((AmpTargetInformation *)type)->token, targetname, after, var);
	}
	g_free (targetname);
	
	if (args != NULL)
	{
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);
		if (after)
		{
			anjuta_token_insert_word_after (args, prev, token);
		}
		else
		{
			anjuta_token_insert_word_before (args, prev, token);
		}
	
		anjuta_token_style_format (project->am_space_list, args);
		anjuta_token_file_update (AMP_GROUP_DATA (parent)->tfile, token);
		
		amp_target_add_token (child, token);
	}

	return child;
}

AmpTarget*
amp_project_add_target (AmpProject  *project,
		 AmpGroup *parent,
		 const gchar *name,
		 AnjutaProjectTargetType type,
		 GError     **error)
{
	return amp_project_add_sibling_target (project, parent, name, type, TRUE, NULL, error);
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
		anjuta_token_remove_word ((AnjutaToken *)token_list->data, NULL);
	}

	amp_target_free (target);
}

AmpSource* 
amp_project_add_sibling_source (AmpProject  *project, AmpTarget *target, GFile *file, gboolean after, AmpSource *sibling, GError **error)
{
	AmpGroup *group;
	AmpSource *source;
	AnjutaToken *token;
	AnjutaToken *prev;
	AnjutaToken *args;
	gchar *relative_name;
	
	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (target != NULL, NULL);

	if (AMP_NODE_DATA (target)->type != ANJUTA_PROJECT_TARGET) return NULL;
	
	group = (AmpGroup *)(target->parent);
	relative_name = g_file_get_relative_path (AMP_GROUP_DATA (group)->base.file, file);

	/* Add in Makefile.am */

	// Get token corresponding to sibling and check if the target are compatible
	prev = NULL;
	args = NULL;
	if (sibling != NULL)
	{
		prev = AMP_SOURCE_DATA (sibling)->token;
		args = anjuta_token_list (prev);
	}

	if (args == NULL)
	{
		gchar *target_var;
		gchar *canon_name;
		AnjutaToken *var;
		GList *list;
		
		canon_name = canonicalize_automake_variable (AMP_TARGET_DATA (target)->base.name);
		target_var = g_strconcat (canon_name,  "_SOURCES", NULL);

		/* Search where the target is declared */
		var = NULL;
		list = amp_target_get_token (target);
		if (list != NULL)
		{
			var = (AnjutaToken *)list->data;
			if (var != NULL)
			{
				var = anjuta_token_list (var);
				if (var != NULL)
				{
					var = anjuta_token_list (var);
				}
			}
		}
		
		args = amp_project_write_source_list (AMP_GROUP_DATA (group)->make_token, target_var, after, var);
		g_free (target_var);
	}
	
	if (args != NULL)
	{
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, relative_name);
		if (after)
		{
			anjuta_token_insert_word_after (args, prev, token);
		}
		else
		{
			anjuta_token_insert_word_before (args, prev, token);
		}
	
		anjuta_token_style_format (project->am_space_list, args);
		anjuta_token_file_update (AMP_GROUP_DATA (group)->tfile, token);
	}

	/* Add source node in project tree */
	source = amp_source_new (file);
	AMP_SOURCE_DATA(source)->token = token;
	if (after)
	{
		anjuta_project_node_insert_after (target, sibling, source);
	}
	else
	{
		anjuta_project_node_insert_before (target, sibling, source);
	}

	return source;
}


AmpSource* 
amp_project_add_source (AmpProject  *project,
		 AmpTarget *target,
		 GFile *file,
		 GError     **error)
{
	return amp_project_add_sibling_source (project, target, file, TRUE, NULL, error);
}

void 
amp_project_remove_source (AmpProject  *project,
		    AmpSource *source,
		    GError     **error)
{
	amp_dump_node (source);
	if (AMP_NODE_DATA (source)->type != ANJUTA_PROJECT_SOURCE) return;
	
	anjuta_token_remove_word (AMP_SOURCE_DATA (source)->token, NULL);

	amp_source_free (source);
}

GList *
amp_project_get_config_modules   (AmpProject *project, GError **error)
{
	AmpModule *module;
	GList *modules = NULL;

	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (module != NULL, NULL);
	
	for (module = anjuta_project_node_first_child (project->root_node); module != NULL; module = anjuta_project_node_next_sibling (module))
	{
		if (anjuta_project_node_get_type(module) == ANJUTA_PROJECT_MODULE)
		{
				modules = g_list_prepend (modules, anjuta_project_node_get_name (module));
		}
	}
	modules = g_list_reverse (modules);

	return modules;
}

GList *
amp_project_get_config_packages  (AmpProject *project,
			   const gchar* module_name,
			   GError **error)
{
	AmpModule *module;
	GList *packages = NULL;

	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (module != NULL, NULL);
	
	for (module = anjuta_project_node_first_child (project->root_node); module != NULL; module = anjuta_project_node_next_sibling (module))
	{
		gchar *name = anjuta_project_node_get_name (module);

		if ((anjuta_project_node_get_type(module) == ANJUTA_PROJECT_MODULE) && (strcmp (name, module_name) == 0))
		{
			AmpPackage *package;

			for (package = anjuta_project_node_first_child (module); package != NULL; package = anjuta_project_node_next_sibling (package))
			{
				if (anjuta_project_node_get_type (package) == ANJUTA_PROJECT_PACKAGE)
				{
					packages = g_list_prepend (packages, anjuta_project_node_get_name (package));
				}
			}
		}
	}
	packages = g_list_reverse (packages);

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

typedef struct _AmpMovePacket {
	AmpProject *project;
	GFile *old_root_file;
} AmpMovePacket;

static void
foreach_node_move (AnjutaProjectNode *g_node, gpointer data)
{
	AmpProject *project = ((AmpMovePacket *)data)->project;
	const gchar *old_root_file = ((AmpMovePacket *)data)->old_root_file;
	GFile *relative;
	GFile *new_file;
	
	switch (AMP_NODE_DATA (g_node)->type)
	{
	case ANJUTA_PROJECT_GROUP:
		relative = get_relative_path (old_root_file, AMP_GROUP_DATA (g_node)->base.file);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (AMP_GROUP_DATA (g_node)->base.file);
		AMP_GROUP_DATA (g_node)->base.file = new_file;

		g_hash_table_insert (project->groups, g_file_get_uri (new_file), g_node);
		break;
	case ANJUTA_PROJECT_SOURCE:
		relative = get_relative_path (old_root_file, AMP_SOURCE_DATA (g_node)->base.file);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (AMP_SOURCE_DATA (g_node)->base.file);
		AMP_SOURCE_DATA (g_node)->base.file = new_file;
		break;
	default:
		break;
	}
}

gboolean
amp_project_move (AmpProject *project, const gchar *path)
{
	GFile *new_file;
	gchar *relative;
	GHashTableIter iter;
	gpointer key;
	AnjutaTokenFile *tfile;
	AmpConfigFile *cfg;
	GHashTable* old_hash;
	AmpMovePacket packet= {project, NULL};

	/* Change project root directory */
	packet.old_root_file = project->root_file;
	project->root_file = g_file_new_for_path (path);

	/* Change project root directory in groups */
	old_hash = project->groups;
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	anjuta_project_node_all_foreach (project->root_node, foreach_node_move, &packet);
	g_hash_table_destroy (old_hash);

	/* Change all files */
	old_hash = project->files;
	project->files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_object_unref);
	g_hash_table_iter_init (&iter, old_hash);
	while (g_hash_table_iter_next (&iter, &key, (gpointer *)&tfile))
	{
		relative = get_relative_path (packet.old_root_file, anjuta_token_file_get_file (tfile));
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
	project->configs = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, NULL, (GDestroyNotify)amp_config_file_free);
	g_hash_table_iter_init (&iter, old_hash);
	while (g_hash_table_iter_next (&iter, &key, (gpointer *)&cfg))
	{
		relative = get_relative_path (packet.old_root_file, cfg->file);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (cfg->file);
		cfg->file = new_file;
		
		g_hash_table_insert (project->configs, new_file, cfg);
	}
	g_hash_table_steal_all (old_hash);
	g_hash_table_destroy (old_hash);

	
	g_object_unref (packet.old_root_file);

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

	switch (AMP_NODE_DATA (node)->type)
	{
		case ANJUTA_PROJECT_GROUP:
			return g_file_get_uri (AMP_GROUP_DATA (node)->base.file);
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

AnjutaProjectPropertyList *
amp_project_get_property_list (AmpProject *project)
{
	return project->properties;
}

#if 0
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
	if (project->property == NULL)
	{
		project->property = amp_property_new (NULL, NULL);
	}
	switch (type)
	{
		case AMP_PROPERTY_NAME:
			STR_REPLACE (project->property->name, value);
			break;
		case AMP_PROPERTY_VERSION:
			STR_REPLACE (project->property->version, value);
			break;
		case AMP_PROPERTY_BUG_REPORT:
			STR_REPLACE (project->property->bug_report, value);
			break;
		case AMP_PROPERTY_TARNAME:
			STR_REPLACE (project->property->tarname, value);
			break;
		case AMP_PROPERTY_URL:
			STR_REPLACE (project->property->url, value);
			break;
	}
	
	return amp_project_update_property (project, type);
}
#endif

/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static AnjutaProjectNode* 
iproject_add_group (IAnjutaProject *obj, AnjutaProjectNode *parent,  const gchar *name, GError **err)
{
	return amp_project_add_group (AMP_PROJECT (obj), AMP_GROUP (parent), name, err);
}

static AnjutaProjectNode* 
iproject_add_source (IAnjutaProject *obj, AnjutaProjectNode *parent,  GFile *file, GError **err)
{
	return amp_project_add_source (AMP_PROJECT (obj), AMP_TARGET (parent), file, err);
}

static AnjutaProjectNode* 
iproject_add_target (IAnjutaProject *obj, AnjutaProjectNode *parent,  const gchar *name,  AnjutaProjectTargetType type, GError **err)
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

static AnjutaProjectNode* 
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

static GtkWidget*
iproject_configure_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	switch (AMP_NODE_DATA (node)->type)
	{
		case ANJUTA_PROJECT_GROUP:
			return amp_configure_group_dialog (AMP_PROJECT (obj), AMP_GROUP (node), err);
		case ANJUTA_PROJECT_TARGET:
			return amp_configure_target_dialog (AMP_PROJECT (obj), AMP_TARGET (node), err);
		case ANJUTA_PROJECT_SOURCE:
			return amp_configure_source_dialog (AMP_PROJECT (obj), AMP_SOURCE (node), err);
		default:
			return NULL;
	}
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
	iface->configure_node = iproject_configure_node;
}

/* Group access functions
 *---------------------------------------------------------------------------*/

GFile*
amp_group_get_directory (AmpGroup *group)
{
	return AMP_GROUP_DATA (group)->base.file;
}

GFile*
amp_group_get_makefile (AmpGroup *group)
{
	return AMP_GROUP_DATA (group)->makefile;
}

gchar *
amp_group_get_id (AmpGroup *group)
{
	return g_file_get_uri (AMP_GROUP_DATA (group)->base.file);
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
	return AMP_TARGET_DATA (target)->base.target_type;
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
	project->configure_token = NULL;
	project->root_node = NULL;
	project->properties = amp_get_project_property_list ();
	project->ac_init = NULL;
	project->args = NULL;

	project->am_space_list = NULL;
	project->ac_space_list = NULL;
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

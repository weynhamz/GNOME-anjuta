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
#include "amp-node.h"
#include "amp-module.h"
#include "amp-package.h"
#include "amp-group.h"
#include "amp-target.h"
#include "amp-source.h"
#include "command-queue.h"

#include <libanjuta/interfaces/ianjuta-project.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-pkg-config.h>

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
#include "am-writer.h"
//#include "am-config.h"
#include "am-properties.h"


#define UNIMPLEMENTED  G_STMT_START { g_warning (G_STRLOC": unimplemented"); } G_STMT_END

/* Constant strings for parsing perl script error output */
#define ERROR_PREFIX      "ERROR("
#define WARNING_PREFIX    "WARNING("
#define MESSAGE_DELIMITER ": "

const gchar *valid_am_makefiles[] = {"GNUmakefile.am", "makefile.am", "Makefile.am", NULL};


#define STR_REPLACE(target, source) \
	{ g_free (target); target = source == NULL ? NULL : g_strdup (source);}


typedef struct _AmpConfigFile AmpConfigFile;

struct _AmpConfigFile {
	GFile *file;
	AnjutaToken *token;
};

/* Node types
 *---------------------------------------------------------------------------*/

static AmpNodeInfo AmpNodeInformations[] = {
	{{ANJUTA_PROJECT_GROUP,
	N_("Group"),
	"text/plain"},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_SOURCE,
	N_("Source"),
	"text/plain"},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_UNKNOWN | ANJUTA_PROJECT_READ_ONLY,
	/* Translator: Unknown here is a target type, if not unknown it can
	 * be a program or a shared library by example */
	N_("Unknown"),
	"text/plain"},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PRIMARY |  ANJUTA_PROJECT_SHAREDLIB,
	N_("Shared Library"),
	"application/x-sharedlib"},
	AM_TOKEN__LTLIBRARIES,
	"LTLIBRARIES",
	"lib"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PRIMARY |  ANJUTA_PROJECT_STATICLIB,
	N_("Static Library"),
	"application/x-archive"},
	AM_TOKEN__LIBRARIES,
	"LIBRARIES",
	"lib"},

	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PRIMARY | ANJUTA_PROJECT_PROGRAM | ANJUTA_PROJECT_EXECUTABLE,
	N_("Program"),
	"application/x-executable"},
	AM_TOKEN__PROGRAMS,
	"PROGRAMS",
	"bin"},

	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PYTHON,
	N_("Python Module"),
	"application/x-python"},
	AM_TOKEN__PYTHON,
	"PYTHON",
	"python"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_JAVA,
	N_("Java Module"),
	"application/x-java"},
	AM_TOKEN__JAVA,
	"JAVA",
	"java"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_LISP,
	N_("Lisp Module"),
	"text/plain"},
	AM_TOKEN__LISP,
	"LISP",
	"lisp"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_HEADER,
	N_("Header Files"),
	"text/x-chdr"},
	AM_TOKEN__HEADERS,
	"HEADERS",
	"include"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_MAN,
	N_("Man Documentation"),
	"text/x-troff-man"},
	AM_TOKEN__MANS,
	"MANS",
	"man"},

	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_INFO,
	N_("Info Documentation"),
	"application/x-tex-info"},
	AM_TOKEN__TEXINFOS,
	"TEXINFOS",
	"info"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_DATA,
	N_("Miscellaneous Data"),
	"application/octet-stream"},
	AM_TOKEN__DATA,
	"DATA",
	"data"},
	
	{{ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_SCRIPT,
	N_("Script"),
	"text/x-shellscript"},
	AM_TOKEN__SCRIPTS,
	"SCRIPTS",
	"bin"},

	{{ANJUTA_PROJECT_MODULE,
	N_("Module"),
	""},
	ANJUTA_TOKEN_NONE,
	NULL,
	NULL},

	{{ANJUTA_PROJECT_PACKAGE,
	N_("Package"),
	""},
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

/* Work even if file is not a descendant of parent */
gchar*
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

GFileType
file_type (GFile *file, const gchar *filename)
{
	GFile *child;
	GFileInfo *info;
	GFileType type;

	child = filename != NULL ? g_file_get_child (file, filename) : g_object_ref (file);

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
gchar*
canonicalize_automake_variable (const gchar *name)
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

static void
amp_project_clear (AmpProject *project)
{
	if (project->configure_file != NULL) anjuta_token_file_free (project->configure_file);
	project->configure_file = NULL;
	if (project->configure_token) anjuta_token_free (project->configure_token);
}

static void
on_project_monitor_changed (GFileMonitor *monitor,
											GFile *file,
											GFile *other_file,
											GFileMonitorEvent event_type,
											gpointer data)
{
	AmpProject *project = AMP_PROJECT (data);

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* project can be NULL, if the node is dummy node because the
			 * original one is reloaded. */
			g_signal_emit_by_name (G_OBJECT (project), "file-changed", data);
			break;
		default:
			break;
	}
}

AnjutaTokenFile*
amp_project_set_configure (AmpProject *project, GFile *configure)
{
	if (project->configure != NULL) g_object_unref (project->configure);
	if (project->configure_file != NULL) anjuta_token_file_free (project->configure_file);
	if (project->monitor) g_object_unref (project->monitor);
	if (configure != NULL)
	{
		project->configure_file = anjuta_token_file_new (configure);
		project->configure = g_object_ref (configure);

		project->monitor = g_file_monitor_file (configure, 
						      									G_FILE_MONITOR_NONE,
						       									NULL,
						       									NULL);
		if (project->monitor != NULL)
		{
			g_signal_connect (G_OBJECT (project->monitor),
					  "changed",
					  G_CALLBACK (on_project_monitor_changed),
					  project);
		}
	}
	else
	{
		project->configure_file = NULL;
		project->configure = NULL;
		project->monitor = NULL;
	}
	
	return project->configure_file;
}

gboolean
amp_project_update_configure (AmpProject *project, AnjutaToken *token)
{
	return anjuta_token_file_update (project->configure_file, token);
}

AnjutaToken*
amp_project_get_configure_token (AmpProject *project)
{
	return project->configure_token;

}

AnjutaToken *
amp_project_get_config_token (AmpProject *project, GFile *file)
{
	AmpConfigFile *config;

	config = g_hash_table_lookup (project->configs, file);

	return config != NULL ? config->token : NULL;
}

static void
remove_config_file (gpointer data, GObject *object)
{
	AmpProject *project = (AmpProject *)data;
	
	g_return_if_fail (project->files != NULL);

	project->files = g_list_remove (project->files, object);
}

void
amp_project_update_root (AmpProject *project, AmpProject *new_project)
{
	GHashTable *hash;
	GList *list;
	AnjutaTokenStyle *style;

	if (project->configure != NULL) g_object_unref (project->configure);
	if (project->configure_file != NULL) anjuta_token_file_free (project->configure_file);
	if (project->monitor) g_object_unref (project->monitor);

	project->configure = new_project->configure;
	if (project->configure != NULL)
	{
		project->monitor = g_file_monitor_file (project->configure,
						      						G_FILE_MONITOR_NONE,
						       						NULL,
						       						NULL);
		if (project->monitor != NULL)
		{
			g_signal_connect (G_OBJECT (project->monitor),
					  "changed",
					  G_CALLBACK (on_project_monitor_changed),
					  project);
		}
	}
	else
	{
		project->monitor = NULL;
	}
	new_project->configure = NULL;
	project->configure_file = new_project->configure_file;
	new_project->configure_file = NULL;
	project->configure_token = new_project->configure_token;

	project->ac_init = new_project->ac_init;
	project->args   = new_project->args;

	hash = project->groups;
	project->groups = new_project->groups;
	new_project->groups = hash;

	list = project->files;
	project->files = new_project->files;
	new_project->files = list;

	for (list = project->files; list != NULL; list = g_list_next (list))
	{
		GObject *tfile = (GObject *)list->data;
		
		g_object_weak_unref (tfile, remove_config_file, new_project);
		g_object_weak_ref (tfile, remove_config_file, project);
	}
	for (list = new_project->files; list != NULL; list = g_list_next (list))
	{
		GObject *tfile = (GObject *)list->data;
		
		g_object_weak_unref (tfile, remove_config_file, project);
		g_object_weak_ref (tfile, remove_config_file, new_project);
	}
	
	hash = project->configs;
	project->configs = new_project->configs;
	new_project->configs = hash;


	style = project->ac_space_list;
	project->ac_space_list = new_project->ac_space_list;
	new_project->ac_space_list = style;
	
	style = project->am_space_list;
	project->am_space_list = new_project->am_space_list;
	new_project->am_space_list = style;

	style = project->arg_list;
	project->arg_list = new_project->arg_list;
	new_project->arg_list = style;
}


/* Target objects
 *---------------------------------------------------------------------------*/

static gboolean
find_target (AnjutaProjectNode *node, gpointer data)
{
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_TARGET)
	{
		if (strcmp (anjuta_project_node_get_name (node), *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(AnjutaProjectNode **)data = node;

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
find_canonical_target (AnjutaProjectNode *node, gpointer data)
{
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_TARGET)
	{
		gchar *canon_name = canonicalize_automake_variable (anjuta_project_node_get_name (node));	
		DEBUG_PRINT ("compare canon %s vs %s node %p", canon_name, *(gchar **)data, node);
		if (strcmp (canon_name, *(gchar **)data) == 0)
		{
			/* Find target, return node value in pointer */
			*(AnjutaProjectNode **)data = node;
			g_free (canon_name);

			return TRUE;
		}
		g_free (canon_name);
	}

	return FALSE;
}

/*
 * ---------------- Data structures managment
 */

void
amp_project_load_properties (AmpProject *project, AnjutaToken *macro, AnjutaToken *args)
{
	GList *item;
	
	project->ac_init = macro;
	project->args = args;

	for (item = anjuta_project_node_get_native_properties (ANJUTA_PROJECT_NODE (project)); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		if (prop->position >= 0)
		{
			AnjutaProjectProperty *new_prop;
			AnjutaToken *arg;

			new_prop = anjuta_project_node_remove_property (ANJUTA_PROJECT_NODE (project), (AnjutaProjectProperty *)prop);
			if (new_prop != NULL)
			{
				amp_property_free (new_prop);
			}
			new_prop = amp_property_new (NULL, prop->token_type, prop->position, NULL, macro);
			arg = anjuta_token_nth_word (args, prop->position);
			if ((new_prop->value != NULL) && (new_prop->value != prop->base.value))
			{
				g_free (new_prop->value);
			}
			new_prop->value = anjuta_token_evaluate (arg);
			anjuta_project_node_insert_property (ANJUTA_PROJECT_NODE (project), (AnjutaProjectProperty *)prop, new_prop);
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
		AmpModuleNode *module;
		AmpPackageNode *package;
		gchar *compare;

		/* Module name */
		arg = anjuta_token_first_item (module_token);
		value = anjuta_token_evaluate (arg);
		module = amp_module_node_new (value);
		amp_module_node_add_token (module, module_token);
		anjuta_project_node_append (ANJUTA_PROJECT_NODE (project), ANJUTA_PROJECT_NODE (module));

		/* Package list */
		arg = anjuta_token_next_word (arg);
		if (arg != NULL)
		{
			scanner = amp_ac_scanner_new (project);
			list = amp_ac_scanner_parse_token (scanner, arg, AC_SPACE_LIST_STATE, NULL);
			anjuta_token_free_children (arg);
			list = anjuta_token_delete_parent (list);
			anjuta_token_prepend_items (arg, list);
			amp_ac_scanner_free (scanner);
		}
		
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
				amp_package_node_set_version (package, compare, value);
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
				package = amp_package_node_new (value);
				amp_package_node_add_token (package, item);
				anjuta_project_node_append (ANJUTA_PROJECT_NODE (module), ANJUTA_PROJECT_NODE (package));
				anjuta_project_node_set_state (ANJUTA_PROJECT_NODE (package), ANJUTA_PROJECT_INCOMPLETE);
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
		
			cfg = amp_config_file_new (value, anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project)), item);
			g_hash_table_replace (project->configs, cfg->file, cfg);
			g_free (value);
		}
	}
}

static AnjutaToken*
project_load_target (AmpProject *project, AnjutaProjectNode *parent, AnjutaToken *variable, GHashTable *orphan_properties)
{
	AnjutaToken *arg;
	gchar *install = NULL;
	gchar *value;
	gint flags = 0;
	AmpNodeInfo *info = AmpNodeInformations; 

	while (info->base.type != 0)
	{
		if (anjuta_token_get_type (variable) == info->token)
		{
			break;
		}
		info++;
	}

	value = anjuta_token_evaluate (anjuta_token_first_word (variable));
	split_automake_variable (value, &flags, &install, NULL);

	amp_group_node_add_token (AMP_GROUP_NODE (parent), variable, AM_GROUP_TARGET);

	for (arg = anjuta_token_first_word (anjuta_token_last_item (variable)); arg != NULL; arg = anjuta_token_next_word (arg))
	{
		gchar *value;
		gchar *canon_id;
		AmpTargetNode *target;
		AmpTargetNode *orphan;
		gchar *orig_key;
		gpointer find;

		value = anjuta_token_evaluate (arg);

		/* This happens for variable token which are considered as value */
		if (value == NULL) continue;
		canon_id = canonicalize_automake_variable (value);
		
		/* Check if target already exists */
		find = value;
		anjuta_project_node_children_traverse (parent, find_target, &find);
		if ((gchar *)find != value)
		{
			/* Find target */
			g_free (canon_id);
			g_free (value);
			continue;
		}

		/* Create target */
		target = amp_target_node_new_valid (value, info->base.type, install, flags, NULL);
		if (target != NULL)
		{
			amp_target_node_add_token (target, ANJUTA_TOKEN_ARGUMENT, arg);
			anjuta_project_node_append (parent, ANJUTA_PROJECT_NODE (target));
			DEBUG_PRINT ("create target %p name %s", target, value);

			/* Check if there are sources or properties availables */
			if (g_hash_table_lookup_extended (orphan_properties, canon_id, (gpointer *)&orig_key, (gpointer *)&orphan))
			{
				AnjutaTokenType type;
				GList *properties;
				AnjutaProjectNode *child;

				g_hash_table_steal (orphan_properties, canon_id);
				
				/* Copy all token */
				for (type = amp_target_node_get_first_token_type (orphan); type != 0; type = amp_target_node_get_next_token_type (orphan, type))
				{
					GList *tokens;
					tokens = amp_target_node_get_token (orphan, type);

					for (tokens = g_list_first (tokens); tokens != NULL; tokens = g_list_next (tokens))
					{
						AnjutaToken *token = (AnjutaToken *)tokens->data;

						amp_target_node_add_token (target, type, token);
					}
				}

				/* Copy all properties */
				while ((properties = anjuta_project_node_get_custom_properties (ANJUTA_PROJECT_NODE (orphan))) != NULL)
				{
					AnjutaProjectProperty *prop;
					
					prop = (AnjutaProjectProperty *)anjuta_project_node_remove_property (ANJUTA_PROJECT_NODE (orphan), (AnjutaProjectProperty *)properties->data);

					amp_node_property_add (ANJUTA_PROJECT_NODE (target), prop);
				}

				/* Copy all sources */
				while ((child = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (orphan))) != NULL)
				{
					anjuta_project_node_remove (child);
					anjuta_project_node_append (ANJUTA_PROJECT_NODE (target), child);
					g_object_unref (child);
				}
				amp_target_changed (target);
				g_free (orig_key);
				amp_target_node_free (orphan);
			}

			/* Set target properties */
			if (flags & AM_TARGET_NOBASE) 
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 0, "1", arg);
			if (flags & AM_TARGET_NOTRANS) 
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 1, "1", arg);
			if (flags & AM_TARGET_DIST) 
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 2, "1", arg);
			if (flags & AM_TARGET_NODIST) 
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 2, "0", arg);
			if (flags & AM_TARGET_NOINST) 
			{
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 3, "1", arg);
			}
			else if (install != NULL)
			{
				gchar *instdir = g_strconcat (install, "dir", NULL);
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 6, instdir, arg);
				g_free (instdir);
			}
		
			if (flags & AM_TARGET_CHECK) 
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 4, "1", arg);
			if (flags & AM_TARGET_MAN)
			{
				gchar section[] = "0";

				section[0] += (flags >> 7) & 0x1F;
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 4, section, arg);
			}
		}
		
		g_free (canon_id);
		g_free (value);
	}
	g_free (value);

	return NULL;
}

static AnjutaToken*
project_load_sources (AmpProject *project, AnjutaProjectNode *group, AnjutaToken *variable, GHashTable *orphan_properties)
{
	AnjutaToken *arg;
	GFile *group_file = g_object_ref (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (group)));
	gchar *target_id = NULL;

	target_id = anjuta_token_evaluate (anjuta_token_first_word (variable));
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
		AnjutaProjectNode *target;
		
		find = target_id;
		DEBUG_PRINT ("search for canonical %s", target_id);
		anjuta_project_node_children_traverse (group, find_canonical_target, &find);
		target = (gchar *)find != target_id ? (AnjutaProjectNode *)find : NULL;

		/* Get orphan buffer if there is no target */
		if (target == NULL)
		{
			gchar *orig_key;
			
			if (g_hash_table_lookup_extended (orphan_properties, target_id, (gpointer *)&orig_key, (gpointer *)&target))
			{
				g_hash_table_steal (orphan_properties, target_id);
				g_free (orig_key);
			}
			else
			{
				target = ANJUTA_PROJECT_NODE (amp_target_node_new ("dummy", 0, NULL, 0));
			}
			g_hash_table_insert (orphan_properties, target_id, target);
		}
		else
		{
			g_free (target_id);
		}
		amp_target_node_add_token (AMP_TARGET_NODE (target), AM_TOKEN__SOURCES, variable);
		
		for (arg = anjuta_token_first_word (anjuta_token_last_item (variable)); arg != NULL; arg = anjuta_token_next_word (arg))
		{
			gchar *value;
			AnjutaProjectNode *source;
			AnjutaProjectNode *parent = target;
			GFile *src_file;
			GFileInfo* file_info;
		
			value = anjuta_token_evaluate (arg);
			if (value == NULL) continue;

			src_file = g_file_get_child (group_file, value);
			if (project->lang_manager != NULL)
			{
				file_info = g_file_query_info (src_file,
					                                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				    	                                      G_FILE_QUERY_INFO_NONE,
		    		    	                                  NULL,
		        		    	                              NULL);
				if (file_info)
				{
					gint id = ianjuta_language_get_from_mime_type (project->lang_manager,
			    		                                           g_file_info_get_content_type (file_info),
			        		                                       NULL);
					if (id > 0)
					{
						const gchar *obj_ext = ianjuta_language_get_make_target (project->lang_manager, id, NULL);
						if (obj_ext != NULL)
						{
							/* Create object node */
							gchar *object_name;
							gchar *basename;
							gchar *ext;
							GFile *obj_file;
							AnjutaProjectNode *object;

							ext = strrchr (value, '.');
							if ((ext != NULL) && (ext != value)) *ext = '\0';
							object_name = g_strconcat (value, obj_ext, NULL);
							basename = g_path_get_basename (object_name);
							obj_file = g_file_get_child (group_file, basename);
							g_free (basename);
							g_free (object_name);
							object = amp_node_new_valid (group, ANJUTA_PROJECT_OBJECT | ANJUTA_PROJECT_PROJECT, obj_file, NULL, NULL); 
							g_object_unref (obj_file);
							anjuta_project_node_append (parent, object);
							parent = object;
						}
					}
					g_object_unref (file_info);
				}
			}
			
			/* Create source */
			source = amp_node_new_valid (group, ANJUTA_PROJECT_SOURCE | ANJUTA_PROJECT_PROJECT, src_file, NULL, NULL);
			g_object_unref (src_file);
			if (source != NULL)
			{
				amp_source_node_add_token (AMP_SOURCE_NODE (source), arg);
	
				DEBUG_PRINT ("add target child %p", target);
				/* Add as target child */
				anjuta_project_node_append (parent, source);
			}

			g_free (value);
		}
		amp_target_changed (target);
	}

	g_object_unref (group_file);

	return NULL;
}

static AnjutaToken*
project_load_data (AmpProject *project, AnjutaProjectNode *parent, AnjutaToken *variable, GHashTable *orphan_properties, gint data_flags)
{
	gchar *install = NULL;
	AmpTargetNode *target;
	gchar *target_id;
	gpointer find;
	gint flags = 0;
	AmpNodeInfo *info = AmpNodeInformations; 
	AnjutaToken *arg;
	AnjutaToken *list;

	while (info->base.name != NULL)
	{
		if (anjuta_token_get_type (variable) == info->token)
		{
			break;
		}
		info++;
	}

	target_id = anjuta_token_evaluate (anjuta_token_first_word (variable));
	split_automake_variable (target_id, &flags, &install, NULL);
	
	amp_group_node_add_token (AMP_GROUP_NODE (parent), variable, AM_GROUP_TARGET);

	/* Check if target already exists */
	find = target_id;
	anjuta_project_node_children_traverse (parent, find_target, &find);
	if ((gchar *)find == target_id)
	{
		/* Create target */
		target = amp_target_node_new_valid (target_id, info->base.type, install, flags, NULL);
		if (target != NULL)
		{
			anjuta_project_node_append (parent, ANJUTA_PROJECT_NODE (target));
			DEBUG_PRINT ("create target %p name %s", target, target_id);
		}
	}
	else
	{
		target = AMP_TARGET_NODE (find);
	}

	if (target != NULL)
	{
		GFile *parent_file = g_object_ref (anjuta_project_node_get_file (parent));

		amp_target_node_add_token (AMP_TARGET_NODE (target), AM_TOKEN__DATA, variable);

		list = anjuta_token_last_item (variable);
		for (arg = anjuta_token_first_word (list); arg != NULL; arg = anjuta_token_next_word (arg))
		{
			gchar *value;
			AnjutaProjectNode *source;
			GFile *src_file;
		
			value = anjuta_token_evaluate (arg);
			if (value == NULL) continue;

			/* Create source */
			src_file = g_file_get_child (parent_file, value);
			source = amp_node_new_valid (parent, ANJUTA_PROJECT_SOURCE | ANJUTA_PROJECT_PROJECT | data_flags, src_file, NULL, NULL);
			g_object_unref (src_file);
			if (source != NULL)
			{
				amp_source_node_add_token (AMP_SOURCE_NODE(source), arg);

				/* Add as target child */
				DEBUG_PRINT ("add target child %p", target);
				anjuta_project_node_append (ANJUTA_PROJECT_NODE (target), source);
			}

			g_free (value);
		}
		g_object_unref (parent_file);

		/* Set target properties */
		if (flags & AM_TARGET_NOBASE) 
			amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 0, "1", arg);
		if (flags & AM_TARGET_NOTRANS) 
			amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 1, "1", arg);
		if (flags & AM_TARGET_DIST) 
			amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 2, "1", arg);
		if (flags & AM_TARGET_NODIST) 
			amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 2, "0", arg);
		if (flags & AM_TARGET_NOINST) 
		{
			amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 3, "1", arg);
		}
		else if (install != NULL)
		{
				gchar *instdir = g_strconcat (install, "dir", NULL);
				amp_node_property_load (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 6, instdir, arg);
				g_free (instdir);
		}
	}
	g_free (target_id);
	
	return NULL;
}

static AnjutaToken*
project_load_target_properties (AmpProject *project, AnjutaProjectNode *parent, AnjutaToken *variable, GHashTable *orphan_properties)
{
	gchar *target_id = NULL;
	
	target_id = anjuta_token_evaluate (anjuta_token_first_word (variable));
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
		AnjutaProjectProperty *prop;
		AnjutaToken *list;
		AnjutaTokenType type;
		
		find = target_id;
		DEBUG_PRINT ("search for canonical %s", target_id);
		anjuta_project_node_children_traverse (parent, find_canonical_target, &find);
		parent = (gchar *)find != target_id ? (AnjutaProjectNode *)find : NULL;

		/* Create property */
		list = anjuta_token_last_item (variable);
		type = anjuta_token_get_type (variable);
		value = anjuta_token_evaluate (list);
		prop = amp_property_new (NULL, type, 0, value, list);

		if (parent == NULL)
		{
			/* Add property to non existing target, create a dummy target */
			gchar *orig_key;
			
			if (g_hash_table_lookup_extended (orphan_properties, target_id, (gpointer *)&orig_key, (gpointer *)&parent))
			{
				/* Dummy target already created */
				g_hash_table_steal (orphan_properties, target_id);
				g_free (orig_key);
			}
			else
			{
				/* Create dummy target */
				parent = ANJUTA_PROJECT_NODE (amp_target_node_new ("dummy", 0, NULL, 0));
			}
			g_hash_table_insert (orphan_properties, target_id, parent);
		}
		else
		{
			g_free (target_id);
		}
		g_free (value);

		/* Add property to target */
		amp_node_property_add (parent, prop);
		amp_target_node_add_token (AMP_TARGET_NODE (parent), type, variable);
		amp_target_changed (AMP_TARGET_NODE (parent));
	}

	return NULL;
}

static AnjutaToken*
project_load_group_properties (AmpProject *project, AnjutaProjectNode *parent, AnjutaToken *variable)
{
	gchar *value;
	gchar *name;
	AnjutaProjectProperty *prop;
	AnjutaToken *list;
		
	/* Create property */
	list = anjuta_token_last_item (variable);
	name = anjuta_token_evaluate (anjuta_token_first_word (variable));
	value = anjuta_token_evaluate (list);

	prop = amp_property_new (name, anjuta_token_get_type (variable), 0, value, list);

	amp_node_property_add (parent, prop);
	g_free (value);
	g_free (name);

	return NULL;
}

static gboolean
find_group (AnjutaProjectNode *node, gpointer data)
{
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_GROUP)
	{
		if (g_file_equal (anjuta_project_node_get_file (node), (GFile *)data))
		{
			/* Find group, return node value in pointer */
			return TRUE;
		}
	}

	return FALSE;
}

static void
project_load_subdirs (AmpProject *project, AnjutaToken *list, AnjutaProjectNode *parent, gboolean dist_only)
{
	AnjutaToken *arg;

	for (arg = anjuta_token_first_word (list); arg != NULL; arg = anjuta_token_next_word (arg))
	{
		gchar *value;
		
		value = anjuta_token_evaluate (arg);
		if (value == NULL) continue;		/* Empty value, a comment of a quote by example */

		/* Skip ., it is a special case, used to defined build order */
		if (strcmp (value, ".") != 0)
		{
			GFile *subdir;
			AmpGroupNode *group;

			subdir = g_file_resolve_relative_path (anjuta_project_node_get_file (parent), value);
			
			/* Look for already existing group */
			group = AMP_GROUP_NODE (anjuta_project_node_children_traverse (parent, find_group, subdir));

			if (group != NULL)
			{
				/* Already existing group, mark for built if needed */
				if (!dist_only) amp_group_node_set_dist_only (group, FALSE);
			}
			else
			{
				/* Create new group */
				group = amp_group_node_new_valid (subdir, dist_only, NULL);
				
				/* Group can be NULL if the name is not valid */
				if (group != NULL)
				{
					g_hash_table_insert (project->groups, g_file_get_uri (subdir), group);
					anjuta_project_node_append (parent, ANJUTA_PROJECT_NODE (group));

					amp_node_load (AMP_NODE (group), NULL, project, NULL);
				}
			}
			if (group) amp_group_node_add_token (group, arg, dist_only ? AM_GROUP_TOKEN_DIST_SUBDIRS : AM_GROUP_TOKEN_SUBDIRS);
			g_object_unref (subdir);
		}
		g_free (value);
	}
}

void
amp_project_set_am_variable (AmpProject* project, AmpGroupNode* group, AnjutaToken *variable, GHashTable *orphan_properties)
{
	AnjutaToken *list;
	
	switch (anjuta_token_get_type (variable))
	{
	case AM_TOKEN_SUBDIRS:
		list = anjuta_token_last_item (variable);
		project_load_subdirs (project, list, ANJUTA_PROJECT_NODE (group), FALSE);
		break;
	case AM_TOKEN_DIST_SUBDIRS:
		list = anjuta_token_last_item (variable);
		project_load_subdirs (project, list, ANJUTA_PROJECT_NODE (group), TRUE);
		break;
	case AM_TOKEN__DATA:
	case AM_TOKEN__HEADERS:
	case AM_TOKEN__LISP:
	case AM_TOKEN__MANS:
	case AM_TOKEN__PYTHON:
	case AM_TOKEN__JAVA:
	case AM_TOKEN__TEXINFOS:
		project_load_data (project, ANJUTA_PROJECT_NODE (group), variable, orphan_properties, 0);
		break;
	case AM_TOKEN__SCRIPTS:
		project_load_data (project, ANJUTA_PROJECT_NODE (group), variable, orphan_properties, ANJUTA_PROJECT_EXECUTABLE);
		break;
	case AM_TOKEN__LIBRARIES:
	case AM_TOKEN__LTLIBRARIES:
	case AM_TOKEN__PROGRAMS:
		project_load_target (project, ANJUTA_PROJECT_NODE (group), variable, orphan_properties);
		break;
	case AM_TOKEN__SOURCES:
		project_load_sources (project, ANJUTA_PROJECT_NODE (group), variable, orphan_properties);
		break;
	case AM_TOKEN_DIR:
	case AM_TOKEN__LDFLAGS:
	case AM_TOKEN__CPPFLAGS:
	case AM_TOKEN__CFLAGS:
	case AM_TOKEN__CXXFLAGS:
	case AM_TOKEN__JAVACFLAGS:
	case AM_TOKEN__VALAFLAGS:
	case AM_TOKEN__FCFLAGS:
	case AM_TOKEN__OBJCFLAGS:
	case AM_TOKEN__LFLAGS:
	case AM_TOKEN__YFLAGS:
		project_load_group_properties (project, ANJUTA_PROJECT_NODE (group), variable);
		break;
	case AM_TOKEN_TARGET_LDFLAGS:
	case AM_TOKEN_TARGET_CPPFLAGS:
	case AM_TOKEN_TARGET_CFLAGS:
	case AM_TOKEN_TARGET_CXXFLAGS:
	case AM_TOKEN_TARGET_JAVACFLAGS:
	case AM_TOKEN_TARGET_VALAFLAGS:
	case AM_TOKEN_TARGET_FCFLAGS:
	case AM_TOKEN_TARGET_OBJCFLAGS:
	case AM_TOKEN_TARGET_LFLAGS:
	case AM_TOKEN_TARGET_YFLAGS:
	case AM_TOKEN_TARGET_DEPENDENCIES:
	case AM_TOKEN_TARGET_LIBADD:
	case AM_TOKEN_TARGET_LDADD:
		project_load_target_properties (project, ANJUTA_PROJECT_NODE (group), variable, orphan_properties);
		break;
	default:
		break;
	}

	/* Keep the autotools variable as a normal variable too */
    amp_group_node_update_variable (group, variable);
}

/* Map function
 *---------------------------------------------------------------------------*/

static gint
amp_project_compare_node (AnjutaProjectNode *old_node, AnjutaProjectNode *new_node)
{
	const gchar *name1;
	const gchar *name2;
	GFile *file1;
	GFile *file2;

	name1 = anjuta_project_node_get_name (old_node);
	name2 = anjuta_project_node_get_name (new_node);
	file1 = anjuta_project_node_get_file (old_node);
	file2 = anjuta_project_node_get_file (new_node);

	return (anjuta_project_node_get_full_type (old_node) == anjuta_project_node_get_full_type (new_node))
		&& ((name1 == NULL) || (name2 == NULL) || (strcmp (name1, name2) == 0))
		&& ((file1 == NULL) || (file2 == NULL) || g_file_equal (file1, file2)) ? 0 : 1;
}

static void
amp_project_map_children (GHashTable *map, AnjutaProjectNode *old_node, AnjutaProjectNode *new_node)
{
	GList *children = NULL;

	if (new_node != NULL)
	{
		for (new_node = anjuta_project_node_first_child (new_node); new_node != NULL; new_node = anjuta_project_node_next_sibling (new_node))
		{
			children = g_list_prepend (children, new_node);
		}
		children = g_list_reverse (children);
	}

	for (old_node = anjuta_project_node_first_child (old_node); old_node != NULL; old_node = anjuta_project_node_next_sibling (old_node))
	{
		GList *same;

		same = g_list_find_custom (children, old_node, (GCompareFunc)amp_project_compare_node);

		if (same != NULL)
		{
			/* Add new to old node mapping */
			g_hash_table_insert (map, (AnjutaProjectNode *)same->data, old_node);

			amp_project_map_children (map, old_node, (AnjutaProjectNode *)same->data);
			children = g_list_delete_link (children, same);
		}
		else
		{
			/* Keep old_node to be deleted in the hash table as a key with a NULL
			 * value */
			g_hash_table_insert (map, old_node, NULL);
		}
	}

	/* Add remaining node in hash table */
	for (; children != NULL; children = g_list_delete_link (children,  children))
	{
		/* Keep new node without a corresponding old node as a key and a identical
		 * value */
		g_hash_table_insert (map, children->data, children->data);
	}
}

/* Find the correspondance between the new loaded node (new_node) and the
 * original node currently in the tree (old_node) */
static GHashTable *
amp_project_map_node (AnjutaProjectNode *old_node, AnjutaProjectNode *new_node)
{
	GHashTable *map;
	
	map = g_hash_table_new (g_direct_hash, NULL);

	g_hash_table_insert (map, new_node, old_node);

	amp_project_map_children (map, old_node, new_node);

	return map;
}


static void
amp_project_update_node (AnjutaProjectNode *key, AnjutaProjectNode *value, GHashTable *map)
{
	AnjutaProjectNode *old_node = NULL;	 /* The node that will be deleted */
	
	if (value == NULL)
	{
		/* if value is NULL, delete the old node which is the key */
		old_node = key;
	}
	else
	{
		AnjutaProjectNode *node = value;	/* The node that we keep in the tree */
		AnjutaProjectNode *new_node = key;  /* The node with the new data */
		
		if (new_node && new_node != node)
		{
			GList *properties;

			amp_node_update (AMP_NODE (node), AMP_NODE (new_node));

			/* Swap custom properties */
			properties = node->custom_properties;
			node->custom_properties = new_node->custom_properties;
			new_node->custom_properties = properties;

			if (new_node->parent == NULL)
			{
				/* This is the root node, update only the children */
				node->children = new_node->children;
			}
			else
			{
				/* Other node update all pointers */
				node->parent = new_node->parent;
				node->children = new_node->children;
				node->next = new_node->next;
				node->prev = new_node->prev;
			}

			/* Destroy node with data */
			old_node = new_node;
		}

		/* Update links, using original node address if they stay in the tree */
		new_node = g_hash_table_lookup (map, node->parent);
		if (new_node != NULL) node->parent = new_node;
		new_node = g_hash_table_lookup (map, node->children);
		if (new_node != NULL) node->children = new_node;
		new_node = g_hash_table_lookup (map, node->next);
		if (new_node != NULL) node->next = new_node;
		new_node = g_hash_table_lookup (map, node->prev);
		if (new_node != NULL) node->prev = new_node;
	}

		
	/* Unlink old node and free it */
	if (old_node != NULL)
	{
		old_node->parent = NULL;
		old_node->children = NULL;
		old_node->next = NULL;
		old_node->prev = NULL;
		g_object_unref (old_node);
	}
}

static AnjutaProjectNode *
amp_project_duplicate_node (AnjutaProjectNode *old_node)
{
	AnjutaProjectNode *new_node;

	/* Create new node */
	new_node = g_object_new (G_TYPE_FROM_INSTANCE (old_node), NULL);
	if (old_node->file != NULL) new_node->file = g_file_dup (old_node->file);
	if (old_node->name != NULL) new_node->name = g_strdup (old_node->name);
	if (anjuta_project_node_get_node_type (old_node) == ANJUTA_PROJECT_TARGET)
	{
		amp_target_node_set_type (AMP_TARGET_NODE (new_node), anjuta_project_node_get_full_type (old_node));
	}
	if (anjuta_project_node_get_node_type (old_node) == ANJUTA_PROJECT_PACKAGE)
	{
		// FIXME: We should probably copy the version number too because it will not
		// be updated when the node is reloaded. So later when updating the old_node,
		// the value will be overwritten with the new node empty value.
		amp_package_node_add_token (AMP_PACKAGE_NODE (new_node), amp_package_node_get_token (AMP_PACKAGE_NODE (old_node)));
	}
	if (anjuta_project_node_get_node_type (old_node) == ANJUTA_PROJECT_ROOT)
	{
		// FIXME: It would be better to write a duplicate function to avoid this code
		((AmpProject *)new_node)->lang_manager = (((AmpProject *)old_node)->lang_manager != NULL) ? g_object_ref (((AmpProject *)old_node)->lang_manager) : NULL;
	}
	/* Keep old parent, Needed for source node to find project root node */
	new_node->parent = old_node->parent;

	return new_node;
}

/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaProjectNodeInfo *
amp_project_get_type_info (AmpProject *project, AnjutaProjectNodeType type)
{
	AmpNodeInfo *info;
		
	for (info = AmpNodeInformations; info->base.type != type; info++)
	{
		if ((info->base.type == type) || (info->base.type == 0)) break;
	}

	return (AnjutaProjectNodeInfo *)info;
}

static gboolean
amp_project_load_root (AmpProject *project, GError **error) 
{
	AmpAcScanner *scanner;
	AnjutaToken *arg;
	AmpGroupNode *group;
	GFile *root_file;
	GFile *configure_file;
	AnjutaTokenFile *configure_token_file;
	GError *err = NULL;

	root_file = anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project));
	DEBUG_PRINT ("reload project %p root file %p", project, root_file);
	
	/* Unload current project */
	amp_project_unload (project);

	/* Initialize list styles */
	project->ac_space_list = anjuta_token_style_new (NULL, " ", "\n", NULL, 0);
	project->am_space_list = anjuta_token_style_new (NULL, " ", " \\\n\t", NULL, 0);
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

	/* Parse configure */
	configure_token_file = amp_project_set_configure (project, configure_file);
	amp_project_add_file (project, configure_file, configure_token_file);
	arg = anjuta_token_file_load (configure_token_file, NULL);
	scanner = amp_ac_scanner_new (project);
	project->configure_token = amp_ac_scanner_parse_token (scanner, arg, 0, &err);
	amp_ac_scanner_free (scanner);
	if (project->configure_token == NULL)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
						IANJUTA_PROJECT_ERROR_PROJECT_MALFORMED,
						err == NULL ? _("Unable to parse project file") : err->message);
		if (err != NULL) g_error_free (err);
			return FALSE;
	}

	/* Load all makefiles recursively */
	group = amp_group_node_new (root_file, FALSE);
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);
	anjuta_project_node_append (ANJUTA_PROJECT_NODE (project), ANJUTA_PROJECT_NODE (group));

	if (!amp_node_load (AMP_NODE (group), NULL, project, NULL))
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
					IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			_("Project doesn't exist or has an invalid path"));

		return FALSE;
	}

	return TRUE;
}

void
amp_project_unload (AmpProject *project)
{
	/* project data */
	amp_project_clear (project);
	
	/* shortcut hash tables */
	if (project->groups) g_hash_table_remove_all (project->groups);
	if (project->files != NULL)
	{
		GList *list;

		for (list = project->files; list != NULL; list = g_list_delete_link (list, list))
		{
			g_object_weak_unref (G_OBJECT (list->data), remove_config_file, project);
		}
		project->files = NULL;
	}
	if (project->configs) g_hash_table_remove_all (project->configs);

	/* List styles */
	if (project->am_space_list) anjuta_token_style_free (project->am_space_list);
	if (project->ac_space_list) anjuta_token_style_free (project->ac_space_list);
	if (project->arg_list) anjuta_token_style_free (project->arg_list);
}

gboolean
amp_project_is_loaded (AmpProject *project)
{
	return project->loading == 0;
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
	GList *list;

	for (list = project->files; list != NULL; list = g_list_next (list))
	{
		if (anjuta_token_file_get_token_location ((AnjutaTokenFile *)list->data, location, token))
		{
			return TRUE;
		}
	}

	return FALSE;
}

void 
amp_project_remove_group (AmpProject  *project,
		   AmpGroupNode *group,
		   GError     **error)
{
	GList *token_list;

	if (anjuta_project_node_get_node_type (ANJUTA_PROJECT_NODE (group)) != ANJUTA_PROJECT_GROUP) return;
	
	for (token_list = amp_group_node_get_token (group, AM_GROUP_TOKEN_CONFIGURE); token_list != NULL; token_list = g_list_next (token_list))
	{
		anjuta_token_remove_word ((AnjutaToken *)token_list->data);
	}
	for (token_list = amp_group_node_get_token (group, AM_GROUP_TOKEN_SUBDIRS); token_list != NULL; token_list = g_list_next (token_list))
	{
		anjuta_token_remove_word ((AnjutaToken *)token_list->data);
	}
	for (token_list = amp_group_node_get_token (group, AM_GROUP_TOKEN_DIST_SUBDIRS); token_list != NULL; token_list = g_list_next (token_list))
	{
		anjuta_token_remove_word ((AnjutaToken *)token_list->data);
	}

	amp_group_node_free (group);
}

void 
amp_project_remove_source (AmpProject  *project,
		    AmpSourceNode *source,
		    GError     **error)
{
	if (anjuta_project_node_get_node_type (ANJUTA_PROJECT_NODE (source)) != ANJUTA_PROJECT_SOURCE) return;
	
	anjuta_token_remove_word (amp_source_node_get_token (source));

	amp_source_node_free (source);
}

const GList *
amp_project_get_node_info (AmpProject *project, GError **error)
{
	static GList *info_list = NULL;

	if (info_list == NULL)
	{
		AmpNodeInfo *node;
		
		for (node = AmpNodeInformations; node->base.type != 0; node++)
		{
			info_list = g_list_prepend (info_list, node);
		}

		info_list = g_list_reverse (info_list);
	}
	
	return info_list;
}

/* Public functions
 *---------------------------------------------------------------------------*/

typedef struct _AmpMovePacket {
	AmpProject *project;
	GFile *old_root_file;
} AmpMovePacket;

static void
foreach_node_move (AnjutaProjectNode *g_node, gpointer data)
{
	AmpProject *project = ((AmpMovePacket *)data)->project;
	GFile *old_root_file = ((AmpMovePacket *)data)->old_root_file;
	GFile *root_file = anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project));
	gchar *relative;
	GFile *new_file;
	
	switch (anjuta_project_node_get_node_type (g_node))
	{
	case ANJUTA_PROJECT_GROUP:
		relative = get_relative_path (old_root_file, anjuta_project_node_get_file (g_node));
		new_file = g_file_resolve_relative_path (root_file, relative);
		g_free (relative);
		amp_group_node_set_file (AMP_GROUP_NODE (g_node), new_file);
		g_object_unref (new_file);	
			
		g_hash_table_insert (project->groups, g_file_get_uri (new_file), g_node);
		break;
	case ANJUTA_PROJECT_SOURCE:
		relative = get_relative_path (old_root_file, anjuta_project_node_get_file (g_node));
		new_file = g_file_resolve_relative_path (root_file, relative);
		g_free (relative);
		amp_source_node_set_file (AMP_SOURCE_NODE (g_node), new_file);
		g_object_unref (new_file);	
		break;
	default:
		break;
	}
}

gboolean
amp_project_move (AmpProject *project, const gchar *path)
{
	GFile *new_file;
	GFile *root_file;
	gchar *relative;
	GList *list;
	gpointer key;
	AmpConfigFile *cfg;
	GHashTable* old_hash;
	GHashTableIter iter;
	AmpMovePacket packet= {project, NULL};

	/* Change project root directory */
	packet.old_root_file = g_object_ref (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project)));
	root_file = g_file_new_for_path (path);
	amp_root_node_set_file (AMP_ROOT_NODE (project), root_file);
	
	/* Change project root directory in groups */
	old_hash = project->groups;
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	anjuta_project_node_foreach (ANJUTA_PROJECT_NODE (project), G_POST_ORDER, foreach_node_move, &packet);
	g_hash_table_destroy (old_hash);

	/* Change all files */
	for (list = project->files; list != NULL; list = g_list_next (list))
	{
		AnjutaTokenFile *tfile = (AnjutaTokenFile *)list->data;
		
		relative = get_relative_path (packet.old_root_file, anjuta_token_file_get_file (tfile));
		new_file = g_file_resolve_relative_path (root_file, relative);
		g_free (relative);
		anjuta_token_file_move (tfile, new_file);
	}

	/* Change all configs */
	old_hash = project->configs;
	project->configs = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, NULL, (GDestroyNotify)amp_config_file_free);
	g_hash_table_iter_init (&iter, old_hash);
	while (g_hash_table_iter_next (&iter, &key, (gpointer *)&cfg))
	{
		relative = get_relative_path (packet.old_root_file, cfg->file);
		new_file = g_file_resolve_relative_path (root_file, relative);
		g_free (relative);
		g_object_unref (cfg->file);
		cfg->file = new_file;
		
		g_hash_table_insert (project->configs, new_file, cfg);
	}
	g_hash_table_steal_all (old_hash);
	g_hash_table_destroy (old_hash);


	g_object_unref (root_file);
	g_object_unref (packet.old_root_file);

	return TRUE;
}

/* Dump file content of corresponding node */
gboolean
amp_project_dump (AmpProject *project, AnjutaProjectNode *node)
{
	gboolean ok = FALSE;
	
	switch (anjuta_project_node_get_node_type (node))
	{
	case ANJUTA_PROJECT_GROUP:
		anjuta_token_dump (amp_group_node_get_makefile_token (AMP_GROUP_NODE (node)));
		break;
	case ANJUTA_PROJECT_ROOT:
		anjuta_token_dump (AMP_PROJECT (node)->configure_token);
		break;
	default:
		break;
	}

	return ok;
}


AmpProject *
amp_project_new (GFile *file, IAnjutaLanguage *language, GError **error)
{
	AmpProject *project;
	GFile *new_file;
	
	project = AMP_PROJECT (g_object_new (AMP_TYPE_PROJECT, NULL));
	new_file = g_file_dup (file);
	amp_root_node_set_file (AMP_ROOT_NODE (project), new_file);
	g_object_unref (new_file);

	project->lang_manager = (language != NULL) ? g_object_ref (language) : NULL;
	
	return project;
}

/* Project access functions
 *---------------------------------------------------------------------------*/

AmpProject *
amp_project_get_root (AmpProject *project)
{
	return AMP_PROJECT (project);
}

AmpGroupNode *
amp_project_get_group (AmpProject *project, const gchar *id)
{
	return (AmpGroupNode *)g_hash_table_lookup (project->groups, id);
}

AmpTargetNode *
amp_project_get_target (AmpProject *project, const gchar *id)
{
	AmpTargetNode **buffer;
	AmpTargetNode *target;
	gsize dummy;

	buffer = (AmpTargetNode **)g_base64_decode (id, &dummy);
	target = *buffer;
	g_free (buffer);

	return target;
}

AmpSourceNode *
amp_project_get_source (AmpProject *project, const gchar *id)
{
	AmpSourceNode **buffer;
	AmpSourceNode *source;
	gsize dummy;

	buffer = (AmpSourceNode **)g_base64_decode (id, &dummy);
	source = *buffer;
	g_free (buffer);

	return source;
}

gchar *
amp_project_get_uri (AmpProject *project)
{
	g_return_val_if_fail (project != NULL, NULL);

	return g_file_get_uri (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project)));
}

GFile*
amp_project_get_file (AmpProject *project)
{
	g_return_val_if_fail (project != NULL, NULL);

	return anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project));
}

void
amp_project_add_file (AmpProject *project, GFile *file, AnjutaTokenFile* token)
{
	project->files = g_list_prepend (project->files, token);
	g_object_weak_ref (G_OBJECT (token), remove_config_file, project);
}

gboolean
amp_project_is_busy (AmpProject *project)
{
	if (project->queue == NULL) return FALSE;

	return pm_command_queue_is_busy (project->queue);
}

/* Worker thread
 *---------------------------------------------------------------------------*/

static gboolean
amp_load_setup (PmJob *job)
{
	//anjuta_project_node_check (job->node);
	pm_job_set_parent (job, anjuta_project_node_parent (job->node));
	job->proxy = amp_project_duplicate_node (job->node);

	return TRUE;
}

static gboolean
amp_load_work (PmJob *job)
{
	return amp_node_load (AMP_NODE (job->proxy), AMP_NODE (job->parent), AMP_PROJECT (job->user_data), &job->error);
}

static gboolean
amp_load_complete (PmJob *job)
{
	GHashTable *map;
	//static GTimer *timer = NULL;

	g_return_val_if_fail (job->proxy != NULL, FALSE);

	//anjuta_project_node_check (job->node);
	/*if (timer == NULL)
	{
		timer = g_timer_new ();		
	}
	else
	{
		g_timer_continue (timer);
	}*/
	map = amp_project_map_node (job->node, job->proxy);
	g_object_ref (job->proxy);
	g_hash_table_foreach (map, (GHFunc)amp_project_update_node, map);
	//anjuta_project_node_check (job->node);
	g_hash_table_destroy (map);
	g_object_unref (job->proxy);
	job->proxy = NULL;
	AMP_PROJECT (job->user_data)->loading--;
	g_signal_emit_by_name (AMP_PROJECT (job->user_data), "node-loaded", job->node,  job->error);
	//g_timer_stop (timer);
	//g_message ("amp_load_complete completed in %g", g_timer_elapsed (timer, NULL));

	return TRUE;
}

static PmCommandWork amp_load_job = {amp_load_setup, amp_load_work, amp_load_complete};

static gboolean
amp_save_setup (PmJob *job)
{
	return TRUE;
}

static gboolean
amp_save_work (PmJob *job)
{
	/* It is difficult to save only a particular node, so the whole project is saved */
	amp_node_save (AMP_NODE (job->user_data), NULL, AMP_PROJECT (job->user_data), &job->error);

	return TRUE;
}

static gboolean
amp_save_complete (PmJob *job)
{
	g_signal_emit_by_name (AMP_PROJECT (job->user_data), "node-saved", job->node,  job->error);

	return TRUE;
}

static PmCommandWork amp_save_job = {amp_save_setup, amp_save_work, amp_save_complete};

static gboolean
amp_add_before_setup (PmJob *job)
{
	anjuta_project_node_insert_before (job->parent, job->sibling, job->node);
	
	return TRUE;
}

static gboolean
amp_add_after_setup (PmJob *job)
{
	anjuta_project_node_insert_after (job->parent, job->sibling, job->node);
	
	return TRUE;
}

static gboolean
amp_add_work (PmJob *job)
{
	AmpNode *parent = AMP_NODE (job->parent);
	gboolean ok;

	ok = amp_node_write (AMP_NODE (job->node), parent, AMP_PROJECT (job->user_data), &job->error);
	
	return ok;
}

static gboolean
amp_add_complete (PmJob *job)
{
	g_signal_emit_by_name (AMP_PROJECT (job->user_data), "node-changed", job->parent,  job->error);

	return TRUE;
}

static PmCommandWork amp_add_before_job = {amp_add_before_setup, amp_add_work, amp_add_complete};
static PmCommandWork amp_add_after_job = {amp_add_after_setup, amp_add_work, amp_add_complete};


static gboolean
amp_remove_setup (PmJob *job)
{
	AnjutaProjectNode *parent;

	parent = anjuta_project_node_parent (job->node);
	if (parent == NULL) parent = job->node;
	pm_job_set_parent (job, parent);
	anjuta_project_node_set_state (job->node, ANJUTA_PROJECT_REMOVED);
	
	return TRUE;
}

static gboolean
amp_remove_work (PmJob *job)
{
	AmpNode *parent = AMP_NODE (job->parent);
	gboolean ok;

	ok = amp_node_erase (AMP_NODE (job->node), parent, AMP_PROJECT (job->user_data), &job->error);

	return ok;
}

static gboolean
amp_remove_complete (PmJob *job)
{
	g_signal_emit_by_name (AMP_PROJECT (job->user_data), "node-changed", job->parent,  job->error);

	return TRUE;
}

static PmCommandWork amp_remove_job = {amp_remove_setup, amp_remove_work, amp_remove_complete};

static gboolean
amp_set_property_setup (PmJob *job)
{
	return TRUE;
}

static gboolean
amp_set_property_work (PmJob *job)
{
	gint flags;
	
	flags = ((AmpProperty *)job->property->native)->flags;
	
	if (flags & AM_PROPERTY_IN_CONFIGURE)
	{
		amp_project_update_ac_property (AMP_PROJECT (job->user_data), job->property);
	}
	else if (flags & AM_PROPERTY_IN_MAKEFILE)
	{
		if (((AnjutaProjectProperty *)job->property->native)->flags & ANJUTA_PROJECT_PROPERTY_READ_WRITE)
		{
			amp_project_update_am_property (AMP_PROJECT (job->user_data), job->node, job->property);
		}
	}

	return TRUE;
}

static gboolean
amp_set_property_complete (PmJob *job)
{
	g_signal_emit_by_name (AMP_PROJECT (job->user_data), "node-changed", job->node,  job->error);

	return TRUE;
}

static PmCommandWork amp_set_property_job = {amp_set_property_setup, amp_set_property_work, amp_set_property_complete};

static gboolean
amp_remove_property_setup (PmJob *job)
{
	anjuta_project_node_set_state (job->node, ANJUTA_PROJECT_REMOVED);
	
	return TRUE;
}

static gboolean
amp_remove_property_work (PmJob *job)
{
	return FALSE;
}

static gboolean
amp_remove_property_complete (PmJob *job)
{
	g_signal_emit_by_name (AMP_PROJECT (job->user_data), "node-changed", job->node,  job->error);

	return TRUE;
}

static PmCommandWork amp_remove_property_job = {amp_remove_property_setup, amp_remove_property_work, amp_remove_property_complete};

/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static gboolean
iproject_load_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **error)
{
	PmJob *load_job;
	
	if (node == NULL) node = ANJUTA_PROJECT_NODE (obj);
	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();

	AMP_PROJECT (obj)->loading++;
	load_job = pm_job_new (&amp_load_job, node, NULL, NULL, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);

	pm_command_queue_push (AMP_PROJECT (obj)->queue, load_job);

	return TRUE;
}

static gboolean
iproject_save_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **error)
{
	PmJob *save_job;

	if (node == NULL) node = ANJUTA_PROJECT_NODE (obj);
	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();

	save_job = pm_job_new (&amp_save_job, node, NULL, NULL, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);
	pm_command_queue_push (AMP_PROJECT (obj)->queue, save_job);

	return TRUE;
}

static AnjutaProjectNode *
iproject_add_node_before (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **err)
{
	AnjutaProjectNode *node;
	PmJob *add_job;

	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();

	node = amp_node_new_valid (parent, type, file, name, err);
	if (node != NULL)
	{	
		add_job = pm_job_new (&amp_add_before_job, node, parent, sibling, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);
		pm_command_queue_push (AMP_PROJECT (obj)->queue, add_job);
	}
	
	return node;
}

static AnjutaProjectNode *
iproject_add_node_after (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **err)
{
	AnjutaProjectNode *node;
	PmJob *add_job;

	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();

	node = amp_node_new_valid (parent, type, file, name, err);
	if (node != NULL)
	{	
		add_job = pm_job_new (&amp_add_after_job, node, parent, sibling, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);
		pm_command_queue_push (AMP_PROJECT (obj)->queue, add_job);
	}
	
	return node;
}

static gboolean
iproject_remove_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	PmJob *remove_job;

	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();
	
	remove_job = pm_job_new (&amp_remove_job, node, NULL, NULL, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);
	pm_command_queue_push (AMP_PROJECT (obj)->queue, remove_job);

	return TRUE;
}

static AnjutaProjectProperty *
iproject_set_property (IAnjutaProject *obj, AnjutaProjectNode *node, AnjutaProjectProperty *property, const gchar *value, GError **error)
{
	AnjutaProjectProperty *new_prop;
	PmJob *set_property_job;

	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();
	
	new_prop = amp_node_property_set (node, property, value);
	set_property_job = pm_job_new (&amp_set_property_job, node, NULL, NULL, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);
	set_property_job->property = new_prop;
	pm_command_queue_push (AMP_PROJECT (obj)->queue, set_property_job);
	
	return new_prop;
}

static gboolean
iproject_remove_property (IAnjutaProject *obj, AnjutaProjectNode *node, AnjutaProjectProperty *property, GError **error)
{
	PmJob *remove_property_job;

	if (AMP_PROJECT (obj)->queue == NULL) AMP_PROJECT (obj)->queue = pm_command_queue_new ();
	
	remove_property_job = pm_job_new (&amp_remove_property_job, node, NULL, NULL, ANJUTA_PROJECT_UNKNOWN, NULL, NULL, obj);
	remove_property_job->property = property;
	pm_command_queue_push (AMP_PROJECT (obj)->queue, remove_property_job);
	
	return TRUE;
}

static AnjutaProjectNode *
iproject_get_root (IAnjutaProject *obj, GError **err)
{
	return ANJUTA_PROJECT_NODE (obj);
}

static const GList* 
iproject_get_node_info (IAnjutaProject *obj, GError **err)
{
	return amp_project_get_node_info (AMP_PROJECT (obj), err);
}

static gboolean
iproject_is_loaded (IAnjutaProject *obj, GError **err)
{
	return amp_project_is_loaded (AMP_PROJECT (obj));
}

static void
iproject_iface_init(IAnjutaProjectIface* iface)
{
	iface->load_node = iproject_load_node;
	iface->save_node = iproject_save_node;
	iface->add_node_before = iproject_add_node_before;
	iface->add_node_after = iproject_add_node_after;
	iface->remove_node = iproject_remove_node;
	iface->set_property = iproject_set_property;
	iface->remove_property = iproject_remove_property;
	iface->get_root = iproject_get_root;
	iface->get_node_info = iproject_get_node_info;
	iface->is_loaded = iproject_is_loaded;
}

/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_project_load (AmpNode *root, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_project_load_root (AMP_PROJECT (root), error);
}

static gboolean
amp_project_save (AmpNode *root, AmpNode *parent, AmpProject *project, GError **error)
{
	AnjutaTokenFile *tfile;
	AnjutaProjectNode *child;

	/* Save node */
	tfile = AMP_PROJECT (root)->configure_file;
	if (anjuta_token_file_is_dirty (tfile))
	{
		if (!anjuta_token_file_save (tfile, error)) return FALSE;
	}

	/* Save all children */
	for (child = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (root)); child != NULL; child = anjuta_project_node_next_sibling (child))
	{
		if (!amp_node_save (AMP_NODE (child), root, project, error)) return FALSE;
	}

	return TRUE;
}


static gboolean
amp_project_update (AmpNode *node, AmpNode *new_node)
{
	amp_project_update_root (AMP_PROJECT (node), AMP_PROJECT (new_node));

	return TRUE;
}


/* GObject implementation
 *---------------------------------------------------------------------------*/

static void
amp_project_dispose (GObject *object)
{
	AmpProject *project;

	g_return_if_fail (AMP_IS_PROJECT (object));

	project = AMP_PROJECT (object);
	amp_project_unload (project);

	amp_project_clear (project);

	if (project->groups) g_hash_table_destroy (project->groups);
	project->groups = NULL;
	if (project->configs) g_hash_table_destroy (project->configs);
	project->configs = NULL;
	
	if (project->queue) pm_command_queue_free (project->queue);
	project->queue = NULL;

	if (project->monitor) g_object_unref (project->monitor);
	project->monitor = NULL;

	if (project->lang_manager) g_object_unref (project->lang_manager);
	project->lang_manager = NULL;
	
	G_OBJECT_CLASS (parent_class)->dispose (object);	
}

static void
amp_project_init (AmpProject *project)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (AMP_IS_PROJECT (project));

	/* project data */
	project->configure_file = NULL;
	project->configure_token = NULL;
	
	project->ac_init = NULL;
	project->args = NULL;

	/* Hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->files = NULL;
	project->configs = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, NULL, (GDestroyNotify)amp_config_file_free);

	/* Default style */
	project->am_space_list = NULL;
	project->ac_space_list = NULL;
	project->arg_list = NULL;

	project->queue = NULL;
	project->loading = 0;
}

static void
amp_project_class_init (AmpProjectClass *klass)
{
	GObjectClass *object_class;
	AmpNodeClass *node_class;
	
	parent_class = g_type_class_peek_parent (klass);

	object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = amp_project_dispose;

	node_class = AMP_NODE_CLASS (klass);
	node_class->load = amp_project_load;
	node_class->save = amp_project_save;
	node_class->update = amp_project_update;
}

static void
amp_project_class_finalize (AmpProjectClass *klass)
{
}

G_DEFINE_DYNAMIC_TYPE_EXTENDED (AmpProject,
                                amp_project,
                                AMP_TYPE_ROOT_NODE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (IANJUTA_TYPE_PROJECT,
                                                               iproject_iface_init));

void
amp_project_register (GTypeModule *module)
{
	amp_node_register (module);
	amp_project_register_type (module);
}

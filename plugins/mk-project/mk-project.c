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
#include "mk-scanner.h"


#define UNIMPLEMENTED  G_STMT_START { g_warning (G_STRLOC": unimplemented"); } G_STMT_END

/* Constant strings for parsing perl script error output */
#define ERROR_PREFIX      "ERROR("
#define WARNING_PREFIX    "WARNING("
#define MESSAGE_DELIMITER ": "

static const gchar *valid_makefiles[] = {"GNUmakefile", "makefile", "Makefile", NULL};


/* Target types
 *---------------------------------------------------------------------------*/

static MkpNodeInfo MkpNodeInformation[] = {
	{{ANJUTA_PROJECT_GROUP,
	N_("Group"),
	""}},

	{{ANJUTA_PROJECT_SOURCE,
	N_("Source"),
	""}},

	{{ANJUTA_PROJECT_TARGET,
	N_("Unknown"),
	"text/plain"}},

	{{ANJUTA_PROJECT_UNKNOWN,
	NULL,
	NULL}}
};


/* ----- Standard GObject types and variables ----- */

enum {
	PROP_0,
	PROP_PROJECT_DIR
};

static GObject *parent_class;

/* Helper functions
 *---------------------------------------------------------------------------*/

#if 0
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
#endif

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

/* Group objects
 *---------------------------------------------------------------------------*/

static AnjutaProjectNode*
mkp_group_new (GFile *file)
{
	MkpGroup *group = g_object_new (MKP_TYPE_GROUP, NULL);;
	group->base.file = g_object_ref (file);

	group->base.type = ANJUTA_PROJECT_GROUP;
	group->base.native_properties = NULL;
	group->base.custom_properties = NULL;
	group->base.name = NULL;
	group->base.state = 0;


	return ANJUTA_PROJECT_NODE(group);
}

static void
mkp_group_class_init (MkpGroupClass *klass)
{

}

static void
mkp_group_init (MkpGroup *obj)
{

}

G_DEFINE_TYPE (MkpGroup, mkp_group, ANJUTA_TYPE_PROJECT_NODE);

/* Target objects
 *---------------------------------------------------------------------------*/

void
mkp_target_add_token (MkpTarget *target, AnjutaToken *token)
{
	target->tokens = g_list_prepend (target->tokens, token);
}


static GList *
mkp_target_get_token (MkpTarget *target)
{
	return target->tokens;
}

AnjutaProjectNode*
mkp_target_new (const gchar *name, AnjutaProjectNodeType type)
{
	MkpTarget *target = NULL;

	target = g_object_new (MKP_TYPE_TARGET, NULL);
	target->base.name = g_strdup (name);
	target->base.type = ANJUTA_PROJECT_TARGET;
	target->base.state = 0;

	return ANJUTA_PROJECT_NODE(target);
}

static void
mkp_target_class_init (MkpTargetClass *klass)
{

}

static void
mkp_target_init (MkpTarget *obj)
{

}

G_DEFINE_TYPE (MkpTarget, mkp_target, ANJUTA_TYPE_PROJECT_NODE);

/* Source objects
 *---------------------------------------------------------------------------*/

AnjutaProjectNode*
mkp_source_new (GFile *file)
{
	MkpSource *source = NULL;

	source = g_object_new (MKP_TYPE_SOURCE, NULL);
	source->base.file = g_object_ref (file);
	source->base.type = ANJUTA_PROJECT_SOURCE;
	source->base.native_properties = NULL;
	source->base.custom_properties = NULL;
	source->base.name = NULL;
	source->base.state = 0;

	return ANJUTA_PROJECT_NODE (source);
}

static void
mkp_source_class_init (MkpSourceClass *klass)
{

}

static void
mkp_source_init (MkpSource *obj)
{

}

G_DEFINE_TYPE (MkpSource, mkp_source, ANJUTA_TYPE_PROJECT_NODE);

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
		case G_FILE_MONITOR_EVENT_CREATED:
			g_signal_emit_by_name (G_OBJECT (project), "file-changed", data);
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
files_hash_foreach_monitor (gpointer key,
				gpointer value,
				gpointer user_data)
{
	GFile *makefile = (GFile *)key;
	MkpProject *project = user_data;

	monitor_add (project, makefile);
}

static void
monitors_setup (MkpProject *project)
{
	g_return_if_fail (project != NULL);

	monitors_remove (project);

	/* setup monitors hash */
	project->monitors = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
						   (GDestroyNotify) g_file_monitor_cancel);

	if (project->files)
		g_hash_table_foreach (project->files, files_hash_foreach_monitor, project);
}


/*
 * ---------------- Data structures managment
 */

static AnjutaProjectNode *
project_node_new (MkpProject *project, AnjutaProjectNode *parent, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **error)
{
	AnjutaProjectNode *node = NULL;

	switch (type & ANJUTA_PROJECT_TYPE_MASK) {
		case ANJUTA_PROJECT_ROOT:
		case ANJUTA_PROJECT_GROUP:
			node = ANJUTA_PROJECT_NODE (mkp_group_new (file));
			break;
		case ANJUTA_PROJECT_TARGET:
			node = ANJUTA_PROJECT_NODE (mkp_target_new (name, 0));
			break;
		case ANJUTA_PROJECT_SOURCE:
			node = ANJUTA_PROJECT_NODE (mkp_source_new (file));
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	if (node != NULL) node->type = type;

	return node;
}

static AnjutaProjectNode*
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
	tfile = anjuta_token_file_new (file);
	g_hash_table_insert (project->files, g_object_ref (file), g_object_ref (tfile));
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
	mkp_project_enumerate_targets (project, ANJUTA_PROJECT_NODE(parent));

	return ANJUTA_PROJECT_NODE(parent);
}

/* Project access functions
 *---------------------------------------------------------------------------*/

MkpProject *
mkp_project_get_root (MkpProject *project)
{
	return MKP_PROJECT(project);
}

static const GList *
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

	//fprintf(stdout, "update variable");
	//anjuta_token_dump (variable);

	arg = anjuta_token_first_item (variable);
	name = g_strstrip (anjuta_token_evaluate (arg));
	arg = anjuta_token_next_item (arg);

	//g_message ("new variable %s", name);
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

		//g_message ("assign %d name %s value %s\n", assign, name, anjuta_token_evaluate (value));
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

	//g_message ("update variable %s", name);

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
	GFile *make_file = NULL;
	const gchar **makefile;
	MkpGroup *group;

	/* Unload current project */
	root_file = g_object_ref (anjuta_project_node_get_file (node));
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

		return NULL;
	}

	/* Create group */
	group = MKP_GROUP(mkp_group_new (root_file));
	anjuta_project_node_append (node, ANJUTA_PROJECT_NODE(group));
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);


	/* Parse make file */
	project_load_makefile (project, make_file, group, error);
	g_object_unref (make_file);

	monitors_setup (project);

	return node;
}

AnjutaProjectNode*
mkp_project_load_node (MkpProject *project, AnjutaProjectNode *node, GError **error)
{
	switch (anjuta_project_node_get_node_type (node))
	{
	case ANJUTA_PROJECT_ROOT:
		project->loading++;
		DEBUG_PRINT("*** Loading project: %p root file: %p directory: \n", project, project->root_file, project->file);
		return mkp_project_load_root (project, node, error);
	case ANJUTA_PROJECT_GROUP:
		project->loading++;
		return project_load_makefile (project, node->file, MKP_GROUP(node), error);
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
	group = MKP_GROUP(mkp_group_new (root_file));
	anjuta_project_node_append (ANJUTA_PROJECT_NODE(project), ANJUTA_PROJECT_NODE(group));
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);

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

	return mkp_project_load_root (project, ANJUTA_PROJECT_NODE(project), error) != NULL;
}

void
mkp_project_unload (MkpProject *project)
{
	AnjutaProjectNode *node;
	
	monitors_remove (project);

	/* project data */
	if (project->root_file) g_object_unref (project->root_file);
	project->root_file = NULL;

	/* Remove all children */
	while ((node = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (project))) != NULL)
	{
		g_object_unref (node);
	}
	
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

		relative = get_relative_path (old_root_file, group->base.file);
		new_file = g_file_resolve_relative_path (project->root_file, relative);
		g_free (relative);
		g_object_unref (group->base.file);
		group->base.file = new_file;

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
mkp_project_new (GFile *file, GError  **error)
{
	MkpProject *project;

	project = MKP_PROJECT (g_object_new (MKP_TYPE_PROJECT, NULL));
	project->parent.file = (file != NULL) ? g_file_dup (file) : NULL;
	project->parent.type = ANJUTA_PROJECT_ROOT;

	return project;
}

static gboolean
mkp_project_is_loaded (MkpProject *project)
{
	return project->loading == 0;
}


/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static gboolean
iproject_load_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	if (node == NULL) node = ANJUTA_PROJECT_NODE (obj);


	if (mkp_project_load_node (MKP_PROJECT (obj), node, err) != NULL) {
		(MKP_PROJECT(obj))->loading--;
		g_signal_emit_by_name (MKP_PROJECT(obj), "node-loaded", node, err);

		return TRUE;
	}

	return  FALSE;
}

static gboolean
iproject_save_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	return mkp_project_save (MKP_PROJECT(node), err);
}

static AnjutaProjectProperty *
iproject_set_property (IAnjutaProject *obj, AnjutaProjectNode *node, AnjutaProjectProperty *property, const gchar *value, GError **error)
{
	g_set_error (error, IANJUTA_PROJECT_ERROR,
				IANJUTA_PROJECT_ERROR_NOT_SUPPORTED,
		_("Project doesn't allow to set properties"));

	return NULL;
}

static gboolean
iproject_remove_property (IAnjutaProject *obj, AnjutaProjectNode *node, AnjutaProjectProperty *property, GError **error)
{
	g_set_error (error, IANJUTA_PROJECT_ERROR,
				IANJUTA_PROJECT_ERROR_NOT_SUPPORTED,
		_("Project doesn't allow to set properties"));

	return FALSE;
}

static const GList*
iproject_get_node_info (IAnjutaProject *obj, GError **err)
{
	return mkp_project_get_node_info (MKP_PROJECT (obj), err);
}

static AnjutaProjectNode *
iproject_get_root (IAnjutaProject *obj, GError **err)
{
	return ANJUTA_PROJECT_NODE(mkp_project_get_root (MKP_PROJECT(obj)));
}

static gboolean
iproject_is_loaded (IAnjutaProject *obj, GError **err)
{
	return mkp_project_is_loaded (MKP_PROJECT (obj));
}

static gboolean
iproject_remove_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	anjuta_project_node_set_state (node, ANJUTA_PROJECT_REMOVED);
	g_signal_emit_by_name (obj, "node-modified", node,  NULL);

	return TRUE;
}

static AnjutaProjectNode *
iproject_add_node_before (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;

	node = project_node_new (MKP_PROJECT (obj), parent, type, file, name, error);
	anjuta_project_node_set_state (node, ANJUTA_PROJECT_MODIFIED);
	anjuta_project_node_insert_before (parent, sibling, node);

	g_signal_emit_by_name (obj, "node-changed", node,  NULL);

	return node;
}

static AnjutaProjectNode *
iproject_add_node_after (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;

	node = project_node_new (MKP_PROJECT (obj), parent, type, file, name, error);
	anjuta_project_node_set_state (node, ANJUTA_PROJECT_MODIFIED);
	anjuta_project_node_insert_after (parent, sibling, node);

	g_signal_emit_by_name (obj, "node-modified", node,  NULL);

	return node;
}

static void
iproject_iface_init(IAnjutaProjectIface* iface)
{
	iface->load_node = iproject_load_node;
	iface->save_node = iproject_save_node;
	iface->set_property = iproject_set_property;
	iface->remove_property = iproject_remove_property;
	iface->get_node_info = iproject_get_node_info;
	iface->get_root = iproject_get_root;

	iface->is_loaded = iproject_is_loaded;

	iface->remove_node = iproject_remove_node;

	iface->add_node_before = iproject_add_node_before;
	iface->add_node_after = iproject_add_node_after;

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


ANJUTA_TYPE_BEGIN(MkpProject, mkp_project, ANJUTA_TYPE_PROJECT_NODE);
ANJUTA_TYPE_ADD_INTERFACE(iproject, IANJUTA_TYPE_PROJECT);
ANJUTA_TYPE_END;

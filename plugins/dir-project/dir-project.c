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

#include "dir-project.h"

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

#define SOURCES_FILE	PACKAGE_DATA_DIR"/sources.list"

struct _DirProject {
	GObject         parent;

	GFile			*root_file;

	AnjutaProjectGroup        *root_node;
	
	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	
	/* project files monitors */
	GHashTable      *monitors;

	/* List of source files pattern */
	GList	*sources;
};

/* convenient shortcut macro the get the AnjutaProjectNode from a GNode */
#define DIR_NODE_DATA(node)  ((node) != NULL ? (AnjutaProjectNodeData *)((node)->data) : NULL)
#define DIR_GROUP_DATA(node)  ((node) != NULL ? (DirGroupData *)((node)->data) : NULL)
#define DIR_TARGET_DATA(node)  ((node) != NULL ? (DirTargetData *)((node)->data) : NULL)
#define DIR_SOURCE_DATA(node)  ((node) != NULL ? (DirSourceData *)((node)->data) : NULL)


typedef struct _DirGroupData DirGroupData;

struct _DirGroupData {
	AnjutaProjectGroupData base;
};

typedef struct _DirTargetData DirTargetData;

struct _DirTargetData {
	AnjutaProjectTargetData base;
};

typedef struct _DirSourceData DirSourceData;

struct _DirSourceData {
	AnjutaProjectSourceData base;
};

/* A file or directory name part of a path */
typedef struct _DirMatchString DirMatchString;

struct _DirMatchString
{
	gchar *string;
	gchar *reverse;
	guint length;
	GFile *file;
	gboolean parent;
};


/* A pattern used to match a part of a path */
typedef struct _DirPattern DirPattern;

struct _DirPattern
{
	GList *names;
	gboolean match;
	gboolean local;
	gboolean directory;
};

/* A list of pattern found in one file */
typedef struct _DirPatternList DirPatternList;

struct _DirPatternList
{
	GList *pattern;
	GFile *directory;
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
	DirProject *project = data;

	g_return_if_fail (project != NULL && DIR_IS_PROJECT (project));

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* monitor will be removed here... is this safe? */
			dir_project_reload (project, NULL);
			g_signal_emit_by_name (G_OBJECT (project), "project-updated");
			break;
		default:
			break;
	}
}


static void
monitor_add (DirProject *project, GFile *file)
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
monitors_remove (DirProject *project)
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
	DirGroup *group_node = value;
	DirProject *project = user_data;

	monitor_add (project, DIR_GROUP_DATA(group_node)->base.directory);
}

static void
monitors_setup (DirProject *project)
{
	g_return_if_fail (project != NULL);

	monitors_remove (project);
	
	/* setup monitors hash */
	project->monitors = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
						   (GDestroyNotify) g_file_monitor_cancel);

	monitor_add (project, project->root_file);
	
	if (project->groups)
		g_hash_table_foreach (project->groups, group_hash_foreach_monitor, project);
}

static DirGroup*
dir_group_new (GFile *file)
{
    DirGroupData *group = NULL;

	g_return_val_if_fail (file != NULL, NULL);
	
	group = g_slice_new0(DirGroupData); 
	group->base.node.type = ANJUTA_PROJECT_GROUP;
	group->base.directory = g_object_ref (file);

    return g_node_new (group);
}

static void
dir_group_free (DirGroup *node)
{
    DirGroupData *group = (DirGroupData *)node->data;
	
	if (group->base.directory) g_object_unref (group->base.directory);
    g_slice_free (DirGroupData, group);

	g_node_destroy (node);
}

/* Target objects
 *---------------------------------------------------------------------------*/

static DirTarget*
dir_target_new (const gchar *name, AnjutaProjectTargetType type)
{
    DirTargetData *target = NULL;

	target = g_slice_new0(DirTargetData); 
	target->base.node.type = ANJUTA_PROJECT_TARGET;
	target->base.name = g_strdup (name);
	target->base.type = type;

    return g_node_new (target);
}

static void
dir_target_free (DirTarget *node)
{
    DirTargetData *target = DIR_TARGET_DATA (node);
	
    g_free (target->base.name);
    g_slice_free (DirTargetData, target);

	g_node_destroy (node);
}

/* Source objects
 *---------------------------------------------------------------------------*/

static DirSource*
dir_source_new (GFile *file)
{
    DirSourceData *source = NULL;

	source = g_slice_new0(DirSourceData); 
	source->base.node.type = ANJUTA_PROJECT_SOURCE;
	source->base.file = g_object_ref (file);

    return g_node_new (source);
}

static void
dir_source_free (DirSource *node)
{
    DirSourceData *source = DIR_SOURCE_DATA (node);
	
    g_object_unref (source->base.file);
    g_slice_free (DirSourceData, source);

	g_node_destroy (node);
}


static void
foreach_node_destroy (AnjutaProjectNode    *g_node,
		      gpointer  data)
{
	switch (DIR_NODE_DATA (g_node)->type) {
		case ANJUTA_PROJECT_GROUP:
			dir_group_free (g_node);
			break;
		case ANJUTA_PROJECT_TARGET:
			dir_target_free (g_node);
			break;
		case ANJUTA_PROJECT_SOURCE:
			dir_source_free (g_node);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
}

static void
project_node_destroy (DirProject *project, AnjutaProjectNode *g_node)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (DIR_IS_PROJECT (project));
	
	if (g_node) {
		/* free each node's data first */
		anjuta_project_node_all_foreach (g_node,
				 foreach_node_destroy, project);
	}
}

/* File name objects
 *---------------------------------------------------------------------------*/

static DirMatchString*
dir_match_string_new (gchar *string)
{
	DirMatchString *str = NULL;

	str = g_slice_new0(DirMatchString);
	str->length = strlen (string);
	str->string = string;
	str->reverse = g_strreverse(g_strdup (string));

	return str;
}

static void
dir_match_string_free (DirMatchString *str)
{
	g_free (str->string);
	g_free (str->reverse);
	if (str->file) g_object_unref (str->file);
    g_slice_free (DirMatchString, str);
}

/* Cut a filename is part representing one directory or file name. By example
 * /home/user/project/foo will be generate a list containing:
 * foo, project, user, home */
static GList*
dir_cut_filename (GFile *file)
{
	GList *name_list = NULL;

	g_object_ref (file);
	do
	{
		DirMatchString *str;
		gchar *name = g_file_get_basename (file);

		if (strcmp(name, G_DIR_SEPARATOR_S) == 0)
		{
			g_free (name);
			g_object_unref (file);
			break;
		}
		
		str = dir_match_string_new (name);
		str->file = file;

		name_list = g_list_prepend (name_list, str);

		file = g_file_get_parent (file);
	}
	while (file != NULL);
	
	name_list = g_list_reverse (name_list);
	
	return name_list;
}

static void
dir_filename_free (GList *name)
{
	g_list_foreach (name, (GFunc)dir_match_string_free, NULL);
	g_list_free (name);
}

/* Pattern objects
 *---------------------------------------------------------------------------*/

/* Create a new pattern matching a directory of a file name in a path */
 
static DirPattern*
dir_pattern_new (const gchar *pattern, gboolean reverse)
{
    DirPattern *pat = NULL;
	GString *str = g_string_new (NULL);
	const char *ptr = pattern;

	pat = g_slice_new0(DirPattern);
	/* Check if it is a reverse pattern */
	if (*ptr == '!')
	{
		pat->match = reverse ? TRUE : FALSE;
		ptr++;
	}
	else
	{
		pat->match = reverse ? FALSE : TRUE;
	}
	/* Check if the pattern is local */
	if (*ptr == '/')
	{
		pat->local = TRUE;
		ptr++;
	}
	else
	{
		pat->local = FALSE;
	}
	pat->names = NULL;

	while (*ptr != '\0')
	{
		const gchar *next = strchr (ptr, '/');

		if (next == NULL)
		{
			pat->names = g_list_prepend (pat->names, g_pattern_spec_new (ptr));
			break;
		}
		else
		{
			if (next != ptr)
			{
				g_string_overwrite_len (str, 0, ptr, next - ptr);
				pat->names = g_list_prepend (pat->names, g_pattern_spec_new (str->str));
			}
			ptr = next + 1;
		}
	}
	g_string_free (str, TRUE);

	/* Check if the pattern has to match a directory */
	pat->directory = (ptr != pattern) && (*(ptr-1) == '/');

	return pat;
}

static void
dir_pattern_free (DirPattern *pat)
{
	g_list_foreach (pat->names, (GFunc)g_pattern_spec_free, NULL);
	g_list_free (pat->names);
	
    g_slice_free (DirPattern, pat);
}

/* Read a file containing pattern, the syntax is similar to .gitignore file.
 * 
 * It is not a regular expression, only * and ? are used as joker.
 * If the name end with / it will match only a directory.
 * If the name starts with / it must be relative to the project directory, so
 * by example /.git/ will match only a directory named .git in the project
 * directory, while CVS/ will match a directory named CVS anywhere in the
 * project.
 * If the name starts with ! the meaning is reversed. In a file containing
 * matching file, if a pattern starting ! matches, it means that the file has
 * to be removed from the matching list.
 * All pattern are read in order, so it is possible to match a group of files
 * and add pattern afterward to remove some of these files.
 * A name starting with # is a comment.
 * All spaces at the beginning of a name are ignored.
 */
static GList*
dir_push_pattern_list (GList *stack, GFile *dir, GFile *file, gboolean ignore, GError **error)
{
	char *content;
	char *ptr;
	DirPatternList *list = NULL;
	

	if (!g_file_load_contents (file, NULL, &content, NULL, NULL, error))
	{
		return stack;
	}

	list = g_slice_new0(DirPatternList);
	list->pattern = NULL;
	list->directory = dir;

	for (ptr = content; *ptr != '\0';)
	{
		gchar *next;
		
		next = strchr (ptr, '\n');
		if (next != NULL) *next = '\0';

		/* Discard space at the beginning */
		while (isspace (*ptr)) ptr++;

		if ((*ptr != '#') && (ptr != next))
		{
			/* Create pattern */
			DirPattern *pat = NULL;

			if (next != NULL) *next = '\0';
			pat = dir_pattern_new (ptr, ignore);
			list->pattern = g_list_prepend (list->pattern, pat);
		}

		if (next == NULL) break;
		ptr = next + 1;
	}
	g_free (content);

	list->pattern = g_list_reverse (list->pattern);
	
	return g_list_prepend (stack, list);
}

static GList *
dir_pop_pattern_list (GList *stack)
{
	DirPatternList *top = (DirPatternList *)stack->data;

	stack = g_list_remove_link (stack, stack);

	g_list_foreach (top->pattern, (GFunc)dir_pattern_free, NULL);
	g_list_free (top->pattern);
	g_object_unref (top->directory);
    g_slice_free (DirPatternList, top);

	return stack;
}

static gboolean
dir_pattern_stack_is_match (GList *stack, GFile *file)
{
	gboolean match;
	GList *list;
	GList *name_list;
	gboolean directory;

	/* Create name list from file */
	name_list = dir_cut_filename (file);
	
	directory = g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL) == G_FILE_TYPE_DIRECTORY;
	/* Include directories by default */
	match = directory;

	/* Check all valid patterns */
	for (list = g_list_last (stack); list != NULL; list = g_list_previous (list))
	{
		DirPatternList *pat_list = (DirPatternList *)list->data;
		GList *node;

		/* Mark parent level */
		for (node = g_list_first (name_list); node != NULL; node = g_list_next (node))
		{
			DirMatchString *str = (DirMatchString *)node->data;

			str->parent = g_file_equal (pat_list->directory, str->file);
		}

		for (node = g_list_first (pat_list->pattern); node != NULL; node = g_list_next (node))
		{
			DirPattern *pat = (DirPattern *)node->data;
			GList *pat_part;
			GList *name_part;
			gboolean match_part;

			if (pat->directory && !directory)
				continue;

			name_part = g_list_first (name_list);
			for (pat_part = g_list_first (pat->names); pat_part != NULL; pat_part = g_list_next (pat_part))
			{
				DirMatchString *part = (DirMatchString *)name_part->data;
				match_part = g_pattern_match ((GPatternSpec *)pat_part->data, part->length, part->string, part->reverse);

				if (!match_part) break;
				name_part = g_list_next (name_part);
			}

			/* Local match are relative to parent directory only */
			if (match_part && pat->local && (!((DirMatchString *)name_part->data)->parent)) match_part = FALSE;

			if (match_part)	match = pat->match;
		}
	}

	dir_filename_free (name_list);

	return match;
}

static gboolean
dir_project_list_directory (DirProject *project, DirGroup* parent, GError **error) 
{
	gboolean ok;
	GFileEnumerator *enumerator;

	enumerator = g_file_enumerate_children (DIR_GROUP_DATA (parent)->base.directory,
	    G_FILE_ATTRIBUTE_STANDARD_NAME,
	    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	    NULL,
	    error);

	ok = enumerator != NULL;
	if (ok)
	{
		GFileInfo *info;
		
		while ((info = g_file_enumerator_next_file (enumerator, NULL, error)) != NULL)
		{
			const gchar *name;
			GFile *file;

			name = g_file_info_get_name (info);
			file = g_file_get_child (DIR_GROUP_DATA (parent)->base.directory, name);
			g_object_unref (info);

			/* Check if file is a source */
			if (!dir_pattern_stack_is_match (project->sources, file)) continue;
			
			if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
			{
				/* Create a group for directory */
				DirGroup *group;
				
				group = dir_group_new (file);
				g_hash_table_insert (project->groups, g_file_get_uri (file), group);
				anjuta_project_node_append (parent, group);
				ok = dir_project_list_directory (project, group, error);
				if (!ok) break;
			}
			else
			{
				/* Create a source for files */
				DirSource *source;

				source = dir_source_new (file);
				anjuta_project_node_append (parent, source);
			}
		}
        g_file_enumerator_close (enumerator, NULL, NULL);
        g_object_unref (enumerator);
	}

	return ok;
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
dir_project_reload (DirProject *project, GError **error) 
{
	GFile *root_file;
	GFile *source_file;
	DirGroup *group;
	gboolean ok = TRUE;

	/* Unload current project */
	root_file = g_object_ref (project->root_file);
	dir_project_unload (project);
	project->root_file = root_file;
	DEBUG_PRINT ("reload project %p root file %p", project, project->root_file);

	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	if (g_file_query_file_type (root_file, G_FILE_QUERY_INFO_NONE, NULL) != G_FILE_TYPE_DIRECTORY)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));

		return FALSE;
	}

	group = dir_group_new (root_file);
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);
	project->root_node = group;

	/* Load source pattern */
	source_file = g_file_new_for_path (SOURCES_FILE);
	project->sources = dir_push_pattern_list (NULL, g_object_ref (root_file), source_file, FALSE, NULL);
	g_object_unref (source_file);
	
	dir_project_list_directory (project, group, NULL);
	
	monitors_setup (project);
	
	return ok;
}

gboolean
dir_project_load (DirProject  *project,
    GFile *directory,
	GError     **error)
{
	g_return_val_if_fail (directory != NULL, FALSE);

	project->root_file = g_object_ref (directory);
	if (!dir_project_reload (project, error))
	{
		g_object_unref (project->root_file);
		project->root_file = NULL;
	}

	return project->root_file != NULL;
}

void
dir_project_unload (DirProject *project)
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

	/* sources patterns */
	while (project->sources)
	{
		project->sources = dir_pop_pattern_list (project->sources);
	}
}

gint
dir_project_probe (GFile *file,
	    GError     **error)
{
	gint probe;

	probe = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY;
	if (!probe)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));
	}

	return probe ? IANJUTA_PROJECT_PROBE_FILES : 0;
}

static DirGroup* 
dir_project_add_group (DirProject  *project,
		DirGroup *parent,
		const gchar *name,
		GError     **error)
{
	return NULL;
}

static DirTarget*
dir_project_add_target (DirProject  *project,
		 DirGroup *parent,
		 const gchar *name,
		 AnjutaProjectTargetType type,
		 GError     **error)
{
	return NULL;
}

static DirSource* 
dir_project_add_source (DirProject  *project,
		 DirTarget *target,
		 GFile *file,
		 GError     **error)
{
	return NULL;
}

static GList *
dir_project_get_target_types (DirProject *project, GError **error)
{
	static AnjutaProjectTargetInfo unknown_type = {N_("Unknown"), ANJUTA_TARGET_UNKNOWN,"text/plain"};

	return g_list_prepend (NULL, &unknown_type);
}

static DirGroup *
dir_project_get_root (DirProject *project)
{
	return project->root_node;
}


/* Public functions
 *---------------------------------------------------------------------------*/

DirProject *
dir_project_new (void)
{
	return DIR_PROJECT (g_object_new (DIR_TYPE_PROJECT, NULL));
}


/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static AnjutaProjectGroup* 
iproject_add_group (IAnjutaProject *obj, AnjutaProjectGroup *parent,  const gchar *name, GError **err)
{
	return dir_project_add_group (DIR_PROJECT (obj), parent, name, err);
}

static AnjutaProjectSource* 
iproject_add_source (IAnjutaProject *obj, AnjutaProjectGroup *parent,  GFile *file, GError **err)
{
	return dir_project_add_source (DIR_PROJECT (obj), parent, file, err);
}

static AnjutaProjectTarget* 
iproject_add_target (IAnjutaProject *obj, AnjutaProjectGroup *parent,  const gchar *name,  AnjutaProjectTargetType type, GError **err)
{
	return dir_project_add_target (DIR_PROJECT (obj), parent, name, type, err);
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

static AnjutaProjectGroup* 
iproject_get_root (IAnjutaProject *obj, GError **err)
{
	return dir_project_get_root (DIR_PROJECT (obj));
}

static GList* 
iproject_get_target_types (IAnjutaProject *obj, GError **err)
{
	return dir_project_get_target_types (DIR_PROJECT (obj), err);
}

static gboolean
iproject_load (IAnjutaProject *obj, GFile *file, GError **err)
{
	return dir_project_load (DIR_PROJECT (obj), file, err);
}

static gboolean
iproject_refresh (IAnjutaProject *obj, GError **err)
{
	return dir_project_reload (DIR_PROJECT (obj), err);
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

/* GbfProject implementation
 *---------------------------------------------------------------------------*/

static void
dir_project_dispose (GObject *object)
{
	g_return_if_fail (DIR_IS_PROJECT (object));

	dir_project_unload (DIR_PROJECT (object));

	G_OBJECT_CLASS (parent_class)->dispose (object);	
}

static void
dir_project_instance_init (DirProject *project)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (DIR_IS_PROJECT (project));
	
	/* project data */
	project->root_file = NULL;
	project->root_node = NULL;

	project->monitors = NULL;
	project->groups = NULL;

	project->sources = NULL;
}

static void
dir_project_class_init (DirProjectClass *klass)
{
	GObjectClass *object_class;
	
	parent_class = g_type_class_peek_parent (klass);

	object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = dir_project_dispose;
}

ANJUTA_TYPE_BEGIN(DirProject, dir_project, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(iproject, IANJUTA_TYPE_PROJECT);
ANJUTA_TYPE_END;

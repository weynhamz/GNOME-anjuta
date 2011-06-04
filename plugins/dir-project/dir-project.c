/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* dir-project.c
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

#include "dir-node.h"

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

#define SOURCES_FILE	PACKAGE_DATA_DIR "/sources.list"

struct _DirProject {
	GObject         parent;

	AnjutaProjectNode *root;

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	
	/* project files monitors */
	GHashTable      *monitors;

	/* List of source files pattern */
	GList	*sources;
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
	gboolean match;
	gboolean directory;
	GRegex *source;
	gchar *object;
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
project_node_destroy (DirProject *project, AnjutaProjectNode *node)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (DIR_IS_PROJECT (project));

	if (node) {
		g_object_unref (node);
	}
}

static AnjutaProjectNode *
project_node_new (DirProject *project, AnjutaProjectNode *parent, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **error)
{
	AnjutaProjectNode *node = NULL;
	
	switch (type & ANJUTA_PROJECT_TYPE_MASK) {
		case ANJUTA_PROJECT_GROUP:
			if (file == NULL)
			{
				if (name == NULL)
				{
					g_set_error (error, IANJUTA_PROJECT_ERROR, 
							IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
							_("Missing name"));
				}
				else
				{
					GFile *group_file;
					
					group_file = g_file_get_child (anjuta_project_node_get_file (parent), name);
					node = dir_group_node_new (group_file, G_OBJECT (project));
					g_object_unref (group_file);
				}
			}
			else
			{
				node = dir_group_node_new (file, G_OBJECT (project));
			}
			break;
		case ANJUTA_PROJECT_OBJECT:
			if (file == NULL)
			{
				if (name == NULL)
				{
					g_set_error (error, IANJUTA_PROJECT_ERROR, 
							IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
							_("Missing name"));
				}
				else
				{
					GFile *object_file;
					
					object_file = g_file_get_child (anjuta_project_node_get_file (parent), name);
					node = dir_object_node_new (object_file);
					g_object_unref (object_file);
				}
			}
			else
			{
				node = dir_object_node_new (file);
			}
			break;
		case ANJUTA_PROJECT_SOURCE:
			if (file == NULL)
			{
				if (name == NULL)
				{
					g_set_error (error, IANJUTA_PROJECT_ERROR, 
							IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
							_("Missing name"));
				}
				else
				{
					GFile *source_file;
					
					source_file = g_file_get_child (anjuta_project_node_get_file (parent), name);
					node = dir_source_node_new (source_file);
					g_object_unref (source_file);
				}
			}
			else
			{
				node = dir_source_node_new (file);
			}
			break;
		case ANJUTA_PROJECT_ROOT:
			node = dir_root_node_new (file);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	if (node != NULL)
	{
		node->type = type;
		node->parent = parent;
	}
	
	return node;
}


/* Pattern objects
 *---------------------------------------------------------------------------*/

static void
dir_pattern_free (DirPattern *pat)
{
	if (pat->source != NULL) g_regex_unref (pat->source);
	g_free (pat->object);
	
    g_slice_free (DirPattern, pat);
}

/* Create a new pattern matching a directory of a file name in a path */
 
static DirPattern*
dir_pattern_new (const gchar *pattern, gboolean reverse)
{
    DirPattern *pat = NULL;
	GString *regex = g_string_new (NULL);
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
		g_string_append_c (regex, '^');
		ptr++;
	}
	else
	{
		g_string_append (regex, "(?:^|\\" G_DIR_SEPARATOR_S ")");
	}
	                 

	while (*ptr != '\0')
	{
		const gchar *next;

		next = ptr + strcspn (ptr, "\\:.^$[|()?*+{");

		g_string_append_len (regex, ptr, next - ptr);
		ptr = next;
		if (*ptr == ':')
		{
			/* Remaining data are for object */
			break;
		}
		else if (*ptr == '*')
		{
			/* Replace with corresponding regular expression */
			g_string_append (regex, "(.+)");
			ptr++;
		}
		else if (*ptr == '?')
		{
			/* Replace with corresponding regular expression */
			do
			{
				ptr++;
			} while  (*ptr == '?');
			g_string_append_printf (regex, "(.{%d})", ptr - next);
		}
		else if (*ptr == '\\')
		{
			/* Add next character without a special signification */
			g_string_append_c (regex, *ptr++);
			if (*ptr != '\0') g_string_append_c (regex, *ptr++);
		}
		else if (isspace (*ptr))
		{
			/* Skip space */
			continue;
		}
		else if (*ptr != '\0')
		{
			/* Automatically escape character if not done */
			if ((ptr == pattern) || (*(ptr - 1) != '\\'))
			{
				g_string_append_c (regex, '\\');
			}
			g_string_append_c (regex, *ptr++);
		}
	}
	if ((regex->len > 1) && (regex->str[regex->len - 1] == '/'))
	{
		/* Match directory only */
		pat->directory = TRUE;
		g_string_truncate (regex, regex->len - 1);
	}
	g_string_append_c (regex, '$');
	pat->source = g_regex_new (regex->str, 0, 0, NULL);
	if (pat->source == NULL)
	{
		dir_pattern_free (pat);
		pat = NULL;
	}

	if ((pat != NULL) && (*ptr == ':'))
	{
		g_string_truncate (regex, 0);

		ptr++;
		while (isspace (*ptr)) ptr++;

		while (*ptr != '\0')
		{
			const gchar *next;
			gint replace = 1;

			next = ptr + strcspn (ptr, "\\?*");

			g_string_append_len (regex, ptr, next - ptr);
			ptr = next;
			if (*ptr == '*')
			{
				/* Replace with corresponding replacement */
				g_string_append_printf (regex, "\\%d", replace++);
				ptr++;
			}
			else if (*ptr == '?')
			{
				do
				{
					ptr++;
				} while  (*ptr == '?');
				g_string_append_printf (regex, "\\%d", replace++);
			}
			else if (*ptr == '\\')
			{
				/* Add next character without a special signification */
				g_string_append_c (regex, *ptr++);
				if (*ptr != '\0') g_string_append_c (regex, *ptr++);
			}
		}
		pat->object = g_string_free (regex, FALSE);
	}
	else
	{
		g_string_free (regex, TRUE);
	}

	return pat;
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
 * If a name is followed by :  a following name is taken as the object file name
 */
static GList*
dir_push_pattern_list (GList *stack, GFile *dir, GFile *file, gboolean ignore, GError **error)
{
	char *content;
	char *ptr;
	DirPatternList *list = NULL;
	guint line;

	if (!g_file_load_contents (file, NULL, &content, NULL, NULL, error))
	{
		return stack;
	}

	list = g_slice_new0(DirPatternList);
	list->pattern = NULL;
	list->directory = dir;

	line = 1;
	for (ptr = content; *ptr != '\0';)
	{
		gchar *next;
		
		next = strchr (ptr, '\n');
		if (next != NULL) *next = '\0';
		line++;

		/* Discard space at the beginning */
		while (isspace (*ptr)) ptr++;

		if ((*ptr != '#') && (ptr != next))
		{
			/* Create pattern */
			DirPattern *pat = NULL;

			if (next != NULL) *next = '\0';
			pat = dir_pattern_new (ptr, ignore);
			if (pat != NULL)
			{
				list->pattern = g_list_prepend (list->pattern, pat);
			}
			else
			{
				gchar *filename = g_file_get_path (file);
				g_warning("Invalid pattern in %s line %d", filename, line);
				g_free (filename);
			}
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
dir_pattern_stack_is_match (GFile *root, GList *stack, GFile *file)
{
	gboolean match;
	GList *list;
	gchar *filename;
	gboolean directory;

	/* Create name from file */
	filename = g_file_get_relative_path (root, file);
	
	directory = g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL) == G_FILE_TYPE_DIRECTORY;
	/* Include directories by default */
	match = directory;

	/* Check all valid patterns */
	for (list = g_list_last (stack); list != NULL; list = g_list_previous (list))
	{
		DirPatternList *pat_list = (DirPatternList *)list->data;
		GList *node;
		
		for (node = g_list_first (pat_list->pattern); node != NULL; node = g_list_next (node))
		{
			DirPattern *pat = (DirPattern *)node->data;

			if (pat->directory && !directory)
				continue;

			if (g_regex_match (pat->source, filename,  0, NULL))
			{
				match = pat->match;
			}
		}
	}
	g_free (filename);

	return match;
}

static GFile *
dir_pattern_find_file_object (GFile *root, GList *stack, GFile *file)
{
	GFile *object = NULL;

	if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL) != G_FILE_TYPE_DIRECTORY)
	{
		GList *list;
		gchar *filename;
		
		/* Create name from file */
		filename = g_file_get_relative_path (root, file);
	
		/* Check all valid patterns */
		for (list = g_list_last (stack); list != NULL; list = g_list_previous (list))
		{
			DirPatternList *pat_list = (DirPatternList *)list->data;
			GList *node;
		
			for (node = g_list_first (pat_list->pattern); node != NULL; node = g_list_next (node))
			{
				DirPattern *pat = (DirPattern *)node->data;

				if (pat->directory  || !pat->match || (pat->object == NULL) )
					continue;

				if (g_regex_match (pat->source, filename,  0, NULL))
				{
					gchar *objname;
					
					objname = g_regex_replace (pat->source, filename, -1, 0, pat->object, 0, NULL);
					object = g_file_get_child (root, objname);
					g_free (objname);
				}
			}
		}
		g_free (filename);
	}

	return object;
}


typedef struct {
	DirProject *proj;
	AnjutaProjectNode *parent;
} DirData;

/* the number of files to enumerate each time */
#define NUM_FILES 10

static void
dir_project_load_directory_callback (GObject      *source_object,
                                     GAsyncResult *res,
                                     gpointer      user_data)
{
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source_object);
	GList *infos, *l;
	GError *err = NULL;
	DirData *data = (DirData *) user_data;
	GFile *root;

	infos = g_file_enumerator_next_files_finish (enumerator, res, &err);
	if (infos == NULL) {
		/* either we are finished or an error occured */
		anjuta_project_node_clear_state (data->parent, ANJUTA_PROJECT_LOADING);
		if (err != NULL) {
			g_signal_emit_by_name (data->proj, "node-loaded", data->parent, err);
			g_error_free (err);
		} else {
			AnjutaProjectNode *node, *remove;
			for (node = anjuta_project_node_first_child (data->parent);
			     node != NULL;
			     node = anjuta_project_node_next_sibling (node))
			{
				int state = anjuta_project_node_get_state (node);
				if (state & ANJUTA_PROJECT_LOADING)
				{
					/* these nodes are no longer necessary */
					gchar *uri = g_file_get_uri (node->file);
					remove = node;
					node = anjuta_project_node_prev_sibling (node);
					g_hash_table_remove (data->proj->groups, uri);
					g_free (uri);

					anjuta_project_node_remove (remove);
					g_object_unref (remove);
				}
			}
			if (anjuta_project_node_parent (data->parent) == data->proj->root)
			{
				/* Emit signal on root node instead of the first group */
				g_signal_emit_by_name (data->proj, "node-loaded", data->proj->root, NULL);
			}
			else
			{
				g_signal_emit_by_name (data->proj, "node-loaded", data->parent, NULL);
			}
		}
		g_object_unref (data->parent);
		g_slice_free (DirData, data);
		g_object_unref (enumerator);

		return;
	}

	root = anjuta_project_node_get_file (data->proj->root);
	for (l = infos; l != NULL; l = g_list_next(l))
	{
		GFileInfo *info;
		const gchar *name;
		GFile *file;

		info = G_FILE_INFO(l->data);

		name = g_file_info_get_name (info);
		file = g_file_get_child (data->parent->file, name);
		g_object_unref (info);

		/* Check if file is a source */
		if (!dir_pattern_stack_is_match (root, data->proj->sources, file)) continue;
		
		if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
		{
			AnjutaProjectNode *group;
			gchar *uri;

			uri = g_file_get_uri (file);
			group = g_hash_table_lookup (data->proj->groups, uri);
			if (group != NULL)
			{
				anjuta_project_node_clear_state (group, ANJUTA_PROJECT_LOADING);
				g_free (uri);
			}
			else
			{
				/* Create a group for directory */
				group = project_node_new (data->proj, NULL, ANJUTA_PROJECT_GROUP, file, NULL, NULL);
				g_hash_table_insert (data->proj->groups, uri, group);
				anjuta_project_node_append (data->parent, group);
				anjuta_project_node_set_state (group, ANJUTA_PROJECT_INCOMPLETE);
			}
		}
		else
		{
			AnjutaProjectNode *source = NULL;

			AnjutaProjectNode *node;
			for (node = anjuta_project_node_first_child (data->parent);
			     node != NULL;
			     node = anjuta_project_node_next_sibling (node))
			{
				if (g_file_equal (file, node->file))
				{
					source = node;
					anjuta_project_node_clear_state (source, ANJUTA_PROJECT_LOADING);
					break;
				}
			}

			if (source == NULL)
			{
				GFile *object;
				AnjutaProjectNode *parent;

				/* Create object if possible */
				object = dir_pattern_find_file_object (root, data->proj->sources, file);
				if (object != NULL)
				{
					parent = project_node_new (data->proj, NULL, ANJUTA_PROJECT_OBJECT | ANJUTA_PROJECT_PROJECT, object, NULL, NULL);
					g_object_unref (object);
					anjuta_project_node_append (data->parent, parent);
				}
				else
				{
					parent = data->parent;
				}
				
				/* Create a source for files */
				source = project_node_new (data->proj, NULL, ANJUTA_PROJECT_SOURCE | ANJUTA_PROJECT_PROJECT, file, NULL, NULL);
				anjuta_project_node_append (parent, source);
			}
		}
	}
	g_list_free (infos);

	g_file_enumerator_next_files_async (enumerator, NUM_FILES, G_PRIORITY_DEFAULT, NULL,
	                                    dir_project_load_directory_callback, data);
}

/* Public functions
 *---------------------------------------------------------------------------*/

static AnjutaProjectNode *
dir_project_load_directory (DirProject *project, AnjutaProjectNode *parent, GError **error)
{
	GFileEnumerator *enumerator;

	enumerator = g_file_enumerate_children (parent->file,
	    G_FILE_ATTRIBUTE_STANDARD_NAME,
	    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	    NULL,
	    error);

	if (enumerator == NULL)
		return parent;

	/* mark all children as loading so we can remove them if no longer relevant */
	AnjutaProjectNode *node;
	for (node = anjuta_project_node_first_child (parent);
	     node != NULL;
	     node = anjuta_project_node_next_sibling (node))
	{
		anjuta_project_node_set_state (node, ANJUTA_PROJECT_LOADING);
	}

	DirData *data = g_slice_new (DirData);
	data->proj = project;
	data->parent = g_object_ref (parent);

	g_file_enumerator_next_files_async (enumerator, NUM_FILES, G_PRIORITY_DEFAULT, NULL,
	                                    dir_project_load_directory_callback, data);

	anjuta_project_node_set_state (parent, ANJUTA_PROJECT_LOADING);
	return parent;
}

static AnjutaProjectNode *
dir_project_load_root (DirProject *project, GError **error) 
{
	GFile *source_file;
	GFile *root_file;
	AnjutaProjectNode *group;

	root_file = anjuta_project_node_get_file (project->root);
	DEBUG_PRINT ("reload project %p root file %p", project, root_file);

	group = anjuta_project_node_first_child (project->root);
	if (group != NULL)
	{
		dir_project_load_directory (project, group, NULL);
		return project->root;
	}

	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	if (g_file_query_file_type (root_file, G_FILE_QUERY_INFO_NONE, NULL) != G_FILE_TYPE_DIRECTORY)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
		             IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));

		return NULL;
	}

	group = project_node_new (project, NULL, ANJUTA_PROJECT_GROUP, root_file, NULL, NULL);
	anjuta_project_node_append (project->root, group);
	g_hash_table_insert (project->groups, g_file_get_uri (root_file), group);

	/* Load source pattern */
	source_file = g_file_new_for_path (SOURCES_FILE);
	project->sources = dir_push_pattern_list (NULL, g_object_ref (root_file), source_file, FALSE, NULL);
	g_object_unref (source_file);
	
	dir_project_load_directory (project, group, NULL);

	return project->root;
}

AnjutaProjectNode *
dir_project_load_node (DirProject *project, AnjutaProjectNode *node, GError **error) 
{
	if (node == NULL) node = project->root;
	switch (anjuta_project_node_get_node_type (node))
	{
	case ANJUTA_PROJECT_ROOT:
		return dir_project_load_root (project, error);
	case ANJUTA_PROJECT_GROUP:
		return dir_project_load_directory (project, node, error);
	default:
		return NULL;
	}
}

static void
foreach_node_save (AnjutaProjectNode *node,
					gpointer  data)
{
	gint state = anjuta_project_node_get_state (node);
	GError *err = NULL;
	gboolean ret;
	
	if (state & ANJUTA_PROJECT_MODIFIED)
	{
		switch (anjuta_project_node_get_node_type (node))
		{
		case ANJUTA_PROJECT_GROUP:
			g_file_make_directory_with_parents (anjuta_project_node_get_file (node), NULL, NULL);
			break;
		default:
			break;
		}
	}
	else if (state & ANJUTA_PROJECT_REMOVED)
	{
		switch (anjuta_project_node_get_node_type (node))
		{
		case ANJUTA_PROJECT_GROUP:
		case ANJUTA_PROJECT_SOURCE:
			ret = g_file_trash (anjuta_project_node_get_file (node), NULL, &err);
			if (err != NULL)
			{
				g_warning ("trashing file failed with %s", err->message);
				g_error_free (err);
			}
			break;
		default:
			break;
		}
	}
}

AnjutaProjectNode *
dir_project_save_node (DirProject *project, AnjutaProjectNode *node, GError **error)
{
	/* Save children */
	anjuta_project_node_foreach (node, G_POST_ORDER, foreach_node_save, project);
	
	return node;
}

void
dir_project_unload (DirProject *project)
{
	/* project data */
	/*project_node_destroy (project, project->root_node);
	project->root_node = NULL;*/

	if (project->root) project_node_destroy (project, project->root);
	project->root = NULL;

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

static GList *
dir_project_get_node_info (DirProject *project, GError **error)
{
	static AnjutaProjectNodeInfo node_info[] = {
					{ANJUTA_PROJECT_GROUP,
					N_("Group"),
					""},
					{ANJUTA_PROJECT_SOURCE,
					N_("Source"),
					""},
					{ANJUTA_PROJECT_UNKNOWN,
					NULL,
					NULL}};
	static GList *info_list = NULL;

	if (info_list == NULL)
	{
		AnjutaProjectNodeInfo *node;
		
		for (node = node_info; node->type != 0; node++)
		{
			info_list = g_list_prepend (info_list, node);
		}

		info_list = g_list_reverse (info_list);
	}
	
	return info_list;
}

static gboolean
find_not_loaded_node (gpointer key, gpointer value, gpointer user_data)
{
	AnjutaProjectNode *node = (AnjutaProjectNode *)value;
	gboolean found;
	
	found = anjuta_project_node_get_state (node) & (ANJUTA_PROJECT_LOADING | ANJUTA_PROJECT_INCOMPLETE);

	return found;
}
 
static gboolean
dir_project_is_loaded (DirProject *project)
{
	return g_hash_table_find (project->groups, find_not_loaded_node, NULL) == NULL;
}

/* Public functions
 *---------------------------------------------------------------------------*/

DirProject *
dir_project_new (GFile *directory, GError **error)
{
	DirProject *project;
	
	project = DIR_PROJECT (g_object_new (DIR_TYPE_PROJECT, NULL));
	project->root = dir_root_node_new (directory);

	return project;
}


/* Implement IAnjutaProject
 *---------------------------------------------------------------------------*/

static gboolean
iproject_load_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	node = dir_project_load_node (DIR_PROJECT (obj), node, err);

	return node != NULL;
}

static gboolean
iproject_save_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	node = dir_project_save_node (DIR_PROJECT (obj), node, err);
	g_signal_emit_by_name (obj, "node-saved", node,  err);

	return node != NULL;
}

static AnjutaProjectNode *
iproject_add_node_before (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;
	
	node = project_node_new (DIR_PROJECT (obj), parent, type, file, name, error);
	anjuta_project_node_set_state (node, ANJUTA_PROJECT_MODIFIED);
	anjuta_project_node_insert_before (parent, sibling, node);

	g_signal_emit_by_name (obj, "node-changed", node,  NULL);

	return node;
}

static AnjutaProjectNode *
iproject_add_node_after (IAnjutaProject *obj, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;
	
	node = project_node_new (DIR_PROJECT (obj), parent, type, file, name, error);
	anjuta_project_node_set_state (node, ANJUTA_PROJECT_MODIFIED);
	anjuta_project_node_insert_after (parent, sibling, node);

	g_signal_emit_by_name (obj, "node-modified", node,  NULL);

	return node;
}

static gboolean
iproject_remove_node (IAnjutaProject *obj, AnjutaProjectNode *node, GError **err)
{
	anjuta_project_node_set_state (node, ANJUTA_PROJECT_REMOVED);
	g_signal_emit_by_name (obj, "node-modified", node,  NULL);

	return TRUE;
}

static AnjutaProjectProperty*
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

static AnjutaProjectNode *
iproject_get_root (IAnjutaProject *obj, GError **error)
{
	return DIR_PROJECT (obj)->root;
}

static const GList* 
iproject_get_node_info (IAnjutaProject *obj, GError **err)
{
	return dir_project_get_node_info (DIR_PROJECT (obj), err);
}

static gboolean
iproject_is_loaded (IAnjutaProject *obj, GError **err)
{
	return dir_project_is_loaded (DIR_PROJECT (obj));
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
	project->root = NULL;
	//project->root_node = NULL;

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

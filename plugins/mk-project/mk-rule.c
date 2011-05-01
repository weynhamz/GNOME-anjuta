/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* mk-rule.c
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

#include "mk-rule.h"

#include "mk-project-private.h"
#include "mk-scanner.h"

#include <string.h>
#include <stdio.h>

/* Rule object
 *---------------------------------------------------------------------------*/

/* Maximum level of dependencies when searching for source files */
#define MAX_DEPENDENCIES	16

/* Rule object
 *---------------------------------------------------------------------------*/

static MkpRule*
mkp_rule_new (gchar *name, AnjutaToken *token)
{
    MkpRule *rule = NULL;

	g_return_val_if_fail (name != NULL, NULL);
	
	rule = g_slice_new0(MkpRule); 
	rule->name = g_strdup (name);
	rule->rule = token;

	return rule;
}

static void
mkp_rule_free (MkpRule *rule)
{
	g_free (rule->name);
	g_list_foreach (rule->prerequisite, (GFunc)g_free, NULL);
	g_list_free (rule->prerequisite);
	
    g_slice_free (MkpRule, rule);
}

/* Private functions
 *---------------------------------------------------------------------------*/

/* Find a source for target checking pattern rule. If no source is found,
 * return target, else free target and return a newly allocated source name */

static gchar *
mkp_project_find_source (MkpProject *project, gchar *target, AnjutaProjectNode *parent, guint backtrack)
{
	GFile *child;
	gboolean exist;
	GHashTableIter iter;
	gchar *key;
	MkpRule *rule;

	/* Check pattern rules */
	if (backtrack < MAX_DEPENDENCIES)
	{
		for (g_hash_table_iter_init (&iter, project->rules); g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&rule);)
		{
			if (rule->pattern)
			{
				gchar *source;
				
				if (rule->part == NULL)
				{	
					/* simple suffix rule */
					source = g_strconcat (target, rule->name, NULL);
				}
				else
				{
					/* double suffix rule */
					if (strcmp (target + strlen (target) - strlen (rule->part), rule->part) == 0)
					{
						gchar *suffix;

						source = g_strconcat (target, rule->name, NULL);
						suffix = source + strlen (target) - strlen (rule->part);

						memcpy (suffix, rule->name, rule->part - rule->name);
						*(suffix + (rule->part - rule->name)) = '\0';
					}
					else
					{
						continue;
					}
				}
					
				source = mkp_project_find_source (project, source, parent, backtrack + 1);

				if (source != NULL)
				{
					g_free (target);

					return source;
				}
			}
		}
	}
		
	child = g_file_get_child (anjuta_project_node_get_file (parent), target);
	exist = g_file_query_exists (child, NULL);
	//g_message ("target =%s= filename =%s=", target, g_file_get_parse_name (child));
	g_object_unref (child);

	if (!exist)
	{
		g_free (target);
		return NULL;
	}
	else
	{
		return target;
	}
}


/* Parser functions
 *---------------------------------------------------------------------------*/

void
mkp_project_add_rule (MkpProject *project, AnjutaToken *group)
{
	AnjutaToken *targ;
	AnjutaToken *dep;
	AnjutaToken *arg;
	gboolean double_colon = FALSE;

	//fprintf(stdout, "add rule\n");
	//anjuta_token_dump (group);
	
	targ = anjuta_token_first_item (group);
	arg = anjuta_token_next_word (targ);
	if (anjuta_token_get_type (arg) == MK_TOKEN_DOUBLE_COLON) double_colon = TRUE;
	dep = anjuta_token_next_word (arg);
	for (arg = anjuta_token_first_word (targ); arg != NULL; arg = anjuta_token_next_word (arg))
	{
		AnjutaToken *src = NULL;
		gchar *target = NULL;
		gboolean order = FALSE;
		gboolean no_token = TRUE;
		MkpRule *rule = NULL;

		switch (anjuta_token_get_type (arg))
		{
		case MK_TOKEN__PHONY:
			for (src = anjuta_token_first_word (dep); src != NULL; src = anjuta_token_next_word (src))
			{
				if (anjuta_token_get_type (src) != MK_TOKEN_ORDER)
				{
					target = anjuta_token_evaluate (src);
					
					rule = g_hash_table_lookup (project->rules, target);
					if (rule == NULL)
					{
						rule = mkp_rule_new (target, NULL);
						g_hash_table_insert (project->rules, rule->name, rule);
					}
					rule->phony = TRUE;
					
					//g_message ("    with target %s", target);
					if (target != NULL) g_free (target);
				}
			}
			break;
		case MK_TOKEN__SUFFIXES:
			for (src = anjuta_token_first_word (dep); src != NULL; src = anjuta_token_next_word (src))
			{
				if (anjuta_token_get_type (src) != MK_TOKEN_ORDER)
				{
					gchar *suffix;

					suffix = anjuta_token_evaluate (src);
					/* The pointer value must only be not NULL, it does not matter if it is
	 				 * invalid */
					g_hash_table_replace (project->suffix, suffix, suffix);
					//g_message ("    with suffix %s", suffix);
					no_token = FALSE;
				}
			}
			if (no_token == TRUE)
			{
				/* Clear all suffix */
				g_hash_table_remove_all (project->suffix);
			}
			break;
		case MK_TOKEN__DEFAULT:
		case MK_TOKEN__PRECIOUS:
		case MK_TOKEN__INTERMEDIATE:
		case MK_TOKEN__SECONDARY:
		case MK_TOKEN__SECONDEXPANSION:
		case MK_TOKEN__DELETE_ON_ERROR:
		case MK_TOKEN__IGNORE:
		case MK_TOKEN__LOW_RESOLUTION_TIME:
		case MK_TOKEN__SILENT:
		case MK_TOKEN__EXPORT_ALL_VARIABLES:
		case MK_TOKEN__NOTPARALLEL:
			/* Do nothing with these targets, just ignore them */
			break;
		default:
			target = g_strstrip (anjuta_token_evaluate (arg));
			if (*target == '\0') break;	
			//g_message ("add rule =%s=", target);
				
			rule = g_hash_table_lookup (project->rules, target);
			if (rule == NULL)
			{
				rule = mkp_rule_new (target, group);
				g_hash_table_insert (project->rules, rule->name, rule);
			}
			else
			{
				rule->rule = group;
			}
				
			for (src = anjuta_token_first_word (dep); src != NULL; src = anjuta_token_next_word (src))
			{
				gchar *src_name = anjuta_token_evaluate (src);

				if (src_name != NULL)
				{
					//g_message ("    with source %s", src_name);
					if (anjuta_token_get_type (src) == MK_TOKEN_ORDER)
					{
						order = TRUE;
					}
					rule->prerequisite = g_list_prepend (rule->prerequisite, src_name);
				}
			}

			if (target != NULL) g_free (target);
		}
	}
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
mkp_project_enumerate_targets (MkpProject *project, AnjutaProjectNode *parent)
{
	GHashTableIter iter;
	gpointer key;
	MkpRule *rule;

	/* Check pattern rules */
	for (g_hash_table_iter_init (&iter, project->rules); g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&rule);)
	{
		if (rule->phony) continue;

		if (g_hash_table_lookup (project->suffix, rule->name))
		{
			/* Single suffix rule */
			rule->pattern = TRUE;
			rule->part = NULL;
		}
		else
		{
			GString *pattern = g_string_sized_new (16);
			GList *suffix;
			GList *src;
			
			/* Check double suffix rule */
			suffix = g_hash_table_get_keys (project->suffix);
			
			for (src = g_list_first (suffix); src != NULL; src = g_list_next (src))
			{
				GList *obj;

				for (obj = g_list_first (suffix); obj != NULL; obj = g_list_next (obj))
				{
					g_string_assign (pattern, src->data);
					g_string_append (pattern, obj->data);

					if (strcmp (pattern->str, rule->name) == 0)
					{
						rule->pattern = TRUE;
						rule->part = rule->name + strlen (src->data);
						break;
					}
				}
				if (rule->pattern) break;
			}

			g_string_free (pattern, TRUE);
			g_list_free (suffix);
		}
	}

	/* Create new target */
	for (g_hash_table_iter_init (&iter, project->rules); g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&rule);)
	{
		MkpTarget *target;
		AnjutaToken *prerequisite;
		AnjutaToken *arg;

		//g_message ("rule =%s=", rule->name);
		if (rule->phony || rule->pattern) continue;
		
		/* Create target */
		target = MKP_TARGET(mkp_target_new (rule->name, ANJUTA_PROJECT_UNKNOWN));
		mkp_target_add_token (target, rule->rule);
		anjuta_project_node_append (parent, ANJUTA_PROJECT_NODE(target));

		/* Get prerequisite */
		prerequisite = anjuta_token_first_word (rule->rule);
		if (prerequisite != NULL) prerequisite = anjuta_token_next_word (prerequisite);
		if (prerequisite != NULL) prerequisite = anjuta_token_next_word (prerequisite);
		
		/* Add prerequisite */
		for (arg = anjuta_token_first_word (prerequisite); arg != NULL; arg = anjuta_token_next_word (arg))
		{
			MkpSource *source;
			GFile *src_file;
			gchar *name;

			name = anjuta_token_evaluate (arg);
			if (name != NULL)
			{
				name = g_strstrip (name);
				name = mkp_project_find_source (project, name, parent, 0);
			}

			if (name != NULL)
			{
				src_file = g_file_get_child (project->root_file, name);
				source = MKP_SOURCE(mkp_source_new (src_file));
				source->base.type = ANJUTA_PROJECT_SOURCE | ANJUTA_PROJECT_PROJECT;
				g_object_unref (src_file);
				anjuta_project_node_append (ANJUTA_PROJECT_NODE(target), ANJUTA_PROJECT_NODE(source));

				g_free (name);
			}
		}
		
	}
}

void 
mkp_project_init_rules (MkpProject *project)
{
	project->rules = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)mkp_rule_free);
	project->suffix = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

void 
mkp_project_free_rules (MkpProject *project)
{
	if (project->rules) g_hash_table_destroy (project->rules);
	project->rules = NULL;
	if (project->suffix) g_hash_table_destroy (project->suffix);
	project->suffix = NULL;
}


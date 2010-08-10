/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-group.h
    Copyright (C) Dragos Dena 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#include "snippets-group.h"
#include <gio/gio.h>


#define ANJUTA_SNIPPETS_GROUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_GROUP, AnjutaSnippetsGroupPrivate))

struct _AnjutaSnippetsGroupPrivate
{
	gchar* name;
	
	GList* snippets;
};


G_DEFINE_TYPE (AnjutaSnippetsGroup, snippets_group, G_TYPE_OBJECT);

static void
snippets_group_dispose (GObject* snippets_group)
{
	AnjutaSnippetsGroup *anjuta_snippets_group = ANJUTA_SNIPPETS_GROUP (snippets_group);
	AnjutaSnippetsGroupPrivate *priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);
	AnjutaSnippet *cur_snippet = NULL;
	GList *iter = NULL;

	/* Delete the name and description fields */
	g_free (priv->name);
	priv->name = NULL;
	
	/* Delete the snippets in the group */
	for (iter = g_list_first (priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		g_object_unref (cur_snippet);
	}
	g_list_free (anjuta_snippets_group->priv->snippets);
	
	G_OBJECT_CLASS (snippets_group_parent_class)->dispose (snippets_group);
}

static void
snippets_group_finalize (GObject* snippets_group)
{
	G_OBJECT_CLASS (snippets_group_parent_class)->finalize (snippets_group);
}

static void
snippets_group_class_init (AnjutaSnippetsGroupClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippets_group_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippets_group_dispose;
	object_class->finalize = snippets_group_finalize;

	g_type_class_add_private (klass, sizeof (AnjutaSnippetsGroupPrivate));
}

static void
snippets_group_init (AnjutaSnippetsGroup* snippets_group)
{
	AnjutaSnippetsGroupPrivate* priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);
	
	snippets_group->priv = priv;

	/* Initialize the private field */
	priv->name = NULL;
	priv->snippets = NULL;
}

/**
 * snippets_group_new:
 * @snippets_group_name: A name for the group. It's unique.
 *
 * Makes a new #AnjutaSnippetsGroup object.
 *
 * Returns: A new #AnjutaSnippetsGroup object or NULL on failure.
 **/
AnjutaSnippetsGroup* 
snippets_group_new (const gchar* snippets_group_name)
{
	AnjutaSnippetsGroup* snippets_group = NULL;
	AnjutaSnippetsGroupPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (snippets_group_name != NULL, NULL);
	
	/* Initialize the object */
	snippets_group = ANJUTA_SNIPPETS_GROUP (g_object_new (snippets_group_get_type (), NULL));
	priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);

	/* Copy the name, description and filename */
	priv->name = g_strdup (snippets_group_name);
	
	return snippets_group;
}


const gchar*
snippets_group_get_name (AnjutaSnippetsGroup* snippets_group)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), NULL);

	return snippets_group->priv->name;
}

void
snippets_group_set_name (AnjutaSnippetsGroup* snippets_group,
                         const gchar* new_group_name)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));

	g_free (snippets_group->priv->name);
	snippets_group->priv->name = g_strdup (new_group_name);
}

static gint
compare_snippets_by_name (gconstpointer a,
                          gconstpointer b)
{
	AnjutaSnippet *snippet_a = (AnjutaSnippet *)a,
	              *snippet_b = (AnjutaSnippet *)b;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet_a), 0);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet_b), 0);

	return g_utf8_collate (snippet_get_name (snippet_a),
	                       snippet_get_name (snippet_b));
}

/**
 * snippets_group_add_snippet:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 * @snippet: A snippet to be added.
 *
 * Adds a new #AnjutaSnippet to the snippet group, checking for conflicts.
 *
 * Returns: TRUE on success.
 **/
gboolean 
snippets_group_add_snippet (AnjutaSnippetsGroup* snippets_group,
                            AnjutaSnippet* snippet)
{
	AnjutaSnippetsGroupPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);


	/* Check if there is a snippet with the same key */
	if (snippets_group_has_snippet (snippets_group, snippet))
		return FALSE;
	
	/* Add the new snippet to the group */
	priv->snippets = g_list_insert_sorted (snippets_group->priv->snippets,
	                                       snippet,
	                                       compare_snippets_by_name);
	snippet->parent_snippets_group = G_OBJECT (snippets_group);

	return TRUE;
}

/**
 * snippets_group_remove_snippet:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 * @trigger_key: The trigger-key of the #AnjutaSnippet to be removed.
 * @language: The language of the #AnjutaSnippet to be removed.
 * @remove_all_languages_support: If it's FALSE it will remove just the support of the snippet for
 *                                the language. If it's TRUE it will actually remove the snippet.
 *
 * If remove_all_languages_support is TRUE, this will remove the snippet from the snippet-group,
 * if it's FALSE, it will just remove the language support for the given language.
 **/
void 
snippets_group_remove_snippet (AnjutaSnippetsGroup* snippets_group,
                               const gchar* trigger_key,
                               const gchar* language,
                               gboolean remove_all_languages_support)
{

	GList *iter = NULL;
	AnjutaSnippet *to_be_deleted_snippet = NULL, *cur_snippet;
	const gchar *cur_snippet_trigger = NULL;
	AnjutaSnippetsGroupPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));
	g_return_if_fail (trigger_key != NULL);
	priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);
	
	/* Check if there is a snippet with the same key */
	for (iter = g_list_first (priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = ANJUTA_SNIPPET (iter->data);
		if (!ANJUTA_IS_SNIPPET (cur_snippet))
			g_return_if_reached ();
		cur_snippet_trigger = snippet_get_trigger_key (cur_snippet);
		
		if (!g_strcmp0 (cur_snippet_trigger, trigger_key) &&
		    snippet_has_language (cur_snippet, language))
		{
			if (remove_all_languages_support)
			{
				to_be_deleted_snippet = cur_snippet;
				break;
			}
			else
			{
				snippet_remove_language (cur_snippet, language);
				return;
			}
		}
		
	}

	/* If we found a snippet that should be deleted we remove it from the list and unref it */
	if (to_be_deleted_snippet)
	{
		priv->snippets = g_list_remove (priv->snippets, to_be_deleted_snippet);
		g_object_unref (to_be_deleted_snippet);

	}
	
}

gboolean
snippets_group_has_snippet (AnjutaSnippetsGroup *snippets_group,
                            AnjutaSnippet *snippet)
{
	AnjutaSnippetsGroupPrivate *priv = NULL;
	GList *iter = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	priv = ANJUTA_SNIPPETS_GROUP_GET_PRIVATE (snippets_group);

	for (iter = g_list_first (priv->snippets); iter != NULL; iter = g_list_next (iter))
	{
		if (!ANJUTA_IS_SNIPPET (iter->data))
			continue;
		if (snippet_is_equal (snippet, ANJUTA_SNIPPET (iter->data)))
			return TRUE;
	}

	return FALSE;
}

/**
 * snippets_group_get_snippets_list:
 * @snippets_group: A #AnjutaSnippetsGroup object.
 *
 * Gets the snippets in this group.
 * Important: This returns the actual list as saved in the #AnjutaSnippetsGroup.
 *
 * Returns: A #GList with entries of type #AnjutaSnippet.
 **/
GList* 
snippets_group_get_snippets_list (AnjutaSnippetsGroup* snippets_group)
{
	return snippets_group->priv->snippets;
}

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-interaction-interpreter.c
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

#include "snippets-interaction-interpreter.h"
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <string.h>

#define IN_WORD(c)    (g_ascii_isalnum (c) || c == '_')

#define ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                                      ANJUTA_TYPE_SNIPPETS_INTERACTION,\
													  SnippetsInteractionPrivate))

typedef struct _SnippetEditingInfo SnippetEditingInfo;
typedef struct _SnippetVariableInfo SnippetVariableInfo;

struct _SnippetsInteractionPrivate
{
	AnjutaSnippet *cur_snippet;
	gboolean editing;
	SnippetEditingInfo *editing_info;
	
	IAnjutaEditor *cur_editor;
	gulong changed_handler_id;
	gulong cursor_moved_handler_id;

	gboolean selection_set_blocker;
	gboolean changing_values_blocker;
	IAnjutaIterable *cur_sel_start_iter;
	
	AnjutaShell *shell;
};

struct _SnippetVariableInfo
{
	gint cur_value_length;

	/* List of IAnjutaIterable objects */
	GList *appearances;

};

struct _SnippetEditingInfo
{
	IAnjutaIterable *snippet_start;
	IAnjutaIterable *snippet_end;
	IAnjutaIterable *snippet_finish_position;

	/* List of SnippetVariableInfo structures */
	GList *snippet_vars_info;
	GList *cur_var;

};

G_DEFINE_TYPE (SnippetsInteraction, snippets_interaction, G_TYPE_OBJECT);

static void
snippets_interaction_init (SnippetsInteraction *snippets_interaction)
{
	SnippetsInteractionPrivate* priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	/* Initialize the private field */
	priv->cur_snippet  = NULL;
	priv->editing      = FALSE;
	priv->editing_info = NULL;

	priv->cur_editor = NULL;

	priv->selection_set_blocker = FALSE;
	priv->changing_values_blocker = FALSE;
	priv->cur_sel_start_iter = NULL;

	priv->shell = NULL;
	
}

static void
snippets_interaction_class_init (SnippetsInteractionClass *snippets_interaction_class)
{
	snippets_interaction_parent_class = g_type_class_peek_parent (snippets_interaction_class);
	g_type_class_add_private (snippets_interaction_class, sizeof (SnippetsInteractionPrivate));
}

/* Private */

static gboolean  focus_on_next_snippet_variable (SnippetsInteraction *snippets_interaction);
static void      update_snippet_positions       (SnippetsInteraction *snippets_interaction,
                                                 IAnjutaIterable *start_position_iter,
                                                 gint modified_count);
static gchar     char_at_iterator               (IAnjutaEditor *editor,
                                                 IAnjutaIterable *iter);
static void      on_cur_editor_changed          (IAnjutaEditor *cur_editor,
                                                 GObject *position,
                                                 gboolean added,
                                                 gint length,
                                                 gint lines,
                                                 gchar *text,
                                                 gpointer user_data);
static void      on_cur_editor_cursor_moved     (IAnjutaEditor *cur_editor,
                                                 gpointer user_data);
static void      delete_snippet_editing_info    (SnippetsInteraction *snippets_interaction);
static void      start_snippet_editing_session  (SnippetsInteraction *snippets_interaction,
                                                 IAnjutaIterable *start_pos,
                                                 gint len);
static void      stop_snippet_editing_session   (SnippetsInteraction *snippets_interaction);


static gboolean
focus_on_next_snippet_variable (SnippetsInteraction *snippets_interaction)
{
	SnippetsInteractionPrivate *priv = NULL;
	SnippetVariableInfo *var_info = NULL;
	IAnjutaIterable *first_var_appearance = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction), FALSE);
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (priv->cur_editor), FALSE);

	/* If we aren't editing we should just return */
	if (!priv->editing)
		return FALSE;
	g_return_val_if_fail (priv->editing_info != NULL, FALSE);

	/* If the current variable doesn't point to anything we stop editing */
	if (priv->editing_info->cur_var == NULL)
	{
		if (IANJUTA_IS_ITERABLE (priv->editing_info->snippet_finish_position))
		{
			ianjuta_editor_goto_position (priv->cur_editor,
			                              priv->editing_info->snippet_finish_position,
			                              NULL);
		}
		stop_snippet_editing_session (snippets_interaction);

		return FALSE;
	}

	/* We set the cursor to the current variable (the selection will be done in the
	   "move-cursor" signal handler) ... */
	var_info = (SnippetVariableInfo *)priv->editing_info->cur_var->data;
	if (var_info->appearances)
	{
		GList *first_elem = g_list_first (var_info->appearances);
		first_var_appearance = IANJUTA_ITERABLE (first_elem->data);
		g_return_val_if_fail (IANJUTA_IS_ITERABLE (first_elem->data), FALSE);

		ianjuta_editor_goto_position (priv->cur_editor,
		                              first_var_appearance,
		                              NULL);
	}
	
	/* ... and move to the next variable */
	priv->editing_info->cur_var = g_list_next (priv->editing_info->cur_var);

	return TRUE;
}

static gboolean
update_editor_iter (IAnjutaIterable *iter,
                    IAnjutaIterable *start_position_iter,
                    gint modified_count,
                    SnippetsInteraction *snippets_interaction)
{
	gint iter_position = 0, start_position = 0;
	
	/* Assertions */
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (iter), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction), FALSE);

	/* We only return FALSE if a special case occured like deleting an iter */
	if (!modified_count)
		return TRUE;

	iter_position  = ianjuta_iterable_get_position (iter, NULL);
	start_position = ianjuta_iterable_get_position (start_position_iter, NULL);

	/* If the iter_position is less than the start_position, we don't modify
	   the iter. */
	if (iter_position <= start_position)
		return TRUE;

	/* If we deleted this iterator, we should stop the editing session */
	if (modified_count < 0 && 
	    start_position <= iter_position && 
	    start_position - modified_count >= iter_position)
	{
		return FALSE;
	}

	/* Update the iter position */
	ianjuta_iterable_set_position (iter, iter_position + modified_count, NULL);

	return TRUE;
}

static void
update_snippet_positions (SnippetsInteraction *snippets_interaction,
                          IAnjutaIterable *start_position_iter,
                          gint modified_count)
{
	GList *iter = NULL, *iter2 = NULL;
	SnippetsInteractionPrivate *priv = NULL;
	SnippetVariableInfo *cur_var_info = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_if_fail (priv->editing);
	g_return_if_fail (priv->editing_info != NULL);

	/* Update all the iter's if necessary */
	if (!update_editor_iter (priv->editing_info->snippet_start, 
	                         start_position_iter, 
	                         modified_count,
	                         snippets_interaction))
	{
		stop_snippet_editing_session (snippets_interaction);
		return;
	}
	
	if (!update_editor_iter (priv->editing_info->snippet_end, 
	                         start_position_iter, 
	                         modified_count,
	                         snippets_interaction))
	{
		stop_snippet_editing_session (snippets_interaction);
		return;
	}

	if (priv->editing_info->snippet_finish_position != NULL)
		if (!update_editor_iter (priv->editing_info->snippet_finish_position, 
			                     start_position_iter, 
			                     modified_count,
			                     snippets_interaction))
		{
			stop_snippet_editing_session (snippets_interaction);
			return;
		}

	for (iter = priv->editing_info->snippet_vars_info; iter != NULL; iter = g_list_next (iter))
	{
		cur_var_info = (SnippetVariableInfo *)iter->data;

		for (iter2 = cur_var_info->appearances; iter2 != NULL; iter2 = g_list_next (iter2))
		{
			if (!update_editor_iter (IANJUTA_ITERABLE (iter2->data), 
			                         start_position_iter, 
			                         modified_count,
			                         snippets_interaction))
			{
				stop_snippet_editing_session (snippets_interaction);
				return;
			}
		}

	}

}

static void
update_variables_values (SnippetsInteraction *snippets_interaction,
	                     IAnjutaIterable *iter,
	                     gint modified_value,
                         gchar *text)
{
	SnippetsInteractionPrivate *priv = NULL;
	GList *iter1 = NULL, *iter2 = NULL, *edited_app_node = NULL;
	SnippetVariableInfo *var_info = NULL;
	IAnjutaIterable *var_iter = NULL, *start_iter = NULL, *end_iter = NULL;
	gboolean found = FALSE;
	gint diff = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_if_fail (priv->editing);
	g_return_if_fail (priv->editing_info);
	if (priv->changing_values_blocker)
		return;
	priv->changing_values_blocker = TRUE;

	/* Search for any modified value of a variable */
	for (iter1 = priv->editing_info->snippet_vars_info; iter1 != NULL; iter1 = g_list_next (iter1))
	{
		found = FALSE;
		var_info = (SnippetVariableInfo *)iter1->data;

		for (iter2 = g_list_first (var_info->appearances); iter2 != NULL; iter2 = g_list_next (iter2))
		{
			var_iter = IANJUTA_ITERABLE (iter2->data);
			g_return_if_fail (IANJUTA_IS_ITERABLE (var_iter));

			/* We found a variable value modified */
			diff = ianjuta_iterable_diff (var_iter, iter, NULL);

			if ((diff == 0) ||
			    (diff > 0 && diff <= var_info->cur_value_length))
			{
				edited_app_node = iter2;
				found = TRUE;
				var_info->cur_value_length += modified_value;

				break;
			}
		}
		if (found) break;

	}

	if (found)
	{
		g_return_if_fail (edited_app_node != NULL);

		/* Modify the other appearances of the variables */
		for (iter1 = g_list_first (edited_app_node); iter1 != NULL; iter1 = g_list_next (iter1))
		{

			/* Skipping the already visited appeareance */
			if (iter1 == edited_app_node)
				continue;

			var_iter = IANJUTA_ITERABLE (iter1->data);
			g_return_if_fail (IANJUTA_IS_ITERABLE (var_iter));

			start_iter = ianjuta_iterable_clone (var_iter, NULL);
			ianjuta_iterable_set_position (start_iter, 
			                               ianjuta_iterable_get_position (var_iter, NULL) + diff,
			                               NULL);
			end_iter = ianjuta_iterable_clone (var_iter, NULL);
			ianjuta_iterable_set_position (end_iter,
			                               ianjuta_iterable_get_position (var_iter, NULL) + diff - modified_value,
			                               NULL);

			if (modified_value > 0)
				ianjuta_editor_insert (priv->cur_editor, start_iter, text, modified_value, NULL);
			else
				ianjuta_editor_erase (priv->cur_editor, start_iter, end_iter, NULL);

			g_object_unref (start_iter);
			g_object_unref (end_iter);
		}
	}
	
	priv->changing_values_blocker = FALSE;
}

static gchar
char_at_iterator (IAnjutaEditor *editor,
                  IAnjutaIterable *iter)
{
	IAnjutaIterable *next = NULL;
	gchar *text = NULL, returned_char = 0;
	
	/* Assertions */
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), 0);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (iter), 0);

	next = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_next (next, NULL);
	
	text = ianjuta_editor_get_text (editor, iter, next, NULL);
	if (text == NULL)
		return 0;

	returned_char = text[0];
	g_free (text);
	g_object_unref (next);

	return returned_char;	
}

static void
on_cur_editor_changed (IAnjutaEditor *cur_editor,
                       GObject *position,
                       gboolean added,
                       gint length,
                       gint lines,
                       gchar *text,
                       gpointer user_data)
{
	SnippetsInteractionPrivate *priv = NULL;
	gint sign = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (user_data));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (user_data);
	g_return_if_fail (IANJUTA_IS_ITERABLE (position));

	if (!priv->editing)
		return;

	sign = (added)? 1:-1;
	update_snippet_positions (ANJUTA_SNIPPETS_INTERACTION (user_data),
	                          IANJUTA_ITERABLE (position),
	                          sign * length);

	if (!priv->editing)
		return;
	
	update_variables_values (ANJUTA_SNIPPETS_INTERACTION (user_data),
	                         IANJUTA_ITERABLE (position),
	                         sign * length,
	                         text);

}

static void
on_cur_editor_cursor_moved (IAnjutaEditor *cur_editor,
                            gpointer user_data)
{
	SnippetsInteractionPrivate *priv = NULL;
	IAnjutaIterable *cur_pos = NULL, *var_iter = NULL;
	GList *iter = NULL, *iter2 = NULL;
	SnippetVariableInfo *cur_var_info = NULL;
	gboolean found = FALSE;
	gint end_var_pos = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (user_data));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (user_data);
	g_return_if_fail (IANJUTA_IS_EDITOR (priv->cur_editor));

	if (!priv->editing)
		return;
	g_return_if_fail (priv->editing_info != NULL);
	cur_pos = ianjuta_editor_get_position (priv->cur_editor, NULL);
	if (!IANJUTA_IS_EDITOR_SELECTION (priv->cur_editor))
		return;

	if (priv->selection_set_blocker)
	{
		priv->selection_set_blocker = FALSE;
		return;
	}

	/* We check to see if the current position is the start of a variable appearance
	   and select it if that happens */
	for (iter = priv->editing_info->snippet_vars_info; iter != NULL; iter = g_list_next (iter))
	{
		found = FALSE;

		cur_var_info = (SnippetVariableInfo *)iter->data;
		for (iter2 = cur_var_info->appearances; iter2 != NULL; iter2 = g_list_next (iter2))
		{

			var_iter = IANJUTA_ITERABLE (iter2->data);
			g_return_if_fail (IANJUTA_IS_ITERABLE (var_iter));

			if (ianjuta_iterable_diff (cur_pos, var_iter, NULL) == 0)
			{
				found = TRUE;

				if (IANJUTA_IS_ITERABLE (priv->cur_sel_start_iter))
				{
					if (ianjuta_iterable_diff (cur_pos, priv->cur_sel_start_iter, NULL) == 0)
					{
						g_object_unref (priv->cur_sel_start_iter);
						priv->cur_sel_start_iter = NULL;
						break;
					}
					g_object_unref (priv->cur_sel_start_iter);
				}

				IAnjutaIterable *end_iter = ianjuta_iterable_clone (var_iter, NULL);

				end_var_pos = cur_var_info->cur_value_length + 
					          ianjuta_iterable_get_position (var_iter, NULL);
				ianjuta_iterable_set_position (end_iter, end_var_pos, NULL);
				
				ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (priv->cur_editor),
				                              cur_pos, end_iter, TRUE, NULL);
				priv->cur_sel_start_iter = ianjuta_iterable_clone (cur_pos, NULL);

				priv->selection_set_blocker = TRUE;
				g_object_unref (end_iter);
				break;
			}
		}
		if (found)
			break;

	}
#if 0
	/* Check if we got out of the snippet bounds. */
	if (ianjuta_iterable_diff (priv->editing_info->snippet_start, cur_pos, NULL) > 0 || 
	    ianjuta_iterable_diff (priv->editing_info->snippet_end, cur_pos, NULL) < 0)
	{
		stop_snippet_editing_session (ANJUTA_SNIPPETS_INTERACTION (user_data));
		g_object_unref (cur_pos);
		return;
	}	
#endif

	g_object_unref (cur_pos);

}

static void
delete_snippet_editing_info (SnippetsInteraction *snippets_interaction)
{
	SnippetsInteractionPrivate *priv = NULL;
	GList *iter = NULL, *iter2 = NULL;
	SnippetVariableInfo *cur_var_info = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	if (priv->editing_info == NULL)
		return;

	g_object_unref (priv->editing_info->snippet_start);
	g_object_unref (priv->editing_info->snippet_end);
	if (priv->editing_info->snippet_finish_position != NULL)
		g_object_unref (priv->editing_info->snippet_finish_position);

	for (iter = g_list_first (priv->editing_info->snippet_vars_info); iter != NULL; iter = g_list_next (iter))	
	{
		cur_var_info = (SnippetVariableInfo *)iter->data;

		for (iter2 = g_list_first (cur_var_info->appearances); iter2 != NULL; iter2 = g_list_next (iter2))
		{
			if (!IANJUTA_IS_ITERABLE (iter2->data))
				g_return_if_reached ();

			g_object_unref (IANJUTA_ITERABLE (iter2->data));
		}
		g_list_free (cur_var_info->appearances);

		g_free (cur_var_info);
	}
	g_list_first (priv->editing_info->snippet_vars_info);

	priv->editing_info = NULL;
}

static gint
sort_appearances (gconstpointer a,
                  gconstpointer b)
{
	IAnjutaIterable *ia = IANJUTA_ITERABLE (a),
	                *ib = IANJUTA_ITERABLE (b);

	/* Assertions */
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (a), 0);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (b), 0);

	return ianjuta_iterable_get_position (ia, NULL) - ianjuta_iterable_get_position (ib, NULL);
}

static gint
sort_variables (gconstpointer a,
                gconstpointer b)
{
	SnippetVariableInfo *var1 = (SnippetVariableInfo *)a,
	                    *var2 = (SnippetVariableInfo *)b;
	IAnjutaIterable *var1_min = NULL, *var2_min = NULL;

	var1->appearances = g_list_sort (var1->appearances, sort_appearances);
	var2->appearances = g_list_sort (var2->appearances, sort_appearances);

	var1_min = IANJUTA_ITERABLE (var1->appearances->data);
	var2_min = IANJUTA_ITERABLE (var2->appearances->data);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (var1_min), 0);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (var2_min), 0);

	return ianjuta_iterable_get_position (var1_min, NULL) - 
	       ianjuta_iterable_get_position (var2_min, NULL);
}

static void
start_snippet_editing_session (SnippetsInteraction *snippets_interaction,
                               IAnjutaIterable *start_pos,
                               gint len)
{
	SnippetsInteractionPrivate *priv = NULL;
	gint finish_position = -1, cur_var_length = -1, i = 0, cur_appearance_pos = 0;
	GList *relative_positions = NULL, *variables_length = NULL,
	      *iter = NULL, *iter2 = NULL;
	GPtrArray *cur_var_positions = NULL;
	SnippetVariableInfo *cur_var_info = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_if_fail (ANJUTA_IS_SNIPPET (priv->cur_snippet));
	g_return_if_fail (IANJUTA_IS_EDITOR (priv->cur_editor));

	/* Mark the editing session */
	priv->editing = TRUE;

	/* Clear the previous editing_info structure if needed */
	delete_snippet_editing_info (snippets_interaction);

	/* Create the editing_info structure */
	priv->editing_info = g_new0 (SnippetEditingInfo, 1);
	priv->editing_info->snippet_start = ianjuta_iterable_clone (start_pos, NULL);
	priv->editing_info->snippet_end = ianjuta_iterable_clone (start_pos, NULL);
	ianjuta_iterable_set_position (priv->editing_info->snippet_end,
	                               ianjuta_iterable_get_position (start_pos, NULL) + len,
	                               NULL);

	finish_position = snippet_get_cur_value_end_position (priv->cur_snippet);
	if (finish_position >= 0)
	{
		priv->editing_info->snippet_finish_position = ianjuta_iterable_clone (start_pos, NULL);
		ianjuta_iterable_set_position (priv->editing_info->snippet_finish_position, 
			                           ianjuta_iterable_get_position (start_pos, NULL) + finish_position,
			                           NULL);
	}
	else
	{
		priv->editing_info->snippet_finish_position = NULL;
	}

	/* Calculate positions of each variable appearance */
	relative_positions = snippet_get_variable_relative_positions (priv->cur_snippet);
	variables_length   = snippet_get_variable_cur_values_len (priv->cur_snippet);

	iter  = g_list_first (relative_positions);
	iter2 = g_list_first (variables_length);
	while (iter != NULL && iter2 != NULL)
	{
		cur_var_positions = (GPtrArray *)iter->data;
		cur_var_length    = GPOINTER_TO_INT (iter2->data);

		/* If the variable doesn't have any appearance, we don't add it */
		if (!cur_var_positions->len)
		{
			iter  = g_list_next (iter);
			iter2 = g_list_next (iter2);
			continue;
		}

		/* Initialize the current variable info */
		cur_var_info = g_new0 (SnippetVariableInfo, 1);
		cur_var_info->cur_value_length = cur_var_length;
		cur_var_info->appearances      = NULL;

		/* Add each variable appearance relative positions */
		for (i = 0; i < cur_var_positions->len; i ++)
		{
			IAnjutaIterable *new_iter = NULL;
			cur_appearance_pos = GPOINTER_TO_INT (g_ptr_array_index (cur_var_positions, i));
			new_iter = ianjuta_iterable_clone (start_pos, NULL);
			ianjuta_iterable_set_position (new_iter,
			                               ianjuta_iterable_get_position (new_iter, NULL) + cur_appearance_pos,
			                               NULL);

			cur_var_info->appearances = g_list_append (cur_var_info->appearances,
			                                           new_iter);
		}
		
		g_ptr_array_unref (cur_var_positions);
		iter  = g_list_next (iter);
		iter2 = g_list_next (iter2);

		priv->editing_info->snippet_vars_info = g_list_append (priv->editing_info->snippet_vars_info,
		                                                       cur_var_info);

	}
	g_list_free (relative_positions);
	g_list_free (variables_length);

	/* Sort the list with appearances so the user will edit the ones that appear first
	   when the editing starts. */
	priv->editing_info->snippet_vars_info = 
		g_list_sort (priv->editing_info->snippet_vars_info, sort_variables);
	
	priv->editing_info->cur_var = g_list_first (priv->editing_info->snippet_vars_info);
	focus_on_next_snippet_variable (snippets_interaction);

}

static void
stop_snippet_editing_session (SnippetsInteraction *snippets_interaction)
{
	SnippetsInteractionPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	if (!priv->editing)
		return;

	/* Mark the editing session */
	priv->editing = FALSE;
	priv->changing_values_blocker = FALSE;
	priv->selection_set_blocker = FALSE;
	if (IANJUTA_IS_ITERABLE (priv->cur_sel_start_iter))
		g_object_unref (priv->cur_sel_start_iter);
	priv->cur_sel_start_iter = NULL;

	/* Clear the previous editing info structure if needed */
	delete_snippet_editing_info (snippets_interaction);
	
}

/* Public methods */

SnippetsInteraction* 
snippets_interaction_new ()
{
	return ANJUTA_SNIPPETS_INTERACTION (g_object_new (snippets_interaction_get_type (), NULL));
}

void
snippets_interaction_start (SnippetsInteraction *snippets_interaction,
                            AnjutaShell *shell)
{
	SnippetsInteractionPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	priv->shell = shell;
	priv->cur_editor = NULL;

}

void
snippets_interaction_destroy (SnippetsInteraction *snippets_interaction)
{
	/* TODO */
}


void                 
snippets_interaction_insert_snippet (SnippetsInteraction *snippets_interaction,
                                     SnippetsDB *snippets_db,
                                     AnjutaSnippet *snippet)
{
	SnippetsInteractionPrivate *priv = NULL;
	gchar *indent = NULL, *cur_line = NULL, *snippet_default_content = NULL;
	IAnjutaIterable *line_begin = NULL, *cur_pos = NULL;
	gint cur_line_no = -1, i = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	/* Check we have an editor loaded */
	if (!IANJUTA_IS_EDITOR (priv->cur_editor))
		return;

	/* Get the current line */
	cur_line_no = ianjuta_editor_get_lineno (priv->cur_editor, NULL);
	line_begin  = ianjuta_editor_get_line_begin_position (priv->cur_editor, cur_line_no, NULL);
	cur_pos     = ianjuta_editor_get_position (priv->cur_editor, NULL);
	cur_line    = ianjuta_editor_get_text (priv->cur_editor, line_begin, cur_pos, NULL);

	/* Calculate the current indentation */
	indent = g_strdup (cur_line);
	while (cur_line[i] == ' ' || cur_line[i] == '\t')
		i ++;
	indent[i] = 0;

	/* Get the default content of the snippet */
	snippet_default_content = snippet_get_default_content (snippet, 
	                                                       G_OBJECT (snippets_db), 
	                                                       indent);
	g_return_if_fail (snippet_default_content != NULL);
	
	/* Insert the default content into the editor */
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (priv->cur_editor), NULL);
	ianjuta_editor_insert (priv->cur_editor, 
	                       cur_pos, 
	                       snippet_default_content, 
	                       -1,
	                       NULL);
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (priv->cur_editor), NULL);
	ianjuta_document_grab_focus (IANJUTA_DOCUMENT (priv->cur_editor), NULL);

	priv->cur_snippet = snippet;
	start_snippet_editing_session (snippets_interaction, 
	                               cur_pos, 
	                               strlen (snippet_default_content));

	g_free (indent);
	g_free (snippet_default_content);
	g_object_unref (line_begin);
	g_object_unref (cur_pos);
	
}

void
snippets_interaction_trigger_insert_request (SnippetsInteraction *snippets_interaction,
                                             SnippetsDB *snippets_db)
{
	SnippetsInteractionPrivate *priv = NULL;
	IAnjutaIterable *rewind_iter = NULL, *cur_pos = NULL;
	gchar *trigger = NULL, cur_char = 0;
	gboolean reached_start = FALSE;
	AnjutaSnippet *snippet = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);
	g_return_if_fail (ANJUTA_IS_SHELL (priv->shell));

	/* Verify we have an editor */
	if (!IANJUTA_IS_EDITOR (priv->cur_editor))
		return;

	/* We first try to focus on the next variable in case we are editing a snippet */
	if (focus_on_next_snippet_variable (snippets_interaction))
		return;

	/* Initialize the iterators */
	cur_pos     = ianjuta_editor_get_position (priv->cur_editor, NULL);
	rewind_iter = ianjuta_iterable_clone (cur_pos, NULL);

	/* If we are inside a word we can't insert a snippet */
	cur_char = char_at_iterator (priv->cur_editor, cur_pos);
	if (IN_WORD (cur_char))
		return;

	/* If we can't decrement the cur_pos iterator, then we are at the start of the document,
	   so the trigger key is NULL */
	if (!ianjuta_iterable_previous (rewind_iter, NULL))
		return;
	cur_char = char_at_iterator (priv->cur_editor, rewind_iter);

	/* We iterate until we get to the start of the word or the start of the document */
	while (IN_WORD (cur_char))
	{	
		if (!ianjuta_iterable_previous (rewind_iter, NULL))
		{
			reached_start = TRUE;
			break;
		}

		cur_char = char_at_iterator (priv->cur_editor, rewind_iter);
	}

	/* If we didn't reached the start of the document, we move the iterator forward one
	   step so we don't delete one extra char. */
	if (!reached_start)
		ianjuta_iterable_next (rewind_iter, NULL);

	/* We compute the trigger-key */
	trigger = ianjuta_editor_get_text (priv->cur_editor, rewind_iter, cur_pos, NULL);

	/* If there is a snippet for our trigger-key we delete the trigger from the editor
	   and insert the snippet. */
	snippet = snippets_db_get_snippet (snippets_db, trigger, NULL);

	if (ANJUTA_IS_SNIPPET (snippet))
	{
		ianjuta_editor_erase (priv->cur_editor, rewind_iter, cur_pos, NULL);
		snippets_interaction_insert_snippet (snippets_interaction, snippets_db, snippet);
	}

	g_free (trigger);
	g_object_unref (rewind_iter);
	g_object_unref (cur_pos);

}

void
snippets_interaction_set_editor (SnippetsInteraction *snippets_interaction,
                                 IAnjutaEditor *editor)
{
	SnippetsInteractionPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_INTERACTION_GET_PRIVATE (snippets_interaction);

	/* Disconnect the handlers of the old editor */
	if (IANJUTA_IS_EDITOR (priv->cur_editor))
	{
		g_signal_handler_disconnect (priv->cur_editor, priv->changed_handler_id);
		g_signal_handler_disconnect (priv->cur_editor, priv->cursor_moved_handler_id);
	}

	/* Connect the handlers for the new editor */	
	if (IANJUTA_IS_EDITOR (editor))
	{
		priv->cur_editor = editor;
		
		priv->changed_handler_id = g_signal_connect (G_OBJECT (priv->cur_editor),
			                                         "changed",
			                                         G_CALLBACK (on_cur_editor_changed),
			                                         snippets_interaction);
		priv->cursor_moved_handler_id = g_signal_connect (G_OBJECT (priv->cur_editor),
			                                              "cursor-moved",
			                                              G_CALLBACK (on_cur_editor_cursor_moved),
			                                              snippets_interaction);
	}
	else
	{
		priv->cur_editor = NULL;
	}

	/* We end our editing session (if we had one) */
	stop_snippet_editing_session (snippets_interaction);

}

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
 
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "file_history.h"

#define OPT_ENTRIES 5
#define MAX_ENTRIES 6

typedef struct _AnFileHistory
{
	GList *items;
	GList *current;
	gboolean history_move;
} AnFileHistory;

static AnFileHistory *s_history = NULL;

static void an_file_history_init()
{
	s_history = g_new(AnFileHistory, 1);
	s_history->items = NULL;
	s_history->current = NULL;
	s_history->history_move = FALSE;
}

AnHistFile *an_hist_file_new (GFile *file, gint line)
{
	AnHistFile *h_file;

	g_return_val_if_fail(file, NULL);
	
	h_file= g_new(AnHistFile, 1);
	h_file->file = g_object_ref (file);
	h_file->line = line;
	return h_file;
}

void an_hist_file_free(AnHistFile *h_file)
{
	g_return_if_fail(h_file);
	g_object_unref (h_file->file);
	g_free(h_file);
}

static void an_hist_items_free(GList *items)
{
	GList *tmp;

	g_return_if_fail(items);
	for (tmp = items; tmp; tmp = g_list_next(tmp))
		an_hist_file_free((AnHistFile *) tmp->data);
	g_list_free(items);
}

void an_file_history_reset(void)
{
	g_return_if_fail(s_history && s_history->items);

	an_hist_items_free(s_history->items);
	s_history->items = NULL;
	s_history->current = NULL;
}

void an_file_history_push (GFile *file, gint line)
{
	AnHistFile *h_file;

	g_return_if_fail (file);

	if (!s_history)
		an_file_history_init();
	
	if (s_history->current)
	{
		GList *next;
		
		/* Only update line number when called by forward/backward */
		if (s_history->history_move)
		{
			AnHistFile *h_file = (AnHistFile *) s_history->current->data;
			
			if (g_file_equal (file,h_file->file))
			{
				h_file->line = line;
			}
			return;
		}
				
		next = s_history->current->next;
		s_history->current->next = NULL;
		an_hist_items_free(s_history->items);
			
		s_history->items = next;			
		if (next) next->prev = NULL;
		s_history->current = NULL;
		if (g_list_length(s_history->items) > MAX_ENTRIES)
		{
			GList *tmp = g_list_nth(s_history->items, OPT_ENTRIES);
			an_hist_items_free(tmp->next);
			tmp->next = NULL;
		}
	}
	h_file = an_hist_file_new(file, line);
	s_history->items = g_list_prepend(s_history->items, h_file);
	s_history->current = NULL;
}

void an_file_history_back(AnjutaDocman *docman)
{
	AnHistFile *h_file;
	GList *current;

	if (!(s_history && (!s_history->current || s_history->current->next)))
		return;

	current = s_history->current ? s_history->current->next : s_history->items;
	h_file = (AnHistFile *) current->data;
	
	s_history->history_move = TRUE;
	anjuta_docman_goto_file_line_mark (docman, h_file->file,
									   h_file->line, FALSE);
	s_history->history_move = FALSE;
	
	s_history->current = current;
}

void an_file_history_forward(AnjutaDocman *docman)
{
	AnHistFile *h_file;
	GList *current;

	if (!(s_history && s_history->current && s_history->current->prev))
		return;
	
	current = s_history->current->prev;
	h_file = (AnHistFile *) current->data;
	
	s_history->history_move = TRUE;
	anjuta_docman_goto_file_line_mark(docman, h_file->file,
									  h_file->line, FALSE);
	s_history->history_move = FALSE;
	
	s_history->current = current;
}

void an_file_history_dump(void)
{
	GList *tmp;
	AnHistFile *h_file;

	g_return_if_fail(s_history && s_history->items);
	fprintf(stderr, "--------------------------\n");
	for (tmp = s_history->items; tmp; tmp = g_list_next(tmp))
	{
		gchar *uri;
		h_file = (AnHistFile *) tmp->data;
		uri = g_file_get_uri (h_file->file);
		fprintf(stderr, "%s:%d", uri, h_file->line);
		g_free (uri);
		if (tmp == s_history->current)
			fprintf(stderr, " (*)");
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "--------------------------\n");
}

void an_file_history_free(void)
{
	an_file_history_reset();
	g_free(s_history);
	s_history = NULL;
}

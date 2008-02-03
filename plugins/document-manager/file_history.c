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
} AnFileHistory;

static AnFileHistory *s_history = NULL;

static void an_file_history_init()
{
	s_history = g_new(AnFileHistory, 1);
	s_history->items = NULL;
	s_history->current = NULL;
}

AnHistFile *an_hist_file_new (const gchar *uri, gint line)
{
	AnHistFile *h_file;

	g_return_val_if_fail(uri, NULL);
	h_file= g_new(AnHistFile, 1);
	h_file->uri = g_strdup(uri);
	h_file->line = line;
	return h_file;
}

void an_hist_file_free(AnHistFile *h_file)
{
	g_return_if_fail(h_file);
	g_free(h_file->uri);
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

void an_file_history_push (const gchar *uri, gint line)
{
	AnHistFile *h_file;

	g_return_if_fail (uri);
	if (!s_history)
		an_file_history_init();
	if (s_history->current)
	{
		AnHistFile *current = (AnHistFile *) s_history->current->data;
		if (strcmp(uri, current->uri) == 0 &&
			(current->line < 1 || line == current->line))
		{
			current->line = line;
			return;
		}
		if (s_history->current != s_history->items)
		{
			GList *tmp = s_history->current->prev;
			if (tmp)
			{
				tmp->next = NULL;
				an_hist_items_free(s_history->items);
			}
			s_history->items = s_history->current;
			s_history->current->prev = NULL;
		}
		if (g_list_length(s_history->items) > MAX_ENTRIES)
		{
			GList *tmp = g_list_nth(s_history->items, OPT_ENTRIES);
			an_hist_items_free(tmp->next);
			tmp->next = NULL;
		}
	}
	h_file = an_hist_file_new(uri, line);
	s_history->items = g_list_prepend(s_history->items, h_file);
	s_history->current = s_history->items;
}

void an_file_history_back(AnjutaDocman *docman)
{
	AnHistFile *h_file;

	if (!(s_history && s_history->current && s_history->current->next))
		return;

	s_history->current = s_history->current->next;
	h_file = (AnHistFile *) s_history->current->data;
	anjuta_docman_goto_uri_line_mark (docman, h_file->uri,
									   h_file->line, FALSE);
}

void an_file_history_forward(AnjutaDocman *docman)
{
	AnHistFile *h_file;

	if (!(s_history && s_history->current && s_history->current->prev))
		return;
	
	s_history->current = s_history->current->prev;
	h_file = (AnHistFile *) s_history->current->data;
	anjuta_docman_goto_uri_line_mark(docman, h_file->uri,
									  h_file->line, FALSE);
}

void an_file_history_dump(void)
{
	GList *tmp;
	AnHistFile *h_file;

	g_return_if_fail(s_history && s_history->items);
	fprintf(stderr, "--------------------------\n");
	for (tmp = s_history->items; tmp; tmp = g_list_next(tmp))
	{
		h_file = (AnHistFile *) tmp->data;
		fprintf(stderr, "%s:%d", h_file->uri, h_file->line);
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

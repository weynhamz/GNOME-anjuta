/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/*
** search-replace_backend.c: Generic Search and Replace
** Author: Biswapesh Chattopadhyay
*/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gnome.h>
#include <glade/glade.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/anjuta-convert.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

/*
#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"
*/

#include "search-replace_backend.h"

/* Information about a matched substring */
typedef struct _MatchSubStr
{
	gint start;
	gint len;
} MatchSubStr;

static SearchReplace *sr = NULL;

void clear_search_replace_instance(void);

static void match_substr_free(MatchSubStr *ms)
{
	g_free(ms);
}


void
match_info_free (MatchInfo *mi)
{
	if (mi)
	{
		if (mi->subs)
		{
			GList *tmp;
			for (tmp = mi->subs; tmp; tmp = g_list_next(tmp))
				match_substr_free((MatchSubStr *) tmp->data);
			g_list_free(mi->subs);
		}
		g_free(mi);
	}
}


void
file_buffer_free (FileBuffer *fb)
{
	if (fb)
	{
		if (fb->file)
			g_object_unref (fb->file);
		if (fb->buf)
			g_free(fb->buf);
		if (fb->lines)
			g_list_free(fb->lines);
		g_free(fb);
	}
}

/* Create a file buffer structure from a TextEditor structure */
FileBuffer *
file_buffer_new_from_te (IAnjutaEditor *te)
{
	FileBuffer *fb = NULL;
	gint len;
	
	g_return_val_if_fail(te, NULL);
	
	len = ianjuta_editor_get_length(te, NULL);
	if (len != 0)
	{
		fb = g_new0(FileBuffer, 1);
		fb->type = FB_EDITOR;
		fb->te = te;
		fb->file = ianjuta_file_get_file(IANJUTA_FILE(te), NULL);
		fb->buf = ianjuta_editor_get_text_all (fb->te, NULL);
		fb->len = len;
	}
	
	return fb;
}

/* Only use the first 500 chars for validating (yes, I feel lucky...) */
#define MAX_VALIDATE 500

FileBuffer *
file_buffer_new_from_uri (const gchar *uri)
{
	FileBuffer *fb;
	GFile *file;
	IAnjutaDocument* doc;
	gchar *buffer;
	gsize length;

	g_return_val_if_fail(uri, NULL);
	
	
	file = g_file_new_for_uri (uri);
	
	/* There might be an already open TextEditor with this path */
	doc = ianjuta_document_manager_find_document_with_file (sr->docman,
														 file, NULL);
	if (doc && IANJUTA_IS_EDITOR (doc))
	{
		IAnjutaEditor *te;
		
		te = IANJUTA_EDITOR (doc);
		g_object_unref (file);

		return file_buffer_new_from_te(te);
	}

	if (!g_file_load_contents (file, NULL, &buffer, &length, NULL, NULL))
	{
		/* Unable to read file */
		g_object_unref (file);
		
		return NULL;
	}
	
	if (!g_utf8_validate (buffer, MIN(MAX_VALIDATE, length), NULL))
	{
		const AnjutaEncoding *encoding_used = NULL;
		gchar* converted_text;
		gsize converted_len;
		converted_text = anjuta_convert_to_utf8 (buffer, 
												 length, 
												 &encoding_used,
												 &converted_len, 
												 NULL);
		if (converted_text == NULL)
		{
			/* Last change, let's try 8859-15 */
			encoding_used =  
				anjuta_encoding_get_from_charset("ISO-8859-15");
			
			converted_text = anjuta_convert_to_utf8 (buffer,
												  length,
												  &encoding_used,
												  &converted_len,
												  NULL);
		}
		g_free (buffer);
		
		if (converted_text == NULL)
		{
			/* Give up */
			g_object_unref (file);
			
			return NULL;
		}
		
		buffer = converted_text;
		length = converted_len;
	}
	
	fb = g_new0(FileBuffer, 1);
	fb->type = FB_FILE;
	
	fb->file = file;
	fb->len = length;
	fb->buf = buffer;
	
	return fb;
}

static long
file_buffer_line_from_pos(FileBuffer *fb, gint pos)
{
	gint lineno = -1;
	
	g_return_val_if_fail(fb && pos >= 0, 1);
	
	if (FB_FILE == fb->type)
	{
		GList *tmp;
		
		if (fb->lines == NULL)
		{
			gint i;
			/* First line starts at column 0 */
			fb->lines = g_list_prepend(fb->lines, GINT_TO_POINTER(0));
			lineno = 0;
			for (i=0; i < fb->len; ++i)
			{
				if ('\n' == fb->buf[i] && '\0' != fb->buf[i+1])
				{
					fb->lines = g_list_prepend(fb->lines, GINT_TO_POINTER(i + 1));
					if (0 == fb->line && fb->pos > i)
						fb->line = lineno;
					++ lineno;
				}
			}
			fb->lines = g_list_reverse(fb->lines);
		}
		
		for (tmp = fb->lines; tmp; tmp = g_list_next(tmp))
		{
			if (pos < GPOINTER_TO_INT(tmp->data))
				return lineno;
			++ lineno;
		}
		return lineno;
	}
	else if (FB_EDITOR == fb->type)
	{
		IAnjutaIterable *position;
		gint offset;
		
		offset = g_utf8_strlen (fb->buf, pos);
		position = ianjuta_editor_get_position_from_offset (fb->te, offset, NULL);
		lineno = ianjuta_editor_get_line_from_position (fb->te, position, NULL);
		g_object_unref (position);
		
		return lineno;
	}
	else
		return -1;
}

gchar *
file_match_line_from_pos(FileBuffer *fb, int pos)
{
	gint length=1;
	gint i;
	g_return_val_if_fail(fb && pos >= 0, NULL);

	for (i= pos+1; ((fb->buf[i] != '\n') && (fb->buf[i] != '\0')); i++, length++);
	for (i= pos -1; (fb->buf[i] != '\n') && (i >= 0); i--, length++);
	
	return g_strndup (fb->buf + i + 1, length);
}

/* Get a list of all project files */
static GList *
get_project_file_list(void)
{
	GList* list = NULL;
	GList *files = NULL;
	gchar *project_root_uri = NULL;
	
	anjuta_shell_get (ANJUTA_PLUGIN(sr->docman)->shell,
					  "project_root_uri", G_TYPE_STRING,
					  &project_root_uri, NULL);
	
	if (project_root_uri)
	{
		IAnjutaProjectManager* prjman;
		prjman = anjuta_shell_get_interface(ANJUTA_PLUGIN(sr->docman)->shell, 
											IAnjutaProjectManager , NULL);
		
		list = ianjuta_project_manager_get_elements (prjman,
													 IANJUTA_PROJECT_MANAGER_SOURCE,
													 NULL);
		if (list)
		{
			const gchar *uri;
			GList *node;
			node = list;
	
			while (node)
			{
				uri = (const gchar *)node->data;
				files = g_list_prepend (files, g_strdup (uri));
				node = g_list_next (node);
			}
			files = g_list_reverse (files);
			g_list_free(list);
		}
	}
	g_free (project_root_uri);
	return files;
}


static gboolean
isawordchar (gunichar c)
{
	return (g_unichar_isalnum(c) || '_' == c);
}

static gboolean
extra_match (gboolean at_start, gchar* begin, gchar* end, SearchExpression *s)
{
	gunichar b, e;
	
	b = g_utf8_get_char (g_utf8_prev_char (begin));
	e = g_utf8_get_char (end);
	if (g_unichar_ismark(e))
		return FALSE;  /* matched character continues past match end */
	
	if (s->whole_line)
		if ((at_start || b == '\n' || b == '\r') &&
			(e == '\0'	|| e == '\n' || e == '\r'))
			return TRUE;
		else
			return FALSE;
	else if (s->whole_word)
		if ((at_start || !isawordchar(b)) && 
			(e=='\0' || !isawordchar(e)))
			return TRUE;
		else
			return FALSE;
	else if (s->word_start)
		if (at_start || !isawordchar(b))
			return TRUE;
		else
			return FALSE;	
	else
		return TRUE;
}

static MatchInfo *
get_next_regex_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s)
{
	MatchInfo *mi = NULL;
	GMatchInfo* match_info;
	
	if (s->regex_info == NULL)
	{
		GError* error = NULL;
		GRegexCompileFlags compile_flags = 0;
		GRegexMatchFlags match_flags = 0;
	
		match_flags |= G_REGEX_MATCH_NOTEMPTY;
		if (!s->match_case)
		{
			compile_flags |= G_REGEX_CASELESS;
		}
		if (!s->greedy)
		{
			compile_flags |= G_REGEX_UNGREEDY;
		}
		s->regex_info = g_regex_new (s->search_str, compile_flags,
									 match_flags, &error);
		if (error)
		{
			anjuta_util_dialog_error (NULL, error->message);
			g_error_free(error);
			s->regex_info = NULL;
			return NULL;
		}
	}

	g_regex_match_full (s->regex_info, fb->buf, fb->len,
				   fb->pos, 
				   G_REGEX_MATCH_NOTEMPTY, &match_info, NULL);
	
	if (g_match_info_matches (match_info))
	{
		gint start;
		gint end;
		int i;
		mi = g_new0(MatchInfo, 1);
		if (g_match_info_fetch_pos (match_info, 0, &start, &end))
		{
			DEBUG_PRINT ("Regex: %d %d", start, end);
			mi->pos = start;
			mi->len = end - mi->pos;
			mi->line = file_buffer_line_from_pos(fb, mi->pos);
		}
		for (i = 1; i < g_match_info_get_match_count(match_info); i++) /* Captured subexpressions */
		{
			MatchSubStr *ms;
			ms = g_new0(MatchSubStr, 1);
			if (g_match_info_fetch_pos (match_info, i, &start, &end))
			{
				ms->start =start;
				ms->len = end - ms->start;
			}
			mi->subs = g_list_prepend(mi->subs, ms);
		}
		mi->subs = g_list_reverse(mi->subs);
		fb->pos = end;
	}
	return mi;
}

static MatchInfo *match_info(FileBuffer *fb, gchar* match, gchar* match_end,
							 SearchDirection direction)
{
	MatchInfo *mi = g_new0 (MatchInfo, 1);
	mi->pos = match - fb->buf;
	mi->len = match_end - match;
	mi->line = file_buffer_line_from_pos (fb, mi->pos);

	if (direction == SD_BACKWARD)
		fb->pos = mi->pos;
	else
		fb->pos = mi->pos + mi->len;
	
	return mi;
}

/* Get the next match, given that the search string is ASCII (as will nearly
 * always be the case).  In this case we don't need to perform any UTF-8
 * normalization, since any bytes with values 0-127 in a UTF-8 string always
 * actually represent ASCII characters. */
static MatchInfo *
get_next_ascii_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s)
{
	int len = strlen(s->search_str);
	gint (*compare)(const gchar *, const gchar *, gsize) =
		s->match_case ? strncmp : g_ascii_strncasecmp;
	gchar *p = fb->buf + fb->pos;
	
	if (direction == SD_BACKWARD)
	{
		for (; p >= fb->buf; --p)
			if (!compare(p, s->search_str, len) &&
				extra_match(p == fb->buf, p, p + len, s))
				return match_info(fb, p, p + len, direction);
	}
	else
	{
		for (; *p ; ++p)
			if (!compare(p, s->search_str, len) &&
				extra_match(p == fb->buf, p, p + len, s))
				return match_info(fb, p, p + len, direction);
	}
	
	return NULL;			
}

/* Normalize a UTF-8 string, optionally folding case.  Returns NULL if invalid. */
static gchar* normalize(gchar *s, int len_bytes, gboolean match_case) {
	gchar *c, *n;
	
	if (match_case)
		return g_utf8_normalize(s, len_bytes, G_NORMALIZE_NFD);
	
	c = g_utf8_casefold(s, len_bytes);
	n = g_utf8_normalize(c, -1, G_NORMALIZE_NFD);
	g_free(c);
	return n;
}

/* Given a UTF-8 string s, advance past a prefix whose normalized length in
 * bytes is [normal_bytes]. */
static gchar* normal_advance(gchar *s, int normal_bytes, gboolean match_case)
{
	while (*s && normal_bytes > 0)
	{
		gchar *next = g_utf8_next_char(s);
		gchar *n = normalize(s, next - s, match_case);
		normal_bytes -= strlen(n);
		g_free(n);
		s = next;
	}
	return s;
}

static MatchInfo *
get_next_utf8_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s)
{
	gchar* search_key = normalize(s->search_str, -1, s->match_case);
	gchar* current = fb->buf + fb->pos;
	gchar* search_buf;
	gchar* p = NULL;
	MatchInfo *mi = NULL;
	int keylen;
	
	if (!search_key)
		return NULL;
	keylen = strlen(search_key);
	
	if (direction == SD_BACKWARD)
	{
		search_buf = normalize(fb->buf, current + strlen (s->search_str) - 1 - fb->buf, s->match_case);
		if (search_buf)
		{
			p = search_buf + strlen(search_buf);
			while ((p = g_strrstr_len(search_buf, p - search_buf, search_key)) != NULL)
			{
				if (extra_match (p == search_buf, p, p + keylen, s))
					break;
				p = p + keylen - 1;
			}
		}
	}
	else
	{  /* forward search */
		search_buf = normalize(current, -1, s->match_case);
		if (search_buf)
		{
			p = search_buf;
			while ((p = strstr(p, search_key)) != NULL)
			{
				if (extra_match (fb->pos == 0 && p == search_buf, p, p + keylen, s))
					break;
				++p;
			}
		}
	}
	g_free(search_key);
	
	if (p)
	{
		/* We've found a match in the normalized buffer.  Now find the
		 * corresponding position in the source buffer. */
		gchar* match = normal_advance(
								direction == SD_BACKWARD ? fb->buf : current,
								p - search_buf, s->match_case);
		gchar* match_end = normal_advance(match, keylen, s->match_case);
		mi = match_info(fb, match, match_end, direction);
	}
	
	g_free(search_buf);
	return mi;
}

static gboolean str_is_ascii(gchar* s)
{
	for (; *s; ++s)
		if (!isascii(*s))
			return FALSE;
	return TRUE;
}

/* Returns the next match in the passed buffer.  If direction is SD_BACKWARD,
   finds the previous match whose end falls before the current position.
   The search expression should be pre-compiled.  The returned pointer should
   be freed with match_info_free() when no longer required. */
MatchInfo *
get_next_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s)
{
	g_return_val_if_fail(fb && s, NULL);
	
	if (s->regex)
		return get_next_regex_match(fb, direction, s);
	
	return str_is_ascii(s->search_str) ?
		get_next_ascii_match(fb, direction, s) :
		get_next_utf8_match(fb, direction, s);
}

static gint search_entry_compare(gconstpointer a, gconstpointer b)
{
	gchar* a_uri = ((SearchEntry *) a)->uri;
	gchar* b_uri = ((SearchEntry *) b)->uri;
	return g_strcmp0(a_uri, b_uri);
}

static void search_entry_free (gpointer data, gpointer user_data)
{
	g_free (((SearchEntry *)data)->uri);
	g_free (data);
}

/* Remove duplicate entries in the list, the list must be sorted */
static GList* search_entries_remove_duplicate (GList *entries)
{
	GList *node;
	
	for (node = g_list_first (entries); (node != NULL) && (node->next != NULL); )
	{
		GList *next = g_list_next (node);
		SearchEntry *se = (SearchEntry *)node->data;
		SearchEntry *next_se = (SearchEntry *)next->data;
		
		if ((se->te == next_se->te) && (search_entry_compare (se, next_se) == 0))
		{
			/* Find a duplicate entry , remove it*/
			
			search_entry_free (next_se, NULL);
			entries = g_list_delete_link (entries, next);
		}
		else
		{
			/* Get next entry */
			node = g_list_next(node);
		}
	}
	
	return entries;
}

/* Create list of search entries */
GList *
create_search_entries (Search *s)
{
	GList *entries = NULL;
	GList *tmp;
	GList *editors;
	IAnjutaDocument *doc;
	SearchEntry *se;
	gint tmp_pos;
	gint selstart;

	switch (s->range.type)
	{
		case SR_BUFFER:
			doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
			if (doc && IANJUTA_IS_EDITOR (doc))
			{
				se = g_new0 (SearchEntry, 1);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR (doc);
				se->direction = s->range.direction;
				if (SD_BEGINNING == se->direction)
				{
					se->start_pos = 0;
					se->end_pos = -1;
					se->direction = SD_FORWARD;
				}
				else
				{	
					IAnjutaIterable *start;
					/* forward-search from after beginning of selection, if any
					   backwards-search from before beginning of selection, if any
					   treat -ve positions except -1 as high +ve */
					start = ianjuta_editor_selection_get_start
							(IANJUTA_EDITOR_SELECTION (se->te), NULL);
					if (start)
					{
						selstart =
							ianjuta_iterable_get_position (start, NULL);
						if (se->direction == SD_BACKWARD)
						{
							se->start_pos = (selstart != 0) ?
								 selstart - 1 : selstart;
						}
						else
						{
							se->start_pos =
								(selstart != -2 &&
								 selstart < ianjuta_editor_get_length (IANJUTA_EDITOR (se->te), NULL)) ?
								 selstart + 1 : selstart;
						}
						g_object_unref (start);
					}
					else
					{
						se->start_pos = ianjuta_editor_get_offset (se->te, NULL);
					}
					se->end_pos = -1;	/* not actually used when backward searching */
				}
				entries = g_list_prepend(entries, se);
			}
			break;
		case SR_SELECTION:
		case SR_BLOCK:
		case SR_FUNCTION: 
			doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
			if (doc && IANJUTA_IS_EDITOR (doc))
			{
				gint selend;
				
				se = g_new0 (SearchEntry, 1);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR (doc);
				se->direction = s->range.direction;
				if (se->direction == SD_BEGINNING)
					se->direction = SD_FORWARD;

				if (s->range.type == SR_SELECTION)
				{
					selstart = selend = 0;	/* warning prevention only */
				}				
				else
				{
					IAnjutaIterable* end =
						ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					if (end)
					{
						selstart = selend = ianjuta_iterable_get_position (end, NULL);
						g_object_unref (end);
					}
					else
					{
						selstart = selend = 0;	/* warning prevention only */
						g_assert ("No selection end position");
					}
				}

				if (s->range.type == SR_BLOCK)
					ianjuta_editor_selection_select_block(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				if (s->range.type == SR_FUNCTION)
					ianjuta_editor_selection_select_function(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				{
					IAnjutaIterable *start, *end;
					start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					end = ianjuta_editor_selection_get_end(IANJUTA_EDITOR_SELECTION (se->te), NULL);
					se->start_pos =  ianjuta_iterable_get_position (start, NULL);
					se->end_pos = ianjuta_iterable_get_position (end, NULL);
					g_object_unref (start);
					g_object_unref (end);
				}
				if (se->direction == SD_BACKWARD)
				{
					tmp_pos = se->start_pos;
					se->start_pos = se->end_pos;
					se->end_pos = tmp_pos;
				}	
				entries = g_list_prepend (entries, se);
				if (s->range.type != SR_SELECTION)
				{
					IAnjutaIterable *start, *end;
					start = ianjuta_editor_get_position_from_offset (se->te, selstart, NULL);
					end = ianjuta_editor_get_position_from_offset (se->te, selend, NULL);
					ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (se->te), 
				                                 start, end, TRUE, NULL);	
					g_object_unref (start);
					g_object_unref (end);
				}
			}
			break;
		case SR_OPEN_BUFFERS:
			editors = ianjuta_document_manager_get_doc_widgets (sr->docman, NULL);
			for (tmp = editors; tmp; tmp = g_list_next(tmp))
			{
				if (IANJUTA_IS_EDITOR (tmp->data))
				{
					gchar *uri = NULL;

					if (IANJUTA_IS_FILE (tmp->data))
					{
						GFile *file = ianjuta_file_get_file (IANJUTA_FILE (tmp->data), NULL);
						
						if (file != NULL)
						{
							uri = g_file_get_uri (file);
							g_object_unref (file);
						}
					}
					
					se = g_new0 (SearchEntry, 1);
					se->type = SE_BUFFER;
					se->te = IANJUTA_EDITOR (tmp->data);
					se->uri = uri;
					se->direction = SD_FORWARD;
					se->start_pos = 0;
					se->end_pos = -1;
					entries = g_list_prepend(entries, se);
				}
			}
			g_list_free (editors);
			break;
		case SR_FILES:
		case SR_PROJECT:
		{
			GList *files = NULL;
			gchar *dir = NULL;
			gchar *dir_uri = NULL;		
					
			anjuta_shell_get (ANJUTA_PLUGIN(sr->docman)->shell,
							  "project_root_uri", G_TYPE_STRING,
							  &dir_uri, NULL);
			// FIXME : Replace Standard UNIX IO functions by gio 
			if (dir_uri)			
				dir = anjuta_util_get_local_path_from_uri(dir_uri);
			
			if (!dir)
			{
				if (SR_PROJECT == s->range.type)
					s->range.type = SR_FILES;
				dir = g_get_current_dir();
			}

			files = get_project_file_list();	
			
			if (files)
			{
				for (tmp = files; tmp; tmp = g_list_next(tmp))
				{
					se = g_new0(SearchEntry, 1);
					se->type = SE_FILE;
					se->uri = (char *) tmp->data;
					se->direction = SD_FORWARD;
					se->type = SE_FILE;
					se->start_pos = 0;
					se->end_pos = -1;
					entries = g_list_prepend(entries, se);
				}
				g_list_free(files);
			}
			g_free(dir);
			g_free(dir_uri);
			break;
		}
	}

	entries = g_list_sort(entries, search_entry_compare);
	entries = search_entries_remove_duplicate (entries);
	
	return entries;		
}

void free_search_entries (GList *entries)
{
	g_list_foreach (entries, search_entry_free, NULL);
	g_list_free (entries);
}

gchar *
regex_backref (MatchInfo *mi, FileBuffer *fb)
{
#define REGX_BUFSIZE 1024
	gint i, j, k;
	gint nb_backref;
	gint i_backref;
	gint plen;
	gint start, len;
	gint backref[10] [2];	/* backref [0][2] unused */
	gchar buf [REGX_BUFSIZE + 4];	/* usable space + word-sized space for trailing 0 */
	GList *tmp;
	
	i = 1;
	/* Extract back references */
	tmp = mi->subs;
	while (tmp && i < 10)
	{
		backref[i] [0] = ((MatchSubStr*)tmp->data)->start;
		backref[i] [1] = ((MatchSubStr*)tmp->data)->len;
		tmp= g_list_next(tmp);
		i++;
	}
	nb_backref = i;
	plen = strlen (sr->replace.repl_str);
	for(i=0, j=0; i < plen && j < REGX_BUFSIZE; i++)
	{
		if (sr->replace.repl_str[i] == '\\')
		{
			i++;
			if (sr->replace.repl_str[i] > '0' && sr->replace.repl_str[i] <= '9')
			{
				i_backref = sr->replace.repl_str[i] - '0';
				if (i_backref < nb_backref)
				{
					start = backref[i_backref] [0];
					len = backref[i_backref] [1];
					for (k=0; k < len && j < REGX_BUFSIZE; k++)
						buf[j++] = fb->buf[start + k];	
				}
			}	
		}	
		else
			buf[j++] = sr->replace.repl_str[i];				
	}
	buf[j] = '\0';

	return g_strdup (buf);
}

#define FREE_FN(fn, v) if (v) { fn(v); v = NULL; }

void
clear_search_replace_instance(void)
{
	g_free (sr->search.expr.search_str);
	if (sr->search.expr.regex_info)
	{
		g_regex_unref (sr->search.expr.regex_info);
		sr->search.expr.regex_info = NULL;
	}
	if (SR_FILES == sr->search.range.type)
	{
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_files);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_files);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_dirs);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_dirs);
	}
	FREE_FN(anjuta_util_glist_strings_free, sr->search.expr_history);
	g_free (sr->replace.repl_str);
	FREE_FN(anjuta_util_glist_strings_free, sr->replace.expr_history);
}

SearchReplace *
create_search_replace_instance(IAnjutaDocumentManager *docman)
{
	/* Create a new SearchReplace instance */
	if (NULL == sr)
	{
		sr = g_new0(SearchReplace, 1);
		sr->search.expr.regex_info = NULL;
	}
	else
		clear_search_replace_instance ();
	if (docman)
		sr->docman = docman;
	return sr;
}

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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
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
#include "tm_tagmanager.h"

/* Information about a matched substring */
typedef struct _MatchSubStr
{
	long start;
	long len;
} MatchSubStr;

static SearchReplace *sr = NULL;

void clear_search_replace_instance(void);


static void
pcre_info_free (PcreInfo *re)
{
	if (re)
	{
		if (re->re)
			(*pcre_free)(re->re);
		if (re->extra)
			(*pcre_free)(re->extra);
		if (re->ovector)
			g_free(re->ovector);
		g_free(re);
	}
}

static PcreInfo *
pcre_info_new (SearchExpression *s)
{
	PcreInfo *re;
	int options = 0;
	const char *err;
	int err_offset;
	int status;

	g_return_val_if_fail(s && s->search_str, NULL);
	re = g_new0(PcreInfo, 1);
	if (s->ignore_case)
		options |= PCRE_CASELESS;
	if (!s->greedy)
		options |= PCRE_UNGREEDY;
	re->re = pcre_compile(s->search_str, options, &err, &err_offset, NULL);
	if (NULL == re->re)
	{
		/* Compile failed - check error message */
		g_warning("Regex compile failed! %s at position %d", err, err_offset);
		pcre_info_free(re);
		return NULL;
	}
	re->extra = pcre_study(re->re, 0, &err);
	status = pcre_fullinfo(re->re, re->extra, PCRE_INFO_CAPTURECOUNT
	  , &(re->ovec_count));
	re->ovector = g_new0(int, 3 *(re->ovec_count + 1));
	return re;
}


static void match_substr_free(MatchSubStr *ms)
{
	if (ms)
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
		if (fb->path)
			g_free(fb->path);
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
	FileBuffer *fb;
	gchar* uri;
	
	g_return_val_if_fail(te, NULL);
	fb = g_new0(FileBuffer, 1);
	fb->type = FB_EDITOR;
	fb->te = te;
	
	uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
	if (uri)
		fb->path = tm_get_real_path(uri);
	fb->len = ianjuta_editor_get_length(te, NULL);
	fb->buf = ianjuta_editor_get_text(fb->te, 0, fb->len, NULL);
	fb->pos = ianjuta_editor_get_position(fb->te, NULL);
	fb->line = ianjuta_editor_get_lineno(fb->te, NULL);
	
	return fb;
}

FileBuffer *
file_buffer_new_from_path (const char *path, const char *buf, int len, int pos)
{
	FileBuffer *fb;
	IAnjutaEditor *te;
	char *real_path;
	int i;
	int lineno;

	g_return_val_if_fail(path, NULL);
	real_path = tm_get_real_path(path);
	
	/* There might be an already open TextEditor with this path */
	te = ianjuta_document_manager_find_editor_with_path (sr->docman, 
														 real_path, NULL);
	if (te)
	{
		g_free(real_path);
		return file_buffer_new_from_te(te);
	}
	fb = g_new0(FileBuffer, 1);
	fb->type = FB_FILE;
	fb->path = real_path;
	fb->name = strrchr(path, '/');
	if (fb->name)
		++ fb->name;
	else
		fb->name = fb->path;
	if (buf && len > 0)
	{
		fb->buf = g_new(char, len + 1);
		memcpy(fb->buf, buf, len);
		fb->buf[len] = '\0';
		fb->len = len;
	}
	else
	{
		struct stat s;

		if ((0 == stat(fb->path, &s)) && (S_ISREG(s.st_mode)))
		{
			if ((fb->len = s.st_size) < 0) return NULL;
			fb->buf = g_new(char, s.st_size + 1);
			{
				int total_bytes = 0, bytes_read, fd;
				if (0 > (fd = open(fb->path, O_RDONLY)))
				{
					perror(fb->path);
					file_buffer_free(fb);
					return NULL;
				}
				while (total_bytes < s.st_size)
				{
					if (0 > (bytes_read = read(fd, fb->buf + total_bytes
					  , s.st_size - total_bytes)))
					{
						perror(fb->path);
						close(fd);
						file_buffer_free(fb);
						return NULL;
					}
					total_bytes += bytes_read;
				}
				close(fd);
				fb->buf[fb->len] = '\0';
			}
		}
	}
	if (pos <= 0 || pos > fb->len)
	{
		fb->pos = 0;
		fb->line = 0;
	}
	else
	{
		fb->pos = pos;
		fb->line = 0;
	}
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
	return fb;
}

static long
file_buffer_line_from_pos(FileBuffer *fb, int pos)
{
	GList *tmp;
	int lineno = -1;
	g_return_val_if_fail(fb && pos >= 0, 1);
	if (FB_FILE == fb->type)
	{
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
		return ianjuta_editor_get_line_from_position(fb->te, pos, NULL);
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
	for (i= pos-1; (fb->buf[i] != '\n') && (i >= 0); i--, length++);
	
	return g_strndup (fb->buf + i + 1, length);
}

/* Generate a list of files to search in. Call with start = TRUE and
** top_dir = sf->top_dir. This is used when the search range is specified as
SR_FILES */
static GList *
create_search_files_list(SearchFiles *sf, const char *top_dir)
{
	TMFileEntry *entry;
	GList *files;

	g_return_val_if_fail(sf && top_dir, NULL);
	entry = tm_file_entry_new(top_dir, NULL, sf->recurse, sf->match_files
	  , sf->ignore_files, sf->match_dirs, sf->ignore_dirs
	  , sf->ignore_hidden_files, sf->ignore_hidden_dirs);
	if (!entry)
		return NULL;
	files = tm_file_entry_list(entry, NULL);
	tm_file_entry_free(entry);
	return files;
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
				gchar *file_path;
		
				uri = (const gchar *)node->data;
				file_path = gnome_vfs_get_local_path_from_uri (uri);
				if (file_path)
				files = g_list_prepend (files, file_path);
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
isawordchar (int c)
{
	return (isalnum(c) || '_' == c);
}

static gboolean
extra_match (FileBuffer *fb, SearchExpression *s, gint match_len)
{
	gchar b, e;
	
	b = fb->buf[fb->pos-1];
	e = fb->buf[fb->pos+match_len];
	
	if (s->whole_line)
		if ((fb->pos == 0 || b == '\n' || b == '\r') &&
			(e == '\0'	|| e == '\n' || e == '\r'))
			return TRUE;
		else
			return FALSE;
	else if (s->whole_word)
		if ((fb->pos ==0 || !isawordchar(b)) && 
			(e=='\0' || !isawordchar(e)))
			return TRUE;
		else
			return FALSE;
	else if (s->word_start)
		if (fb->pos ==0 || !isawordchar(b))
			return TRUE;
		else
			return FALSE;	
	else
		return TRUE;
}

/* Returns the next match in the passed buffer. The search expression should
** be pre-compiled. The returned pointer should be freed with match_info_free()
** when no longer required. */
MatchInfo *
get_next_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s)
{
	MatchInfo *mi = NULL;

	g_return_val_if_fail(fb && s, NULL);
	
	if (s->regex)
	{
		/* Regular expression match */
		int options = PCRE_NOTEMPTY;
		int status;
		if (NULL == s->re)
		{
			if (NULL == (s->re = pcre_info_new(s)))
				return NULL;
		}
		status = pcre_exec(s->re->re, s->re->extra, fb->buf, fb->len, fb->pos
		  , options, s->re->ovector, 3 * (s->re->ovec_count + 1));
		if (0 == status)
		{
			/* ovector too small - this should never happen ! */
			g_warning("BUG ! ovector found to be too small");
			return NULL;
		}
		else if (0 > status && status != PCRE_ERROR_NOMATCH)
		{
			/* match error - again, this should never happen */
			g_warning("PCRE Match error");
			return NULL;
		}
		else if (PCRE_ERROR_NOMATCH != status)
		{
			mi = g_new0(MatchInfo, 1);
			mi->pos = s->re->ovector[0];
			mi->len = s->re->ovector[1] - s->re->ovector[0];
			mi->line = file_buffer_line_from_pos(fb, mi->pos);
			if (status > 1) /* Captured subexpressions */
			{
				int i;
				MatchSubStr *ms;
				for (i=1; i < status; ++i)
				{
					ms = g_new0(MatchSubStr, 1);
					ms->start = s->re->ovector[i * 2];
					ms->len = s->re->ovector[i * 2 + 1] - ms->start;
					mi->subs = g_list_prepend(mi->subs, ms);
				}
				mi->subs = g_list_reverse(mi->subs);
			}
			fb->pos = s->re->ovector[1];
		}
	}
	else
	{
		/* Simple string search - this needs to be performance-tuned */
		int match_len = strlen(s->search_str);

		if (match_len == 0) return mi;

		if (SD_BACKWARD == direction)
		{
			/* Backward matching. */
			fb->pos -= match_len;
			if (fb->pos < 0)
				fb->pos = 0;
			if (s->ignore_case)
			{
				for (; fb->pos; -- fb->pos)
				{
					if (tolower(s->search_str[0]) == tolower(fb->buf[fb->pos]))
					{
						if (0 == g_strncasecmp(s->search_str, fb->buf + fb->pos
						  , match_len) &&  extra_match(fb, s, match_len))
						{
							mi = g_new0(MatchInfo, 1);
							mi->pos = fb->pos;
							mi->len = match_len;
							mi->line = file_buffer_line_from_pos(fb, mi->pos);
							return mi;
						}
					}
				}
			}
			else
			{
				for (; fb->pos; -- fb->pos)
				{
					if (s->search_str[0] == fb->buf[fb->pos])
					{
						if (0 == strncmp(s->search_str, fb->buf + fb->pos
						  , match_len) &&  extra_match(fb, s, match_len))
						{
							mi = g_new0(MatchInfo, 1);
							mi->pos = fb->pos;
							mi->len = match_len;
							mi->line = file_buffer_line_from_pos(fb, mi->pos);
							return mi;
						}
					}
				}
			}
		}
		else
		{
			/* Forward match */
			if (s->ignore_case)
			{
				for (; fb->pos < fb->len; ++ fb->pos)
				{
					if (tolower(s->search_str[0]) == tolower(fb->buf[fb->pos]))
					{
						if (0 == g_strncasecmp(s->search_str, fb->buf + fb->pos
						  , match_len) &&  extra_match(fb, s, match_len))
						{
							mi = g_new0(MatchInfo, 1);
							mi->pos = fb->pos;
							mi->len = match_len;
							mi->line = file_buffer_line_from_pos(fb, mi->pos);
							fb->pos += match_len;
							return mi;
						}
					}
				}
			}
			else
			{
				for (; fb->pos < fb->len; ++ fb->pos)
				{
					if (s->search_str[0] == fb->buf[fb->pos])
					{
						if (0 == strncmp(s->search_str, fb->buf + fb->pos
						  , match_len) &&  extra_match(fb, s, match_len))
						{
							mi = g_new0(MatchInfo, 1);
							mi->pos = fb->pos;
							mi->len = match_len;
							mi->line = file_buffer_line_from_pos(fb, mi->pos);
							fb->pos += match_len;
							return mi;
						}
					}
				}
			}
		}
	}
	return mi;
}

/* Create list of search entries */
GList
*create_search_entries(Search *s)
{
	GList *entries = NULL;
	GList *tmp;
	GList *editors;
	SearchEntry *se;
	long selstart;
	long tmp_pos;

	switch(s->range.type)
	{
		case SR_BUFFER:
			se = g_new0(SearchEntry, 1);
			se->type = SE_BUFFER;
			se->te = ianjuta_document_manager_get_current_editor (sr->docman, NULL);
			if (se->te != NULL)
			{
				se->direction = s->range.direction;
				if (SD_BEGINNING == se->direction)
				{
					se->start_pos = 0;
					se->end_pos = -1;
					se->direction = SD_FORWARD;
				}
				else
				{	
					selstart = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					se->start_pos = ianjuta_editor_get_position(se->te, NULL);
					if ((se->direction == SD_BACKWARD) && (selstart != se->start_pos))
						se->start_pos = selstart;						
					se->end_pos = -1;
				}
				entries = g_list_prepend(entries, se);
			}
			break;
		case SR_SELECTION:
		case SR_BLOCK:
		case SR_FUNCTION: 
			se = g_new0(SearchEntry, 1);
			se->type = SE_BUFFER;
			se->te = ianjuta_document_manager_get_current_editor (sr->docman, NULL);
			if (se->te != NULL)
			{
				gint sel_start = 0, sel_end = 0;
				
				if (s->range.type != SR_SELECTION)
				{
					sel_start = 
					sel_end = ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (se->te), NULL);
				}				
				se->direction = s->range.direction;
				if (s->range.type == SR_BLOCK)
					ianjuta_editor_selection_select_block(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				if (s->range.type == SR_FUNCTION)
					ianjuta_editor_selection_select_function(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				if (SD_BEGINNING == se->direction)
					se->direction = SD_FORWARD;
				se->start_pos = ianjuta_editor_selection_get_start(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				se->end_pos = ianjuta_editor_selection_get_end(IANJUTA_EDITOR_SELECTION (se->te), NULL);
			
				if (se->direction == SD_BACKWARD)
				{
					tmp_pos = se->start_pos;
					se->start_pos = se->end_pos;
					se->end_pos = tmp_pos;
				}	
				entries = g_list_prepend(entries, se);
				if (s->range.type != SR_SELECTION)
				{
					gboolean backward;
					backward = se->direction == SD_BACKWARD?TRUE:FALSE;
					ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (se->te), 
				                                 sel_start, sel_end, backward,
				                                 NULL);	
				}					
			}
			break;
		case SR_OPEN_BUFFERS:
			editors = ianjuta_document_manager_get_editors (sr->docman, NULL);
			for (tmp = editors; tmp; tmp = g_list_next(tmp))
			{
				se = g_new0(SearchEntry, 1);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR(tmp->data);
				se->direction = SD_FORWARD;
				se->start_pos = 0;
				se->end_pos = -1;
				entries = g_list_prepend(entries, se);
			}
			entries = g_list_reverse(entries);
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
			// FIXME : Replace Standard UNIX IO functions by gnome-vfs 
			if (dir_uri)			
				dir = gnome_vfs_get_local_path_from_uri(dir_uri);
			
			if (!dir)
			{
				if (SR_PROJECT == s->range.type)
					s->range.type = SR_FILES;
				dir = g_get_current_dir();
			}

			if (SR_FILES == s->range.type)
				files = create_search_files_list(&(s->range.files), dir);
			else /* if (SR_PROJECT == s->range.type) */
				files = get_project_file_list();	
			
			if (files)
			{
				for (tmp = files; tmp; tmp = g_list_next(tmp))
				{
					se = g_new0(SearchEntry, 1);
					se->type = SE_FILE;
					se->path = (char *) tmp->data;
					se->direction = SD_FORWARD;
					se->type = SE_FILE;
					se->start_pos = 0;
					se->end_pos = -1;
					entries = g_list_prepend(entries, se);
				}
				g_list_free(files);
				entries = g_list_reverse(entries);
			}
			g_free(dir);
			g_free(dir_uri);
			break;
		}
	}	
	return entries;		
}

gchar*
regex_backref(MatchInfo *mi, FileBuffer *fb)
{
	gint i, j, k;
	long start, len;
	gint nb_backref;
	gint i_backref;
	long backref[10] [2];
	static gchar buf[512];
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
	for(i=0, j=0; i < strlen(sr->replace.repl_str) && j < 512; i++)
	{
		if (sr->replace.repl_str[i] == '\\')
		{
			i++;
			if (sr->replace.repl_str[i] >= '0' && sr->replace.repl_str[i] <= '9')
			{
				i_backref = sr->replace.repl_str[i] - '0';
				if (i_backref != 0 && i_backref < nb_backref)
				{
					start = backref[i_backref] [0];
					len = backref[i_backref] [1];
					for (k=0; k < len; k++)
						buf[j++] = fb->buf[start + k];	
				}
			}	
		}	
		else
			buf[j++] = sr->replace.repl_str[i];				
	}
	buf[j] = '\0';

	return buf;
}

#define FREE_FN(fn, v) if (v) { fn(v); v = NULL; }

void
clear_search_replace_instance(void)
{
	g_free (sr->search.expr.search_str);
	g_free (sr->search.expr.re);
	FREE_FN(pcre_info_free, sr->search.expr.re);
	if (SR_FILES == sr->search.range.type)
	{
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_files);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_files);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_dirs);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_dirs);
	}
	g_free (sr->replace.repl_str);
}

void
clear_pcre(void)
{
	FREE_FN(pcre_info_free, sr->search.expr.re);
}

SearchReplace *
create_search_replace_instance(IAnjutaDocumentManager *docman)
{
	if (NULL == sr) /* Create a new SearchReplace instance */
		sr = g_new0(SearchReplace, 1);
	else
		clear_search_replace_instance ();
	if (docman)
		sr->docman = docman;
	return sr;
}

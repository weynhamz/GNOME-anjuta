/*
** search-replace.c: Generic Search and Replace
** Author: Biswapesh Chattopadhyay
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
#include <pcre.h>

#include "anjuta.h"
#include "text_editor.h"
#include "anjuta-tools.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "search-replace.h"

/* PCRE search compiled pattern and other info */
typedef struct _PcreInfo
{
	int ovec_count;;
	int *ovector;
	pcre *re;
	pcre_extra *extra;
} PcreInfo;

/* Search expression options */
typedef struct _SearchExpression
{
	char *search_str;
	gboolean regex;
	gboolean greedy;
	gboolean ignore_case;
	gboolean whole_word;
	gboolean whole_line;
	gboolean word_start;
	PcreInfo *re;
} SearchExpression;

static void pcre_info_free(PcreInfo *re)
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

static PcreInfo *pcre_info_new(SearchExpression *s)
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

/* Direction to search (only valid for the current buffer). Note that backward
search does not work for regular expressions. */
typedef enum _SearchDirection
{
	SD_FORWARD, /* Forward from the cursor */
	SD_BACKWARD, /* Backward from the cursor */
	SD_BEGINNING /* From the beginning of the buffer */
} SearchDirection;

/* Where to search - type */
typedef enum _SearchRangeType
{
	SR_BUFFER, /* Current buffer */
	SR_SELECTION, /* Selected text only */
	SR_BLOCK, /* Current block */
	SR_FUNCTION, /* Current function */
	SR_OPEN_BUFFERS, /* All open buffers */
	SR_PROJECT, /* All project files */
	SR_VARIABLE, /* A string specifying file names (expanded with prop_expand() */
	SR_FILES /* A set of patterns specifying which files to search */
} SearchRangeType;

/*
** Search variable is a string which is expanded using the properties interface
*/
typedef gchar *SearchVar;

/* Specify files to search in. Note that each GList is a list of strings which
can be fixed strings or contain globbing patterns (the type used by ls). */
typedef struct _SearchFiles
{
	gchar *top_dir;
	GList *match_files;
	GList *match_dirs;
	GList *ignore_files;
	GList *ignore_dirs;
	gboolean ignore_hidden_files;
	gboolean ignore_hidden_dirs;
	gboolean recurse;
} SearchFiles;

/* Search range - used to create search list of files */
typedef struct _SearchRange
{
	SearchRangeType type;
	SearchDirection direction; /* type = SR_BUFFER */
	SearchVar var; /* type = SR_VARIABLE */
	SearchFiles files; /* type = SR_FILES */
} SearchRange;

/* What to do with the result of the search */
typedef enum _SearchAction
{
	SA_SELECT, /* Jump to the first match and select it (current buffer only)*/
	SA_FIND_PANE, /* Show result in find pane */
	SA_BOOKMARK, /* Bookmark the line (open buffers only) */
	SA_HIGHLIGHT, /* Highlight matched string (open buffers only) */
	SA_REPLACE, /* Replace first match with specified string */
	SA_REPLACEALL /* Replace all matches with specified string */
} SearchAction;

/* Search master option structure */
typedef struct _Search
{
	SearchExpression expr;
	SearchRange range;
	SearchAction action;
} Search;

/* Contains information about replacement */
typedef struct _Replace
{
	gchar *repl_str;
	gboolean regex;
	gboolean confirm;
	gboolean load_file;
} Replace;

typedef struct _SearchReplace
{
	Search search;
	Replace replace;
} SearchReplace;

static SearchReplace *sr = NULL;

/* Information about a matched substring */
typedef struct _MatchSubStr
{
	long start;
	long len;
} MatchSubStr;

static void match_substr_free(MatchSubStr *ms)
{
	if (ms)
		g_free(ms);
}

typedef struct _MatchInfo
{
	long pos;
	long len;
	long line;
	GList *subs; /* <MatchSubStr *> */
} MatchInfo;

static void match_info_free(MatchInfo *mi)
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

typedef enum _FileBufferType
{
	FB_NONE,
	FB_FILE, /* File loaded from disk */
	FB_EDITOR /* Corresponding to a TextEditor structure */
} FileBufferType;

typedef struct _FileBuffer
{
	FileBufferType type;
	/* The following are valid only for files loaded from disk */
	char *name; /* Name of the file */
	char *path; /* Full path to the file */
	char *buf; /* COntents of the file */
	long len; /* Length of the buffer */
	long pos; /* Current position */
	long endpos; /* Restrict action upto this position */
	long line; /* Current line */
	GList *lines; /* List of integers specifying line start positions */
	/* The following are valid only for files corresponding to a TextEditor */
	TextEditor *te;
} FileBuffer;

static void file_buffer_free(FileBuffer *fb)
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
static FileBuffer *file_buffer_new_from_te(TextEditor *te)
{
	FileBuffer *fb;

	g_return_val_if_fail(te, NULL);
	fb = g_new0(FileBuffer, 1);
	fb->type = FB_EDITOR;
	fb->te = te;
	if (te->full_filename)
		fb->path = tm_get_real_path(te->full_filename);
	fb->len = scintilla_send_message(SCINTILLA(fb->te->widgets.editor)
		  , SCI_GETLENGTH, 0, 0);
	fb->buf = g_new(char, fb->len + 1);
	scintilla_send_message(SCINTILLA(fb->te->widgets.editor)
		  , SCI_GETTEXT, fb->len + 1, (long) fb->buf);
	fb->pos = 	scintilla_send_message(SCINTILLA(fb->te->widgets.editor)
		  , SCI_GETCURRENTPOS, 0, 0);
	fb->line = 	scintilla_send_message(SCINTILLA(fb->te->widgets.editor)
		  , SCI_GETCURLINE, 0, 0);
	return fb;
}

static FileBuffer *file_buffer_new_from_path(const char *path, const char *buf
  , int len, int pos)
{
	FileBuffer *fb;
	TextEditor *te;
	GList *tmp;
	char *real_path;
	int i;
	int lineno;

	g_return_val_if_fail(path, NULL);
	real_path = tm_get_real_path(path);
	/* There might be an already open TextEditor with this path */
	for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
	{
		te = (TextEditor *) tmp->data;
		if (te->full_filename && 0 == strcmp(real_path, te->full_filename))
		{
			g_free(real_path);
			return file_buffer_new_from_te(te);
		}
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
			fb->len = s.st_size;
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

static long file_buffer_line_from_pos(FileBuffer *fb, int pos)
{
	GList *tmp;
	int lineno = 0;
	g_return_val_if_fail(fb && pos >= 0, 1);
	if (FB_FILE == fb->type)
	{
		for (tmp = fb->lines; tmp; tmp = g_list_next(tmp))
		{
			if (pos >= GPOINTER_TO_INT(tmp->data))
				return lineno;
			++ lineno;
		}
		return lineno;
	}
	else if (FB_EDITOR == fb->type)
		return 	scintilla_send_message(SCINTILLA(fb->te->widgets.editor)
		  , SCI_LINEFROMPOSITION, pos, 0);
	else
		return -1;
}

/* Generate a list of files to search in. Call with start = TRUE and
** top_dir = sf->top_dir. This is used when the search range is specified as
SR_FILES */
static GList *create_search_files_list(SearchFiles *sf, const char *top_dir)
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

/* Create a list of files to search in from a user specified list. Variables
should be expanded so that the user can use strings like:
'$(module.source.files)', '$(module.include.files)', etc. */
static GList *expand_search_file_list(const char *top_dir, const char *str)
{
	gchar *dir;
	GList *names;
	GList *files = NULL;
	GList *tmp;
	char path[PATH_MAX];
	struct stat s;
	gchar *files_str = prop_expand(app->project_dbase->props, str);
	if (top_dir)
		dir = prop_expand(app->project_dbase->props, top_dir);
	else if (app->project_dbase->project_is_open)
		dir = g_strdup(app->project_dbase->top_proj_dir);
	else
		dir = g_strdup(".");
	names = glist_from_string(files_str);
	for (tmp = names; tmp; tmp = g_list_next(tmp))
	{
		snprintf(path, PATH_MAX, "%s/%s", dir, (char *) tmp->data);
		if ((0 == stat(path, &s)) && (S_ISREG(s.st_mode)))
		{
			files = g_list_prepend(files, tm_get_real_path(path));
		}
	}
	glist_strings_free(names);
	g_free(files_str);
	g_free(dir);
	files = g_list_reverse(files);
	return files;
}

/* Get a list of all project files */
static GList *get_project_file_list(void)
{
	GList *files = NULL;

	if (app->project_dbase->project_is_open)
	{
		GList *names = NULL;
		GList *tmp;
		struct stat s;
		gchar module_file_var[128];
		char path[PATH_MAX];
		int i;

		for (i=0; i < MODULE_END_MARK; ++i)
		{
			snprintf(module_file_var, 128, "module.%s.files", module_map[i]);
			names = g_list_concat(names, glist_from_data(
			  app->project_dbase->props, module_file_var));
		}
		for (tmp = names; tmp; tmp = g_list_next(tmp))
		{
			snprintf(path, PATH_MAX, "%s/%s", app->project_dbase->top_proj_dir
			  , (char *) tmp->data);
			if ((0 == stat(path, &s)) && (S_ISREG(s.st_mode)))
				files = g_list_prepend(files, tm_get_real_path(path));
		}
		glist_strings_free(names);
		files = g_list_reverse(files);
	}
	return files;
}

/* Returns the next match in the passed buffer. The search expression should
** be pre-compiled. The returned pointer should be freed with match_info_free()
** when no longer required. */
static MatchInfo *get_next_match(FileBuffer *fb, SearchDirection direction
  , SearchExpression *s)
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
		else if (0 > status)
		{
			/* match error - again, this should never happen */
			g_warning("PCRE Match error");
			return NULL;
		}
		else
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
					ms->start = s->re->ovector[(i-1) * 2];
					ms->len = s->re->ovector[(i-i) * 2 + 1] - ms->start;
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
						  , match_len))
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
						  , match_len))
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
						  , match_len))
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
						  , match_len))
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

/* A search entry is a file or buffer to search. This can be a file,
a buffer or part of a buffer (such as selected text) */
typedef enum _SearchEntryType
{
	SE_FILE,
	SE_BUFFER,
} SearchEntryType;

typedef struct _SearchEntry
{
	SearchEntryType type;
	char *path;
	TextEditor *te;
	SearchDirection direction;
	long start_pos;
	long end_pos;
} SearchEntry;

/* Create list of search entries */
static GList *create_search_entries(Search *s)
{
	GList *entries = NULL;
	GList *files;
	GList *tmp;
	SearchEntry *se;
	char *dir;
	long selstart;
	long tmp_pos;
	
	switch(s->range.type)
	{
		case SR_BUFFER:
		case SR_BLOCK: /* FIXME */
		case SR_FUNCTION: /* FIXME */
			se = g_new0(SearchEntry, 1);
			se->type = SE_BUFFER;
			if (se->te == app->current_text_editor)
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
					selstart = scintilla_send_message(SCINTILLA(
							  se->te->widgets.editor), SCI_GETSELECTIONSTART,0,0);
					se->start_pos = scintilla_send_message(SCINTILLA(
			  		se->te->widgets.editor), SCI_GETCURRENTPOS, 0, 0);
					if ((se->direction == SD_BACKWARD) && (selstart != se->start_pos))
						se->start_pos = selstart;						
					se->end_pos = -1;
				}
				entries = g_list_prepend(entries, se);
			}
			break;
		case SR_SELECTION:
			se = g_new0(SearchEntry, 1);
			se->type = SE_BUFFER;
			if (se->te == app->current_text_editor)
			{
				se->direction = s->range.direction;
				if (SD_BEGINNING == se->direction)
					se->direction = SD_FORWARD;
				se->start_pos = scintilla_send_message(SCINTILLA(
			  		se->te->widgets.editor), SCI_GETSELECTIONSTART, 0, 0);
				se->end_pos = scintilla_send_message(SCINTILLA(
			  		se->te->widgets.editor), SCI_GETSELECTIONEND, 0, 0);	
				if (se->direction == SD_BACKWARD)
				{
					tmp_pos = se->start_pos;
					se->start_pos = se->end_pos;
					se->end_pos = tmp_pos;
				}	
				entries = g_list_prepend(entries, se);				
			}
			break;
		case SR_OPEN_BUFFERS:
			for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
			{
				se = g_new0(SearchEntry, 1);
				se->type = SE_BUFFER;
				se->te = (TextEditor *) tmp->data;
				se->direction = SD_FORWARD;
				se->start_pos = 0;
				se->end_pos = -1;
				entries = g_list_prepend(entries, se);
			}
			entries = g_list_reverse(entries);
			break;
		case SR_FILES:
		case SR_VARIABLE:
		case SR_PROJECT:
			if (!app->project_dbase->top_proj_dir)
				dir = g_get_current_dir();
			else
				dir = g_strdup(app->project_dbase->top_proj_dir);
			if (SR_FILES == s->range.type)
				files = create_search_files_list(&(s->range.files), dir);
			else if (SR_VARIABLE == s->range.type)
				files = expand_search_file_list(dir, s->range.var);
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
					se->start_pos = 0;
					se->end_pos = -1;
					entries = g_list_prepend(entries, se);
				}
				g_list_free(files);
				entries = g_list_reverse(entries);
			}
			g_free(dir);
			break;
	}
	return entries;
}

static void search_and_replace(void)
{
	GList *entries;
	GList *tmp;
	char buf[BUFSIZ];
	SearchEntry *se;
	FileBuffer *fb;
	MatchInfo *mi;
	Search *s;
	gint offset;

	g_return_if_fail(sr);
	s = &(sr->search);
	entries = create_search_entries(s);
	if (SA_FIND_PANE == s->action)
	{
		anjuta_message_manager_clear(app->messages, MESSAGE_FIND);
		anjuta_message_manager_show(app->messages, MESSAGE_FIND);
	}
	for (tmp = entries; tmp; tmp = g_list_next(tmp))
	{
		se = (SearchEntry *) tmp->data;
		if (SE_BUFFER == se->type)
			fb = file_buffer_new_from_te(se->te);
		else /* if (SE_FILE == se->type) */
			fb = file_buffer_new_from_path(se->path, NULL, -1, 0);
		if (fb)
		{
			fb->pos = se->start_pos;
			offset = 0;
			while ((NULL != (mi = get_next_match(fb, se->direction, &(s->expr)))))
			{
				if ((s->range.direction == SD_BACKWARD) && (mi->pos < se->end_pos))
						break; 
				if ((s->range.direction != SD_BACKWARD) && ((se->end_pos != -1) &&
					(mi->pos+mi->len > se->end_pos)))
						break; 
				
				switch (s->action)
				{
					case SA_BOOKMARK:
					case SA_HIGHLIGHT: /* FIXME */
						if (fb->te)
						{
							scintilla_send_message(SCINTILLA(
							  fb->te->widgets.editor), SCI_MARKERADD
							  , mi->line, 0);
						}
						break;
					case SA_SELECT:
						if (fb->te)
						{
							scintilla_send_message(SCINTILLA(
							  fb->te->widgets.editor), SCI_SETSEL, mi->pos
							  , (mi->pos + mi->len));
						}
						break;
					case SA_FIND_PANE: /* FIXME - add the line */
						snprintf(buf, BUFSIZ, "%s:%ld:Match found\n", fb->path
						  , mi->line + 1);
						anjuta_message_manager_append(app->messages, buf
						  , MESSAGE_FIND);
						break;
					case SA_REPLACE:
					case SA_REPLACEALL:
						if (fb->te)
						{
							scintilla_send_message(SCINTILLA(
							  fb->te->widgets.editor), SCI_SETSEL, mi->pos - offset
							  , (mi->pos + mi->len - offset));
							scintilla_send_message(SCINTILLA(
							  fb->te->widgets.editor), SCI_REPLACESEL, 0
							  ,(long) sr->replace.repl_str); 
							if (s->range.direction != SD_BACKWARD)							
								offset += mi->len - strlen(sr->replace.repl_str ) ;
						}
						break;
					default:
						break;
				}
				match_info_free(mi);
				if (SA_SELECT == s->action || SA_REPLACE == s->action)
					break;
			}
		}
		file_buffer_free(fb);
		g_free(se);
	}
	g_list_free(entries);
	return;
}

/* The GUI part starts here */

#define GLADE_FILE "anjuta.glade"
#define SEARCH_REPLACE_DIALOG "dialog.search.replace"
#define CLOSE_BUTTON "button.close"
#define HELP_BUTTON "button.help"
#define SAVE_BUTTON "button.save"
#define SEARCH_BUTTON "button.next"

/* Frames */
#define SEARCH_EXPR_FRAME "frame.search.expression"
#define SEARCH_TARGET_FRAME "frame.search.target"
#define SEARCH_VAR_FRAME "frame.search.var"
#define FILE_FILTER_FRAME "frame.file.filter"
#define REPLACE_FRAME "frame.replace"

/* Entries */
#define SEARCH_STRING "search.string"
#define SEARCH_TARGET "search.target"
#define SEARCH_ACTION "search.action"
#define SEARCH_VAR "search.var"
#define MATCH_FILES "file.filter.match"
#define UNMATCH_FILES "file.filter.unmatch"
#define MATCH_DIRS "dir.filter.match"
#define UNMATCH_DIRS "dir.filter.unmatch"
#define REPLACE_STRING "replace.string"
#define SEARCH_DIRECION "search.direction"

/* Checkboxes */
#define SEARCH_REGEX "search.regex"
#define GREEDY "search.greedy"
#define IGNORE_CASE "search.ignore.case"
#define WHOLE_WORD "search.match.whole.word"
#define WORD_START "search.match.word.start"
#define WHOLE_LINE "search.match.whole.line"
#define IGNORE_HIDDEN_FILES "ignore.hidden.files"
#define IGNORE_BINARY_FILES "ignore.binary.files"
#define IGNORE_HIDDEN_DIRS "ignore.hidden.dirs"
#define SEARCH_RECURSIVE "search.dir.recursive"
#define REPLACE_REGEX "replace.regex"

/* Combo boxes */
#define SEARCH_STRING_COMBO "search.string.combo"
#define SEARCH_TARGET_COMBO "search.target.combo"
#define SEARCH_ACTION_COMBO "search.action.combo"
#define SEARCH_VAR_COMBO "search.var.combo"
#define MATCH_FILES_COMBO "file.filter.match.combo"
#define UNMATCH_FILES_COMBO "file.filter.unmatch.combo"
#define MATCH_DIRS_COMBO "dir.filter.match.combo"
#define UNMATCH_DIRS_COMBO "dir.filter.unmatch.combo"
#define REPLACE_STRING_COMBO "replace.string.combo"
#define SEARCH_DIRECION_COMBO "search.direction.combo"

/* GUI dropdown option strings */
StringMap search_direction_strings[] = {
	{SD_FORWARD, "Forward"},
	{SD_BACKWARD, "Backward"},
	{SD_BEGINNING, "Full Buffer"},
	{-1, NULL}
};

StringMap search_target_strings[] = {
	{SR_BUFFER, "Current Buffer"},
	{SR_SELECTION,"Current Selection"},
	{SR_BLOCK, "Current Block"},
	{SR_FUNCTION, "Current Function"},
	{SR_OPEN_BUFFERS, "All Open Buffers"},
	{SR_PROJECT, "All Project Files"},
	{SR_VARIABLE, "Specify File List"},
	{SR_FILES, "Specify File Patterns"},
	{-1, NULL}
};

StringMap search_action_strings[] = {
	{SA_SELECT, "Select the first match"},
	{SA_BOOKMARK, "Bookmark all matched lines"},
	{SA_HIGHLIGHT, "Mark all matched strings"},
	{SA_FIND_PANE, "Show result in find pane"},
	{SA_REPLACE, "Replace first match"},
	{SA_REPLACEALL, "Replace all matches"},
	{-1, NULL}
};

typedef enum _GUIElementType
{
	GE_NONE,
	GE_BUTTON,
	GE_TEXT,
	GE_BOOLEAN,
	GE_OPTION,
	GE_COMBO
} GUIElementType;

typedef struct _GladeWidget
{
	GUIElementType type;
	char *name;
	gpointer extra;
	GtkWidget *widget;
} GladeWidget;

static GladeWidget glade_widgets[] = {
	{GE_NONE, SEARCH_REPLACE_DIALOG, NULL, NULL},
	{GE_BUTTON, CLOSE_BUTTON, NULL, NULL},
	{GE_BUTTON, HELP_BUTTON, NULL, NULL},
	{GE_BUTTON, SAVE_BUTTON, NULL, NULL},
	{GE_BUTTON, SEARCH_BUTTON, NULL, NULL},
	{GE_NONE, SEARCH_EXPR_FRAME, NULL, NULL},
	{GE_NONE, SEARCH_TARGET_FRAME, NULL, NULL},
	{GE_NONE, SEARCH_VAR_FRAME, NULL, NULL},
	{GE_NONE, FILE_FILTER_FRAME, NULL, NULL},
	{GE_NONE, REPLACE_FRAME, NULL, NULL},
	{GE_TEXT, SEARCH_STRING, NULL, NULL},
	{GE_OPTION, SEARCH_TARGET, search_target_strings, NULL},
	{GE_OPTION, SEARCH_ACTION, search_action_strings, NULL},
	{GE_TEXT, SEARCH_VAR, NULL, NULL},
	{GE_TEXT, MATCH_FILES, NULL, NULL},
	{GE_TEXT, UNMATCH_FILES, NULL, NULL},
	{GE_TEXT, MATCH_DIRS, NULL, NULL},
	{GE_TEXT, UNMATCH_DIRS, NULL, NULL},
	{GE_TEXT, REPLACE_STRING, NULL, NULL},
	{GE_OPTION, SEARCH_DIRECION, search_direction_strings, NULL},
	{GE_BOOLEAN, SEARCH_REGEX, NULL, NULL},
	{GE_BOOLEAN, GREEDY, NULL, NULL},
	{GE_BOOLEAN, IGNORE_CASE, NULL, NULL},
	{GE_BOOLEAN, WHOLE_WORD, NULL, NULL},
	{GE_BOOLEAN, WORD_START, NULL, NULL},
	{GE_BOOLEAN, WHOLE_LINE, NULL, NULL},
	{GE_BOOLEAN, IGNORE_HIDDEN_FILES, NULL, NULL},
	{GE_BOOLEAN, IGNORE_BINARY_FILES, NULL, NULL},
	{GE_BOOLEAN, IGNORE_HIDDEN_DIRS, NULL, NULL},
	{GE_BOOLEAN, SEARCH_RECURSIVE, NULL, NULL},
	{GE_TEXT, REPLACE_REGEX, NULL, NULL},
	{GE_COMBO, SEARCH_STRING_COMBO, NULL, NULL},
	{GE_COMBO, SEARCH_TARGET_COMBO, search_target_strings, NULL},
	{GE_COMBO, SEARCH_ACTION_COMBO, search_action_strings, NULL},
	{GE_COMBO, SEARCH_VAR_COMBO, NULL, NULL},
	{GE_COMBO, MATCH_FILES_COMBO, NULL, NULL},
	{GE_COMBO, UNMATCH_FILES_COMBO, NULL, NULL},
	{GE_COMBO, MATCH_DIRS_COMBO, NULL, NULL},
	{GE_COMBO, UNMATCH_DIRS_COMBO, NULL, NULL},
	{GE_COMBO, REPLACE_STRING_COMBO, NULL, NULL},
	{GE_COMBO, SEARCH_DIRECION_COMBO, search_direction_strings, NULL},
	{GE_NONE, NULL, NULL, NULL}
};

static void populate_value(const char *name, gpointer val_ptr)
{
	StringMap *map;
	GladeWidget *gw = NULL;
	char *s;
	int i;

	g_return_if_fail(name && val_ptr);
	for (i=0; NULL != glade_widgets[i].name; ++i)
	{
		if (0 == strcmp(glade_widgets[i].name, name))
		{
			gw = & (glade_widgets[i]);
			break;
		}
	}
	g_return_if_fail(gw);
	switch(gw->type)
	{
		case GE_TEXT:
			if (*((char **) val_ptr))
				g_free(* ((char **) val_ptr));
			*((char **) val_ptr) = gtk_editable_get_chars(
			  GTK_EDITABLE(gw->widget), 0, -1);
			break;
		case GE_BOOLEAN:
			* ((gboolean *) val_ptr) = gtk_toggle_button_get_active(
			  GTK_TOGGLE_BUTTON(gw->widget));
			break;
		case GE_OPTION:
			map = (StringMap *) gw->extra;
			g_return_if_fail(map);
			s = gtk_editable_get_chars(GTK_EDITABLE(gw->widget), 0, -1);
			*((int *) val_ptr) = type_from_string(map, s);
			FREE(s);
			break;
		default:
			g_warning("Bad option %d to populate_value", gw->type);
			break;
	}
}

typedef struct _SearchReplaceGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean showing;
} SearchReplaceGUI;

static SearchReplaceGUI *sg = NULL;

/* Callbacks */
gboolean on_search_replace_delete_event(GtkWidget *window, GdkEvent *event
  , gboolean user_data)
{
	if (sg->showing)
	{
		gtk_widget_hide(sg->dialog);
		sg->showing = FALSE;
	}
	return TRUE;
}

void on_search_action_changed(GtkEditable *editable, gpointer user_data)
{
	GtkWidget *replace_frame = glade_xml_get_widget(sg->xml, REPLACE_FRAME);
	GtkWidget *action_button = glade_xml_get_widget(sg->xml, SEARCH_BUTTON);
	char *s;
	SearchAction act;

	s = gtk_editable_get_chars(editable, 0, -1);
	act = type_from_string(search_action_strings, s);
	switch(act)
	{
		case SA_REPLACE:
		case SA_REPLACEALL:
			gtk_widget_show(replace_frame);
			gtk_label_set_text(GTK_LABEL(GTK_BIN(action_button)->child)
			  , (act == SA_REPLACE) ? "Replace" : "Replace All");
			break;
		default:
			gtk_widget_hide(replace_frame);
			gtk_label_set_text(GTK_LABEL(GTK_BIN(action_button)->child)
			  , "Search");
			break;
	}
}

void on_search_target_changed(GtkEditable *editable, gpointer user_data)
{
	SearchRangeType type;
	char *s;
	GtkWidget *search_var_frame = glade_xml_get_widget(sg->xml, SEARCH_VAR_FRAME);
	GtkWidget *file_filter_frame = glade_xml_get_widget(sg->xml, FILE_FILTER_FRAME);

	s = gtk_editable_get_chars(editable, 0, -1);
	type = type_from_string(search_target_strings, s);
	g_free(s);
	switch(type)
	{
		case SR_FILES:
			gtk_widget_hide(search_var_frame);
			gtk_widget_show(file_filter_frame);
			break;
		case SR_VARIABLE:
			gtk_widget_show(search_var_frame);
			gtk_widget_hide(file_filter_frame);
			break;
		default:
			gtk_widget_hide(search_var_frame);
			gtk_widget_hide(file_filter_frame);
			break;
	}
}

void on_search_button_close_clicked(GtkButton *button, gpointer user_data)
{
	if (sg->showing)
	{
		/* Save current search items */
		gtk_widget_hide(sg->dialog);
		sg->showing = FALSE;
	}
}

void on_search_button_help_clicked(GtkButton *button, gpointer user_data)
{
	anjuta_tools_show_variables();
}

#define FREE_FN(fn, v) if (v) { fn(v); v = NULL; }
#define POP_LIST(str, var) populate_value(str, &s); \
			if (s) \
			{ \
				sr->search.range.files.var = glist_from_string(s); \
				g_free(s); \
			}

static void search_replace_populate(void)
{
	char *s;

	if (NULL == sr) /* Create a new SearchReplace instance */
	{
		sr = g_new0(SearchReplace, 1);
	}
	else /* Cleanup the old instance to prevent memory leaks */
	{
		FREE(sr->search.expr.search_str);
		FREE(sr->search.expr.re);
		FREE_FN(pcre_info_free, sr->search.expr.re);
		if (SR_VARIABLE == sr->search.range.type)
		{
			FREE(sr->search.range.var);
		}
		else if (SR_FILES == sr->search.range.type)
		{
			FREE_FN(glist_strings_free, sr->search.range.files.match_files);
			FREE_FN(glist_strings_free, sr->search.range.files.ignore_files);
			FREE_FN(glist_strings_free, sr->search.range.files.match_dirs);
			FREE_FN(glist_strings_free, sr->search.range.files.ignore_dirs);
		}
		FREE(sr->replace.repl_str);
	}
	/* Now, populate the instance with values from the GUI */
	populate_value(SEARCH_STRING, &(sr->search.expr.search_str));
	populate_value(SEARCH_REGEX, &(sr->search.expr.regex));
	populate_value(GREEDY, &(sr->search.expr.greedy));
	populate_value(IGNORE_CASE, &(sr->search.expr.ignore_case));
	populate_value(WHOLE_WORD, &(sr->search.expr.whole_word));
	populate_value(WHOLE_LINE, &(sr->search.expr.whole_line));
	populate_value(WORD_START, &(sr->search.expr.word_start));
	populate_value(SEARCH_TARGET, &(sr->search.range.type));
	populate_value(SEARCH_DIRECION, &(sr->search.range.direction));
	switch (sr->search.range.type)
	{
		case SR_VARIABLE:
			populate_value(SEARCH_VAR, &(sr->search.range.var));
			break;
		case SR_FILES:
			POP_LIST(MATCH_FILES, match_files);
			POP_LIST(UNMATCH_FILES, ignore_files);
			POP_LIST(MATCH_DIRS, match_dirs);
			POP_LIST(UNMATCH_DIRS, ignore_files);
			break;
		default:
			break;
	}
	populate_value(SEARCH_ACTION, &(sr->search.action));
	switch (sr->search.action)
	{
		case SA_REPLACE:
		case SA_REPLACEALL:
			populate_value(REPLACE_STRING, &(sr->replace.repl_str));
			populate_value(REPLACE_REGEX, &(sr->replace.regex));
			break;
		default:
			break;
	}
}

void on_search_button_next_clicked(GtkButton *button, gpointer user_data)
{
	search_replace_populate();
	search_and_replace();
}

void on_search_button_save_clicked(GtkButton *button, gpointer user_data)
{
}

static gboolean create_dialog(void)
{
	char glade_file[PATH_MAX];
	GladeWidget *w;
	GList *combo_strings;
	int i;

	g_return_val_if_fail(NULL == sr, FALSE);
	sg = g_new0(SearchReplaceGUI, 1);
	snprintf(glade_file, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, GLADE_FILE);
	if (NULL == (sg->xml = glade_xml_new(glade_file
      , SEARCH_REPLACE_DIALOG)))
	{
		anjuta_error(_("Unable to build user interface for Search And Replace"));
		g_free(sg);
		sg = NULL;
		return FALSE;
	}
	sg->dialog = glade_xml_get_widget(sg->xml, SEARCH_REPLACE_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW(sg->dialog)
	  , GTK_WINDOW(app->widgets.window));

	for (i=0; NULL != glade_widgets[i].name; ++i)
	{
		w = &(glade_widgets[i]);
		w->widget = glade_xml_get_widget(sg->xml, w->name);
		gtk_widget_ref(w->widget);
		if (GE_COMBO == w->type && NULL != w->extra)
		{
			combo_strings = glist_from_map((StringMap *) w->extra);
			gtk_combo_set_popdown_strings(GTK_COMBO(w->widget), combo_strings);
			g_list_free(combo_strings);
		}
	}
	glade_xml_signal_autoconnect(sg->xml);
	return TRUE;
}

static void show_dialog()
{
	if (!sg->showing)
	{
		gtk_widget_show(sg->dialog);
		sg->showing = TRUE;
	}
}

void anjuta_search_replace_activate(void)
{
	if (NULL == sr)
	{
		if (! create_dialog())
			return;
    }
	/* Set properties */
	
	/* Show the dialog */
	show_dialog();
}

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
 
#ifndef _SEARCH_REPLACE_BACKEND_H
#define _SEARCH_REPLACE_BACKEND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <glib/gi18n.h>

/* Search expression options */
typedef struct _SearchExpression
{
	gchar *search_str;
	gboolean regex;
	gboolean greedy;
	gboolean match_case;
	gboolean whole_word;
	gboolean whole_line;
	gboolean word_start;
	gboolean no_limit;
	gint actions_max;
	GRegex* regex_info;
} SearchExpression;


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
	gboolean whole;
	SearchVar var; /* type = SR_VARIABLE */
	SearchFiles files; /* type = SR_FILES */
} SearchRange;

/* What to do with the result of the search */
typedef enum _SearchAction
{
	SA_SELECT, /* Jump to the next match and select it (current buffer only)*/
	SA_BOOKMARK, /* Bookmark all lines containing a match (open buffers only) */
	SA_HIGHLIGHT, /* Highlight all matched strings (open buffers only) */
	SA_FIND_PANE, /* Show result in find pane */
	SA_REPLACE, /* Replace next match with specified string */
	SA_REPLACEALL /* Replace all matches with specified string */
} SearchAction;

/* Search master option structure */
typedef struct _Search
{
	SearchExpression expr;
	SearchRange range;
	SearchAction action;
	GList *expr_history;
	gint incremental_pos;
	gboolean incremental_wrap;
	gboolean basic_search;
} Search;

/* Contains information about replacement */
typedef struct _Replace
{
	gchar *repl_str;
	gboolean regex;
	gboolean confirm;
	gboolean load_file;
	GList *expr_history;
} Replace;

typedef struct _SearchReplace
{
	Search search;
	Replace replace;
	IAnjutaDocumentManager *docman;
	
} SearchReplace;

typedef enum _FileBufferType
{
	FB_NONE,
	FB_FILE, /* File loaded from disk */
	FB_EDITOR /* Corresponding to a TextEditor structure */
} FileBufferType;

typedef struct _FileBuffer
{
	FileBufferType type;

	GFile *file;			/* GFile object */

	gchar *buf;		/* Contents of the file, null-terminated */
	gsize len;			/* Length of the buffer */

	/* Current position: a count of bytes, *not* UTF-8 characters. */
	gint pos; 

	gint line; 			/* Current line */
	GList *lines; 		/* List of integers specifying line start positions in bytes */

	gchar *canonical;   /* buffer text converted to canonical utf-8 form */
	gchar *canonical_p; /* current pointer into canonical text */

	/* The following are valid only for files corresponding to a TextEditor */
	IAnjutaEditor *te;

} FileBuffer;


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
	gchar *uri;
	IAnjutaEditor *te;
	SearchDirection direction;
	gint start_pos;		/* Both position in characters */
	gint end_pos;
} SearchEntry;
	
typedef struct _MatchInfo
{
	gint pos;			/* in bytes */
	gint len;			/* in bytes */
	gint line;
	GList *subs; /* <MatchSubStr *> */
} MatchInfo;


//  void search_and_replace_backend_init (AnjutaDocman *dm);

void function_select (IAnjutaEditor *te);

GList *create_search_entries(Search *s);

void free_search_entries (GList *entries);

FileBuffer *file_buffer_new_from_te (IAnjutaEditor *te);

FileBuffer *file_buffer_new_from_uri(const gchar *uri);

gchar *file_match_line_from_pos(FileBuffer *fb, gint pos);

MatchInfo *get_next_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s);

gchar *regex_backref(MatchInfo *mi, FileBuffer *fb);

void match_info_free (MatchInfo *mi);

void file_buffer_free (FileBuffer *fb);

SearchReplace *create_search_replace_instance(IAnjutaDocumentManager *docman);

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */

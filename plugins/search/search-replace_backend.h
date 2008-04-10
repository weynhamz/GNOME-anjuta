/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * search-replace_backend.h: Header for Generic Search and Replace
 * Copyright (C) 2004 Biswapesh Chattopadhyay
 * Copyright (C) 2004-2007 Naba Kumar  <naba@gnome.org>
 *
 * This file is part of anjuta.
 * Anjuta is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with anjuta; if not, contact the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _SEARCH_REPLACE_BACKEND_H
#define _SEARCH_REPLACE_BACKEND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pcre.h>
#include <glade/glade.h>

#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#include "plugin.h"

/* FIXME make these global */
typedef gint line_t;
typedef gint position_t;
#define POINTER_TO_LINE GPOINTER_TO_INT
#define LINE_TO_POINTER GINT_TO_POINTER
#define POINTER_TO_POSITION GPOINTER_TO_INT
#define POSITION_TO_POINTER GINT_TO_POINTER

/* define to include code for managing EOL chars in local file buffers */
/*#define MANAGE_EOL */

/* PCRE search compiled pattern and other info */
typedef struct _PcreInfo
{
	gint ovec_count;
	position_t *ovector;	/* note pcre always uses gint */
	pcre *re;
	pcre_extra *extra;
} PcreInfo;

/* Flags for type of position data available from a search-process */
typedef enum _SearchPosType
{
	SP_UNKNOWN	= 0,
	SP_BYTES	= 1,
	SP_CHARS	= 2,
	/* rest are unused ATM*/
	SP_GINT		= 4,
	SP_GUINT	= 8,
	SP_GLONG	= 16,
	SP_GULONG	= 32,
	SP_GLLONG	= 64,
	SP_GULLONG	= 128
} SearchPosType;

/* Search expression options */
typedef struct _SearchExpression
{
	gchar *search_str;
	gboolean regex;
	gboolean utf8regex;	/* TRUE when pcre compiled with utf-8 support */
	gboolean greedy;
	gboolean ignore_case;
	gboolean whole_word;
	gboolean whole_line;
	gboolean word_start;
	gboolean no_limit;
	gint actions_max;
	PcreInfo *re;
	SearchPosType postype;
} SearchExpression;

/* Direction to search (only valid for the current buffer).
   Note that backward search does not work for regular expressions. */
typedef enum _SearchDirection
{
	SD_WHOLE, /* From the beginning of the buffer|selection|function|block (0 == default) */
	SD_FORWARD, /* Forward from the cursor */
	SD_BACKWARD /* Backward from the cursor */
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
//	SR_VARIABLE,	/* A nominated list of files */
	SR_FILES	/* A series of patterns specifying which files to search */
} SearchRangeType;

/*
 * Search variable is a string which is expanded using the properties interface
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
	gboolean recurse;
	gboolean ignore_hidden_dirs;
	gboolean ignore_hidden_files;
//	gboolean ignore_binary_files;
} SearchFiles;

/* search range - used to create search list of files */
typedef struct _SearchRange
{
	SearchRangeType target;
	SearchDirection direction; /* type = SR_BUFFER */
	gboolean whole;
	SearchVar var; /* type = SR_VARIABLE */
	SearchFiles files; /* type = SR_FILES */
} SearchRange;

/* What s/r operation to perform */
typedef enum _SearchAction
{
	SA_SELECT,		/* find the next/previos match and select it */
	SA_BOOKMARK,	/* bookmark all lines containing a match */
	SA_HIGHLIGHT,	/* mark/highlight all matched strings */
	SA_UNLIGHT,		/* remove all highlights */
	SA_FIND_PANE,	/* show result in find pane */
	SA_REPLACE,		/* replace all with a check for each match */
	SA_REPLACEALL	/* replace all matches without confirmation */
} SearchAction;

typedef enum _ReplacePhase
{
	SA_REPL_FIRST,	/* beginning an interactive replace operation */
	SA_REPL_SKIP,	/* during a replace operation, the user has decided to skip (keep) the current match */
	SA_REPL_CONFIRM,/* during a replace operation, the user has decided to replace the current match */
} ReplacePhase;

/* search/replace master data structure */
typedef struct _Search
{
	SearchExpression expr;
	SearchRange range;
	SearchAction action;
	GList *expr_history;
	position_t incremental_pos;	//want chars not bytes
	gboolean incremental_wrap;
	gboolean basic_search;
	gboolean limited; /* TRUE when processing: block, function, selection */
	gint stop_count;		/* count of stop-button clicks, or < 0 to abort operation */
	/* non-file-specific data that needs to persist between phases of an interactive replacement */
	GList *candidates;		/* list of SeacrchEntry's */
	guint matches_sofar;	/* count of matches processed */
	gboolean busy;			/* TRUE to block sr data changes (when a search is in progress
								(a friendlier approach than de-sensitizing most widgets) */
} Search;

/* Contains information about replacement */
typedef struct _Replace
{
	gchar *repl_str;
	gboolean regex;
	gboolean confirm;
	gboolean load_file;
//	gboolean interactive;	/* TRUE for the 2nd phase of a confirmed replacement */
	ReplacePhase phase;		/* == SA_REPL_CONFIRM for the 2nd (replacement) phase of a replacement */
	GList *expr_history;
} Replace;

typedef struct _MatchInfo
{
	position_t pos;	/* char- or byte-index (as neeeded by the editor) of match start */
	position_t len;	/* char- or byte-length (as neeeded by the editor) of match */
	line_t line;	/* 1-based no. of line in file-buffer containing start of match */
	GList *subs;	/* list of <MatchSubStr *> sub-expression/backref data, if any */
} MatchInfo;

typedef struct _SearchReplace
{
	Search search;
	Replace replace;
	struct _SearchReplaceGUI *sg;	/* for cross-referencing */
	IAnjutaDocumentManager *docman;
} SearchReplace;

typedef enum _FileBufferType
{
	FB_NONE,
	FB_FILE,	/* File not yet opened */
	FB_BUFFER	/* File already open */
} FileBufferType;

/* this conforms to scintilla:
#define SC_EOL_CRLF 0
#define SC_EOL_CR 1
#define SC_EOL_LF 2
*/
typedef enum _EOLType
{
	EOL_UNKNOWN = -1,
	EOL_CRLF,
	EOL_CR,
	EOL_LF,
	EOL_UNI, //unicode, GDK_paragraph,  0x0b6, scintilla should use SC_EOL_LF ?
} EOLType;

typedef struct _FileBuffer
{
	FileBufferType type;
	/* Some of the following are valid only for files loaded from disk */
//UNUSED	const gchar *name;	/* name of the file, typically part of uri */
	gchar *uri;			/* file  uri (may be NULL) */
	gchar *encoding;	/* for fresh files */
#ifdef MANAGE_EOL
	EOLType separator_type;	/* for fresh files */
#endif
	gchar *buf;			/* contents of the file - free after use */
	position_t len;		/* total chars (NOT bytes) in the buffer */
	/* pos and endpos are char-offsets, NOT byte-offsets */
	position_t start_pos;/* char-position to start a search */
	position_t end_pos;	/* char-position to end a search, may be -1 for EOF or else always > start_pos */
	line_t line;		/* Current line, 1-based index */
	GList *lines;		/* list of EOLdata's specifying line-end byte-offsets and
						   char-offsets (actually, they are offsets of '\n' chars)
						   THe first member will be {0,0} implying position before
						   start of text */
	/* The following are valid only for files corresponding to a TextEditor */
	IAnjutaEditor *te;
} FileBuffer;

typedef enum _SearchEntryType
{
	SE_FILE,
	SE_BUFFER,
} SearchEntryType;

/* A search entry holds data about a search. It can apply to a file not opened,
   or to an open buffer or part of a buffer (such as selected text) */
typedef struct _SearchEntry
{
	SearchEntryType type;	/* SE_BUFFER for item already open, else SE_FILE */
	gchar *uri;				/* NULL for SE_BUFFER, canonical uri for SE_FILE */
	IAnjutaEditor *te;		/* non-NULL for SE_BUFFER, NULL for SE_FILE */
	FileBuffer *fb;			/* for retaining buffer between phases of a SA_REPLACE */
	gboolean fresh;			/* for retaining status */
	SearchDirection direction;
	/* start_pos and end_pos are char-offsets, NOT byte-offsets */
	position_t start_pos;	/* char-offset where to start the search (> end_pos for SD_BACKWARD direction) */
	position_t end_pos;		/* char-offset where to end the search (-1 for EOF) */
	/* data which needs to persist between phases of a confirmed replacement */
	MatchInfo mi;
	gchar *regx_repl_str;
	line_t found_line;
	/* selection-position often changes during a search, so these remember its initial position */
	position_t sel_first_start;
	position_t sel_first_end;
//	position_t offset;			/ * differential length for current (maybe each) replacement, bytes or chars * /
	position_t total_offset;	/* cumulative difference between searched buffer
									and edited buffer with replacement(s),
									bytes or chars to suit the editor */
} SearchEntry;

void create_search_entries (SearchReplace *sr);
void clear_search_entries (GList **entries);
void editor_new_from_file_buffer (SearchEntry *se);
FileBuffer *file_buffer_new_from_te (IAnjutaEditor *te);
FileBuffer *file_buffer_new_from_uri (SearchReplace *sr,
									  const gchar *uri,
									  const gchar *buf,
									  position_t len);
void file_buffer_freshen_lines_from_pos (FileBuffer *fb,
										position_t offset,
										gboolean offset_bytes);
position_t file_buffer_get_byte_offset (FileBuffer *fb, position_t charoffset);
position_t file_buffer_get_char_offset (FileBuffer *fb, position_t byteoffset);
//gchar *file_buffer_get_linetext_for_pos (FileBuffer *fb, position_t pos);
gchar *file_buffer_get_linetext_for_line (FileBuffer *fb, line_t lineno);
gboolean get_next_match (FileBuffer *fb,
						 SearchDirection direction,
						 SearchExpression *se,
						 MatchInfo *mi);
gboolean save_file_buffer (FileBuffer *fb);
gboolean replace_in_local_buffer (FileBuffer *fb,
								  position_t matchstart,
								  position_t matchlen,
								  gchar *repl_str);
gchar *regex_backref (SearchReplace *sr, MatchInfo *mi, FileBuffer *fb);
void match_info_free_subs (MatchInfo *mi);
void file_buffer_free (FileBuffer *fb);
SearchReplace *search_replace_data_new (void);
SearchReplace *search_get_default_data (void);
void search_replace_init (AnjutaPlugin *plugin);
void search_replace_data_destroy (SearchReplace *sr);
void pcre_info_free (PcreInfo *re);

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_BACKEND_H */

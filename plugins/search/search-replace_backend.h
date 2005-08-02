#ifndef _SEARCH_REPLACE_BACKEND_H
#define _SEARCH_REPLACE_BACKEND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pcre.h>

#include "anjuta-docman.h"

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
	gboolean no_limit;
	gint actions_max;
	PcreInfo *re;
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
	SA_SELECT, /* Jump to the first match and select it (current buffer only)*/
	SA_BOOKMARK, /* Bookmark the line (open buffers only) */
	SA_HIGHLIGHT, /* Highlight matched string (open buffers only) */
	SA_FIND_PANE, /* Show result in find pane */
	SA_REPLACE, /* Replace first match with specified string */
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
	AnjutaDocman *docman;
	
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
	
typedef struct _MatchInfo
{
	long pos;
	long len;
	long line;
	GList *subs; /* <MatchSubStr *> */
} MatchInfo;


//  void search_and_replace_backend_init (AnjutaDocman *dm);

void function_select (TextEditor *te);

GList *create_search_entries(Search *s);

FileBuffer *file_buffer_new_from_te (TextEditor *te);

FileBuffer *
file_buffer_new_from_path(const char *path, const char *buf, int len, int pos);

FileBuffer *
file_buffer_new_from_path(const char *path, const char *buf, int len, int pos);

gchar *file_match_line_from_pos(FileBuffer *fb, int pos);

MatchInfo *get_next_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s);

gchar *regex_backref(MatchInfo *mi, FileBuffer *fb);

void match_info_free (MatchInfo *mi);

void file_buffer_free (FileBuffer *fb);

SearchReplace *create_search_replace_instance(AnjutaDocman *docman);

void clear_pcre(void);

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */

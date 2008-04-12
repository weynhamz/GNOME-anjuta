/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * search-replace_backend.c: Generic Search and Replace
 * Copyright (C) 2004 Biswapesh Chattopadhyay
 * Copyright (C) 2004-2007 Naba Kumar  <naba@gnome.org>
 *
 * This file is part of anjuta.
 * Anjuta is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with anjuta; if not, contact the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-line-mode.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

#include "search-replace_backend.h"
#include "tm_tagmanager.h"

/* Information about a matched substring in a regex search */
typedef struct _MatchSubStr
{
	position_t start;	/* relates to pcre ovector (gint), bytes or chars depending on utf8 support */
	position_t len;		/* ditto */
} MatchSubStr;
/* Information about an EOL in a file buffer (note - 2 pointers are enough for a slice */
typedef struct _EOLdata
{
	position_t offb;	/* byte-offset of EOL */
	position_t offc;	/* char-offset of EOL */
} EOLdata;

#define REGX_BACKCOUNT 10
#define REGX_BUFSIZE 1024

static SearchPlugin *splugin;			/* the plugin pointer set when plugin was activated */
static SearchReplace *def_sr = NULL;	/* session-static sr data, typically used
										   with the single "main" search dialog */

//static gboolean search_locally (SearchExpression *se,
//								  SearchDirection direction,
//								  position_t hlen);
static void file_buffer_find_lines (FileBuffer *fb, GList *startpos);
//static void file_buffer_set_byte_length (FileBuffer *fb, position_t bytelength);
static void clear_search_replace_instance (SearchReplace *sr);


/**
 * regex_backref:
 * @sr: pointer to populated search/replace data struct
 * @mi: pointer to match info data for the match being processed
 * @fb: pointer to file-buffer data for file being processed
 *
 * Construct a replacement string from regex patterns with back reference(s)
 *
 * Return value: the newly-allocated string
 */
gchar *
regex_backref (SearchReplace *sr, MatchInfo *mi, FileBuffer *fb)
{
	gint i, j, k;
	gint nb_backref;
	gint i_backref;
	gint plen;
	position_t start, len;
	position_t backref [REGX_BACKCOUNT][2];	/* contains byte-positions and -lengths */
	gchar buf [REGX_BUFSIZE + 4];	/* usable space + word-sized space for trailing 0 */
	GList *node;

	/* get easier access to back reference data */
	for (node = mi->subs, i = 1;	/* \0 is not supported */
		node != NULL && i < REGX_BACKCOUNT;
		node = g_list_next (node), i++)
 	{
		backref[i][0] = ((MatchSubStr*)node->data)->start;
		backref[i][1] = ((MatchSubStr*)node->data)->len;
	}
	nb_backref = i;
	plen = strlen (sr->replace.repl_str);
	for (i = 0, j = 0; i < plen && j < REGX_BUFSIZE; i++)
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
					for (k = 0; k < len && j < REGX_BUFSIZE; k++)
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

/**
 * pcre_info_free:
 * @re: pointer to re data struct to clear, may be NULL
 *
 * Return value: none
 */
void
pcre_info_free (PcreInfo *re)
{
	if (re != NULL)
	{
		if (re->re)
			(*pcre_free)(re->re);
		if (re->extra)
			(*pcre_free)(re->extra);
		if (re->ovector)
			g_free (re->ovector);
		g_slice_free1 (sizeof (PcreInfo), re);
	}
}

/**
 * pcre_info_new:
 * @se:
 *
 * Compile and setup data for regex search/replace
 *
 * Return value: pcre data struct
 */
static PcreInfo *
pcre_info_new (SearchExpression *se)
{
	PcreInfo *re;
	gint options;
	const gchar *err;
	gint err_offset;	//CHECKME type position_t
	gint status;

	g_return_val_if_fail (se && se->search_str, NULL);
	re = g_slice_new0 (PcreInfo);
	options = PCRE_NEWLINE_ANYCRLF;	//or PCRE_NEWLINE_ANY ?
	if (se->ignore_case)
		options |= PCRE_CASELESS;
	if (!se->greedy)
		options |= PCRE_UNGREEDY;
	if (se->utf8regex)
		options |= PCRE_UTF8|PCRE_NO_UTF8_CHECK;
	re->re = pcre_compile (se->search_str, options, &err, &err_offset, NULL);
	if (re->re == NULL)
	{
		/* Compile failed - check error message */
		g_warning("Regex compile failed! %s at position %d", err, err_offset);
		pcre_info_free (re);
		return NULL;
	}
	re->extra = pcre_study (re->re, 0, &err);
	status = pcre_fullinfo (re->re, re->extra, PCRE_INFO_CAPTURECOUNT,
							&(re->ovec_count));
	re->ovector = g_new0 (position_t, 3 *(re->ovec_count + 1));	/* pcre uses gint */
	return re;
}

static void
match_substr_free (MatchSubStr *ms)
{
	if (ms)
		g_slice_free1 (sizeof (MatchSubStr), ms);
}

/**
 * match_info_free_subs:
 * @mi:
 *
 * Clear list of regex replacment backref data referenced in @mi
 *
 * Return value: None
 */
void
match_info_free_subs (MatchInfo *mi)
{
	if (mi)
	{
		if (mi->subs)
		{
			GList *tmp;
			for (tmp = mi->subs; tmp; tmp = g_list_next(tmp))
				match_substr_free ((MatchSubStr *) tmp->data);
			g_list_free (mi->subs);
		}
	}
}

/**
 * editor_new_from_file_buffer:
 * @te: pointer to file buffer data struct for a not-open file
 *
 * Add a new editor to docman, and set it up according to @fb data
 *
 * Return value: None, but failure will be indicated by fb->te == NULL
 */
void
editor_new_from_file_buffer (SearchEntry *se)
{
	FileBuffer *fb;

	fb = se->fb;
	/* CHECKME se->uri must be an escaped uri string such as returned by
		gnome_vfs_get_uri_from_local_path() */
	fb->te = ianjuta_document_manager_goto_uri_line (splugin->docman,
													 se->uri,
													 se->mi.line, NULL);
	g_return_if_fail (fb->te);

	/* CHECKME can we safely omit the rest ?*/
//	g_free (fb->buf);
//	fb->buf =
	gchar *newtext = ianjuta_editor_get_text_all (fb->te, NULL);
	if (strcmp (newtext, fb->buf) == 0)
		DEBUG_PRINT ("File buffer matches editor text");
	else
		DEBUG_PRINT ("File buffer DOPES NOT MATCH editor text");
	g_free (fb->buf);
	fb->buf = newtext;

	fb->line = ianjuta_editor_get_lineno (fb->te, NULL);
#ifdef MANAGE_EOL
	fb->separator_type = EOL_UNKNOWN;	/* log the EOL-type when finding lines */
#endif
	file_buffer_find_lines (fb, NULL);	/* get all line-ends data for file */
	fb->len = file_buffer_get_char_offset (fb, -1);	/* character-length */
}

/**
 * file_buffer_new_from_te:
 * @te: pointer to text editor data struct for an upen file
 *
 * Create a file buffer structure for an already-opened file
 * The file text is copied into a newly-allocated buffer. This should already
 * be encoded as UTF-8 by the normal opening process
 *
 * Return value: the new data struct
 */
FileBuffer *
file_buffer_new_from_te (IAnjutaEditor *te)
{
	FileBuffer *fb;
	gchar *uri;

	g_return_val_if_fail (te, NULL);
	fb = g_slice_new0 (FileBuffer);
	fb->type = FB_BUFFER;
	fb->te = te;

	uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
	if (uri)
	{
		//CHECKME any escaping or other cleanup ?
		fb->uri = gnome_vfs_make_uri_canonical (uri);
		if (fb->uri == NULL)
			fb->uri = uri;
		else
			g_free (uri);
	}
	/*CHECKME is NULL uri ok if uri was not found or conversion failed ?
	else
		fb->uri = ?; */
	/* specific text length N/A as length returned by editor may be chars, not bytes */
	/* can't yet efficiently check whether a local buffer is needed, as such check
	   includes the byte-length of the text to search */
/*	position_t hlen = length reported by editor
	if editor uses char-positions, convert length to bytes
	BUT line-ends not yet parsed
	if (search_locally (SearchExpression *se, SearchDirection direction, position_t hlen)) */
		fb->buf = ianjuta_editor_get_text_all (fb->te, NULL);
	fb->line = ianjuta_editor_get_lineno (fb->te, NULL);
	/* don't log the EOL-type when finding lines, in this context */
	file_buffer_find_lines (fb, NULL);	/* get all line-ends data for file */
	fb->len = file_buffer_get_char_offset (fb, -1);	/* character-length */

	return fb;
}

/**
 * file_buffer_new_from_uri:
 * @sr: pointer to populated search/replace data struct
 * @uri: uri of file to open
 * @buf: buffer holding current contents of the file, or NULL to read the file
 * @len: byte-size of non-NULL @buf, or -1 to get length of 0-terminated @buf
 *
 * Create and populate a file buffer for file with path @path.
 * The file may be opened already.
 * Symlinks in @path are reconciled.
 * The contents of non-NULL @buf are copied.
 * If the file is not open before, its contents are converted to UTF-8 if needed.
 *
 * Return value: the new data struct, or NULL upon error
 */
FileBuffer *
file_buffer_new_from_uri (SearchReplace *sr,
						  const gchar *uri,
						  const gchar *buf,
						  position_t len)
{
	FileBuffer *fb;
	GnomeVFSURI *vfs_uri;
	GnomeVFSHandle *vfs_read;
	GnomeVFSResult result;
	GnomeVFSFileInfo info;
	GnomeVFSFileSize nchars;
	gchar *buffer;
	gchar *converted;
	gchar *encoding = NULL;

	g_return_val_if_fail (uri, NULL);

	vfs_uri = gnome_vfs_uri_new (uri);

	if (gnome_vfs_uri_is_local (vfs_uri))
	{
		result = gnome_vfs_get_file_info_uri (vfs_uri, &info, GNOME_VFS_FILE_INFO_DEFAULT);
		if (result != GNOME_VFS_OK)
		{
			//FIXME warn user
			gnome_vfs_uri_unref (vfs_uri);
//			perror (path);
			return NULL;
		}

		result = gnome_vfs_open_uri (&vfs_read, vfs_uri, GNOME_VFS_OPEN_READ);
		if (result != GNOME_VFS_OK)
		{
			//FIXME warn user
			gnome_vfs_uri_unref (vfs_uri);
//			perror (path);
			return NULL;
		}

		buffer = g_try_malloc (info.size + 1);
		if (buffer == NULL && info.size != 0)
		{
			/* DEBUG_PRINT ("This file is too big. Unable to allocate memory."); */
			gnome_vfs_close (vfs_read);
			gnome_vfs_uri_unref (vfs_uri);
//			perror (path);
			return NULL;
		}

		result = gnome_vfs_read (vfs_read, buffer, info.size, &nchars);
		if (result != GNOME_VFS_OK && !(result == GNOME_VFS_ERROR_EOF && info.size == 0))
		{
			//FIXME warn user
			g_free (buffer);
			gnome_vfs_close (vfs_read);
			gnome_vfs_uri_unref (vfs_uri);
//			perror (path);
			return NULL;
		}

		gnome_vfs_close (vfs_read);
		gnome_vfs_uri_unref (vfs_uri);

		*(buffer + info.size) = '\0';
	}
	else /* not a local uri*/
	{
	/* FIXME do async read, with gtk_main() until the completion callback */
		g_return_val_if_reached (NULL);
/*
		GnomeVFSAsyncHandle *handle;

		gnome_vfs_async_open_uri (&handle,
								  vfs_uri,
								  GNOME_VFS_OPEN_READ,
								  GNOME_VFS_PRIORITY_MAX,
								  (GnomeVFSAsyncOpenCallback) async_open_callback,
								  fb);
		gtk_main ();
*/
	}

	if (info.size > 0)
	{
		GError *error;
		/*  FIXME need to tell scintilla about EOL type
			convert_to_utf8 (te->props_base, fb->buf, info.size, &te->encoding);
		*/
		error = NULL;
		/* needs 0-terminated buffer */
		converted = buffer; /*anjuta_convert_to_utf8 (buffer,
											(gsize) info.size,
											&encoding,
											NULL,
											&error);*/
		if (error != NULL)
		{
			gchar *msg;
			msg = g_strdup_printf (_("UTF-8 conversion failed for %s: %s"),
									uri, error->message);
			perror (msg);
			g_free (msg);
			g_error_free (error);
			g_free (buffer);
			if (converted != NULL)
				g_free (converted);
			return NULL;
		}

		if (converted != buffer)
			g_free (buffer);
	}
	else	/* empty file */
	{
		converted = buffer;
		encoding = NULL;	//no way to determine what this really is
	}

	fb = g_slice_new0 (FileBuffer);
	fb->type = FB_FILE;
	fb->buf = converted;
	fb->encoding = encoding;
	//CHECKME any escaping or other cleanup ?
	fb->uri = gnome_vfs_make_uri_canonical (uri);
/*	CHECKME is fb->name used anywhere? does it need to be utf-8 ?
	fb->name = strrchr (fb->uri, '/');
	if (fb->name)
		++ fb->name;
	else
		fb->name = ?;
*/
	/* fb->len (char-length) is set upstream, after the line-ends are located */
	fb->start_pos = 0;
	fb->line = 0;	/* signal we want to find the current cursor position whne doing line-ends */
#ifdef MANAGE_EOL
	fb->separator_type = EOL_UNKNOWN;	/* log the EOL-type when finding lines */
#endif
	file_buffer_find_lines (fb, NULL);	/* get all line-ends data for buffer */
	fb->len = file_buffer_get_char_offset (fb, -1);	/* character-length */

	return fb;
}

/**
 * file_buffer_free:
 * @fb:
 *
 * Clear all data for @fb
 *
 * Return value: None
 */
void
file_buffer_free (FileBuffer *fb)
{
	if (fb)
	{
		if (fb->uri)
			g_free (fb->uri);
		if (fb->encoding)
			g_free (fb->encoding);
		if (fb->buf)
			g_free (fb->buf);
		/* the editor fb->te is removed from docman elsewhere, if it's not wanted */
		if (fb->lines)
		{
			GList *node;
			for (node = fb->lines; node != NULL; node = g_list_next (node))
				g_slice_free1 (sizeof (EOLdata), node->data);
			g_list_free (fb->lines);
		}
		g_slice_free1 (sizeof (FileBuffer), fb);
	}
}

/* *
 * file_buffer_set_byte_length:
 * @fb: pointer to file-buffer data for file being processed
 * @bytelength: length of file buffer
 *
 * Return value: None
 */
/*static void
file_buffer_set_byte_length (FileBuffer *fb, position_t bytelength)
{
	GList *node;

	node = g_list_last (fb->lines);
	((EOLdata *)node->data)->offb = bytelength;
}
*/
/**
 * file_buffer_get_byte_offset:
 * @fb: pointer to file-buffer data for file being processed
 * @charoffset: offset into file buffer
 *
 * Get byte-offset corresonding to @charoffset in file associated with @fb
 * This can handle fb->len offset which is past EOF
 *
 * Return value: The offset, or -1 after error
 */
position_t
file_buffer_get_byte_offset (FileBuffer *fb, position_t charoffset)
{
	GList *node;

	g_return_val_if_fail (fb->lines, -1);

	if (charoffset > 0)
	{
		/* skip 1st entry with all-0 data */
		for (node = g_list_next (fb->lines); node != NULL; node = g_list_next (node))
		{
			EOLdata *ed;

			ed = (EOLdata *)node->data;
			if (charoffset < ed->offc)
			{
				gchar *tmp;
				position_t priorb, priorc;

				node = g_list_previous (node);
				priorb = ((EOLdata *)node->data)->offb;
				priorc = ((EOLdata *)node->data)->offc;
				tmp = g_utf8_offset_to_pointer (fb->buf + priorb,
												charoffset - priorc);
				return (tmp - fb->buf);
			}
			else if (charoffset == ed->offc)
				return ((EOLdata *)node->data)->offb;
		}
		return -1;
	}
	else if (charoffset == 0)
		return 0;
	else /* charoffset < 0 */
	{
		node = g_list_last (fb->lines);
		return (((EOLdata *)node->data)->offb);
	}
}

/**
 * file_buffer_get_char_offset:
 * @fb: pointer to file-buffer data for file being processed
 * @byteoffset: offset into file buffer
 *
 * Get character-offset corresonding to @byteoffset in file associated with @fb
 * This can handle offset == strlen(file) which is past EOF, or offset == -1
 *
 * Return value: the offset
 */
position_t
file_buffer_get_char_offset (FileBuffer *fb, position_t byteoffset)
{
	GList *node;

	g_return_val_if_fail (fb->lines, -1);

	if (byteoffset > 0)
	{
		/* skip 1st entry with all-0 data */
		for (node = g_list_next (fb->lines); node != NULL; node = g_list_next (node))
		{
			EOLdata *ed;

			ed = (EOLdata *)node->data;
			if (byteoffset < ed->offb)
			{
				position_t indx, priorb;

				node = g_list_previous (node);
				priorb = ((EOLdata *)node->data)->offb;
				indx = (position_t) g_utf8_pointer_to_offset (fb->buf + priorb, fb->buf + byteoffset);
				return (((EOLdata *)node->data)->offc + indx);
			}
			else if (byteoffset == ed->offb)
				return ((EOLdata *)node->data)->offc;
		}
		return -1;
	}
	else if (byteoffset == 0)
		return 0;
	else /* byteoffset < 0 */
	{
		node = g_list_last (fb->lines);
		return (((EOLdata *)node->data)->offc);
	}
}

/**
 * file_buffer_get_char_offset_pair:
 * @fb: pointer to file-buffer data for file being processed
 * @byteoffset1: pointer to store for offset into file buffer
 * @byteoffset2: pointer to store for offset into file buffer, or NULL
 *
 * Get character-offsets corresonding to @byteoffset1 and @byteoffset2 in file
 * associated with @fb
 * This is a bit faster than finding 2 values separately
 *
 * Return value: None
 */
static void
file_buffer_get_char_offset_pair (FileBuffer *fb,
								 position_t *byteoffset1,
								 position_t *byteoffset2)
{
	position_t real_start, real_end;
	GList *node;

	g_return_if_fail (byteoffset1 && byteoffset2);

	if (byteoffset2 == NULL)
	{
		real_start = *byteoffset1;
		*byteoffset1 = file_buffer_get_char_offset (fb, real_start);
		return;
	}
	else if (byteoffset1 == NULL)
	{
		real_end = *byteoffset2;
		*byteoffset2 = file_buffer_get_char_offset (fb, real_end);
		return;
	}
	else if (*byteoffset2 > *byteoffset1)
	{
		if (*byteoffset1 != -1)
		{
			real_start = *byteoffset1;
			real_end = *byteoffset2;
		}
		else
		{
			real_start = *byteoffset2;
			real_end = *byteoffset1;
		}
	}
	else if (*byteoffset2 < *byteoffset1)
	{
		if (*byteoffset2 != -1)
		{
			real_start = *byteoffset2;
			real_end = *byteoffset1;
		}
		else
		{
			real_start = *byteoffset1;
			real_end = *byteoffset2;
		}
	}
	else /* *byteoffset2 == *byteoffset1 */
	{
		real_start = *byteoffset1;
		*byteoffset1 = file_buffer_get_char_offset (fb, real_start);
		*byteoffset2 = *byteoffset1;
		return;
	}

	/* skip 1st entry with all-0 data */
	for (node = g_list_next (fb->lines); node != NULL; node = g_list_next (node))
	{
		EOLdata *ed;

		ed = (EOLdata *)node->data;
		if (real_start <= ed->offb)
		{
			position_t indx, priorb;

			if (real_start < ed->offb)
			{
				node = g_list_previous (node);
				priorb = ((EOLdata *)node->data)->offb;
				indx = (position_t) g_utf8_pointer_to_offset (fb->buf + priorb, fb->buf + real_start);
				real_start = (((EOLdata *)node->data)->offc + indx); /* remember the start */
			}
			else
				real_start = ((EOLdata *)node->data)->offc;

			for (node = g_list_next (node); node != NULL; node = g_list_next (node))
			{
				ed = (EOLdata *)node->data;
				if (real_end <= ed->offb)
				{
					if (real_end < ed->offb)
					{
						node = g_list_previous (node);
						priorb = ((EOLdata *)node->data)->offb;
						indx = (position_t) g_utf8_pointer_to_offset (fb->buf + priorb, fb->buf + real_end);
						real_end = (((EOLdata *)node->data)->offc + indx); /* remember the end */
					}
					else
						real_end = ((EOLdata *)node->data)->offc;
					break;
				}
			}
			if (node == NULL)
			{
				real_end = -1;
				node = fb->lines;	/* prevent further assignment */
			}
			break;
		}
	}
	if (node == NULL)
		real_start = -1;

	if (*byteoffset1 < *byteoffset2)
	{
		*byteoffset1 = real_start;
		*byteoffset2 = real_end;
	}
	else
	{
		*byteoffset2 = real_start;
		*byteoffset1 = real_end;
	}
}

/**
 * file_buffer_find_lines:
 * @fb: pointer to file-buffer data for file being processed
 * @startpos: pointer to first list-member to be updated, or NULL for whole list
 *
 * Populate list of line-end offsets for whole or part of file associated with @fb
 * NOTE ATM also sets fb->line value if fb->line == 0 upon arrival here
 * The list length is one bigger than the no. of lines in the file.
 * The first member of the list represents the start of the file (positions {0,0}),
 * so that each other member's index represents the corresonding 1-based line no.
 * Except that the last member represents the end of the file, with byte- and
 * char-lengths of the file.
 * This also records the "majority" EOL format in the buffer
 *
 * Return value: None
 */
static void
file_buffer_find_lines (FileBuffer *fb, GList *startpos)
{
	position_t indx, current, priorb, priorc;
	line_t lineno;
	EOLdata *ed;
	gchar *buf, *this;
	GList *node;
#ifdef MANAGE_EOL
	guint cr, lf, crlf, unicr;
#endif

	g_return_if_fail (fb->buf);

	if (fb->lines != NULL && startpos == NULL)
	{
doall:
		for (node = fb->lines; node != NULL; node = g_list_next (node))
			g_slice_free1 (sizeof (EOLdata), node->data);
		g_list_free (fb->lines);
		fb->lines = NULL;
	}

	if (fb->lines == NULL || fb->lines == startpos || startpos == NULL)
	{
		/* (re-)processing whole file buffer */
		/* First line data corresponds to start of buffer */
		ed = g_slice_new0 (EOLdata);
		fb->lines = g_list_prepend (fb->lines, ed);
		priorb = priorc = 0;
		lineno = 1;
	}
	else if ((lineno = g_list_position (fb->lines, startpos)) != -1)
	{
		/* update trailing part of list */
		if (lineno == 0)
			goto doall;
		for (node = startpos; node != NULL; node = g_list_next (node))
		{
			g_slice_free1 (sizeof (EOLdata), node->data);
			node->data = NULL;
		}
//		fb->lines = g_list_remove_all (fb->lines, NULL);
		g_list_previous (startpos)->next = NULL;
		g_list_free (startpos);
		fb->lines = g_list_reverse (fb->lines);
		node = g_list_first (fb->lines);
		priorb = ((EOLdata*)node->data)->offb;
		priorc = ((EOLdata*)node->data)->offc;
		lineno++;	/* adjust for 1st member of list */
	}
	else
	{
		//FIXME UI warning
		//g_return_if_reached ();
		g_warning ("Error in line-cacheing for %s", fb->uri);
		return;
	}

#ifdef MANAGE_EOL
	cr = lf = crlf = unicr = 0;
#endif
	buf = fb->buf;	/* for faster lookup */
	current = (fb->line == 0) ?
		(position_t) g_utf8_pointer_to_offset (buf, buf + fb->start_pos):	/* can't yet work from lines data */
		0;	/* 0 for warning prevention */

	for (indx = priorb, this = buf + priorb; ; indx++, this++)
	{
		guchar c;

		c = *this;
		//CHECKME assumes when EOL == \r\n, can simply ignore the \n
		//CHECKME can any utf-8 char include \r or \n byte(s) ?
		if (c == '\n' || c == '\r' || c == 0xb6 || c == 0)
		{
			ed = g_slice_new (EOLdata);
			ed->offb = indx;

			ed->offc = priorc + g_utf8_strlen (buf + priorb, indx - priorb);
			priorb = indx;
			priorc = ed->offc;

			if (fb->line == 0 && current > indx)
				fb->line = lineno;	/* current position is in this line */
			fb->lines = g_list_prepend (fb->lines, ed);
			if (c != 0)
			{
#ifdef MANAGE_EOL
				switch (c)
				{
					case '\n':
						lf++;
						break;
					case '\r':
						if (*(this+1) != '\n')
							cr++;
						else
						{
							crlf++;
							indx++;
							this++;
							ed->offb++;
							ed->offc++;
						}
						break;
					case 0xb6:
						unicr++;
					default:
						break;
				}
#endif
				lineno++;
			}
			else
				break;
		}
	}

	fb->lines = g_list_reverse (fb->lines);

#ifdef MANAGE_EOL
	if (fb->separator_type == EOL_UNKNOWN)
	{
		gboolean conform;
		guint maxmode;

		/* vote for the EOL-type */
		maxmode = lf;
		conform = FALSE;
		if (cr > maxmode)
		{
			maxmode = cr;
			conform = (lf > 0 || crlf > 0 || unicr > 0);
		}
		if (crlf > maxmode)
		{
			maxmode = crlf;
			conform = (cr > 0 || lf > 0 || unicr > 0);
		}
		if (unicr > maxmode)
		{
			maxmode = unicr;
			conform = (cr > 0 || lf > 0 || crlf > 0);
		}

		if (maxmode == lf)
			fb->separator_type = EOL_LF;
		else if (maxmode == crlf)
			fb->separator_type = EOL_CRLF;
		else if (maxmode == cr)
			fb->separator_type = EOL_CR;
		else /* max_mode == unicr */
			fb->separator_type = EOL_UNI;
		if (conform)
		{
			DEBUG_PRINT ("need to conform line-separators for %s",
				(fb->uri == NULL) ? "<open file>" : fb->uri);
/* FIXME	if (maxmode == crlf)
				(cr + lf + unicr) expansion needed
			else if (crlf > 0)
				 crlf contraction needed
			walk list and convert each bad separator, slide the trailing text and update list data if 1<>2 bytes
*/
		}
	}
#endif
}

/**
 * file_buffer_freshen_lines:
 * @fb: pointer to file-buffer data for file being processed
 * @startpos: pointer to first list-member to be updated, or NULL for whole list
 *
 * Repopulate list of line-end offsets for whole or part of file associated with @fb
 *
 * Return value: None
 */
static void
file_buffer_freshen_lines (FileBuffer *fb, GList *startpos)
{
	g_return_if_fail (fb->buf);

	if (fb->lines == NULL || fb->lines == startpos || startpos == NULL) /* should never happen */
	{
		file_buffer_find_lines (fb, NULL);
		return;
	}
	else if (g_list_position (fb->lines, startpos) != -1)
	{
		position_t indx, priorb, priorc, /*nextb, nextc,*/ deltab, deltac, total;
		EOLdata *ed;
		gchar *buf, *this;
		GList *node;

		node = g_list_previous (startpos);
		if (node == NULL)	/* ignore 1st member */
			node = startpos;
		priorb = ((EOLdata*)node->data)->offb;
		priorc = ((EOLdata*)node->data)->offc;

		buf = fb->buf;	/* for faster lookup */
		for (indx = priorb, this = buf + priorb; ; indx++, this++)
		{
			gchar c;

			c = *this;
			//CHECKME assumes when EOL == \r\n, can simply ignore the \n
			//CHECKME can any utf-8 char include \r or \n byte(s) ?
			if (c == '\n' || c == '\r' || c == '\0')
				break;
		}
		node = g_list_next (node);	/* must always be at least 1 more member */
//		nextb = ((EOLdata*)node->data)->offb;
//		nextc = ((EOLdata*)node->data)->offc;
		deltab = indx - ((EOLdata*)node->data)->offb;
		deltac = priorc + g_utf8_strlen (buf + priorb, indx - priorb)
					- ((EOLdata*)node->data)->offc;
		total = strlen (buf);

		for (; node != NULL; node = g_list_next (node))
		{
			ed = (EOLdata*)node->data;
			ed->offb += deltab;
			ed->offc += deltac;

			if (ed->offb > total)	/* fewer lines now */
			{
				for (; node != NULL; node = g_list_next (node))
				{
					g_slice_free1 (sizeof (EOLdata), node->data);
					node->data = NULL;
				}
				fb->lines = g_list_remove_all (fb->lines, NULL);
//				node = NULL;
				break;
			}
		}
		if (ed->offb < total)	/* more lines now */
		{
			/* append the rest */
			file_buffer_find_lines (fb, g_list_last (fb->lines));
		}
	}
	else
	{
		//FIXME UI warning
		g_warning ("Error in line-cache refresh for %s", fb->uri);
		return;
	}
}

/**
 * file_buffer_freshen_lines_from_pos:
 * @fb: pointer to file-buffer data for file being processed
 * @offset: position in file buffer
 * @offset_bytes: TRUE if @offset is bytes, FALSE if chars
 *
 * Repopulate list of line-end offsets for whole or part of file associated with @fb
 *
 * Return value: None
 */
void
file_buffer_freshen_lines_from_pos (FileBuffer *fb, position_t offset, gboolean offset_bytes)
{
	GList *node;

	g_return_if_fail (fb->lines);
	/* 1st member of list is effectively before start of file */
	if (offset_bytes)
	{
		for (node = g_list_next (fb->lines); node != NULL; node = g_list_next (node))
		{
			if (offset <= ((EOLdata *)node->data)->offb)
			{
				file_buffer_freshen_lines (fb, node);
				return;
			}
		}
	}
	else
	{
		for (node = g_list_next (fb->lines); node != NULL; node = g_list_next (node))
		{
			if (offset <= ((EOLdata *)node->data)->offc)
			{
				file_buffer_freshen_lines (fb, node);
				return;
			}
		}
	}
	//FIXME UI warning
	g_warning ("Error in line-cache refresh for %s", fb->uri);
}
/* unused ATM
static position_t
file_buffer_byte_len (FileBuffer *fb)
{
	if (fb->buf == NULL || *fb->buf == '\0')
		return 0;
	if (fb->lines != NULL)
	{
		GList *last;
		last = g_list_last (fb->lines);
		return ((EOLdata *)last->data)->offb;
	}
	else
		return strlen (fb->buf);
}
*/

/* *
 * file_buffer_char_len:
 * @fb: pointer to file-buffer data for file being processed
 *
 * Determine character-size of file buffer associated with @fb
 *
 * Return value: the length
 */
/* unused ATM
static position_t
file_buffer_char_len (FileBuffer *fb)
{
	if (fb->buf == NULL || *fb->buf == '\0')
		return 0;
	if (fb->lines)
	{
		GList *last;
		last = g_list_last (fb->lines);
		return ((EOLdata *)last->data)->offc;
	}
	else
		return g_utf8_strlen (fb->buf, -1);
}
*/
/**
 * file_buffer_line_for_pos:
 * @fb: pointer to file-buffer data for file being processed
 * @pos: buffer position, in chars
 * @pos_bytes: TRUE if @pos is byte-position, FALSE if char-position
 *
 * Find the line number of the text at @pos in file buffer associated with @fb
 *
 * Return value: the 1-based line-number for @pos, or -1
 */
static line_t
file_buffer_line_for_pos (FileBuffer *fb, position_t pos, gboolean pos_bytes)
{
	g_return_val_if_fail (fb && pos >= 0, -1);

	line_t lineno;
	GList *node;

	g_return_val_if_fail (fb->lines, -1);
	/* 1st member of list is effectively before start of file */
	if (pos_bytes)
	{
		for (node = g_list_next (fb->lines), lineno = 1; node != NULL; node = g_list_next(node), lineno++)
		{
			if (pos <= ((EOLdata *)node->data)->offb)
				return lineno;
		}
	}
	else
	{
		for (node = g_list_next (fb->lines), lineno = 1; node != NULL; node = g_list_next(node), lineno++)
		{
			if (pos <= ((EOLdata *)node->data)->offc)
				return lineno;
		}
	}
	return -1;
}

/* *
 * file_buffer_get_linetext_for_pos:
 * @fb: pointer to file-buffer data for file being processed
 * @pos: buffer position, in chars
 *
 * Looks for strings separated by newline chars CHECKME all EOL's ok ?
 *
 * Return value: newly-allocated string which spans @pos, or NULL
 */
/*
gchar *
file_buffer_get_linetext_for_pos (FileBuffer *fb, position_t pos)
{
	gchar *buf;
	position_t indx;

	g_return_val_if_fail (fb && pos >= 0, NULL);

	buf = fb->buf;
	indx = file_buffer_get_byte_offset (fb, pos);
	if (indx >= 0)	/ * no error * /
	{
		gint length;
		gchar *this;
		register gchar c;

		length = 1;
		/ * use index instead of array cuz it's faster * /
		/ * simple ascii scanning is ok cuz we want the whole line * /
		for (this = buf + indx + sizeof (gchar); ; this++, length++)
		{
			c = *this;
			if (c == '\n' || c == '\r' || c == '\0')
				break;
		}
		for (this = buf + indx - sizeof (gchar); ; this--, length++)
		{
			if (this < buf)
				break;
			c = *this;
			if (c =='\n' || c =='\r')
				break;
		}

		return g_strndup (++this, length);
	}
	return NULL;
}
*/

/**
 * file_buffer_get_linetext_for_line:
 * @fb: pointer to file-buffer data for file being processed
 * @lineno: buffer line number, 1-based
 *
 * Looks for strings separated by newline chars CHECKME all EOL's ok ?
 *
 * Return value: newly-allocated string corresonding to line @lineno, or NULL
 */
gchar *
file_buffer_get_linetext_for_line (FileBuffer *fb, line_t lineno)
{
	GList *node;

	g_return_val_if_fail (fb && fb->lines, NULL);

	node = g_list_nth (fb->lines, (guint) lineno);
	if (node != NULL)
	{
		gchar *start, *end;

		end = fb->buf + ((EOLdata *)node->data)->offb;
		node = g_list_previous (node);
		start = fb->buf + ((EOLdata *)node->data)->offb + 1;
		while (start < end)
		{
			register gchar c;
			c = *start;
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
				start++;
			else
				break;
		}
		return g_strndup (start, end - start);
	}
	return NULL;
}

/**
 * save_file_buffer:
 * @fb: pointer to file-buffer data for file being processed
 *
 * Save file buffer for @fb, after reverting its encoding if necessary
 *
 * Return value: TRUE if any conversion and the save were completed successfully
 */
gboolean
save_file_buffer (FileBuffer *fb)
{
	position_t bytelength;
	gsize bytes_new;
	gchar *converted;
	gchar *local_path;

	/* revert to original encoding if possible */
	/*CHECKME according to anj save-option ? revert EOL's too ? */
	bytelength = file_buffer_get_byte_offset (fb, -1);
	if (bytelength > 0)
	{
		GError *error;

		error = NULL;
		converted = fb->buf; /*anjuta_convert_from_utf8 (fb->buf,
											(gsize) bytelength,
											fb->encoding,
											&bytes_new,
											&error);*/
		if (error != NULL)
		{
//			gchar *msg;
			switch (error->code)
			{
				case G_CONVERT_ERROR_ILLEGAL_SEQUENCE:
/* FIXME warn user
					msg = g_strdup_printf (
					_("Cannot save %s in its original encoding %s. Reverting to UTF-8"),
						fb->uri, (fb->encoding) ? fb->encoding : "UNKNOWN");
					perror (msg);
					g_free (msg);
*/
					g_warning ("Cannot save %s in its original encoding %s. Reverting to UTF-8",
								fb->uri, (fb->encoding) ? fb->encoding : "UNKNOWN");
					g_error_free (error);
					converted = fb->buf;
					bytes_new = bytelength;
					break;
				default:
/* FIXME warn user
					msg = g_strdup_printf (
					_("Encoding conversion failed for %s"), fb->uri);
					perror (msg);
					g_free (msg);
*/
					g_warning ("Encoding conversion failed for %s", fb->uri);
					g_error_free (error);
					return FALSE;
			}
		}
	}
	else
	{
		converted = fb->buf;
		bytes_new = 0;
	}

	/* success */

	local_path = gnome_vfs_get_local_path_from_uri (fb->uri);
	if (local_path != NULL)
	{
		GnomeVFSHandle* vfs_write;
		GnomeVFSResult result;
		GnomeVFSFileSize nchars;

		g_free (local_path);

		result = gnome_vfs_create (&vfs_write, fb->uri, GNOME_VFS_OPEN_WRITE,
								   FALSE, 0664);	/* CHECKME this mode always ok ? */
		if (result != GNOME_VFS_OK)
			return FALSE;

		result = gnome_vfs_write (vfs_write, converted, bytes_new, &nchars);

		if (result == GNOME_VFS_OK)
			result = gnome_vfs_close (vfs_write);
		else
			gnome_vfs_close (vfs_write);

		if (converted != fb->buf)
			g_free (converted);
		return (result == GNOME_VFS_OK);
	}
	else
	{
		g_free (local_path);
		/* FIXME do async save, with gtk_main() until the completion callback */
		g_return_val_if_reached (FALSE);
	}
	return FALSE;
}

/**
 * replace_in_local_buffer:
 * @fb: pointer to file-buffer data for file being processed
 * @matchstart: byte-position of the start of the string to be replaced in the file-buffer
 * @matchlen: byte-length of the matched string
 * @repl_str: string to be substituted
 *
 * Replace some content of the the buffer associated with @fb
 *
 * Return value: TRUE if the update completed successfully
 */
gboolean
replace_in_local_buffer (FileBuffer *fb,
						 position_t matchstart,
						 position_t matchlen,
						 gchar *repl_str)
{
	position_t l, replen, buflen;
	GList *node;

	g_return_val_if_fail (repl_str != NULL, FALSE);

	buflen = file_buffer_get_byte_offset (fb, -1);
	replen = strlen (repl_str);
	if (replen > matchlen)
	{
		/* new string is longer */
		l = buflen - matchstart;
		buflen = buflen + replen - matchlen;
		if ( (fb->buf = g_try_realloc (fb->buf, buflen)) == NULL )
			return FALSE;
		/* make a hole */
		memmove (fb->buf + matchstart + replen - matchlen, fb->buf + matchstart, l);
	}
	else if  (replen < matchlen)
	{
		/* new string is shorter */
		l = buflen - matchstart - matchlen;
		memmove (fb->buf + matchstart + replen, fb->buf + matchstart + matchlen, l);
		buflen = buflen + replen - matchlen;
		if ( (fb->buf = g_try_realloc (fb->buf, buflen)) == NULL)
			return FALSE;
	}

	memcpy (fb->buf + matchstart, repl_str, replen);
	/* update byte length, but don't bother with anything else in the lines-list */
	node = g_list_last (fb->lines);
	((EOLdata *)node->data)->offb += (replen - matchlen);

	return TRUE;
}

/**
 * get_search_files_list:
 * @sf:
 * @stop_uri:
 *
 * Generate a list of files to search in. This is used when the search range
 * is specified as SR_FILES
 * Call with start = TRUE and top_dir = sf->top_dir.
 *
 * Return value: the newly-allocated list of files, or NULL
 */
static GList *
get_search_files_list (SearchFiles *sf, const gchar *top_uri)
{
	gchar *top_dir;

	g_return_val_if_fail (sf && top_uri, NULL);

	top_dir = gnome_vfs_get_local_path_from_uri (top_uri);		//FIXME allow func to use uri
	if (top_dir != NULL)
	{
		TMFileEntry *entry;
		entry = tm_file_entry_new (top_dir, NULL, sf->recurse, sf->match_files,
									sf->ignore_files, sf->match_dirs, sf->ignore_dirs,
	  								sf->ignore_hidden_files, sf->ignore_hidden_dirs);
		g_free (top_dir);
		if (entry != NULL)
		{
			GList *files;
			/* CHECKME want list of non-constant uri's */
			files = tm_file_entry_list (entry, NULL);
			tm_file_entry_free (entry);
			return files;
		}
	}
	else
	{
		g_warning ("Cannot create local path for requested uri");
	}
	return NULL;
}

/**
 * get_project_files_list:
 * @sr: pointer to populated search/replace data struct
 *
 * Get a list of all project files from the project manager plugin
 *
 * Return value: the newly-allocated list of files, or NULL
 */
static GList *
get_project_files_list (SearchReplace *sr)
{
	gchar *project_root_uri = NULL;

//	anjuta_shell_get (ANJUTA_PLUGIN (sr->docman)->shell,
	anjuta_shell_get (ANJUTA_PLUGIN (splugin)->shell,
					  "project_root_uri", G_TYPE_STRING,
					  &project_root_uri, NULL);

	if (project_root_uri)
	{
		GList *files;
		IAnjutaProjectManager* prjman;

//		prjman = anjuta_shell_get_interface (ANJUTA_PLUGIN (sr->docman)->shell,
		prjman = anjuta_shell_get_interface (ANJUTA_PLUGIN (splugin)->shell,
											IAnjutaProjectManager, NULL);
		/* get a list of non-constant project-file uri's */
		files = ianjuta_project_manager_get_elements (prjman,
													 IANJUTA_PROJECT_MANAGER_SOURCE,
													 NULL);
		g_free (project_root_uri);
		return files;
	}

	g_warning ("OOPS, there's no known root-directory for the project");
	return NULL;
}

/**
 * isawordchar:
 * @c: character to test
 *
 * Decide whether @c is a "word" character, not whitespace or punctuation
 *
 * Return value: TRUE if @c is letter, number or '_'
 */
static gboolean
isawordchar (gunichar c)
{
	return (g_unichar_isalnum (c) || (gchar)c == '_');
}

/**
 * extra_match:
 * @fb: pointer to file-buffer data for file being processed
 * @se: pointer to search data struct
 * @start_pos: offset of possible-match start, >= 0, bytes or chars as used by the editor
 * @end_pos: offset of possible-match end, may be -1 for EOF, bytes or chars as used by the editor
 *
 * Determine whether relevant extra search condition(s) (whole word etc) are
 * satisfied for a non-regex seach operation
 *
 * Return value: TRUE if all extra conditiions are satisfied, or there are none
 */
static gboolean
extra_match (FileBuffer *fb, SearchExpression *se, position_t start_pos, position_t end_pos)
{
	gunichar b, e;
	gchar *s;

	if (!(se->word_start || se->whole_word || se->whole_line))
		return TRUE;

	if (start_pos == 0)
	{
		s = fb->buf;
		b = (gunichar)' '; /* warning prevention only */
	}
	else
	{
/* ATM only local searching uses this, and it works with byte-positions
		if (se->postype == SP_BYTES)
		{ */
			s = g_utf8_find_prev_char (fb->buf, fb->buf + start_pos);
			if (s == NULL)
				return FALSE;
/*		}
		else
		{
			s = fb->buf + file_buffer_get_byte_offset (fb, start_pos - 1);
		}
*/
		b = g_utf8_get_char_validated (s, -1);
		if (b == (gunichar)-1 || b == (gunichar)-2)
			//FIXME warning
			return FALSE;
	}

	if (se->word_start)
	{
		if (start_pos == 0 || !isawordchar(b))
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		if (end_pos == -1)
			e = (gunichar)'\n';	/* warning prevention only */
		else
		{
//			if (se->postype == SP_BYTES)
				s = fb->buf + end_pos + 1;
//			else
//				s = g_utf8_next_char (fb->buf + end_pos);
			if (*s == '\0')
				e = (gunichar) '\0';
			else
			{
				e = g_utf8_get_char_validated (s, -1);
				if (e == (gunichar)-1 || b == (gunichar)-2)
					//FIXME warning
					return FALSE;
			}
		}

		if (se->whole_word)
		{
			if ((start_pos == 0 || !isawordchar(b))
			 && (end_pos == -1 || e == (gunichar)'\0' || !isawordchar (e)))
				return TRUE;
			else
				return FALSE;
		}
		else /*se->whole_line*/
		{
			if ((start_pos == 0 || b == (gunichar)'\n' || b == (gunichar)'\r')
			 && (end_pos == -1 || e == (gunichar)'\0' || e == (gunichar)'\n' || e == (gunichar)'\r'))
				return TRUE;
			else
				return FALSE;
		}
	}
}

/**
 * search_locally:
 * @se: search data struct
 * @direction: enumerator of search-direction
 * @hlen byte-length of target "haystack"
 *
 * Decide whether a local copy of file text is needed for searching
 * As this test includes a review of advanced-search skip-table, that table is
 * initialised if it wasn't already
 *
 * Return value: TRUE if a local buffer is needed
 */
/* UNUSED ATM
static gboolean
search_locally (SearchExpression *se, SearchDirection direction, position_t hlen)
{
	if (se->regex)
		return TRUE;
	/ * FIXME case insensitive searching needed too * /
	if ( hlen < ADV_SRCH_MIN_LENGTH2
	  || strlen (se->search_str) < ADV_SRCH_MIN_LENGTH	/ * short patterns use simple search * /
//	  || direction == SD_BACKWARD
	  || se->ignore_case	/ * native searching for any case is easier * /
	  //etc ?
		)
		return FALSE;

	if (se->skiptable == NULL)
	{
		/ * populate and check skip-table at se->skiptable * /
		se->advanced = search_table_new (se, direction);
		if (!se->advanced)
		/ * freeable pointer to prevent repeated useless attempts to create table * /
			se->skiptable = g_malloc (1);
	}
	return se->advanced;
}
*/

/**
 * get_next_match:
 * @fb: pointer to file-buffer data for file being processed
 * @direction: enumerator of search-direction
 * @se: search data struct
 * @mi: pointer to place to store data for the next match in the buffer related to @fb
 *
 * The search expression should be pre-compiled. ??
 * After a match, fb->start_pos or fb->end_pos is updated to suit
 * Pre-checking here for validity of start- and end-position, and some tweaking if needed
 *
 * Return value: TRUE if a match is found
 */
gboolean
get_next_match (FileBuffer *fb, SearchDirection direction, SearchExpression *se, MatchInfo *mi)
{
	gboolean retval;

	g_return_val_if_fail (fb && se && se->search_str, FALSE);

	if (*se->search_str == '\0')
		return FALSE;

	/* CHECKME these tests are more relevant for forward search ?*/
	if (fb->start_pos == -1)
		fb->start_pos = 0;
	if (fb->end_pos == -1)
		fb->end_pos = fb->len - 1;
	else if (fb->end_pos == fb->len)	/* this is another proxy for last position */
		fb->end_pos--;
	if (fb->start_pos >= fb->end_pos)
		return FALSE;

	retval = FALSE;

	if (se->regex)
	{
		/* regular expression search */
		gint options;
		gint status;
		position_t ps, pl;

		if (se->re == NULL)
		{
			/* the pattern will be compiled with utf-8 support if that's available */
			se->re = pcre_info_new (se);
			if (se->re == NULL)
				return FALSE;
		}

		/* pcre takes/gives positions as byte-counts even if utf-8 is suppported */
		ps = file_buffer_get_byte_offset (fb, fb->start_pos);
		pl = file_buffer_get_byte_offset (fb, -1);

		options = PCRE_NOTEMPTY;
		status = pcre_exec (se->re->re, se->re->extra, fb->buf, pl, ps, options,
							se->re->ovector, 3 * (se->re->ovec_count + 1));
		if (status == 0)
		{
			/* ovector too small - this should never happen ! */
			g_warning ("BUG ! ovector found to be too small");
			return FALSE;
		}
		else if (status < 0 && status != PCRE_ERROR_NOMATCH)
		{
			/* match error - again, this should never happen */
			g_warning ("PCRE Match error");
			return FALSE;
		}
		else if (status != PCRE_ERROR_NOMATCH)
		{
			position_t ms, mf;

			retval = TRUE;
			ms = (position_t) se->re->ovector[0];	/* pcre uses gint */
			/* mf will be one-past last matched position */
			mf = (position_t) se->re->ovector[1];
			if (se->postype == SP_BYTES)
			{
				mi->pos = ms;
				mi->len = mf - ms;
			}
			file_buffer_get_char_offset_pair (fb, &ms, &mf);
			/* start-position for next time after end of this match, may be past EOF */
			fb->start_pos =	mf;
			mi->line = file_buffer_line_for_pos (fb, ms, FALSE);
			if (se->postype == SP_CHARS)	/* want char-positions */
			{
				mi->pos = ms;
				mi->len = mf - ms;
			}
			DEBUG_PRINT ("Match found at position %d (%s)", mi->pos,
							(se->postype == SP_BYTES) ? "bytes":"chars");
			if (status > 1) /* captured sub-expressions */
			{
				gint i;
				for (i = 1; i < status; i++)
				{
					MatchSubStr *mst;

					mst = g_slice_new (MatchSubStr);
					ms = (position_t) se->re->ovector[i * 2];
					mf = (position_t) se->re->ovector[i * 2 + 1];	/* after end of match */
					/* cuz backref replacements are done in local buffer,
					   always want byte-positions as reported by pcre */
					mst->start = ms;
					mst->len = mf - ms;
					mi->subs = g_list_prepend (mi->subs, mst);
				}
				if (mi->subs != NULL)	/* should never fail */
					mi->subs = g_list_reverse (mi->subs);
			}
		} /* match found */
	} /* regex-search */
	else
	{
		/* naive brute-force string scan
		   more "advanced" algorithms have been investigated and found to be of
		   no practical net benefit in this context */
		gchar *needle, *haystack, *sc, *sf;
		gunichar uc;
		gchar c;
		gboolean found;
		gint match_len;
		position_t ps, pf;

		ps = file_buffer_get_byte_offset (fb, fb->start_pos);
		pf = file_buffer_get_byte_offset (fb, fb->end_pos);

		if (se->ignore_case)
		{
			needle = g_utf8_casefold (se->search_str, -1);
			uc = g_utf8_get_char (se->search_str);
			uc = g_unichar_tolower (uc);
			c = (gchar) uc;
		}
		else
		{
			needle = se->search_str;
			c = *needle;
		}
		match_len = strlen (se->search_str);

		found = FALSE;
		if (direction == SD_BACKWARD)
		{
			/* backward scan */
			sf = fb->buf + ps;
			if (se->ignore_case)
			{
				sc = fb->buf + pf - match_len + 1;
				uc = g_utf8_get_char_validated (sc, 1);
				if (uc == (gunichar)-1 || uc == (gunichar)-2)
					sc = g_utf8_find_prev_char (fb->buf, sc);
				for ( ; sc >= sf; sc = g_utf8_prev_char (sc))
				{
					uc = g_utf8_get_char (sc);
					uc = g_unichar_tolower (uc);
					if ((gchar)uc == c)
					{
						/* FIXME make this less repetitive while still knowing match position */
						haystack = g_utf8_casefold (sc, match_len);
						found = (strcmp (haystack, needle) == 0);
						g_free (haystack);
						if (found &&
							extra_match (fb, se, sc - fb->buf, sc - fb->buf + match_len - 1))
						{
							//ps = sc - fb->buf;
							//pf = ps + match_len - 1;
							break;
						}
						else
							found = FALSE;
					}
				}
			}
			else /* case-specific */
			{
				for (sc = fb->buf + pf - match_len + 1; sc >= sf; sc--)
				{
					if (*sc == c)
					{
						if (strncmp (sc, needle, match_len) == 0 &&
							extra_match (fb, se, sc - fb->buf, sc - fb->buf + match_len - 1))
						{
							found = TRUE;
							//ps = sc - fb->buf;
							//pf = ps + match_len - 1;
							break;
						}
					}
				}
			}
		}
		else
		{
			/* forward scan */
			sf = fb->buf + pf - match_len + 1;
			if (se->ignore_case)
			{
				for (sc = fb->buf + ps; sc <= sf; sc = g_utf8_next_char (sc))
				{
					uc = g_utf8_get_char (sc);
					uc = g_unichar_tolower (uc);
					if ((gchar)uc == c)
					{
						/* FIXME make this less repetitive while still knowing match position */
						haystack = g_utf8_casefold (sc, match_len);
						found = (strcmp (haystack, needle) == 0);
						g_free (haystack);
						if (found &&
							extra_match (fb, se, sc - fb->buf, sc - fb->buf + match_len - 1))
						{
							//ps = sc - fb->buf;
							//pf = ps + match_len - 1;
							break;
						}
						else
							found = FALSE;
					}
				}
			}
			else /* case-specific */
			{
				for (sc = fb->buf + ps; sc <= sf; sc++)
				{
					if (*sc == c)
					{
						if (strncmp (sc, needle, match_len) == 0 &&
							extra_match (fb, se, sc - fb->buf, sc - fb->buf + match_len - 1))
						{
							found = TRUE;
							//ps = sc - fb->buf;
							//pf = ps + match_len - 1;
							break;
						}
					}
				}
			}
		}
		if (found)
		{
			ps = sc - fb->buf;
			//pf = ps + match_len - 1;
			if (direction == SD_BACKWARD)
			{
				fb->end_pos = (ps > 0) ? file_buffer_get_char_offset (fb, ps - 1) : 0;
			}
			else
				fb->start_pos = file_buffer_get_char_offset (fb, ps + match_len);
			mi->pos = (se->postype == SP_BYTES) ?
				ps : file_buffer_get_char_offset (fb, ps);
			DEBUG_PRINT ("Match found at position %d (%s)", mi->pos,
							(se->postype == SP_BYTES) ? "bytes":"chars");
			/* fixed length (==mf-ms) is set upstream */
			mi->line = file_buffer_line_for_pos (fb, ps, TRUE);
			retval = TRUE;
		}
		if (se->ignore_case)
		{
			g_free (needle);
		}
	}	/* naive-search */

	return retval;
}

/**
 * create_search_entries:
 * @sr: pointer to populated search/replace data struct
 *
 * Create list of search entries, with a member for each buffer or file to be processed
 * Open buffers are checked for being searchable before being added to the list
 * Start and end positions are set, using editor-native offsets if not 0 or -1
 * End position is -1 when searching forward to the end of buffer
 * Unless end == -1, start > end regardless of search direction
 * In general, there is no file-buffer yet to allow conversion from byte-positions
 * if that's needed. So byte-postitions need to be converted to chars upstream
 *
 * Return value: None
 */
void
create_search_entries (SearchReplace *sr)
{
	GList *entries;
	GList *node;
	GList *wids;
	IAnjutaDocument *doc;
	Search *s;
	SearchEntry *se;
	position_t selstart;
	position_t tmp_pos;

	entries = NULL;
	s = &(sr->search);
	switch (s->range.target)
	{
		case SR_BUFFER:
			/* scope is all of current buffer */
			doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
			if (doc && IANJUTA_IS_EDITOR_SEARCH (doc))
			{
				se = g_slice_new0 (SearchEntry);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR (doc);
				/* backward search does not work for regular expressions, that's
					probabley enforced by the GUI, but ... */
				if (s->expr.regex && s->range.direction == SD_BACKWARD)
					s->range.direction = SD_FORWARD;
				if (s->range.direction == SD_WHOLE)	/* from start of buffer */
				{
					se->start_pos = 0;
					se->end_pos = -1;
					se->direction = SD_FORWARD;	/* the actual direction to use */
				}
				else
				{
					se->direction = s->range.direction;
					if (ianjuta_editor_selection_has_selection
						(IANJUTA_EDITOR_SELECTION (se->te), NULL))
					{
						IAnjutaIterable *x =
						ianjuta_editor_selection_get_start
							(IANJUTA_EDITOR_SELECTION (se->te), NULL);
						selstart = ianjuta_iterable_get_position (x, NULL);
						g_object_unref (G_OBJECT (x));
					}
					else
						selstart = -1;
					if (selstart != -1)
					{
						if (se->direction == SD_BACKWARD)
						{
							se->start_pos = (selstart != 0) ?
								 selstart - 1 : selstart;
						}
						else
							se->start_pos = selstart;								
					}
					else
					{
						IAnjutaIterable *x =
							ianjuta_editor_get_position (se->te, NULL);
						se->start_pos = ianjuta_iterable_get_position (x, NULL);
						g_object_unref (G_OBJECT (x));
					}

					if (se->direction == SD_BACKWARD)
					{
						se->end_pos = se->start_pos;
						se->start_pos = 0;
					}
					else
						se->end_pos = -1;
				}
				entries = g_list_prepend (entries, se);
			}
			else if (doc)
			{
				g_warning ("Current buffer is not searchable");
			}
			break;
		case SR_SELECTION:
		case SR_BLOCK:
		case SR_FUNCTION:
			/* scope is some of current buffer */
			doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
			if (doc && IANJUTA_IS_EDITOR_SEARCH (doc))
			{
				position_t current;
				position_t selend;

				se = g_slice_new0 (SearchEntry);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR (doc);
				if (s->range.direction == SD_WHOLE)	/* start of scope */
					se->direction = SD_FORWARD;
				else
				/* Note that backward search does not work for regular expressions */
					se->direction = s->range.direction;

	//FIXME

				if (s->range.target == SR_SELECTION)
				{
					if (selstart < 0)
						break;
					se->start_pos = selstart;
					se->end_pos = selend;
				}
				else
				{
					if (s->range.target == SR_BLOCK)
						ianjuta_editor_selection_select_block (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					else if (s->range.target == SR_FUNCTION)
						ianjuta_editor_selection_select_function (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					IAnjutaIterable *x =
						ianjuta_editor_selection_get_start
							(IANJUTA_EDITOR_SELECTION (se->te), NULL);
					se->start_pos = ianjuta_iterable_get_position (x, NULL);
					IAnjutaIterable *y =
						ianjuta_editor_selection_get_end
							(IANJUTA_EDITOR_SELECTION (se->te), NULL);
					se->start_pos = ianjuta_iterable_get_position (y, NULL);
					ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (se->te),
				       	                         x, y, NULL);
					g_object_unref (x);
					g_object_unref (y);
					if (selstart < 0)
					{
						ianjuta_editor_goto_position (IANJUTA_EDITOR (se->te),
														x, NULL);
					}
				}

				switch (s->range.direction)
				{
					//case SD_WHOLE:
					default:
						/* start of scope forward to end, no change */
						break;
					case SD_FORWARD:
						/* forward from current or start of selection if inside scope */
						tmp_pos = (selstart < 0) ? /* no selection */
							current : selstart + 1;
						if (tmp_pos > se->start_pos && tmp_pos < se->end_pos)
							se->start_pos = tmp_pos;
						break;
					case SD_BACKWARD:
						/* backward from current or start of selection if inside scope */
						tmp_pos = (selstart < 0) ? /* no selection */
							current : selstart;
						if (tmp_pos > se->start_pos && tmp_pos < se->end_pos)
							se->end_pos = tmp_pos;
						/* CHECKME with in-enditor searching * /
						tmp_pos = se->start_pos;
						se->start_pos = se->end_pos;
						se->end_pos = tmp_pos;
*/						break;
				}

				entries = g_list_prepend (entries, se);
			}
			else if (doc)
			{
				g_warning ("Current buffer is not searchable");
			}
			break;
		case SR_OPEN_BUFFERS:
			wids = ianjuta_document_manager_get_doc_widgets (sr->docman, NULL);
			for (node = wids; node != NULL; node = g_list_next (node))
			{
				if (IANJUTA_IS_EDITOR_SEARCH (node->data))
				{
					se = g_slice_new0 (SearchEntry);
					se->type = SE_BUFFER;
					se->te = IANJUTA_EDITOR (node->data);
					se->direction = SD_FORWARD;
					se->start_pos = 0;
					se->end_pos = -1;
					entries = g_list_prepend (entries, se);
				}
				else
				{
					g_warning ("Skipping un-searchable file");
				}
			}
			if (entries != NULL)
				entries = g_list_reverse (entries);
			g_list_free (wids);
			break;
		case SR_FILES:
		case SR_PROJECT:
		{
			GList *files = NULL;
			gchar *dir_uri = NULL;
			gchar *freeme;

//			anjuta_shell_get (ANJUTA_PLUGIN (sr->docman)->shell,
			anjuta_shell_get (ANJUTA_PLUGIN (splugin)->shell,
							  "project_root_uri", G_TYPE_STRING,
							  &dir_uri, NULL);
			if (dir_uri == NULL)
			{
				/* fallback to matching files in current dir */
				dir_uri = gnome_vfs_get_uri_from_local_path (g_get_current_dir ());
				if (dir_uri == NULL)
				{
					if (entries != NULL)
						clear_search_entries (&entries);
					g_return_if_fail (dir_uri);
				}
				if (s->range.target == SR_PROJECT)
					s->range.target = SR_FILES;
			}

			freeme = dir_uri;
			dir_uri = gnome_vfs_make_uri_canonical (dir_uri);
			g_free (freeme);
			if (dir_uri == NULL)
			{
				if (entries != NULL)
					clear_search_entries (&entries);
				g_return_if_fail (dir_uri);
			}

			if (s->range.target == SR_FILES)
				files = get_search_files_list (&(s->range.files), dir_uri); //CHECKME what does this list contain ?
			else /*  s->range.type == SR_PROJECT */
				files = get_project_files_list (sr);

			if (files)
			{
				gint applen;
				GList *node;
				const gchar *check_mimes [7] =
				{
					GNOME_VFS_MIME_TYPE_UNKNOWN,	/* for warnings */
					"text/",	/* this is a prefix */
					"application/x-", /* ditto */
					"glade",	/* acceptable suffixes for application/x- */
					"shellscript",
					"perl",
					"python",	/* unlikely as a source */
				};

				applen = strlen (check_mimes[2]);
				for (node = files; node != NULL; node = g_list_next (node))
				{
					gchar *uri;

					uri = (gchar *)node->data;
					if (s->range.target == SR_FILES)	/* the returned list is paths, not uris */
					{
						gchar *real_uri;

						real_uri =  gnome_vfs_get_uri_from_local_path (uri);
						if (real_uri != NULL)
						{
							g_free (uri);
							uri = real_uri;
						}
						else
						{
							if (entries != NULL)
								clear_search_entries (&entries);
							g_return_if_fail (real_uri);
						}
					}
					if (uri)
					{
						gchar *mime_type;

						mime_type = anjuta_util_get_uri_mime_type (uri);
						if (mime_type)
						{
							if (g_str_has_prefix (mime_type, check_mimes [1])
							 ||(g_str_has_prefix (mime_type, check_mimes [2]) &&
								(  strcmp (mime_type + applen, check_mimes [3]) == 0
								|| strcmp (mime_type + applen, check_mimes [4]) == 0
								|| strcmp (mime_type + applen, check_mimes [5]) == 0
								|| strcmp (mime_type + applen, check_mimes [6]) == 0
								)))
							{
								gchar *canon_uri;

								canon_uri = gnome_vfs_make_uri_canonical (uri);
								//CHECKME any escaping or other cleanup ?
								if (canon_uri != NULL)
								{
									/* there might be an already-open document with this uri */
									doc = ianjuta_document_manager_find_document_with_uri
											(sr->docman, canon_uri, NULL);
									if (doc == NULL	/* not open now */
										|| IANJUTA_IS_EDITOR_SEARCH (doc)) /* open and searchable CHECKME glade files ? */
									{
										se = g_slice_new0 (SearchEntry);
										se->type = (doc) ? SE_BUFFER:SE_FILE;
										if (doc)
										{
											g_free (canon_uri);
											se->te = IANJUTA_EDITOR (doc);
										}
										else
											se->uri = canon_uri;
										se->direction = SD_FORWARD;	/* default scope, may change later */
										se->start_pos = 0;
										se->end_pos = -1;
										entries = g_list_prepend (entries, se);
									}
									else
									{
										g_warning ("skipping open file %s which is not searchable",
													uri);
									}
								}
								else
								{
									//FIXME UI warning
									g_warning ("Skipping %s, cannot get its real uri", uri);
								}
							}
							else if (strcmp (mime_type, check_mimes [3]) == 0)
							{
								//FIXME UI warning
								g_warning ("Skipping %s, unknown mime-type", uri);
							}
							g_free (mime_type);
						}
						else
						{
							//FIXME UI warning
							g_warning ("Skipping %s, cannot get its mime-type", uri);
						}
						g_free (uri);
					}
				}
				g_list_free (files);
				if (entries != NULL)
					entries = g_list_reverse (entries);
			}
			g_free (dir_uri);
			break;
		}
	}
	sr->search.candidates = entries;
}

/**
 * clear_search_entries:
 * @entries: pointer to store of list-pointer
 *
 * Cleanup list of search entries
 *
 * Return value: none
 */
void
clear_search_entries (GList **entries)
{
	if (*entries != NULL)
	{
		GList *node;

		for (node = *entries; node != NULL; node = g_list_next (node))
		{
			SearchEntry *se;
			se = (SearchEntry *)node->data;
			if (se != NULL)
			{
				g_free (se->uri);
				g_free (se->regx_repl_str);
				if (se->fb != NULL)
					file_buffer_free (se->fb);
				if (se->mi.subs != NULL)
					match_info_free_subs (&(se->mi));
				g_slice_free1 (sizeof (SearchEntry), se);
			}
		}
		g_list_free (*entries);
		*entries = NULL;
	}
}

/**
 * search_get_default_data:
 *
 * Get pointer to static data struct with search/replace data
 *
 * Return value: the data
 */
SearchReplace *
search_get_default_data (void)
{
	if (def_sr == NULL)
	{
		def_sr = search_replace_data_new ();
		g_return_val_if_fail (def_sr != NULL, NULL);
	}
	return def_sr;
}

/**
 * search_replace_data_new:
 *
 * Create new search/replace data struct with some generally-useful content
 *
 * Return value: pointer to the data struct
 */
SearchReplace *
search_replace_data_new (void)
{
	SearchReplace *sr;

	sr = g_slice_new0 (SearchReplace);
	sr->docman = splugin->docman;

	/* log whether pcre can handle utf-8 */
	pcre_config (PCRE_CONFIG_UTF8, &sr->search.expr.utf8regex);

	return sr;
}

#undef FREE_FN
#define FREE_FN(fn,v) if (v) { fn(v); }

/**
 * clear_search_replace_instance:
 * @sr: pointer to sr data struct to clear
 *
 * Clears allocated memory, and zero's all contents of @sr other than docman and sg
 *
 * Return value: none
 */
static void
clear_search_replace_instance (SearchReplace *sr)
{
	g_free (sr->search.expr.search_str);
	g_free (sr->search.expr.re);
	FREE_FN(pcre_info_free, sr->search.expr.re);
	FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_files);
	FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_files);
	FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_dirs);
	FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_dirs);
	FREE_FN(anjuta_util_glist_strings_free, sr->search.expr_history);
	g_free (sr->replace.repl_str);
	FREE_FN(anjuta_util_glist_strings_free, sr->replace.expr_history);

//	memset (&sr->search, 0, sizeof (Search));
//	memset (&sr->replace, 0, sizeof (Replace));
}

/**
 * search_replace_data_destroy:
 * @sr: pointer to sr data struct to clear, or NULL for default
 *
 * Return value: none
 */
void
search_replace_data_destroy (SearchReplace *sr)
{
	if (sr == NULL)
		sr = def_sr;
	if (sr != NULL)
	{
		clear_search_replace_instance (sr);
		g_slice_free1 (sizeof (SearchReplace), sr);
	}
}

/**
 * search_replace_init:
 * @plugin: pointer to plugin data struct
 *
 * Called when search plugin is activated.
 *
 * Return value: none
 */
void
search_replace_init (AnjutaPlugin *plugin)
{
	/* make application data available to funcs here*/
	splugin = ANJUTA_PLUGIN_SEARCH (plugin);
}

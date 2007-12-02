/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "svn-log-entry.h"

struct _SvnLogEntryPriv
{
	gchar *author;
	gchar *date;
	glong revision;
	gchar *log;
	gchar *short_log;
};

G_DEFINE_TYPE (SvnLogEntry, svn_log_entry, G_TYPE_OBJECT);

static void
svn_log_entry_init (SvnLogEntry *self)
{
	self->priv = g_new0 (SvnLogEntryPriv, 1);
}

static void
svn_log_entry_finalize (GObject *object)
{
	SvnLogEntry *self;
	
	self = SVN_LOG_ENTRY (object);
	
	g_free (self->priv->author);
	g_free (self->priv->date);
	g_free (self->priv->log);
	g_free (self->priv->short_log);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_log_entry_parent_class)->finalize (object);
}

static void
svn_log_entry_class_init (SvnLogEntryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = svn_log_entry_finalize;
}

static gchar *
strip_whitespace (gchar *buffer)
{
	gchar *buffer_pos;
	
	buffer_pos = buffer;
	
	while (buffer_pos)
	{
		if (g_ascii_isspace (*buffer_pos))
			buffer_pos++;
		else
			break;
	}
	
	return buffer_pos;
}

SvnLogEntry *
svn_log_entry_new (gchar *author, gchar *date, glong revision, gchar *log)
{
	SvnLogEntry *self;
	gchar *log_filtered;  /* No leading whitespace */
	gchar *first_newline;
	size_t first_newline_pos;
	gchar *first_log_line;
	gchar *short_log;
	
	self = g_object_new (SVN_TYPE_LOG_ENTRY, NULL);
	self->priv->author = g_strdup (author);
	self->priv->date = g_strdup (date);
	self->priv->revision = revision;
	self->priv->log = g_strdup (log);
	
	/* Now create the "short log", or a one-line summary of a log.
	 * This is just the first line of a commit log message. If there is more 
	 * than one line to a message, take the first line and put an ellipsis 
	 * after it to create the short log. Otherwise, the short log is just a 
	 * copy of the log messge. */
	log_filtered = strip_whitespace (log);
	first_newline = strchr (log_filtered, '\n');
	
	if (first_newline)
	{
		first_newline_pos = first_newline - log_filtered;
		
		if (first_newline_pos < (strlen (log_filtered) - 1))
		{
			first_log_line = g_strndup (log_filtered, first_newline_pos);
			short_log = g_strconcat (first_log_line, " ...", NULL);
			g_free (first_log_line);
		}
		else /* There could be a newline and nothing after it... */
			short_log = g_strndup (log_filtered, first_newline_pos);
	}
	else
		short_log = g_strdup (log_filtered);
	
	self->priv->short_log = g_strdup (short_log);
	g_free (short_log);
	
	return self;
}

void
svn_log_entry_destroy (SvnLogEntry *self)
{
	g_object_unref (self);
}

gchar *
svn_log_entry_get_author (SvnLogEntry *self)
{
	return g_strdup (self->priv->author);
}

gchar *
svn_log_entry_get_date (SvnLogEntry *self)
{
	return g_strdup (self->priv->date);
}

glong
svn_log_entry_get_revision (SvnLogEntry *self)
{
	return self->priv->revision;
}

gchar *
svn_log_entry_get_short_log (SvnLogEntry *self)
{
	return g_strdup (self->priv->short_log);
}

gchar *
svn_log_entry_get_full_log (SvnLogEntry *self)
{
	return g_strdup (self->priv->log);
}

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

#ifndef _SVN_LOG_ENTRY_H_
#define _SVN_LOG_ENTRY_H_

#include <glib-object.h>
#include <string.h>
#include <ctype.h>

G_BEGIN_DECLS

#define SVN_TYPE_LOG_ENTRY             (svn_log_entry_get_type ())
#define SVN_LOG_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_TYPE_LOG_ENTRY, SvnLogEntry))
#define SVN_LOG_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_TYPE_LOG_ENTRY, SvnLogEntryClass))
#define SVN_IS_LOG_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_TYPE_LOG_ENTRY))
#define SVN_IS_LOG_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_TYPE_LOG_ENTRY))
#define SVN_LOG_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SVN_TYPE_LOG_ENTRY, SvnLogEntryClass))

typedef struct _SvnLogEntryClass SvnLogEntryClass;
typedef struct _SvnLogEntry SvnLogEntry;
typedef struct _SvnLogEntryPriv SvnLogEntryPriv;

struct _SvnLogEntryClass
{
	GObjectClass parent_class;
};

struct _SvnLogEntry
{
	GObject parent_instance;
	
	SvnLogEntryPriv *priv;
};

GType svn_log_entry_get_type (void) G_GNUC_CONST;
SvnLogEntry *svn_log_entry_new (gchar *author, gchar *date, 
								glong revision, gchar *log);
void svn_log_entry_destroy (SvnLogEntry *self);
gchar *svn_log_entry_get_author (SvnLogEntry *self);
gchar *svn_log_entry_get_date (SvnLogEntry *self);
glong svn_log_entry_get_revision (SvnLogEntry *self);
gchar *svn_log_entry_get_short_log (SvnLogEntry *self);
gchar *svn_log_entry_get_full_log (SvnLogEntry *self);

G_END_DECLS

#endif /* _SVN_LOG_ENTRY_H_ */

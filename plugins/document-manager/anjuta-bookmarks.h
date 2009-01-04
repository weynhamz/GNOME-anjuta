/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2008 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_BOOKMARKS_H_
#define _ANJUTA_BOOKMARKS_H_

#include <glib-object.h>

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/anjuta-session.h>
#include "plugin.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_BOOKMARKS             (anjuta_bookmarks_get_type ())
#define ANJUTA_BOOKMARKS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_BOOKMARKS, AnjutaBookmarks))
#define ANJUTA_BOOKMARKS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_BOOKMARKS, AnjutaBookmarksClass))
#define ANJUTA_IS_BOOKMARKS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_BOOKMARKS))
#define ANJUTA_IS_BOOKMARKS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_BOOKMARKS))
#define ANJUTA_BOOKMARKS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_BOOKMARKS, AnjutaBookmarksClass))

typedef struct _AnjutaBookmarksClass AnjutaBookmarksClass;
typedef struct _AnjutaBookmarks AnjutaBookmarks;

struct _AnjutaBookmarksClass
{
	GObjectClass parent_class;
};

struct _AnjutaBookmarks
{
	GObject parent_instance;
};

GType anjuta_bookmarks_get_type (void) G_GNUC_CONST;
AnjutaBookmarks* anjuta_bookmarks_new (DocmanPlugin* docman);
void anjuta_bookmarks_add (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor, gint line, 
						   const gchar* title, gboolean use_selection);
void anjuta_bookmarks_add_file (AnjutaBookmarks* bookmarks, GFile* file, 
								gint line, const gchar* title);
void anjuta_bookmarks_remove (AnjutaBookmarks* bookmarks);
void anjuta_bookmarks_session_save (AnjutaBookmarks* bookmarks, AnjutaSession* session);
void anjuta_bookmarks_session_load (AnjutaBookmarks* bookmarks, AnjutaSession* session);

G_END_DECLS

#endif /* _ANJUTA_BOOKMARKS_H_ */

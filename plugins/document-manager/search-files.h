/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2012 <jhs@gnome.org>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SEARCH_FILES_H_
#define _SEARCH_FILES_H_

#include "anjuta-docman.h"
#include "search-box.h"

G_BEGIN_DECLS

#define SEARCH_TYPE_FILES             (search_files_get_type ())
#define SEARCH_FILES(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEARCH_TYPE_FILES, SearchFiles))
#define SEARCH_FILES_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SEARCH_TYPE_FILES, SearchFilesClass))
#define SEARCH_IS_FILES(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEARCH_TYPE_FILES))
#define SEARCH_IS_FILES_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SEARCH_TYPE_FILES))
#define SEARCH_FILES_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SEARCH_TYPE_FILES, SearchFilesClass))

typedef struct _SearchFilesClass SearchFilesClass;
typedef struct _SearchFiles SearchFiles;
typedef struct _SearchFilesPrivate SearchFilesPrivate;

struct _SearchFilesClass
{
	GObjectClass parent_class;
};

struct _SearchFiles
{
	GObject parent_instance;

	SearchFilesPrivate* priv;
};

GType search_files_get_type (void) G_GNUC_CONST;
SearchFiles* search_files_new (AnjutaDocman* docman, SearchBox* search_box);

void search_files_present (SearchFiles* files);

G_END_DECLS

#endif /* _SEARCH_FILES_H_ */

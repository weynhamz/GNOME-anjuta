/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
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

#ifndef _ANJUTA_FILE_DROP_ENTRY_H_
#define _ANJUTA_FILE_DROP_ENTRY_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-drop-entry.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_FILE_DROP_ENTRY             (anjuta_file_drop_entry_get_type ())
#define ANJUTA_FILE_DROP_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_FILE_DROP_ENTRY, AnjutaFileDropEntry))
#define ANJUTA_FILE_DROP_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_FILE_DROP_ENTRY, AnjutaFileDropEntryClass))
#define ANJUTA_IS_FILE_DROP_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_FILE_DROP_ENTRY))
#define ANJUTA_IS_FILE_DROP_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_FILE_DROP_ENTRY))
#define ANJUTA_FILE_DROP_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_FILE_DROP_ENTRY, AnjutaFileDropEntryClass))

typedef struct _AnjutaFileDropEntryClass AnjutaFileDropEntryClass;
typedef struct _AnjutaFileDropEntry AnjutaFileDropEntry;
typedef struct _AnjutaFileDropEntryPriv AnjutaFileDropEntryPriv;

struct _AnjutaFileDropEntryClass
{
	AnjutaDropEntryClass parent_class;
};

struct _AnjutaFileDropEntry
{
	AnjutaDropEntry parent_instance;

	AnjutaFileDropEntryPriv *priv;
};

GType anjuta_file_drop_entry_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_file_drop_entry_new (void);
void anjuta_file_drop_entry_set_relative_path (AnjutaFileDropEntry *self, 
                                               const gchar *path);

G_END_DECLS

#endif /* _ANJUTA_FILE_DROP_ENTRY_H_ */

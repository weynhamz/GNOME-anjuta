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

#ifndef _ANJUTA_ENTRY_H_
#define _ANJUTA_ENTRY_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_ENTRY             (anjuta_entry_get_type ())
#define ANJUTA_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_ENTRY, AnjutaEntry))
#define ANJUTA_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_ENTRY, AnjutaEntryClass))
#define ANJUTA_IS_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_ENTRY))
#define ANJUTA_IS_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_ENTRY))
#define ANJUTA_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_ENTRY, AnjutaEntryClass))

typedef struct _AnjutaEntryClass AnjutaEntryClass;
typedef struct _AnjutaEntry AnjutaEntry;
typedef struct _AnjutaEntryPriv AnjutaEntryPriv;

struct _AnjutaEntryClass
{
	GtkEntryClass parent_class;
};

struct _AnjutaEntry
{
	GtkEntry parent_instance;

	AnjutaEntryPriv *priv;
};

GType anjuta_entry_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_entry_new (void);
const gchar *anjuta_entry_get_text (AnjutaEntry *self);
gchar *anjuta_entry_dup_text (AnjutaEntry *self);
void anjuta_entry_set_text (AnjutaEntry *self, const gchar *text);
gboolean anjuta_entry_is_showing_help_text (AnjutaEntry *self);

G_END_DECLS

#endif /* _ANJUTA_ENTRY_H_ */

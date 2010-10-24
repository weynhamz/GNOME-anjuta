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

#ifndef _ANJUTA_DROP_ENTRY_H_
#define _ANJUTA_DROP_ENTRY_H_

#include <glib-object.h>
#include "anjuta-entry.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_DROP_ENTRY             (anjuta_drop_entry_get_type ())
#define ANJUTA_DROP_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DROP_ENTRY, AnjutaDropEntry))
#define ANJUTA_DROP_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_DROP_ENTRY, AnjutaDropEntryClass))
#define ANJUTA_IS_DROP_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_DROP_ENTRY))
#define ANJUTA_IS_DROP_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DROP_ENTRY))
#define ANJUTA_DROP_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_DROP_ENTRY, AnjutaDropEntryClass))

typedef struct _AnjutaDropEntryClass AnjutaDropEntryClass;
typedef struct _AnjutaDropEntry AnjutaDropEntry;

struct _AnjutaDropEntryClass
{
	AnjutaEntryClass parent_class;
};

struct _AnjutaDropEntry
{
	AnjutaEntry parent_instance;
};

GType anjuta_drop_entry_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_drop_entry_new (void);

G_END_DECLS

#endif /* _ANJUTA_DROP_ENTRY_H_ */

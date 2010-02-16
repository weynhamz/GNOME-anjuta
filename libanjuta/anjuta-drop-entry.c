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

#include "anjuta-drop-entry.h"

struct _AnjutaDropEntryPriv
{
};

G_DEFINE_TYPE (AnjutaDropEntry, anjuta_drop_entry, GTK_TYPE_ENTRY);

static void
anjuta_drop_entry_init (AnjutaDropEntry *self)
{
	self->priv = g_new0 (AnjutaDropEntryPriv, 1);

	gtk_widget_set_size_request (GTK_WIDGET (self), -1, 40);
}

static void
anjuta_drop_entry_finalize (GObject *object)
{
	AnjutaDropEntry *self;

	self = ANJUTA_DROP_ENTRY (object);

	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_drop_entry_parent_class)->finalize (object);
}

static void
anjuta_drop_entry_class_init (AnjutaDropEntryClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

/* May need this later */
#if 0
	GtkEntryClass* parent_class = GTK_ENTRY_CLASS (klass);
#endif

	object_class->finalize = anjuta_drop_entry_finalize;
}


GtkWidget *
anjuta_drop_entry_new (void)
{
	return g_object_new (ANJUTA_TYPE_DROP_ENTRY, NULL);
}

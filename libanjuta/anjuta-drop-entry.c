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

/* DnD targets */
enum
{
	DND_TYPE_STRING
};

static GtkTargetEntry dnd_target_entries[] = 
{
	{
		"STRING",
		0,
		DND_TYPE_STRING
	},
	{
		"text/plain",
		0,
		DND_TYPE_STRING
	}
};

G_DEFINE_TYPE (AnjutaDropEntry, anjuta_drop_entry, ANJUTA_TYPE_ENTRY);

static void
anjuta_drop_entry_init (AnjutaDropEntry *self)
{
	gtk_drag_dest_set (GTK_WIDGET (self), 
	                   GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT, 
	                   dnd_target_entries,
	                   G_N_ELEMENTS (dnd_target_entries), GDK_ACTION_COPY);
}

static void
anjuta_drop_entry_finalize (GObject *object)
{
	AnjutaDropEntry *self;

	self = ANJUTA_DROP_ENTRY (object);

	G_OBJECT_CLASS (anjuta_drop_entry_parent_class)->finalize (object);
}

static void
anjuta_drop_entry_drag_data_received (GtkWidget *widget, 
                                      GdkDragContext *context, gint x, gint y,
                                      GtkSelectionData *data, guint target_type,
                                      guint time)
{
	gboolean success;
	gboolean delete;

	success = FALSE;
	delete = FALSE;

	if ((data != NULL) && 
	    (gtk_selection_data_get_length (data) >= 0))
	{
		delete = (gdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE);

		if (target_type == DND_TYPE_STRING)
		{
			anjuta_entry_set_text (ANJUTA_ENTRY (widget), 
			                       (const gchar *) gtk_selection_data_get_data (data));
			success = TRUE;
		}
	}

	gtk_drag_finish (context, success, delete, time);
}

static gboolean
anjuta_drop_entry_drag_drop (GtkWidget *widget, GdkDragContext *context, 
                             gint x, gint y, guint time)
{
	GdkAtom target_type;

	target_type = gtk_drag_dest_find_target (widget, context, NULL);

	if (target_type != GDK_NONE)
		gtk_drag_get_data (widget, context, target_type, time);
	else
		gtk_drag_finish (context, FALSE, FALSE, time);

	return TRUE;
}

static void
anjuta_drop_entry_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (anjuta_drop_entry_parent_class)->size_request (widget,
	                                                                 requisition);

	/* Make the entry box 40 pixels tall so that it is easier to drag into. */
	requisition->height = 40;
}

static void
anjuta_drop_entry_class_init (AnjutaDropEntryClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = anjuta_drop_entry_finalize;
	widget_class->drag_data_received = anjuta_drop_entry_drag_data_received;
	widget_class->drag_drop = anjuta_drop_entry_drag_drop;
	widget_class->size_request = anjuta_drop_entry_size_request;
	
}

GtkWidget *
anjuta_drop_entry_new (void)
{
	return g_object_new (ANJUTA_TYPE_DROP_ENTRY, NULL);
}

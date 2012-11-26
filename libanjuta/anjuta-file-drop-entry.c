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

#include "anjuta-file-drop-entry.h"

enum
{
	PROP_0,

	PROP_RELATIVE_PATH
};

/* DND Targets */
static GtkTargetEntry dnd_target_entries[] =
{
	{
		"text/uri-list",
		0,
		0
	}
};

struct _AnjutaFileDropEntryPriv
{
	gchar *relative_path;
};

G_DEFINE_TYPE (AnjutaFileDropEntry, anjuta_file_drop_entry, ANJUTA_TYPE_DROP_ENTRY);

static void
anjuta_file_drop_entry_init (AnjutaFileDropEntry *self)
{
	self->priv = g_new0 (AnjutaFileDropEntryPriv, 1);

	gtk_drag_dest_set (GTK_WIDGET (self), 
	                   GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT, 
	                   dnd_target_entries,
	                   G_N_ELEMENTS (dnd_target_entries), 
	                   GDK_ACTION_COPY | GDK_ACTION_MOVE);
}

static void
anjuta_file_drop_entry_finalize (GObject *object)
{
	AnjutaFileDropEntry *self;

	self = ANJUTA_FILE_DROP_ENTRY (object);

	g_free (self->priv->relative_path);
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_file_drop_entry_parent_class)->finalize (object);
}

static void
anjuta_file_drop_entry_set_property (GObject *object, guint prop_id, 
                                     const GValue *value, GParamSpec *pspec)
{
	AnjutaFileDropEntry *self;

	g_return_if_fail (ANJUTA_IS_FILE_DROP_ENTRY (object));

	self = ANJUTA_FILE_DROP_ENTRY (object);

	switch (prop_id)
	{
		case PROP_RELATIVE_PATH:
			g_free (self->priv->relative_path);

			self->priv->relative_path = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
anjuta_file_drop_entry_get_property (GObject *object, guint prop_id, 
                                     GValue *value, GParamSpec *pspec)
{
	AnjutaFileDropEntry *self;

	g_return_if_fail (ANJUTA_IS_FILE_DROP_ENTRY (object));

	self = ANJUTA_FILE_DROP_ENTRY (object);

	switch (prop_id)
	{
		case PROP_RELATIVE_PATH:
			g_value_set_string (value, self->priv->relative_path);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
anjuta_file_drop_entry_drag_data_received (GtkWidget *widget, 
                                           GdkDragContext *context, gint x, gint y,
                                           GtkSelectionData *data, guint target_type,
                                           guint time)
{
	AnjutaFileDropEntry *self;
	gboolean success;
	gchar **uri_list;
	GFile *parent_file;
	GFile *file;
	gchar *path;

	self = ANJUTA_FILE_DROP_ENTRY (widget);
	success = FALSE;

	if ((data != NULL) && 
	    (gtk_selection_data_get_length (data) >= 0))
	{
		if (target_type == 0)
		{
			uri_list = gtk_selection_data_get_uris (data);
			parent_file = NULL;

			if (self->priv->relative_path)
				parent_file = g_file_new_for_path (self->priv->relative_path);

			/* Take only the first file */
			file = g_file_new_for_uri (uri_list[0]);

			if (parent_file)
			{
				path = g_file_get_relative_path (parent_file, file);

				g_object_unref (parent_file);
			}
			else
				path = g_file_get_path (file);

			if (path)
			{
				anjuta_entry_set_text (ANJUTA_ENTRY (self), path);

				g_free (path);
			}
			
			success = TRUE;

			g_object_unref (file);
			g_strfreev (uri_list);
		}
	}

	/* Do not delete source data */
	gtk_drag_finish (context, success, FALSE, time);
}

static gboolean
anjuta_file_drop_entry_drag_drop (GtkWidget *widget, GdkDragContext *context, 
                                  gint x, gint y, guint time)
{
	GdkAtom target_type;

	target_type = gtk_drag_dest_find_target (widget, context, NULL);

	g_print ("Drag drop target type: %p\n", GDK_ATOM_TO_POINTER (target_type));

	if (target_type != GDK_NONE)
		gtk_drag_get_data (widget, context, target_type, time);
	else
		gtk_drag_finish (context, FALSE, FALSE, time);

	return TRUE;
}

static void
anjuta_file_drop_entry_class_init (AnjutaFileDropEntryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = anjuta_file_drop_entry_finalize;
	object_class->set_property = anjuta_file_drop_entry_set_property;
	object_class->get_property = anjuta_file_drop_entry_get_property;
	widget_class->drag_data_received = anjuta_file_drop_entry_drag_data_received;
	widget_class->drag_drop = anjuta_file_drop_entry_drag_drop;

	g_object_class_install_property (object_class,
	                                 PROP_RELATIVE_PATH,
	                                 g_param_spec_string ("relative-path",
	                                                      "relative-path",
	                                                      _("Path that dropped files should be relative to"),
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

GtkWidget *
anjuta_file_drop_entry_new (void)
{
	return g_object_new (ANJUTA_TYPE_FILE_DROP_ENTRY, NULL);
}

void
anjuta_file_drop_entry_set_relative_path (AnjutaFileDropEntry *self, 
                                          const gchar *path)
{
	g_object_set (G_OBJECT (self), "relative-path", path, NULL);
}

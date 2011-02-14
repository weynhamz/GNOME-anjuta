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

#include "anjuta-column-text-view.h"

struct _AnjutaColumnTextViewPriv
{
	GtkWidget *text_view;
	GtkWidget *column_label;
};

G_DEFINE_TYPE (AnjutaColumnTextView, anjuta_column_text_view, GTK_TYPE_VBOX);

static void
set_text_view_column_label (GtkTextBuffer *buffer, 
                            GtkTextIter *location,
                            GtkTextMark *mark,
                            GtkLabel *column_label)
{
	gint column;
	gchar *text;

	column = gtk_text_iter_get_line_offset (location) + 1;
	text = g_strdup_printf (_("Column %i"), column);

	gtk_label_set_text (column_label, text);

	g_free (text);
}

static void
anjuta_column_text_view_init (AnjutaColumnTextView *self)
{
	GtkWidget *scrolled_window;
	GtkTextBuffer *text_buffer;

	self->priv = g_new0 (AnjutaColumnTextViewPriv, 1);

	/* Text view */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
	                                     GTK_SHADOW_IN);

	self->priv->text_view = gtk_text_view_new ();

	gtk_container_add (GTK_CONTAINER (scrolled_window), self->priv->text_view);
	gtk_box_pack_start (GTK_BOX (self), scrolled_window, TRUE, TRUE, 0);

	/* Column label */
	self->priv->column_label = gtk_label_new (_("Column 1"));

	/* Right justify the label text */
	g_object_set (G_OBJECT (self->priv->column_label), "xalign", 1.0, NULL);

	/* Column change handling */
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->text_view));

	g_signal_connect (G_OBJECT (text_buffer), "mark-set",
	                  G_CALLBACK (set_text_view_column_label),
	                  self->priv->column_label);

	gtk_box_pack_start (GTK_BOX (self), self->priv->column_label, FALSE, FALSE,
	                    0);

	/* Allow focusing */
	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

	gtk_widget_show_all (GTK_WIDGET (self));
}

static void
anjuta_column_text_view_finalize (GObject *object)
{
	AnjutaColumnTextView *self;

	self = ANJUTA_COLUMN_TEXT_VIEW (object);

	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_column_text_view_parent_class)->finalize (object);
}

static void
anjuta_column_text_view_grab_focus (GtkWidget *widget)
{
	AnjutaColumnTextView *self;

	self = ANJUTA_COLUMN_TEXT_VIEW (widget);

	gtk_widget_grab_focus (self->priv->text_view);
}

static void
anjuta_column_text_view_class_init (AnjutaColumnTextViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = anjuta_column_text_view_finalize;
	widget_class->grab_focus = anjuta_column_text_view_grab_focus;
}


GtkWidget *
anjuta_column_text_view_new (void)
{
	return g_object_new (ANJUTA_TYPE_COLUMN_TEXT_VIEW, NULL);
}

gchar *
anjuta_column_text_view_get_text (AnjutaColumnTextView *self)
{
	GtkTextBuffer *text_buffer;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *text;

	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->text_view));
	
	gtk_text_buffer_get_start_iter (text_buffer, &start_iter);
	gtk_text_buffer_get_end_iter (text_buffer, &end_iter) ;

	text = gtk_text_buffer_get_text (text_buffer, &start_iter, &end_iter, FALSE);
	
	return text;
}

GtkTextBuffer *
anjuta_column_text_view_get_buffer (AnjutaColumnTextView *self)
{
	return gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->text_view));
} 
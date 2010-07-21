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

#ifndef _ANJUTA_COLUMN_TEXT_VIEW_H_
#define _ANJUTA_COLUMN_TEXT_VIEW_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_COLUMN_TEXT_VIEW             (anjuta_column_text_view_get_type ())
#define ANJUTA_COLUMN_TEXT_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_COLUMN_TEXT_VIEW, AnjutaColumnTextView))
#define ANJUTA_COLUMN_TEXT_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_COLUMN_TEXT_VIEW, AnjutaColumnTextViewClass))
#define ANJUTA_IS_COLUMN_TEXT_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_COLUMN_TEXT_VIEW))
#define ANJUTA_IS_COLUMN_TEXT_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_COLUMN_TEXT_VIEW))
#define ANJUTA_COLUMN_TEXT_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_COLUMN_TEXT_VIEW, AnjutaColumnTextViewClass))

typedef struct _AnjutaColumnTextViewClass AnjutaColumnTextViewClass;
typedef struct _AnjutaColumnTextView AnjutaColumnTextView;
typedef struct _AnjutaColumnTextViewPriv AnjutaColumnTextViewPriv;

struct _AnjutaColumnTextViewClass
{
	GtkVBoxClass parent_class;
};

struct _AnjutaColumnTextView
{
	GtkVBox parent_instance;

	AnjutaColumnTextViewPriv *priv;
};

GType anjuta_column_text_view_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_column_text_view_new (void);
gchar *anjuta_column_text_view_get_text (AnjutaColumnTextView *self);

G_END_DECLS

#endif /* _ANJUTA_COLUMN_TEXT_VIEW_H_ */

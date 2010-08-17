/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-editor.h
    Copyright (C) Dragos Dena 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#ifndef __SNIPPETS_EDITOR_H__
#define __SNIPPETS_EDITOR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "snippet.h"
#include "snippets-db.h"

G_BEGIN_DECLS

typedef struct _SnippetsEditor SnippetsEditor;
typedef struct _SnippetsEditorPrivate SnippetsEditorPrivate;
typedef struct _SnippetsEditorClass SnippetsEditorClass;

#define ANJUTA_TYPE_SNIPPETS_EDITOR            (snippets_editor_get_type ())
#define ANJUTA_SNIPPETS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPETS_EDITOR, SnippetsEditor))
#define ANJUTA_SNIPPETS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPETS_EDITOR, SnippetsEditorClass))
#define ANJUTA_IS_SNIPPETS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPETS_EDITOR))
#define ANJUTA_IS_SNIPPETS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPETS_EDITOR))

struct _SnippetsEditor
{
	GtkHBox parent;

	/*< private >*/
	SnippetsEditorPrivate *priv;
};

struct _SnippetsEditorClass
{
	GtkHBoxClass parent_class;

	/* Signals */
	void (*snippet_saved)    (SnippetsEditor *snippets_editor,
	                          GObject *snippet);
	void (*close_request)    (SnippetsEditor *snippets_editor);
};


GType            snippets_editor_get_type               (void) G_GNUC_CONST;
SnippetsEditor*  snippets_editor_new                    (SnippetsDB *snippets_db);

void             snippets_editor_set_snippet            (SnippetsEditor *snippets_editor,
                                                         AnjutaSnippet *snippet);
void             snippets_editor_set_snippet_new        (SnippetsEditor *snippets_editor);

G_END_DECLS

#endif /* __SNIPPETS_EDITOR_H__ */


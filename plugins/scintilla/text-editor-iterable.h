/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * text-editor-iterable.h
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _TEXT_EDITOR_ITERABLE_H_
#define _TEXT_EDITOR_ITERABLE_H_

#include <glib.h>
#include "text_editor.h"

G_BEGIN_DECLS

#define TYPE_TEXT_EDITOR_CELL        (text_editor_cell_get_type ())
#define TEXT_EDITOR_CELL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TEXT_EDITOR_CELL, TextEditorCell))
#define TEXT_EDITOR_CELL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), TYPE_TEXT_EDITOR_CELL, TextEditorCellClass))
#define IS_TEXT_EDITOR_CELL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TEXT_EDITOR_CELL))
#define IS_TEXT_EDITOR_CELL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_TEXT_EDITOR_CELL))

typedef struct _TextEditorCellPrivate TextEditorCellPrivate;

typedef struct _TextEditorCell
{
	GObject parent;
	TextEditorCellPrivate *priv;
} TextEditorCell;

typedef struct _TextEditorCellClass
{
	GObjectClass parent_class;
} TextEditorCellClass;

GType text_editor_cell_get_type (void);

/* New instance of TextEditorCell */
TextEditorCell* text_editor_cell_new (TextEditor *editor, gint position);
TextEditor *text_editor_cell_get_editor (TextEditorCell *iter);
void text_editor_cell_set_position (TextEditorCell *iter, gint position);
gint text_editor_cell_get_position (TextEditorCell *iter);

G_END_DECLS

#endif /* _TEXT_EDITOR_ITERABLE_H_ */

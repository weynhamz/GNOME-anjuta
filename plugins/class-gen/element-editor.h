/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  element-editor.h
 *  Copyright (C) 2006 Armin Burgmeier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __CLASSGEN_ELEMENT_EDITOR_H__
#define __CLASSGEN_ELEMENT_EDITOR_H__

#include <plugins/project-wizard/values.h>

#include <gtk/gtk.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define CG_TYPE_ELEMENT_EDITOR             (cg_element_editor_get_type ())
#define CG_ELEMENT_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CG_TYPE_ELEMENT_EDITOR, CgElementEditor))
#define CG_ELEMENT_EDITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CG_TYPE_ELEMENT_EDITOR, CgElementEditorClass))
#define CG_IS_ELEMENT_EDITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CG_TYPE_ELEMENT_EDITOR))
#define CG_IS_ELEMENT_EDITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CG_TYPE_ELEMENT_EDITOR))
#define CG_ELEMENT_EDITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CG_TYPE_ELEMENT_EDITOR, CgElementEditorClass))

typedef struct _CgElementEditorClass CgElementEditorClass;
typedef struct _CgElementEditor CgElementEditor;

typedef enum
{
	CG_ELEMENT_EDITOR_COLUMN_LIST,
	CG_ELEMENT_EDITOR_COLUMN_FLAGS,
	CG_ELEMENT_EDITOR_COLUMN_STRING,
	CG_ELEMENT_EDITOR_COLUMN_ARGUMENTS
} CgElementEditorColumnType;

typedef struct CgElementEditorFlags_
{
	const gchar *name;
	const gchar *abbrevation;
} CgElementEditorFlags;

struct _CgElementEditorClass
{
	GObjectClass parent_class;
};

struct _CgElementEditor
{
	GObject parent_instance;
};

GType cg_element_editor_get_type (void) G_GNUC_CONST;

CgElementEditor *cg_element_editor_new (GtkTreeView *view,
                                        GtkButton *add_button,
                                        GtkButton *remove_button,
                                        guint n_columns,
                                        ...);

typedef void(*CgElementEditorTransformFunc) (GHashTable *, gpointer);

/* The CgElementEditorTransformFunc may be used to modify the values
 * before they are stored in the value heap. It is also possible to
 * remove some or to create new ones. You should keep the key string
 * static because it is not freed when the hash table is destroyed, but
 * the data string may be dynamically allocated, it will be freed
 * eventually. */
void cg_element_editor_set_values (CgElementEditor *editor,
                                   const gchar *name,
                                   GHashTable *values,
                                   CgElementEditorTransformFunc func,
                                   gpointer user_data,
                                   ...);

typedef gboolean(*CgElementEditorConditionFunc) (const gchar **, gpointer);

void cg_element_editor_set_value_count (CgElementEditor *editor,
                                        const gchar *name,
                                        GHashTable *values,
                                        CgElementEditorConditionFunc func,
                                        gpointer user_data);

G_END_DECLS

#endif /* __CLASSGEN_ELEMENT_EDITOR_H__ */

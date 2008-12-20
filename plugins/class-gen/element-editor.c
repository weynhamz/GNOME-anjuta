/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  element-editor.c
 *  Copyright (C) 2006 Armin Burgmeier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "element-editor.h"
#include "cell-renderer-flags.h"

#include <gtk/gtk.h>

typedef struct _CgElementEditorColumn CgElementEditorColumn;
struct _CgElementEditorColumn
{
	CgElementEditor *parent;
	CgElementEditorColumnType type;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
};

typedef struct _CgElementEditorReference CgElementEditorReference;
struct _CgElementEditorReference
{
	CgElementEditorColumn *column;
	gchar *path_str;
};

typedef struct _CgElementEditorPrivate CgElementEditorPrivate;
struct _CgElementEditorPrivate
{
	GtkTreeView *view;
	GtkTreeModel *list;

	guint n_columns;
	CgElementEditorColumn *columns;
	
	GtkButton *add_button;
	GtkButton *remove_button;
};

#define CG_ELEMENT_EDITOR_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE( \
		(o), \
		CG_TYPE_ELEMENT_EDITOR, \
		CgElementEditorPrivate \
	))

enum {
	PROP_0,

	/* Construct only */
	PROP_TREEVIEW
};

static GObjectClass *parent_class = NULL;

static CgElementEditorReference *
cg_element_editor_reference_new (CgElementEditorColumn *column,
                                 const gchar *path_str)
{
	CgElementEditorReference *ref;
	ref = g_new (CgElementEditorReference, 1);
	
	ref->column = column;
	ref->path_str = g_strdup (path_str);
	
	return ref;
}

static void
cg_element_editor_reference_free (CgElementEditorReference *ref)
{
	g_free (ref->path_str);
	g_free (ref);
}

static void
cg_element_editor_select (CgElementEditor *editor,
                          GtkTreePath *path,
                          guint column)
{
	CgElementEditorPrivate *priv;
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);
	
	if (column < priv->n_columns)
	{
		gtk_widget_grab_focus (GTK_WIDGET (priv->view));
		

		gtk_tree_view_scroll_to_cell (priv->view, path,
		                              priv->columns[column].column, FALSE,
		                              0.0, 0.0);

		gtk_tree_view_set_cursor_on_cell (priv->view, path,
		                                  priv->columns[column].column,
		                                  priv->columns[column].renderer,
		                                  TRUE);
	}
}

static gboolean
cg_element_editor_edited_idle_cb (gpointer user_data)
{
	CgElementEditorReference *ref;
	CgElementEditorPrivate *priv;
	GtkTreePath *path;

	ref = (CgElementEditorReference *) user_data;
	priv = CG_ELEMENT_EDITOR_PRIVATE (ref->column->parent);

	path = gtk_tree_path_new_from_string (ref->path_str);

	cg_element_editor_select (ref->column->parent, path,
	                          ref->column - priv->columns);

	gtk_tree_path_free (path);
	return FALSE;
}

static void
cg_element_editor_row_inserted_cb (G_GNUC_UNUSED GtkTreeModel *model,
                                   GtkTreePath *path,
                                   G_GNUC_UNUSED GtkTreeIter *iter,
                                   gpointer user_data)
{
	CgElementEditor *editor;
	CgElementEditorPrivate *priv;
	CgElementEditorReference* ref;
	gchar* path_str;

	editor = CG_ELEMENT_EDITOR (user_data);
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);
	
	path_str = gtk_tree_path_to_string(path);
	ref = cg_element_editor_reference_new (&priv->columns[0], path_str);
	g_free(path_str);

	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
	                 cg_element_editor_edited_idle_cb, ref,
	                 (GDestroyNotify) cg_element_editor_reference_free);
}

static void
cg_element_editor_list_edited_cb (G_GNUC_UNUSED GtkCellRendererText *renderer,
                                  const gchar *path_str,
                                  const gchar *text,
                                  gpointer user_data)
{
	CgElementEditorReference *new_ref;
	CgElementEditorColumn* column;
	CgElementEditorPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	column = (CgElementEditorColumn *) user_data;
	priv = CG_ELEMENT_EDITOR_PRIVATE (column->parent);

	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (priv->list, &iter, path);

	gtk_list_store_set (GTK_LIST_STORE (priv->list), &iter,
	                    column - priv->columns, text, -1);
	gtk_tree_path_free (path);

	if(column - priv->columns + 1 < priv->n_columns)
	{
		/* We do not immediately select the new column because if this entry
		 * caused the column to be resized we first want to get the resize done.
		 * Otherwise, the next editing widget would appear at the old position
		 * of the next column (before the resize of this one). */
		new_ref = cg_element_editor_reference_new (column + 1, path_str);
		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
		                 cg_element_editor_edited_idle_cb, new_ref,
		                 (GDestroyNotify) cg_element_editor_reference_free);
	}
}

static void
cg_element_editor_string_edited_cb (G_GNUC_UNUSED GtkCellRendererText
                                    *renderer,
                                    const gchar *path_str,
                                    const gchar *text,
                                    gpointer user_data)
{
	CgElementEditorColumn *column;
	CgElementEditorPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	column = (CgElementEditorColumn *) user_data;
	priv = CG_ELEMENT_EDITOR_PRIVATE (column->parent);

	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (priv->list, &iter, path);
	gtk_tree_path_free (path);

	gtk_list_store_set(GTK_LIST_STORE(priv->list), &iter,
	                   column - priv->columns, text, -1);
}

static void
cg_element_editor_string_activate_cb (G_GNUC_UNUSED GtkEntry *entry,
                                      gpointer user_data)
{
	CgElementEditorPrivate* priv;
	CgElementEditorReference *ref;
	CgElementEditorReference *new_ref;

	ref = (CgElementEditorReference *) user_data;
	priv = CG_ELEMENT_EDITOR_PRIVATE(ref->column->parent);

	/* We do not immediately select the new column because if this entry
	 * caused the column to be resized we first want to get the resize done.
	 * Otherwise, the next editing widget would appear at the old position
	 * of the next column (before the resize of this one). */
	if(ref->column - priv->columns + 1 < priv->n_columns)
	{
		new_ref = cg_element_editor_reference_new (ref->column + 1,
		                                           ref->path_str);

		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
		                 cg_element_editor_edited_idle_cb, new_ref,
		                 (GDestroyNotify) cg_element_editor_reference_free);
	}
}

static void
cg_element_editor_string_editing_started_cb (G_GNUC_UNUSED
                                             GtkCellRenderer *renderer,
                                             GtkCellEditable *editable,
                                             const gchar *path_str,
                                             gpointer user_data)
{
	if (GTK_IS_ENTRY (editable))
	{
		g_signal_connect_data (G_OBJECT (editable), "activate",
		    G_CALLBACK (cg_element_editor_string_activate_cb),
			cg_element_editor_reference_new (user_data, path_str),
			(GClosureNotify) cg_element_editor_reference_free,
			G_CONNECT_AFTER);
	}
}

static void
cg_element_editor_arguments_editing_started_cb (G_GNUC_UNUSED
                                                GtkCellRenderer *renderer,
                                                GtkCellEditable *editable,
                                                const gchar *path_str,
                                                gpointer user_data)
{
	const gchar *text;

	if (GTK_IS_ENTRY (editable))
	{
		text = gtk_entry_get_text (GTK_ENTRY (editable));
		if (text == NULL || *text == '\0')
		{
			gtk_entry_set_text (GTK_ENTRY (editable), "()");

			/* TODO: This does not work, although we are connected with
			 * G_CONNECT_AFTER... */
			gtk_editable_set_position (GTK_EDITABLE (editable), 1);
		}

		/* cg_element_editor_argumens_activate_cb would to exactly the same
		 * as cg_element_editor_string_activate_cb, so there is no
		 * extra function. */
		g_signal_connect_data (G_OBJECT (editable), "activate",
			G_CALLBACK (cg_element_editor_string_activate_cb),
			cg_element_editor_reference_new (user_data, path_str),
			(GClosureNotify) cg_element_editor_reference_free,
			G_CONNECT_AFTER);
	}
}

static void
cg_element_editor_add_button_clicked_cb (G_GNUC_UNUSED GtkButton *button,
                                         gpointer user_data)
{
	CgElementEditor *editor;
	CgElementEditorPrivate *priv;
	GtkTreeIter iter;
	
	editor = CG_ELEMENT_EDITOR (user_data);
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);

	gtk_list_store_append (GTK_LIST_STORE (priv->list), &iter);
}

static void
cg_element_editor_remove_button_clicked_cb (G_GNUC_UNUSED GtkButton *button,
                                            gpointer user_data)
{
	CgElementEditor *editor;
	CgElementEditorPrivate *priv;
	GtkTreeSelection *selection;
	GtkTreeIter *iter;
	GtkTreePath *path;
	GList *selected_rows;
	GList *selected_iters;
	GList *cur_item;
	
	editor = CG_ELEMENT_EDITOR (user_data);
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);
	selection = gtk_tree_view_get_selection (priv->view);
	selected_rows = gtk_tree_selection_get_selected_rows (selection, NULL);
	selected_iters = NULL;

	/* Convert paths to iters because changing the model borks offsets
	 * in paths, but since GtkListStore has GTK_TREE_MODEL_PERSIST set,
	 * iters continue to live after changing the model. */
	for (cur_item = selected_rows; cur_item != NULL; cur_item = cur_item->next)
	{
		path = cur_item->data;
		iter = g_new (GtkTreeIter, 1);

		gtk_tree_model_get_iter (priv->list, iter, path);
		selected_iters = g_list_prepend (selected_iters, iter);

		gtk_tree_path_free (path);
	}
	
	for (cur_item = selected_iters;
	     cur_item != NULL;
	     cur_item = cur_item->next)
	{
		iter = cur_item->data;
		gtk_list_store_remove (GTK_LIST_STORE (priv->list), iter);
		g_free (iter);
	}
	
	g_list_free (selected_rows);
	g_list_free (selected_iters);
}

static void
cg_element_editor_selection_changed_cb (GtkTreeSelection *selection,
                                        gpointer user_data)
{
	CgElementEditor *editor;
	CgElementEditorPrivate *priv;

	editor = CG_ELEMENT_EDITOR (user_data);
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);
	
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
		gtk_widget_set_sensitive (GTK_WIDGET (priv->remove_button), TRUE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (priv->remove_button), FALSE);
}

static void
cg_element_editor_init (CgElementEditor *element_editor)
{
	CgElementEditorPrivate *priv;
	priv = CG_ELEMENT_EDITOR_PRIVATE (element_editor);

	priv->view = NULL;
	priv->list = NULL;
	priv->n_columns = 0;
	priv->columns = NULL;
}

static void 
cg_element_editor_finalize (GObject *object)
{
	CgElementEditor *element_editor;
	CgElementEditorPrivate *priv;

	element_editor = CG_ELEMENT_EDITOR (object);
	priv = CG_ELEMENT_EDITOR_PRIVATE (element_editor);

	g_free (priv->columns);
	if (priv->list != NULL)
		g_object_unref (G_OBJECT (priv->list));

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cg_element_editor_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
	CgElementEditor *element_editor;
	CgElementEditorPrivate *priv;

	g_return_if_fail (CG_IS_ELEMENT_EDITOR (object));

	element_editor = CG_ELEMENT_EDITOR (object);
	priv = CG_ELEMENT_EDITOR_PRIVATE (element_editor);

	switch (prop_id)
	{
	case PROP_TREEVIEW:
		priv->view = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_element_editor_get_property (GObject *object,
                                guint prop_id,
                                GValue *value, 
                                GParamSpec *pspec)
{
	CgElementEditor *element_editor;
	CgElementEditorPrivate *priv;

	g_return_if_fail (CG_IS_ELEMENT_EDITOR (object));

	element_editor = CG_ELEMENT_EDITOR (object);
	priv = CG_ELEMENT_EDITOR_PRIVATE (element_editor);

	switch (prop_id)
	{
	case PROP_TREEVIEW:
		g_value_set_object (value, G_OBJECT (priv->view));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_element_editor_class_init(CgElementEditorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (CgElementEditorPrivate));

	object_class->finalize = cg_element_editor_finalize;
	object_class->set_property = cg_element_editor_set_property;
	object_class->get_property = cg_element_editor_get_property;

	g_object_class_install_property(object_class,
	                                PROP_TREEVIEW,
	                                g_param_spec_object("tree-view",
	                                                    "Tree view",
	                                                    "Tree view the element editor works on",
	                                                    GTK_TYPE_TREE_VIEW,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

GType
cg_element_editor_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (CgElementEditorClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cg_element_editor_class_init,
			NULL,
			NULL,
			sizeof (CgElementEditor),
			0,
			(GInstanceInitFunc) cg_element_editor_init,
			NULL
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "CgElementEditor",
		                                   &our_info, 0);
	}

	return our_type;
}

static void
cg_element_editor_init_list_renderer(CgElementEditorColumn *column,
                                     GType *type,
                                     va_list *arglist)
{
	GtkTreeModel *combo_list;
	const gchar **items;
	GtkTreeIter iter;	

	*type = G_TYPE_STRING;

	column->renderer = gtk_cell_renderer_combo_new ();
	combo_list = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));

	for (items = va_arg (*arglist, const gchar **);
	     *items != NULL;
	     ++ items)
	{
		gtk_list_store_append (GTK_LIST_STORE (combo_list), &iter);
		gtk_list_store_set (GTK_LIST_STORE (combo_list), &iter, 0, *items, -1);
	}

	g_object_set(column->renderer, "model", combo_list, "text-column", 0,
	             "editable", TRUE, "has-entry", FALSE, NULL);

	g_signal_connect (G_OBJECT (column->renderer), "edited",
	                  G_CALLBACK (cg_element_editor_list_edited_cb), column);

	g_object_unref (G_OBJECT (combo_list));
}

static void
cg_element_editor_init_flags_renderer (CgElementEditorColumn *column,
                                       GType *type,
                                       va_list *arglist)
{
	GtkTreeModel *combo_list;
	const CgElementEditorFlags *items;
	GtkTreeIter iter;
	
	*type = G_TYPE_STRING;
	
	column->renderer = cg_cell_renderer_flags_new ();
	combo_list = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING,
	                             G_TYPE_STRING));

	for (items = va_arg (*arglist, const CgElementEditorFlags *);
	     items->name != NULL;
	     ++ items)
	{
		gtk_list_store_append (GTK_LIST_STORE (combo_list), &iter);
		gtk_list_store_set (GTK_LIST_STORE (combo_list), &iter,
		                    0, items->name, 1, items->abbrevation, -1);
	}

	g_object_set (column->renderer, "model", combo_list, "text-column", 0,
	              "abbrevation_column", 1, "editable", TRUE, NULL);

	g_signal_connect (G_OBJECT (column->renderer), "edited",
	                  G_CALLBACK (cg_element_editor_list_edited_cb), column);

	g_object_unref (G_OBJECT (combo_list));
}

static void
cg_element_editor_init_string_renderer (CgElementEditorColumn *column,
                                        GType *type,
                                        G_GNUC_UNUSED va_list *arglist)
{
	*type = G_TYPE_STRING;
	column->renderer = gtk_cell_renderer_text_new ();
			
	g_object_set (G_OBJECT (column->renderer), "editable", TRUE, NULL);

	/* We intentionally do not only connect to the "edited" signal here
	 * because this one is also fired when the user types something in and
	 * then clicks somewhere else. In such a situation, s/he wants to continue
	 * editing somewhere else, but we would start editing the next column
	 * which seems wrong.
	 *
	 * The "edited" signal handler does only store the new text whereas the
	 * "editing-started" signal handler connects to the "activate" signal of
	 * the resulting entry. This way, pressing enter to terminate the editing
	 * goes on to the next column, but other ways to terminate the input
	 * process do not. */
	g_signal_connect_after (G_OBJECT (column->renderer), "edited",
	                        G_CALLBACK (cg_element_editor_string_edited_cb),
	                        column);

	g_signal_connect_after (G_OBJECT (column->renderer), "editing-started",
		G_CALLBACK(cg_element_editor_string_editing_started_cb), column);
}

static void
cg_element_editor_init_arguments_renderer (CgElementEditorColumn *column,
                                           GType *type,
                                           G_GNUC_UNUSED va_list *arglist)
{
	*type = G_TYPE_STRING;
	column->renderer = gtk_cell_renderer_text_new ();
	
	g_object_set (G_OBJECT (column->renderer), "editable", TRUE, NULL);
	
	/* Same as above */
	g_signal_connect_after (G_OBJECT (column->renderer), "edited",
	                        G_CALLBACK (cg_element_editor_string_edited_cb),
	                        column);

	g_signal_connect_after (G_OBJECT (column->renderer), "editing-started",
		G_CALLBACK(cg_element_editor_arguments_editing_started_cb), column);
}

CgElementEditor *
cg_element_editor_new (GtkTreeView *view,
                       GtkButton *add_button,
                       GtkButton *remove_button,
                       guint n_columns,
                       ...)
{
	CgElementEditor *editor;
	CgElementEditorPrivate *priv;
	CgElementEditorColumnType column;
	GtkTreeSelection *selection;
	const gchar *title;
	GType *types;
	va_list arglist;
	guint i;

	editor = CG_ELEMENT_EDITOR (g_object_new (CG_TYPE_ELEMENT_EDITOR,
	                                          "tree-view", view, NULL));
	
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);

	types = g_malloc (sizeof (GType) * n_columns);
	priv->n_columns = n_columns;
	priv->columns = g_malloc (sizeof (CgElementEditorColumn) * n_columns);

	va_start (arglist, n_columns);
	for (i = 0; i < n_columns; ++ i)
	{
		priv->columns[i].parent = editor;

		title = va_arg (arglist, const gchar *);
		column = va_arg (arglist, CgElementEditorColumnType);
		priv->columns[i].type = column;

		priv->columns[i].column = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (priv->columns[i].column, title);

		switch (column)
		{
		case CG_ELEMENT_EDITOR_COLUMN_LIST:
			cg_element_editor_init_list_renderer (&priv->columns[i],
			                                      &types[i], &arglist);
			break;
		case CG_ELEMENT_EDITOR_COLUMN_FLAGS:
			cg_element_editor_init_flags_renderer (&priv->columns[i],
			                                       &types[i], &arglist);
			break;
		case CG_ELEMENT_EDITOR_COLUMN_STRING:
			cg_element_editor_init_string_renderer (&priv->columns[i],
			                                        &types[i], &arglist);
			break;
		case CG_ELEMENT_EDITOR_COLUMN_ARGUMENTS:
			cg_element_editor_init_arguments_renderer (&priv->columns[i],
			                                           &types[i], &arglist);
			break;
		default:
			g_assert_not_reached ();
			break;
		}
		
		gtk_tree_view_column_pack_start (priv->columns[i].column,
		                                 priv->columns[i].renderer, TRUE);

		gtk_tree_view_append_column (view, priv->columns[i].column);
	}
	va_end (arglist);

	priv->list = GTK_TREE_MODEL (gtk_list_store_newv (n_columns, types));
	g_free (types);

	/* Second pass, associate attributes */
	for (i = 0; i < n_columns; ++ i)
	{
		switch (priv->columns[i].type)
		{
		case CG_ELEMENT_EDITOR_COLUMN_LIST:
		case CG_ELEMENT_EDITOR_COLUMN_FLAGS:
		case CG_ELEMENT_EDITOR_COLUMN_STRING:
		case CG_ELEMENT_EDITOR_COLUMN_ARGUMENTS:
			gtk_tree_view_column_add_attribute (priv->columns[i].column,
			                                    priv->columns[i].renderer,
			                                    "text", i);

			break;
		default:
			g_assert_not_reached ();
			break;
		}
	}

	g_signal_connect_after (G_OBJECT (priv->list), "row-inserted",
	                        G_CALLBACK (cg_element_editor_row_inserted_cb),
	                        editor);
	
	priv->add_button = add_button;
	priv->remove_button = remove_button;
	
	if(priv->add_button != NULL)
	{
		g_signal_connect (G_OBJECT(priv->add_button), "clicked",
		                  G_CALLBACK (cg_element_editor_add_button_clicked_cb),
		                  editor);
	}
	
	if(priv->remove_button != NULL)
	{
		g_signal_connect (G_OBJECT (priv->remove_button), "clicked",
			G_CALLBACK (cg_element_editor_remove_button_clicked_cb), editor);
	}
	
	selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	if(priv->remove_button != NULL)
	{	
		g_signal_connect(G_OBJECT (selection), "changed",
		                 G_CALLBACK (cg_element_editor_selection_changed_cb),
		                 editor);
	}

	gtk_tree_view_set_model (view, priv->list);
	return editor;
}

static void
cg_element_editor_set_valuesv_foreach_func (gpointer key,
                                            gpointer data,
                                            gpointer user_data)
{

	if (data)
	{
		GString *str;
		gchar *escaped;

		str = (GString*)user_data;
		escaped = g_strescape ((const gchar *) data, NULL);
	
		g_string_append (str, (const gchar *) key);
		g_string_append (str, "=\"");
		g_string_append (str, escaped);
		g_string_append (str, "\";");
		g_free (escaped);
	}
}

static void
cg_element_editor_set_valuesv (CgElementEditor *editor,
                               const gchar *name,
                               NPWValueHeap *values,
                               CgElementEditorTransformFunc func,
                               gpointer user_data,
                               const gchar **field_names)
{
	CgElementEditorPrivate *priv;
	GtkTreeIter iter;
	gboolean result;
	GString *value_str;
	gchar *value_name;
	GHashTable *table;
	gchar *single_value;
	NPWValue *value;
	guint32 i;
	guint32 row_counter;

	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);
	row_counter = 0;

	value_str = g_string_sized_new (256);

	for (result = gtk_tree_model_get_iter_first (priv->list, &iter);
	     result == TRUE;
	     result = gtk_tree_model_iter_next (priv->list, &iter))
	{
		value_name = g_strdup_printf ("%s[%d]", name, row_counter);

		table = g_hash_table_new_full (g_str_hash, g_str_equal,
		                               NULL, (GDestroyNotify) g_free);

		for (i = 0; i < priv->n_columns; ++ i)
		{
			gtk_tree_model_get (priv->list, &iter, i, &single_value, -1);
			g_hash_table_insert (table, (gpointer) field_names[i],
			                     single_value);
		}

		if(func != NULL) func (table, user_data);
	
		g_string_append_c (value_str, '{');
		g_hash_table_foreach (table,
		                      cg_element_editor_set_valuesv_foreach_func,
		                      value_str);
		g_string_append_c (value_str, '}');
		g_hash_table_destroy (table);

		value = npw_value_heap_find_value (values, value_name);

		npw_value_heap_set_value (values, value, value_str->str,
		                          NPW_VALID_VALUE);

		g_string_set_size (value_str, 0);
		g_free (value_name);
		
		++ row_counter;
	}
	
	g_string_free (value_str, TRUE);
}

void
cg_element_editor_set_values (CgElementEditor *editor,
                              const gchar *name,
                              NPWValueHeap *values,
                              CgElementEditorTransformFunc func,
                              gpointer user_data,
                              ...)
{
	const gchar **field_names;
	CgElementEditorPrivate *priv;
	va_list arglist;
	guint32 i;
	
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);

	field_names = g_malloc (sizeof (const gchar *) * priv->n_columns);
	va_start (arglist, user_data);

	for (i = 0; i < priv->n_columns; ++ i)
		field_names[i] = va_arg (arglist, const gchar*);

	cg_element_editor_set_valuesv (editor, name, values, func,
	                               user_data, field_names);

	va_end (arglist);
	g_free (field_names);
}

void
cg_element_editor_set_value_count (CgElementEditor *editor,
                                   const gchar *name,
                                   NPWValueHeap *values,
                                   CgElementEditorConditionFunc func,
                                   gpointer user_data)
{
	CgElementEditorPrivate* priv;
	GtkTreeIter iter;
	gboolean result;
	NPWValue *value;
	const gchar **vals;
	gchar count_str[16];
	guint count;
	guint i;
	
	priv = CG_ELEMENT_EDITOR_PRIVATE (editor);
	vals = g_malloc (priv->n_columns * sizeof (const gchar *));
	count = 0;

	for (result = gtk_tree_model_get_iter_first (priv->list, &iter);
	     result == TRUE;
	     result = gtk_tree_model_iter_next (priv->list, &iter))
	{
		for (i = 0; i < priv->n_columns; ++ i)
		{
			gtk_tree_model_get (priv->list, &iter, i, &vals[i], -1);
		}

		if (func == NULL)
		{
			++ count;
		}
		else if (func (vals, user_data) == TRUE)
		{
			++ count;
		}
	}
	
	g_free (vals);

	sprintf (count_str, "%u", count);
	value = npw_value_heap_find_value (values, name);
	npw_value_heap_set_value (values, value, count_str, NPW_VALID_VALUE);
}

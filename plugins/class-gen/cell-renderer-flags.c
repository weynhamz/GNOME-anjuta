/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  cell-renderer-flags.c
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

#include "cell-renderer-flags.h"
#include "combo-flags.h"

#include <gtk/gtk.h>

typedef struct _CgCellRendererFlagsPrivate CgCellRendererFlagsPrivate;
struct _CgCellRendererFlagsPrivate
{
	GtkTreeModel *model;
	gint text_column;
	gint abbr_column;

	GHashTable *edit_status;
	guint focus_out_id;
};

#define CG_CELL_RENDERER_FLAGS_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE( \
		(o), \
		CG_TYPE_CELL_RENDERER_FLAGS, \
		CgCellRendererFlagsPrivate \
	))

enum {
	PROP_0,

	PROP_MODEL,
	PROP_TEXT_COLUMN,
	PROP_ABBR_COLUMN
};

#define CG_CELL_RENDERER_FLAGS_PATH "cg-cell-renderer-flags-path"

static GtkCellRendererTextClass *parent_class = NULL;

static void
cg_cell_renderer_flags_init (CgCellRendererFlags *cell_renderer_flags)
{
	CgCellRendererFlagsPrivate *priv;
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (cell_renderer_flags);

	priv->model = NULL;
	priv->text_column = -1;
	priv->abbr_column = -1;

	priv->edit_status = NULL;
	priv->focus_out_id = 0;
}

static void 
cg_cell_renderer_flags_finalize (GObject *object)
{
	CgCellRendererFlags *cell_renderer_flags;
	CgCellRendererFlagsPrivate *priv;

	cell_renderer_flags = CG_CELL_RENDERER_FLAGS (object);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (cell_renderer_flags);

	if (priv->edit_status != NULL)
	{
		g_hash_table_destroy (priv->edit_status);
		priv->edit_status = NULL;
	}

	if (priv->model != NULL)
	{
		g_object_unref (G_OBJECT(priv->model));
		priv->model = NULL;
	}

	G_OBJECT_CLASS (parent_class)-> finalize(object);
}

static void
cg_cell_renderer_flags_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
	CgCellRendererFlags *renderer;
	CgCellRendererFlagsPrivate *priv;

	g_return_if_fail (CG_IS_CELL_RENDERER_FLAGS (object));

	renderer = CG_CELL_RENDERER_FLAGS (object);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (renderer);

	switch (prop_id)
	{
	case PROP_MODEL:
		if(priv->model != NULL) g_object_unref (G_OBJECT (priv->model));
		priv->model = GTK_TREE_MODEL (g_value_dup_object (value));
		break;
	case PROP_TEXT_COLUMN:
		priv->text_column = g_value_get_int (value);
		break;
	case PROP_ABBR_COLUMN:
		priv->abbr_column = g_value_get_int (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_cell_renderer_flags_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value, 
                                     GParamSpec *pspec)
{
	CgCellRendererFlags *renderer;
	CgCellRendererFlagsPrivate *priv;

	g_return_if_fail (CG_IS_CELL_RENDERER_FLAGS (object));

	renderer = CG_CELL_RENDERER_FLAGS (object);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (renderer);

	switch(prop_id)
	{
	case PROP_MODEL:
		g_value_set_object (value, G_OBJECT (priv->model));
		break;
	case PROP_TEXT_COLUMN:
		g_value_set_int (value, priv->text_column);
		break;
	case PROP_ABBR_COLUMN:
		g_value_set_int (value, priv->abbr_column);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_cell_renderer_flags_editing_done (GtkCellEditable *editable,
                                     G_GNUC_UNUSED gpointer data)
{
	CgCellRendererFlags *cell_flags;
	CgCellRendererFlagsPrivate *priv;

	const gchar *path;
	GString *str;
	gchar *abbr;
	GtkTreeIter iter;
	gboolean result;
	gboolean canceled;

	cell_flags = CG_CELL_RENDERER_FLAGS (data);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (cell_flags);
	
	g_assert (priv->edit_status != NULL);

	if (priv->focus_out_id > 0)
	{
		g_signal_handler_disconnect (G_OBJECT (editable), priv->focus_out_id);
		priv->focus_out_id = 0;
	}

	canceled = cg_combo_flags_editing_canceled (CG_COMBO_FLAGS (editable));
	gtk_cell_renderer_stop_editing (GTK_CELL_RENDERER(cell_flags), canceled);

	if (canceled == FALSE)
	{
		str = g_string_sized_new (128);

		/* We do not just call g_hash_table_foreach to get the flags
		 * in the correct order. */
		for (result = gtk_tree_model_get_iter_first (priv->model, &iter);
		     result != FALSE;
		     result = gtk_tree_model_iter_next (priv->model, &iter))
		{
			gtk_tree_model_get (priv->model, &iter,
			                    priv->abbr_column, &abbr, -1);

			if (g_hash_table_lookup (priv->edit_status, abbr) != NULL)
			{
				if (str->len > 0) g_string_append_c (str, '|');
				g_string_append (str, abbr);
			}
			
			g_free (abbr);
		}

		path = g_object_get_data (G_OBJECT (editable),
		                          CG_CELL_RENDERER_FLAGS_PATH);

		g_signal_emit_by_name (G_OBJECT (cell_flags), "edited",
		                       path, str->str);

		g_string_free (str, TRUE);
	}
	
	g_hash_table_destroy (priv->edit_status);
	priv->edit_status = NULL;
}

static void
cg_cell_renderer_flags_selected (CgComboFlags *combo,
                                 GtkTreeIter *iter,
                                 CgComboFlagsSelectionType type,
                                 gpointer user_data)
{
	CgCellRendererFlags *cell_flags;
	CgCellRendererFlagsPrivate *priv;
	gpointer result;
	gchar *name;
	gchar *abbr;

	cell_flags = CG_CELL_RENDERER_FLAGS (user_data);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (cell_flags);

	gtk_tree_model_get (priv->model, iter, priv->text_column, &name,
	                    priv->abbr_column, &abbr, -1);
	
	g_assert (priv->edit_status != NULL);
	result = g_hash_table_lookup (priv->edit_status, abbr);

	/* abbr needs not to be freed if it gets inserted into the hash table
	 * because the hash table then takes ownership of it. */
	switch (type)
	{
	case CG_COMBO_FLAGS_SELECTION_NONE:
		g_free (abbr);
		break;
	case CG_COMBO_FLAGS_SELECTION_SELECT:
		if (GPOINTER_TO_INT(result) != 1)
			g_hash_table_insert (priv->edit_status, abbr, GINT_TO_POINTER (1));
		else
			g_free (abbr);

		break;
	case CG_COMBO_FLAGS_SELECTION_UNSELECT:
		if (GPOINTER_TO_INT (result) == 1)
			g_hash_table_remove(priv->edit_status, abbr);

		g_free (abbr);
		break;
	case CG_COMBO_FLAGS_SELECTION_TOGGLE:
		if (GPOINTER_TO_INT (result) == 1)
		{
			g_hash_table_remove (priv->edit_status, abbr);
			g_free(abbr);
		}
		else
		{
			g_hash_table_insert (priv->edit_status, abbr, GINT_TO_POINTER (1));
		}

		break;
	default:
		g_assert_not_reached ();
		break;
	}

	/* This is done to get GTK+ to re-render this row with the changed flag
	 * status that is set via the cell data func, but GTK+ does not call it
	 * again because it does not know that the hash table changed. There are
	 * probably better means to achieve this, but I am not aware of those. */
	gtk_list_store_set (GTK_LIST_STORE (priv->model), iter,
	                    priv->text_column, name, -1);

	g_free (name);
}

static gboolean
cg_cell_renderer_flags_focus_out_event (GtkWidget *widget,
                                        G_GNUC_UNUSED GdkEvent *event,
                                        gpointer data)
{
	cg_cell_renderer_flags_editing_done (GTK_CELL_EDITABLE (widget), data);
	return FALSE;
}

static void
cg_cell_renderer_flags_set_data_func (G_GNUC_UNUSED GtkCellLayout *cell_layout,
                                      GtkCellRenderer *cell,
	                                  GtkTreeModel *model,
	                                  GtkTreeIter *iter,
	                                  gpointer data)
{
	CgCellRendererFlags *cell_flags;
	CgCellRendererFlagsPrivate *priv;
	gchar *abbr;

	cell_flags = CG_CELL_RENDERER_FLAGS (data);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (cell_flags);

	if(priv->edit_status != NULL)
	{
		gtk_tree_model_get (model, iter, priv->abbr_column, &abbr, -1);

		if (g_hash_table_lookup (priv->edit_status, abbr) != NULL)
			g_object_set (G_OBJECT (cell), "active", TRUE, NULL);
		else
			g_object_set (G_OBJECT (cell), "active", FALSE, NULL);

		g_free (abbr);
	}
}

static GtkCellEditable *
cg_cell_renderer_flags_start_editing (GtkCellRenderer *cell,
                                      G_GNUC_UNUSED GdkEvent *event,
                                      G_GNUC_UNUSED GtkWidget *widget,
                                      const gchar *path,
                                      G_GNUC_UNUSED GdkRectangle
                                      *background_area,
                                      G_GNUC_UNUSED GdkRectangle *cell_area,
                                      G_GNUC_UNUSED GtkCellRendererState flags)
{
	CgCellRendererFlags *cell_flags;
	CgCellRendererFlagsPrivate *priv;
	GtkCellRendererText *cell_text;
	const gchar *prev;
	const gchar *pos;

	GtkWidget *combo;
	GtkCellRenderer *cell_combo_set;
	GtkCellRenderer *cell_combo_text;

	cell_flags = CG_CELL_RENDERER_FLAGS (cell);
	priv = CG_CELL_RENDERER_FLAGS_PRIVATE (cell_flags);

	cell_text = GTK_CELL_RENDERER_TEXT (cell);
	if (cell_text->editable == FALSE) return NULL;

	if (priv->model == NULL || priv->text_column < 0 || priv->abbr_column < 0)
		return NULL;

	cell_combo_set = gtk_cell_renderer_toggle_new ();
	cell_combo_text = gtk_cell_renderer_text_new ();

	combo = cg_combo_flags_new_with_model (priv->model);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
	                            cell_combo_set, FALSE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
	                            cell_combo_text, TRUE);

	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo),
	                               cell_combo_text, "text", priv->text_column);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
	                                    cell_combo_set,
                                        cg_cell_renderer_flags_set_data_func,
                                        cell_flags, NULL);

	g_object_set (G_OBJECT (cell_combo_set), "activatable", FALSE, NULL);

	/* Create hash table with current status. We could also operate
	 * directly on a string here, but a hash table is probably more
	 * efficient. */
	g_assert (priv->edit_status == NULL);
	priv->edit_status = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                           (GDestroyNotify) g_free, NULL);

	pos = cell_text->text;
	prev = cell_text->text;

	while (prev != NULL && *prev != '\0')
	{
		while (*pos != '|' && *pos != '\0') ++ pos;

		g_hash_table_insert (priv->edit_status, g_strndup(prev, pos - prev),
		                     GINT_TO_POINTER(1));

		if(*pos != '\0') ++ pos;
		prev = pos;
	}

	g_object_set_data_full (G_OBJECT (combo), CG_CELL_RENDERER_FLAGS_PATH,
	                        g_strdup (path), g_free);

	gtk_widget_show (combo);

	g_signal_connect (G_OBJECT (combo), "editing-done",
	                  G_CALLBACK (cg_cell_renderer_flags_editing_done),
                      cell_flags);

	g_signal_connect (G_OBJECT (combo), "selected",
	                  G_CALLBACK (cg_cell_renderer_flags_selected),
	                  cell_flags);

	priv->focus_out_id =
		g_signal_connect (G_OBJECT (combo), "focus_out_event",
		                  G_CALLBACK (cg_cell_renderer_flags_focus_out_event),
		                  cell_flags);

	return GTK_CELL_EDITABLE (combo);
}

static void
cg_cell_renderer_flags_class_init (CgCellRendererFlagsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (CgCellRendererFlagsPrivate));

	object_class->finalize = cg_cell_renderer_flags_finalize;
	object_class->set_property = cg_cell_renderer_flags_set_property;
	object_class->get_property = cg_cell_renderer_flags_get_property;

	cell_class->start_editing = cg_cell_renderer_flags_start_editing;

	g_object_class_install_property (object_class,
	                                 PROP_MODEL,
	                                 g_param_spec_object ("model",
	                                                      "Model",
	                                                      "Model holding the available flags",
	                                                      GTK_TYPE_TREE_MODEL,
	                                                      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
	                                 PROP_TEXT_COLUMN,
	                                 g_param_spec_int ("text-column",
	                                                   "Text column",
	                                                   "Column in the model holding the text for a flag",
	                                                   -1,
	                                                   G_MAXINT,
	                                                   -1,
	                                                   G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
	                                 PROP_ABBR_COLUMN,
	                                 g_param_spec_int ("abbrevation-column",
	                                                   "Abbrevation column",
	                                                   "Column in the model holding the abbrevation for a flag",
	                                                   -1,
	                                                   G_MAXINT,
	                                                   -1,
	                                                   G_PARAM_READWRITE));
}

GType
cg_cell_renderer_flags_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (CgCellRendererFlagsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cg_cell_renderer_flags_class_init,
			NULL,
			NULL,
			sizeof (CgCellRendererFlags),
			0,
			(GInstanceInitFunc) cg_cell_renderer_flags_init,
			NULL
		};

		our_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_TEXT,
		                                   "CgCellRendererFlags",
		                                   &our_info, 0);
	}

	return our_type;
}

GtkCellRenderer *
cg_cell_renderer_flags_new (void)
{
	GObject *object;

	object = g_object_new (CG_TYPE_CELL_RENDERER_FLAGS, NULL);

	return GTK_CELL_RENDERER (object);
}

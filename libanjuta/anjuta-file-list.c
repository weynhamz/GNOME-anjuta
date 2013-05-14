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

#include "anjuta-file-list.h"

enum
{
	PROP_0,

	PROP_RELATIVE_PATH,
	PROP_SHOW_ADD_BUTTON
};

enum
{
	COL_PATH,

	NUM_COLS
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

struct _AnjutaFileListPriv
{
	gchar *relative_path;
	GtkWidget *list_view;
	GtkListStore *list_model;
	GtkWidget *add_button;
	GtkWidget *copy_button;
	GtkWidget *remove_button;

	/* The placeholder iter tells the user that a file can be entered into the
	 * view, or dragged into it. */
	GtkTreeIter placeholder;
};

G_DEFINE_TYPE (AnjutaFileList, anjuta_file_list, GTK_TYPE_BOX);

static void
anjuta_file_list_append_placeholder (AnjutaFileList *self)
{
	GtkTreeIter iter;

	gtk_list_store_append (self->priv->list_model, &iter);

	gtk_list_store_set (self->priv->list_model, &iter, COL_PATH, NULL, -1);

	self->priv->placeholder = iter;
}


static void
path_cell_data_func (GtkTreeViewColumn *column, GtkCellRenderer *renderer,  
                     GtkTreeModel *model,  GtkTreeIter *iter, 
                     GtkTreeView *list_view)
{
	gchar *path;
	GtkStyleContext *context;

	gtk_tree_model_get (model, iter, COL_PATH, &path, -1);
	context = gtk_widget_get_style_context (GTK_WIDGET (list_view));

	/* NULL path means this is the placeholder */
	if (path)
	{
		GdkRGBA fg_color;

		gtk_style_context_get_color (context, GTK_STATE_NORMAL, &fg_color);
		g_object_set (G_OBJECT (renderer), 
		              "foreground-rgba", &fg_color,
		              "style", PANGO_STYLE_NORMAL,
		              "text", path, 
		              NULL);
	}
	else
	{
		GdkRGBA fg_color;

		gtk_style_context_get_color (context, GTK_STATE_INSENSITIVE, &fg_color);
		g_object_set (G_OBJECT (renderer), 
		              "foreground-rgba", &fg_color,
		              "style", PANGO_STYLE_ITALIC,
		              "text", _("Drop a file or enter a path here"), 
		              NULL);
	}

	g_free (path);
}

static void
on_path_renderer_editing_started (GtkCellRenderer *renderer, 
                                  GtkCellEditable *editable,
                                  const gchar *tree_path,
                                  GtkTreeModel *list_model)
{
	GtkTreeIter iter;
	gchar *path;

	/* Don't show placeholder text in the edit widget */
	gtk_tree_model_get_iter_from_string (list_model, &iter, tree_path);

	gtk_tree_model_get (list_model, &iter, COL_PATH, &path, -1);

	if (!path)
	{
		if (GTK_IS_ENTRY (editable))
			gtk_entry_set_text (GTK_ENTRY (editable), "");
	}
}

static void
on_path_renderer_edited (GtkCellRendererText *renderer, gchar *path,
                         gchar *new_text, AnjutaFileList *self)
{
	GtkTreeIter iter;
	gchar *current_path;

	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (self->priv->list_model),
	                                     &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (self->priv->list_model), &iter,  
	                    COL_PATH, &current_path, -1);

	/* Interpret empty new_text as a cancellation of the edit */
	if (g_utf8_strlen (new_text, -1) > 0)
	{
		/* If the placeholder is being edited, a new one has to be created */
		if (!current_path)
			anjuta_file_list_append_placeholder (self);

		gtk_list_store_set (self->priv->list_model, &iter, COL_PATH, new_text, 
		                    -1);
	}
}

static void
on_list_view_drag_data_received (GtkWidget *list_view, GdkDragContext *context,
                                 gint x, gint y, GtkSelectionData *data, 
                                 gint target_type, gint time, 
                                 AnjutaFileList *self)
{
	gboolean success;
	gchar **uri_list;
	gint i;
	GFile *parent_file;
	GFile *file;
	gchar *path;
	GtkTreeIter iter;
	
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
			
			for (i = 0; uri_list[i]; i++)
			{
				file = g_file_new_for_uri (uri_list[i]);

				if (parent_file)
				{
					path = g_file_get_relative_path (parent_file, file);

					g_object_unref (parent_file);
				}
				else
					path = g_file_get_path (file);

				if (path)
				{
					gtk_list_store_insert_before (self->priv->list_model, 
					                              &iter, 
					                              &(self->priv->placeholder));
					gtk_list_store_set (self->priv->list_model, &iter,
					                    COL_PATH, path,
					                    -1);

					g_free (path);
				}

				g_object_unref (file);
			}
			
			success = TRUE;

			g_strfreev (uri_list);
		}
	}

	/* Do not delete source data */
	gtk_drag_finish (context, success, FALSE, time);
}

static gboolean
on_list_view_drag_drop (GtkWidget *widget, GdkDragContext *context, 
                        gint x, gint y, guint time, gpointer user_data)
{
	GdkAtom target_type;

	target_type = gtk_drag_dest_find_target (widget, context, NULL);

	if (target_type != GDK_NONE)
		gtk_drag_get_data (widget, context, target_type, time);
	else
		gtk_drag_finish (context, FALSE, FALSE, time);

	return TRUE;
}

static gboolean
on_list_view_item_selected (GtkTreeSelection *selection, GtkTreeModel *model,
                            GtkTreePath *tree_path,  
                            gboolean path_currently_selected, 
                            AnjutaFileList *self)
{

	gboolean sensitive;
	GtkTreeIter iter;
	gchar *path;

	sensitive = FALSE;

	if (!path_currently_selected)
	{
		gtk_tree_model_get_iter (model, &iter, tree_path);
		gtk_tree_model_get (model, &iter, COL_PATH, &path, -1);

		if (path)
		{
			sensitive = TRUE;
			
			g_free (path);
		}
	}

	gtk_widget_set_sensitive (self->priv->copy_button, sensitive);
	gtk_widget_set_sensitive (self->priv->remove_button, sensitive);

	return TRUE;
}

static void
on_add_button_clicked (GtkButton *button, AnjutaFileList *self)
{
	GtkWidget *file_dialog;
	GSList *paths;
	GSList *current_path;
	GtkTreeIter iter;

	file_dialog = gtk_file_chooser_dialog_new (_("Select Files"),
	                                           GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
	                                           GTK_FILE_CHOOSER_ACTION_OPEN,
	                                           GTK_STOCK_CANCEL,
	                                           GTK_RESPONSE_CANCEL,
	                                           GTK_STOCK_OPEN,
	                                           GTK_RESPONSE_ACCEPT,
	                                           NULL);

	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (file_dialog),
	                                      TRUE);

	if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		paths = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (file_dialog));
		current_path = paths;

		while (current_path)
		{
			gtk_list_store_insert_before (self->priv->list_model, &iter,
			                              &(self->priv->placeholder));
			gtk_list_store_set (self->priv->list_model, &iter, 
			                    COL_PATH, current_path->data,
			                    -1);

			g_free (current_path->data);

			current_path = g_slist_next (current_path);
		}

		g_slist_free (paths);

		
	}

	gtk_widget_destroy (file_dialog);
}

static void
on_copy_button_clicked (GtkButton *button, GtkTreeSelection *selection)
{
	GtkTreeModel *list_model;
	GtkTreeIter selected_iter;
	GtkTreeIter new_iter;
	gchar *path;

	if (gtk_tree_selection_get_selected (selection, &list_model, 
	                                     &selected_iter))
	{
		gtk_tree_model_get (list_model, &selected_iter, COL_PATH, &path, -1);
		gtk_list_store_insert_after (GTK_LIST_STORE (list_model), &new_iter, 
		                             &selected_iter);

		gtk_list_store_set (GTK_LIST_STORE (list_model), &new_iter, COL_PATH,
		                    path, -1);

		g_free (path);
	}
}

static void
on_remove_button_clicked (GtkButton *button, GtkTreeSelection *selection)
{
	GtkTreeIter iter;
	GtkTreeModel *list_model;
	
	if (gtk_tree_selection_get_selected (selection, &list_model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (list_model), &iter);
}

static void
anjuta_file_list_init (AnjutaFileList *self)
{
	GtkWidget *scrolled_window;
	GtkWidget *button_box;
	GtkWidget *clear_button;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;


	/* Set properties */
	g_object_set (self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

	self->priv = g_new0 (AnjutaFileListPriv, 1);
	self->priv->list_view = gtk_tree_view_new ();
	self->priv->list_model = gtk_list_store_new (NUM_COLS, G_TYPE_STRING);
	self->priv->add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	self->priv->copy_button = gtk_button_new_from_stock (GTK_STOCK_COPY);
	self->priv->remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);

	button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	clear_button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);

	gtk_widget_set_sensitive (self->priv->copy_button, FALSE);
	gtk_widget_set_sensitive (self->priv->remove_button, FALSE);

	g_signal_connect_swapped (G_OBJECT (clear_button), "clicked",
	                          G_CALLBACK (anjuta_file_list_clear),
	                          self);

	/* File list view */
	gtk_box_set_spacing (GTK_BOX (self), 2);
	gtk_box_set_homogeneous (GTK_BOX (self), FALSE);

	gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->list_view), 
	                         GTK_TREE_MODEL (self->priv->list_model));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->list_view),
	                                   FALSE);

	/* Path column */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, 
	                                         (GtkTreeCellDataFunc) path_cell_data_func,
	                                         self->priv->list_view,
	                                         NULL);
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->list_view),
	                             column);

	g_signal_connect (G_OBJECT (renderer), "editing-started",
	                  G_CALLBACK (on_path_renderer_editing_started),
	                  self->priv->list_model);

	g_signal_connect (G_OBJECT (renderer), "edited",
	                  G_CALLBACK (on_path_renderer_edited),
	                  self);

	/* DND */
	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (self->priv->list_view), 
	                                      dnd_target_entries,
	                                      G_N_ELEMENTS (dnd_target_entries),
	                                      GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (self->priv->list_view), "drag-drop",
	                  G_CALLBACK (on_list_view_drag_drop),
	                  NULL);

	g_signal_connect (G_OBJECT (self->priv->list_view), "drag-data-received",
	                  G_CALLBACK (on_list_view_drag_data_received),
	                  self);

	/* Selection handling */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->list_view));
	
	gtk_tree_selection_set_select_function (selection, 
	                                        (GtkTreeSelectionFunc) on_list_view_item_selected,
	                                        self, NULL);

	g_signal_connect (G_OBJECT (self->priv->add_button), "clicked",
	                  G_CALLBACK (on_add_button_clicked),
	                  self);

	g_signal_connect (G_OBJECT (self->priv->copy_button), "clicked",
	                  G_CALLBACK (on_copy_button_clicked),
	                  selection);

	g_signal_connect (G_OBJECT (self->priv->remove_button), "clicked",
	                  G_CALLBACK (on_remove_button_clicked), 
	                  selection);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
	                                                     GTK_POLICY_AUTOMATIC,
	                                                     GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
	                                     GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), self->priv->list_view);
	gtk_box_pack_start (GTK_BOX (self), scrolled_window, TRUE, TRUE, 0);

	/* Button box */
	gtk_box_set_spacing (GTK_BOX (button_box), 5);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), 
	                           GTK_BUTTONBOX_START);

	gtk_container_add (GTK_CONTAINER (button_box), self->priv->add_button);
	gtk_container_add (GTK_CONTAINER (button_box), self->priv->copy_button);
	gtk_container_add (GTK_CONTAINER (button_box), self->priv->remove_button);
	gtk_container_add (GTK_CONTAINER (button_box), clear_button);
	gtk_box_pack_start (GTK_BOX (self), button_box, FALSE, FALSE, 0);

	anjuta_file_list_append_placeholder (self);

	gtk_widget_show_all (GTK_WIDGET (self));

	/* Don't show the Add button by default */
	gtk_widget_set_visible (self->priv->add_button, FALSE);
}

static void
anjuta_file_list_finalize (GObject *object)
{
	AnjutaFileList *self;

	self = ANJUTA_FILE_LIST (object);

	g_free (self->priv->relative_path);
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_file_list_parent_class)->finalize (object);
}

static void
anjuta_file_list_set_property (GObject *object, guint prop_id, 
                               const GValue *value, GParamSpec *pspec)
{
	AnjutaFileList *self;

	g_return_if_fail (ANJUTA_IS_FILE_LIST (object));

	self = ANJUTA_FILE_LIST (object);

	switch (prop_id)
	{
		case PROP_RELATIVE_PATH:
			g_free (self->priv->relative_path);

			self->priv->relative_path = g_value_dup_string (value);
			break;
		case PROP_SHOW_ADD_BUTTON:
			gtk_widget_set_visible (self->priv->add_button,
			                        g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
anjuta_file_list_get_property (GObject *object, guint prop_id, GValue *value, 
                               GParamSpec *pspec)
{
	AnjutaFileList *self;

	g_return_if_fail (ANJUTA_IS_FILE_LIST (object));

	self = ANJUTA_FILE_LIST (object);

	switch (prop_id)
	{
		case PROP_RELATIVE_PATH:
			g_value_set_string (value, self->priv->relative_path);
			break;
		case PROP_SHOW_ADD_BUTTON:
			g_value_set_boolean (value, 
			                     gtk_widget_get_visible (self->priv->add_button));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
anjuta_file_list_class_init (AnjutaFileListClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_file_list_finalize;
	object_class->set_property = anjuta_file_list_set_property;
	object_class->get_property = anjuta_file_list_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_RELATIVE_PATH,
	                                 g_param_spec_string ("relative-path",
	                                                      "relative-path",
	                                                      _("Path that all files in the list should be relative to"),
	                                                      "",
	                                                      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
	                                 PROP_SHOW_ADD_BUTTON,
	                                 g_param_spec_boolean ("show-add-button",
	                                                       _("Show Add button"),
	                                                       _("Display an Add button"),
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
}


GtkWidget *
anjuta_file_list_new (void)
{
	return g_object_new (ANJUTA_TYPE_FILE_LIST, NULL);
}

static gboolean
list_model_foreach (GtkTreeModel *list_model, GtkTreePath *tree_path,
                    GtkTreeIter *iter, GList **list)
{
	gchar *path;

	gtk_tree_model_get (list_model, iter, COL_PATH, &path, -1);

	/* Make sure not to add the placeholder to the list */
	if (path)
		*list = g_list_append (*list, path);

	return FALSE;
}

GList *
anjuta_file_list_get_paths (AnjutaFileList *self)
{
	GList *list;

	list = NULL;

	gtk_tree_model_foreach (GTK_TREE_MODEL (self->priv->list_model),
	                        (GtkTreeModelForeachFunc) list_model_foreach,
	                        &list);

	return list;
}

void
anjuta_file_list_set_relative_path (AnjutaFileList *self, const gchar *path)
{
	g_object_set (G_OBJECT (self), "relative-path", path, NULL);
}

void
anjuta_file_list_clear (AnjutaFileList *self)
{	
	gtk_list_store_clear (self->priv->list_model);

	anjuta_file_list_append_placeholder (self);
}

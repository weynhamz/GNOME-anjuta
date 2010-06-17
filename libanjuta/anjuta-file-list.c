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

	PROP_RELATIVE_PATH
};

enum
{
	COL_PATH,

	NUM_COLS
};

struct _AnjutaFileListPriv
{
	gchar *relative_path;
	GtkWidget *list_view;
	GtkListStore *list_model;
	GtkWidget *copy_button;
	GtkWidget *remove_button;

	/* The placeholder iter tells the user that a file can be entered into the
	 * view, or dragged into it. */
	GtkTreeIter placeholder;
};

G_DEFINE_TYPE (AnjutaFileList, anjuta_file_list, GTK_TYPE_VBOX);

static void
anjuta_file_list_init (AnjutaFileList *self)
{
	GtkWidget *scrolled_window;
	GtkWidget *button_box;
	GtkWidget *clear_button;

	self->priv = g_new0 (AnjutaFileListPriv, 1);
	self->priv->list_view = gtk_tree_view_new ();
	self->priv->list_model = gtk_list_store_new (NUM_COLS, G_TYPE_STRING);
	self->priv->copy_button = gtk_button_new_from_stock (GTK_STOCK_COPY);
	self->priv->remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);

	button_box = gtk_hbutton_box_new ();
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	clear_button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);

	/* File list view */
	gtk_box_set_spacing (GTK_BOX (self), 2);
	gtk_box_set_homogeneous (GTK_BOX (self), FALSE);

	gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->list_view), 
	                         GTK_TREE_MODEL (self->priv->list_model));

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
	                                                     GTK_POLICY_AUTOMATIC,
	                                                     GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
	                                     GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), self->priv->list_view);
	gtk_box_pack_start (GTK_BOX (self), scrolled_window, TRUE, TRUE, 0);

	/* Button box */
	gtk_box_set_spacing (GTK_BOX (button_box), 5);

	gtk_container_add (GTK_CONTAINER (button_box), self->priv->copy_button);
	gtk_container_add (GTK_CONTAINER (button_box), self->priv->remove_button);
	gtk_container_add (GTK_CONTAINER (button_box), clear_button);
	gtk_box_pack_start (GTK_BOX (self), button_box, FALSE, FALSE, 0);
}

static void
anjuta_file_list_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

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
	                                                      0));
}


GtkWidget *
anjuta_file_list_new (void)
{
	return g_object_new (ANJUTA_TYPE_FILE_LIST, NULL);
}

GList *
anjuta_file_list_anjuta_file_lst_get_paths (AnjutaFileList *self)
{
	return NULL;
}

void
anjuta_file_list_set_relative_path (AnjutaFileList *self, const gchar *path)
{

}

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "file-model.h"
#include <glib/gi18n.h>
#include <string.h>
#include <libanjuta/anjuta-debug.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#define DIRECTORY_MIME_TYPE "x-directory/normal"
#define ICON_SIZE 12

enum
{
	PROP_0,
	PROP_BASE_URI,
	PROP_FILTER_BINARY,
	PROP_FILTER_HIDDEN,
	PROP_FILTER_BACKUP
};

typedef struct _FileModelPrivate FileModelPrivate;

struct _FileModelPrivate
{
	gchar* base_uri;
	gboolean filter_binary;
	gboolean filter_hidden;
	gboolean filter_backup;
	
	GCancellable* expand_cancel;
	GtkTreeIter* expand_iter;
};

#define FILE_MODEL_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), FILE_TYPE_MODEL, FileModelPrivate))

G_DEFINE_TYPE (FileModel, file_model, GTK_TYPE_TREE_STORE)

static void
file_model_add_dummy (FileModel* model,
					 GtkTreeIter* iter)
{
	GtkTreeStore* store = GTK_TREE_STORE (model);
	GtkTreeIter dummy;
	
	gtk_tree_store_append (store, &dummy, iter);
	gtk_tree_store_set (store, &dummy, 
					    COLUMN_FILENAME, _("Loading..."),
						COLUMN_SORT, -1,
					    -1);
}

static gboolean
file_model_filter_file (FileModel* model,
						GFileInfo* file_info)
{
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	
	if (priv->filter_hidden && g_file_info_get_is_hidden(file_info))
		return FALSE;
	else if (priv->filter_backup && g_file_info_get_is_backup(file_info))
		return FALSE;
	
	return TRUE;
}

static void
file_model_expand_row_real(GObject* src_object, GAsyncResult* result,
						   gpointer data)
{
	FileModel* model = FILE_MODEL(data);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	GFile* dir = G_FILE(src_object);
	GError* err = NULL;
	GFileEnumerator* files = g_file_enumerate_children_finish(dir, result, &err);
	GFileInfo* file_info;
	GtkTreeIter dummy;
	
	while (files && (file_info = g_file_enumerator_next_file (files, NULL, NULL)))
	{
		GtkTreeIter iter;
		GtkTreeStore* store = GTK_TREE_STORE(model);
		gboolean is_dir = FALSE;
		const gchar** icon_names;
		GtkIconInfo* icon_info;
		GIcon* icon;
		GdkPixbuf* pixbuf;
		GFile* file;
		
		if (!file_model_filter_file (model, file_info))
			continue;
		
		file = g_file_get_child (dir, g_file_info_get_name(file_info));
		
		icon = g_file_info_get_icon(file_info);
		g_object_get (icon, "names", &icon_names, NULL);
		icon_info = gtk_icon_theme_choose_icon (gtk_icon_theme_get_default(),
												icon_names,
												ICON_SIZE,
												GTK_ICON_LOOKUP_GENERIC_FALLBACK);
		pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
		gtk_icon_info_free(icon_info);
		gtk_tree_store_append (store, &iter, priv->expand_iter);
		
		if (g_file_info_get_file_type(file_info) == G_FILE_TYPE_DIRECTORY)
			is_dir = TRUE;
		
		gtk_tree_store_set (store, &iter, 
							COLUMN_FILENAME, g_file_info_get_display_name(file_info),
							COLUMN_FILE, file,
							COLUMN_PIXBUF, pixbuf,
							COLUMN_IS_DIR, is_dir,
							COLUMN_SORT, g_file_info_get_sort_order(file_info),
							-1);
		if (is_dir)
			file_model_add_dummy(model, &iter);
		
		g_object_unref (file);
		g_object_unref (pixbuf);
	}
	/* Remove dummy node */
	gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &dummy, priv->expand_iter);
	gtk_tree_store_remove (GTK_TREE_STORE(model), &dummy);
	
	if (priv->expand_iter)
		gtk_tree_iter_free(priv->expand_iter);
	priv->expand_iter = NULL;
	g_cancellable_reset(priv->expand_cancel);
	g_object_unref(files);
}

static void
file_model_row_expanded (GtkTreeView* tree_view, GtkTreeIter* iter,
					    GtkTreePath* path, gpointer data)
{
	GtkTreeModel* sort_model = gtk_tree_view_get_model(tree_view);
	FileModel* model = FILE_MODEL(data);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	GFile* dir;
	GtkTreeIter real_iter;
	
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
												   &real_iter, iter);
	
	gtk_tree_model_get(GTK_TREE_MODEL(model), &real_iter,
					   COLUMN_FILE, &dir, -1);
	priv->expand_iter = gtk_tree_iter_copy(&real_iter);
	
	g_file_enumerate_children_async (dir,
									 "standard::*",
									 G_FILE_QUERY_INFO_NONE,
									 G_PRIORITY_DEFAULT,
									 priv->expand_cancel,
									 file_model_expand_row_real,
									 model);
}

static void
file_model_row_collapsed (GtkTreeView* tree_view, GtkTreeIter* iter,
						 GtkTreePath* path, gpointer data)
{
	GtkTreeModel* sort_model = gtk_tree_view_get_model(tree_view);
	FileModel* model = FILE_MODEL(data);
	GtkTreeIter child;
	GtkTreeIter real_iter;
	
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
												   &real_iter, iter);

	while (gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &real_iter))
	{
		gtk_tree_store_remove (GTK_TREE_STORE (model), &child);
	}
	file_model_add_dummy (model, &real_iter);
}

static void
file_model_expand_cancelled(GObject* cancel, gpointer data)
{
	FileModel* model = FILE_MODEL(data);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	
	if (priv->expand_iter)
		gtk_tree_iter_free(priv->expand_iter);
	
	priv->expand_iter = NULL;
}
	
static void
file_model_init (FileModel *object)
{
	FileModel* model = FILE_MODEL(object);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	
	priv->expand_cancel = g_cancellable_new();
	g_signal_connect(priv->expand_cancel, "cancelled", G_CALLBACK(file_model_expand_cancelled),
					 model);
	priv->expand_iter = NULL;
}

static void
file_model_finalize (GObject *object)
{
	FileModel* model = FILE_MODEL(object);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);
	
	g_object_unref (priv->expand_cancel);
	
	G_OBJECT_CLASS (file_model_parent_class)->finalize (object);
}

static void
file_model_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (FILE_IS_MODEL (object));
	FileModel* model = FILE_MODEL(object);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);	
	
	switch (prop_id)
	{
	case PROP_BASE_URI:
		g_free (priv->base_uri);
		priv->base_uri = g_value_dup_string (value);
		if (!priv->base_uri || !strlen (priv->base_uri))
		{
			priv->base_uri = g_strdup("file:///");
		}
		break;
	case PROP_FILTER_BINARY:
		priv->filter_binary = g_value_get_boolean (value);
		break;
	case PROP_FILTER_HIDDEN:
		priv->filter_hidden = g_value_get_boolean (value);
		break;
	case PROP_FILTER_BACKUP:
		priv->filter_backup = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
file_model_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (FILE_IS_MODEL (object));
	FileModel* model = FILE_MODEL(object);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);
	
	switch (prop_id)
	{
	case PROP_BASE_URI:
		g_value_set_string (value, priv->base_uri);
		break;
	case PROP_FILTER_BINARY:
		g_value_set_boolean (value, priv->filter_binary);
	case PROP_FILTER_HIDDEN:
		g_value_set_boolean (value, priv->filter_hidden);
	case PROP_FILTER_BACKUP:
		g_value_set_boolean (value, priv->filter_backup);	
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
file_model_class_init (FileModelClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = file_model_finalize;
	object_class->set_property = file_model_set_property;
	object_class->get_property = file_model_get_property;

	g_type_class_add_private (object_class, sizeof(FileModelPrivate));
	
	g_object_class_install_property (object_class,
	                                 PROP_BASE_URI,
	                                 g_param_spec_string ("base_uri",
	                                                      "Base uri",
	                                                      "Base uri",
	                                                      "NULL",
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
									 PROP_FILTER_BINARY,
									 g_param_spec_boolean ("filter_binary",
														   "Filter binary files",
														   "file_binary",
														   TRUE,
														   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	
	g_object_class_install_property (object_class,
									 PROP_FILTER_HIDDEN,
									 g_param_spec_boolean ("filter_hidden",
														   "Filter hidden files",
														   "file_hidden",
														   TRUE,
														   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	
	g_object_class_install_property (object_class,
									 PROP_FILTER_BACKUP,
									 g_param_spec_boolean ("filter_backup",
														   "Filter backup files",
														   "file_backup",
														   TRUE,
														   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	
}

FileModel*
file_model_new (GtkTreeView* tree_view, const gchar* base_uri)
{
	GObject* model =
		g_object_new (FILE_TYPE_MODEL, "base_uri", base_uri, NULL);
	GType types[N_COLUMNS] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_OBJECT,
		G_TYPE_BOOLEAN, G_TYPE_INT};
	
	g_signal_connect (G_OBJECT (tree_view), "row-collapsed", 
					  G_CALLBACK (file_model_row_collapsed), model);
	g_signal_connect (G_OBJECT (tree_view), "row-expanded", 
					  G_CALLBACK (file_model_row_expanded), model);
	
	gtk_tree_store_set_column_types (GTK_TREE_STORE (model), N_COLUMNS,
									 types);
	
	return FILE_MODEL(model);
}

void
file_model_refresh (FileModel* model)
{
	GtkTreeStore* store = GTK_TREE_STORE (model);
	GtkTreeIter iter;
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);
	GdkPixbuf *pixbuf;
	GFile* base;
	GFileInfo* base_info;
	const gchar** icon_names;
	GtkIconInfo* icon_info;
	
	GIcon* icon;

	g_cancellable_cancel(priv->expand_cancel);
	g_cancellable_reset(priv->expand_cancel);
	gtk_tree_store_clear (store);
	
	base = g_file_new_for_uri (priv->base_uri);
	base_info = g_file_query_info (base, "standard::display-name,standard::icon",
								  G_FILE_QUERY_INFO_NONE, NULL, NULL);
	
	if (!base_info)
		return;
	
	icon = g_file_info_get_icon(base_info);
	g_object_get (icon, "names", &icon_names, NULL);
	icon_info = gtk_icon_theme_choose_icon (gtk_icon_theme_get_default(),
							  icon_names,
							  ICON_SIZE,
							  GTK_ICON_LOOKUP_GENERIC_FALLBACK);
	pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
	gtk_icon_info_free(icon_info);
							  
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter, 
						COLUMN_FILENAME, g_file_info_get_display_name(base_info),
						COLUMN_FILE, base,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_IS_DIR, TRUE,
						COLUMN_SORT, 0,
						-1);
	file_model_add_dummy (model, &iter);

	g_object_unref (pixbuf);
	g_object_unref (base);
}

gchar*
file_model_get_uri (FileModel* model, GtkTreeIter* iter)
{
	GFile* file;
	gchar* uri;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, COLUMN_FILE, &file, -1);
	
	uri = g_file_get_uri (file);
	g_object_unref (file);
	
	return uri;
}

gchar*
file_model_get_filename (FileModel* model, GtkTreeIter* iter)
{
	gchar* filename;
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, COLUMN_FILENAME, &filename, -1);
	
	return filename;
}

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
#include <gdl/gdl-icons.h>
#include <glib/gi18n.h>
#include <string.h>
#include <libanjuta/anjuta-debug.h>
#include <libgnomevfs/gnome-vfs.h>

#define DIRECTORY_MIME_TYPE "x-directory/normal"
#define DIRECTORY_OPEN "x-directory/open"
#define ICON_SIZE 12

enum
{
	PROP_0,
	PROP_BASE_URI,
	PROP_FILTER_BINARY,
	PROP_FILTER_HIDDEN,
	PROP_FILTER_BACKUP
};

const gchar* binary_mime[] = {
	"application/x-core",
	"application/x-shared-library-la.xml",
	"application/x-sharedlib.xml",
	NULL
};

const gchar* binary_suffix[] = {
	".lo",
	".o",
	".a",
	NULL
};

typedef struct _FileModelPrivate FileModelPrivate;

struct _FileModelPrivate
{
	gchar* base_uri;
	gboolean filter_binary;
	gboolean filter_hidden;
	gboolean filter_backup;
	
	GdlIcons* icons;
	
	guint expand_idle_id;
};

typedef struct _FileModelIdleExpand FileModelIdleExpand;
struct _FileModelIdleExpand
{
	FileModel* model;
	GList* files;
	GtkTreePath* path;
	gchar* uri;
};

#define FILE_MODEL_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), FILE_TYPE_MODEL, FileModelPrivate))

G_DEFINE_TYPE (FileModel, file_model, GTK_TYPE_TREE_STORE);

static gboolean
file_model_filter_file (FileModel* model, GnomeVFSFileInfo* info)
{
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);	
	/* Ignore hidden files */
	if (priv->filter_hidden && g_str_has_prefix (info->name, "."))
	{
		return TRUE;
	}
	/* Ignore backup files */
	if (priv->filter_backup && (g_str_has_suffix (info->name, "~") ||
		g_str_has_suffix (info->name, ".bak")))
	{
		return TRUE;
	}
	if (priv->filter_binary)
	{
		int i;
		if (info->mime_type)
		{
			for (i = 0; binary_mime[i] != NULL; i++)
			{
				if (g_str_equal (info->mime_type, binary_mime[i]))
					return TRUE;
			}
		}
		for (i = 0; binary_suffix[i] != NULL; i++)
		{
			if (g_str_has_suffix (info->name, binary_suffix[i]))
				return TRUE;
		}
	}
	/* Be sure to ignore "." and ".." */
	if (g_str_equal (info->name, ".") || g_str_equal (info->name, ".."))
	{
		return TRUE;
	}
	return FALSE;
}

static void
file_model_cancel_expand_idle(FileModel* model)
{
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);
	
	if (priv->expand_idle_id)
	{
		g_source_remove (priv->expand_idle_id);
		priv->expand_idle_id = 0;
	}
}

static void
file_model_add_dummy (FileModel* model,
					 GtkTreeIter* iter)
{
	GtkTreeStore* store = GTK_TREE_STORE (model);
	GtkTreeIter dummy;
	
	gtk_tree_store_append (store, &dummy, iter);
	gtk_tree_store_set (store, &dummy, 
					    COLUMN_FILENAME, _("Loading..."),
					    -1);
}

static gboolean
file_model_expand_idle (gpointer data)
{
	FileModelIdleExpand* expand = (FileModelIdleExpand*) data;
	FileModel* model = expand->model;
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	GtkTreeStore* store = GTK_TREE_STORE (model);
	GtkTreeIter parent;
	
	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL(model), &parent, expand->path))
		return FALSE;
	if (expand->files)
	{
		GList* file = expand->files;
		GnomeVFSFileInfo* info = (GnomeVFSFileInfo*) file->data;
		GtkTreeIter new_iter;
		GdkPixbuf* pixbuf = NULL;
		gboolean directory = FALSE;
		
		/* Set pointer to the next element */
		expand->files = g_list_next (file);
		
		/* Ignore user-defined files */
		if (file_model_filter_file (model, info))
		{
			return TRUE;
		}
		
		/* Create file entry in tree */
		const gchar* mime_type = gnome_vfs_file_info_get_mime_type (info);
		gchar* uri = g_build_filename (expand->uri, info->name, NULL);
		
		if (pixbuf)
		  pixbuf = gdl_icons_get_mime_icon (priv->icons, mime_type);
		
		gtk_tree_store_append (store, &new_iter, &parent);
		
		if (mime_type && g_str_equal (mime_type, DIRECTORY_MIME_TYPE))
		{
			file_model_add_dummy (model, &new_iter);
			directory = TRUE;
		}
		
		gtk_tree_store_set (store, &new_iter, 
						    COLUMN_FILENAME, info->name,
						    COLUMN_URI, uri,
						    COLUMN_PIXBUF, pixbuf,
						    COLUMN_IS_DIR, directory,
						    -1);
		g_free (uri);
		return TRUE;
	}
	else
	{
		GtkTreeIter dummy;
		gnome_vfs_file_info_list_free (expand->files);
		gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &dummy, &parent);
		gtk_tree_store_remove (store, &dummy);
		
		g_free (expand->uri);
		
		priv->expand_idle_id = 0;
		
		return FALSE;
	}
}

static int
file_model_sort (gconstpointer a, gconstpointer b)
{
	GnomeVFSFileInfo* info_a = (GnomeVFSFileInfo*) a;
	GnomeVFSFileInfo* info_b = (GnomeVFSFileInfo*) b;
	gboolean dir_a = (info_a->type  == GNOME_VFS_FILE_TYPE_DIRECTORY);
	gboolean dir_b = (info_b->type  == GNOME_VFS_FILE_TYPE_DIRECTORY);
	
	if (dir_a == dir_b)
	{
		return strcasecmp (info_a->name, info_b->name);
	}
	else if (dir_a)
	{
		return -1;
	}
	else if (dir_b)
	{
		return 1;
	}
	else
		return 0;
}

static void
file_model_row_expanded (GtkTreeView* tree_view, GtkTreeIter* iter,
					    GtkTreePath* path, gpointer data)
{
	FileModel* model = FILE_MODEL(data);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	gchar* uri;
	GList* files = NULL;
	
	gtk_tree_model_get (GTK_TREE_MODEL(model), iter, COLUMN_URI, &uri, -1);
	
	if (gnome_vfs_directory_list_load (&files, uri,
									   GNOME_VFS_FILE_INFO_GET_MIME_TYPE) ==
		GNOME_VFS_OK)
	{
		files = g_list_sort (files, file_model_sort);
		FileModelIdleExpand* expand = g_new0 (FileModelIdleExpand, 1);
		expand->files = files;
		expand->path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), iter);
		expand->model = model;
		expand->uri = uri;
		
		priv->expand_idle_id = g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
						 (GSourceFunc) file_model_expand_idle, 
						 (gpointer) expand,
						 (GDestroyNotify) g_free);
		
		gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
							COLUMN_PIXBUF, 
							gdl_icons_get_mime_icon (priv->icons, DIRECTORY_OPEN),
							-1);
							
	}
}

static void
file_model_row_collapsed (GtkTreeView* tree_view, GtkTreeIter* iter,
						 GtkTreePath* path, gpointer data)
{
	FileModel* model = FILE_MODEL(data);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	GtkTreeIter child;
	
	while (gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, iter))
	{
		gtk_tree_store_remove (GTK_TREE_STORE (model), &child);
	}
	file_model_add_dummy (model, iter);
	gtk_tree_store_set (GTK_TREE_STORE (model), iter, 
						COLUMN_PIXBUF, 
						gdl_icons_get_folder_icon (priv->icons),
						-1);
}

static void
file_model_init (FileModel *object)
{
	FileModel* model = FILE_MODEL(object);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE (model);
	
	priv->icons = gdl_icons_new(ICON_SIZE);
}

static void
file_model_finalize (GObject *object)
{
	FileModel* model = FILE_MODEL(object);
	FileModelPrivate* priv = FILE_MODEL_GET_PRIVATE(model);
	
	g_object_unref (G_OBJECT(priv->icons));
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
		priv->base_uri = g_strdup (g_value_get_string (value));
		if (!priv->base_uri || !strlen (priv->base_uri))
		{
			priv->base_uri = gnome_vfs_get_uri_from_local_path("/");
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
	GType types[N_COLUMNS] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_BOOLEAN};
	
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
	gchar *path, *basename;
	GdkPixbuf *pixbuf;

	gtk_tree_store_clear (store);
	path = gnome_vfs_get_local_path_from_uri (priv->base_uri);
	if (!path)
		return;

	basename = g_path_get_basename (path);
	pixbuf = gdl_icons_get_folder_icon (priv->icons);
	
	file_model_cancel_expand_idle(model);
	
	gtk_tree_store_append (store, &iter, NULL);
	
	gtk_tree_store_set (store, &iter, 
						COLUMN_FILENAME, basename,
						COLUMN_URI, priv->base_uri,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_IS_DIR, TRUE,
						-1);
	g_free (basename);
	g_free (path);
	
	file_model_add_dummy (model, &iter);
}

gchar*
file_model_get_uri (FileModel* model, GtkTreeIter* iter)
{
	gchar* uri;
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, COLUMN_URI, &uri, -1);
	
	return uri;
}

gchar*
file_model_get_filename (FileModel* model, GtkTreeIter* iter)
{
	gchar* filename;
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, COLUMN_FILENAME, &filename, -1);
	
	return filename;
}

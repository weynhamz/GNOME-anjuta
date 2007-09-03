/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * file-manager
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * file-manager is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * file-manager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with file-manager.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "file-view.h"
#include "file-view-marshal.h"

#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrendererprogress.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreemodelsort.h>

#include <glib/gi18n.h>

#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-debug.h>
#include <gdl/gdl-icons.h>

#define DIRECTORY_MIME_TYPE "x-directory/normal"

typedef struct _AnjutaFileViewPrivate AnjutaFileViewPrivate;

struct _AnjutaFileViewPrivate
{
	GtkTreeModel* real_model;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	GdlIcons* icons;
	gchar* base_uri;
	GList* saved_paths;
};

#define ANJUTA_FILE_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), ANJUTA_TYPE_FILE_VIEW, AnjutaFileViewPrivate))

G_DEFINE_TYPE (AnjutaFileView, file_view, GTK_TYPE_TREE_VIEW);

enum
{
	PROP_BASE_URI = 1,
	PROP_END
};

enum
{
	COLUMN_PIXBUF,
	COLUMN_STATUS,
	COLUMN_FILENAME,
	COLUMN_URI,
	COLUMN_IS_DIR,
	N_COLUMNS
};

typedef struct _FileViewIdleExpand FileViewIdleExpand;
struct _FileViewIdleExpand
{
	AnjutaFileView* view;
	GList* files;
	GtkTreePath* path;
	gchar* uri;
};

static void
file_view_add_dummy (AnjutaFileView* view,
					 GtkTreeIter* iter)
{
	AnjutaFileViewPrivate* priv =
		ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeStore* store = GTK_TREE_STORE (priv->model);
	GtkTreeIter dummy;
	
	gtk_tree_store_append (store, &dummy, iter);
	gtk_tree_store_set (store, &dummy, 
					    COLUMN_FILENAME, _("Loading..."),
					    -1);
}

static gboolean
file_view_expand_idle (gpointer data)
{
	FileViewIdleExpand* expand = (FileViewIdleExpand*) data;
	AnjutaFileView* view = expand->view;
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeStore* store = GTK_TREE_STORE (priv->model);
	GtkTreeIter parent;
	
	if (!gtk_tree_model_get_iter (priv->model, &parent, expand->path))
		return FALSE;
	if (expand->files)
	{
		GList* file = expand->files;
		GnomeVFSFileInfo* info = (GnomeVFSFileInfo*) file->data;
		GtkTreeIter new_iter;
		GdkPixbuf* pixbuf;
		gboolean directory = FALSE;
		
		/* Set pointer to the next element */
		expand->files = g_list_next (file);
		
		/* Ignore hidden files */
		if (g_str_has_prefix (info->name, "."))
		{
			return TRUE;
		}
		
		/* Create file entry in tree */
		const gchar* mime_type = gnome_vfs_file_info_get_mime_type (info);
		gchar* uri = g_build_filename (expand->uri, info->name, NULL);
		
		pixbuf = gdl_icons_get_mime_icon (priv->icons, mime_type);
		
		gtk_tree_store_append (store, &new_iter, &parent);
		
		if (g_str_equal (mime_type, DIRECTORY_MIME_TYPE))
		{
			file_view_add_dummy (view, &new_iter);
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
		gtk_tree_model_iter_children (priv->model, &dummy, &parent);
		gtk_tree_store_remove (store, &dummy);
		
		g_free (expand->uri);
		
		return FALSE;
	}
}

static int
file_view_sort (gconstpointer a, gconstpointer b)
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
}

static void
file_view_row_expanded (GtkTreeView* tree_view, GtkTreeIter* iter,
					    GtkTreePath* path)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (tree_view);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeStore* store = GTK_TREE_STORE (priv->model);
	gchar* uri;
	GList* files = NULL;
	GtkTreeIter dummy;
	
	gtk_tree_model_get (priv->model, iter, COLUMN_URI, &uri, -1);
	
	if (gnome_vfs_directory_list_load (&files, uri,
									   GNOME_VFS_FILE_INFO_GET_MIME_TYPE) ==
		GNOME_VFS_OK)
	{
		files = g_list_sort (files, file_view_sort);
		FileViewIdleExpand* expand = g_new0 (FileViewIdleExpand, 1);
		expand->files = files;
		expand->path = gtk_tree_model_get_path (priv->model, iter);
		expand->view = view;
		expand->uri = uri;
		
		g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
						 (GSourceFunc) file_view_expand_idle, 
						 (gpointer) expand,
						 (GDestroyNotify) g_free);
	}
}

static void
file_view_row_collapsed (GtkTreeView* tree_view, GtkTreeIter* iter,
						 GtkTreePath* path)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (tree_view);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeIter child;
	
	while (gtk_tree_model_iter_children (priv->model, &child, iter))
	{
		gtk_tree_store_remove (GTK_TREE_STORE (priv->model), &child);
	}
	file_view_add_dummy (view, iter);
}

static void
file_view_on_file_clicked (GtkTreeViewColumn* column, AnjutaFileView* view)
{
	
}

static void
file_view_save_expanded_row (GtkTreeView* tree_view, GtkTreePath* path,
							 gpointer user_data)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (tree_view);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	priv->saved_paths = g_list_append (priv->saved_paths, 
									   gtk_tree_path_to_string (path));
}

static void
file_view_get_expanded_rows (AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	
	priv->saved_paths = NULL;
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (view),
									 file_view_save_expanded_row,
									 priv->saved_paths);
}

static gboolean
file_view_expand_row_idle (gpointer user_data)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (user_data);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	
	if (priv->saved_paths)
	{
		gchar* path_string = (gchar*) priv->saved_paths->data;
		GtkTreePath* path = gtk_tree_path_new_from_string (path_string);
		
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), path);
		gtk_tree_path_free (path);
		g_free (path_string);
		priv->saved_paths = g_list_next (priv->saved_paths);
		return TRUE;
	}
	else
	{
		g_list_free (priv->saved_paths);
		priv->saved_paths = NULL;
		return FALSE;
	}	
}

void
file_view_refresh (AnjutaFileView* view, gboolean remember_open)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeStore* store = GTK_TREE_STORE (priv->model);
	GtkTreeIter iter;
	GtkTreePath* tree_path;
	
	g_return_if_fail (priv->base_uri != NULL);
	
	gchar* path = gnome_vfs_get_local_path_from_uri (priv->base_uri);
	gchar* basename = g_path_get_basename (path);
	GdkPixbuf* pixbuf = gdl_icons_get_folder_icon (priv->icons);
	
	if (remember_open)
	{
		file_view_get_expanded_rows (view);
	}
	
	gtk_tree_store_clear (store);
	gtk_tree_store_append (store, &iter, NULL);
	
	gtk_tree_store_set (store, &iter, 
						COLUMN_FILENAME, basename,
						COLUMN_URI, priv->base_uri,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_IS_DIR, TRUE,
						-1);
	g_free (basename);
	g_free (path);
	
	file_view_add_dummy (view, &iter);
	
	tree_path = gtk_tree_path_new_first ();
	gtk_tree_view_expand_row (GTK_TREE_VIEW (view), tree_path, FALSE);
	if (remember_open)
	{
		g_idle_add (file_view_expand_row_idle, view);
	}
}

gchar*
file_view_get_selected (AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeSelection* selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	GtkTreeIter selected;
	gchar* uri;
	if (gtk_tree_selection_get_selected (selection, priv->model, &selected))
	{
		gtk_tree_model_get (priv->model, &selected, COLUMN_URI, &uri, -1);
		return uri;
	}
	else
		return NULL;
}

static gboolean
file_view_button_press_event (GtkWidget* widget, GdkEventButton* event)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (widget);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeIter selected;
	gchar* uri;
	gboolean is_dir;
	
	GtkTreeSelection* selection = 
		gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	
	if (gtk_tree_selection_get_selected (selection, NULL, &selected))
	{
		GtkTreePath* path = gtk_tree_model_get_path (priv->model, &selected);
		gtk_tree_model_get (priv->model, &selected,
							COLUMN_URI, &uri,
							COLUMN_IS_DIR, &is_dir,
							-1);
		
		switch (event->button)
		{
			case 1: /* Left mouse button */
			{
				if (event->type == GDK_2BUTTON_PRESS)
				{
					if (is_dir)
					{
						if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (view),
														 path))
						{
							gtk_tree_view_expand_row (GTK_TREE_VIEW (view),
													  path,
													  FALSE);
						}
						else
						{
							gtk_tree_view_collapse_row (GTK_TREE_VIEW (view),
														path);
						}	
					}
					else
					{
						g_signal_emit_by_name (G_OBJECT (view),
											   "file-open",
											   uri);
					}
				}
				break;
			}
			case 3: /* Right mouse button */
			{
				g_signal_emit_by_name (G_OBJECT (view),
									   "show-popup-menu",
									   uri,
									   is_dir,
									   event->button,
									   event->time);
			}
		}
		gtk_tree_path_free (path);
		g_free (uri);
	}
	return 	
		GTK_WIDGET_CLASS (file_view_parent_class)->button_press_event (widget,
																	   event);
}

static void
file_view_selection_changed (GtkTreeSelection* selection, AnjutaFileView* view)
{
	GtkTreeIter selected;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected (selection, &model, &selected))
	{
		gchar* uri;
		gtk_tree_model_get (model, &selected, COLUMN_URI, &uri, -1);
		g_signal_emit_by_name (G_OBJECT (view), "current-uri-changed",
							   uri, NULL);
	}
	else
	{
		g_signal_emit_by_name (G_OBJECT (view), "current-uri-changed",
							   NULL, NULL);
	}
}

static void
file_view_init (AnjutaFileView *object)
{
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkCellRenderer* renderer_progress;
	GtkTreeViewColumn* column;
	
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);
	
	priv->icons = gdl_icons_new (12);
	priv->model = GTK_TREE_MODEL (gtk_tree_store_new (N_COLUMNS,
													  GDK_TYPE_PIXBUF,
													  G_TYPE_STRING,
													  G_TYPE_STRING,
													  G_TYPE_STRING,
													  G_TYPE_BOOLEAN,
													  G_TYPE_INT,
													  G_TYPE_BOOLEAN));
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (object), priv->model);
	
	renderer_pixbuf = gtk_cell_renderer_pixbuf_new ();
	renderer_text = gtk_cell_renderer_text_new ();
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Filename"));
	gtk_tree_view_column_pack_start (column, renderer_pixbuf, FALSE);
	gtk_tree_view_column_pack_start (column, renderer_text, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer_pixbuf,
										 "pixbuf", COLUMN_PIXBUF,
										 NULL);
	gtk_tree_view_column_set_attributes (column, renderer_text,
										 "text", COLUMN_FILENAME,
										 NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (object), column);
	
	g_signal_connect (G_OBJECT (column), "clicked", 
					  G_CALLBACK (file_view_on_file_clicked), object);
	
	priv->selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
	g_signal_connect (priv->selection, "changed",
					  G_CALLBACK (file_view_selection_changed),
					  object);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (object), FALSE);
}

static void
file_view_get_property (GObject *object, guint prop_id, GValue *value,
						GParamSpec *pspec)
{
	AnjutaFileViewPrivate *priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);

	switch (prop_id)
	{
		case PROP_BASE_URI:
			g_value_set_string (value, priv->base_uri);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
file_view_set_property (GObject *object, guint prop_id, const GValue *value,
						GParamSpec *pspec)
{
	AnjutaFileViewPrivate *priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);

	switch (prop_id)
	{
		case PROP_BASE_URI:
			g_free (priv->base_uri);
			priv->base_uri = g_strdup (g_value_get_string (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
file_view_finalize (GObject *object)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (object);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	
	g_object_unref (priv->icons);
	g_object_unref (priv->selection);
	
	G_OBJECT_CLASS (file_view_parent_class)->finalize (object);
}

static void
file_view_class_init (AnjutaFileViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkTreeViewClass* parent_class = GTK_TREE_VIEW_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (AnjutaFileViewPrivate));
	
	object_class->finalize = file_view_finalize;
	object_class->set_property = file_view_set_property;
	object_class->get_property = file_view_get_property;
	
	g_object_class_install_property (object_class,
									 PROP_BASE_URI,
									 g_param_spec_string ("base_uri",
														  _("Base uri"),
														  _("Uri of the top-most path displayed"),
														  NULL,
														  G_PARAM_READABLE |
														  G_PARAM_WRITABLE |
														  G_PARAM_CONSTRUCT));
	g_signal_new ("file-open",
				  ANJUTA_TYPE_FILE_VIEW,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaFileViewClass, file_open),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__STRING,
				  G_TYPE_NONE,
				  1,
				  G_TYPE_STRING,
				  NULL);
	
	g_signal_new ("current-uri-changed",
				  ANJUTA_TYPE_FILE_VIEW,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaFileViewClass, current_uri_changed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__STRING,
				  G_TYPE_NONE,
				  1,
				  G_TYPE_STRING,
				  NULL);
	
	g_signal_new ("show-popup-menu",
				  ANJUTA_TYPE_FILE_VIEW,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaFileViewClass, show_popup_menu),
				  NULL, NULL,
				  file_view_cclosure_marshal_VOID__STRING_BOOLEAN_INT_INT,
				  G_TYPE_NONE,
				  4,
				  G_TYPE_STRING,
				  G_TYPE_BOOLEAN,
				  G_TYPE_INT,
				  G_TYPE_INT,
				  NULL);
	
	widget_class->button_press_event = file_view_button_press_event;
	
	parent_class->row_collapsed = file_view_row_collapsed;
	parent_class->row_expanded = file_view_row_expanded;
}

GtkWidget*
file_view_new (const gchar* base_uri)
{
	return g_object_new (ANJUTA_TYPE_FILE_VIEW, "base_uri", NULL, NULL);
}

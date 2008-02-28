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
#include "file-model.h"
#include "file-view-marshal.h"

#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrendererprogress.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreemodelsort.h>
#include <gtk/gtkversion.h>

#define HAVE_TOOLTIP_API (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 12))
#include <glib/gi18n.h>

#if HAVE_TOOLTIP_API
#	include <gtk/gtktooltip.h>
#endif

#include <libanjuta/anjuta-debug.h>

typedef struct _AnjutaFileViewPrivate AnjutaFileViewPrivate;

struct _AnjutaFileViewPrivate
{
	FileModel* model;
	
	GList* saved_paths;
	guint refresh_idle_id;
};

#define ANJUTA_FILE_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), ANJUTA_TYPE_FILE_VIEW, AnjutaFileViewPrivate))

G_DEFINE_TYPE (AnjutaFileView, file_view, GTK_TYPE_TREE_VIEW);

enum
{
	PROP_BASE_URI = 1,
	PROP_END
};

static void
file_view_cancel_refresh(AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE(view);
	if (priv->refresh_idle_id)
	{
		GSource* source = g_main_context_find_source_by_id (g_main_context_default(),
															priv->refresh_idle_id);
		g_source_destroy (source);
		priv->refresh_idle_id = 0;
	}
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
		priv->refresh_idle_id = 0;
		return FALSE;
	}	
}

void
file_view_refresh (AnjutaFileView* view, gboolean remember_open)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreePath* tree_path;
	
	file_view_cancel_refresh(view);
	
	if (remember_open)
	{
		file_view_get_expanded_rows (view);
	}
	
	file_model_refresh (priv->model);
	
	tree_path = gtk_tree_path_new_first ();
	gtk_tree_view_expand_row (GTK_TREE_VIEW (view), tree_path, FALSE);
	gtk_tree_path_free (tree_path);
	if (remember_open)
	{
		priv->refresh_idle_id = g_idle_add (file_view_expand_row_idle, view);
	}
}

gchar*
file_view_get_selected (AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeSelection* selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	GtkTreeIter selected;
	if (gtk_tree_selection_get_selected (selection, NULL, &selected))
	{
		gchar* uri = file_model_get_uri (priv->model, &selected);
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
	GtkTreePath* path = NULL;
	
	GtkTreeSelection* selection = 
		gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	
	if (gtk_tree_selection_get_selected (selection, NULL, &selected))
	{
		path = gtk_tree_model_get_path (GTK_TREE_MODEL(priv->model), &selected);
		gtk_tree_model_get (GTK_TREE_MODEL(priv->model), &selected,
							COLUMN_URI, &uri,
							COLUMN_IS_DIR, &is_dir,
							-1);
	}
	else
	{
		uri = NULL;
		is_dir = FALSE;
	}
		
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
	if (path != NULL)
		gtk_tree_path_free (path);
	g_free (uri);
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
		gchar* uri = file_model_get_uri (FILE_MODEL(model), &selected);
		g_signal_emit_by_name (G_OBJECT (view), "current-uri-changed",
							   uri, NULL);
		g_free(uri);
	}
	else
	{
		g_signal_emit_by_name (G_OBJECT (view), "current-uri-changed",
							   NULL, NULL);
	}
}

#if HAVE_TOOLTIP_API
static gboolean
file_view_query_tooltip (GtkWidget* widget, gint x, gint y, gboolean keyboard,
						 GtkTooltip* tooltip)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (widget);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeModel* model = GTK_TREE_MODEL (priv->model);
	GtkTreePath* path;
	GtkTreeIter iter;
	
	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(view),
								   x, y, &path, NULL, NULL, NULL);
	if (path)
	{
		if (gtk_tree_model_get_iter (model, &iter, path))
		{
			gchar* filename = file_model_get_filename (priv->model, &iter);
			gtk_tooltip_set_markup (tooltip, filename);
			g_free(filename);
			gtk_tree_path_free (path);
			return TRUE;
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}
#endif
		
static void
file_view_init (AnjutaFileView *object)
{
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkTreeViewColumn* column;
	GtkTreeSelection* selection;
	
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);
	
	priv->model = file_model_new (GTK_TREE_VIEW(object), NULL);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (object), GTK_TREE_MODEL(priv->model));
	
	renderer_pixbuf = gtk_cell_renderer_pixbuf_new ();
	renderer_text = gtk_cell_renderer_text_new ();
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Filename"));
	gtk_tree_view_column_pack_start (column, renderer_pixbuf, FALSE);
	gtk_tree_view_column_pack_start (column, renderer_text, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer_pixbuf,
										 "pixbuf", COLUMN_PIXBUF, NULL);
	gtk_tree_view_column_set_attributes (column, renderer_text,
										 "text", COLUMN_FILENAME, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (object), column);
	
	g_signal_connect (G_OBJECT (column), "clicked", 
					  G_CALLBACK (file_view_on_file_clicked), object);
	
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
	g_signal_connect (selection, "changed",
					  G_CALLBACK (file_view_selection_changed), object);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (object), FALSE);
	
#if HAVE_TOOLTIP_API
	g_object_set (object, "has-tooltip", TRUE, NULL);
#endif
}

static void
file_view_get_property (GObject *object, guint prop_id, GValue *value,
						GParamSpec *pspec)
{
	AnjutaFileViewPrivate *priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);
	gchar* uri;
	
	switch (prop_id)
	{
		case PROP_BASE_URI:
			g_object_get (G_OBJECT(priv->model), "base_uri", &uri, NULL);
			g_value_set_string (value, uri);
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
			g_object_set (G_OBJECT (priv->model), "base_uri", g_value_get_string (value),
						  NULL);
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
	
	/* Tooltips */
#if HAVE_TOOLTIP_API
	widget_class->query_tooltip = file_view_query_tooltip;
#endif
	
}

GtkWidget*
file_view_new (void)
{
	return g_object_new (ANJUTA_TYPE_FILE_VIEW, NULL);
}

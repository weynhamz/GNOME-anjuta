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

#include <gio/gio.h>

#include <string.h>

#define HAVE_TOOLTIP_API (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 12))
#include <glib/gi18n.h>

#include <gtk/gtktooltip.h>

#include <libanjuta/anjuta-debug.h>

typedef struct _AnjutaFileViewPrivate AnjutaFileViewPrivate;

struct _AnjutaFileViewPrivate
{
	FileModel* model;
	
	GList* saved_paths;
	GtkTreeRowReference* current_selection;
};

#define ANJUTA_FILE_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), ANJUTA_TYPE_FILE_VIEW, AnjutaFileViewPrivate))

G_DEFINE_TYPE (AnjutaFileView, file_view, GTK_TYPE_TREE_VIEW);

enum
{
	PROP_BASE_URI = 1,
	PROP_END
};

void
file_view_refresh (AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreePath* tree_path;
	
	file_model_refresh (priv->model);
	
	tree_path = gtk_tree_path_new_first ();
	gtk_tree_view_expand_row (GTK_TREE_VIEW (view), tree_path, FALSE);
	gtk_tree_path_free (tree_path);
}

void file_view_rename (AnjutaFileView* view)
{
	/* TODO */
}

gboolean file_view_can_rename (AnjutaFileView* view)
{
	/* TODO */
	return FALSE;
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
		GtkTreeIter select_iter;
		GtkTreeModel* sort_model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
													   &select_iter, &selected);
		gtk_tree_model_get (GTK_TREE_MODEL(priv->model), &select_iter,
							COLUMN_IS_DIR, &is_dir,
							-1);
		uri = file_model_get_uri (priv->model, &select_iter);
		
		path = gtk_tree_model_get_path(sort_model, &selected);
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
	g_free (uri);
	if (path)
		gtk_tree_path_free(path);
	return 	
		GTK_WIDGET_CLASS (file_view_parent_class)->button_press_event (widget,
																	   event);
}

static void
file_view_show_extended_data (AnjutaFileView* view, GtkTreeIter* iter)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeModel* file_model = GTK_TREE_MODEL (priv->model);
	GFile* file;
	GFileInfo* file_info;
	gboolean is_dir;
		
	gtk_tree_model_get (file_model, iter, COLUMN_IS_DIR, &is_dir, -1);
	if (!is_dir)
	{
		gchar* display;
		gchar time_str[128];
		gtk_tree_model_get (file_model, iter, COLUMN_FILE, &file, -1);
		time_t time;
		
		file_info = g_file_query_info (file,
									   "standard::*,time::changed",
									   G_FILE_QUERY_INFO_NONE,
									   NULL, NULL);
		time = g_file_info_get_attribute_uint64(file_info, "time::changed");
		strftime(time_str, 127, "%x %X", localtime(&time));
		display = g_markup_printf_escaped("%s\n"
										  "<small><tt>%s</tt></small>",
										  g_file_info_get_display_name(file_info),
										  time_str);
		
		gtk_tree_store_set (GTK_TREE_STORE(file_model), iter,
							COLUMN_DISPLAY, display,
							-1);
		
		g_object_unref (file_info);
		g_free(display);
	}
}

static void
file_view_selection_changed (GtkTreeSelection* selection, AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeModel* file_model = GTK_TREE_MODEL(priv->model);
	GtkTreeIter selected;
	GtkTreeModel* model = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	
	if (priv->current_selection)
	{
		GtkTreeIter iter;
		GtkTreePath* path = gtk_tree_row_reference_get_path (priv->current_selection);
		if (path && gtk_tree_model_get_iter (file_model, &iter, path))
		{
			gchar* filename;
			gtk_tree_model_get (file_model, &iter, COLUMN_FILENAME, &filename, -1);
			gtk_tree_store_set (GTK_TREE_STORE (file_model), &iter,
								COLUMN_DISPLAY, filename, -1);
			g_free(filename);
			gtk_tree_path_free(path);
		}
		gtk_tree_row_reference_free(priv->current_selection);
		priv->current_selection = NULL;
	}
	
	if (gtk_tree_selection_get_selected (selection, &model, &selected))
	{
		GtkTreeIter real_selection;
		GtkTreePath* path;
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(model),
												   &real_selection, &selected);
		
		path = gtk_tree_model_get_path(file_model, &real_selection);
		priv->current_selection = gtk_tree_row_reference_new (file_model, path);
		gtk_tree_path_free(path);
		
		file_view_show_extended_data (view, &real_selection);
		
		gchar* uri = file_model_get_uri (FILE_MODEL(file_model), &real_selection);
		g_signal_emit_by_name (G_OBJECT (view), "current-uri-changed",
							   uri, NULL);
		g_free(uri);
	}
	else
	{
		g_signal_emit_by_name (G_OBJECT (view), "current-uri-changed",
							   NULL, NULL);
	}
	DEBUG_PRINT ("selection_changed");
}

static gboolean
file_view_query_tooltip (GtkWidget* widget, gint x, gint y, gboolean keyboard,
						 GtkTooltip* tooltip)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (widget);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeModel* model_sort;
	GtkTreeModel* file_model = GTK_TREE_MODEL (priv->model);
	GtkTreePath* path;
	GtkTreeIter iter;
	GtkTreeIter real_iter;
	gchar* filename;
	
	if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (view),
											&x, &y, keyboard,
											&model_sort,
											&path,
											&iter))
		return FALSE;
	
	gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (model_sort),
											&real_iter, &iter);
	
	filename = file_model_get_filename (FILE_MODEL (file_model), &real_iter);
	gtk_tooltip_set_text (tooltip, filename);
	gtk_tree_view_set_tooltip_row (GTK_TREE_VIEW (view),
								   tooltip,
								   path);
	
	g_free (filename);
	gtk_tree_path_free (path);
	
	return TRUE;
}

static int
file_view_sort_model(GtkTreeModel* model, 
					 GtkTreeIter* iter1, 
					 GtkTreeIter* iter2,
					 gpointer null)
{
	gint sort1, sort2;
	gchar *filename1 = NULL, *filename2 = NULL;
	gboolean is_dir1, is_dir2;
	gint retval = 0;
	
	gtk_tree_model_get (model, iter1, 
						COLUMN_FILENAME, &filename1,
						COLUMN_SORT, &sort1,
						COLUMN_IS_DIR, &is_dir1, -1);
	gtk_tree_model_get (model, iter2, 
						COLUMN_FILENAME, &filename2,
						COLUMN_SORT, &sort2,
						COLUMN_IS_DIR, &is_dir2, -1);
	
	if (sort1 != sort2)
	{
		retval = sort2 - sort1;
	}
	else if (is_dir1 != is_dir2)
	{
		retval = is_dir1 ? -1 : 1;
	}
	else if (filename1 && filename2)
	{
		retval = strcmp(filename1, filename2);
	}
	g_free(filename1);
	g_free(filename2);
	
	return retval;
}

static void
file_view_init (AnjutaFileView *object)
{
	GtkCellRenderer* renderer_pixbuf;
	GtkCellRenderer* renderer_display;
	GtkTreeViewColumn* column;
	GtkTreeSelection* selection;
	GtkTreeModel* sort_model;
	
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);
	
	priv->current_selection = NULL;
	
	priv->model = file_model_new (GTK_TREE_VIEW(object), NULL);
	sort_model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(priv->model));									  
	
	gtk_tree_view_set_model (GTK_TREE_VIEW(object), sort_model);
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE(sort_model),
											 file_view_sort_model,
											 NULL,
											 NULL);
	
	renderer_pixbuf = gtk_cell_renderer_pixbuf_new ();
	renderer_display = gtk_cell_renderer_text_new ();
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Filename"));
	gtk_tree_view_column_pack_start (column, renderer_pixbuf, FALSE);
	gtk_tree_view_column_pack_start (column, renderer_display, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer_pixbuf,
										 "pixbuf", COLUMN_PIXBUF, NULL);
	gtk_tree_view_column_set_attributes (column, renderer_display,
										 "markup", COLUMN_DISPLAY, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (object), column);
	
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
	g_signal_connect (selection, "changed",
					  G_CALLBACK (file_view_selection_changed), object);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (object), FALSE);
	
	g_object_set (object, "has-tooltip", TRUE, NULL);
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
	G_OBJECT_CLASS (file_view_parent_class)->finalize (object);
}

static void
file_view_class_init (AnjutaFileViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
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
	widget_class->query_tooltip = file_view_query_tooltip;	
}

GtkWidget*
file_view_new (void)
{
	return g_object_new (ANJUTA_TYPE_FILE_VIEW, NULL);
}

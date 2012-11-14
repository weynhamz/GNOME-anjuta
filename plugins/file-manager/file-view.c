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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <gio/gio.h>

#include <string.h>

#define HAVE_TOOLTIP_API (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 12))
#include <glib/gi18n.h>

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-vcs-status.h>

typedef struct _AnjutaFileViewPrivate AnjutaFileViewPrivate;

struct _AnjutaFileViewPrivate
{
	FileModel* model;
	
	GList* saved_paths;
	GtkTreeRowReference* current_selection;
};

GtkTargetEntry uri_targets[] =
{
	{
		"text/uri-list",
		0, /* no flags */
		0
	}
};
	

#define ANJUTA_FILE_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), ANJUTA_TYPE_FILE_VIEW, AnjutaFileViewPrivate))

G_DEFINE_TYPE (AnjutaFileView, file_view, GTK_TYPE_TREE_VIEW);

static const gchar*
get_status_string(AnjutaVcsStatus status)
{
	switch (status)
	{
		case ANJUTA_VCS_STATUS_NONE:
			return NULL;
		case ANJUTA_VCS_STATUS_MODIFIED:
			return _("Modified");
		case ANJUTA_VCS_STATUS_DELETED:
			return _("Deleted");
		case ANJUTA_VCS_STATUS_ADDED:
			return _("Added");
		case ANJUTA_VCS_STATUS_CONFLICTED:
			return _("Conflicted");
		case ANJUTA_VCS_STATUS_UPTODATE:
			return _("Up-to-date");
		case ANJUTA_VCS_STATUS_LOCKED:
			return _("Locked");
		case ANJUTA_VCS_STATUS_MISSING:
			return _("Missing");
		case ANJUTA_VCS_STATUS_UNVERSIONED:
			return _("Unversioned");
		case ANJUTA_VCS_STATUS_IGNORED:
			return _("Ignored");
		default:
			break;
	}		

	return NULL;
}

enum
{
	PROP_BASE_PATH = 1,
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

GFile*
file_view_get_selected (AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeSelection* selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	GtkTreeIter selected;
	if (gtk_tree_selection_get_selected (selection, NULL, &selected))
	{
		GFile* file;
		GtkTreeIter real_iter;
		GtkTreeModel* sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW(view));

		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
		                                               &real_iter, &selected);

		file = file_model_get_file (priv->model, &real_iter);
		return file;
	}
	else
		return NULL;
}

static void
file_view_drag_data_get (GtkWidget* widget,
                         GdkDragContext *drag_context,
                         GtkSelectionData *data,
                         guint info,
                         guint time)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW(widget);
	GFile* selected = file_view_get_selected (view);

	if (selected)
	{
		gchar* uris[2];
		uris[0] = g_file_get_uri (selected);
		uris[1] = NULL;
		gtk_selection_data_set_uris (data, uris);
		g_free(uris[0]);
	}
}

static void
file_view_row_activated (GtkTreeView* widget, GtkTreePath* sort_path,
						 GtkTreeViewColumn* column)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (widget);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeIter selected;
	GtkTreePath* path = NULL;
	gboolean is_dir;
	GFile* file;
	
	GtkTreeIter select_iter;
	GtkTreeModel* sort_model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter (sort_model, &selected, sort_path);
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
												   &select_iter, &selected);
	gtk_tree_model_get (GTK_TREE_MODEL(priv->model), &select_iter,
						COLUMN_IS_DIR, &is_dir,
						-1);
	file = file_model_get_file (priv->model, &select_iter);
	
	path = gtk_tree_model_get_path(sort_model, &selected);
		
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
							   file);
	}
	if (file)
		g_object_unref (file);
	if (path)
		gtk_tree_path_free(path);
}

static gboolean
file_view_key_press_event (GtkWidget* widget, GdkEventKey* event)
{
	if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter)
	{
		AnjutaFileView* view = ANJUTA_FILE_VIEW (widget);
		AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
		GtkTreeIter selected;
	
		GtkTreeSelection* selection = 
			gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	
		if (gtk_tree_selection_get_selected (selection, NULL, &selected))
		{
			GFile* file;
			GtkTreeIter select_iter;
			GtkTreeModel* sort_model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
			gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
													   &select_iter, &selected);
			file = file_model_get_file (priv->model, &select_iter);
			if (file != NULL)
			{
				g_signal_emit_by_name (G_OBJECT (view),
												"file-open",
												 file);
				g_object_unref (file);
			}
		}
	}
	return 	
		GTK_WIDGET_CLASS (file_view_parent_class)->key_press_event (widget,
																	event);
}

static void 
file_view_do_popup_menu (GtkWidget* widget, GdkEventButton* event)
{
	AnjutaFileView* view = ANJUTA_FILE_VIEW (widget);
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
	GtkTreeSelection* selection = 
			gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	GFile* file = NULL;
	gboolean is_dir = FALSE;
	GtkTreeIter selected;
	gint button, event_time;
	
	if (gtk_tree_selection_get_selected (selection, NULL, &selected))
	{
		GtkTreeIter select_iter;
		GtkTreeModel* sort_model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort_model),
													   &select_iter, &selected);
		gtk_tree_model_get (GTK_TREE_MODEL(priv->model), &select_iter,
							COLUMN_IS_DIR, &is_dir,
							-1);
		file = file_model_get_file (priv->model, &select_iter);
		
	}
	if (event)
	{
		button = event->button;
		event_time = event->time;
	}
	else
	{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}
		
	g_signal_emit_by_name (G_OBJECT(widget), "show-popup-menu",
						   file, is_dir, button, event_time);
	if (file)
		g_object_unref (file);
}

static gboolean
file_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	gint retval = 
		GTK_WIDGET_CLASS (file_view_parent_class)->button_press_event (widget,
																	   event);
	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
		GtkTreePath *path;

		/* Select file under cursor if not selected */
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                        event->x,event->y, &path, NULL, NULL, NULL))
		{
			GtkTreeSelection *selection;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (widget));
	        if (!gtk_tree_selection_path_is_selected(selection, path))
    	    {
        	    gtk_tree_selection_unselect_all(selection);
	            gtk_tree_selection_select_path(selection, path);
    	        gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget),
        	                        path, NULL, FALSE);
        	}
        	gtk_tree_path_free (path);
			
			file_view_do_popup_menu (widget, event);
		
      		return TRUE;
		}
    }

	return 	retval;
		
}

static gboolean
file_view_popup_menu (GtkWidget* widget)
{
	file_view_do_popup_menu(widget, NULL);
	return TRUE;
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
		AnjutaVcsStatus status;
		gtk_tree_model_get (file_model, iter, COLUMN_FILE, &file, 
							COLUMN_STATUS, &status, -1);
		time_t time;
		
		file_info = g_file_query_info (file,
									   "standard::*,time::changed",
									   G_FILE_QUERY_INFO_NONE,
									   NULL, NULL);
		time = g_file_info_get_attribute_uint64(file_info, "time::changed");
		g_object_unref (file);
		strftime(time_str, 127, "%x %X", localtime(&time));
		if (get_status_string(status))
		{
			display = g_markup_printf_escaped("%s\n"
											  "<small><tt>%s</tt></small>\n"
											  "<small>%s</small>",
											  g_file_info_get_display_name(file_info),
											  time_str,
											  get_status_string(status));
		}
		else
		{
			display = g_markup_printf_escaped("%s\n"
											  "<small><tt>%s</tt></small>",
											  g_file_info_get_display_name(file_info),
											  time_str);
		}
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
		GFile* file;
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(model),
												   &real_selection, &selected);
		
		path = gtk_tree_model_get_path(file_model, &real_selection);
		priv->current_selection = gtk_tree_row_reference_new (file_model, path);
		gtk_tree_path_free(path);
		
		file_view_show_extended_data (view, &real_selection);
		
		file = file_model_get_file(FILE_MODEL(file_model), &real_selection);
		g_signal_emit_by_name (G_OBJECT (view), "current-file-changed",
							   file, NULL);
		g_object_unref (file);
	}
	else
	{
		g_signal_emit_by_name (G_OBJECT (view), "current-file-changed",
							   NULL, NULL);
	}
	DEBUG_PRINT ("%s", "selection_changed");
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
	gboolean result = TRUE;
	GdkRectangle visible_rect, column_rect;
	GdkScreen *screen = gdk_screen_get_default ();
	PangoContext *context;
	PangoLayout *layout;
	gint width, height;
	
	if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (view),
											&x, &y, keyboard,
											&model_sort,
											&path,
											&iter))
		return FALSE;
	
	gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (model_sort),
											&real_iter, &iter);
	
	filename = file_model_get_filename (FILE_MODEL (file_model), &real_iter);

	context = gdk_pango_context_get_for_screen (screen);
	layout = pango_layout_new (context);

	pango_layout_set_text (layout, filename, -1);
	pango_layout_get_pixel_size (layout, &width, &height);

	gtk_tree_view_get_visible_rect (GTK_TREE_VIEW (view), &visible_rect);
	gtk_tree_view_get_cell_area (GTK_TREE_VIEW (view), path,
								 gtk_tree_view_get_column (GTK_TREE_VIEW (view), 0), &column_rect);

	if (column_rect.x + width > visible_rect.x + visible_rect.width ||
		column_rect.x < visible_rect.x)
	{
		gtk_tooltip_set_text (tooltip, filename);
		gtk_tree_view_set_tooltip_row (GTK_TREE_VIEW (view),
									   tooltip,
									   path);
	} else
	{
		result = FALSE;
	}

	g_free (filename);
	gtk_tree_path_free (path);
	g_object_unref (layout);
	g_object_unref (context);
	
	return result;
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
		retval = strcasecmp(filename1, filename2);
	}
	g_free(filename1);
	g_free(filename2);
	
	return retval;
}

#define EMBLEM_ADDED "vcs-added.png"
#define EMBLEM_CONFLICT "vcs-conflict.png"
#define EMBLEM_DELETED "vcs-deleted.png"
#define EMBLEM_IGNORED "vcs-ignored.png"
#define EMBLEM_LOCKED "vcs-locked.png"
#define EMBLEM_UNVERSIONED "vcs-unversioned.png"
#define EMBLEM_UPTODATE "vcs-updated.png"
#define EMBLEM_MODIFIED "vcs-modified.png"

#define COMPOSITE_ALPHA 225

static GdkPixbuf* 
get_vcs_emblem (AnjutaVcsStatus status)
{
	GdkPixbuf* emblem ;
	switch (status)
	{
		case ANJUTA_VCS_STATUS_ADDED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_ADDED, NULL);
			break;
		case ANJUTA_VCS_STATUS_MODIFIED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_MODIFIED, NULL);
			break;
		case ANJUTA_VCS_STATUS_DELETED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_DELETED, NULL);
			break;
		case ANJUTA_VCS_STATUS_CONFLICTED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_CONFLICT, NULL);
			break;
		case ANJUTA_VCS_STATUS_LOCKED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_LOCKED, NULL);
			break;
		case ANJUTA_VCS_STATUS_UNVERSIONED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_UNVERSIONED, NULL);
			break;
		case ANJUTA_VCS_STATUS_UPTODATE:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_UPTODATE, NULL);
			break;
		case ANJUTA_VCS_STATUS_IGNORED:
			emblem = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"EMBLEM_IGNORED, NULL);
			break;
		default:
			emblem = NULL;
	}
	return emblem;
}

static void
file_view_render_pixbuf_with_emblem (GtkTreeViewColumn* tree_column,
                                     GtkCellRenderer* cell,
                                     GtkTreeModel* tree_model,
                                     GtkTreeIter* iter,
                                     gpointer data)
{
	GdkPixbuf* file_icon = NULL;
	GdkPixbuf* emblem = NULL;

	AnjutaVcsStatus status;
	
	gtk_tree_model_get (tree_model, iter,
	                    COLUMN_STATUS, &status, 
	                    COLUMN_PIXBUF, &file_icon, -1);
	
	if (file_icon)
	{
		emblem = get_vcs_emblem (status);
		if (emblem)
		{
			GdkPixbuf *new_icon;

			new_icon = gdk_pixbuf_copy (file_icon);
			gdk_pixbuf_composite (emblem,
			                      new_icon,
			                      0, 0,
			                      gdk_pixbuf_get_width (file_icon),
			                      gdk_pixbuf_get_height (file_icon),
			                      0, 0,
			                      1, 1,
			                      GDK_INTERP_BILINEAR,
			                      COMPOSITE_ALPHA);

			g_object_set (cell, "pixbuf", new_icon, NULL);
			
			g_object_unref (new_icon);
			g_object_unref (emblem);
		}
		else
		{
			g_object_set (cell, "pixbuf", file_icon, NULL);
		}
		g_object_unref (file_icon);
	}
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
	gtk_tree_view_column_set_cell_data_func(column,
	                                        renderer_pixbuf,
	                                        file_view_render_pixbuf_with_emblem,
	                                        object, NULL);
	gtk_tree_view_column_set_attributes (column, renderer_display,
										 "markup", COLUMN_DISPLAY, 
										 NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (object), column);
	
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
	g_signal_connect (selection, "changed",
					  G_CALLBACK (file_view_selection_changed), object);

	/* DND */
	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW(object),
	                                        GDK_BUTTON1_MASK,
	                                        uri_targets,
	                                        1,
	                                        GDK_ACTION_MOVE);                                    
}

static void
file_view_get_property (GObject *object, guint prop_id, GValue *value,
						GParamSpec *pspec)
{
	AnjutaFileViewPrivate *priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);
	
	switch (prop_id)
	{
		case PROP_BASE_PATH:
			g_object_get_property (G_OBJECT(priv->model), "base-path", value);
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
		case PROP_BASE_PATH:
			g_object_set_property (G_OBJECT (priv->model), "base-path", value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
file_view_finalize (GObject *object)
{
	AnjutaFileViewPrivate *priv = ANJUTA_FILE_VIEW_GET_PRIVATE (object);

	g_object_unref (priv->model);

	G_OBJECT_CLASS (file_view_parent_class)->finalize (object);
}

static void
file_view_class_init (AnjutaFileViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	GtkTreeViewClass* tree_class = GTK_TREE_VIEW_CLASS(klass);
	
	g_type_class_add_private (klass, sizeof (AnjutaFileViewPrivate));
	
	object_class->finalize = file_view_finalize;
	object_class->set_property = file_view_set_property;
	object_class->get_property = file_view_get_property;
	
	g_object_class_install_property (object_class,
									 PROP_BASE_PATH,
									 g_param_spec_object ("base-path",
														  _("Base Path"),
														  _("GFile representing the top-most path displayed"),
														  G_TYPE_FILE,
														  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_signal_new ("file-open",
				  ANJUTA_TYPE_FILE_VIEW,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaFileViewClass, file_open),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__OBJECT,
				  G_TYPE_NONE,
				  1,
				  G_TYPE_OBJECT,
				  NULL);
	
	g_signal_new ("current-file-changed",
				  ANJUTA_TYPE_FILE_VIEW,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaFileViewClass, current_uri_changed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__OBJECT,
				  G_TYPE_NONE,
				  1,
				  G_TYPE_OBJECT,
				  NULL);
	
	g_signal_new ("show-popup-menu",
				  ANJUTA_TYPE_FILE_VIEW,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaFileViewClass, show_popup_menu),
				  NULL, NULL,
				  file_view_cclosure_marshal_VOID__OBJECT_BOOLEAN_INT_INT,
				  G_TYPE_NONE,
				  4,
				  G_TYPE_OBJECT,
				  G_TYPE_BOOLEAN,
				  G_TYPE_INT,
				  G_TYPE_INT,
				  NULL);
	
	tree_class->row_activated = file_view_row_activated;
	widget_class->key_press_event = file_view_key_press_event;
	widget_class->popup_menu = file_view_popup_menu;
	widget_class->button_press_event = file_view_button_press_event;
	
	/* Tooltips */
	widget_class->query_tooltip = file_view_query_tooltip;

	/* Dnd */
	widget_class->drag_data_get = file_view_drag_data_get;
}

void 
file_view_refresh_vcs (AnjutaFileView* view)
{
	AnjutaFileViewPrivate* priv = ANJUTA_FILE_VIEW_GET_PRIVATE (view);
    file_model_update_vcs_status(priv->model);
}


GtkWidget*
file_view_new (void)
{
	return g_object_new (ANJUTA_TYPE_FILE_VIEW, 
	                     "headers-visible", FALSE,
	                     "has-tooltip", TRUE,
	                     "enable-search", FALSE,
	                     NULL);
}

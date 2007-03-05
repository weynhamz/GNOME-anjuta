/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_view.c
 * Copyright (C) 2004 Naba Kumar
 * Copyright (C) 2006 Martin Stubenschrott <stubenschrott@gmx.net>: thankyou for
 * the code of IComplete [which anyway here is heavily modified] and the ideas
 * for autocompletion
 * Copyright (C) 2006 Massimo Cora' <maxcvs@email.it> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <libgnomeui/gnome-stock-icons.h> 
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-macros.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>
#include <tm_tagmanager.h>
#include "an_symbol_view.h"
#include "an_symbol_info.h"
#include "an_symbol_prefs.h"

#define MAX_STRING_LENGTH 256
#define TOOLTIP_TIMEOUT 1000 /* milliseconds */


struct _AnjutaSymbolViewPriv
{
	TMWorkObject *tm_project;
	const TMWorkspace *tm_workspace;
	GHashTable *tm_files;
	GtkTreeModel *file_symbol_model;
	TMSymbol *symbols;
	gboolean symbols_need_update;
	
	/* Tooltips */
	GdkRectangle tooltip_rect;
	GtkWidget *tooltip_window;
	gulong tooltip_timeout;
	PangoLayout *tooltip_layout;
};

enum
{
	PIXBUF_COLUMN,
	NAME_COLUMN,
	SVFILE_ENTRY_COLUMN,
	SYMBOL_NODE,
	COLUMNS_NB
};


static 	gchar *keywords[] = { 
			"asm",
			"auto",
			"bool",
			"break",
			"case",
			"catch",
			"class",
			"const",
			"const_cast",
			"continue",
			"default",
			"delete",
			"do",
			"dynamic_cast",
			"else",
			"enum",
			"explicit",
			"export",
			"extern",
			"false",
			"for",
			"friend",
			"goto",
			"if",
			"inline",
			"mutable",
			"namespace",
			"new",
			"operator",
			"private",
			"protected",
			"public",
			"register",
			"reinterpret_cast",
			"return",
			"signed",
			"sizeof",
			"static",
			"static_cast",
			"struct",
			"switch",
			"template",
			"this",
			"throw",
			"true",
			"try",
			"typedef",
			"typeid",
			"typename",
			"union",
			"unsigned",
			"using",
			"virtual",
			"volatile",
			"while",
			NULL
		};
	

static GtkTreeViewClass *parent_class;

/* Tooltip operations -- taken from gtodo/message_view */

static gchar *
tooltip_get_display_text (AnjutaSymbolView * sv)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
	
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(sv),
		sv->priv->tooltip_rect.x, sv->priv->tooltip_rect.y,
		&path, NULL, NULL, NULL))
	{
		gchar *text;
		
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &text, -1); 
		gtk_tree_path_free(path);
				
		return text;
	}
	return NULL;
}

static void
tooltip_paint (GtkWidget *widget, GdkEventExpose *event, AnjutaSymbolView * sv)
{
	GtkStyle *style;
	gchar *tooltiptext;

	tooltiptext = tooltip_get_display_text (sv);
	
	if (!tooltiptext)
		tooltiptext = g_strdup (_("No message details"));

	pango_layout_set_markup (sv->priv->tooltip_layout,
							 tooltiptext,
							 strlen (tooltiptext));
	pango_layout_set_wrap(sv->priv->tooltip_layout, PANGO_WRAP_CHAR);
	pango_layout_set_width(sv->priv->tooltip_layout, 600000);
	style = sv->priv->tooltip_window->style;

	gtk_paint_flat_box (style, sv->priv->tooltip_window->window,
						GTK_STATE_NORMAL, GTK_SHADOW_OUT,
						NULL, sv->priv->tooltip_window,
						"tooltip", 0, 0, -1, -1);

	gtk_paint_layout (style, sv->priv->tooltip_window->window,
					  GTK_STATE_NORMAL, TRUE,
					  NULL, sv->priv->tooltip_window,
					  "tooltip", 4, 4, sv->priv->tooltip_layout);
	/*
	   g_object_unref(layout);
	   */
	g_free(tooltiptext);
	return;
}

static gboolean
tooltip_timeout (AnjutaSymbolView * sv)
{
	gint scr_w,scr_h, w, h, x, y;
	gchar *tooltiptext;

	tooltiptext = tooltip_get_display_text (sv);
	
	if (!tooltiptext)
		tooltiptext = g_strdup (_("No file details"));
	
	sv->priv->tooltip_window = gtk_window_new (GTK_WINDOW_POPUP);
	sv->priv->tooltip_window->parent = GTK_WIDGET(sv);
	gtk_widget_set_app_paintable (sv->priv->tooltip_window, TRUE);
	gtk_window_set_resizable (GTK_WINDOW(sv->priv->tooltip_window), FALSE);
	gtk_widget_set_name (sv->priv->tooltip_window, "gtk-tooltips");
	g_signal_connect (G_OBJECT(sv->priv->tooltip_window), "expose_event",
					  G_CALLBACK(tooltip_paint), sv);
	gtk_widget_ensure_style (sv->priv->tooltip_window);

	sv->priv->tooltip_layout =
		gtk_widget_create_pango_layout (sv->priv->tooltip_window, NULL);
	pango_layout_set_wrap (sv->priv->tooltip_layout, PANGO_WRAP_CHAR);
	pango_layout_set_width (sv->priv->tooltip_layout, 600000);
	pango_layout_set_markup (sv->priv->tooltip_layout, tooltiptext,
							 strlen (tooltiptext));
	scr_w = gdk_screen_width();
	scr_h = gdk_screen_height();
	pango_layout_get_size (sv->priv->tooltip_layout, &w, &h);
	w = PANGO_PIXELS(w) + 8;
	h = PANGO_PIXELS(h) + 8;

	gdk_window_get_pointer (NULL, &x, &y, NULL);
	if (GTK_WIDGET_NO_WINDOW (sv))
		y += GTK_WIDGET(sv)->allocation.y;

	x -= ((w >> 1) + 4);

	if ((x + w) > scr_w)
		x -= (x + w) - scr_w;
	else if (x < 0)
		x = 0;

	if ((y + h + 4) > scr_h)
		y = y - h;
	else
		y = y + 6;
	/*
	   g_object_unref(layout);
	   */
	gtk_widget_set_size_request (sv->priv->tooltip_window, w, h);
	gtk_window_move (GTK_WINDOW (sv->priv->tooltip_window), x, y);
	gtk_widget_show (sv->priv->tooltip_window);
	g_free (tooltiptext);
	
	return FALSE;
}

static gboolean
tooltip_motion_cb (GtkWidget *tv, GdkEventMotion *event, AnjutaSymbolView * sv)
{
	GtkTreePath *path;
	
	if (sv->priv->tooltip_rect.y == 0 &&
		sv->priv->tooltip_rect.height == 0 &&
		sv->priv->tooltip_timeout)
	{
		g_source_remove (sv->priv->tooltip_timeout);
		sv->priv->tooltip_timeout = 0;
		if (sv->priv->tooltip_window) {
			gtk_widget_destroy (sv->priv->tooltip_window);
			sv->priv->tooltip_window = NULL;
		}
		return FALSE;
	}
	if (sv->priv->tooltip_timeout) {
		if (((int)event->y > sv->priv->tooltip_rect.y) &&
			(((int)event->y - sv->priv->tooltip_rect.height)
				< sv->priv->tooltip_rect.y))
			return FALSE;

		if(event->y == 0)
		{
			g_source_remove (sv->priv->tooltip_timeout);
			sv->priv->tooltip_timeout = 0;
			return FALSE;
		}
		/* We've left the cell.  Remove the timeout and create a new one below */
		if (sv->priv->tooltip_window) {
			gtk_widget_destroy (sv->priv->tooltip_window);
			sv->priv->tooltip_window = NULL;
		}
		g_source_remove (sv->priv->tooltip_timeout);
		sv->priv->tooltip_timeout = 0;
	}

	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(sv),
									   event->x, event->y, &path,
									   NULL, NULL, NULL))
	{
		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (sv),
									 path, NULL, &sv->priv->tooltip_rect);
		
		if (sv->priv->tooltip_rect.y != 0 &&
			sv->priv->tooltip_rect.height != 0)
		{
			gchar *tooltiptext;
			
			tooltiptext = tooltip_get_display_text (sv);
			if (tooltiptext == NULL)
				return FALSE;
			g_free (tooltiptext);
			
			sv->priv->tooltip_timeout =
				g_timeout_add (TOOLTIP_TIMEOUT,
							   (GSourceFunc) tooltip_timeout, sv);
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}

static void
tooltip_leave_cb (GtkWidget *w, GdkEventCrossing *e, AnjutaSymbolView * sv)
{
	if (sv->priv->tooltip_timeout) {
		g_source_remove (sv->priv->tooltip_timeout);
		sv->priv->tooltip_timeout = 0;
	}
	if (sv->priv->tooltip_window) {
		gtk_widget_destroy (sv->priv->tooltip_window);
		g_object_unref (sv->priv->tooltip_layout);
		sv->priv->tooltip_window = NULL;
	}
}

static void anjuta_symbol_view_add_children (AnjutaSymbolView *sv,
											 TMSymbol *sym,
											 GtkTreeStore *store,
											 GtkTreeIter *iter);
static void anjuta_symbol_view_refresh_tree (AnjutaSymbolView *sv);

static void
destroy_tm_hash_value (gpointer data)
{
	AnjutaSymbolView *sv;
	TMWorkObject *tm_file;

	sv = g_object_get_data (G_OBJECT (data), "symbol_view");
	tm_file = g_object_get_data (G_OBJECT (data), "tm_file");

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	if (tm_file)
	{
		if (tm_file->parent ==
		    TM_WORK_OBJECT (sv->priv->tm_workspace))
		{
			DEBUG_PRINT ("Removing tm_file");
			tm_workspace_remove_object (tm_file, TRUE);
		}
	}
	g_object_unref (G_OBJECT (data));
}

static gboolean
on_remove_project_tm_files (gpointer key, gpointer val, gpointer data)
{
	AnjutaSymbolView *sv;
	TMWorkObject *tm_file;

	sv = g_object_get_data (G_OBJECT (val), "symbol_view");
	tm_file = g_object_get_data (G_OBJECT (val), "tm_file");

	g_return_val_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv), FALSE);
	g_return_val_if_fail (tm_file != NULL, FALSE);
	
	if (tm_file &&
		TM_WORK_OBJECT (tm_file)->parent == sv->priv->tm_project)
	{
		DEBUG_PRINT ("Removing tm_file");
		if (sv->priv->file_symbol_model == val)
			sv->priv->file_symbol_model = NULL;
		return TRUE;
	}
	return FALSE;
}

#if 0
static gboolean
on_treeview_row_search (GtkTreeModel * model, gint column,
			const gchar * key, GtkTreeIter * iter, gpointer data)
{
	DEBUG_PRINT ("Search key == '%s'", key);
	return FALSE;
}
#endif

static AnjutaSymbolInfo *
sv_current_symbol (AnjutaSymbolView * sv)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	AnjutaSymbolInfo *info;

	g_return_val_if_fail (GTK_IS_TREE_VIEW (sv), FALSE);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sv));

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return NULL;

	gtk_tree_model_get (model, &iter, SVFILE_ENTRY_COLUMN, &info, -1);
	return info;
}

static gboolean
on_symbol_view_refresh_idle (gpointer data)
{
	AnjutaSymbolView *view;
	
	view = ANJUTA_SYMBOL_VIEW (data);
	
	/* If symbols need update */
	anjuta_symbol_view_refresh_tree (view);
	return FALSE;
}

static void
on_symbol_view_row_expanded (GtkTreeView * view,
							 GtkTreeIter * iter, GtkTreePath *iter_path,
							 AnjutaSymbolView *sv)
{
	// GdkPixbuf *pix;
	// gchar *full_path;
	GtkTreeIter child;
	GList *row_refs, *row_ref_node;
	GtkTreeRowReference *row_ref;
	GtkTreePath *path;
	TMSymbol *sym;
	/*
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (fv)->shell, NULL);
	anjuta_status_busy_push (status);
	*/
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	
	if (sv->priv->symbols_need_update)
	{
		DEBUG_PRINT ("Update required requested");
		g_idle_add (on_symbol_view_refresh_idle, sv);
		return;
	}

	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
	{
		/* Make sure the symbol children are not yet created */
		gtk_tree_model_get (GTK_TREE_MODEL (store), &child,
							SYMBOL_NODE, &sym, -1);
		/* Symbol has been created, no need to create them again */
		if (sym)
			return;
	}
	else 
	{
		/* Has no children */
		return;
	}
	
	/*
	 * Delete old children.
	 * Deleting multiple rows at one go is little tricky. We need to
	 * take row references before they are deleted
	 *
	 * Get row references
	 */
	row_refs = NULL;
	do
	{
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
		row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
		row_refs = g_list_prepend (row_refs, row_ref);
		gtk_tree_path_free (path);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &child));
	
	/* Update with new info */
	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
						SYMBOL_NODE, &sym, -1);
	if (sym)
	{
		anjuta_symbol_view_add_children (sv, sym, store, iter);
	}

	/* Delete the referenced rows */
	row_ref_node = row_refs;
	while (row_ref_node)
	{
		row_ref = row_ref_node->data;
		path = gtk_tree_row_reference_get_path (row_ref);
		g_assert (path != NULL);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &child, path);
		gtk_tree_store_remove (store, &child);
		gtk_tree_path_free (path);
		gtk_tree_row_reference_free (row_ref);
		row_ref_node = g_list_next (row_ref_node);
	}
	if (row_refs)
	{
		g_list_free (row_refs);
	}
	/* anjuta_status_busy_pop (status); */
}

static void
on_symbol_view_row_collapsed (GtkTreeView * view,
							  GtkTreeIter * iter, GtkTreePath * path)
{
}

static void
sv_create (AnjutaSymbolView * sv)
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	/* Tree and his model */
	store = gtk_tree_store_new (COLUMNS_NB,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								ANJUTA_TYPE_SYMBOL_INFO,
								G_TYPE_POINTER);

	gtk_tree_view_set_model (GTK_TREE_VIEW (sv), GTK_TREE_MODEL (store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (sv), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sv));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (sv), NAME_COLUMN);
	
	/* FIXME: Implement on_tree_view_row_search
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (sv),
					     on_treeview_row_search,
					     NULL, NULL);
	*/
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (sv), TRUE);

	g_signal_connect (G_OBJECT (sv), "row_expanded",
			  G_CALLBACK (on_symbol_view_row_expanded), sv);
	g_signal_connect (G_OBJECT (sv), "row_collapsed",
			  G_CALLBACK (on_symbol_view_row_collapsed), sv);
			  
	/* Tooltip signals */
	g_signal_connect (G_OBJECT (sv), "motion-notify-event",
					  G_CALLBACK (tooltip_motion_cb), sv);
	g_signal_connect (G_OBJECT (sv), "leave-notify-event",
					  G_CALLBACK (tooltip_leave_cb), sv);

	g_object_unref (G_OBJECT (store));

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Symbol"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
					    PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (sv), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (sv), column);
}

void
anjuta_symbol_view_clear (AnjutaSymbolView * sv)
{
	GtkTreeModel *model;
	AnjutaSymbolViewPriv* priv;
	
	priv = sv->priv;
	
	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));

	if (sv->priv->tm_project)
	{
		tm_project_save (TM_PROJECT (sv->priv->tm_project));
	}
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
	if (model)
	{
		/* clean out gtk_tree_store. We won't need it anymore */
		gtk_tree_store_clear (GTK_TREE_STORE (model));
	}
	if (sv->priv->symbols)
	{
		tm_symbol_tree_free (sv->priv->symbols);
		sv->priv->symbols = NULL;
		sv->priv->symbols_need_update = FALSE;
	}
	g_hash_table_foreach_remove (sv->priv->tm_files,
								 on_remove_project_tm_files,
								 sv);
	if (sv->priv->tm_project)
	{
		tm_project_free (sv->priv->tm_project);
		sv->priv->tm_project = NULL;
	}
}

static void
sv_assign_node_name (TMSymbol * sym, GString * s)
{
	gboolean append_var_type = TRUE;
	
	g_assert (sym && sym->tag && s);
	g_string_assign (s, sym->tag->name);

	switch (sym->tag->type)
	{
	case tm_tag_function_t:
	case tm_tag_method_t:
	case tm_tag_prototype_t:
		if (sym->tag->atts.entry.arglist)
			g_string_append (s, sym->tag->atts.entry.arglist);
		break;
	case tm_tag_macro_with_arg_t:
		if (sym->tag->atts.entry.arglist)
			g_string_append (s, sym->tag->atts.entry.arglist);
		append_var_type = FALSE;
		break;

	default:
		break;
	}
	if (append_var_type)
	{
		char *vt = sym->tag->atts.entry.type_ref[1];
		if (vt)
		{
			if((vt = strstr(vt, "_fake_")))
			{
				char backup = *vt;
				int i;
				*vt = '\0';

				g_string_append_printf (s, " [%s",
						sym->tag->atts.entry.type_ref[1]);
				i = sym->tag->atts.entry.pointerOrder;
				for (; i > 0; i--)
					g_string_append_printf (s, "*");
				g_string_append_printf (s, "]");

				*vt = backup;
			}
			else
			{
				int i;
				g_string_append_printf (s, " [%s",
						sym->tag->atts.entry.type_ref[1]);
				i = sym->tag->atts.entry.pointerOrder;
				for (; i > 0; i--)
					g_string_append_printf (s, "*");
				g_string_append_printf (s, "]");
			}
		}
	}
}

static void
mapping_function (GtkTreeView * treeview, GtkTreePath * path, gpointer data)
{
	gchar *str;
	GList *map = *((GList **) data);

	str = gtk_tree_path_to_string (path);
	map = g_list_append (map, str);
	*((GList **) data) = map;
};


TMSourceFile *
anjuta_symbol_view_get_tm_file (AnjutaSymbolView * sv, const gchar * uri)
{
	gchar *filename;
	TMSourceFile *tm_file;
	
	g_return_val_if_fail (uri != NULL, NULL);
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	
	tm_file = (TMSourceFile *)tm_workspace_find_object (TM_WORK_OBJECT
					 (sv->priv->tm_workspace), filename,
					 FALSE);
	
	g_free (filename);
	return tm_file;				
}



void
anjuta_symbol_view_add_source (AnjutaSymbolView * sv, const gchar *filename)
{
	if (!sv->priv->tm_project)
		return;
	tm_project_add_file (TM_PROJECT (sv->priv->tm_project), filename, TRUE);
	
	DEBUG_PRINT ("Project changed: Flagging refresh required");
	
	sv->priv->symbols_need_update = TRUE;
/*
	for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
	{
		te = TEXT_EDITOR (tmp->data);
		if (te && !te->tm_file && (0 == strcmp(te->full_filename, filename)))
			te->tm_file = tm_workspace_find_object(TM_WORK_OBJECT(app->tm_workspace)
			  , filename, FALSE);
	}
*/
/*	sv_populate (sv); */
}

void
anjuta_symbol_view_remove_source (AnjutaSymbolView *sv, const gchar *filename)
{
	TMWorkObject *source_file;
	
	if (!sv->priv->tm_project)
		return;

	source_file = tm_project_find_file (sv->priv->tm_project, filename, FALSE);
	if (source_file)
	{
		/* GList *node;
		TextEditor *te; */
		tm_project_remove_object (TM_PROJECT (sv->priv->tm_project),
								  source_file);
		
		DEBUG_PRINT ("Project changed: Flagging refresh required");
		
		sv->priv->symbols_need_update = TRUE;
/*		for (node = app->text_editor_list; node; node = g_list_next(node))
		{
			te = TEXT_EDITOR (node->data);
			if (te && (source_file == te->tm_file))
				te->tm_file = NULL;
		}
		sv_populate (sv);
*/
	}
/*	else
		g_warning ("Unable to find %s in project", full_fn);
*/
}

void anjuta_symbol_view_update_source_from_buffer (AnjutaSymbolView *sv, const gchar *uri,
													gchar* text_buffer, gint buffer_size) 
{
	TMWorkObject *tm_file;
	gchar *filename = NULL;

	GTimer *timer;
	gulong ms;

	g_return_if_fail(sv != NULL);
	if (uri == NULL || text_buffer == NULL)
		return;

	filename = gnome_vfs_get_local_path_from_uri (uri);
	
	if (sv->priv->tm_workspace == NULL ||
		sv->priv->tm_project == NULL ) {
		DEBUG_PRINT ("workspace or project are null");
		return;
	}
	
	tm_file =
		tm_workspace_find_object (TM_WORK_OBJECT
								  (sv->priv->tm_workspace),
								  filename, FALSE);
	if (tm_file == NULL) {
		DEBUG_PRINT ("tm_file is null");
		return;
	}	
	
	timer = g_timer_new ();
	tm_source_file_buffer_update (tm_file, (unsigned char *)text_buffer, buffer_size, TRUE);
	
	g_timer_stop (timer);
	g_timer_elapsed (timer, &ms);
	
	DEBUG_PRINT ("updating took %d microseconds", (unsigned int)ms );
	
	if (sv->priv->tm_project &&
		TM_PROJECT(sv->priv->tm_project)->work_object.tags_array)
	{
		DEBUG_PRINT ("total tags discovered AFTER buffer updating...: %d", 
				TM_PROJECT(sv->priv->tm_project)->work_object.tags_array->len);	
	}
}


static void
anjuta_symbol_view_add_children (AnjutaSymbolView *sv, TMSymbol *sym,
								 GtkTreeStore *store,
								 GtkTreeIter *iter)
{
	if (((iter == NULL) || (tm_tag_function_t != sym->tag->type)) &&
		(sym->info.children) && (sym->info.children->len > 0))
	{
		unsigned int j;
		SVNodeType type;
		AnjutaSymbolInfo *sfile;

		if (iter == NULL)
			DEBUG_PRINT ("Total nodes: %d", sym->info.children->len);
		
		for (j = 0; j < sym->info.children->len; j++)
		{
			TMSymbol *sym1 = TM_SYMBOL (sym->info.children->pdata[j]);
			GtkTreeIter sub_iter, child_iter;
			GString *s;
			
			// if (!sym1 || ! sym1->tag || ! sym1->tag->atts.entry.file)
			//	continue;
			
			if (!sym1 || ! sym1->tag)
				continue;
			
			type = anjuta_symbol_info_get_node_type (sym1, NULL);
			
			if (sv_none_t == type)
				continue;
			
			s = g_string_sized_new (MAX_STRING_LENGTH);
			sv_assign_node_name (sym1, s);
			
			if (sym && sym->tag && sym->tag->atts.entry.scope)
			{
				g_string_insert (s, 0, "::");
				g_string_insert (s, 0, sym->tag->atts.entry.scope);
			}
			sfile = anjuta_symbol_info_new (sym1, type);
			
			gtk_tree_store_append (store, &sub_iter, iter);
			gtk_tree_store_set (store, &sub_iter,
								PIXBUF_COLUMN,
							    anjuta_symbol_info_get_pixbuf (type),
								NAME_COLUMN, s->str,
								SVFILE_ENTRY_COLUMN, sfile,
								SYMBOL_NODE, sym1,
								-1);
			
			if (((tm_tag_function_t != sym1->tag->type)) &&
				(sym1->info.children) && (sym1->info.children->len > 0))
			{
				/* Append a dummy children node */
				gtk_tree_store_append (store, &child_iter, &sub_iter);
				gtk_tree_store_set (store, &child_iter,
									NAME_COLUMN, _("Loading..."),
									-1);
			}			
			anjuta_symbol_info_free (sfile);
			g_string_free (s, TRUE);
		}
	}
}

static void
anjuta_symbol_view_refresh_tree (AnjutaSymbolView *sv)
{
	GtkTreeStore *store;
	GList *selected_items = NULL;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (sv)));
	
	selected_items = anjuta_symbol_view_get_node_expansion_states (sv);
	gtk_tree_store_clear (store);

	/* Clear symbols */
	if (sv->priv->symbols)
	{
		tm_symbol_tree_free (sv->priv->symbols);
		sv->priv->symbols = NULL;
	}
	/* Destroy file symbol models that belong to the project */
	g_hash_table_foreach_remove (sv->priv->tm_files,
								 on_remove_project_tm_files,
								 sv);
	if (!(sv->priv->symbols =
		 tm_symbol_tree_new (sv->priv->tm_project->tags_array)))
		goto clean_leave;
	
	sv->priv->symbols_need_update = FALSE;
	
	DEBUG_PRINT ("Populating symbol view: Creating symbol view...");
	
	if (!sv->priv->symbols->info.children
	    || (0 == sv->priv->symbols->info.children->len))
	{
		tm_symbol_tree_free (sv->priv->symbols);
		sv->priv->symbols = NULL;
		goto clean_leave;
	}
	anjuta_symbol_view_add_children (sv, sv->priv->symbols, store, NULL);
	
	/* tm_symbol_tree_free (symbol_tree); */
	anjuta_symbol_view_set_node_expansion_states (sv, selected_items);
	
clean_leave:
	if (selected_items)
		anjuta_util_glist_strings_free (selected_items);
}

/*------------------------------------------------------------------------------
 * this function will add the symbol_tag entries on the GtkTreeStore
 */
void
anjuta_symbol_view_open (AnjutaSymbolView * sv, const gchar * root_dir)
{
	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (root_dir != NULL);
	
	DEBUG_PRINT ("Populating symbol view: Loading tag database...");

	/* make sure we clear anjuta_symbol_view from previous data */
	anjuta_symbol_view_clear (sv);
	
	sv->priv->tm_project = tm_project_new (root_dir, NULL, NULL, FALSE);
	
	DEBUG_PRINT ("Populating symbol view: Creating symbol tree...");
	
	if (sv->priv->tm_project &&
	    sv->priv->tm_project->tags_array &&
	    (sv->priv->tm_project->tags_array->len > 0))
	{
		anjuta_symbol_view_refresh_tree (sv);
	}
}

static void
anjuta_symbol_view_finalize (GObject * obj)
{
	AnjutaSymbolView *sv = ANJUTA_SYMBOL_VIEW (obj);
	DEBUG_PRINT ("Finalizing symbolview widget");
	
	anjuta_symbol_view_clear (sv);
	
	if (sv->priv->tooltip_timeout)
		g_source_remove (sv->priv->tooltip_timeout);
	sv->priv->tooltip_timeout = 0;
	
	g_hash_table_destroy (sv->priv->tm_files);
	tm_workspace_free ((gpointer) sv->priv->tm_workspace);
	g_free (sv->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_symbol_view_dispose (GObject * obj)
{
	AnjutaSymbolView *sv = ANJUTA_SYMBOL_VIEW (obj);
	
	/* All file symbol refs would be freed when the hash table is distroyed */
	sv->priv->file_symbol_model = NULL;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

/* Anjuta symbol view class */
static void
anjuta_symbol_view_instance_init (GObject * obj)
{
	AnjutaSymbolView *sv;
	gchar *system_tags_path;
	
	sv = ANJUTA_SYMBOL_VIEW (obj);
	sv->priv = g_new0 (AnjutaSymbolViewPriv, 1);
	sv->priv->file_symbol_model = NULL;
	sv->priv->symbols_need_update = FALSE;
	sv->priv->tm_workspace = tm_get_workspace ();
	sv->priv->tm_files = g_hash_table_new_full (g_str_hash, g_str_equal,
												g_free,
												destroy_tm_hash_value);
	
	system_tags_path = g_build_filename (g_get_home_dir(), ".anjuta",
										 "system-tags.cache", NULL);
	/* Load gloabal tags on gtk idle */
	if (!tm_workspace_load_global_tags (system_tags_path))
	{
		g_idle_add((GSourceFunc) symbol_browser_prefs_create_global_tags, 
			ANJUTA_PLUGIN_SYMBOL_BROWSER (obj));
	}

	/* let's create symbol_view tree and other gui stuff */
	sv_create (sv);
}

static void
anjuta_symbol_view_class_init (AnjutaSymbolViewClass * klass)
{
	AnjutaSymbolViewClass *svc;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	svc = ANJUTA_SYMBOL_VIEW_CLASS (klass);
	object_class->finalize = anjuta_symbol_view_finalize;
	object_class->dispose = anjuta_symbol_view_dispose;
}

GType
anjuta_symbol_view_get_type (void)
{
	static GType obj_type = 0;

	if (!obj_type)
	{
		static const GTypeInfo obj_info = {
			sizeof (AnjutaSymbolViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) anjuta_symbol_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,	/* class_data */
			sizeof (AnjutaSymbolViewClass),
			0,	/* n_preallocs */
			(GInstanceInitFunc) anjuta_symbol_view_instance_init,
			NULL	/* value_table */
		};
		obj_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
						   "AnjutaSymbolView",
						   &obj_info, 0);
	}
	return obj_type;
}

GtkWidget *
anjuta_symbol_view_new (void)
{
	return gtk_widget_new (ANJUTA_TYPE_SYMBOL_VIEW, NULL);
}

void
anjuta_symbol_view_update (AnjutaSymbolView * sv, GList *source_files)
{
	g_return_if_fail (sv->priv->tm_project != NULL);
	
	if (sv->priv->tm_project)
	{
		g_hash_table_foreach_remove (sv->priv->tm_files,
									 on_remove_project_tm_files,
									 sv);
		if (source_files)
			tm_project_sync (TM_PROJECT (sv->priv->tm_project), source_files);
		else
			tm_project_autoscan (TM_PROJECT (sv->priv->tm_project));
		tm_project_save (TM_PROJECT (sv->priv->tm_project));
		anjuta_symbol_view_refresh_tree (sv);
	}
}

void
anjuta_symbol_view_save (AnjutaSymbolView * sv)
{
	tm_project_save (TM_PROJECT (sv->priv->tm_project));
}

GList *
anjuta_symbol_view_get_node_expansion_states (AnjutaSymbolView * sv)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (sv),
					 mapping_function, &map);
	return map;
}

void
anjuta_symbol_view_set_node_expansion_states (AnjutaSymbolView * sv,
											  GList * expansion_states)
{
	/* Restore expanded nodes */
	if (expansion_states)
	{
		GtkTreePath *path;
		GtkTreeModel *model;
		GList *node;
		node = expansion_states;

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
		while (node)
		{
			path = gtk_tree_path_new_from_string (node->data);
			gtk_tree_view_expand_row (GTK_TREE_VIEW (sv), path,
						  FALSE);
			gtk_tree_path_free (path);
			node = g_list_next (node);
		}
	}
}

gchar *
anjuta_symbol_view_get_current_symbol (AnjutaSymbolView * sv)
{
	AnjutaSymbolInfo *info;
	gchar *sym_name;

	info = sv_current_symbol (sv);
	if (!info)
		return NULL;
	sym_name = g_strdup (info->sym_name);
	anjuta_symbol_info_free (info);
	return sym_name;
}

gboolean
anjuta_symbol_view_get_current_symbol_def (AnjutaSymbolView * sv,
										   gchar ** filename,
										   gint * line)
{
	AnjutaSymbolInfo *info;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (line != NULL, FALSE);

	info = sv_current_symbol (sv);
	if (!info)
		return FALSE;
	if (!info->def.name)
	{
		anjuta_symbol_info_free (info);
		return FALSE;
	}
	*filename = g_strdup (info->def.name);
	*line = info->def.line;
	anjuta_symbol_info_free (info);
	return TRUE;
}

gboolean
anjuta_symbol_view_get_current_symbol_decl (AnjutaSymbolView * sv,
											gchar ** filename,
											gint * line)
{
	AnjutaSymbolInfo *info;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (line != NULL, FALSE);

	info = sv_current_symbol (sv);
	if (!info)
		return FALSE;
	if (!info->decl.name)
	{
		anjuta_symbol_info_free (info);
		return FALSE;
	}
	*filename = g_strdup (info->decl.name);
	*line = info->decl.line;
	anjuta_symbol_info_free (info);
	return TRUE;
}

GtkTreeModel *
anjuta_symbol_view_get_file_symbol_model (AnjutaSymbolView * sv)
{
	return sv->priv->file_symbol_model;
}

static GtkTreeModel *
create_file_symbols_model (AnjutaSymbolView * sv, TMWorkObject * tm_file,
			   guint tag_types)
{
	GtkTreeStore *store;
	GtkTreeIter iter;

	store = gtk_tree_store_new (N_COLS, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT);


	g_return_val_if_fail (tm_file != NULL, NULL);

	if ((tm_file->tags_array) && (tm_file->tags_array->len > 0))
	{
		TMTag *tag;
		guint i;

		/* let's parse every tag in the array */
		for (i = 0; i < tm_file->tags_array->len; ++i)
		{
			gchar *tag_name;

			tag = TM_TAG (tm_file->tags_array->pdata[i]);
			if (tag == NULL)
				continue;
			if (tag->type & tag_types)
			{
				SVNodeType sv_type =
				    anjuta_symbol_info_get_node_type (NULL, tag);

				if ((NULL != tag->atts.entry.scope)
				    && isalpha (tag->atts.entry.scope[0]))
					tag_name =
						g_strdup_printf
						("%s::%s [%ld]",
						 tag->atts.entry.scope,
						 tag->name,
						 tag->atts.entry.line);
				else
					tag_name =
						g_strdup_printf ("%s [%ld]",
								 tag->name,
								 tag->atts.
								 entry.line);
				/*
				 * Appends a new row to tree_store. If parent is non-NULL, then it will 
				 * append the new row after the last child of parent, otherwise it will append 
				 * a row to the top level. iter will be changed to point to this new row. The 
				 * row will be empty after this function is called. To fill in values, you need 
				 * to call gtk_tree_store_set() or gtk_tree_store_set_value().
				 */
				gtk_tree_store_append (store, &iter, NULL);

				/* filling the row */
				gtk_tree_store_set (store, &iter,
						    COL_PIX,
							anjuta_symbol_info_get_pixbuf (sv_type),
						    COL_NAME, tag_name,
						    COL_LINE, tag->atts.entry.line, -1);
				g_free (tag_name);
			}
		}
	}
	return GTK_TREE_MODEL (store);
}

void
anjuta_symbol_view_workspace_add_file (AnjutaSymbolView * sv,
									   const gchar * file_uri)
{
	const gchar *uri;
	TMWorkObject *tm_file;
	GtkTreeModel *store = NULL;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (file_uri != NULL);

	if (strncmp (file_uri, "file://", 7) == 0)
		uri = &file_uri[7];
	else
		return;
	
	store = g_hash_table_lookup (sv->priv->tm_files, uri);
	if (!store)
	{
		DEBUG_PRINT ("Adding Symbol URI: %s", file_uri);
		tm_file =
			tm_workspace_find_object (TM_WORK_OBJECT
									  (sv->priv->tm_workspace),
									  uri, FALSE);
		if (!tm_file)
		{
			tm_file = tm_source_file_new (uri, TRUE);
			if (tm_file)
				tm_workspace_add_object (tm_file);
		}
		else
		{
			tm_source_file_update (TM_WORK_OBJECT (tm_file), TRUE,
								   FALSE, TRUE);
			if (sv->priv->tm_project &&
				TM_WORK_OBJECT (tm_file)->parent == sv->priv->tm_project)
			{
				DEBUG_PRINT ("Project changed: Flagging refresh required");
				sv->priv->symbols_need_update = TRUE;
			}
		}
		if (tm_file)
		{
			store = create_file_symbols_model (sv, tm_file,
											   tm_tag_max_t);
			g_object_set_data (G_OBJECT (store), "tm_file",
							   tm_file);
			g_object_set_data (G_OBJECT (store), "symbol_view",
							   sv);
			g_hash_table_insert (sv->priv->tm_files,
								 g_strdup (uri), store);
		}
	}
	sv->priv->file_symbol_model = store;
}

void
anjuta_symbol_view_workspace_remove_file (AnjutaSymbolView * sv,
										  const gchar * file_uri)
{
	const gchar *uri;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (file_uri != NULL);

	DEBUG_PRINT ("Removing Symbol URI: %s", file_uri);
	if (strncmp (file_uri, "file://", 7) == 0)
		uri = &file_uri[7];
	else
		uri = file_uri;
	
	if (g_hash_table_lookup (sv->priv->tm_files, uri))
		g_hash_table_remove (sv->priv->tm_files, uri);
}


// FIXME
void
anjuta_symbol_view_workspace_update_file (AnjutaSymbolView * sv,
										  const gchar * old_file_uri,
										  const gchar * new_file_uri)
{
	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (new_file_uri != NULL);
	if (old_file_uri)
		anjuta_symbol_view_workspace_remove_file (sv, old_file_uri);
	anjuta_symbol_view_workspace_add_file (sv, old_file_uri);
#if 0
	const gchar *uri;
	TMWorkObject *tm_file;
	GtkTreeModel *store = NULL;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (new_file_uri != NULL);
	g_return_if_fail (strncmp (new_file_uri, "file://", 7) == 0);

	if (old_file_uri)
	{
		/* Rename old uri to new one */
		gchar *orig_key;
		gboolean success;

		g_return_if_fail (strncmp (old_file_uri, "file://", 7) == 0);

		uri = &old_file_uri[7];
		success =
			g_hash_table_lookup_extended (sv->priv->tm_files, uri,
										  (gpointer *) & orig_key,
										  (gpointer *) & store);
		if (success)
		{
			if (strcmp (old_file_uri, new_file_uri) != 0)
			{
				DEBUG_PRINT ("Renaming Symbol URI: %s to %s",
					     old_file_uri, new_file_uri);
				g_hash_table_steal (sv->priv->tm_files, uri);
				g_free (orig_key);
				uri = &new_file_uri[7];
				g_hash_table_insert (sv->priv->tm_files,
						     g_strdup (uri), store);
			}
			/* Update tm_file */
			tm_file =
				g_object_get_data (G_OBJECT (store),
						   "tm_file");
			g_assert (tm_file != NULL);
			tm_source_file_update (TM_WORK_OBJECT (tm_file), TRUE,
					       FALSE, TRUE);
		}
		else
		{
			/* Old uri not found. Just add the new one. */
			anjuta_symbol_view_workspace_add_file (sv,
							       new_file_uri);
		}
	}
	else
	{
		/* No old uri to rename. Just add the new one. */
		anjuta_symbol_view_workspace_add_file (sv, new_file_uri);
	}
#endif
}

gint
anjuta_symbol_view_workspace_get_line (AnjutaSymbolView * sv,
									   GtkTreeIter * iter)
{
	g_return_val_if_fail (iter != NULL, -1);
	if (sv->priv->file_symbol_model)
	{
		gint line;
		gtk_tree_model_get (GTK_TREE_MODEL
				    (sv->priv->file_symbol_model), iter,
				    COL_LINE, &line, -1);
		return line;
	}
	return -1;
}

#define IS_DECLARATION(T) ((tm_tag_prototype_t == (T)) || (tm_tag_externvar_t == (T)) \
  || (tm_tag_typedef_t == (T)))

gboolean
anjuta_symbol_view_get_file_symbol (AnjutaSymbolView * sv,
									const gchar * symbol,
									gboolean prefer_definition,
									const gchar ** const filename,
									gint * line)
{
	TMWorkObject *tm_file;
	GPtrArray *tags;
	guint i;
	int cmp;
	TMTag *tag = NULL, *local_tag = NULL, *global_tag = NULL;
	TMTag *local_proto = NULL, *global_proto = NULL;

	g_return_val_if_fail (symbol != NULL, FALSE);

	/* Get the matching definition and declaration in the local file */
	if (sv->priv->file_symbol_model != NULL)
	{
		tm_file =
			g_object_get_data (G_OBJECT
					   (sv->priv->file_symbol_model),
					   "tm_file");
		if (tm_file && tm_file->tags_array
		    && tm_file->tags_array->len > 0)
		{
			for (i = 0; i < tm_file->tags_array->len; ++i)
			{
				tag = TM_TAG (tm_file->tags_array->pdata[i]);
				cmp = strcmp (symbol, tag->name);
				if (0 == cmp)
				{
					if (IS_DECLARATION (tag->type))
						local_proto = tag;
					else
						local_tag = tag;
				}
				else if (cmp < 0)
					break;
			}
		}
	}
	/* Get the matching definition and declaration in the workspace */
	if (!(((prefer_definition) && (local_tag)) ||
	      ((!prefer_definition) && (local_proto))))
	{
		tags = TM_WORK_OBJECT (tm_get_workspace ())->tags_array;
		if (tags && (tags->len > 0))
		{
			for (i = 0; i < tags->len; ++i)
			{
				tag = TM_TAG (tags->pdata[i]);
				if (tag->atts.entry.file)
				{
					cmp = strcmp (symbol, tag->name);
					if (cmp == 0)
					{
						if (IS_DECLARATION
						    (tag->type))
							global_proto = tag;
						else
							global_tag = tag;
					}
					else if (cmp < 0)
						break;
				}
			}
		}
	}
	if (prefer_definition)
	{
		if (local_tag)
			tag = local_tag;
		else if (global_tag)
			tag = global_tag;
		else if (local_proto)
			tag = local_proto;
		else
			tag = global_proto;
	}
	else
	{
		if (local_proto)
			tag = local_proto;
		else if (global_proto)
			tag = global_proto;
		else if (local_tag)
			tag = local_tag;
		else
			tag = global_tag;
	}

	if (tag)
	{
		*filename =
			g_strdup (tag->atts.entry.file->work_object.
				  file_name);
		*line = tag->atts.entry.line;
		return TRUE;
	}
	return FALSE;
}




/*
 * for a given expression "myvar->getX().toFloat"
 * the function should just return "myvar"
 * and move the **expr pointer after the ident
 * returns NULL, if no identifier is found, and then the value of expr is undefined
 */
static gchar* 
sv_scan_for_ident(const gchar **expr)
{
	static gchar ident[1024];
	/* number of identified characters in the ident string */
    int valid_chars = 0; 

	gchar c;
	while ((c = **expr) != '\0')
	{
		if(valid_chars == 0 && isspace(c))
		{
			(*expr)++;
			continue;
		}
		// XXX: Namespace support
		else if(isalpha(c) || c == '_') // ???: || c == ':')
		{
			ident[valid_chars] = c;
			if (++valid_chars == 1023)
			{
				ident[1023] = '\0';
				return ident;
			}
		}
		/* a digit may be part of an ident but not from the start */
		else if (isdigit(c))
		{
			if (valid_chars)
			{
				ident[valid_chars] = c;
				if (++valid_chars == 1023)
				{
					ident[1023] = '\0';
					return ident;
				}
			}
			else
				return NULL;

		}
		else
			break;

		(*expr)++;
	}

	if (valid_chars)
	{
		ident[valid_chars] = '\0';
		return ident;
	}
	else
		return NULL;
}


static
gchar* sv_extract_type_qualifier_from_expr (const gchar *string, const gchar *expr )
{
	/* check with a regular expression for the type */
	regex_t re;
	gint i;
	regmatch_t pm[8]; // 7 sub expression -> 8 matches
	memset (&pm, -1, sizeof(pm));
	/*
	 * this regexp catches things like:
	 * a) std::vector<char*> exp1[124] [12], *exp2, expr;
	 * b) QClass* expr1, expr2, expr;
	 * c) int a,b; char r[12] = "test, argument", q[2] = { 't', 'e' }, expr;
	 *
	 * it CAN be fooled, if you really want it, but it should
	 * work for 99% of all situations.
	 *
	 * QString
	 * 		var;
	 * in 2 lines does not work, because //-comments would often bring wrong results
	 */

	#define STRING      "\\\".*\\\""
	#define BRACKETEXPR "\\{.*\\}"
	#define IDENT       "[a-zA-Z_][a-zA-Z0-9_]*"
	#define WS          "[ \t\n]*"
	#define PTR         "[\\*&]?\\*?"
	#define INITIALIZER "=(" WS IDENT WS ")|=(" WS STRING WS ")|=(" WS BRACKETEXPR WS ")" WS
	#define ARRAY 		WS "\\[" WS "[0-9]*" WS "\\]" WS

	gchar *res = NULL;
	gchar pattern[512] =
		"(" IDENT "\\>)" 	// the 'std' in example a)
		"(::" IDENT ")*"	// ::vector
		"(" WS "<[^>;]*>)?"	// <char *>
		"(" WS PTR WS IDENT WS "(" ARRAY ")*" "(" INITIALIZER ")?," WS ")*" // other variables for the same ident (string i,j,k;)
		"[ \t\\*&]*";		// check again for pointer/reference type

	/* must add a 'termination' symbol to the regexp, otherwise
	 * 'exp' would match 'expr' */
	gchar regexp[512];
	snprintf (regexp, 512, "%s\\<%s\\>", pattern, expr);

	/* compile regular expression */
	gint error = regcomp (&re, regexp, REG_EXTENDED) ;
	if (error)
		g_warning ("regcomp failed");

	/* this call to regexec finds the first match on the line */
	error = regexec (&re, string, 8, &pm[0], 0) ;
	
	/* while matches found. We'll find the *last* occurrence of the expr. This will
	 * guarantee us that it is the right one we want to use. I.e.
	 *
	 * given:
	 *
	 *
	 * int func_1 (void) {
	 *
	 *   MyClass *klass;
	 *
	 *
	 *   for (int i=0; i < 20; i++) {
	 *     YourClass *klass;
	 *
	 *     klass->
	 *   }
	 *
	 *  / ... /
	 *
	 * Here the klass-> will be matched as YourClass [right], not the MyClass' ones [wrong]
	 * 
	 */
	while (error == 0) 
	{   
		/* subString found between pm.rm_so and pm.rm_eo */
		/* only include the ::vector part in the indentifier, if the second subpattern matches at all */
		gint len = (pm[2].rm_so != -1 ? pm[2].rm_eo : pm[1].rm_eo) - pm[1].rm_so;
		if (res)
			free (res);
		res = (gchar*) malloc (len + 1);
		if (!res)
		{
			regfree (&re);
			return NULL;
		}
		strncpy (res, string + pm[1].rm_so, len); 
		res[len] = '\0';
		
		string += pm[0].rm_eo;
		
		/* This call to regexec finds the next match */
		error = regexec (&re, string, 8, &pm[0], 0) ;
	}
	regfree(&re);

	if (!res)
		return NULL;
	
	/* if the proposed type is a keyword, we did something wrong, return NULL instead */	

	i=0;
	while (keywords[i] != NULL) {
		if (strcmp(res, keywords[i]) == 0) {
			return NULL;
		}
		i++;
	}
	
	return res;
}



/* Sample class:
 * class QWidget {
 * public:
 *    QWidget();
 *    [..]
 *    QRect getRect();
 *
 * };
 * 
 * for ident=getRect and klass="QWidget" return a tag of type QRect.
 * 
 * */
static TMTag* 
sv_get_type_of_token (const gchar* ident, const gchar* klass, const TMTag* local_scope_of_ident, 
					const TMTag* local_declaration_type)
{
	const GPtrArray *tags_array;
	TMTag *klass_tag = NULL;
	gint i;
	
	/* if we have a variable and already found a local definition, just return it */
	if (local_declaration_type != NULL && strlen(local_declaration_type->name)) {
		return (TMTag*)local_declaration_type;
	}

	/* if the identifier is this-> return the current class */
	if (!strcmp (ident, "this")) {
		const GPtrArray *tags_array;
		TMTag* scope_tag = NULL;
		gint i;
		
		scope_tag = (TMTag*)local_scope_of_ident;
		
		if (scope_tag == NULL || scope_tag->atts.entry.scope == NULL)
			return NULL;
		
		tags_array = tm_workspace_find (scope_tag->atts.entry.scope, 
				tm_tag_struct_t | tm_tag_typedef_t |tm_tag_union_t| tm_tag_class_t, 
				NULL, FALSE, TRUE);
		
		if (tags_array  != NULL) {
			for (i=0; i < tags_array ->len; i++) {
				TMTag *cur_tag;
			
				cur_tag = (TMTag*)g_ptr_array_index (tags_array , i);
				g_message ("found following %d array_tmp tag: %s", i, cur_tag ->name );
				if ( strcmp (cur_tag->name, scope_tag->atts.entry.scope) == 0) {
					scope_tag = cur_tag;
					break;
				}
			}
		}
				
		if (scope_tag)
			DEBUG_PRINT ("scope_tag [class] %s", scope_tag->name);
		return scope_tag;;
	}
	
	/* ok, search the klass for the symbols in workspace */
	if (klass == NULL || (strcmp("", klass) == 0))
		return NULL;
	
	tags_array = tm_workspace_find_scope_members (NULL, klass,
												  TRUE);
	
	if (tags_array != NULL)
	{
		for (i=0; i < tags_array->len; i++) {
			TMTag *tmp_tag;
		
			tmp_tag = (TMTag*)g_ptr_array_index (tags_array, i);
		
			/* store the klass_tag. It will be useful later */
			if (strcmp (tmp_tag->name, klass) == 0) {
				klass_tag = tmp_tag;
			}
		
			if (strcmp (tmp_tag->name, ident) == 0 ) {
				return tmp_tag;
			}
		}
	}	
		
	/* checking for inherited members. We'll reach this point only if the right tag 
     * hasn't been detected yet.
	 */
	GPtrArray *inherited_tags_array;
	
	inherited_tags_array = anjuta_symbol_view_get_completable_members (klass_tag, TRUE);
	
	if (inherited_tags_array != NULL)
	{
		for (i=0; i < inherited_tags_array->len; i++) {
			TMTag *tmp_tag;
		
			tmp_tag = (TMTag*)g_ptr_array_index (inherited_tags_array, i);
			DEBUG_PRINT ("parsing inherited member as %s ", tmp_tag->name);
			if ( (strcmp (tmp_tag->name, ident) == 0) ) {
				const GPtrArray *inherit_array;
		
				TMTagAttrType attrs[] = {
					tm_tag_attr_type_t
				};
			
				inherit_array = tm_workspace_find (tmp_tag->atts.entry.type_ref[1], 
						tm_tag_class_t, attrs, FALSE, TRUE);
					
				if (inherit_array == NULL)
					continue;
								
				gint j;
				for (j=0; j < inherit_array->len; j++) {
					TMTag *cur_tag = (TMTag*)g_ptr_array_index (inherit_array, j);
				
					if (strcmp(tmp_tag->atts.entry.type_ref[1], cur_tag->name) == 0 )
						return cur_tag;
				}
			
				return tmp_tag;
			}
		}
		g_ptr_array_free (inherited_tags_array, TRUE);
	}

	DEBUG_PRINT ("returning NULL from get_type_of_token. ident %s", ident);
	return NULL;
}


/*----------------------------------------------------------------------------
 * Lists the completable members given a klass_tag. I.e.: it returns all current
 * class members [private/protected/public] [or in case of a struct all its members]
 * and then it goes up for parents retrieving public/protected members.
 */

GPtrArray*
anjuta_symbol_view_get_completable_members (TMTag* klass_tag, gboolean include_parents) {
	gchar *symbol_name;

	if (klass_tag == NULL)
		return NULL;
	
	
	symbol_name = klass_tag->atts.entry.type_ref[1];
	if (symbol_name == NULL)
		symbol_name = klass_tag->name;
	
	DEBUG_PRINT ("get_completable_members --> scope of tag name %s is %s", klass_tag->name, klass_tag->atts.entry.scope);
	// FIXME: comment this
	tm_tag_print (klass_tag, stdout);

	switch (klass_tag->type) {
		case tm_tag_struct_t: 
		case tm_tag_typedef_t: 
		case tm_tag_union_t: 
		{
			const GPtrArray* tags_array;
			GPtrArray *completable_array;
			gint i;
			/* we should list all members of our struct/typedef/union...*/
			tags_array = tm_workspace_find_scope_members (NULL, symbol_name, TRUE);
			if (tags_array == NULL) {
				DEBUG_PRINT ("returning NULL from struct-completable");
				return NULL;
			}

			DEBUG_PRINT ("returning NULL from struct-completable. Tags array len %d", tags_array->len);
			
			completable_array = g_ptr_array_new ();
			for (i=0; i < tags_array->len; i++)
				g_ptr_array_add (completable_array, g_ptr_array_index (tags_array, i));
				
			return completable_array;
		}
		case tm_tag_class_t: 
		case tm_tag_member_t:
		case tm_tag_method_t:
		case tm_tag_prototype_t:
		{
			
			/* list all the public/protected members of super classes, if it's a class
	  		obviously and if there are some super classes */
			
			GPtrArray *completable_array;			
			const GPtrArray *tags_array;
			gchar* tmp_str;
			gint i;

			DEBUG_PRINT ("completable: klass_tag is");
			tm_tag_print (klass_tag, stdout);
			
			// FIXME: this part needs an improvement!
			// It needs a search for symbols under a particular namespace. Infact right now 
			// it cannot detect if a MyPrettyClass is defined under namespace1:: or under
			// namespace2::. It just looks for the symbols of klass/struct/.. name
			if (klass_tag->atts.entry.scope != NULL && 
					(tmp_str=strstr(klass_tag->atts.entry.scope, "")) != NULL) {
				DEBUG_PRINT ("scope with ::. FIXME");
			}					
			
			tags_array = tm_workspace_find_scope_members (NULL, symbol_name, TRUE);
			if (tags_array == NULL) {
				DEBUG_PRINT ("returning NULL from class&c-completable with symbol name %s [scope of klass_tag: %s]",
							symbol_name, klass_tag->atts.entry.scope);
				return NULL;
			}

			completable_array = g_ptr_array_new ();
			/* we *must* duplicate the contents of the tags_array coz it will be 
			 * overwritten the next call to tm_workspace_find_scope_members ().
			 * Just the tag-pointers will be saved, not the entire struct behind them.
			 */
			for (i=0; i < tags_array->len; i++)
				g_ptr_array_add (completable_array, g_ptr_array_index (tags_array, i));

			/* should we go for parents [include_parents]? or is it a struct [inheritance == NULL]? */
			if (!include_parents || klass_tag->atts.entry.inheritance == NULL)
				return completable_array;
				
			DEBUG_PRINT ("parents from klass_tag [name] %s: %s", symbol_name, klass_tag->atts.entry.inheritance);
			
			const GPtrArray *parents_array = tm_workspace_get_parents (symbol_name);
			if (parents_array == NULL) {
				DEBUG_PRINT ("returning tags_array coz parents_array is null");
				return completable_array;
			}
			
			for (i=0; i < parents_array->len; i++) {
				gint j;
				const GPtrArray *tmp_parents_array;
				TMTag* cur_parent_tag = g_ptr_array_index (parents_array, i);

				/* we don't need the members for the symbols_name's class. We 
				   already have them */
				if ( (strcmp(cur_parent_tag->name, symbol_name) == 0) )
					continue;
				
				if ( (tmp_parents_array = tm_workspace_find_scope_members (NULL, 
						cur_parent_tag->name, TRUE)) == NULL) {
					continue;		
				}
					
				gint length = tmp_parents_array->len;
				for (j = 0; j < length; j++) {
					TMTag *test_tag;
					test_tag = (TMTag*)g_ptr_array_index (tmp_parents_array, j);
					
					if (test_tag->atts.entry.access == TAG_ACCESS_PUBLIC ||
						test_tag->atts.entry.access == TAG_ACCESS_PROTECTED ||
						test_tag->atts.entry.access == TAG_ACCESS_FRIEND) {

						g_ptr_array_add (completable_array, test_tag);	
					}					
				}	
			}			
			
			return completable_array;
		}

		case tm_tag_namespace_t:
		{
			const GPtrArray *namespace_classes;
			GPtrArray *completable_array = NULL;
			gint i;
			
			DEBUG_PRINT ("we got a namespace!, %s", klass_tag->name);

			namespace_classes = tm_workspace_find_namespace_members (NULL, klass_tag->name, TRUE);

			/* we *must* duplicate the contents of the tags_array coz it will be 
			 * overwritten the next call to tm_workspace_find_scope_members ().
			 * Just the tag-pointers will be saved, not the entire struct behind them.
			 */
			completable_array = g_ptr_array_new ();				
			for (i=0; i < namespace_classes->len; i++) {
				TMTag *cur_tag;
			
				cur_tag = (TMTag*)g_ptr_array_index (namespace_classes, i);
				
//				tm_tag_print (cur_tag, stdout);
				g_ptr_array_add (completable_array, cur_tag);
			}
		
			return completable_array;
		}	
		default: 
			return NULL;
	}
	return NULL;
}

/* local_declaration_type_str must be NOT NULL.
 *
 */
static TMTag * 
sv_get_local_declaration_type (const gchar *local_declaration_type_str) {

	const GPtrArray * loc_decl_tags_array;
	TMTag* local_declaration_type = NULL;
	
	g_return_val_if_fail (local_declaration_type_str != NULL, NULL);
	
	/* local_declaration_type_str  != NULL */
	gchar *occurrence;

	DEBUG_PRINT ("local_declaration_type_str is %s", local_declaration_type_str);
	DEBUG_PRINT ("checking for :: [namespace scoping] inside it...");
			
	if ( (occurrence=strstr(local_declaration_type_str, "::")) == NULL ) {
		gint i;
		DEBUG_PRINT ("No, it hasn't..");
			
		loc_decl_tags_array = tm_workspace_find (local_declaration_type_str, 
			tm_tag_struct_t | tm_tag_typedef_t |tm_tag_union_t| tm_tag_class_t, 
			NULL, FALSE, TRUE);

		/* We should have a look at the tags_array for the correct tag. */
		if (loc_decl_tags_array != NULL) {
			for (i=0; i < loc_decl_tags_array->len; i++) {
				TMTag *cur_tag;

				cur_tag = (TMTag*)g_ptr_array_index (loc_decl_tags_array, i);
				if ( strcmp(cur_tag->name, local_declaration_type_str) == 0) {
					local_declaration_type = cur_tag;
					break;
				}
			}
			return local_declaration_type;
		}
		else 
			return NULL;
	}
	else {			/* namespace string parsing */
		/* This is the case of
		 * void foo_func() {
		 *
		 *  namespace1::namespace2::MyClass *myclass;	
	 	 *  
	 	 *  myclass->
	 	 *
	 	 * }
		 *
		 * we should parse the "local_declaration_type_str" and retrieve the 
		 * TMTag of MyClass under namespace1::namespace2.
		 *
		 */
	
		gchar **splitted_str;
		const GPtrArray *tmp_tags_array;
		gint indx;

		DEBUG_PRINT ("Yes, it has!");
		
		splitted_str = g_strsplit (local_declaration_type_str, "::", -1);

		DEBUG_PRINT ("Check for splitted_str[0] existence into workspace...");
		tmp_tags_array =  tm_workspace_find (splitted_str[0], 
									tm_tag_struct_t | tm_tag_union_t |
									tm_tag_typedef_t | tm_tag_class_t |
									tm_tag_macro_t | tm_tag_macro_with_arg_t |
									tm_tag_namespace_t, NULL, FALSE, TRUE);
		
		
		if (tmp_tags_array == NULL) {
			DEBUG_PRINT ("tmp_tags_array is NULL");
			g_strfreev (splitted_str);
			return NULL;
		}

		
		/* ok, we can start from index 1 to check the correctness of expression */
		for (indx=1; splitted_str[indx] != NULL; indx++) {
			gint i;
			gboolean found_end_node;
			TMTag *tmp_tag = NULL;
				
			DEBUG_PRINT ("===string %d is %s====", indx, splitted_str[indx]);
				
			/* check the availability of namespace2 under the members of namespace1 
			 * and so on 
			 */
			tmp_tags_array = tm_workspace_find_namespace_members (NULL, splitted_str[indx-1], TRUE);

			if (tmp_tags_array == NULL) {
				local_declaration_type = NULL;
				break;
			}


			for (i=0; i < tmp_tags_array->len; i++) {
				TMTag *cur_tag;
	
				cur_tag = (TMTag*)g_ptr_array_index (tmp_tags_array, i);
				DEBUG_PRINT ("cur_tag scanned to check scoping %d out of %d --> %s", 
							 i, tmp_tags_array->len, cur_tag->name );
				if ( strcmp(cur_tag->name, splitted_str[indx]) == 0) {
					/* ok we found our tag */
					tmp_tag = cur_tag;
					break;
				}
			}
			
			found_end_node = FALSE;
			if (tmp_tag == NULL)
				break;
			
			switch (tmp_tag->type) {
				case tm_tag_class_t:
				case tm_tag_enum_t:
				case tm_tag_struct_t:
				case tm_tag_typedef_t:
				case tm_tag_union_t:
					/* are we at the end of the splitted_str[] parsing? Or
				     * have we just found a class/struct/etc but there's something more
					 * left in the stack? 
				     */
					if (splitted_str[indx +1] == NULL) {
						found_end_node = TRUE;
						break;
					}
					else {
						continue;	
					}
						
				case tm_tag_namespace_t:
				default:
					continue;
			}
			
			if (found_end_node == TRUE) {
				/* set the local declaration type*/
				local_declaration_type = tmp_tag;
				break;
			}
		}	
			
			
		g_strfreev (splitted_str);
		return local_declaration_type;
	}
	
	
	/* never reached */
	return local_declaration_type;

}

TMTag* anjuta_symbol_view_get_type_of_expression(AnjutaSymbolView * sv,
		const gchar* expr, int expr_len, const TMTag *func_scope_tag, gint *access_method)
{
	gchar *ident = NULL;
	*access_method = COMPLETION_ACCESS_NONE;
	
	/* very basic stack implementation to keep track of tokens */
	const int max_items = 30;
	const gchar* stack[max_items];
	int num_stack = 0;

	memset (stack, 0, sizeof(stack));
									
	/* skip nested brackets */
	int brackets = 0, square_brackets = 0;

	/* if the current position is within an identifier */
	gboolean in_ident = FALSE; 
	
	/* if we extract the next string which looks like an identifier - only after: . -> and ( */			
	gboolean extract_ident = FALSE; 

	if (strlen(expr) < 1)
		return FALSE;

	if (!expr_len)
		return FALSE;

	g_return_val_if_fail (func_scope_tag != NULL, NULL);
	
	/* check the access method */
	if (*access_method == COMPLETION_ACCESS_NONE) {
		switch (expr[expr_len-1]) {
			case '.':
				*access_method = COMPLETION_ACCESS_DIRECT;
				DEBUG_PRINT ("access_method = COMPLETION_ACCESS_DIRECT;");
				break;
			case '>':
				if (expr[expr_len-2]	== '-') {
					*access_method = COMPLETION_ACCESS_POINTER;
					DEBUG_PRINT ("access_method = COMPLETION_ACCESS_POINTER;");
				}
				break;
			case ':':
				if (expr[expr_len-2]	== ':') {
					*access_method = COMPLETION_ACCESS_STATIC;
					DEBUG_PRINT ("access_method = COMPLETION_ACCESS_STATIC;");
				}
				break;				
			default:
				*access_method = COMPLETION_ACCESS_NONE;
		}		
	}

	const gchar *start = expr + expr_len;
	while (--start >= expr )
	{
		/* skip brackets */
		if (brackets > 0 || square_brackets > 0)
		{
			if (*start == '(')
				--brackets;
			else if (*start == ')')
				++brackets;
			else if (*start == '[')
				--square_brackets;
			else if (*start == ']')
				++square_brackets;
			continue;
		}
		/* identifier */
		if (isdigit(*start))
			in_ident = FALSE;
		else if (isalpha(*start) || *start == '_')
		{
			in_ident = TRUE;
		}
		else
		{
			switch (*start)
			{
				/* skip whitespace */
				case ' ':
				case '\t':
					if (in_ident)
						goto extract;
					else
					{
						in_ident = FALSE;
						continue;
					}

				/* continue searching to the left, if we
				 * have a . or -> accessor */
				case '.':
					if (in_ident && extract_ident)
					{
						const gchar *ident = start+1;
						if (num_stack < max_items) {
							stack[num_stack++] = ident;
						}
						else
							return FALSE;
					}
					in_ident = FALSE;
					extract_ident = TRUE;
					continue;
				case '>': /* pointer access */
					if ((*start == '>' && (start-1 >= expr && (start-1)[0] == '-')))
					{
						if (in_ident && extract_ident)
						{
							const gchar *ident = start+1;
							if (num_stack < max_items) {
								stack[num_stack++] = ident;
							}
							else
								return FALSE;
						}
						in_ident = FALSE;
						extract_ident = TRUE;
						--start;
						continue;
					}
					else
					{
						start++;
						goto extract;
					}				
				case ':': /* static access */
					if ((*start == ':' && (start-1 >= expr && (start-1)[0] == ':')))
					{
						if (in_ident && extract_ident)
						{
							const gchar *ident = start+1;
							if (num_stack < max_items) {
								stack[num_stack++] = ident;
							}
							else
								return FALSE;
						}
						in_ident = FALSE;
						extract_ident = TRUE;
						--start;
						continue;
					}
					else
					{
						start++;
						goto extract;
					}
				case '(': /* start of a function */
					if (extract_ident)
					{
						start++;
						goto extract;
					}
					else
					{
						extract_ident = TRUE;
						in_ident = FALSE;
						break;
					}

				case ')':
					if (in_ident) /* probably a cast - (const char*)str */
					{
						start++;
						goto extract;
					}
					brackets++;
					break;

				case ']':
					if (in_ident)
					{
						start++;
						goto extract;
					}						
					square_brackets++;
					break;

				default:
					start++;
					goto extract;
			}
		}
	}

extract:
	/* ident is the leftmost token, stack[*] the ones to the right
	 * Example: str.left(2,5).toUpper()
	 *          ^^^ ^^^^      ^^^^^^^
	 *        ident stack[1]  stack[0]
	 */

	ident = sv_scan_for_ident (&start);
	if (!ident)
		return FALSE;

	DEBUG_PRINT("ident found: %s", ident);

	TMTag *tag_type = NULL, *old_type = NULL;
		
	/* for static access, parsing is done, just return the identifier.
	 * If this isn't the case we should process something like
	 * my_class_instance->getValue()->...->lastFunc()->
	 * and try to get the correct type of lastFunc token, so that we can have 
	 * a completable list.
	 */
	if (*access_method != COMPLETION_ACCESS_STATIC) {
		gchar *local_declaration_type_str;
		const TMTag* local_scope_of_ident;
		TMTag* local_declaration_type = NULL;
				
		local_scope_of_ident = func_scope_tag;
		/* in a context like:
		 * void foo_func() {
		 *
		 *  MyClass *myclass;
		 *  
		 *  myclass->
		 *
		 * }
		 *
		 * calling the following function should return the strings "MyClass".
		 * FIXME: there would need a better regex which returns also the pointer
		 * order of myclass, in this case should be "MyClass *", or better 1.
		 *
		 */
		local_declaration_type_str = sv_extract_type_qualifier_from_expr (expr, ident);

		/* check between the scope and parents members if "ident" is present */
		if (local_declaration_type_str  == NULL) {

			DEBUG_PRINT ("local_declaration_type_string is NULL, checking between its members "
						 "if it's a class");
			
			local_declaration_type = sv_get_type_of_token (ident, local_scope_of_ident->atts.entry.scope,
											local_scope_of_ident, NULL);
			if (local_declaration_type) {
				DEBUG_PRINT ("found local_declaration_type->name %s",local_declaration_type->name);
			}
		}
		else {
			local_declaration_type = sv_get_local_declaration_type (local_declaration_type_str);
		}
	
				
		if (local_declaration_type == NULL) {
			DEBUG_PRINT ("warning: local_declaration_type is NULL");
		}
		else {
			DEBUG_PRINT ("local_declaration_type detected:");
			tm_tag_print (local_declaration_type, stdout);
		}
						
		if (local_scope_of_ident != NULL && local_declaration_type != NULL ) {
			DEBUG_PRINT ("found local_scope_of_ident->name  %s and local_declaration_type->name %s", 
					local_scope_of_ident->name , local_declaration_type->name);
		}


		/* if we have the start of a function/method, don't return the type
		 * of this function, but class, which it is member of */
		tag_type = sv_get_type_of_token (ident, NULL, local_scope_of_ident, local_declaration_type);
		
		if (tag_type!=NULL)
			DEBUG_PRINT ("type of token: %s : %s, var_type %s\n", ident, 
						 tag_type->name, tag_type->atts.entry.type_ref[1]);
		else
			DEBUG_PRINT  ("WARNING: tag_type is null. cannot determine ident");


		while (tag_type && tag_type->name && num_stack > 0) {
			ident = sv_scan_for_ident (&stack[--num_stack]);
			DEBUG_PRINT ("!!scanning for ident %s\n", ident );
			
			old_type = tag_type;
		
			/* Example: 
			 * String str;
			 * str->left(param1)
			 *
			 * We already parsed the "str" token. Now it's left() turn.
			 * If we don't have the local_declaration_type nor the local_scope_of_ident let's find
			 * the type of the member. Obviously we should also check the parent 
			 * classes if we don't find it in the old_type->name class.
			 */
			
			if (old_type->atts.entry.type_ref[1] == NULL )
				tag_type = sv_get_type_of_token (ident, old_type->name, 
												 local_scope_of_ident, NULL);
			else
				tag_type = sv_get_type_of_token (ident, old_type->atts.entry.type_ref[1], 
												 local_scope_of_ident, NULL);

			if (tag_type != NULL) {
				DEBUG_PRINT ("tag_type of token: %s : [pointer order] %d\n", 
							 ident, tag_type->atts.entry.pointerOrder);
				DEBUG_PRINT ("we have setted a completion of %s", 
					*access_method == COMPLETION_ACCESS_DIRECT ? "DIRECT ACCESS" : "POINTER ACCESS");
				tm_tag_print (tag_type, stdout);
				
				if (tag_type->atts.entry.pointerOrder >= 1) {
					/* we shouldn't permit a direct access on tags which are on
					 * pointerOrder >=1. This means we can't complete with "."
					 * on MyClass* variables */
					if (*access_method == COMPLETION_ACCESS_DIRECT) {
						DEBUG_PRINT ("completion not permitted. Pointer Order is wrong");
						tag_type = NULL;
					}
				}
				else if (tag_type->atts.entry.pointerOrder == 0) {
					if (*access_method == COMPLETION_ACCESS_POINTER) {
						DEBUG_PRINT ("completion not permitted. Pointer Order is wrong");
						tag_type = NULL;
					}
				}
			}
		}
	}
	else { /* static member */
		/* just return the ident's tag */
		const GPtrArray * tags_array = NULL;
		gint i;
		
		DEBUG_PRINT ("Gonna parsing static member section: ident %s", ident );
		
		if (ident != NULL) {
			tags_array = tm_workspace_find (ident, 
				tm_tag_struct_t | tm_tag_union_t |
				tm_tag_typedef_t | tm_tag_class_t |
				tm_tag_macro_t | tm_tag_macro_with_arg_t |
				tm_tag_namespace_t, NULL, FALSE, TRUE);
		}
				
		if (tags_array == NULL) {
			DEBUG_PRINT ("tags_array is NULL");
			return NULL;
		}



				
		/* check if stack has members or not. For an expression like 
		 * Gnome::Glade::Xml:: we have a stack of
		 * stack[0] = Xml::
		 * stack[1] = Glade::Xml::
		 */
		
		//*/ DEBUG REMOVE ME
		for (i=0; i < num_stack; i++ ) {
			DEBUG_PRINT ("we have a stack[%d] = %s", i, stack[i]);
		}
		//*/
		

		// probably an useless loop
		for (i=0; i < tags_array->len; i++) {
			TMTag *cur_tag;

			cur_tag = (TMTag*)g_ptr_array_index (tags_array, i);
			DEBUG_PRINT ("cur_tag scanned %d out of %d --> %s", i, tags_array->len, cur_tag->name );
			if ( strcmp(cur_tag->name, ident) == 0) {
				/* ok we found our tag */
				tag_type = cur_tag;
				break;
			}
		}

		
		/* recursive expression parsing to check its correctness */
		if (tag_type != NULL && num_stack > 0) {
			gint indx;
			
			for (indx=0; indx < num_stack; indx++) {
				switch (tag_type->type) {
					case tm_tag_namespace_t:
					{
						const GPtrArray * tags_array = NULL;
						
						/* get next ident in stack */
						gchar * ident_to_check;
						gchar **new_idents;
					
						new_idents = g_strsplit (stack[num_stack - indx -1], ":", 0);
	
						ident_to_check = new_idents[0];
						if (ident_to_check == NULL) {
							g_strfreev (new_idents);
							break;
						}
					
						/* is our ident_to_check included into tag_type? 
						 * Or better, in our example, is Glade included into Gnome:: ?
						 */
						tags_array = tm_workspace_find_namespace_members (NULL, 
																		  tag_type->name, 
																		  TRUE);
						if (tags_array == NULL) {
							g_strfreev (new_idents);
							break;
						}
					
						gint i;
						for (i=0; i < tags_array->len; i++) {
							TMTag *cur_tag;
	
							cur_tag = (TMTag*)g_ptr_array_index (tags_array, i);
							DEBUG_PRINT ("cur_tag scanned to check correctness %d out of %d --> %s", 
										 i, tags_array->len, cur_tag->name );
							if ( strcmp(cur_tag->name, ident_to_check) == 0) {
								/* ok we found our tag */
								tag_type = cur_tag;
								break;
							}
						}
					
					 	g_strfreev (new_idents);
						break;
					}
					default:
					break;
				}	
			}
		}
	}

	return tag_type;
}





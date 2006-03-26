/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_view.c
 * Copyright (C) 2004 Naba Kumar
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
#include <libgnomeui/gnome-stock-icons.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-macros.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>
#include <tm_tagmanager.h>
#include "an_symbol_view.h"
#include "an_symbol_info.h"

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

static gboolean
on_treeview_row_search (GtkTreeModel * model, gint column,
			const gchar * key, GtkTreeIter * iter, gpointer data)
{
	DEBUG_PRINT ("Search key == '%s'", key);
	return FALSE;
}

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
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (sv),
					     on_treeview_row_search,
					     NULL, NULL);
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
		char *vt = sym->tag->atts.entry.var_type;
		if (vt)
		{
			if((vt = strstr(vt, "_fake_")))
			{
				char backup = *vt;
				int i;
				*vt = '\0';

				g_string_append_printf (s, " [%s",
						sym->tag->atts.entry.var_type);
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
						sym->tag->atts.entry.var_type);
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

#if 0
TMFile *
sv_get_tm_file (AnjutaSymbolView * sv, const gchar * filename)
{
	return tm_workspace_find_object (TM_WORK_OBJECT
					 (sv->priv->tm_workspace), filename,
					 TRUE);
}

#endif

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
		te = (TextEditor *) tmp->data;
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
			te = (TextEditor *) node->data;
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

enum
{
	COL_PIX, COL_NAME, COL_LINE, N_COLS
};

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
		g_warning ("Unable to load global tag file");

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

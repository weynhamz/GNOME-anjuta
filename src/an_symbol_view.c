#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomeui/gnome-stock-icons.h>

#include "anjuta.h"
#include "resources.h"
#include "mainmenu_callbacks.h"
#include "pixmaps.h"

#include "an_symbol_view.h"

#define MAX_STRING_LENGTH 256

typedef enum
{
	sv_none_t,
	sv_class_t,
	sv_struct_t,
	sv_function_t,
	sv_variable_t,
	sv_macro_t,
	sv_private_func_t,
	sv_private_var_t,
	sv_protected_func_t,
	sv_protected_var_t,
	sv_public_func_t,
	sv_public_var_t,
	sv_cfolder_t,
	sv_ofolder_t,
	sv_max_t
} SVNodeType;

typedef enum
{
	sv_root_none_t,
	sv_root_class_t,
	sv_root_struct_t,
	sv_root_function_t,
	sv_root_variable_t,
	sv_root_macro_t,
	sv_root_max_t
} SVRootType;

enum {
	PIXBUF_COLUMN,
	NAME_COLUMN,
	SVFILE_ENTRY_COLUMN,
	COLUMNS_NB
};

static char *sv_root_names[] = {
	N_("Others"), N_("Classes"), N_("Structs"), N_("Functions"),
	N_("Variables"), N_("Macros"), NULL
};

static AnSymbolView *sv = NULL;
static GdkPixbuf **sv_pixbufs = NULL;

static SymbolFileInfo *symbol_file_info_new(TMSymbol *sym)
{
	SymbolFileInfo *sfile = g_new0(SymbolFileInfo, 1);
	if (sym && sym->tag && sym->tag->atts.entry.file)
	{
		sfile->sym_name = g_strdup(sym->tag->name);
		sfile->def.name = g_strdup(sym->tag->atts.entry.file->work_object.file_name);
		sfile->def.line = sym->tag->atts.entry.line;
		if ((tm_tag_function_t == sym->tag->type) && sym->info.equiv)
		{
			sfile->decl.name = g_strdup(sym->info.equiv->atts.entry.file->work_object.file_name);
			sfile->decl.line = sym->info.equiv->atts.entry.line;
		}
	}
	return sfile;
}

static SymbolFileInfo *symbol_file_info_dup(SymbolFileInfo *from)
{
	if (NULL != from)
	{
		SymbolFileInfo *to = g_new0(SymbolFileInfo, 1);
		if (from->sym_name)
			to->sym_name = g_strdup(from->sym_name);
		if (from->def.name)
		{
			to->def.name = g_strdup(from->def.name);
			to->def.line = from->def.line;
		}
		if (from->decl.name)
		{
			to->decl.name = g_strdup(from->decl.name);
			to->decl.line = from->decl.line;
		}
		return to;
	}
	else
		return NULL;
}

static void symbol_file_info_free(SymbolFileInfo *sfile)
{
	if (sfile)
	{
		if (sfile->sym_name)
			g_free(sfile->sym_name);
		if (sfile->def.name)
			g_free(sfile->def.name);
		if (sfile->decl.name)
			g_free(sfile->decl.name);
		g_free(sfile);
	}
}

static SVNodeType
sv_get_node_type (TMSymbol *sym)
{
	SVNodeType type;
	char access;

	if (!sym || !sym->tag || (tm_tag_file_t == sym->tag->type))
		return sv_none_t;
	access = sym->tag->atts.entry.access;
	switch (sym->tag->type)
	{
		case tm_tag_class_t:
			type = sv_class_t;
			break;
		case tm_tag_struct_t:
			type = sv_struct_t;
			break;
		case tm_tag_function_t:
		case tm_tag_prototype_t:
			if ((sym->info.equiv) && (TAG_ACCESS_UNKNOWN == access))
				access = sym->info.equiv->atts.entry.access;
			switch (access)
			{
				case TAG_ACCESS_PRIVATE:
					type = sv_private_func_t;
					break;
				case TAG_ACCESS_PROTECTED:
					type = sv_protected_func_t;
					break;
				case TAG_ACCESS_PUBLIC:
					type = sv_public_func_t;
					break;
				default:
					type = sv_function_t;
					break;
			}
			break;
		case tm_tag_member_t:
			switch (access)
			{
				case TAG_ACCESS_PRIVATE:
					type = sv_private_var_t;
					break;
				case TAG_ACCESS_PROTECTED:
					type = sv_protected_var_t;
					break;
				case TAG_ACCESS_PUBLIC:
					type = sv_public_var_t;
					break;
				default:
					type = sv_variable_t;
					break;
			}
			break;
		case tm_tag_externvar_t:
		case tm_tag_variable_t:
			type = sv_variable_t;
			break;
		case tm_tag_macro_t:
		case tm_tag_macro_with_arg_t:
			type = sv_macro_t;
			break;
		default:
			type = sv_none_t;
			break;
	}
	return type;
}

static SVRootType
sv_get_root_type (SVNodeType type)
{
	if (!sv || (sv_none_t == type))
		return sv_root_none_t;
	switch (type)
	{
		case sv_class_t:
			return sv_root_class_t;
		case sv_struct_t:
			return sv_root_struct_t;
		case sv_function_t:
			return sv_root_function_t;
		case sv_variable_t:
			return sv_root_variable_t;
		case sv_macro_t:
			return sv_root_macro_t;
		default:
			return sv_root_none_t;
	}
}

#define CREATE_SV_ICON(N, F) \
	pix_file = anjuta_res_get_pixmap_file (F); \
	sv_pixbufs[(N)] = gdk_pixbuf_new_from_file (pix_file, NULL); \
	g_free (pix_file);

static void
sv_load_pixbufs ()
{
	gchar *pix_file;

	if (sv_pixbufs)
		return ;

	g_return_if_fail (sv != NULL && sv->win);

	sv_pixbufs = g_new (GdkPixbuf *, sv_max_t + 1);

	CREATE_SV_ICON (sv_none_t, ANJUTA_PIXMAP_SV_UNKNOWN);
	CREATE_SV_ICON (sv_class_t, ANJUTA_PIXMAP_SV_CLASS);
	CREATE_SV_ICON (sv_struct_t, ANJUTA_PIXMAP_SV_STRUCT);
	CREATE_SV_ICON (sv_function_t, ANJUTA_PIXMAP_SV_FUNCTION);
	CREATE_SV_ICON (sv_variable_t, ANJUTA_PIXMAP_SV_VARIABLE);
	CREATE_SV_ICON (sv_macro_t, ANJUTA_PIXMAP_SV_MACRO);
	CREATE_SV_ICON (sv_private_func_t, ANJUTA_PIXMAP_SV_PRIVATE_FUN);
	CREATE_SV_ICON (sv_private_var_t, ANJUTA_PIXMAP_SV_PRIVATE_VAR);
	CREATE_SV_ICON (sv_protected_func_t, ANJUTA_PIXMAP_SV_PROTECTED_FUN);
	CREATE_SV_ICON (sv_protected_var_t, ANJUTA_PIXMAP_SV_PROTECTED_VAR);
	CREATE_SV_ICON (sv_public_func_t, ANJUTA_PIXMAP_SV_PUBLIC_FUN);
	CREATE_SV_ICON (sv_public_var_t, ANJUTA_PIXMAP_SV_PUBLIC_VAR);
	CREATE_SV_ICON (sv_cfolder_t, ANJUTA_PIXMAP_CLOSED_FOLDER);
	CREATE_SV_ICON (sv_ofolder_t, ANJUTA_PIXMAP_OPEN_FOLDER);

	sv_pixbufs[sv_max_t] = NULL;
}

typedef enum
{
	GOTO_DEFINITION,
	GOTO_DECLARATION,
	SEARCH,
	REFRESH,
	MENU_MAX
} SVSignal;

static void
sv_context_handler (GtkMenuItem *item,
		    gpointer user_data)
{
	SVSignal signal = (SVSignal) user_data;
	switch (signal)
	{
		case GOTO_DEFINITION:
			if (sv->sinfo && sv->sinfo->def.name)
				anjuta_goto_file_line_mark(sv->sinfo->def.name
				  , sv->sinfo->def.line, TRUE);
			break;
		case GOTO_DECLARATION:
			if (sv->sinfo && sv->sinfo->decl.name)
				anjuta_goto_file_line_mark(sv->sinfo->decl.name
				  , sv->sinfo->decl.line, TRUE);
			break;
		case SEARCH:
			if (sv->sinfo && sv->sinfo->sym_name)
				anjuta_search_sources_for_symbol(sv->sinfo->sym_name);
			break;
		case REFRESH:
			sv_populate(TRUE);
			break;
		default:
			break;
	}
}

static GnomeUIInfo an_symbol_view_menu_uiinfo[] = {
	{/* 0 */
	 GNOME_APP_UI_ITEM, N_("Goto Definition"),
	 NULL,
	 sv_context_handler, (gpointer) GOTO_DEFINITION, NULL,
	 PIX_FILE(TAG),
	 0, 0, NULL}
	,
	{/* 1 */
	 GNOME_APP_UI_ITEM, N_("Goto Declaration"),
	 NULL,
	 sv_context_handler, (gpointer) GOTO_DECLARATION, NULL,
	 PIX_FILE(TAG),
	 0, 0, NULL}
	,
	{/* 2 */
	 GNOME_APP_UI_ITEM, N_("Find Usage"),
	 NULL,
	 sv_context_handler, (gpointer) SEARCH, NULL,
	 PIX_STOCK(GTK_STOCK_FIND),
	 0, 0, NULL}
	,
	{/* 3 */
	 GNOME_APP_UI_ITEM, N_("Refresh"),
	 NULL,
	 sv_context_handler, (gpointer) REFRESH, NULL,
	 PIX_STOCK(GTK_STOCK_REFRESH),
	 0, 0, NULL}
	,	GNOMEUIINFO_SEPARATOR, /*4*/
	{/* 5 */
	 GNOME_APP_UI_TOGGLEITEM, N_("Docked"),
	 N_("Dock/Undock the Project Window"),
	 on_project_dock_undock1_activate, NULL, NULL,
	 PIX_FILE(DOCK),
	 0, 0, NULL},
	GNOMEUIINFO_END /* 6 */
};

static void
sv_create_context_menu ()
{
	int i;

	sv->menu.top = gtk_menu_new();
	gtk_widget_ref(sv->menu.top);
	gnome_app_fill_menu (GTK_MENU_SHELL (sv->menu.top),
	  an_symbol_view_menu_uiinfo, NULL, FALSE, 0);
	for (i=0; i < sizeof(an_symbol_view_menu_uiinfo)/sizeof(an_symbol_view_menu_uiinfo[0]); ++i)
		gtk_widget_ref(an_symbol_view_menu_uiinfo[i].widget);
	sv->menu.goto_decl = an_symbol_view_menu_uiinfo[0].widget;
	sv->menu.goto_def = an_symbol_view_menu_uiinfo[1].widget;
	sv->menu.find = an_symbol_view_menu_uiinfo[2].widget;
	sv->menu.refresh = an_symbol_view_menu_uiinfo[3].widget;
	sv->menu.docked = an_symbol_view_menu_uiinfo[5].widget;
	gtk_widget_show_all(sv->menu.top);
}

static void
mapping_function (GtkTreeView *treeview, GtkTreePath *path, gpointer data)
{
	gchar *str;
	GList *map = * ((GList **) data);
	
	str = gtk_tree_path_to_string (path);
	map = g_list_append (map, str);
	* ((GList **) data) = map;
};

static GList *
sv_map_tree_view_state (GtkTreeView *treeview)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (treeview, mapping_function, &map);
	return map;
}

static gboolean
on_treeview_row_search (GtkTreeModel *model, gint column,
						const gchar *key, GtkTreeIter *iter, gpointer data)
{
	g_message ("Search key == '%s'", key);
	return FALSE;
}

static void
on_treeview_row_activated (GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	SymbolFileInfo *info;

	g_return_if_fail (GTK_IS_TREE_VIEW (view));
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, SVFILE_ENTRY_COLUMN, &info, -1);
	if (sv->sinfo)
		symbol_file_info_free(sv->sinfo);
	sv->sinfo = symbol_file_info_dup(info);
	if (sv->sinfo && sv->sinfo->def.name)
			anjuta_goto_file_line_mark (sv->sinfo->def.name,
									    sv->sinfo->def.line,
									    TRUE);
}

static gboolean
on_treeview_event (GtkWidget *widget,
				   GdkEvent  *event,
				   gpointer   user_data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	SymbolFileInfo *info;
	
	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter) || !event)
		return FALSE;

	gtk_tree_model_get (model, &iter, SVFILE_ENTRY_COLUMN, &info, -1);
	
	if (sv->sinfo)
		symbol_file_info_free(sv->sinfo);
	sv->sinfo = symbol_file_info_dup(info);

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;

		if (e->button == 3) {
			gboolean has_sinfo = (NULL != sv->sinfo);

			/* Popup project menu */
			GTK_CHECK_MENU_ITEM (sv->menu.docked)->active = app->project_dbase->is_docked;

			gtk_widget_set_sensitive (sv->menu.goto_decl, has_sinfo);
			gtk_widget_set_sensitive (sv->menu.goto_def, has_sinfo);
			gtk_widget_set_sensitive (sv->menu.find, has_sinfo);

			gtk_menu_popup (GTK_MENU(sv->menu.top), NULL, NULL,
					NULL, NULL, e->button, e->time);

			return TRUE;
		}
	} else if (event->type == GDK_KEY_PRESS) {
		GtkTreePath *path;
		GdkEventKey *e = (GdkEventKey *) event;

		switch (e->keyval) {
			case GDK_Return:
				if (!gtk_tree_model_iter_has_child (model, &iter))
				{
					anjuta_goto_file_line_mark (sv->sinfo->def.name,
								    sv->sinfo->def.line,
								    TRUE);
					return TRUE;
				}
			case GDK_Left:
				if (gtk_tree_model_iter_has_child (model, &iter))
				{
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_collapse_row (GTK_TREE_VIEW (sv->tree),
												path);
					gtk_tree_path_free (path);
					return TRUE;
				}
			case GDK_Right:
				if (gtk_tree_model_iter_has_child (model, &iter))
				{
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_expand_row (GTK_TREE_VIEW (sv->tree),
											  path, FALSE);
					gtk_tree_path_free (path);
					return TRUE;
				}
			default:
				return FALSE;
		}
	}

	return FALSE;
}

static void
sv_disconnect ()
{
	g_return_if_fail (sv != NULL);

	g_signal_handlers_block_by_func (sv->tree, G_CALLBACK (on_treeview_event), NULL);
}

static void
sv_connect ()
{
	g_return_if_fail (sv != NULL);

	g_signal_handlers_unblock_by_func (sv->tree, G_CALLBACK (on_treeview_event), NULL);
}

static void
on_symbol_model_row_deleted (GtkTreeModel *model,
			     GtkTreePath  *path)
{
	GtkTreeIter iter;
	SymbolFileInfo *sfile;

	if (gtk_tree_model_get_iter (model, &iter, path))
	{
		gtk_tree_model_get (model, &iter,
					SVFILE_ENTRY_COLUMN, &sfile,
					-1);
		symbol_file_info_free (sfile);
	}
}

static void
on_symbol_view_row_expanded (GtkTreeView *view,
			     GtkTreeIter *iter,
			     GtkTreePath *path)
{
	GdkPixbuf *pixbuf;
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
						PIXBUF_COLUMN, &pixbuf, -1);
	if (pixbuf == sv_pixbufs[sv_cfolder_t])
	{
		gtk_tree_store_set (store, iter,
					PIXBUF_COLUMN, sv_pixbufs[sv_ofolder_t],
					-1);
	}
}

static void
on_symbol_view_row_collapsed (GtkTreeView *view,
			      GtkTreeIter *iter,
			      GtkTreePath *path)
{
	GdkPixbuf *pixbuf;
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
						PIXBUF_COLUMN, &pixbuf, -1);
	if (pixbuf == sv_pixbufs[sv_ofolder_t])
	{
		gtk_tree_store_set (store, iter,
					PIXBUF_COLUMN, sv_pixbufs[sv_cfolder_t],
					-1);
	}
}

static void
sv_create ()
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	sv = g_new0 (AnSymbolView, 1);

	/* Scrolled window */
	sv->win = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sv->win),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (sv->win);

	/* Tree and his model */
	store = gtk_tree_store_new (COLUMNS_NB,
				    GDK_TYPE_PIXBUF,
					G_TYPE_STRING,
				    G_TYPE_POINTER);
	g_signal_connect (store, "row_deleted", G_CALLBACK (on_symbol_model_row_deleted), NULL);

	sv->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sv->tree), TRUE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (sv->win), sv->tree);

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (sv->tree), NAME_COLUMN);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (sv->tree),
										 on_treeview_row_search,
										 NULL, NULL);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (sv->tree), TRUE);

	g_signal_connect (sv->tree, "row_expanded",
					  G_CALLBACK (on_symbol_view_row_expanded), NULL);
	g_signal_connect (sv->tree, "row_collapsed",
					  G_CALLBACK (on_symbol_view_row_collapsed), NULL);
	g_signal_connect (sv->tree, "event",
					  G_CALLBACK (on_treeview_event), NULL);
	g_signal_connect (sv->tree, "row_activated",
					  G_CALLBACK (on_treeview_row_activated), NULL);
	gtk_widget_show (sv->tree);

	g_object_unref (G_OBJECT (store));

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Symbol"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (sv->tree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (sv->tree), column);

	/* The remaining bits */
	if (!sv_pixbufs)
		sv_load_pixbufs ();

	sv_create_context_menu ();
	gtk_widget_ref (sv->tree);
	gtk_widget_ref (sv->win);
}

void
sv_clear ()
{
	GtkTreeModel *model;

	g_return_if_fail (sv != NULL && sv->tree);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv->tree));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

void sv_hide(void)
{
	g_return_if_fail(sv && sv->tree);
	gtk_widget_hide(sv->tree);
}

void sv_show(void)
{
	g_return_if_fail(sv && sv->tree);
	gtk_widget_show(sv->tree);
}

static void sv_assign_node_name(TMSymbol *sym, GString *s)
{
	g_assert (sym && sym->tag && s);

	g_string_assign (s, sym->tag->name);

	switch (sym->tag->type) {
		case tm_tag_function_t:
		case tm_tag_prototype_t:
		case tm_tag_macro_with_arg_t:
			if (sym->tag->atts.entry.arglist)
				g_string_append (s, sym->tag->atts.entry.arglist);
			break;

		default:
			if (sym->tag->atts.entry.var_type)
				g_string_sprintfa (s, " [%s]", sym->tag->atts.entry.var_type);
			break;
	}
}

AnSymbolView *
sv_populate (gboolean full)
{
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreeIter root[sv_root_max_t + 1];
	SymbolFileInfo *sfile;
	TMSymbol *symbol_tree = NULL;
	static gboolean busy = FALSE;
	GString *s;
	char *arr[1];
	SVRootType root_type;
	int i;
	GList *selected_items = NULL;

#ifdef DEBUG
	g_message("Populating symbol view..");
#endif

	if (!sv)
		sv_create();

	if (busy)
		return sv;
	else
		busy = TRUE;

	sv_disconnect ();
	selected_items = sv_map_tree_view_state (GTK_TREE_VIEW (sv->tree));
	sv_clear ();

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (sv->tree)));

	for (root_type = sv_root_none_t; root_type < sv_root_max_t; ++root_type) {
		gtk_tree_store_append (store, &root[root_type], NULL);
		gtk_tree_store_set (store, &root[root_type],
				    PIXBUF_COLUMN, sv_pixbufs[sv_cfolder_t],
				    NAME_COLUMN, sv_root_names[root_type],
				    -1);
	}

	if (!full)
		goto clean_leave;

	if (!app || !app->project_dbase || !app->project_dbase->tm_project ||
	    !app->project_dbase->tm_project->tags_array ||
	    (0 == app->project_dbase->tm_project->tags_array->len))
		goto clean_leave;

	if (!(symbol_tree = tm_symbol_tree_new (app->project_dbase->tm_project->tags_array)))
		goto clean_leave;

	if (!symbol_tree->info.children || (0 == symbol_tree->info.children->len)) {
		tm_symbol_tree_free(symbol_tree);

		goto clean_leave;
	}

	s = g_string_sized_new (MAX_STRING_LENGTH);

	for (i = 0; i < symbol_tree->info.children->len; ++i) {
		TMSymbol *sym = TM_SYMBOL(symbol_tree->info.children->pdata[i]);
		SVNodeType type;
		GtkTreeIter parent_item;
		gboolean has_children;
		
		if (!sym || ! sym->tag || !sym->tag->atts.entry.file)
			continue ;

		type = sv_get_node_type (sym);
		root_type = sv_get_root_type (type);
		parent_item = root[root_type];

		if (root_type == sv_root_max_t)
			continue ;

		sv_assign_node_name (sym, s);

		if (sym->tag->atts.entry.scope) {
			g_string_insert(s, 0,"::");
			g_string_insert(s, 0, sym->tag->atts.entry.scope);
		}
		
		arr[0] = s->str;
		if ((tm_tag_function_t != sym->tag->type) &&
			(sym->info.children) && (sym->info.children->len > 0))
			has_children = TRUE;
		else
			has_children = FALSE;
		sfile = symbol_file_info_new (sym);
		gtk_tree_store_append (store, &iter, &parent_item);
		gtk_tree_store_set (store, &iter,
				    PIXBUF_COLUMN, sv_pixbufs[type],
				    NAME_COLUMN, s->str,
				    SVFILE_ENTRY_COLUMN, sfile,
				    -1);

		while (gtk_events_pending())
			gtk_main_iteration();
		
		if (has_children)
		{
			int j;

			for (j = 0; j < sym->info.children->len; ++j) {
				TMSymbol *sym1 = TM_SYMBOL (sym->info.children->pdata[j]);
				GtkTreeIter sub_iter;
				if (!sym1 || ! sym1->tag || ! sym1->tag->atts.entry.file)
					continue;

				type = sv_get_node_type (sym1);

				if (sv_none_t == type)
					continue;

				sv_assign_node_name (sym1, s);

				arr[0] = s->str;
				sfile = symbol_file_info_new (sym1);
				gtk_tree_store_append (store, &sub_iter, &iter);
				gtk_tree_store_set (store, &sub_iter,
						    PIXBUF_COLUMN, sv_pixbufs[type],
						    NAME_COLUMN, s->str,
						    SVFILE_ENTRY_COLUMN, sfile,
						    -1);
				while (gtk_events_pending())
					gtk_main_iteration();
			}
		}
	}
	g_string_free (s, TRUE);
	tm_symbol_tree_free (symbol_tree);
	
	if (selected_items)
	{
		GtkTreePath *path;
		GtkTreeModel *model;
		GList *node;
		node = selected_items;
		
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv->tree));
		while (node)
		{
			path = gtk_tree_path_new_from_string (node->data);
			gtk_tree_view_expand_row (GTK_TREE_VIEW (sv->tree), path, FALSE);
			gtk_tree_path_free (path);
			node = g_list_next (node);
		}
	}

clean_leave:
	if (selected_items)
		glist_strings_free (selected_items);
	sv_connect ();
	busy = FALSE;
	return sv;
}

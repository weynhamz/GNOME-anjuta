#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "anjuta.h"
#include "resources.h"
#include "pixmaps.h"

#include "an_symbol_view.h"

#define MAX_STRING_LENGTH 256

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

static char *sv_root_names[] = {
	N_("Others"), N_("Classes"), N_("Structs"), N_("Functions"),
	N_("Variables"), N_("Macros"), NULL
};

static GdkPixmap **sv_icons = NULL;
static GdkBitmap **sv_bitmaps = NULL;
static AnSymbolView *sv = NULL;

static SVNodeType sv_get_node_type(TMSymbol *sym)
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

static SVRootType sv_get_root_type(SVNodeType type)
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

#define CREATE_SV_ICON(N, F) sv_icons[(N)] = gdk_pixmap_colormap_create_from_xpm(\
  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[(N)],\
  NULL, anjuta_res_get_pixmap_file(F));

static void sv_load_pixmaps(void)
{
	if (sv_icons)
		return;
	sv_icons = g_new(GdkPixmap *, (sv_max_t+1));
	sv_bitmaps = g_new(GdkBitmap *, (sv_max_t+1));

	if (!sv || !sv->win)
		return;
	CREATE_SV_ICON(sv_none_t, ANJUTA_PIXMAP_SV_UNKNOWN);
	CREATE_SV_ICON(sv_class_t, ANJUTA_PIXMAP_SV_CLASS);
	CREATE_SV_ICON(sv_struct_t, ANJUTA_PIXMAP_SV_STRUCT);
	CREATE_SV_ICON(sv_function_t, ANJUTA_PIXMAP_SV_FUNCTION);
	CREATE_SV_ICON(sv_variable_t, ANJUTA_PIXMAP_SV_VARIABLE);
	CREATE_SV_ICON(sv_macro_t, ANJUTA_PIXMAP_SV_MACRO);
	CREATE_SV_ICON(sv_private_func_t, ANJUTA_PIXMAP_SV_PRIVATE_FUN);
	CREATE_SV_ICON(sv_private_var_t, ANJUTA_PIXMAP_SV_PRIVATE_VAR);
	CREATE_SV_ICON(sv_protected_func_t, ANJUTA_PIXMAP_SV_PROTECTED_FUN);
	CREATE_SV_ICON(sv_protected_var_t, ANJUTA_PIXMAP_SV_PROTECTED_VAR);
	CREATE_SV_ICON(sv_public_func_t, ANJUTA_PIXMAP_SV_PUBLIC_FUN);
	CREATE_SV_ICON(sv_public_var_t, ANJUTA_PIXMAP_SV_PUBLIC_VAR);
	CREATE_SV_ICON(sv_cfolder_t, ANJUTA_PIXMAP_CLOSED_FOLDER);
	CREATE_SV_ICON(sv_ofolder_t, ANJUTA_PIXMAP_OPEN_FOLDER);
	sv_icons[sv_max_t] = NULL;
	sv_bitmaps[sv_max_t] = NULL;
}

typedef enum
{
	GOTO_DEFINITION,
	GOTO_DECLARATION,
	SEARCH,
	REFRESH,
	MENU_MAX
} SVSignal;

static void sv_context_handler(GtkMenuItem *item, gpointer user_data)
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
			sv_populate();
			break;
		default:
			break;
	}
}

static void sv_create_context_menu(void)
{
	GtkWidget *item;
	sv->menu = gtk_menu_new();
	gtk_widget_ref(sv->menu);
	gtk_widget_show(sv->menu);
	item = gtk_menu_item_new_with_label(_("Goto Definition"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(sv_context_handler)
	  , (gpointer) GOTO_DEFINITION);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(sv->menu), item);
	item = gtk_menu_item_new_with_label(_("Goto Declaration"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(sv_context_handler)
	  , (gpointer) GOTO_DECLARATION);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(sv->menu), item);
	item = gtk_menu_item_new_with_label(_("Find Usage"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(sv_context_handler)
	  , (gpointer) SEARCH);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(sv->menu), item);
	item = gtk_menu_item_new_with_label(_("Refresh Tree"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(sv_context_handler)
	  , (gpointer) REFRESH);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(sv->menu), item);
}

static gboolean
sv_on_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	if (!event)
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		if (((GdkEventButton *) event)->button != 3)
			return FALSE;
		
		/* Popup project menu */
		gtk_menu_popup(GTK_MENU(sv->menu), NULL, NULL, NULL, NULL
		  , ((GdkEventButton *) event)->button, ((GdkEventButton *) event)->time);
		
		return TRUE;
	} else if (event->type == GDK_KEY_PRESS){
		gint row;
		GtkCTree *tree;
		GtkCTreeNode *node;
		tree = GTK_CTREE(widget);
		row = tree->clist.focus_row;
		node = gtk_ctree_node_nth(tree,row);
		
		if (!node)
			return FALSE;
			
		switch(((GdkEventKey *)event)->keyval) {
			case GDK_space:
			case GDK_Return:
				if(GTK_CTREE_ROW(node)->is_leaf) {
					
					sv->sinfo = (SymbolFileInfo *) gtk_ctree_node_get_row_data(
					  GTK_CTREE(sv->tree), GTK_CTREE_NODE(node));
					
					if (sv->sinfo && sv->sinfo->def.name)
							anjuta_goto_file_line_mark(sv->sinfo->def.name
							  , sv->sinfo->def.line, TRUE);
				}
				break;
			case GDK_Left:
				gtk_ctree_collapse(tree, node);
				break;
			case GDK_Right:
				gtk_ctree_expand(tree, node);
				break;
			default:
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

static void sv_on_select_row(GtkCList *clist, gint row, gint column
  , GdkEventButton *event, gpointer user_data)
{
	GtkCTreeNode *node = gtk_ctree_node_nth(GTK_CTREE(sv->tree), row);
	if (!node || !event || row < 0 || column < 0)
		return;
	sv->sinfo = (SymbolFileInfo *) gtk_ctree_node_get_row_data(
	  GTK_CTREE(sv->tree), GTK_CTREE_NODE(node));
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
	{
		if (sv->sinfo && sv->sinfo->def.name)
			anjuta_goto_file_line_mark(sv->sinfo->def.name
			  , sv->sinfo->def.line, TRUE);
	}
}

static void sv_disconnect(void)
{
	g_return_if_fail(sv);
	gtk_signal_disconnect_by_func(GTK_OBJECT(sv->tree)
	  , GTK_SIGNAL_FUNC(sv_on_select_row), NULL);
	gtk_signal_disconnect_by_func(GTK_OBJECT(sv->tree)
	  , GTK_SIGNAL_FUNC(sv_on_event), NULL);
}

static void sv_connect(void)
{
	g_return_if_fail(sv);
	gtk_signal_connect(GTK_OBJECT(sv->tree), "select_row"
	  , GTK_SIGNAL_FUNC(sv_on_select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(sv->tree), "event"
	  , GTK_SIGNAL_FUNC(sv_on_event), NULL);
}

static void sv_create(void)
{
	sv = g_new0(AnSymbolView, 1);
	sv->win=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_ref(sv->win);
	gtk_widget_show(sv->win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sv->win),
	  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	sv->tree=gtk_ctree_new(1,0);
	gtk_ctree_set_line_style (GTK_CTREE(sv->tree), GTK_CTREE_LINES_DOTTED);
	gtk_ctree_set_expander_style (GTK_CTREE(sv->tree), GTK_CTREE_EXPANDER_SQUARE);
	gtk_widget_ref(sv->tree);
	gtk_widget_show(sv->tree);
	gtk_clist_set_column_auto_resize(GTK_CLIST(sv->tree), 0, TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(sv->tree)
	  ,GTK_SELECTION_BROWSE);
	gtk_container_add (GTK_CONTAINER(sv->win), sv->tree);
	if (!sv_icons)
		sv_load_pixmaps();
	sv_create_context_menu();
	sv_connect();
}

static void sv_freeze(void)
{
	g_return_if_fail(sv);
	gtk_clist_freeze(GTK_CLIST(sv->tree));
}

static void sv_thaw(void)
{
	g_return_if_fail(sv);
	gtk_clist_thaw(GTK_CLIST(sv->tree));
}

void sv_clear(void)
{
	g_return_if_fail(sv && sv->tree);
	gtk_clist_clear(GTK_CLIST(sv->tree));
}

static void sv_assign_node_name(TMSymbol *sym, GString *s)
{
	g_assert(sym && sym->tag && s);
	g_string_assign(s, sym->tag->name);
	switch(sym->tag->type)
	{
		case tm_tag_function_t:
		case tm_tag_prototype_t:
		case tm_tag_macro_with_arg_t:
			if (sym->tag->atts.entry.arglist)
				g_string_append(s, sym->tag->atts.entry.arglist);
			break;
		default:
			if (sym->tag->atts.entry.var_type)
				g_string_sprintfa(s, " [%s]"
				  , sym->tag->atts.entry.var_type);
			break;
	}
}

#define CREATE_SV_NODE(T) {\
	arr[0] = sv_root_names[(T)];\
	root[(T)] = gtk_ctree_insert_node(GTK_CTREE(sv->tree),\
	  NULL, NULL, arr, 5, sv_icons[sv_cfolder_t],\
	  sv_bitmaps[sv_cfolder_t], sv_icons[sv_ofolder_t],\
	  sv_bitmaps[sv_ofolder_t], FALSE, FALSE);}

AnSymbolView *sv_populate(void)
{
	GString *s;
	char *arr[1];
	TMSymbol *sym, *sym1, *symbol_tree;
	SVNodeType type;
	SymbolFileInfo *sfile;
	GtkCTreeNode *root[sv_root_max_t+1];
	GtkCTreeNode *item, *parent_item, *subitem;
	SVRootType root_type;
	gboolean has_children;
	int i, j;

#ifdef DEBUG
	g_message("Populating symbol view..");
#endif

	if (!sv)
		sv_create();

	sv_disconnect();
	sv_freeze();
	sv_clear();

	if (!app || !app->project_dbase || !app->project_dbase->tm_project ||
	  !app->project_dbase->tm_project->tags_array ||
	  (0 == app->project_dbase->tm_project->tags_array->len))
		goto clean_leave;

	if (!(symbol_tree = tm_symbol_tree_new(app->project_dbase->tm_project->tags_array)))
		goto clean_leave;

	for (root_type = sv_root_none_t; root_type < sv_root_max_t; ++root_type)
		CREATE_SV_NODE(root_type)
	root[sv_root_max_t] = NULL;

	if (!symbol_tree->info.children || (0 == symbol_tree->info.children->len))
	{
		tm_symbol_tree_free(symbol_tree);
		goto clean_leave;
	}

	s = g_string_sized_new(MAX_STRING_LENGTH);
	for (i=0; i < symbol_tree->info.children->len; ++i)
	{
		sym = TM_SYMBOL(symbol_tree->info.children->pdata[i]);
		if (!sym || ! sym->tag || !sym->tag->atts.entry.file)
			continue;
		type = sv_get_node_type(sym);
		root_type = sv_get_root_type(type);
		parent_item = root[root_type];
		if (!parent_item)
			continue;
		sv_assign_node_name(sym, s);
		if (sym->tag->atts.entry.scope)
		{
			g_string_insert(s, 0,"::");
			g_string_insert(s, 0, sym->tag->atts.entry.scope);
		}
		arr[0] = s->str;
		if ((tm_tag_function_t != sym->tag->type) &&
			(sym->info.children) && (sym->info.children->len > 0))
			has_children = TRUE;
		else
			has_children = FALSE;
		item = gtk_ctree_insert_node(GTK_CTREE(sv->tree)
		  , parent_item, NULL, arr, 5, sv_icons[type], sv_bitmaps[type]
		  , sv_icons[type], sv_bitmaps[type], !has_children, FALSE);
		sfile = symbol_file_info_new(sym);
		gtk_ctree_node_set_row_data_full(GTK_CTREE(sv->tree), item
		  , sfile, (GtkDestroyNotify) symbol_file_info_free);
		if (has_children)
		{
			for (j=0; j < sym->info.children->len; ++j)
			{
				sym1 = TM_SYMBOL(sym->info.children->pdata[j]);
				if (!sym1 || ! sym1->tag || ! sym1->tag->atts.entry.file)
					continue;
				type = sv_get_node_type(sym1);
				if (sv_none_t == type)
					continue;
				sv_assign_node_name(sym1, s);
				arr[0] = s->str;
				subitem = gtk_ctree_insert_node(GTK_CTREE(sv->tree)
				  , item, NULL, arr, 5, sv_icons[type], sv_bitmaps[type]
				  , sv_icons[type], sv_bitmaps[type], TRUE, FALSE);
				sfile = symbol_file_info_new(sym1);
				gtk_ctree_node_set_row_data_full(GTK_CTREE(sv->tree)
				  , subitem, sfile, (GtkDestroyNotify) symbol_file_info_free);
			}
		}
	}
	g_string_free(s, TRUE);

clean_leave:
	sv_connect();
	sv_thaw();
	return sv;
}

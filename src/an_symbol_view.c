#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "an_symbol_view.h"
#include "file_history.h"
#include "anjuta.h"

#include "cfolder.xpm"
#include "ofolder.xpm"
#include "class.xpm"
#include "struct.xpm"
#include "function.xpm"
#include "variable.xpm"
#include "macro.xpm"
#include "private_func.xpm"
#include "private_var.xpm"
#include "protected_func.xpm"
#include "protected_var.xpm"
#include "public_func.xpm"
#include "public_var.xpm"

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
	sv_closed_folder_t,
	sv_open_folder_t,
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
	"None", "Classes", "Structs", "Functions", "Variables"
	, "Macros", "None"
};

static GdkPixmap **sv_icons = NULL;
static GdkBitmap **sv_bitmaps = NULL;
static AnSymbolView *sv = NULL;

static SVNodeType sv_get_node_type(TMSymbol *sym)
{
	SVNodeType type;

	g_return_val_if_fail(sym && sym->tag, sv_none_t);
	switch (sym->tag->type)
	{
		case tm_tag_class_t:
			type = sv_class_t;
			break;
		case tm_tag_struct_t:
			type = sv_struct_t;
			break;
		case tm_tag_function_t:
			if (sym->tag->atts.entry.scope)
			{
				if (sym->tag->atts.entry.access == TAG_ACCESS_PRIVATE)
					type = sv_private_func_t;
				if (sym->tag->atts.entry.access == TAG_ACCESS_PROTECTED)
					type = sv_protected_func_t;
				else
					type = sv_public_func_t;
			}
			else
				type = sv_function_t;
			break;
		case tm_tag_member_t:
			if (sym->tag->atts.entry.access == TAG_ACCESS_PRIVATE)
				type = sv_private_var_t;
			if (sym->tag->atts.entry.access == TAG_ACCESS_PROTECTED)
				type = sv_protected_var_t;
			else
				type = sv_private_var_t;
			break;
		case tm_tag_externvar_t:
		case tm_tag_variable_t:
			if (sym->tag->atts.entry.scope)
			{
			if (sym->tag->atts.entry.access == TAG_ACCESS_PRIVATE)
				type = sv_private_var_t;
			if (sym->tag->atts.entry.access == TAG_ACCESS_PROTECTED)
				type = sv_protected_var_t;
			else
				type = sv_private_var_t;
			}
			else
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
	g_return_val_if_fail(sv || (sv_none_t == type), sv_root_none_t);
	if (sv_class_t == type)
		return sv_root_class_t;
	else if (sv_struct_t == type)
		return sv_root_struct_t;
	else if (sv_function_t == type)
		return sv_root_function_t;
	else if (sv_variable_t == type)
		return sv_root_variable_t;
	else if (sv_macro_t == type)
		return sv_root_macro_t;
	else
		return sv_root_none_t;
}

static void sv_load_pixmaps(void)
{
	if (sv_icons)
		return;
	sv_icons = g_new(GdkPixmap *, sv_max_t);
	sv_bitmaps = g_new(GdkBitmap *, sv_max_t);

	g_return_if_fail(sv && sv->win);
	sv_icons[sv_none_t] = NULL;
	sv_icons[sv_class_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_class_t]
	  , NULL, class_xpm);
	sv_icons[sv_struct_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_struct_t]
	  , NULL, struct_xpm);
	sv_icons[sv_function_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_function_t]
	  , NULL, function_xpm);
	sv_icons[sv_variable_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_variable_t]
	  , NULL, variable_xpm);
	sv_icons[sv_macro_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_macro_t]
	  , NULL, macro_xpm);
	sv_icons[sv_private_func_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_private_func_t]
	  , NULL, private_func_xpm);
	sv_icons[sv_private_var_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_private_var_t]
	  , NULL, private_var_xpm);
	sv_icons[sv_protected_func_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_protected_func_t]
	  , NULL, protected_func_xpm);
	sv_icons[sv_protected_var_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_protected_var_t]
	  , NULL, protected_var_xpm);
	sv_icons[sv_public_func_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_public_func_t]
	  , NULL, public_func_xpm);
	sv_icons[sv_public_var_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_public_var_t]
	  , NULL, public_var_xpm);
	sv_icons[sv_closed_folder_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_closed_folder_t]
	  , NULL, cfolder_xpm);
	sv_icons[sv_open_folder_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(sv->win), &sv_bitmaps[sv_open_folder_t]
	  , NULL, ofolder_xpm);
	sv_icons[sv_max_t] = NULL;
	sv_bitmaps[sv_max_t] = NULL;
}

static void sv_on_button_press(GtkWidget *widget
  , GdkEventButton *event, gpointer user_data)
{
}

static void sv_on_select_row(GtkCList *clist, gint row, gint column
  , GdkEventButton *event, gpointer user_data)
{
	GtkCTreeNode *node;
	AnHistFile *h_file;

	node = gtk_ctree_node_nth(GTK_CTREE(sv->tree), row);
	if (!node || !event || row < 0 || column < 0)
		return;
	if (event->type != GDK_2BUTTON_PRESS)
		return;
	if (event->button != 1)
		return;
	h_file = (AnHistFile *) gtk_ctree_node_get_row_data(
	  GTK_CTREE(sv->tree), GTK_CTREE_NODE(node));
	if (h_file && h_file->name)
		anjuta_goto_file_line_mark(h_file->name, h_file->line, TRUE);
}

static void sv_disconnect(void)
{
	g_return_if_fail(sv);
	gtk_signal_disconnect_by_func(GTK_OBJECT(sv->tree)
	  , GTK_SIGNAL_FUNC(sv_on_select_row), NULL);
	gtk_signal_disconnect_by_func(GTK_OBJECT(sv->tree)
	  , GTK_SIGNAL_FUNC(sv_on_button_press), NULL);
}

static void sv_connect(void)
{
	g_return_if_fail(sv);
	gtk_signal_connect(GTK_OBJECT(sv->tree), "select_row"
	  , GTK_SIGNAL_FUNC(sv_on_select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(sv->tree), "button_press_event"
	  , GTK_SIGNAL_FUNC(sv_on_button_press), NULL);
}

static void sv_create(void)
{
	sv = g_new(AnSymbolView, 1);
	sv->project = NULL;
	sv->win=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_ref(sv->win);
	gtk_widget_show(sv->win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sv->win),
	  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	sv->tree=gtk_ctree_new(1,0);
	gtk_widget_ref(sv->tree);
	gtk_widget_show(sv->tree);
	gtk_ctree_set_line_style(GTK_CTREE(sv->tree), GTK_CTREE_LINES_DOTTED);
	gtk_clist_set_column_auto_resize(GTK_CLIST(sv->tree), 0, TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(sv->tree)
	  ,GTK_SELECTION_BROWSE);
	gtk_container_add (GTK_CONTAINER(sv->win), sv->tree);
	if (!sv_icons)
		sv_load_pixmaps();
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
	sv->project = NULL;
}

static void sv_assign_node_name(TMSymbol *sym, GString *s)
{
	g_assert(sym && sym->tag && s);
	g_string_assign(s, sym->tag->name);
	switch(sym->tag->type)
	{
		case tm_tag_function_t:
		case tm_tag_macro_with_arg_t:
			if (sym->tag->atts.entry.arglist)
				g_string_append(s, sym->tag->atts.entry.arglist);
			break;
		default:
			if (sym->tag->atts.entry.var_type)
				g_string_sprintfa(s, " <%s>"
				  , sym->tag->atts.entry.var_type);
			break;
	}
}

#define CREATE_SV_NODE(T) {\
	arr[0] = sv_root_names[(T)];\
	root[(T)] = gtk_ctree_insert_node(GTK_CTREE(sv->tree),\
	  NULL, NULL, arr, 5, sv_icons[sv_closed_folder_t],\
	  sv_bitmaps[sv_closed_folder_t], sv_icons[sv_open_folder_t],\
	  sv_bitmaps[sv_open_folder_t], FALSE, FALSE);}

AnSymbolView *sv_populate(TMProject *tm_proj)
{
	GString *s;
	char *arr[1];
	GSList *parent, *child;
	TMSymbol *sym;
	SVNodeType type;
	AnHistFile *h_file;
	GtkCTreeNode *root[sv_root_max_t];
	GtkCTreeNode *item, *parent_item, *subitem;
	SVRootType i;

	g_message("Populating symbol view..");
	if (!sv)
		sv_create();
	sv_disconnect();
	sv_freeze();
	sv_clear();
	if (!tm_proj || !IS_TM_PROJECT((TMWorkObject *) tm_proj))
	{
		sv_connect();
		sv_thaw();
		g_warning("No project. Returning..");
		return sv;
	}

	if (!tm_proj->symbol_tree)
	{
		g_message("Updating project..");
		tm_project_update(TM_WORK_OBJECT(tm_proj), TRUE, TRUE, TRUE);
		if (!tm_proj->symbol_tree)
		{
			sv_connect();
			sv_thaw();
			g_warning("No symbol tree. Returning..");
			return sv;
		}
	}

	sv->project = tm_proj;
	root[sv_root_none_t] = NULL;
	for (i = sv_root_class_t; i < sv_root_max_t; ++i)
		CREATE_SV_NODE(i)
	root[sv_root_max_t] = NULL;

	s = g_string_sized_new(255);
	if (!tm_proj->symbol_tree->children)
	{
		g_message("No symbols found!");
	}
	for (parent = tm_proj->symbol_tree->children; parent
	  ; parent = g_slist_next(parent))
	{
		if (!parent->data)
			continue;
		sym = TM_SYMBOL(parent->data);
		if (!sym || ! sym->tag || ! sym->tag->atts.entry.file)
			continue;
		type = sv_get_node_type(sym);
		if (sv_none_t == type)
			continue;
		parent_item = root[sv_get_root_type(type)];
		if (!parent_item)
			continue;
		sv_assign_node_name(sym, s);
		
		/* Disabling high volume debug output */
		/* g_message("Got node %s", s->str); */
		
		arr[0] = s->str;
		item = gtk_ctree_insert_node(GTK_CTREE(sv->tree)
		  , parent_item, NULL, arr, 5, sv_icons[type], sv_bitmaps[type]
		  , sv_icons[type], sv_bitmaps[type], (sym->children)?FALSE:TRUE
		  , FALSE);
		h_file = an_hist_file_new((sym->tag->atts.entry.file)->work_object.file_name
		  , sym->tag->atts.entry.line);
		gtk_ctree_node_set_row_data_full(GTK_CTREE(sv->tree), item
		  , h_file, (GtkDestroyNotify) an_hist_file_free);
        for (child = sym->children; child; child = g_slist_next(child))
		{
			sym = TM_SYMBOL(child->data);
			if (!sym || ! sym->tag || ! sym->tag->atts.entry.file)
				continue;
			type = sv_get_node_type(sym);
			if (sv_none_t == type)
				continue;
			sv_assign_node_name(sym, s);
			arr[0] = s->str;
			subitem = gtk_ctree_insert_node(GTK_CTREE(sv->tree)
			  , item, NULL, arr, 5, sv_icons[type], sv_bitmaps[type]
			  , sv_icons[type], sv_bitmaps[type], TRUE, FALSE);
			h_file = an_hist_file_new((sym->tag->atts.entry.file)->work_object.file_name
			  , sym->tag->atts.entry.line);
			gtk_ctree_node_set_row_data_full(GTK_CTREE(sv->tree)
			  , subitem, h_file, (GtkDestroyNotify) an_hist_file_free);
		}
	}
	g_string_free(s, TRUE);
	sv_connect();
	sv_thaw();
	return sv;
}

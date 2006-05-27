/*
    debug_tree.c
    Copyright (C) 2006  Sébastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "debug_tree.h"

#include "plugin.h"
#include "memory.h"

#include <libgnome/gnome-i18n.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Types
 *---------------------------------------------------------------------------*/

enum _DataType
{
	TYPE_ROOT,
	TYPE_UNKNOWN,
	TYPE_POINTER,
	TYPE_ARRAY,
	TYPE_STRUCT,
	TYPE_VALUE,
	TYPE_REFERENCE,
	TYPE_NAME
};

typedef enum _DataType DataType;

typedef struct _TrimmableItem TrimmableItem;

enum {AUTO_UPDATE_WATCH = 1 << 0};	
	
struct _TrimmableItem
{
	DataType dataType;
	gchar *name;
	gchar *value;
	gboolean expandable;
	gboolean expanded;
	gboolean modified;
	gboolean removed;
	gint display_type;
	
	guint used;         /* Currently used by on callback function */
};

/* Common data */
typedef struct _CommonDebugTree CommonDebugTree;

struct _CommonDebugTree {
	GtkActionGroup *action_group;
	GtkActionGroup *toggle_group;
	guint initialized;
	DebugTree* current;
	DebugTree* user;
};

/* The debug tree object */
struct _DebugTree {
	IAnjutaDebugger *debugger;
	AnjutaPlugin *plugin;
	GtkWidget* tree;        /* the tree widget */
	GtkTreeIter* cur_node;
	GtkWidget* middle_click_menu;
	GSList* free_list;
};

typedef struct _WatchCallBack WatchCallBack;
	
struct _WatchCallBack
{
	DebugTree *var;
	GtkTreeIter iter;
	TrimmableItem *item;
	gchar *value;
};

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define ADD_WATCH_DIALOG "add_watch_dialog"
#define CHANGE_WATCH_DIALOG "change_watch_dialog"
#define INSPECT_EVALUATE_DIALOG "inspect_evaluate_dialog"
#define NAME_ENTRY "name_entry"
#define VALUE_ENTRY "value_entry"
#define AUTO_UPDATE_CHECK "auto_update_check"

/* Constant
 *---------------------------------------------------------------------------*/

#define UNKNOWN_VALUE "???"

#define AUTO_UPDATE 'U'

#if 0
#define FORMAT_DEFAULT  0	/* gdb print command without switches */
#define FORMAT_BINARY   1	/* print/t */
#define FORMAT_OCTAL    2	/* print/o */
#define FORMAT_SDECIMAL 3	/* print/d */
#define FORMAT_UDECIMAL 4	/* print/u */
#define FORMAT_HEX      5	/* print/x */
#define FORMAT_CHAR     6	/* print/c */

/* gdb print commands array for different display types */
gchar* DisplayCommands[] = { "print " ,
							 "print/t ",
							 "print/o ",
							 "print/d ",
							 "print/u ",
							 "print/x ",
							 "print/c " };


gchar *type_names[] = {
	"Root",
	"Unknown",
	"Pointer",
	"Array",
	"Struct",
	"Value",
	"Reference",
	"Name"
};
#endif

/* stores colors for variables */
/*
static GtkStyle *style_red;
static GtkStyle *style_normal;
*/

enum {
	VARIABLE_COLUMN,
	VALUE_COLUMN,
	ITEM_COLUMN,
	N_COLUMNS
};

static gchar *tree_title[] = {
	N_("Variable"), N_("Value")
};

/* Global variable
 *---------------------------------------------------------------------------*/

static CommonDebugTree* gCommonDebugTree = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

static gboolean 
get_current_iter (GtkTreeView *view, GtkTreeIter* iter)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (view);
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}

static void
dma_gtk_tree_model_foreach_child (GtkTreeModel *model, GtkTreeIter *parent,
								  GtkTreeModelForeachFunc func,
								  gpointer user_data)
{
	GtkTreeIter child;
	
	if (gtk_tree_model_iter_children (model, &child, parent) == FALSE) return;
	for(;;)
	{
		dma_gtk_tree_model_foreach_child (model, &child, func, user_data);
		func (model, NULL, &child, user_data);
		if (gtk_tree_model_iter_next (model, &child) == FALSE) break;
	}
}

/* TrimmableItem function
 *---------------------------------------------------------------------------*/

static TrimmableItem*
trimmable_item_new (void)
{
	TrimmableItem* item;
	
	item = g_new0 (TrimmableItem, 1);
	
	return item;
}	

static void
trimmable_item_free (TrimmableItem *this, DebugTree *owner)
{
	g_free (this->name);
	this->name = NULL;
	if (this->value)
	{	
		g_free (this->value);
		this->value = NULL;
	}
	
	if (this->used)
	{
		/* Call back function using this has still not return */
		/* Keep object in free list */
		this->removed = TRUE;
		owner->free_list = g_slist_prepend (owner->free_list, this);
	}
	else
	{
		g_free (this);
	}
}

static void
trimmable_item_set_name (TrimmableItem *this, const gchar *name)
{
	if (this->name) g_free (this->name);
	this->name = g_strconcat (" ", name, NULL);
}

static void
trimmable_item_set_value (TrimmableItem *this, const gchar *value)
{
	if (this->value) g_free (this->value);
	this->value = value == NULL ? NULL : g_strdup (value);
}

/* DebugTree private functions
 *---------------------------------------------------------------------------*/

#if 0
static void
debug_tree_free_all_items (DebugTree *this)
{
	g_slist_foreach (this->free_list, (GFunc)g_free, NULL);
	g_slist_free (this->free_list);
	this->free_list = NULL;
}

/* build a menu item for the middle-click button @param menutext text to
 * display on the menu item @param signalhandler signal handler for item
 * selection @param menu menu the item belongs to @param data private data for 
 * the signal handler */

static
GtkWidget *
build_menu_item (gchar * menutext, GtkSignalFunc signalhandler,
				 GtkWidget * menu, gpointer data)
{
	GtkWidget *menuitem;

	if (menutext)
		menuitem = gtk_menu_item_new_with_label (menutext);
	else
		menuitem = gtk_menu_item_new ();

	if (signalhandler)
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",signalhandler,data);

	if (menu)
		gtk_menu_append (GTK_MENU (menu), menuitem);

	return menuitem;
}

/* add a separator to a menu */
static void
add_menu_separator (GtkWidget * menu)
{
	g_return_if_fail (menu);
                                                                                
	gtk_menu_shell_append (GTK_MENU_SHELL (menu),
			gtk_separator_menu_item_new ());
}

static void
show_hide_popup_menu_items (GtkWidget * menu, gint start, gint end,
							gboolean sensitive)
{
	GtkMenuShell *menu_shell;
	GList *list;
	gint nb = 0;

	g_return_if_fail (menu);
	g_return_if_fail (start <= end);

	menu_shell = GTK_MENU_SHELL (menu);
	list = menu_shell->children;

	while (list)
	{
		/* if (nb >= start && nb <= end)
			gtk_widget_set_sensitive (GTK_WIDGET (list->data), sensitive); */
		list = g_list_next (list);
		nb++;
	}
}
#endif

#if 0
/* middle click call back for bringing up the format menu */

static gboolean
debug_tree_on_middle_click (GtkWidget *widget,
							GdkEvent *event, gpointer data)
{
	GdkEventButton *buttonevent = NULL;
	DebugTree *d_tree = NULL;
	GtkTreeView *tree = NULL;
	GtkTreeIter iter;
	GtkTreeIter parent;
	TrimmableItem *node_data = NULL;
	GtkTreeSelection* selection;
	GtkTreeModel* model = NULL;
		
	buttonevent = (GdkEventButton *) event;

	/* get the selected row */
	d_tree = (DebugTree*)data;
	tree = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model(tree);
	
	selection = gtk_tree_view_get_selection (tree);
	
	if (!gtk_tree_selection_get_selected(selection,NULL,&iter)) {
		g_warning("Unable to get selected row\n");
		return FALSE;
	}
		
	gtk_tree_model_get (model, &iter, ITEM_COLUMN, &node_data, -1);

	/* only for items with 'real' data */
	if (!node_data || node_data->dataType == TYPE_ROOT) {
		g_print("Not real data\n");
		return FALSE;
	}

	if (!gtk_tree_model_iter_parent(model,&parent,&iter)) {
		g_warning("Unable to get parent\n");
		return FALSE;
	}

	/* Do not allow long arrays */
	/*if (is_long_array (GTK_TREE_VIEW (tree), &parent))
		return FALSE;*/

	if (node_data->dataType == TYPE_VALUE)
	{
		show_hide_popup_menu_items (d_tree->middle_click_menu, 0, 9, TRUE);
	}
	else
	{
		show_hide_popup_menu_items (d_tree->middle_click_menu, 0, 7, FALSE);
		show_hide_popup_menu_items (d_tree->middle_click_menu, 8, 9, TRUE);
	}

	d_tree->cur_node = gtk_tree_iter_copy(&iter);

	/* ok - show the menu */
	gtk_widget_show_all (GTK_WIDGET (d_tree->middle_click_menu));
	gtk_menu_popup (GTK_MENU (d_tree->middle_click_menu), NULL, NULL, NULL,
					NULL, buttonevent->button, 0);
	return TRUE;
}

/* changes the display format of a node */
static void
change_display_type (DebugTree *d_tree, gint display_type)
{
	TrimmableItem *node_data = NULL;
	GtkTreeModel* model;

	g_return_if_fail (d_tree);
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(d_tree->tree));

	/* get the node's private data */
	gtk_tree_model_get (model, d_tree->cur_node, ITEM_COLUMN, &node_data, -1);
	
	g_return_if_fail (node_data != NULL);

	/* if the display type did not change - skip the operation altogether */
	if (node_data->display_type == display_type)
		return;

	/* store the new display type in the node's private data */
	node_data->display_type = display_type;

	debug_ctree_cmd_gdb (d_tree, GTK_TREE_VIEW(d_tree->tree),
						 d_tree->cur_node, NULL,
						 display_type, FALSE);
	
	gtk_tree_iter_free(d_tree->cur_node);
	d_tree->cur_node = NULL;
}

static void
on_format_default_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_DEFAULT);
}

static void
on_format_binary_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_BINARY);
}

static void
on_format_octal_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_OCTAL);
}

static void
on_format_signed_decimal_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_SDECIMAL);
}

static void
on_format_unsigned_decimal_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_UDECIMAL);
}

static void
on_format_hex_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_HEX);
}

static void
on_format_char_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
//	change_display_type (d_tree, FORMAT_CHAR);
}
#endif


#if 0
/*
 Return full name of the supplied node. caller must free the returned
 string
*/
static gchar *
extract_full_name (GtkTreeView * ctree, GtkTreeIter *node)
{
	TrimmableItem *data = NULL;
	gchar *full_name = NULL;
	gint pointer_count = 0;
	gint i;
	gboolean first = TRUE;
	gchar *t = NULL;
	GtkTreeModel *model;
	GtkTreeIter child = *node; /* Piter initialized to point to required node */
	GtkTreeIter parent;	
	GtkTreeIter temp;
	
	model = gtk_tree_view_get_model(ctree);
	gtk_tree_model_get (model, &child, ITEM_COLUMN, &data, -1);
	
	if (data->name[1] == '*')
		pointer_count++;
	else
	{
		full_name = g_strdup (data->name);
		first = FALSE;
	}
	/* go from node to the root */
	while (data->dataType != TYPE_ROOT)
	{
		/* get the current node's parent */
		if (!gtk_tree_model_iter_parent(model,&parent,&child))
			break;
		
		gtk_tree_model_get (model, &parent, ITEM_COLUMN, &data, -1);

		temp = child;
		child = parent;
		parent = temp;
		
		if (data->name[1] == '*')
		{
			pointer_count++;
			continue;
		}
		if (data->dataType != TYPE_ROOT && data->dataType != TYPE_ARRAY)
		{
			if (first)
			{
				full_name = g_strdup (data->name);
				first = FALSE;
			}
			else
			{
				t = full_name;
				full_name = g_strconcat (data->name, ".", full_name, NULL);
				g_free (t);
			}
		}
	}
	for (i = 0; i < pointer_count; i++)
	{
		t = full_name;
		full_name = g_strconcat ("*", full_name, NULL);
		g_free (t);
	}
	return full_name;
}
#endif

#if 0
static gboolean
find_expanded (GtkTreeModel *model, GtkTreePath* path,
			   GtkTreeIter* iter, gpointer pdata)
{
	TrimmableItem *data;
	GList ** list = pdata;
	
	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, ITEM_COLUMN, &data, -1);	

	if (data && data->expanded)
	{
		//g_print("Appending %s to the expanded list\n",data->name);
		*list = g_list_prepend (*list, gtk_tree_iter_copy(iter));
	}
	return FALSE;
}

static void
on_debug_tree_row_expanded (GtkTreeView * ctree, GtkTreeIter* iter,
							GtkTreePath* path, gpointer data)
{
	GList *expanded_list = NULL;
	GtkTreeModel* model;
	DebugTree *debug_tree = (DebugTree *)data;
	
	g_return_if_fail (ctree);
	g_return_if_fail (iter);

	model = gtk_tree_view_get_model (ctree);
	
	/* Search expanded */
	gtk_tree_model_foreach(model,find_expanded,&expanded_list);	

	if (expanded_list)
	{
		TrimmableItem *data;
		GtkTreeIter* iter = (GtkTreeIter*)expanded_list->data;
		gtk_tree_model_get (model, iter, ITEM_COLUMN, &data, -1);			
		debug_ctree_cmd_gdb (debug_tree, ctree,iter,expanded_list,data->display_type,TRUE);
		gtk_tree_iter_free(iter);
	}
}

static gboolean
debug_tree_on_select_row (GtkWidget *widget, GdkEvent *event,
						  gpointer user_data)
{
	TrimmableItem *data;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GdkEventButton *buttonevent = NULL;
	
	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	if (event->type == GDK_BUTTON_PRESS) {
		buttonevent = (GdkEventButton *) event;

		if (buttonevent->button == 3)
			return debug_tree_on_middle_click (widget, event, user_data);
	}
	
	/* only when double clicking */
	if (!(event->type == GDK_2BUTTON_PRESS))
		return FALSE;
	
	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter) || !event)
	{
		g_warning("Error getting selection\n");
		return FALSE;
	}
	gtk_tree_model_get (model, &iter, ITEM_COLUMN, &data, -1);
	
	if (!data)
	{
		g_warning("Unable to get data\n");
		return FALSE;
	}

	/* g_print ("SELECT : %p %d %s %s %d %d\n", node, data->dataType,
	 * data->name, data->value, data->expandable, data->expanded); */
	if (data->expandable)
	{
		if (!data->expanded)
		{
			GtkTreePath* path;
			data->expanded = TRUE;			
/*			debug_ctree_cmd_gdb ((DebugTree*)user_data, view, &iter,
								  NULL, FORMAT_DEFAULT, TRUE);*/
			path = gtk_tree_model_get_path(model, &iter);
			if (!path)
				g_warning("cannot get path\n");
			else {
				gtk_tree_view_expand_row(view,path,FALSE);
				gtk_tree_path_free(path);
			}
			/* gtk_ctree_expand (ctree, node); */
		}
		else /* if
			  (GTK_CTREE_ROW(node)->is_leaf) */
		{
			/* g_print("EXPFALSE\n"); */
			data->expanded = FALSE;
			gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
							   VALUE_COLUMN, data->value,-1);			
		}
	}
	else
		g_warning ("%s is not expandable\n", data->name);
	
	return TRUE;
}
#endif

#if 0
static gboolean
set_not_analyzed(GtkTreeModel *model, GtkTreePath* path,
				 GtkTreeIter* iter, gpointer pdata)
{
	TrimmableItem *data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);
	
	gtk_tree_model_get(model, iter, ITEM_COLUMN, &data, -1);

	if (data && data->dataType != TYPE_ROOT)
	{
		//g_print("Setting %s to not analyzed\n",data->name);
		data->analyzed = FALSE;
	}

	return FALSE;	
}

static gboolean
destroy_recursive(GtkTreeModel *model, GtkTreePath* path,
				  GtkTreeIter* iter, gpointer pdata)
{
	TrimmableItem *data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);
	
	gtk_tree_model_get(model, iter, ITEM_COLUMN, &data, -1);
	
	if (!data) {
		g_warning("Error getting data\n");
		return TRUE;
	}
	
	if (data->analyzed == FALSE && data->dataType != TYPE_ROOT)
	{
		//g_print("Destroying %s\n",data->name);
		g_free (data->name);
		g_free (data->value);
		g_free (data);
		gtk_tree_store_remove(GTK_TREE_STORE(model), iter);
	}
	//else
		//g_print("Not destroying %s\n",data->name);

	return FALSE;
}


static void
destroy_non_analyzed (GtkTreeModel* model, GtkTreeIter* parent)
{
	TrimmableItem *data;
	GtkTreeIter iter;
	gboolean success;
	
	g_return_if_fail (model);
	g_return_if_fail (parent);
	
	success = gtk_tree_model_iter_children(model,&iter,parent);
	if (!success) {
		g_warning("Cannot get root\n");
		return;
	}
	do
	{
		gtk_tree_model_get(model, &iter, ITEM_COLUMN, &data, -1);

		if (!data)
		{		
			g_warning ("Failed getting row data\n");
			return;
		}
		if (!data->analyzed)
		{
			/* g_print("destroying %s\n",data->name); */
			g_free (data->name);
			g_free (data->value);
			g_free (data);
			success = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		}
		else
		{
			/* g_print("not destroying %s\n",data->name); */
			success = gtk_tree_model_iter_next (model, &iter);
		}
	}
	while (success);
}
#endif

static gboolean
delete_node(GtkTreeModel *model, GtkTreePath* path,
			GtkTreeIter* iter, gpointer user_data)
{
	DebugTree *var = (DebugTree *)user_data;
	TrimmableItem *item;
	
	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, ITEM_COLUMN, &item, -1);	
	trimmable_item_free (item, var);

	return FALSE;
}

static void
debug_tree_remove (DebugTree* d_tree, GtkTreeIter* parent)
{
	TrimmableItem* item;
	GtkTreeModel* model;
	GtkTreeIter iter;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (d_tree->tree));
	
	if (gtk_tree_model_iter_children(model,&iter,parent))
	{
		do
		{
			gtk_tree_model_get(model, &iter, ITEM_COLUMN, &item, -1);
			trimmable_item_free (item, d_tree);
		}
		while (gtk_tree_store_remove (GTK_TREE_STORE (model), &iter));
	}
	
	gtk_tree_model_get(model, parent, ITEM_COLUMN, &item, -1);
	trimmable_item_free (item, d_tree);
	gtk_tree_store_remove (GTK_TREE_STORE (model), parent);
}

#if 0
static void
debug_tree_pointer_recursive (DebugTree *d_tree, GtkTreeView* tree)
{
	GList *list = NULL;
	TrimmableItem* data;
	GtkTreeModel* model;

	g_return_if_fail (tree);
	
	model = gtk_tree_view_get_model(tree);

	/* Search expanded viewable pointers */
	gtk_tree_model_foreach(model,find_expanded,&list);	

	/* send cmd to gdb - Then wait the end of the processing to send the next */
	if (list)
	{
		GtkTreeIter* iter = (GtkTreeIter*)list->data;
		gtk_tree_model_get(model, iter, ITEM_COLUMN, &data, -1);
		debug_ctree_cmd_gdb (d_tree, tree, iter, list,
							 data->display_type, TRUE);
		gtk_tree_iter_free(iter);
	}
}
#endif

#if 0
static void
on_inspect_memory_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	gchar *buf;
	gchar *start, *end;
	gchar *hexa;
	guchar *adr;
	GtkWidget *memory;
	GtkTreeModel* model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(d_tree->tree));

	gtk_tree_model_get(model, d_tree->cur_node, VALUE_COLUMN, &buf, -1);

	while (*buf != '\0' && !(*buf == '0' && *(buf + 1) == 'x'))
		buf++;
	if (*buf != '\0')
	{
		end = start = buf + 2;
		while ((*end >= '0' && *end <= '9') || (*end >= 'a' && *end <= 'f'))
			end++;
		hexa = g_strndup (start, end - start);
		adr = memory_info_address_to_decimal (hexa);
		memory = memory_info_new (d_tree->debugger, NULL, adr);
		gtk_widget_show (memory);
		g_free (hexa);
	}
}
#endif

static void
debug_tree_cell_data_func (GtkTreeViewColumn *tree_column,
					GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data)
{
	gchar *value;
	static const gchar *colors[] = {"black", "red"};
	GValue gvalue = {0, };
	TrimmableItem *item;

	gtk_tree_model_get (tree_model, iter, VALUE_COLUMN, &value, -1);
	g_value_init (&gvalue, G_TYPE_STRING);
	g_value_set_static_string (&gvalue, value);
	g_object_set_property (G_OBJECT (cell), "text", &gvalue);

	gtk_tree_model_get (tree_model, iter, ITEM_COLUMN, &item, -1);
	g_value_reset (&gvalue);
	/* fix me : 'item_data' can be NULL */
	g_value_set_static_string (&gvalue, colors[(item && item->modified ? 1 : 0)]);

	g_object_set_property (G_OBJECT (cell), "foreground", &gvalue);
	g_free (value);
}

static void
debug_tree_set_auto_update (DebugTree* this, GtkTreeIter* iter, gboolean state)
{
	GtkTreeModel *model;
	TrimmableItem *item;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);
	if (item != NULL)
	{
		if (state)
			item->name[0] |= AUTO_UPDATE_WATCH;
		else
			item->name[0] &= ~AUTO_UPDATE_WATCH;
	}
}

static gboolean
debug_tree_get_auto_update (DebugTree* this, GtkTreeIter* iter)
{
	GtkTreeModel *model;
	TrimmableItem *item;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);
	
	if (item != NULL)
	{
		return item->name[0] & AUTO_UPDATE_WATCH ? TRUE : FALSE;
	}
	else
	{
		return FALSE;
	}
}

static void
debug_tree_update_watch (DebugTree *this, GtkTreeIter *iter, const gchar *value)
{
	GtkTreeModel* model;
	TrimmableItem* parent;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &parent, -1);

	if ((parent->value == NULL) || (strcmp (parent->value, value) != 0))
	{
		if (parent->value != NULL) g_free (parent->value);
		parent->value = g_strdup (value);
		parent->modified = TRUE;
		gtk_tree_store_set(GTK_TREE_STORE(model), iter, 
				   VARIABLE_COLUMN, &parent->name[1],
				   VALUE_COLUMN, parent->value, -1);
	}
}

static void
on_watch_updated (const gchar *value, gpointer user_data)
{
	WatchCallBack *watch_data = (WatchCallBack *)user_data;
	
	if (watch_data->item->removed == FALSE)
	{
		debug_tree_update_watch(watch_data->var, &(watch_data->iter), value);
	}
	
	watch_data->item->used = FALSE;
	g_free (watch_data);
}

static void
on_entry_updated (const gchar *value, gpointer user_data)
{
	GtkWidget *entry = (GtkWidget *)user_data;
	
	gtk_entry_set_text (GTK_ENTRY (entry), value);
	gtk_widget_unref (entry);
}

static void
debug_tree_update (DebugTree* this, GtkTreeIter* iter, gboolean force)
{
	GtkTreeModel *model;
	TrimmableItem *item;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);
	if ((item->name != NULL) && (force || (item->name[0] & AUTO_UPDATE_WATCH)))
	{
		WatchCallBack *watch_data;
		
		watch_data = g_new (WatchCallBack, 1);  /* Free by call back */
		watch_data->var = this;
		watch_data->iter = *iter;
		watch_data->item = item;
		item->used = TRUE;
		ianjuta_debugger_inspect (this->debugger, &item->name[1], on_watch_updated, watch_data, NULL);
	}
}

static void
debug_tree_evaluate (DebugTree* this, GtkTreeIter* iter, const gchar *value)
{
	GtkTreeModel *model;
	TrimmableItem *item;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);
	if (item->name != NULL)
	{
		WatchCallBack *watch_data;
		
		watch_data = g_new (WatchCallBack, 1);  /* Free by call back */
		watch_data->var = this;
		watch_data->iter = *iter;
		watch_data->item = item;
		item->used = TRUE;
		ianjuta_debugger_evaluate (this->debugger, &item->name[1], value, NULL, watch_data, NULL);
		ianjuta_debugger_inspect (this->debugger, &item->name[1], on_watch_updated, watch_data, NULL);
	}
}

static void
debug_tree_add_watch_dialog (DebugTree *d_tree, const gchar* expression)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *name_entry;
	GtkWidget *auto_update_check;
	gint reply;

	gxml = glade_xml_new (GLADE_FILE, ADD_WATCH_DIALOG, NULL);
	dialog = glade_xml_get_widget (gxml, ADD_WATCH_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  NULL);
	auto_update_check = glade_xml_get_widget (gxml, AUTO_UPDATE_CHECK);
	name_entry = glade_xml_get_widget (gxml, NAME_ENTRY);
	g_object_unref (gxml);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_update_check), TRUE);
	gtk_entry_set_text (GTK_ENTRY (name_entry), expression == NULL ? "" : expression);
	
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	if (reply == GTK_RESPONSE_OK)
	{
		debug_tree_add_watch (d_tree, gtk_entry_get_text (GTK_ENTRY (name_entry)),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (auto_update_check)));
	}
	gtk_widget_destroy (dialog);
}

static void
debug_tree_change_watch_dialog (DebugTree *d_tree, GtkTreeIter* iter)
{
 	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *name_entry;
	GtkWidget *value_entry;
	gint reply;
	TrimmableItem *item = NULL;
	GtkTreeModel* model = NULL;
	
	model = gtk_tree_view_get_model(d_tree->tree);
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);

	gxml = glade_xml_new (GLADE_FILE, CHANGE_WATCH_DIALOG, NULL);
	dialog = glade_xml_get_widget (gxml, CHANGE_WATCH_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  NULL);
	name_entry = glade_xml_get_widget (gxml, NAME_ENTRY);
	value_entry = glade_xml_get_widget (gxml, VALUE_ENTRY);
	g_object_unref (gxml);

	gtk_widget_grab_focus (value_entry);
	gtk_entry_set_text (GTK_ENTRY (name_entry), &item->name[1]);
	gtk_entry_set_text (GTK_ENTRY (value_entry), item->value);
	
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	if (reply == GTK_RESPONSE_APPLY)
	{
		debug_tree_evaluate (d_tree, iter, gtk_entry_get_text (GTK_ENTRY (value_entry))); 
	}
	gtk_widget_destroy (dialog);
}

static void
debug_tree_inspect_evaluate_dialog (const gchar* expression)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *name_entry;
	GtkWidget *value_entry;
	gint reply;
	const gchar *name;
	const gchar *value;
	DebugTree *d_tree = gCommonDebugTree->user;

	gxml = glade_xml_new (GLADE_FILE, INSPECT_EVALUATE_DIALOG, NULL);
	dialog = glade_xml_get_widget (gxml, INSPECT_EVALUATE_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  NULL);
	name_entry = glade_xml_get_widget (gxml, NAME_ENTRY);
	value_entry = glade_xml_get_widget (gxml, VALUE_ENTRY);
	g_object_unref (gxml);

	if (expression == NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (name_entry), "");
		gtk_entry_set_text (GTK_ENTRY (value_entry), "");
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY (name_entry), expression);
		if (d_tree->debugger != NULL)
			ianjuta_debugger_inspect (d_tree->debugger, expression, on_entry_updated, value_entry, NULL);
	}

	for(;;)
	{
		reply = gtk_dialog_run (GTK_DIALOG (dialog));
		switch (reply)
		{
		case GTK_RESPONSE_OK:
			name = gtk_entry_get_text (GTK_ENTRY (name_entry));
			debug_tree_add_watch (d_tree, name, FALSE);
		    continue;
		case GTK_RESPONSE_APPLY:
			name = gtk_entry_get_text (GTK_ENTRY (name_entry));
			value = gtk_entry_get_text (GTK_ENTRY (value_entry));
		    gtk_widget_ref (value_entry);
		    if (d_tree->debugger != NULL)
			{
				ianjuta_debugger_evaluate (d_tree->debugger, name, value, NULL, value_entry, NULL);
				ianjuta_debugger_inspect (d_tree->debugger, name, on_entry_updated, value_entry, NULL);
			}
			continue;
		case GTK_RESPONSE_ACCEPT:
			name = gtk_entry_get_text (GTK_ENTRY (name_entry));
		    gtk_widget_ref (value_entry);
		    if (d_tree->debugger != NULL)
				ianjuta_debugger_inspect (d_tree->debugger, name, on_entry_updated, value_entry, NULL);
			continue;
		default:
			break;
		}
		break;
	}
	gtk_widget_destroy (dialog);
}



/* Public functions
 *---------------------------------------------------------------------------*/

/* clear the display of the debug tree and reset the title */
void
debug_tree_remove_all (DebugTree *this)
{
	GtkTreeModel *model;

	g_return_if_fail (this);
	g_return_if_fail (this->tree);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	gtk_tree_model_foreach(model,delete_node, this);	
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}
#if 0
static void
debug_tree_create_child (GtkTreeStore *tree, GtkTreeIter *iter, GtkTreeIter *parent, GtkTreeIter *sibling, const gchar *value, gboolean array)
{
	TrimmableItem *child;
	gchar* buf = NULL;
				
	child = trimmable_item_new ();
	gtk_tree_store_insert_after (tree, iter, parent, sibling);

	if (array)
	{
		// Create name for array
		buf = g_strdup_printf ("[%u]", watch->index);
	}
				
	gtk_tree_store_set(tree, iter, 
		   VARIABLE_COLUMN, array ? buf :  watch->name, 
		   VALUE_COLUMN, UNKNOWN_VALUE,
		   ITEM_COLUMN, child, -1);
				
	if (buf != NULL) g_free (buf);
}
#endif

void
debug_tree_add_watch (DebugTree *this, const gchar *expression, gboolean auto_update)
{
	GtkTreeModel *model;
	TrimmableItem *item;
	GtkTreeIter iter;

	g_return_if_fail (this);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	
	/* Add new watch in tree view */
	item = trimmable_item_new ();
	item->name = g_strconcat (" ", expression, NULL);
	if (auto_update)
		item->name[0] |= AUTO_UPDATE_WATCH;
	else
		item->name[0] &= ~AUTO_UPDATE_WATCH;
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 
					   VARIABLE_COLUMN, expression, 
					   VALUE_COLUMN, UNKNOWN_VALUE,
					   ITEM_COLUMN, item, -1);

	/* Send to debugger if possible */
	if (this->debugger)
	{
		WatchCallBack *watch_data;
		
		watch_data = g_new (WatchCallBack, 1);  /* Free by call back */
		watch_data->var = this;
		watch_data->iter = iter;
		watch_data->item = item;
		item->used = TRUE;
		ianjuta_debugger_inspect (this->debugger, &item->name[1], on_watch_updated, watch_data, NULL);
	}
}

static void
on_add_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	const gchar *expression = &((const gchar *)data)[1];
	gboolean auto_update = ((const gchar *)data)[0] & AUTO_UPDATE_WATCH ? TRUE : FALSE;
	
	debug_tree_add_watch (this, expression, auto_update);
}

void
debug_tree_add_full_watch_list (DebugTree *this, GList *expressions)
{
	g_list_foreach (expressions, on_add_watch, this);
}

static void
on_add_manual_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	const gchar *expression = &((const gchar *)data)[0];
	
	debug_tree_add_watch (this, expression, FALSE);
}

static void
on_add_auto_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	const gchar *expression = &((const gchar *)data)[0];
	
	debug_tree_add_watch (this, expression, TRUE);
}

void
debug_tree_add_watch_list (DebugTree *this, GList *expressions, gboolean auto_update)
{
	g_list_foreach (expressions, auto_update ? on_add_auto_watch : on_add_manual_watch, this);
}

void
debug_tree_update_all (DebugTree* this, gboolean force)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	/* Update if debugger is connected */
	if (this->debugger == NULL) return;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	
	if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
	{
		do
		{
			debug_tree_update (this, &iter, force);
		} while (gtk_tree_model_iter_next (model, &iter) == TRUE);
	}
}

GList*
debug_tree_get_full_watch_list (DebugTree *this)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList* list = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->tree));
	
	if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
	{
		do
		{
			TrimmableItem *item;

			gtk_tree_model_get (model, &iter, ITEM_COLUMN, &item, -1);

			if (item->name != NULL)
			{
				list = g_list_prepend (list, item->name);
			}
		} while (gtk_tree_model_iter_next (model, &iter) == TRUE);
	}
	
	list = g_list_reverse (list);
	
	return list;
}


GtkWidget *
debug_tree_get_tree_widget (DebugTree *this)
{
	return this->tree;
}

void
debug_tree_connect (DebugTree *this, IAnjutaDebugger* debugger)
{
	this->debugger = debugger;
}

void
debug_tree_disconnect (DebugTree *this)
{
	this->debugger = NULL;
}

/* Menu call backs
 *---------------------------------------------------------------------------*/

static void
on_debug_tree_inspect (GtkAction *action, gpointer user_data)
{
	debug_tree_inspect_evaluate_dialog (NULL);
}

static void
on_debug_tree_add_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree;
	CommonDebugTree* common = (CommonDebugTree *)user_data;
	
	d_tree = common->current == NULL ? common->user : common->current;
	
	debug_tree_add_watch_dialog (d_tree, NULL);
}

static void
on_debug_tree_remove_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree = ((CommonDebugTree*) user_data)->current;
	GtkTreeIter iter;
	
	if (get_current_iter (GTK_TREE_VIEW(d_tree->tree), &iter))
	{
		debug_tree_remove (d_tree, &iter);
	}
}

static void
on_debug_tree_update_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree = ((CommonDebugTree*) user_data)->current;
	GtkTreeIter iter;
	
	if (get_current_iter (GTK_TREE_VIEW(d_tree->tree), &iter))
	{
		debug_tree_update (d_tree, &iter, TRUE);
	}
}

static void
on_debug_tree_auto_update_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree = ((CommonDebugTree*) user_data)->current;
	GtkTreeIter iter;

	if (get_current_iter (GTK_TREE_VIEW(d_tree->tree), &iter))
	{
		gboolean state;
		
		state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
		debug_tree_set_auto_update (d_tree, &iter, state);
	}
}

static void
on_debug_tree_edit_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree = ((CommonDebugTree*) user_data)->current;
	GtkTreeIter iter;
	
	if (get_current_iter (GTK_TREE_VIEW(d_tree->tree), &iter))
	{
		debug_tree_change_watch_dialog (d_tree, &iter);
	}
}

static void
on_debug_tree_update_all_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree = ((CommonDebugTree*) user_data)->current;
	
	debug_tree_update_all (d_tree, TRUE);
}

static void
on_debug_tree_remove_all_watch (GtkAction *action, gpointer user_data)
{
	DebugTree *d_tree = ((CommonDebugTree*) user_data)->current;
	
	debug_tree_remove_all (d_tree);
}

static void
on_debug_tree_hide_popup (GtkWidget *widget, gpointer user_data)
{
	gCommonDebugTree->current = NULL;
}

static gboolean
on_debug_tree_button_press (GtkWidget *widget, GdkEventButton *bevent, gpointer user_data)
{
	DebugTree *d_tree = (DebugTree*) user_data;

	if (bevent->button == 3)
	{
		GtkAction *action;
		AnjutaUI *ui;
		GtkTreeIter iter;

		gCommonDebugTree->current = d_tree;
		ui = anjuta_shell_get_ui (d_tree->plugin->shell, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupWatchToggle", "ActionDmaAutoUpdateWatch");

		if (get_current_iter (GTK_TREE_VIEW(d_tree->tree), &iter))
		{
			gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), debug_tree_get_auto_update (d_tree, &iter));
		}
		else
		{
			gtk_action_set_sensitive (GTK_ACTION (action), FALSE);
		}
		
		if (d_tree->middle_click_menu == NULL)
		{
			d_tree->middle_click_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupWatch");
			g_object_ref (d_tree->middle_click_menu);
			g_signal_connect (d_tree->tree, "hide", G_CALLBACK (on_debug_tree_hide_popup), NULL);  
		}
		gtk_menu_popup (GTK_MENU (d_tree->middle_click_menu), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
	}
	
	return FALSE;
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_watch[] = {
    {
		"ActionDmaInspect",                      /* Action name */
		GTK_STOCK_DIALOG_INFO,                   /* Stock icon, if any */
		N_("Ins_pect/Evaluate..."),              /* Display label */
		NULL,                                    /* short-cut */
		N_("Inspect or evaluate an expression or variable"), /* Tooltip */
		G_CALLBACK (on_debug_tree_inspect) /* action callback */
    },
	{
		"ActionDmaAddWatch",                      
		NULL,                                     
		N_("Add Watch"),                      
		NULL,                                    
		NULL,                                     
		G_CALLBACK (on_debug_tree_add_watch)     
	},
	{
		"ActionDmaRemoveWatch",
		NULL,
		N_("Remove Watch"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_remove_watch)
	},
	{
		"ActionDmaUpdateWatch",
		NULL,
		N_("Update Watch"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_update_watch)
	},
	{
		"ActionDmaEditWatch",
		NULL,
		N_("Change Value"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_edit_watch)
	},
	{
		"ActionDmaUpdateAllWatch",
		NULL,
		N_("Update all"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_update_all_watch)
	},
	{
		"ActionDmaRemoveAllWatch",
		NULL,
		N_("Remove all"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_remove_all_watch)
	}
};		

static GtkToggleActionEntry toggle_watch[] = {
	{
		"ActionDmaAutoUpdateWatch",               /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Automatic update"),                   /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		G_CALLBACK (on_debug_tree_auto_update_watch), /* action callback */
		FALSE                                     /* Initial state */
	}
};
	
/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

/* return a pointer to a newly allocated DebugTree object */
DebugTree *
debug_tree_new (AnjutaPlugin* plugin, gboolean user)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	AnjutaUI *ui;

	DebugTree *d_tree = g_new0 (DebugTree, 1);

	d_tree->plugin = plugin;
	
	model = GTK_TREE_MODEL (gtk_tree_store_new
						   (N_COLUMNS, 
	                        G_TYPE_STRING, 
	                        G_TYPE_STRING,
						    G_TYPE_POINTER));
	
	d_tree->tree = gtk_tree_view_new_with_model (model);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (d_tree->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (model));

	/* Columns */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",	VARIABLE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[0]));
	gtk_tree_view_append_column (GTK_TREE_VIEW (d_tree->tree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (d_tree->tree), column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
					debug_tree_cell_data_func, NULL, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[1]));
	gtk_tree_view_append_column (GTK_TREE_VIEW (d_tree->tree), column);

	/* Create popup menu */
	if (gCommonDebugTree == NULL)
	{
		gCommonDebugTree = g_new0 (CommonDebugTree, 1);
		
		ui = anjuta_shell_get_ui (d_tree->plugin->shell, NULL);
		gCommonDebugTree->action_group =
		      anjuta_ui_add_action_group_entries (ui, "ActionGroupWatch",
											_("Watch operations"),
											actions_watch,
											G_N_ELEMENTS (actions_watch),
											GETTEXT_PACKAGE, gCommonDebugTree);
		gCommonDebugTree->toggle_group =
		      anjuta_ui_add_toggle_action_group_entries (ui, "ActionGroupWatchToggle",
											_("Watch operations"),
											toggle_watch,
											G_N_ELEMENTS (toggle_watch),
											GETTEXT_PACKAGE, gCommonDebugTree);
	}
	gCommonDebugTree->initialized++;
	
	/* Allow adding watch in this tree*/
	if (user)
	{
		gCommonDebugTree->user = d_tree;
	}
	
	/* Connect signal */
	g_signal_connect (d_tree->tree, "button-press-event", G_CALLBACK (on_debug_tree_button_press), d_tree);  
	

	return d_tree;
}

/* DebugTree destructor */
void
debug_tree_free (DebugTree * d_tree)
{
	AnjutaUI *ui;
	
	g_return_if_fail (d_tree);

	debug_tree_remove_all (d_tree);
	
	/* Remove menu actions */
	if (gCommonDebugTree != NULL)
	{
		gCommonDebugTree->initialized--;
		if (gCommonDebugTree->initialized == 0)
		{
			ui = anjuta_shell_get_ui (d_tree->plugin->shell, NULL);
			anjuta_ui_remove_action_group (ui, gCommonDebugTree->action_group);
			anjuta_ui_remove_action_group (ui, gCommonDebugTree->toggle_group);
			g_free (gCommonDebugTree);
			gCommonDebugTree = NULL;
		}
	}
	if (d_tree->middle_click_menu != NULL)
	{
		g_object_unref (d_tree->middle_click_menu);
		gtk_widget_destroy (d_tree->middle_click_menu);
	}
	
	gtk_widget_destroy (d_tree->tree);
	g_free (d_tree);
}

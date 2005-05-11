/* 
 * debug_tree.c Copyright (C) 2002
 *        Etay Meiri            <etay-m@bezeqint.net>
 *        Jean-Noel Guiheneuf   <jnoel@saudionline.com.sa>
 *
 * Adapted from kdevelop - gdbparser.cpp  Copyright (C) 1999
 *        by John Birch         <jb.nz@writeme.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <libgnome/gnome-i18n.h>
#include <string.h>
#include <ctype.h>
#include "debug_tree.h"
#include "debugger.h"
#include "memory.h"

static void change_display_type (DebugTree *d_tree, gint display_type);
static void debug_ctree_cmd_gdb (DebugTree *d_tree, GtkTreeView *ctree,
								 GtkTreeIter *node,
								 GList *list, gint display_type,
								 gboolean is_pointer);
static gboolean find_expanded (GtkTreeModel* , GtkTreePath* , GtkTreeIter*, gpointer);
static gboolean debug_tree_on_select_row (GtkWidget *widget, GdkEvent *event,
										  gpointer user_data);
static void parse_data (GtkTreeView *ctree, GtkTreeIter *parent, gchar *buf);
static gchar *get_name (gchar **buf);
static void parse_array (GtkTreeView *ctree, GtkTreeIter *parent, gchar *buf);
static gchar* get_value (gchar **buf);
static gchar* skip_next_token_start (gchar *buf);
static gboolean is_long_array (GtkTreeView *ctree, GtkTreeIter *parent);
static gchar* skip_token_value (gchar *buf);
static gchar* skip_string (gchar *buf);
static void set_item (GtkTreeView *ctree, GtkTreeIter *parent,
					  const gchar *var_name, DataType dataType,
					  const gchar * value, gboolean long_array);
static void set_data (GtkTreeView * ctree, GtkTreeIter* node, DataType dataType,
					  const gchar * var_name, const gchar * value,
					  gboolean expandable, gboolean expanded,
					  gboolean analyzed);
static DataType determine_type (gchar *buf);
static gchar* skip_quotes (gchar *buf, gchar quotes);
static gchar* skip_delim (gchar *buf, gchar open, gchar close);
static gchar* skip_token_end (gchar *buf);
static gboolean destroy_recursive(GtkTreeModel *model, GtkTreePath *path,
								  GtkTreeIter* iter, gpointer pdata);

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

/* stores colors for variables */
/*
static GtkStyle *style_red;
static GtkStyle *style_normal;
*/

enum {
	VARIABLE_COLUMN,
	VALUE_COLUMN,
	DTREE_ENTRY_COLUMN,
	N_COLUMNS
};

static gchar *tree_title[] = {
	N_("Variable"), N_("Value")
};

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
		
	gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &node_data, -1);

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
	if (is_long_array (GTK_TREE_VIEW (tree), &parent))
		return FALSE;

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

static void
on_format_default_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_DEFAULT);
}

static void
on_format_binary_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_BINARY);
}

static void
on_format_octal_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_OCTAL);
}

static void
on_format_signed_decimal_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_SDECIMAL);
}

static void
on_format_unsigned_decimal_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_UDECIMAL);
}

static void
on_format_hex_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_HEX);
}

static void
on_format_char_clicked (GtkMenuItem * menu_item, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	change_display_type (d_tree, FORMAT_CHAR);
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
	gtk_tree_model_get (model, d_tree->cur_node, DTREE_ENTRY_COLUMN, &node_data, -1);
	
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
	gtk_tree_model_get (model, &child, DTREE_ENTRY_COLUMN, &data, -1);
	
	if (data->name[0] == '*')
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
		
		gtk_tree_model_get (model, &parent, DTREE_ENTRY_COLUMN, &data, -1);

		temp = child;
		child = parent;
		parent = temp;
		
		if (data->name[0] == '*')
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

static void
parse_pointer_cbs (Debugger *debugger, const GDBMIValue *mi_results,
				   const GList * list, gpointer user_data)
{
	GtkTreeStore *store;	
	gchar *pos = NULL;
	DataType data_type;
	TrimmableItem *data;
	gchar *full_output = NULL;
	gchar *t;
	gchar *question_mark = "?";
	GtkTreeModel *model;
	Parsepointer *parse;
	
	parse = (Parsepointer *)user_data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (parse->tree));
	store = GTK_TREE_STORE (model);
	
	if (!list) 
		gtk_tree_store_set(store,parse->node, VALUE_COLUMN, question_mark,-1);
	else
	{
		/* Concat the answers of gdb */
		pos = g_strdup ((gchar *) list->data);
		while ((list = g_list_next (list)))
		{
			t = pos;
			pos = g_strconcat (pos, (gchar *) list->data, NULL);
			g_free (t);
		}
		full_output = pos;
		/* g_print("PARSEPOINTER1 %s\n", pos); */
		if (pos[0] == '$')
		{
			if (!(pos = strchr ((gchar *) pos, '=')))
				g_warning ("Format error - cannot find '=' in %s\n",
						   full_output);
			else 
			{
				data_type = determine_type (pos);
				/* g_print("Type of %s is %s\n",pos, type_names[data_type]); */
				gtk_tree_model_get (model, parse->node,
									DTREE_ENTRY_COLUMN, &data, -1);
	
				switch (data_type)
				{
				case TYPE_ARRAY:
					/* Changing to array not supported */
					break;
				case TYPE_STRUCT:
					data->dataType = data_type;
					pos += 3;
					parse_data (parse->tree, parse->node, pos);
					break;
				case TYPE_POINTER:
				case TYPE_VALUE:
					if (parse->is_pointer)
					{
						t = full_output;
						full_output = pos = g_strconcat ("*", data->name,
														 pos, NULL);
						g_free (t);
						parse_data (parse->tree, parse->node, pos);
					}
					else
					{
						pos += 2;
						gtk_tree_store_set(store, parse->node,
										   VALUE_COLUMN, pos, -1);
					}
					break;
				default:
					parse_data (parse->tree, parse->node, pos);
					break;
				}
			}
		}
	}
	/* Send the next cmd to gdb - if exists */
	if (parse->next)
	{
		GtkTreeIter* iter = (GtkTreeIter*)parse->next->data;
		gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);			
		debug_ctree_cmd_gdb (parse->d_tree, parse->tree, iter, parse->next,
							 data->display_type, parse->is_pointer);
		gtk_tree_iter_free(iter);
	}
	g_free (full_output);
	gtk_tree_iter_free(parse->node);
	g_free (parse);
}

static void
debug_ctree_cmd_gdb (DebugTree *d_tree, GtkTreeView * ctree,
					 GtkTreeIter * node, GList * list,
					 gint display_type, gboolean is_pointer)
{
	gchar *full_name;
	Parsepointer *parse;
	gchar *comm;
	gchar *t;

	g_return_if_fail (ctree);
	g_return_if_fail (node);
	
	parse = g_new (Parsepointer, 1);
	parse->tree = ctree;
	parse->node = gtk_tree_iter_copy(node);
	parse->is_pointer = is_pointer;
	parse->d_tree = d_tree;
	
	if (list)
		parse->next = g_list_next (list);
	else
		parse->next = NULL;

	comm = DisplayCommands[display_type];

	/* extract full_name name */
	full_name = extract_full_name (ctree, node);

	if (is_pointer)
	{
		t = full_name;
		full_name = g_strconcat ("*", full_name, NULL);
		g_free (t);
	}

	t = full_name;
	full_name = g_strconcat (comm, full_name, NULL);
	g_free (t);
	//g_print("gdb comm %s\n", full_name); 
//	debugger_put_cmd_in_queqe ("set print pretty on", 0, NULL, NULL);
//	debugger_put_cmd_in_queqe ("set verbose off", 0, NULL, NULL);
	debugger_command (d_tree->debugger, full_name, FALSE,
					  parse_pointer_cbs, parse);
//	debugger_put_cmd_in_queqe ("set verbose on", 0, NULL, NULL);
//	debugger_put_cmd_in_queqe ("set print pretty off", 0, NULL, NULL);

	g_free (full_name);
	/* g_free (comm); */
}

#if 0
static void
on_debug_tree_row_expanded (GtkTreeView * ctree, GtkTreeIter* iter,
							GtkTreePath* path, gpointer data)
{
	GList *expanded_list = NULL;
	GtkTreeModel* model;	
	
	g_return_if_fail (ctree);
	g_return_if_fail (iter);

	model = gtk_tree_view_get_model (ctree);
	
	/* Search expanded */
	gtk_tree_model_foreach(model,find_expanded,&expanded_list);	

	if (expanded_list)
	{
		TrimmableItem *data;
		GtkTreeIter* iter = (GtkTreeIter*)expanded_list->data;
		gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);			
		debug_ctree_cmd_gdb (ctree,iter,expanded_list,data->display_type,TRUE);
		gtk_tree_iter_free(iter);
	}
}
#endif

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
	gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
	
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
			debug_ctree_cmd_gdb ((DebugTree*)user_data, view, &iter,
								  NULL, FORMAT_DEFAULT, TRUE);
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

static void
parse_data (GtkTreeView* ctree, GtkTreeIter* parent, gchar * buf)
{
	TrimmableItem *item;
	gchar *var_name = NULL;
	gchar *value = NULL;
	DataType dataType;
	GtkTreeModel *model;

	g_return_if_fail (parent);
	g_return_if_fail (ctree);

	if (!buf)
		return;

	model = gtk_tree_view_get_model (ctree);
	gtk_tree_model_get (model, parent, DTREE_ENTRY_COLUMN, &item, -1);	

	if (item->dataType == TYPE_ARRAY)
	{
		parse_array (ctree, parent, buf);
		return;
	}

	while (*buf)
	{
		var_name = value = NULL;
		dataType = determine_type (buf);
		if (dataType == TYPE_NAME)
		{
			var_name = get_name (&buf);
			if (!var_name)
				break;
			dataType = determine_type (buf);
		}
		value = get_value (&buf);
		if (!value)
		{
			g_free (var_name);
			break;
		}
		set_item (ctree, parent, var_name, dataType, value, FALSE);
		g_free (var_name);
		g_free (value);
	}
}

static gboolean
is_long_array (GtkTreeView * ctree, GtkTreeIter* parent)
{
	gchar *text;
	GtkTreeModel* model;
	GtkTreeIter iter;
	gboolean success;

	g_return_val_if_fail (ctree, FALSE);
	g_return_val_if_fail (parent, TRUE);
	
	model = gtk_tree_view_get_model (ctree);
	success = gtk_tree_model_iter_children(model,&iter,parent);
	while (success)
	{
		gtk_tree_model_get(model, &iter, VALUE_COLUMN, &text, -1);
		if (text)
		{
			if (strstr (text, "<repeats"))
			{
				g_free (text);
				return TRUE;
			}
			g_free (text);
		}
		else
		{
			g_warning("Error getting value\n");
		}
		success = gtk_tree_model_iter_next(model, &iter);
	}
	return FALSE;
}

/* FIXME : Analyse of long multidimensionnal arrays */
/* if the long .. is not the last dimension !!!! */
static void
parse_array (GtkTreeView* ctree, GtkTreeIter* parent, gchar * buf)
{
	TrimmableItem *item;
	gchar *element_root;
	gchar *value = NULL;
	gchar *var_name = NULL;
	DataType dataType;
	gint idx;
	gchar *pos;
	gint i;
	gboolean long_array;
	GtkTreeModel *model;
	
	g_return_if_fail (ctree);
	g_return_if_fail (parent);

	model = gtk_tree_view_get_model (ctree);
	gtk_tree_model_get (model, parent, DTREE_ENTRY_COLUMN, &item, -1);	

	element_root = item->name;
	idx = 0;

	while (*buf)
	{
		buf = skip_next_token_start (buf);
		if (!*buf)
			return;

		dataType = determine_type (buf);
		value = get_value (&buf);

		var_name = g_strdup_printf ("%s [%d]", element_root, idx);
		long_array = is_long_array (ctree, parent);
		set_item (ctree, parent, var_name, dataType, value, long_array);

		pos = strstr (value, " <repeats");
		if (pos)
		{
			if ((i = atoi (pos + 10)))
				idx += (i - 1);
		}
		idx++;
		g_free (var_name);
		g_free (value);
	}
}

static gchar *
get_name (gchar **buf)
{
	gchar *start;

	start = skip_next_token_start (*buf);
	if (*start)
	{
		gchar *t;

		*buf = skip_token_value (start);
		t = *buf;
		if (*t == '=')
			t--;
		return g_strstrip (g_strndup (start, t - start + 1));
	}
	else
		*buf = start;

	return NULL;
}

static gchar *
get_value (gchar **buf)
{
	gchar *start;
	gchar *value;

	/* g_print("get_value: %s\n",*buf); */

	start = skip_next_token_start (*buf);
	*buf = skip_token_value (start);

	if (*start == '{')
		return g_strstrip (g_strndup (start + 1, *buf - start - 1));
	if (*buf == start)
		return NULL;
	value = g_strstrip (g_strndup (start, *buf - start));

	return value;
}

static void
set_data (GtkTreeView * ctree, GtkTreeIter* iter, DataType dataType,
		  const gchar * var_name, const gchar * value, gboolean expandable,
		  gboolean expanded, gboolean analyzed)
{
	TrimmableItem *data;
	GtkTreeModel* model;

	g_return_if_fail (ctree);
	g_return_if_fail (iter);

	model = gtk_tree_view_get_model (ctree);	
	
	/* get node private data */
	gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);

	/* there was a private data object - delete old data */
	if (data)
	{
		g_free (data->name);
		g_free (data->value);
	}
	else		/* first time - allocate private data object */
	{
		data = g_new (TrimmableItem, 1);
		data->display_type = FORMAT_DEFAULT;
		data->modified = FALSE;
		gtk_tree_store_set(GTK_TREE_STORE(model), iter,
						   DTREE_ENTRY_COLUMN, data, -1);		
	}
	data->name = g_strdup (var_name);
	data->value = g_strdup (value);
	data->dataType = dataType;
	data->expandable = expandable;
	data->expanded = expanded;
	data->analyzed = analyzed;
}

static void
set_item (GtkTreeView* ctree, GtkTreeIter* parent, const gchar * var_name,
		  DataType dataType, const gchar * value, gboolean long_array)
{
	GtkTreeIter iter;
	TrimmableItem *data = NULL;
	//GtkStyle *style;
	gboolean expanded = FALSE;
	GtkTreeModel* model;
	gboolean success, found = FALSE;

	g_return_if_fail (ctree);

	if (!var_name || !*var_name)
		return;
	
	//g_print("Setting variable %s with value %s\n",var_name, value);

	model = gtk_tree_view_get_model (ctree);
	success = gtk_tree_model_iter_children(model,&iter,parent);

	/* find the child of the given parent with the given name */
	while (success)
	{
		gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &data, -1);			
		if (data && g_strcasecmp (var_name, data->name) == 0) {
			found = TRUE;
			break;
		}
		success = gtk_tree_model_iter_next(model, &iter);
	}

	/* child found - update value and change color if value changed */
	if (found)
	{
		// g_print("Variable %s found - updating\n",var_name);
		/* Destroy following items if long array */
		/* x <repeats yy times> */
		if (long_array)
		{
			GtkTreeIter* iter2 = gtk_tree_iter_copy(&iter);
			success = gtk_tree_model_iter_next(model, iter2);
			while (success)
			{
				destroy_recursive (model, NULL,iter2, NULL);
				success = gtk_tree_model_iter_next(model, iter2);
			}
		}
		if (dataType != TYPE_ARRAY && dataType != TYPE_STRUCT)
		{
			gchar *val = g_strdup (value);	/* copy value - orig to be
											 * deleted by caller */
			/* Set red color if var modified */
			data->modified =
					(g_strcasecmp (value, data->value) != 0) ? TRUE : FALSE;
			gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
								   VALUE_COLUMN, val,-1);
		}
		expanded = TRUE;
	}
	else		/* child not found - insert it */
	{
		char* var = NULL;
		char* val = NULL;
		
		/* g_print("Variable %s not found - inserting\n",var_name); */

		var = g_strdup(var_name);

		if (dataType == TYPE_ARRAY || dataType == TYPE_STRUCT)
			val = g_strdup ("");
		else
			val = g_strdup (value);

		gtk_tree_store_append(GTK_TREE_STORE(model), &iter, parent);
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 
						   VARIABLE_COLUMN, var, 
						   VALUE_COLUMN, val, -1);
	}
	switch (dataType)
	{
	case TYPE_POINTER:
		//g_print ("POINTER %s\n", var_name);
		set_data (ctree, &iter, TYPE_POINTER, var_name, value,
				  TRUE, expanded && data->expanded, TRUE);
		break;

	case TYPE_STRUCT:
		set_data (ctree, &iter, TYPE_STRUCT, var_name, value,
				  FALSE, FALSE,TRUE);
		//g_print ("STRUCT %s\n", var_name);
		parse_data (ctree, &iter, (gchar *) value);
		break;

	case TYPE_ARRAY:
		set_data (ctree, &iter, TYPE_ARRAY, var_name, value,
				  FALSE, FALSE, TRUE);
		//g_print ("ARRAY %s\n", var_name);
		parse_data (ctree, &iter, (gchar *) value);
		break;

	case TYPE_REFERENCE:
		/* Not implemented yet */
		break;

	case TYPE_VALUE:
		set_data (ctree, &iter, TYPE_VALUE, var_name, value,
				  FALSE, FALSE, TRUE);
		//g_print ("VALUE %s\n", var_name);
		break;

	default:
		g_warning ("Not setting data for unknown: var [%s] = value[%s]\n",
				   var_name, value);
		break;
	}
}

static DataType
determine_type (gchar * buf)
{
	if (!buf || !*(buf = skip_next_token_start (buf)))
		return TYPE_UNKNOWN;

	// A reference, probably from a parameter value.
	if (*buf == '@')
		return TYPE_REFERENCE;

	// Structures and arrays - (but which one is which?)
	// {void (void)} 0x804a944 <__builtin_new+41> - this is a fn pointer
	// (void (*)(void)) 0x804a944 <f(E *, char)> - so is this - ugly!!!
	if (*buf == '{')
	{
		if (g_strncasecmp (buf, "{{", 2) == 0)
			return TYPE_ARRAY;

		if (g_strncasecmp (buf, "{<No data fields>}", 18) == 0)
			return TYPE_VALUE;

		buf++;
		while (*buf)
		{
			switch (*buf)
			{
			case '=':
				return TYPE_STRUCT;
			case '"':
				buf = skip_string (buf);
				break;
			case '\'':
				buf = skip_quotes (buf, '\'');
				break;
			case ',':
				if (*(buf - 1) == '}')
				{
					g_warning ("??????\n");
				}
				return TYPE_ARRAY;
			case '}':
				if (*(buf + 1) == ',' || *(buf + 1) == '\n' || !*(buf + 1))
					return TYPE_ARRAY;			 // Hmm a single element
												 // array??
				if (g_strncasecmp (buf + 1, " 0x", 3) == 0)
					return TYPE_POINTER;		 // What about references?
				return TYPE_UNKNOWN;			 // very odd?
			case '(':
				buf = skip_delim (buf, '(', ')');
				break;
			case '<':
				buf = skip_delim (buf, '<', '>');
				break;
			default:
				buf++;
				break;
			}
		}
		return TYPE_UNKNOWN;
	}

	// some sort of address. We need to sort out if we have
	// a 0x888888 "this is a char*" type which we'll term a value
	// or whether we just have an address
	if (g_strncasecmp (buf, "0x", 2) == 0)
	{
		while (*buf)
		{
			if (!isspace (*buf))
				buf++;
			else if (*(buf + 1) == '\"')
				return TYPE_VALUE;
			else
				break;
		}
		return TYPE_POINTER;
	}

	// Pointers and references - references are a bit odd
	// and cause GDB to fail to produce all the local data
	// if they haven't been initialised. but that's not our problem!!
	// (void (*)(void)) 0x804a944 <f(E *, char)> - this is a fn pointer
	if (*buf == '(')
	{
		buf = skip_delim (buf, '(', ')');

		switch (*(buf - 2))
		{
		case ')':
		case '*':
			return TYPE_POINTER;
		case '&':
			return TYPE_REFERENCE;
		default:
			return TYPE_UNKNOWN;
		}
	}

	buf = skip_token_value (buf);
	if ((g_strncasecmp (buf, " = ", 3) == 0) || (*buf == '='))
		return TYPE_NAME;

	return TYPE_VALUE;
}

static gchar *
skip_string (gchar *buf)
{
	if (buf && *buf == '\"')
	{
		buf = skip_quotes (buf, *buf);
		while (*buf)
		{
			if ((g_strncasecmp (buf, ", \"", 3) == 0) ||
				(g_strncasecmp (buf, ", '", 3) == 0))
				buf = skip_quotes (buf + 2, *(buf + 2));

			else if (g_strncasecmp (buf, " <", 2) == 0)	// take care of
														// <repeats
				buf = skip_delim (buf + 1, '<', '>');
			else
				break;
		}

		// If the string is long then it's chopped and has ... after it.
		while (*buf && *buf == '.')
			buf++;
	}
	return buf;
}

static gchar *
skip_quotes (gchar *buf, gchar quotes)
{
	if (buf && *buf == quotes)
	{
		buf++;

		while (*buf)
		{
			if (*buf == '\\')
				buf++; // skips \" or \' problems
			else if (*buf == quotes)
				return buf + 1;

			buf++;
		}
	}
	return buf;
}

static gchar *
skip_delim (gchar * buf, gchar open, gchar close)
{
	if (buf && *buf == open)
	{
		buf++;

		while (*buf)
		{
			if (*buf == open)
				buf = skip_delim (buf, open, close);
			else if (*buf == close)
				return buf + 1;
			else if (*buf == '\"')
				buf = skip_string (buf);
			else if (*buf == '\'')
				buf = skip_quotes (buf, *buf);
			else if (*buf)
				buf++;
		}
	}
	return buf;
}

static gchar *
skip_token_value (gchar * buf)
{
	gchar *end;

	if (!buf)
		return NULL;
	while (TRUE)
	{
		buf = skip_token_end (buf);

		end = buf;
		while (*end && isspace (*end) && *end != '\n')
			end++;

		if (*end == 0 || *end == ',' || *end == '\n' || *end == '=' ||
			*end == '}')
			break;

		if (buf == end)
			break;

		buf = end;
	}
	return buf;
}

static gchar *
skip_token_end (gchar * buf)
{
	if (buf)
	{
		switch (*buf)
		{
		case '"':
			return skip_string (buf);
		case '\'':
			return skip_quotes (buf, *buf);
		case '{':
			return skip_delim (buf, '{', '}');
		case '<':
			return skip_delim (buf, '<', '>');
		case '(':
			return skip_delim (buf, '(', ')');
		}

		while (*buf && !isspace (*buf) && *buf != ',' && *buf != '}' &&
			   *buf != '=')
			buf++;
	}
	return buf;
}

static gchar *
skip_next_token_start (gchar * buf)
{
	if (!buf)
		return NULL;

	while (*buf &&
		   (isspace (*buf) || *buf == ',' || *buf == '}' || *buf == '='))
		buf++;

	return buf;
}

static void
debug_tree_init (DebugTree * d_tree)
{
	GtkTreeModel* model;
	GtkTreeIter iter;
	gchar* var_name = _("Local Variables");
	static const gchar* value = "";

	g_return_if_fail (d_tree);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (d_tree->tree));		
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, VARIABLE_COLUMN, var_name
						, VALUE_COLUMN, value, -1);
	
	set_data (GTK_TREE_VIEW(d_tree->tree), &iter, TYPE_ROOT, "", "", FALSE,
			  FALSE, TRUE);
}

static gboolean
set_not_analyzed(GtkTreeModel *model, GtkTreePath* path,
				 GtkTreeIter* iter, gpointer pdata)
{
	TrimmableItem *data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);
	
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);

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
	
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	
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
		gtk_tree_model_get(model, &iter, DTREE_ENTRY_COLUMN, &data, -1);

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


static gboolean
find_expanded (GtkTreeModel *model, GtkTreePath* path,
			   GtkTreeIter* iter, gpointer pdata)
{
	TrimmableItem *data;
	GList ** list = pdata;
	
	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	

	if (data && data->expanded)
	{
		//g_print("Appending %s to the expanded list\n",data->name);
		*list = g_list_prepend (*list, gtk_tree_iter_copy(iter));
	}
	return FALSE;
}

static gboolean
delete_node(GtkTreeModel *model, GtkTreePath* path,
			GtkTreeIter* iter, gpointer pdata)
{
	TrimmableItem *data;
	
	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	

	if (data)
	{
		//g_print("Removing node",data->name);
		g_free (data->name);
		g_free (data->value);
		g_free (data);
	}
	else
		g_warning("Cannot get data entry\n");

	return FALSE;
}


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
		gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);
		debug_ctree_cmd_gdb (d_tree, tree, iter, list,
							 data->display_type, TRUE);
		gtk_tree_iter_free(iter);
	}
}


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
	g_free (buf);
}

/* parse debugger output into the debug tree */
/* param: d_tree - debug tree object */
/* param: list - debugger output */
void
debug_tree_parse_variables (DebugTree *d_tree, const GList * list)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreePath* path;
	
	g_return_if_fail (d_tree);
	g_return_if_fail (list);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(d_tree->tree));
	
	/* Mark variables as not analyzed */
	gtk_tree_model_foreach(model,set_not_analyzed,NULL);	
	gtk_tree_model_get_iter_first(model,&iter);	
	while (list)
	{
		parse_data (GTK_TREE_VIEW(d_tree->tree), &iter, (gchar *) list->data);
		list = g_list_next (list);
	}

	/* Destroy non used variables (non analyzed) */
	destroy_non_analyzed(model, &iter);

	/* Recursive analyze of Pointers */
	debug_tree_pointer_recursive (d_tree, GTK_TREE_VIEW(d_tree->tree));
	
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_expand_row(GTK_TREE_VIEW(d_tree->tree),path,FALSE);
	gtk_tree_path_free(path);
}

static void
debug_tree_cell_data_func (GtkTreeViewColumn *tree_column,
					GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data)
{
	gchar *value;
	static const gchar *colors[] = {"black", "red"};
	GValue gvalue = {0, };
	TrimmableItem *item_data;

	gtk_tree_model_get (tree_model, iter, VALUE_COLUMN, &value, -1);
	g_value_init (&gvalue, G_TYPE_STRING);
	g_value_set_static_string (&gvalue, value);
	g_object_set_property (G_OBJECT (cell), "text", &gvalue);

	gtk_tree_model_get (tree_model, iter, DTREE_ENTRY_COLUMN, &item_data, -1);
	g_value_reset (&gvalue);
	/* fix me : 'item_data' can be NULL */
	g_value_set_static_string (&gvalue, colors[(item_data && item_data->modified ? 1 : 0)]);

	g_object_set_property (G_OBJECT (cell), "foreground", &gvalue);
	g_free (value);
}

/* return a pointer to a newly allocated DebugTree object */
DebugTree *
debug_tree_create (Debugger *debugger)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	DebugTree *d_tree = g_malloc (sizeof (DebugTree));

	d_tree->debugger = debugger;
	
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

	debug_tree_init (d_tree);

	g_signal_connect (d_tree->tree, "event", 
					  G_CALLBACK (debug_tree_on_select_row), d_tree);

	/* build middle click popup menu */
	
	d_tree->middle_click_menu = gtk_menu_new ();
	build_menu_item (_("Default format"),
					 GTK_SIGNAL_FUNC(on_format_default_clicked),
					 d_tree->middle_click_menu, d_tree);
	add_menu_separator (d_tree->middle_click_menu);
	build_menu_item (_("Binary"),
					 GTK_SIGNAL_FUNC(on_format_binary_clicked),
					 d_tree->middle_click_menu, d_tree);
	build_menu_item (_("Octal"),
					 GTK_SIGNAL_FUNC(on_format_octal_clicked),
					 d_tree->middle_click_menu, d_tree);
	build_menu_item (_("Signed decimal"),
					 GTK_SIGNAL_FUNC(on_format_signed_decimal_clicked),
					 d_tree->middle_click_menu, d_tree);
	build_menu_item (_("Unsigned decimal"),
					 GTK_SIGNAL_FUNC(on_format_unsigned_decimal_clicked),
					 d_tree->middle_click_menu, d_tree);
	build_menu_item (_("Hex"),
					 GTK_SIGNAL_FUNC(on_format_hex_clicked), 
					 d_tree->middle_click_menu, d_tree);
	build_menu_item (_("Char"),
					 GTK_SIGNAL_FUNC(on_format_char_clicked),
					 d_tree->middle_click_menu, d_tree);

	add_menu_separator (d_tree->middle_click_menu);
	build_menu_item (_("Inspect memory"),
					 GTK_SIGNAL_FUNC(on_inspect_memory_clicked),
					 d_tree->middle_click_menu, d_tree);
	
	return d_tree;
}

/* DebugTree destructor */
void
debug_tree_destroy (DebugTree * d_tree)
{
	GtkTreeModel* model;
	
	g_return_if_fail (d_tree);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (d_tree->tree));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	gtk_widget_destroy (d_tree->tree);
	gtk_widget_destroy (d_tree->middle_click_menu);
	g_free (d_tree);
}

/* clear the display of the debug tree and reset the title */
void
debug_tree_clear (DebugTree * d_tree)
{
	GtkTreeModel *model;

	g_return_if_fail (d_tree);
	g_return_if_fail(d_tree->tree);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (d_tree->tree));
	gtk_tree_model_foreach(model,delete_node,NULL);	
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	debug_tree_init (d_tree);
}

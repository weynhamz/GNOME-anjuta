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

#include <string.h>
#include <ctype.h>
#include "debug_tree.h"
#include "debugger.h"
#include "memory.h"

#define MAX_BUFFER 10000

#define FORMAT_DEFAULT  0       /* gdb print command without switches */
#define FORMAT_BINARY   0x1     /* print/t */
#define FORMAT_OCTAL    0x2     /* print/o */
#define FORMAT_SDECIMAL 0x4     /* print/d */
#define FORMAT_UDECIMAL 0x8     /* print/u */
#define FORMAT_HEX      0x10    /* print/x */
#define FORMAT_CHAR     0x20    /* print/c */

struct parse_private_data {
        DebugTree* d_tree;
        GtkCTreeNode* node;
};


gchar* type_names[] = { "Root" , 
			"Unknown" , 
			"Pointer", 
			"Array" , 
			"Struct", 
			"Value",
			"Reference",
			"Name" };

			
static void show_hide_popup_menu_items(GtkWidget *menu, gint start, gint end, gboolean sensitive);					
static gboolean debug_tree_on_middle_click(GtkWidget* widget, GdkEvent* event,
                                          gpointer data);
static void on_format_default_clicked(GtkMenuItem* menu_item, gpointer data);
static void on_format_binary_clicked(GtkMenuItem* menu_item, gpointer data);
static void on_format_octal_clicked(GtkMenuItem* menu_item, gpointer data);
static void on_format_signed_decimal_clicked(GtkMenuItem* menu_item, gpointer data);
static void on_format_unsigned_decimal_clicked(GtkMenuItem* menu_item, 
	gpointer data);
static void on_format_hex_clicked(GtkMenuItem* menu_item, gpointer data);
static void on_format_char_clicked(GtkMenuItem* menu_item, gpointer data);
static void change_display_type(DebugTree* d_tree, gint display_type);
static gchar* get_display_command(gint display_mask);
static GtkWidget *build_menu_item (gchar* menutext, GtkSignalFunc signalhandler,
                           GtkWidget* menu, gpointer data);
static void add_menu_separator(GtkWidget* menu);
static void debug_tree_init (DebugTree * d_tree);
static gchar* extract_full_name(GtkCTree *ctree, GtkCTreeNode *node);
static void parse_pointer_cbs (GList* list, Parsepointer *parse);
static void debug_ctree_cmd_gdb(GtkCTree * ctree, GtkCTreeNode *node, GList *next,
	gint display_type, gboolean is_pointer);
static void debug_ctree_on_select_row (GtkCList * list, gint row, gint column,
                                GdkEvent * event, GtkCTree * ctree);
static void debug_ctree_tree_expand (GtkCTree *ctree, GList *node, 
		gpointer user_data);
static void parse_data(GtkCTree * ctree, GtkCTreeNode * parent, gchar * buf);
static gboolean is_long_array(GtkCTree *ctree, GtkCTreeNode *parent);
static void parse_array (GtkCTree * ctree, GtkCTreeNode * parent, gchar * buf);
static gchar *get_name (gchar **buf);
static gchar *get_value (gchar **buf);
static void set_data (GtkCTree * ctree, GtkCTreeNode * node, DataType dataType,
         const gchar * var_name, const gchar * value,
         gboolean expandable, gboolean expanded, gboolean analyzed);
static void set_item (GtkCTree * ctree, GtkCTreeNode * parent,
              const gchar * var_name, DataType dataType,
              const gchar * value, gboolean long_array);
static DataType determine_type (char *buf);
static gchar *skip_string (gchar * buf);
static gchar *skip_quotes (gchar * buf, gchar quotes);
static gchar *skip_delim (gchar * buf, gchar open, gchar close);
static gchar *skip_token_value (gchar * buf);
static gchar *skip_token_end (gchar * buf);
static gchar *skip_next_token_start (gchar * buf);
static void init_style (GtkCTree * ctree);
static void debug_init (DebugTree *d_tree);
static void set_not_analyzed(GtkCTree * ctree, GtkCTreeNode *node);
static void destroy_recursive (GtkCTree *ctree, GtkCTreeNode *node, 
	gpointer user_data);
static void destroy_non_analyzed(GtkCTree * ctree, GtkCTreeNode *node);
static void find_expanded (GtkCTree *ctree,  GtkCTreeNode *node, GList **list);
static void debug_tree_pointer_recursive(GtkCTree *ctree, GtkCTreeNode *node);
static void on_inspect_memory_clicked(GtkMenuItem* menu_item, gpointer data);

/* stores colors for variables */
static GtkStyle *style_red;
static GtkStyle *style_normal;


/* return a pointer to a newly allocated DebugTree object */
/* @param: container - container for new object */
/* @param: title - title of tree root */
DebugTree * debug_tree_create (GtkWidget * container)
{
	gchar *tree_title[] = { "Variable", "Value" };
	DebugTree *d_tree = g_malloc (sizeof (DebugTree));
	d_tree->tree = gtk_ctree_new_with_titles (2, 0, tree_title);

	gtk_widget_show (d_tree->tree);
	gtk_clist_set_column_auto_resize (GTK_CLIST (d_tree->tree), 0, TRUE);
	gtk_clist_set_column_width (GTK_CLIST (d_tree->tree), 1, 200);
	gtk_ctree_set_line_style (GTK_CTREE (d_tree->tree),
				  GTK_CTREE_LINES_DOTTED);
	gtk_container_add (GTK_CONTAINER (container), d_tree->tree);
	debug_tree_init (d_tree);
	gtk_signal_connect (GTK_OBJECT (d_tree->tree), "select_row",
			    GTK_SIGNAL_FUNC (debug_ctree_on_select_row),
			    GTK_CTREE(d_tree->tree));
	gtk_signal_connect_after (GTK_OBJECT (d_tree->tree), "tree_expand",
                      GTK_SIGNAL_FUNC (debug_ctree_tree_expand),
                      NULL);
        gtk_signal_connect(GTK_OBJECT(d_tree->tree), "event",
                                GTK_SIGNAL_FUNC(debug_tree_on_middle_click),
                                d_tree);

        /* build middle click popup menu */
        d_tree->middle_click_menu = gtk_menu_new();
        build_menu_item(_("Default format"),on_format_default_clicked,
                d_tree->middle_click_menu,d_tree);
        add_menu_separator(d_tree->middle_click_menu);
        build_menu_item(_("Binary"),on_format_binary_clicked,
                d_tree->middle_click_menu,d_tree);
        build_menu_item(_("Octal"),on_format_octal_clicked,
                d_tree->middle_click_menu,d_tree);
        build_menu_item(_("Signed decimal"),on_format_signed_decimal_clicked,
                d_tree->middle_click_menu,d_tree);
        build_menu_item(_("Unsigned decimal"),on_format_unsigned_decimal_clicked,
                d_tree->middle_click_menu,d_tree);
        build_menu_item(_("Hex"),on_format_hex_clicked,
                d_tree->middle_click_menu,d_tree);
        build_menu_item(_("Char"),on_format_char_clicked,
		d_tree->middle_click_menu,d_tree);
	add_menu_separator(d_tree->middle_click_menu);
        build_menu_item(_("Inspect memory"),on_inspect_memory_clicked,
                d_tree->middle_click_menu,d_tree);

	return d_tree;
}


/* DebugTree destructor */
void debug_tree_destroy (DebugTree * d_tree)
{
	g_return_if_fail(d_tree);
	
	gtk_widget_destroy (d_tree->tree);
	g_free (d_tree);
}


/* clear the display of the debug tree and reset the title */
void debug_tree_clear (DebugTree * d_tree)
{
	g_return_if_fail (d_tree && d_tree->tree);
	gtk_clist_clear (GTK_CLIST (d_tree->tree));
	debug_tree_init (d_tree);
}


/* parse debugger output into the debug tree */
/* param: d_tree - debug tree object */
/* param: list - debugger output */
void debug_tree_parse_variables (DebugTree *d_tree, GList *list)
{
  g_return_if_fail (d_tree);

  debug_init (d_tree);
	
  /*   Mark variables as not analyzed   */
  set_not_analyzed(GTK_CTREE(d_tree->tree), d_tree->root);

  while (list)
  {
    parse_data(GTK_CTREE(d_tree->tree), d_tree->root, (gchar*) list->data);
    list = g_list_next(list);
  }

  /*   Destroy non used variables (non analyzed)   */
  destroy_non_analyzed(GTK_CTREE(d_tree->tree), d_tree->root);

  /*   Recursive analyze of Pointers   */
  debug_tree_pointer_recursive(GTK_CTREE(d_tree->tree), d_tree->root);


  gtk_ctree_expand (GTK_CTREE(d_tree->tree), d_tree->root);
}


/* build a menu item for the middle-click button
   @param menutext text to display on the menu item
   @param signalhandler signal handler for item selection
   @param menu menu the item belongs to 
   @param data private data for the signal handler */
static
GtkWidget *build_menu_item (gchar* menutext, GtkSignalFunc signalhandler,
                           GtkWidget* menu, gpointer data)
{
        GtkWidget *menuitem;

        if (menutext)
                menuitem = gtk_menu_item_new_with_label (menutext);
        else
                menuitem = gtk_menu_item_new ();

        if (signalhandler)
                gtk_signal_connect (GTK_OBJECT (menuitem),
                           "activate", signalhandler, data);

        if (menu)
                gtk_menu_append (GTK_MENU (menu), menuitem);

        return menuitem;
}

/* add a separator to a menu */
static
void add_menu_separator(GtkWidget* menu)
{
        GtkWidget *menuitem;
	
	g_return_if_fail (menu);
	
        menuitem = gtk_menu_item_new ();
        gtk_container_add(GTK_CONTAINER(menuitem),gtk_hseparator_new());

        gtk_menu_append (GTK_MENU (menu), menuitem);
}

static void
show_hide_popup_menu_items(GtkWidget *menu, gint start, gint end, gboolean sensitive)
{
   GtkMenuShell *menu_shell;
   GList *list;
   gint nb = 0;

   g_return_if_fail (menu);
   g_return_if_fail (start <= end);
 
   menu_shell = GTK_MENU_SHELL(menu);
   list = menu_shell->children;

   while (list)
   {
      if (nb >= start && nb <= end)
        gtk_widget_set_sensitive (GTK_WIDGET(list->data), sensitive);
      list = g_list_next(list);
      nb++;
   }
}

/* middle click call back for bringing up the format menu */
static
gboolean debug_tree_on_middle_click(GtkWidget* widget,
                                   GdkEvent* event,
                                   gpointer data)
{
        GdkEventButton* buttonevent = NULL;
        DebugTree* d_tree = NULL;
        GtkCTree* tree = NULL;
        gint row;
        GtkCTreeNode* node = NULL;
        TrimmableItem* node_data = NULL;

        /* only mouse press events */
        if (event->type != GDK_BUTTON_PRESS)
                return FALSE;

        buttonevent = (GdkEventButton*)event;

        /* only middle mouse button */
        if (buttonevent->button != 2)
                return FALSE;

        /* get the selected row */
        d_tree = (DebugTree*)data;
        tree = GTK_CTREE(d_tree->tree);
        row = tree->clist.focus_row;

        /* extract node */
        node = gtk_ctree_node_nth (GTK_CTREE (d_tree->tree), row);
        
		/* Debug assertions should not be used where the event is bound 
		 * to happen in real situations. Instead use conditional statements.
		 * g_return_val_if_fail (node != NULL, FALSE);
		*/
		if (node == NULL)
			return FALSE;
		
        node_data = gtk_ctree_node_get_row_data (GTK_CTREE (d_tree->tree),
                                                     node);

        /* only for items with 'real' data */
	if ( !node_data || node_data->dataType == TYPE_ROOT)
                 return FALSE;
	
	/* Do not allow long arrays */
        if (is_long_array(GTK_CTREE(d_tree->tree), GTK_CTREE_ROW(node)->parent))
           return FALSE;
	
	if (node_data->dataType == TYPE_VALUE)
        {
          show_hide_popup_menu_items(d_tree->middle_click_menu, 0, 9, TRUE);
        }
        else
        {
          show_hide_popup_menu_items(d_tree->middle_click_menu, 0, 7, FALSE);
          show_hide_popup_menu_items(d_tree->middle_click_menu, 8, 9, TRUE);
        }
	
        d_tree->cur_node = node;

        /* ok - show the menu */
        gtk_widget_show_all(GTK_WIDGET(d_tree->middle_click_menu));
        gtk_menu_popup(GTK_MENU(d_tree->middle_click_menu),
                NULL,NULL,NULL,NULL,buttonevent->button,0);

        return TRUE;
}

static
void on_format_default_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_DEFAULT);
}

static
void on_format_binary_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_BINARY);
}

static
void on_format_octal_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_OCTAL);
}

static
void on_format_signed_decimal_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_SDECIMAL);
}

static
void on_format_unsigned_decimal_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_UDECIMAL);
}

static
void on_format_hex_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_HEX);
}

static
void on_format_char_clicked(GtkMenuItem* menu_item, gpointer data)
{
        DebugTree* d_tree = (DebugTree*)data;
        change_display_type(d_tree,FORMAT_CHAR);
}

/* changes the display format of a node */
static
void change_display_type(DebugTree* d_tree,gint display_type)
{
        TrimmableItem *node_data = NULL;
	
	g_return_if_fail (d_tree);
        
	/* get the node's private data */
        node_data = gtk_ctree_node_get_row_data(GTK_CTREE(d_tree->tree),
                                                d_tree->cur_node);
        g_return_if_fail(node_data != NULL);

        /* if the display type did not change - skip the operation altogether */
        if (node_data->display_type == display_type)
                return;

        /* store the new display type in the node's private data */
        node_data->display_type = display_type;

	debug_ctree_cmd_gdb(GTK_CTREE(d_tree->tree),d_tree->cur_node, 
		NULL, display_type, FALSE);
}

/* return the correct display command according
   to the display type. caller must free the returned
   string! */
static
gchar* get_display_command(gint display_mask)
{
        gchar* s = NULL;

        switch (display_mask) {
		case FORMAT_DEFAULT:
			s = g_strdup("print ");
			break;
                case FORMAT_BINARY:
                        s = g_strdup("print/t ");
                        break;
                case FORMAT_OCTAL:
                        s = g_strdup("print/o ");
                        break;
                case FORMAT_SDECIMAL:
                        s = g_strdup("print/d ");
                        break;
                case FORMAT_UDECIMAL:
                        s = g_strdup("print/u ");
                        break;
                case FORMAT_HEX:
                        s = g_strdup("print/x ");
                        break;
                case FORMAT_CHAR:
                        s = g_strdup("print/c ");
                        break;
                default:
                        g_warning("Warning! unknown display type: %d\n",display_mask);
			s = g_strdup("print ");
        }

        return s;
}



/* initializes an existing debug tree object. */
static 
void debug_tree_init (DebugTree * d_tree)
{
	d_tree->root = NULL;
}


/* return full name of the supplied node 
   caller must free the returned string */
static 
gchar* extract_full_name(GtkCTree *ctree, GtkCTreeNode *node)
{
  TrimmableItem *data;
  gchar *full_name = NULL;
  gint pointer_count = 0;
  gint i;
  gboolean first = TRUE;
  gchar* t;
	
  data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) node);
  if (data->name[0] == '*')
     pointer_count++;
  else 
  {
     full_name = g_strdup(data->name);
     first = FALSE;
  }
     
  
  while ( data->dataType != TYPE_ROOT)
  {
     node = GTK_CTREE_ROW(node)->parent;
     data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) node);

     if (data->name[0] == '*')
     {
	     pointer_count++;
	     continue;
     }
     if ( data->dataType != TYPE_ROOT && data->dataType != TYPE_ARRAY)
     {
	if (first) 
	{
		full_name = g_strdup(data->name);

		first = FALSE;
	}
	else
	{
		t = full_name;
		full_name = g_strconcat(data->name ,".", full_name, NULL);
		g_free(t);
	}	
     }
  }
  
  for (i = 0 ; i < pointer_count ; i++)
  {
	  t = full_name;
	  full_name = g_strconcat("*",full_name,NULL);
	  g_free(t);
  }
  
  return full_name;
}


static
void parse_pointer_cbs (GList* list, Parsepointer *parse)
{
  gchar *pos = NULL;
  DataType data_type;
  TrimmableItem *data;
  gchar* full_output = NULL;	
  gchar* t;

  if (!list)
    gtk_ctree_node_set_text (parse->ctree, parse->node, 1, "?");
  else
  {
    /*  Concat the answers of gdb  */
    pos=g_strdup((gchar*)list->data);
    while ( (list = g_list_next(list)) )
    {
      t = pos;	    
      pos = g_strconcat(pos, (gchar*)list->data, NULL);
      g_free(t);
    }	    
    
    full_output = pos;

    /* g_print("PARSEPOINTER1   %s\n", pos); */

    if ( pos[0] == '$')
    {
      if (!(pos = strchr((gchar*)pos, '=')))
      {
	      g_warning("Format error - cannot find '=' in %s\n",full_output);
	      g_list_free(list); 
	      g_free(parse);
	      g_free(full_output);
	      return;
      }

      data_type = determine_type (pos);
      /*g_print("Type of %s is %s\n",pos, type_names[data_type]);*/
      data = gtk_ctree_node_get_row_data (parse->ctree, 
	      (GtkCTreeNode *) parse->node);
      switch (data_type)
      {
	case TYPE_ARRAY:
	  /* Changing to array not supported */
	  break;
        case TYPE_STRUCT:
          data->dataType = data_type;
          pos += 3;
          parse_data(parse->ctree, parse->node, pos);
          break;
	case TYPE_POINTER:
        case TYPE_VALUE:
	  if (parse->is_pointer) 
	  {
		  t = full_output;		  
		  full_output = pos = g_strconcat("*",data->name,pos,NULL);
		  g_free(t);
		  parse_data(parse->ctree, parse->node, pos);
	  }
	  else
	  {
		  pos += 2;
		  gtk_ctree_node_set_text (parse->ctree, parse->node, 1, pos);
	  }         
          break;
        default :
          parse_data(parse->ctree, parse->node, pos);
          break;

      }
	
     }
  }
  
  /*  Send the next cmd to gdb  */
  if (parse->next)
  {
    data = gtk_ctree_node_get_row_data(parse->ctree,GTK_CTREE_NODE(parse->next->data));	   
    debug_ctree_cmd_gdb(parse->ctree, (parse->next)->data, parse->next,
	  data->display_type,parse->is_pointer);
  }	  
  else
	  g_list_free(list);
  
  g_free(full_output);
  g_free(parse);
}


static void debug_ctree_cmd_gdb(GtkCTree * ctree, GtkCTreeNode *node, GList *list,
	gint display_type, gboolean is_pointer)
{
   gchar *full_name;
   Parsepointer *parse;
   gchar* comm;
   gchar* t;	
   
   g_return_if_fail (ctree);
	
   parse = g_new(Parsepointer, 1);
   parse->ctree = ctree;
   parse->node = node;
   parse->is_pointer = is_pointer;

   if (list)
	   parse->next = g_list_next(list);
   else
	   parse->next = NULL;

   comm = get_display_command(display_type);	
	
   /* extract  full_name name */
   full_name = extract_full_name(ctree, node);

   if (is_pointer)
   {	   
	   t = full_name;
	   full_name = g_strconcat("*",full_name,NULL);
	   g_free(t);
   }	   
	   
   t = full_name;
   full_name = g_strconcat (comm, full_name, NULL);
   g_free(t);
   /*g_print("gdb comm %s\n", full_name);*/
   debugger_put_cmd_in_queqe ("set print pretty on", 0, NULL, NULL);
   debugger_put_cmd_in_queqe ("set verbose off", 0, NULL, NULL);
   debugger_put_cmd_in_queqe (full_name, 0, (void*)parse_pointer_cbs, parse);
   debugger_put_cmd_in_queqe ("set verbose on", 0, NULL, NULL);
   debugger_put_cmd_in_queqe ("set print pretty off", 0, NULL, NULL);
   debugger_execute_cmd_in_queqe ();

   g_free(full_name);
   g_free(comm);
}


static 
void debug_ctree_tree_expand (GtkCTree *ctree, GList *node, gpointer user_data)
{
   GList *expanded_node = NULL;

   g_return_if_fail (ctree);
   g_return_if_fail (node);
	
   /*   Search  expanded   */
   gtk_ctree_pre_recursive (ctree, (GtkCTreeNode *) node,
                                (GtkCTreeFunc) find_expanded, &expanded_node);

   if (expanded_node)
   {
     TrimmableItem* data;
     data = gtk_ctree_node_get_row_data(ctree,GTK_CTREE_NODE(expanded_node->data));	   
     debug_ctree_cmd_gdb(ctree, expanded_node->data, expanded_node,
	   data->display_type, TRUE);
   }	   
}


static 
void debug_ctree_on_select_row (GtkCList * list, gint row, gint column,
                           GdkEvent * event, GtkCTree * ctree)
{
  TrimmableItem *data;
  GtkCTreeNode *node = NULL;


  /* only when double clicking */
  if (!(event->type == GDK_2BUTTON_PRESS))
    return;

  /* extract node */
  node = gtk_ctree_node_nth (GTK_CTREE (ctree), row);
  g_return_if_fail (node != NULL);

  data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) node);
  /*g_print ("SELECT : %p %d  %s  %s  %d  %d\n", node, data->dataType, data->name,
                data->value, data->expandable, data->expanded);*/
  if (data && data->expandable )
  {
    if (!data->expanded)
    {
      data->expanded = TRUE;
      debug_ctree_cmd_gdb(ctree, node, NULL, FORMAT_DEFAULT, TRUE);
      gtk_ctree_expand (ctree, node);
    }
    else //if (GTK_CTREE_ROW(node)->is_leaf)
    {
      /* g_print("EXPFALSE\n"); */
      data->expanded = FALSE;
      gtk_ctree_node_set_text (ctree, node, 1, data->value);
    }
  }
}



static 
void parse_data(GtkCTree * ctree, GtkCTreeNode * parent, gchar * buf)
{
  TrimmableItem *item;
  gchar *var_name = NULL;
  gchar *value = NULL;
  DataType dataType;
  gchar* t;	

  //  g_print ("PARSEDATA %lx  %s\n", (long) buf, buf);
  g_return_if_fail (parent);
  g_return_if_fail (ctree);
	
  if (!buf)
	  return;

  item = gtk_ctree_node_get_row_data (ctree, parent);
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
	    g_free(var_name);
	    break;
    }	    

    set_item (ctree, parent, var_name, dataType, value, FALSE);

    g_free(var_name);
    g_free(value);
  }
}


static 
gboolean is_long_array(GtkCTree *ctree, GtkCTreeNode *parent)
{
  GtkCTreeNode *node;
  gchar *text;

  g_return_val_if_fail(ctree, FALSE);
  g_return_val_if_fail(parent, TRUE);

  node =  GTK_CTREE_ROW(parent)->children;
  while (node )
  {
    gtk_ctree_node_get_text(ctree, node, 1, &text);
    if (strstr( text, "<repeats") )
      return TRUE;
    node =  GTK_CTREE_ROW(node)->sibling;
  }
  return FALSE;
}


/* FIXME : Analyse of long multidimensionnal arrays  */
/* if  the long .. is not the last dimension !!!!    */

static 
void parse_array (GtkCTree * ctree, GtkCTreeNode * parent, gchar * buf)
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
	
  g_return_if_fail (ctree);
  g_return_if_fail (parent);

  item = gtk_ctree_node_get_row_data (ctree, parent);

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

      long_array = is_long_array(ctree, parent);

      set_item (ctree, parent, var_name, dataType, value, long_array);

      pos = strstr (value, " <repeats");
      if (pos)
      {
        if ( (i = atoi (pos + 10)) )
         idx += (i - 1);
      }
      idx++;
      g_free(var_name);
      g_free(value);      
  }
}


static
gchar * get_name (gchar **buf)
{
  gchar *start;
	
  start = skip_next_token_start (*buf);
  if (*start)
  {
      gchar* t;
      *buf = skip_token_value (start);
      t = *buf;
      if (*t == '=')
	      t--;
      return g_strstrip(g_strndup (start, t - start + 1));
  }
  else
    *buf = start;

  return NULL;
}


static
gchar * get_value (gchar **buf)
{
  gchar *start;
  gchar *value;
  /*g_print("get_value: %s\n",*buf);	*/

  start = skip_next_token_start (*buf);
  *buf = skip_token_value (start);

  if (*start == '{')
	return g_strstrip(g_strndup (start + 1, *buf - start - 1));
  if (*buf == start)
	return NULL;
  value = g_strstrip(g_strndup (start, *buf - start));

  return value;
}


static
void set_data (GtkCTree * ctree, GtkCTreeNode * node, DataType dataType,
         const gchar * var_name, const gchar * value,
         gboolean expandable, gboolean expanded, gboolean analyzed)
{
  TrimmableItem *data;

  g_return_if_fail (ctree);
  g_return_if_fail (node);

  /* get node private data */
  data = gtk_ctree_node_get_row_data(ctree,node);
	
  /* there was a private data object - delete old data */	
  if (data)
  {	  
	  if (data->dataType != dataType)
	  g_free(data->name);
	  g_free(data->value);
  }
  else /* first time - allocate private data object */
  {
	  data = g_new (TrimmableItem, 1);
	  data->display_type = FORMAT_DEFAULT;	  	  
	  gtk_ctree_node_set_row_data (ctree, node, data);
  }

  data->name = g_strdup (var_name);
  data->value = g_strdup (value);
  data->dataType = dataType;
  data->expandable = expandable;
  data->expanded = expanded;
  data->analyzed = analyzed;
}


static 
void set_item (GtkCTree * ctree, GtkCTreeNode * parent, const gchar * var_name,
         DataType dataType, const gchar * value, gboolean long_array)
{
  GtkCTreeNode *item;
  GtkCTreeNode *l_item = NULL, *i_item = NULL;
  TrimmableItem *data = NULL;
  gchar *text[2];
  GtkStyle *style;
  gboolean expanded = FALSE;

  g_return_if_fail (ctree);
  
  if (!var_name || !*var_name)
	  return;

  item = GTK_CTREE_ROW(parent)->children;

  /*  find the child of the given parent with the given name */
  while (item)
  {
     data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) item);

     if (data && g_strcasecmp(var_name, data->name) == 0 )
       break;
     item = GTK_CTREE_ROW(item)->sibling;
  }

  /* child found - update value and change color
     if value changed */
  if (item)
  {
     /*  Set red color if var modified  */
     if (g_strcasecmp(value, data->value) == 0)
       style = style_normal;
     else
     {
       style = style_red;
       /*  Destroy following items if long array */
       /*         x <repeats yy times>           */
       if (long_array)
       {
          i_item = GTK_CTREE_ROW(item)->sibling;
          while (i_item)
          {
            l_item = i_item;
            i_item = GTK_CTREE_ROW(i_item)->sibling;
            destroy_recursive(ctree, l_item, NULL);
          }
       }
       if (dataType != TYPE_ARRAY && dataType != TYPE_STRUCT)
       {
         gchar* t = g_strdup(value); /* copy value - orig to be 
				        deleted by caller */
         gtk_ctree_node_set_text (ctree, item, 1, t);
       }	       
     }
     gtk_ctree_node_set_row_style(ctree, item, style);

    expanded = TRUE;
  }
  else  /* child not found - insert it */
  {
    text[0] = g_strdup (var_name);
    if (dataType == TYPE_ARRAY || dataType == TYPE_STRUCT )
      text[1] = g_strdup ("");
    else
      text[1] = g_strdup (value);

    item = gtk_ctree_insert_node (ctree, parent, NULL, text, 5,
           NULL, NULL, NULL, NULL, FALSE, FALSE);
  }

  switch (dataType)
  {
      case TYPE_POINTER:
        // g_print ("POINTER   %s\n", var_name);
        set_data (ctree, item, TYPE_POINTER, var_name, value, TRUE, expanded &&
                  data->expanded, TRUE);
        break;

      case TYPE_STRUCT:
        set_data (ctree, item, TYPE_STRUCT, var_name, value, FALSE, FALSE, TRUE);
        // g_print ("STRUCT  %d\n", dataType);
        parse_data(ctree, item, (gchar *) value);
        break;

      case TYPE_ARRAY:
        set_data (ctree, item, TYPE_ARRAY, var_name, value, FALSE, FALSE, TRUE);
        // g_print ("SETITEM2  %d\n", dataType);
        parse_data(ctree, item, (gchar *) value);
        break;

      case TYPE_REFERENCE:
          /* Not implemented yet */
        break;

      case TYPE_VALUE:
        set_data (ctree, item, TYPE_VALUE, var_name, value, FALSE, FALSE, TRUE);
        // g_print ("SETITEM2  %d\n", dataType);
        break;

      default:
        g_warning("Not setting data for unknown: var [%s] = value[%s]\n",var_name, value);
        break;
  }
}


static 
DataType determine_type (gchar *buf)
{
  if (!buf || !*(buf = skip_next_token_start (buf)))
    return TYPE_UNKNOWN;

  // A reference, probably from a parameter value.
  if (*buf == '@')
    return TYPE_REFERENCE;

  // Structures and arrays - (but which one is which?)
  // {void (void)} 0x804a944 <__builtin_new+41> - this is a fn pointer
  // (void (*)(void)) 0x804a944 <f(E *, char)>  - so is this - ugly!!!
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
            return TYPE_ARRAY;	// Hmm a single element array??
          if (g_strncasecmp (buf + 1, " 0x", 3) == 0)
            return TYPE_POINTER;	// What about references?
          return TYPE_UNKNOWN;	// very odd?
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


static 
gchar * skip_string (gchar * buf)
{
  if (buf && *buf == '\"')
  {
    buf = skip_quotes (buf, *buf);
    while (*buf)
    {
      if ((g_strncasecmp (buf, ", \"", 3) == 0)
          || (g_strncasecmp (buf, ", '", 3) == 0))
        buf = skip_quotes (buf + 2, *(buf + 2));

      else if (g_strncasecmp (buf, " <", 2) == 0)	// take care of <repeats
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


static 
gchar * skip_quotes (gchar * buf, gchar quotes)
{
  if (buf && *buf == quotes)
  {
    buf++;

    while (*buf)
    {
      if (*buf == '\\')
        buf++;		// skips \" or \' problems
      else if (*buf == quotes)
       return buf + 1;

      buf++;
    }
  }
  return buf;
}


static 
gchar * skip_delim (gchar * buf, gchar open, gchar close)
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


static
gchar * skip_token_value (gchar * buf)
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

    if (*end == 0 || *end == ',' || *end == '\n' || *end == '='
          || *end == '}')
       break;

    if (buf == end)
        break;

    buf = end;
  }
  
  return buf;
}


static
gchar * skip_token_end (gchar * buf)
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

    while (*buf && !isspace (*buf) && *buf != ',' && *buf != '}'
           && *buf != '=')
      buf++;
  }
  return buf;
}


static
gchar * skip_next_token_start (gchar * buf)
{
    if (!buf)
	    return NULL;
    
    while (*buf && 
	   (isspace (*buf) || *buf == ',' || *buf == '}' || *buf == '=') )
      buf++;

  return buf;
}


static
void init_style (GtkCTree * ctree)
{
  GdkColor red = { 16, -1, 0, 0 };

  style_normal = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (ctree)));
  style_red = gtk_style_copy (style_normal);
  style_red->fg[GTK_STATE_NORMAL] = red;
}


static
void debug_init (DebugTree *d_tree)
{
  gchar *text[2] = {"Local Variables", ""};

  g_return_if_fail (d_tree);

  if (d_tree->root == NULL)
  {
    init_style (GTK_CTREE(d_tree->tree));
    d_tree->root = gtk_ctree_insert_node (GTK_CTREE(d_tree->tree), NULL, NULL,
                        text, 5, NULL, NULL, NULL, NULL, FALSE, FALSE);
  }
  set_data (GTK_CTREE(d_tree->tree), d_tree->root, TYPE_ROOT, "", "", FALSE,
           FALSE, FALSE);
}


static
void set_not_analyzed(GtkCTree * ctree, GtkCTreeNode *node)
{
  TrimmableItem *data;
  gchar* text;	

  g_return_if_fail (ctree);
  g_return_if_fail (node);

  node = GTK_CTREE_ROW(node)->children;
  while (node)
  {
    data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) node);
    if (!data)
    {
	    gtk_ctree_node_get_text(ctree,node,1,&text);
	    g_warning("Failed getting row data for %s\n",text);
    }	    
    else
	   data->analyzed = FALSE;
    node = GTK_CTREE_ROW(node)->sibling;
  }
}


static 
void destroy_recursive (GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data)
{
   TrimmableItem *data;

   g_return_if_fail (ctree);
   g_return_if_fail (node);

   data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) node);
   if (data)
   {
     g_free(data->name);
     g_free(data->value);
     g_free(data);
   }
   gtk_ctree_remove_node(ctree, node);
}


static
void destroy_non_analyzed(GtkCTree * ctree, GtkCTreeNode *node)
{
  TrimmableItem *data;
  GtkCTreeNode *inode;
  gchar* text = NULL;	

  g_return_if_fail (ctree);
  g_return_if_fail (node);

  node = GTK_CTREE_ROW(node)->children;

  while (node)
  {
    data = gtk_ctree_node_get_row_data (ctree, (GtkCTreeNode *) node);
    if (!data)
    {
	    gtk_ctree_node_get_text(ctree,node,1,&text);
	    g_warning("Failed getting row data for %s\n",text);
    }	    
    inode = node;
    node = GTK_CTREE_ROW(node)->sibling;
    if (data && !data->analyzed )
      gtk_ctree_post_recursive(ctree, inode, destroy_recursive, NULL);
  }
}


static 
void find_expanded (GtkCTree *ctree,  GtkCTreeNode *node, GList **list)
 {
  TrimmableItem *data;

  g_return_if_fail (ctree);
  g_return_if_fail (node); 
	 
  data = gtk_ctree_node_get_row_data (ctree, node);
  if (data)
  {
	if (gtk_ctree_is_viewable(ctree, node) && GTK_CTREE_ROW(node)->expanded &&
           data->expanded)
       *list = g_list_prepend (*list, node);
  }
}


static
void debug_tree_pointer_recursive(GtkCTree *ctree, GtkCTreeNode *node)
{
   GList *expanded_node = NULL;

   g_return_if_fail (ctree);
   g_return_if_fail (node);

   /*  Search expanded viewable pointers  */
   gtk_ctree_pre_recursive (ctree, node, (GtkCTreeFunc) find_expanded,
                            &expanded_node);

   /* send cmd to gdb - Then wait the end of the processing to send the next */
   if (expanded_node)
   {     	 
     TrimmableItem* data = gtk_ctree_node_get_row_data(ctree, 
	   GTK_CTREE_NODE(expanded_node->data));
     
     debug_ctree_cmd_gdb(ctree, expanded_node->data, expanded_node, 
	   data->display_type, TRUE);
   }	         
}


static
void on_inspect_memory_clicked(GtkMenuItem* menu_item, gpointer data)
{
  DebugTree* d_tree = (DebugTree*)data;
  gchar *buf;
  gchar *start, *end;
  gchar *hexa;
  guchar *adr;
  GtkWidget *memory;

  gtk_ctree_node_get_text(GTK_CTREE(d_tree->tree), d_tree->cur_node, 1, &buf);

  while (*buf != '\0' && !( *buf == '0' && *(buf + 1) == 'x') )
    buf++;
  if (*buf != '\0')
  {
    end = start = buf +2;
    while ( (*end >= '0' && *end <= '9') || (*end >= 'a' && *end <= 'f') )
      end++;
    hexa = g_strndup(start, end - start);
    adr = adr_to_decimal( hexa );
    memory = create_info_memory (adr);
    gtk_widget_show (memory);
    g_free(hexa);
  }
}


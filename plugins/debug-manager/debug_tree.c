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

// TODO:
//   Alloc string in item_data object, so treeview can use POINTER
//     => no need to free data after getting the value
//     => value can be access more easily if we have the tree viex
//     => do the same for variable name
//
//   Add a function to get all arguments

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
#include <assert.h>

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

enum {AUTO_UPDATE_WATCH = 1 << 0};	

/* Common data */
typedef struct _CommonDebugTree CommonDebugTree;
typedef struct _local_data_transport local_data_transport;
typedef struct _item_data item_data;

struct _CommonDebugTree {
	guint initialized;
	DebugTree* current;
	DebugTree* user;
	GList *tree_list;
};

/* The debug tree object */
struct _DebugTree {
	IAnjutaDebugger *debugger;
	AnjutaPlugin *plugin;
	GtkWidget* view;        /* the tree widget */
	GtkTreeIter* cur_node;
	GSList* free_list;
	gboolean auto_expand;
};

struct _local_data_transport {
	item_data *item;
	GtkTreeModel *model;
	GtkTreeIter iter;
	DebugTree *tree;
	local_data_transport* next;
};

struct _item_data {

    guchar expanded;
 	guchar expandable;
	guchar analyzed;
	guchar modified;    /* Set by tree update */
	guchar changed;     /* Set by global update */

	guchar is_root;
	guchar is_complex;
	guchar is_array;
	guchar is_string;
	
	gboolean auto_update;
	
	gint pointer_level;
	DebugTree *tree;
	item_data *parent;
	local_data_transport* transport;
		
	gchar* name;
};

struct add_new_tree_item_args {
	DebugTree *const tree;
	GtkTreeModel *const model;
	GtkTreeIter *const parent;
	item_data *parent_data;
	GtkTreeIter *const iter;
	gchar *exp;
	gchar *name;
	gchar *type;
	gchar *value;
	gboolean init;
	gboolean first_exist;
	gboolean is_complex;
	gboolean auto_update;
};

static item_data * 
add_new_tree_item(struct add_new_tree_item_args *const args);

/* Constant
 *---------------------------------------------------------------------------*/

#define UNKNOWN_VALUE "???"
#define UNKNOWN_TYPE "?"
#define AUTO_UPDATE 'U'

enum {
	VARIABLE_COLUMN,
	VALUE_COLUMN,
	TYPE_COLUMN,
	ROOT_COLUMN,
	DTREE_ENTRY_COLUMN,
	N_COLUMNS
};

static gchar *tree_title[] = {
	N_("Variable"), N_("Value"), N_("Type")
};

/* Global variable
 *---------------------------------------------------------------------------*/

static GList* gTreeList = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

static gboolean 
get_current_iter (GtkTreeView *view, GtkTreeIter* iter)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (view);
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}

static gint GetPointerLevel(const gchar *const type)
{
	/* function pointer */
	if (NULL != strstr(type,  "(*)")) return 0;
	/* references */
	if (NULL != strchr(type, '&')) return -1;
	gint level = 0;
	const gchar *ptr = strchr(type, '*');
    while( ptr ) {
		++level;
		ptr = strchr(ptr + 1, '*');
	}
	return level;
}

static unsigned int 
isArray(const gchar *const type)
{
	return ((NULL != strchr(type, '[')));
}

static unsigned int
isString(const gchar *const type)
{
	return ((NULL != strstr(type, "char ")));
}

static unsigned int
isComplex(const gchar *const type)
{
	return ((NULL != strstr(type, "struct ")) ||
            (NULL != strstr(type, "union ")) ||
            (NULL != strstr(type, "class ")) );
}

static item_data *
alloc_item_data(const gchar *const name)
{
	item_data *data;
		
	data = g_new0 (item_data, 1);
	if (name != NULL)
	{
		data->name = g_strdup (name);
	}
		
	return data;
}

static void 
free_item_data(item_data * this)
{
	if (this->tree->debugger)
	{
		if ((this->name) && (this->is_root))
		{
			/* Object has been created in debugger and is not a child
			 * (destroyed with their parent) */
			ianjuta_variable_debugger_delete (IANJUTA_VARIABLE_DEBUGGER (this->tree->debugger), this->name, NULL);
		}
	}
	if (this->transport != NULL)
	{
		/* Mark the data as invalid, the transport structure will
		 * be free later in the callback */
		local_data_transport* tran;
		
		for (tran = this->transport; tran != NULL; tran = tran->next)
		{
			tran->item = NULL;
		}
	}
	if (this->name != NULL)
	{
		g_free (this->name);
	}
	
	g_free(this);
}

/* ------------------------------------------------------------------ */

static local_data_transport *
alloc_data_transport(const GtkTreeModel *const model,
                     const  GtkTreeIter *const iter,
					 item_data *item)
{
	g_return_val_if_fail (model, NULL);
	g_return_val_if_fail (iter, NULL);

	local_data_transport *data = g_new (local_data_transport, 1);

	data->item = item;
	data->model = (GtkTreeModel *)model;
	data->iter = *iter;
	data->tree = item->tree;
	data->next = item->transport;
	item->transport = data;

	item_data *check;
	gtk_tree_model_get (data->model, &data->iter,
                                         DTREE_ENTRY_COLUMN,
                                         &check, -1);

	return data;
}

static void
free_data_transport (local_data_transport* data)
{
	if (data->item != NULL)
	{
		/* Remove from transport data list */
		local_data_transport **find;
		
		for (find = &data->item->transport; *find != NULL; find = &(*find)->next)
		{
			if (*find == data)
			{
				*find = data->next;
				break;
			}
		}
	}
	g_free (data);
}

/* DebugTree private functions
 *---------------------------------------------------------------------------*/

static void
my_gtk_tree_model_foreach_child (GtkTreeModel *const model,
                           GtkTreeIter *const parent,
                           GtkTreeModelForeachFunc func,
                           gpointer user_data)
{
  GtkTreeIter iter;
  gboolean success = gtk_tree_model_iter_children(model, &iter, parent);
  while(success)
  {
    if(gtk_tree_model_iter_has_child(model, &iter))
        my_gtk_tree_model_foreach_child (model, &iter, func, NULL);
	  	
    	success = (!func(model, NULL, &iter, user_data) &&
               gtk_tree_model_iter_next (model, &iter));
  }
}

static gboolean
delete_node(GtkTreeModel *model, GtkTreePath* path,
			GtkTreeIter* iter, gpointer pdata)
{
	item_data *data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	

	/*printf ("free %p ", data == NULL ? "(null)" : data);
	if (data != NULL)			
		printf ("name %s", data->name == NULL ? "(null)" : data->name);
	printf("\n");*/
	if (data) free_item_data(data);
	
	return FALSE;
}

static void
debug_tree_cell_data_func (GtkTreeViewColumn *tree_column,
				GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data)
{
	gchar *value;
	static const gchar *colors[] = {"black", "red"};
	GValue gvalue = {0, };
	item_data *item_data = NULL;

	gtk_tree_model_get (tree_model, iter, VALUE_COLUMN, &value, -1);
	g_value_init (&gvalue, G_TYPE_STRING);
	g_value_set_static_string (&gvalue, value);
	g_object_set_property (G_OBJECT (cell), "text", &gvalue);

	gtk_tree_model_get (tree_model, iter, DTREE_ENTRY_COLUMN, &item_data, -1);
	
	if (item_data)
	{
		g_value_reset (&gvalue);
		g_value_set_static_string (&gvalue, 
                        colors[(item_data && item_data->modified ? 1 : 0)]);

		g_object_set_property (G_OBJECT (cell), "foreground", &gvalue);
	}
	g_free (value);
}


static gboolean
set_not_analyzed(GtkTreeModel *model, GtkTreePath* path,
				 GtkTreeIter* iter, gpointer pdata)
{
	item_data *data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);
	
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	if (data)
		data->analyzed = FALSE;

	return FALSE;	
}

static void 
add_dummy_node(GtkTreeModel *const model, GtkTreeIter *const parent)
{
  GtkTreeIter iter;
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, parent);
  gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                     TYPE_COLUMN, "",
                     VALUE_COLUMN, "", DTREE_ENTRY_COLUMN, NULL, -1);
}

static void
destroy_non_analyzed (GtkTreeModel* model)
{
	item_data *data;
	GtkTreeIter iter;
	gboolean success;
	
	g_return_if_fail (model);

	for (success = gtk_tree_model_get_iter_first (model, &iter); success == TRUE; )
	{
		gtk_tree_model_get(model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
		if ((data != NULL) && (data->analyzed == FALSE))
		{
			my_gtk_tree_model_foreach_child (model,&iter,delete_node, NULL);
			free_item_data(data);
			success = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		}
		else
		{
			success = gtk_tree_model_iter_next (model, &iter);
		}
	};
}

/* support [0xadress] and [(type ***) 0xadress] cases */
static unsigned int isPtrValNULL(const gchar *const value)
{
	  const gchar *const real = strchr(value, ')');
	return real ? 0 == strcmp(real + 2,"0x0") : 0 == strcmp(value,"0x0");
}
 
static void 
update_expandable_pointer(
        GtkTreeModel *const model,
        GtkTreeIter *const node,
        item_data *data,
        const gchar *const value)
{
  /* currently we cant good detect complex var pointers in root
     because child number is avliable after gdb var creation
     then all single level pointers are expandable */
  if( !data->is_string &&
      (data->is_complex || data->is_root) && data->pointer_level == 1)
  {
    if(!isPtrValNULL(value))
    {
      if( !data->expanded && !data->expandable )
      {
        data->expandable = TRUE;
        //add_dummy_node(model, node);
      }
    } else {
      if(data->expanded || data->expandable)
      {
          data->expandable = FALSE;
          data->expanded = FALSE;
          GtkTreeIter iter;
          gtk_tree_model_iter_children(model, &iter, node);
		  do
		  {
        	  	my_gtk_tree_model_foreach_child (model, &iter, delete_node, NULL);
				delete_node (model, NULL, &iter, NULL); 
		  } while (gtk_tree_store_remove (GTK_TREE_STORE (model), &iter));
      }
    }
  }
  return;
}

/*---------------------------------------------------------------------------*/

static void
gdb_var_evaluate_expression (const gchar *value,
                        gpointer user_data, GError* err)
{
	local_data_transport *data =
                         (local_data_transport *) user_data;
	gchar *parent_value;
	g_return_if_fail (data != NULL);

	if ((err != NULL) || (data->item == NULL))
	{
		/* Command failed or item has been deleted */
		free_data_transport (data);
		
		return;
	}
						 
	if ((value == NULL) || (*value == '\0'))
	{
		free_data_transport (data);
		return;
	}
	
	/* hack */
	GtkTreeModel* model = data->model;

	item_data *iData;
	gtk_tree_model_get (model, &data->iter,
                                      DTREE_ENTRY_COLUMN, &iData, -1);
	
	if(iData && iData->parent &&
       iData->parent->is_string && iData->parent->is_array)
    {
		GtkTreeIter iter;
		if(gtk_tree_model_iter_parent(model, &iter, &data->iter))
        {
        	gchar *num;

			gtk_tree_model_get (model, &iter, VALUE_COLUMN, &parent_value, -1);

			num = strrchr(iData->name, '.');
         
			assert(NULL != num && "no array type");

			++num;
			const gulong index = strtol(num, NULL, 0);
			const gulong len = strlen(parent_value) + 1;
			if(index < len) {
            	gchar buffer[len + 1];
            	g_strlcpy(buffer, parent_value, len);
            	buffer[index] = (gchar)strtol(value, NULL, 0);
            	buffer[len] = '\0';
            	gtk_tree_store_set(GTK_TREE_STORE (data->model),
                	               &iter, VALUE_COLUMN, buffer, -1);
          	}
          	g_free(parent_value);
		}
	}

	update_expandable_pointer(model, &data->iter, iData, value);
      
      /* auto expand funtionality: 
         should we add some gdb proference settings for that */
      GtkTreeIter iter;
      if(gtk_tree_model_iter_parent (model, &iter, &data->iter) ) {
        GtkTreePath*path = gtk_tree_model_get_path(model, &iter);
        if(!gtk_tree_view_row_expanded (GTK_TREE_VIEW(data->tree->view),path))
          gtk_tree_view_expand_to_path (GTK_TREE_VIEW(data->tree->view),path);
        gtk_tree_path_free(path);
      }


	  iData->changed = FALSE;
	  gtk_tree_store_set(GTK_TREE_STORE (model), &data->iter, VALUE_COLUMN, value, -1);
    
	free_data_transport (data);
}

static void
gdb_var_list_children (const GList *children, gpointer user_data, GError *err)
{
	local_data_transport *data =
                         (local_data_transport *) user_data;
	GList *child;
	g_return_if_fail (data != NULL);

	if ((err != NULL) || (data->item == NULL))
	{
		/* Command failed or item has been deleted */
		free_data_transport (data);
		
		return;
	}
						 
	GtkTreeIter iter;

    struct add_new_tree_item_args args = {data->tree, data->model,
                              &data->iter, NULL, &iter,
                              NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, TRUE};

	gtk_tree_model_get (data->model, &data->iter,
                                         DTREE_ENTRY_COLUMN,
                                         &args.parent_data, -1);

	if (args.parent_data == NULL)
	{
		gchar *str_iter;
		str_iter = gtk_tree_model_get_string_from_iter (data->model, &data->iter);
		g_free (str_iter);
	}
		
	//if (args.parent_data == NULL) return;
	const gboolean parent_is_array = args.parent_data->is_array;

    args.first_exist = 
                gtk_tree_model_iter_children(data->model, &iter, &data->iter);

	for (child = g_list_first (children); child != NULL; child = g_list_next (child))
    {
		IAnjutaDebuggerVariable *var = (IAnjutaDebuggerVariable *)child->data;
		args.name = var->name;
		args.exp = var->expression;
                
	    if(parent_is_array) {
			  gchar *str_parent_name;
			  gchar *exp = args.exp;
              gtk_tree_model_get (data->model,
                               &data->iter,
                               VARIABLE_COLUMN, &str_parent_name, -1);
               args.exp = g_strdup_printf("%s[%s]", str_parent_name, exp);
               g_free(str_parent_name);
        }

        args.type = var->type;
		args.value = var->value;

        item_data *iData = add_new_tree_item(&args);
                
        if(iData && iData->pointer_level < 1)
        {
			  if(var->children > 0)
              {
				  local_data_transport *tData =
                                       alloc_data_transport(data->model, &iter, iData);
				  item_data parent_data;

				  gtk_tree_model_get (data->model, &iter,
                                         DTREE_ENTRY_COLUMN,
                                         &parent_data, -1);
				  
				  
/* 				  printf ("send list child %s %p parent %p\n", iData->name, tData->iter.user_data, parent_data);
				  GNode *tmp;
 				  tmp = (GNode *)(tData->iter.user_data);
	  			while (tmp != NULL)
				  {
					  printf ("parent %p\n", tmp);
					  tmp = tmp->parent;
				  }*/
				  if (!ianjuta_variable_debugger_list_children (IANJUTA_VARIABLE_DEBUGGER (data->tree->debugger), args.name, gdb_var_list_children, tData, NULL))
				  {
					  break;
				  }
			  }
		}

	    if(parent_is_array) {
			  g_free(args.exp);
        }
	  }
		  
	  free_data_transport (data);
}

static void
gdb_var_create (IAnjutaDebuggerVariable *variable, gpointer user_data, GError *err)
{
	local_data_transport *data =
                         (local_data_transport *) user_data;
	g_return_if_fail (data != NULL);

	if (err != NULL)
	{
		/* Command failed */
		free_data_transport (data);
		
		return;
	}
	if (data->item == NULL)
	{
		/* Item has been deleted */
		if ((data->tree->debugger) && (variable->name))
		{
			ianjuta_variable_debugger_delete (IANJUTA_VARIABLE_DEBUGGER (data->tree->debugger), variable->name, NULL);
		}
		free_data_transport (data);
		
		return;
	}
						 
	item_data *iData;
	gtk_tree_model_get (data->model, &data->iter,
                                      DTREE_ENTRY_COLUMN, &iData, -1);

	if ((iData != NULL) && (variable->name != NULL) && (iData->name == NULL))
	{
		iData->name = strdup (variable->name);
	}
    iData->expanded = TRUE;
    iData->analyzed = TRUE;
    iData->changed = TRUE;
    iData->is_root = TRUE;
    iData->is_array = isArray(variable->type);
    iData->is_string = isString(variable->type);
    if(!iData->is_array && !iData->is_string)
    {
    	iData->is_complex = variable->children > 0;
    } 
    iData->pointer_level = GetPointerLevel(variable->type);
    iData->tree = data->tree;

    if(!iData->is_string)
    {
	if(iData->is_array)
    	{
		gchar buffer[16];
		sprintf(buffer, "[%d]", variable->children);
		gtk_tree_store_set(GTK_TREE_STORE(data->model), &data->iter,
						   TYPE_COLUMN, variable->type,
						   VALUE_COLUMN, buffer, -1);
	} else if(iData->pointer_level < 1) {
        	gtk_tree_store_set(GTK_TREE_STORE(data->model), &data->iter,
						   TYPE_COLUMN, variable->type,
						   VALUE_COLUMN, "{...}", -1);
	}
    } else {
		gtk_tree_store_set(GTK_TREE_STORE(data->model), &data->iter,
						   TYPE_COLUMN, variable->type,
						   VALUE_COLUMN, variable->value, -1);
	}
	
    if (variable->children)
	{
		ianjuta_variable_debugger_list_children (IANJUTA_VARIABLE_DEBUGGER (data->tree->debugger), variable->name, gdb_var_list_children, data, NULL);
	}
	else if (variable->value == NULL)
	{
		local_data_transport *tData =
                   alloc_data_transport(data->model, &data->iter, iData);

		ianjuta_variable_debugger_evaluate (IANJUTA_VARIABLE_DEBUGGER (data->tree->debugger), variable->name, gdb_var_evaluate_expression, tData, NULL);
		free_data_transport (data);
	}
}

/* ------------------------------------------------------------------ */

static item_data * 
add_new_tree_item(struct add_new_tree_item_args *const args)
{
	item_data *data = NULL;
	  
	if(!args->first_exist)
		gtk_tree_store_append(GTK_TREE_STORE(args->model), args->iter, args->parent);
	else
		args->first_exist = FALSE;

	gtk_tree_store_set(GTK_TREE_STORE(args->model), args->iter,
					   VARIABLE_COLUMN, args->exp,
					   ROOT_COLUMN, args->init,-1);
	if ((args->type != NULL) && (args->value != NULL))
		gtk_tree_store_set(GTK_TREE_STORE(args->model), args->iter,
						   TYPE_COLUMN, args->type,
						   VALUE_COLUMN, args->value, -1);

	const gboolean is_array = args->type == NULL ? FALSE : isArray(args->type);
	const gboolean is_complex = args->type == NULL ? TRUE : isComplex(args->type);
	const gint pointer_level = args->type == NULL ? 0 : GetPointerLevel(args->type);

	if(args->init && (is_array || ( is_complex && pointer_level < 1 ) ) )
       	{
		local_data_transport *tData;
		data = alloc_item_data(args->name);
		data->is_root = args->init; 
		data->analyzed = TRUE;
		data->changed = TRUE;
		data->parent = args->parent_data;
		data->is_array = is_array;
		data->is_complex = is_complex;
		data->is_string = args->type == NULL ? FALSE : isString(args->type);
		data->pointer_level = pointer_level;
		data->auto_update = args->auto_update;  
		data->tree = args->tree;

		gtk_tree_store_set(GTK_TREE_STORE(args->model), args->iter,
                         DTREE_ENTRY_COLUMN, data, -1);

		if (args->tree->debugger != NULL)
		{
			tData = alloc_data_transport(args->model, args->iter, data);
			ianjuta_variable_debugger_create (IANJUTA_VARIABLE_DEBUGGER (args->tree->debugger), args->exp, gdb_var_create, tData, NULL);
		}
	} else {
		data = alloc_item_data(args->name);
		data->is_root = args->init; 
		data->analyzed = TRUE;
		data->changed = TRUE;
		data->modified = TRUE;
		data->parent = args->parent_data;
		data->is_array = is_array;
		data->is_complex = is_complex;
		data->is_string = args->type == NULL ? FALSE : isString(args->type);
		data->pointer_level = pointer_level;
		data->auto_update = args->auto_update;  
		data->tree = args->tree;

		gtk_tree_store_set(GTK_TREE_STORE(args->model), args->iter,
                         DTREE_ENTRY_COLUMN, data, -1);

		/* currently we cant good detect complex var pointers in root
		   because child number is avliable after gdb var creation
		   then all single level pointers are expandable */

		if((args->is_complex || args->init) && !data->is_string &&
		        data->pointer_level == 1 && !isPtrValNULL(args->value)) 
        	{
			data->expandable = TRUE;
			//add_dummy_node(args->model, args->iter);
		}

	}
	
	return data;
}

/* ------------------------------------------------------------------ */

static void
on_treeview_row_expanded       (GtkTreeView     *treeview,
                                 GtkTreeIter     *arg1,
                                 GtkTreePath     *arg2,
                                 gpointer         user_data)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeModel *const model =
                  gtk_tree_view_get_model (treeview);

	item_data *iData;
		gtk_tree_model_get (model, arg1, DTREE_ENTRY_COLUMN, &iData, -1);

    if(iData && iData->expandable) {
		local_data_transport *tData = alloc_data_transport(model, arg1, iData);
		if(iData->is_root && !iData->tree->debugger)
		{
			ianjuta_variable_debugger_create (IANJUTA_VARIABLE_DEBUGGER (tree->debugger), iData->name, gdb_var_create, tData, NULL);
		} else {
			iData->expandable = FALSE;
			iData->expanded = TRUE;
			ianjuta_variable_debugger_list_children (IANJUTA_VARIABLE_DEBUGGER (tree->debugger), iData->name, gdb_var_list_children, tData, NULL);
		}
    }

	return;
}

static void
on_debug_tree_variable_changed (GtkCellRendererText *cell,
						  gchar *path_string,
                          gchar *text,
                          gpointer user_data)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeIter iter;
	GtkTreeModel * model;
	
    model = gtk_tree_view_get_model (tree->view);
	if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
	{
		debug_tree_remove (tree, &iter);
		
		if ((text != NULL) && (*text != '\0'))
		{
		    IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};
			
			var.expression = text;
			debug_tree_add_watch (tree, &var, TRUE);
		}
	}
}

static void
on_debug_tree_value_changed (GtkCellRendererText *cell,
						  gchar *path_string,
                          gchar *text,
                          gpointer user_data)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeIter iter;
	GtkTreeModel * model;
	
    model = gtk_tree_view_get_model (tree->view);

	if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
	{
		item_data *item;
		local_data_transport *tran;

		gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &item, -1);
		if (item != NULL)
		{
			/* Variable is valid */
			ianjuta_variable_debugger_assign (tree->debugger, item->name, text, NULL);
			tran = alloc_data_transport(model, &iter, item);
			ianjuta_variable_debugger_evaluate (tree->debugger, item->name, gdb_var_evaluate_expression, tran, NULL);
		}
	}
}

static GtkWidget *
debug_tree_create (DebugTree *tree, GtkTreeView *view)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel * model = gtk_tree_store_new
			                     (N_COLUMNS, 
	                                          G_TYPE_STRING, 
	                                          G_TYPE_STRING,
                                              G_TYPE_STRING,
								              G_TYPE_BOOLEAN,
			                      G_TYPE_POINTER);
	
	if (view == NULL)
	{
		view = gtk_tree_view_new ();
	}
	
    gtk_tree_view_set_model (view, GTK_TREE_MODEL (model));
  
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (model));

	/* Columns */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", VARIABLE_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "editable", ROOT_COLUMN);
	g_signal_connect(renderer, "edited", (GCallback) on_debug_tree_variable_changed, tree);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[0]));
	gtk_tree_view_append_column (view, column);
	gtk_tree_view_set_expander_column (view, column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
                               debug_tree_cell_data_func, NULL, NULL);
	gtk_tree_view_column_add_attribute (column, renderer, "text", VALUE_COLUMN);
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", (GCallback) on_debug_tree_value_changed, tree);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[1]));
	gtk_tree_view_append_column (view, column);


    column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", TYPE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[2]));
	gtk_tree_view_append_column (view, column);

    return GTK_WIDGET (view);
}

/* ----------------------------------------------------------------------- */

/* Public functions
 *---------------------------------------------------------------------------*/

/* clear the display of the debug tree and reset the title */
void
debug_tree_remove_all (DebugTree *l)
{
	GtkTreeModel *model;

	g_return_if_fail (l);
	g_return_if_fail (l->view);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (l->view));
	gtk_tree_model_foreach(model,delete_node, l);	
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

static gboolean
debug_tree_find_name (const GtkTreeModel *model, GtkTreeIter *iter, const gchar *name)
{
	size_t len = 0;
	GtkTreeIter parent_iter;
	GtkTreeIter* parent = NULL;
	
	for (;;)
	{
		const gchar *ptr;
		
		/* Check if we look for a child variable */
		ptr = strchr(name + len + 1, '.');
		if (ptr != NULL)
		{
			/* Child variable */
			gboolean search;
			
			len = ptr - name;
			for (search = gtk_tree_model_iter_children (model, iter, parent);
				search != FALSE;
	    		search = gtk_tree_model_iter_next (model, iter))
			{
				item_data *iData;
        		gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &iData, -1);
			
				if ((iData != NULL) && (iData->name != NULL) && (name[len] == '.') && (strncmp (name, iData->name, len) == 0))
				{
					break;
				}
			}
			
			if (search == TRUE)
			{
				parent_iter = *iter;
				parent = &parent_iter;
				continue;
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			/* Variable without any child */
			gboolean search;

			for (search = gtk_tree_model_iter_children (model, iter, parent);
				search != FALSE;
	    		search = gtk_tree_model_iter_next (model, iter))
			{
				item_data *iData;
        		gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &iData, -1);
			
				if ((iData != NULL) && (iData->name != NULL) && (strcmp (name, iData->name) == 0))
				{
					return TRUE;
				}
			}
			
			return FALSE;
		}
	}
}

static gboolean
debug_tree_find_expression (const GtkTreeModel *model, GtkTreeIter *iter, const gchar *expression, const gchar *type)
{
	gboolean search;
	gboolean found = FALSE;
	
	for (search = gtk_tree_model_get_iter_first (model, iter);
		search && !found;
	    search = gtk_tree_model_iter_next (model, iter))
	{
		gchar *exp;
		gchar *typ;
		
        gtk_tree_model_get (model, iter, TYPE_COLUMN, &typ, VARIABLE_COLUMN, &exp, -1);
		
		found = ((type == NULL) || (strcmp (typ, type) == 0))
			&& ((expression == NULL) || (strcmp (exp, expression) == 0));
		
		if (type != NULL) g_free (type);
		if (exp != NULL) g_free (exp);
	}
	
	return found;
}

static void
on_replace_watch (gpointer data, gpointer user_data)
{
	DebugTree* tree = (DebugTree *)user_data;
	const gchar *expression = (const gchar *)data;
	GtkTreeModel*const model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree->view));
	IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};
	GtkTreeIter iter;
	
	if (debug_tree_find_expression (model, &iter, expression, NULL))
	{
		item_data *iData;
		
        gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &iData, -1);

		if (iData != NULL)
			iData->analyzed = TRUE;
	}
	else
	{
		var.expression = expression;
		debug_tree_add_watch (tree, &var, TRUE);
	}
}

void
debug_tree_replace_list (DebugTree *tree, const GList *expressions)
{
	GtkTreeModel* model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree->view));

	/* Mark variables as not analyzed */
	gtk_tree_model_foreach(model,set_not_analyzed,NULL);	
	
	g_list_foreach (expressions, on_replace_watch, tree);	

	destroy_non_analyzed (model);	
}

void
debug_tree_add_dummy (DebugTree *tree)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	GtkTreeIter iter;
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
					   VARIABLE_COLUMN, "",
					   VALUE_COLUMN, "",
					   TYPE_COLUMN, "",
					   ROOT_COLUMN, TRUE,
					   DTREE_ENTRY_COLUMN, NULL, -1);
}

void
debug_tree_add_watch (DebugTree *tree, const IAnjutaDebuggerVariable* var, gboolean auto_update)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	GtkTreeIter iter;
	item_data *item = NULL;
	gchar *exp;
	
	/* Allocate data */
	item = alloc_item_data(var->name);
	item->analyzed = TRUE;
	item->changed = TRUE;
	item->parent = NULL;
	item->auto_update = auto_update;  
	item->tree = tree;
	
	/* Add node in tree */
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
					   TYPE_COLUMN, var->type == NULL ? UNKNOWN_TYPE : var->type,
					   VALUE_COLUMN, var->value == NULL ? UNKNOWN_VALUE : var->value,
					   VARIABLE_COLUMN, var->expression,
					   ROOT_COLUMN, TRUE,
					   DTREE_ENTRY_COLUMN, item, -1);

	if (tree->debugger != NULL)
	{
		if ((var->value == NULL) || (var->children == -1))
		{
			if (var->name == NULL)
			{
				/* Need to create variable before to get value */
				local_data_transport *tran;
		
				tran = alloc_data_transport(model, &iter, item);
				ianjuta_variable_debugger_create (IANJUTA_VARIABLE_DEBUGGER (tree->debugger), var->expression, gdb_var_create, tran, NULL);
			}
			else
			{
				if (var->value == NULL)
				{
					/* Get value */
					local_data_transport *tran =
					
					tran = alloc_data_transport(model, &iter, item);
					ianjuta_variable_debugger_evaluate (IANJUTA_VARIABLE_DEBUGGER (tree->debugger), var->name, gdb_var_evaluate_expression, tran, NULL);
				}
				if (var->children == -1)
				{
					/* Get number of children */
					local_data_transport *tran =
					
					tran = alloc_data_transport(model, &iter, item);
					ianjuta_variable_debugger_list_children (IANJUTA_VARIABLE_DEBUGGER (tree->debugger), var->name, gdb_var_list_children, tran, NULL);
				}
			}
		}
	}
}

void debug_tree_expand_watch (DebugTree *tree, GtkTreeIter*parent)
{
#if 0
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	GtkTreeIter iter;
	item_data *item = NULL;
	gchar *exp;
	
	/* Allocate data */
	item = alloc_item_data(var->name);
	item->analyzed = TRUE;
	item->changed = TRUE;
	item->parent = NULL;
	item->auto_update = auto_update;  
	item->tree = tree;
	
	/* Add node in tree */
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
					   TYPE_COLUMN, var->type == NULL ? UNKNOWN_TYPE : var->type,
					   VALUE_COLUMN, var->value == NULL ? UNKNOWN_VALUE : var->value,
					   VARIABLE_COLUMN, var->expression,
#endif
}

static void
on_add_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	gboolean auto_update = ((const gchar *)data)[0] & AUTO_UPDATE_WATCH ? TRUE : FALSE;
	IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};

	var.expression = &((const gchar *)data)[1];
	debug_tree_add_watch (this, &var, auto_update);
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
	IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};

	var.expression = &((const gchar *)data)[0];
	debug_tree_add_watch (this, &var, FALSE);
}

static void
on_add_auto_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};
	
	var.expression = &((const gchar *)data)[0];
	debug_tree_add_watch (this, &var, TRUE);
}

void
debug_tree_add_watch_list (DebugTree *this, GList *expressions, gboolean auto_update)
{
	g_list_foreach (expressions, auto_update ? on_add_auto_watch : on_add_manual_watch, this);
}

static void
on_debug_tree_changed (gpointer data, gpointer user_data)
{
	IAnjutaDebuggerVariable *var = (IAnjutaDebuggerVariable *)data;
	
	if (var->name != NULL)
	{
		/* Search corresponding variable in one tree */
		GList *tree;
		
		for (tree = g_list_first (gTreeList); tree != NULL; tree = g_list_next (tree))
		{
			GtkTreeIter iter;
			GtkTreeModel *model;
			
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (((DebugTree *)tree->data)->view));
			
			if (debug_tree_find_name (model, &iter, var->name))
			{
				item_data *iData;
				gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &iData, -1);
		
				if (iData != NULL)
					iData->changed = TRUE;
				
				return;
			}
		}
	}
}

static gboolean
on_debug_tree_modified (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	item_data *data = NULL;
	
	gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	
	if (data->modified != data->changed)
	{
		data->modified = data->changed;
		gtk_tree_model_row_changed (model, path, iter);
	}
	data->changed = FALSE;

	return FALSE;
}

static void
on_debug_tree_update_all (const GList *change, gpointer user_data, GError* err)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (err != NULL) return;

	// Mark all modified variables
	g_list_foreach (change, on_debug_tree_changed, NULL);

	// Update this tree
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
	{
		do
		{
			item_data *data = NULL;
			
			gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
	
			debug_tree_update (tree, &iter, FALSE);
		} while (gtk_tree_model_iter_next (model, &iter) == TRUE);
	}
	
	// Update modified mark
	gtk_tree_model_foreach (model, on_debug_tree_modified, NULL);
}

void
debug_tree_update_all (DebugTree* tree)
{
	if (tree->debugger != NULL)
	{
		/* Update if debugger is connected */
		ianjuta_variable_debugger_update (tree->debugger, on_debug_tree_update_all, tree, NULL);
	}
}

GList*
debug_tree_get_full_watch_list (DebugTree *this)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList* list = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->view));
	
	if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
	{
		do
		{
			item_data *data;
			gchar *exp;
			gchar *exp_with_flag;
	
			gtk_tree_model_get(model, &iter, DTREE_ENTRY_COLUMN, &data, 
							                 VARIABLE_COLUMN, &exp, -1);	

			if (data != NULL)
			{
				exp_with_flag = g_strconcat (" ", exp, NULL); 
				exp_with_flag[0] = data->auto_update ? AUTO_UPDATE_WATCH : ' ';
				list = g_list_prepend (list, exp_with_flag);
			}
			g_free (exp);
		} while (gtk_tree_model_iter_next (model, &iter) == TRUE);
	}
	
	list = g_list_reverse (list);
	
	return list;
}


GtkWidget *
debug_tree_get_tree_widget (DebugTree *this)
{
	return this->view;
}

gboolean
debug_tree_get_current (DebugTree *tree, GtkTreeIter* iter)
{
	return get_current_iter (tree->view, iter);
}

void
debug_tree_remove (DebugTree *tree, GtkTreeIter* iter)
{
	GtkTreeModel *const model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree->view));
	
	delete_node (model, NULL, iter, NULL); 
	gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
}

gboolean
debug_tree_update (DebugTree* tree, GtkTreeIter* iter, gboolean force)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	item_data *data = NULL;
	GtkTreeIter child;
	gboolean search;
	gboolean refresh = TRUE;
	
	gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	if (data == NULL) return FALSE;

	if (data->name == NULL)
	{
		/* Variable need to be created first */
		gchar *exp;
		local_data_transport *tData;
			
		gtk_tree_model_get (model, iter, VARIABLE_COLUMN, &exp, -1);
		tData = alloc_data_transport(model, iter, data);
		data->modified = TRUE;
		ianjuta_variable_debugger_create (IANJUTA_VARIABLE_DEBUGGER (data->tree->debugger), exp, gdb_var_create, tData, NULL);
		g_free (exp);
		
		return FALSE;
	}	
	else if (force || (data->auto_update && data->changed))
	{
		local_data_transport *tData =
                                alloc_data_transport(model, iter, data);
		refresh = data->modified != (data->changed != FALSE);
		data->modified = (data->changed != FALSE);
		ianjuta_variable_debugger_evaluate (IANJUTA_VARIABLE_DEBUGGER (data->tree->debugger), data->name, gdb_var_evaluate_expression, tData, NULL);
	}
	else
	{
		refresh = data->modified;
		data->modified = FALSE;
	}
	
	/* update children */
	for (search = gtk_tree_model_iter_children(model, &child, iter);
		search == TRUE;
	    search = gtk_tree_model_iter_next (model, &child))
	{
		if (debug_tree_update (tree, &child, force))
		{
			refresh = data->modified == TRUE;
			data->modified = TRUE;
		}
	}

	if (refresh)
	{
		GtkTreePath *path;
		path = gtk_tree_model_get_path (model, iter);
		gtk_tree_model_row_changed (model, path, iter);
		gtk_tree_path_free (path);
	}
	
	return data->modified;
}

void
debug_tree_set_auto_update (DebugTree* this, GtkTreeIter* iter, gboolean state)
{
	GtkTreeModel *model;
	item_data *data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->view));
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	
	if (data != NULL)
	{
		data->auto_update = state;
	}
}

gboolean
debug_tree_get_auto_update (DebugTree* this, GtkTreeIter* iter)
{
	GtkTreeModel *model;
	item_data *data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->view));
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	
	
	if (data != NULL)
	{
		return data->auto_update;
	}
	else
	{
		return FALSE;
	}
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

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

/* return a pointer to a newly allocated DebugTree object */
DebugTree *
debug_tree_new_with_view (AnjutaPlugin *plugin, GtkTreeView *view)
{
	DebugTree *tree = g_new0 (DebugTree, 1);

	tree->plugin = plugin;
	tree->view = debug_tree_create(tree, view);
	tree->auto_expand = FALSE;

	/* Add this tree in list */
	gTreeList = g_list_prepend (gTreeList, tree);
	
	/* Connect signal */
    g_signal_connect(GTK_TREE_VIEW (tree->view), "row_expanded", G_CALLBACK (on_treeview_row_expanded), tree);
	

	return tree;
}


/* return a pointer to a newly allocated DebugTree object */
DebugTree *
debug_tree_new (AnjutaPlugin* plugin)
{
	return debug_tree_new_with_view (plugin, NULL);
}

/* DebugTree destructor */
void
debug_tree_free (DebugTree * d_tree)
{
	AnjutaUI *ui;
	
	g_return_if_fail (d_tree);

	debug_tree_remove_all (d_tree);

	/* Remove from list */
	gTreeList = g_list_remove (gTreeList, d_tree);
	
	g_signal_handlers_disconnect_by_func (GTK_TREE_VIEW (d_tree->view),
				  G_CALLBACK (on_treeview_row_expanded), d_tree);
	
	gtk_widget_destroy (d_tree->view);
	
	
	g_free (d_tree);
}

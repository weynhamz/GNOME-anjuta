#include <string.h>
#include "debug_tree.h"
#include "debugger.h"

static void debug_tree_on_select_row(GtkCList* list, gint row, gint column, GdkEvent * event, gpointer data);
static void parse_pointer_cbs(GList* list, gpointer data);

/* return a pointer to a newly allocated DebugTree object */
/* param: container - container for new object */
/* param: title - title of tree root */
DebugTree* debug_tree_create(GtkWidget* container, gchar* title)
{
	gchar* root_name[1];
	DebugTree* d_tree = g_malloc(sizeof(DebugTree));
	root_name[0] = g_strdup(title);
	
	d_tree->tree = gtk_ctree_new(1,0);
    gtk_widget_show(d_tree->tree);
	gtk_container_add(GTK_CONTAINER(container), d_tree->tree);

	d_tree->root = gtk_ctree_insert_node(GTK_CTREE(d_tree->tree),
                                         NULL,              /* parent */
                                         NULL,              /* sibling */
                                         &root_name[0],
                                         5,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         FALSE,             /* is leaf */
                                         TRUE);
	d_tree->subtree = NULL;
	gtk_signal_connect(GTK_OBJECT(d_tree->tree), "select_row", GTK_SIGNAL_FUNC(debug_tree_on_select_row),d_tree);
	return d_tree;
}


/* clear the display of the debug tree */
void debug_tree_clear(DebugTree* d_tree)
{
	g_return_if_fail(d_tree && d_tree->tree);
	gtk_clist_clear(GTK_CLIST(d_tree->tree));
}

/* parse debugger output into the debug tree */
/* param: d_tree - debug tree object */
/* param: list - debugger output */
/* param: current_root - root where parsing should start */
void debug_tree_parse_variables(DebugTree* d_tree, GList* list, GtkCTreeNode* current_root)
{
	gint len;
    GtkCTreeNode* node;
    GSList* t;
    gint node_type = LEAF;
    gchar* str[1];
    GSList* stack = NULL;

    stack = g_slist_append(stack,current_root);

    while (list)
	{
		str[0] = g_strstrip(g_strdup((gchar*)list->data));
        len = strlen(str[0]);

		/* if this is the end of a subtree, pop the root from the stack and skip to next entry */
        if ( str[0][len - 1] == '}' ||
             (len >= 3 && str[0][len - 2] == ',' && str[0][len - 3] == '}') )
        {
			g_print("End of subtree: %d [%s]\n",len,str[0]);
            t = g_slist_last(stack);
            g_slist_remove_link(stack,t);
            g_slist_free_1(t);
            t = g_slist_last(stack);
            current_root = (GtkCTreeNode*)t->data;
            list = g_list_next(list);
            continue;
        }

		/* check if this is the start of a new subtree */
        if (str[0][len - 1] == '{')
        {
			/* subtree like that: '$1 = {' we want to ignore - means nothing */
			if (str[0][0] == '$')
			{
				list = g_list_next(list);
				continue;	
			}
			/* real subtree - just mark it for later */
			node_type = SUBTREE;
            str[0][len - 1] = ' ';
        }
		
		if (node_type == LEAF)
			if (str[0][len - 1] == ',')
				str[0][len - 1] = ' ';
		
        node = gtk_ctree_insert_node(GTK_CTREE(d_tree->tree),
                                     current_root,
                                     NULL,
                                     &str[0],
                                     5,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     FALSE,		/* assume all nodes are not leaf (in case they are pointers) */
                                     TRUE); 

		/* new subtree - push it to stack as a root for the following items */
        if (node_type == SUBTREE)
        {
			g_print("Subtree: %d [%s]\n",len,str[0]);
            stack = g_slist_append(stack,node);
            current_root = node;
        }
        else if (node_type == LEAF)
        {
			g_print("Leaf: %d [%s]\n",len,str[0]);
        }
        node_type = LEAF;
        list = g_list_next (list);
	}
}


/* callback from debugger when double clicking a pointer */
static void parse_pointer_cbs(GList* list, gpointer data)
{
	DebugTree* d_tree = (DebugTree*)data;
	debug_tree_parse_variables(d_tree,list,d_tree->subtree);	
	d_tree->subtree = NULL;
}

static void debug_tree_on_select_row(GtkCList* list, gint row, gint column, GdkEvent * event, gpointer data)
{
	GtkCTreeNode* node = NULL;
	DebugTree* d_tree = (DebugTree*)data;
	GtkCTreeRow* rowp;
	gchar* gdb_buff = NULL;
	gchar* node_text = NULL;
	gchar* parent_text = NULL;
	gchar* tmp = NULL;
	gchar** cur_array = NULL;
	gchar** parent_array = NULL;
	gboolean first = TRUE;
	
	g_return_if_fail(d_tree != NULL);
	g_return_if_fail(d_tree->tree != NULL);
	g_return_if_fail(event != NULL);
	
	/* only when double clicking */
	if (!(event->type == GDK_2BUTTON_PRESS)) 
		return;
		
	node = gtk_ctree_node_nth(GTK_CTREE(d_tree->tree),row);		/* extract node */
	g_return_if_fail(node != NULL);	
	rowp = GTK_CTREE_ROW(node);									/* extract row */
	g_return_if_fail(rowp != NULL);
	gtk_ctree_get_node_info(GTK_CTREE(d_tree->tree),node,&node_text,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	cur_array = g_strsplit(node_text," ",1);
	gdb_buff = cur_array[0];
	d_tree->subtree = node; 					/* save current node - would be used as root by the callback */
	node = rowp->parent;
	
	while (node != d_tree->root)
	{
		gtk_ctree_get_node_info(GTK_CTREE(d_tree->tree),node,&parent_text,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
		parent_array = g_strsplit(parent_text," ",1);
		tmp = gdb_buff;
		gdb_buff = g_strconcat(parent_array[0],".",tmp,NULL);
		g_strfreev(parent_array);
		/* on the first time tmp is pointing to cur_array which is deleted at the end of the function */
		if (first)
			first = FALSE;
		else
			g_free(tmp);
		rowp = GTK_CTREE_ROW(node);
		node = rowp->parent;
	}		

	gdb_buff = g_strconcat("print *",gdb_buff,NULL);
	g_print("GDB command: %s\n",gdb_buff);	
	debugger_put_cmd_in_queqe("set print pretty on", 0, NULL, NULL);
    debugger_put_cmd_in_queqe("set verbose off", 0, NULL, NULL);
    debugger_put_cmd_in_queqe(gdb_buff,0,parse_pointer_cbs,d_tree);
    debugger_put_cmd_in_queqe("set verbose on", 0, NULL, NULL);
    debugger_put_cmd_in_queqe("set print pretty off", 0, NULL, NULL);
    debugger_execute_cmd_in_queqe();
    g_free(gdb_buff);
	g_strfreev(cur_array);
}

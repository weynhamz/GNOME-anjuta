#include <string.h>
#include "debug_tree.h"
#include "debugger.h"

static void debug_tree_on_select_row (GtkCList * list, gint row, gint column,
				      GdkEvent * event, gpointer data);
static void parse_pointer_cbs (GList * list, gpointer data);
static void debug_tree_update_single_var_cbs (GList * list, gpointer data);
static void debug_tree_update_current (DebugTree * d_tree);
static gchar *get_full_var_name (DebugTree * d_tree, GtkCTreeNode * node);
static void debug_tree_data_dtor (gpointer data);
static void debug_tree_init (DebugTree * d_tree);
static gboolean is_end_subtree (gchar * s, int len);
static gboolean is_start_subtree (gchar * s, int len);
static gboolean is_pointer (gchar * s);
static void parse_var_and_value (gchar * s, gchar * t[]);

/* private data of a node */
struct debug_tree_data {
	gint node_type;
};

/* private data for debug_tree_update_current() */
struct parse_private_data {
	GtkWidget* tree;
	GtkCTreeNode* node;
};


/* return a pointer to a newly allocated DebugTree object */
/* @param: container - container for new object */
/* @param: title - title of tree root */
DebugTree *
debug_tree_create (GtkWidget * container, gchar * title)
{
	char *tree_title[] = { "Variable", "Value" };
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
			    GTK_SIGNAL_FUNC (debug_tree_on_select_row),
			    d_tree);
	return d_tree;
}


/* DebugTree destructor */
void
debug_tree_destroy (DebugTree * d_tree)
{
	gtk_widget_destroy (d_tree->tree);
	g_free (d_tree);
}


/* clear the display of the debug tree and reset the title */
void
debug_tree_clear (DebugTree * d_tree)
{
	g_return_if_fail (d_tree != NULL && d_tree->tree != NULL);
	gtk_clist_clear (GTK_CLIST (d_tree->tree));
	debug_tree_init (d_tree);
}


/* parse debugger output into the debug tree */
/* param: d_tree - debug tree object */
/* param: list - debugger output */
/* param: current_root - root where parsing should start */
void
debug_tree_parse_variables (DebugTree * d_tree, GList * list,
			    GtkCTreeNode * current_root,
			    gboolean parse_pointer)
{
	GSList *stack = NULL;	/* used for keeping track of subtrees as we 
				 * descend into structures */
	gchar **array = NULL;

	/* when parsing a pointer we always perform this function. 
	 * on other times we want to tell between the first entry into a
	 * function and all the rest */
	if (!parse_pointer)
	{
		const gchar *frame = NULL;

		frame = debugger_get_last_frame ();

		if (frame)
		{
			/* split frame to get only the function name */
			array = g_strsplit (frame, ")", 2);
			frame = array[0];
		}

		/* on a new function entry - perform this function */
		if (!frame || !d_tree->current_frame ||
		    g_strcasecmp (d_tree->current_frame, frame) != 0)
		{
			g_free (d_tree->current_frame);
			d_tree->current_frame = NULL;
			debug_tree_clear (d_tree);
			/* reset the root parameter - changed after the clear */
			current_root = d_tree->root;
			if (frame)
				d_tree->current_frame = g_strdup (frame);
			g_strfreev (array);
		}
		else	/* while in same function - just update current view */
		{
			g_strfreev (array);
			debug_tree_update_current (d_tree);
			return;
		}
	}

	/* build a stack of subtrees (nodes that begin a struct) 
	 * start with the root */
	stack = g_slist_append (stack, current_root);

	while (list)
	{
		gchar *str[2];
		gint len;
		struct debug_tree_data *node_data;
		GtkCTreeNode *node;
		GSList *t;
		gint node_type;

		node_type = LEAF;	/* always default to a LEAF */

		/* break debugger output into a variable and value
		 * pair */
		parse_var_and_value (list->data, str);

		len = strlen (str[1]);

		/* if this is the end of a subtree, pop the root from 
		 * the stack and skip to next entry */
		if (is_end_subtree (str[1] /*tmp */ , len))
		{
		/*	g_print ("End of subtree: %d [%s]\n", len, tmp);*/
			t = g_slist_last (stack);
			g_slist_remove_link (stack, t);
			g_slist_free_1 (t);
			t = g_slist_last (stack);
			current_root = (GtkCTreeNode *) t->data;
			list = g_list_next (list);
			g_free (str[0]);
			g_free (str[1]);
			continue;
		}

		/* check if this is the start of a new subtree */
		if (is_start_subtree (str[1], len))
		{
			/* subtree like that: '$1 = {' we want to ignore - 
			   means nothing */
			if (str[0][0] == '$')
			{
				list = g_list_next (list);
				g_free (str[0]);
				g_free (str[1]);
				continue;
			}
		
			/* real subtree - just mark it for later */
			node_type = SUBTREE;

			/* remove '{' only when this is not a multi
			 * dimensional array */
			if (!(len >= 2 && str[1][len - 2] == '{'))
				str[1][len - 1] = 0;
			/*g_print ("Subtree: %s = %s\n", str[0], str[1]);*/
		}
		else if (is_pointer (str[1]))
			node_type = POINTER;

		if (node_type != SUBTREE)
			if (str[1][len - 1] == ',')
				str[1][len - 1] = 0;

		/* save the node type as private node data */
		node_data = g_malloc (sizeof (struct debug_tree_data));
		node_data->node_type = node_type;

		/* insert node into the tree */
		node = gtk_ctree_insert_node (GTK_CTREE (d_tree->tree), 
					      current_root, 
					      NULL, str, 5, NULL, NULL, 
					      NULL, NULL, 
					      FALSE,  /* assume all nodes are 
							not leafs (in case 
							they are pointers) */
					      TRUE);

		/* first node - update DebugTree object with new root */
		if (!d_tree->root)
			d_tree->root = node;

		/* set the node's private data */
		gtk_ctree_node_set_row_data_full (GTK_CTREE (d_tree->tree),
						  node, node_data,
						  debug_tree_data_dtor);

		/* new subtree - push it to stack as a root for the following 
		items */
		if (node_type == SUBTREE)
		{
			stack = g_slist_append (stack, node);
			/* root for next entry is this new substree */
			current_root = node;	
		}
		/*else if (node_type == LEAF)
			g_print ("Leaf: %s = %s\n", str[0], str[1]);
		else if (node_type == POINTER)
			g_print ("Pointer: %s = %s\n", str[0], str[1]);*/

		/* get next debugger output line */
		list = g_list_next (list);
	}
}


/* parse the given string into an array with the */
/* variable on the first slot and the value on the */
/* second. does it by searching for '=' */
/* caller must free the array 't' !!! */
/* @param: s debugger output string */
/* @param: returned array with var and value */
static void
parse_var_and_value (gchar * s, gchar * t[])
{
	int i;
	gboolean found = FALSE;	/* flag whether '=' was found */
	int len = strlen (s);

	/* no place for '=' - must be the end of a subtree 
	 * all the string goes back as value */
	if (len < 2)
	{
		t[0] = NULL;
		t[1] = g_strstrip (g_strdup (s));
		return;
	}

	/* search for ' = ' */
	for (i = 0; i < len - 2; i++)
	{
		if (s[i] == ' ' && s[i + 1] == '=' && s[i + 2] == ' ')
		{
			found = TRUE;
			break;
		}
	}

	/* '=' not found - all the string goes back as value */
	if (!found)
	{
		t[0] = NULL;
		t[1] = g_strstrip (g_strdup (s));
		return;
	}

	/* '=' found - cut the string into the two halves */
	t[0] = g_strstrip (g_strndup (s, i + 1));
	t[1] = g_strstrip (g_strndup (s + i + 2, len - i - 2));
}


/* return true if line is the end of a subtree */
/* @param: s gdb output line */
/* @param: len length of s */
static gboolean
is_end_subtree (gchar * s, int len)
{
	return ((s[0] == '}' || s[len - 1] == '}' ||
		 (len >= 3 && s[len - 2] == ',' && s[len - 3] == '}'))
		&& (s[len - 1] != '{'));
}


/* return true if line is the start of a new subtree */
/* @param: s gdb output line */
/* @param: len length of s */
static gboolean
is_start_subtree (gchar * s, int len)
{
	return ((s[len - 1] == '{') && (s[0] != '}'));
}


/* return true if value represents pointer */
/* @param: s value extracted by parse_var_and_value() */ 
static gboolean
is_pointer (gchar * s)
{
	return (s[0] == '(');
}


/* callback from debugger when double clicking a pointer */
static void
parse_pointer_cbs (GList * list, gpointer data)
{
	DebugTree *d_tree = (DebugTree *) data;
	debug_tree_parse_variables (d_tree, list, d_tree->subtree, TRUE);
	d_tree->subtree = NULL;
}


/* a "double click" signal handler for the debug_tree */
static void
debug_tree_on_select_row (GtkCList * list, gint row, gint column,
			  GdkEvent * event, gpointer data)
{
	GtkCTreeNode *node = NULL;
	DebugTree *d_tree = (DebugTree *) data;
	gchar *gdb_buff = NULL;
	struct debug_tree_data *node_data = NULL;

	g_return_if_fail (d_tree != NULL);
	g_return_if_fail (d_tree->tree != NULL);
	g_return_if_fail (event != NULL);

	if (debugger_is_active () == FALSE)
		return;

	/* only when double clicking */
	if (!(event->type == GDK_2BUTTON_PRESS))
		return;

	/* extract node */
	node = gtk_ctree_node_nth (GTK_CTREE (d_tree->tree), row);
	g_return_if_fail (node != NULL);

	/* get the node's private data */
	node_data =
		gtk_ctree_node_get_row_data (GTK_CTREE (d_tree->tree), node);

	/* prevents getting strange results when dereferencing numeric
	 * values of non pointer variables */
	if (node_data->node_type != POINTER)
		return;

	gdb_buff = get_full_var_name (d_tree, node);

	d_tree->subtree = node;	/* save current node - would be used as root by 
				 * the callback */

	gdb_buff = g_strconcat ("print *", gdb_buff, NULL);
	debugger_put_cmd_in_queqe ("set print pretty on", 0, NULL, NULL);
	debugger_put_cmd_in_queqe ("set verbose off", 0, NULL, NULL);
	debugger_put_cmd_in_queqe (gdb_buff, 0, parse_pointer_cbs, d_tree);
	debugger_put_cmd_in_queqe ("set verbose on", 0, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", 0, NULL, NULL);
	debugger_execute_cmd_in_queqe ();
	g_free (gdb_buff);
}


/* updates the current values displayed in the debug tree.
 * used for keeping the current state of tree nodes (expanded/
 * not expanded) as the user left it */
static void
debug_tree_update_current (DebugTree * d_tree)
{
	GtkCTreeNode *node = NULL;
	gboolean do_execute = FALSE;
	gchar *gdb_buff = NULL;
	struct parse_private_data *private_data;
	struct debug_tree_data *node_data;
	int i = 0;

	/* extract first node */
	node = gtk_ctree_node_nth (GTK_CTREE (d_tree->tree), 0);

	g_return_if_fail (node != NULL);

	/* go from the first row to the last row */
	while (node)
	{
		node_data =
			gtk_ctree_node_get_row_data (GTK_CTREE (d_tree->tree),
						     node);
		if (node_data->node_type != SUBTREE)
		{
			do_execute = TRUE;
			gdb_buff = get_full_var_name (d_tree, node);
			gdb_buff = g_strconcat ("print ", gdb_buff, NULL);
			private_data =
				g_malloc (sizeof (struct parse_private_data));
			private_data->tree = d_tree->tree;
			private_data->node = node;
			/*g_print("%s\n",gdb_buff); */
			debugger_put_cmd_in_queqe (gdb_buff, 0,
						   debug_tree_update_single_var_cbs,
						   private_data);
			g_free (gdb_buff);
		}
		i++;
		/* extract next node */
		node = gtk_ctree_node_nth (GTK_CTREE (d_tree->tree), i);
	}

	/* execute a series of "print" only for the leafs/pointers 
	   in the current view */
	if (do_execute == TRUE)
		debugger_execute_cmd_in_queqe ();
}


/* a debugger callback for updating the value of a single entry in the 
   current view */
static void
debug_tree_update_single_var_cbs (GList * list, gpointer data)
{
	struct parse_private_data *private_data =
		(struct parse_private_data *) data;
	struct debug_tree_data *node_data;
	gchar *new_data[2];

	g_return_if_fail (list != NULL);
	g_return_if_fail (data != NULL);

	parse_var_and_value (list->data, new_data);
	gtk_ctree_node_set_text (GTK_CTREE (private_data->tree),
				 private_data->node, 1, new_data[1]);
	if (is_pointer (new_data[1]))
	{
		node_data =
			gtk_ctree_node_get_row_data (GTK_CTREE
						     (private_data->tree),
						     private_data->node);
		node_data->node_type = POINTER;
	}

	g_free (private_data);
	g_free (new_data[0]);
}


/*  return the "full path" of a variable
 *  caller must free the returned string
 *  @param: d_tree - Debug tree object
 *  @param: node - Node where the variable lives */
static gchar *
get_full_var_name (DebugTree * d_tree, GtkCTreeNode * node)
{
	GtkCTreeRow *rowp = NULL;
	gchar *gdb_buff = NULL;
	gchar *tmp = NULL;
	gboolean is_primitive = FALSE;
	gchar *text;

	rowp = GTK_CTREE_ROW (node);	/* extract row */
	g_return_val_if_fail (rowp != NULL, NULL);

	gtk_ctree_node_get_pixtext (GTK_CTREE (d_tree->tree), node, 0, &text,
				    NULL, NULL, NULL);

	if (text[0] == '$')
	{
		is_primitive = TRUE;
		gdb_buff = NULL;
	}
	else
		gdb_buff = g_strdup (text);

	node = rowp->parent;

	/* go from the node to the root */
	while (node)
	{
		gtk_ctree_node_get_pixtext (GTK_CTREE (d_tree->tree), node, 0,
					    &text, NULL, NULL, NULL);
		tmp = gdb_buff;
		gdb_buff = g_strconcat (text, ".", tmp, NULL);
		g_free (tmp);

		rowp = GTK_CTREE_ROW (node);
		node = rowp->parent;	/* jump to parent */
	}

	if (is_primitive == TRUE)
	{
		tmp = gdb_buff;
		gdb_buff = g_strconcat ("*", tmp, NULL);
		gdb_buff[strlen (gdb_buff) - 1] = 0;
		g_free (tmp);
	}

	/*g_print ("get_full_var_name: %s\n", gdb_buff); */

	return gdb_buff;
}


/* destructor of the private data of a debug tree node */
static void
debug_tree_data_dtor (gpointer data)
{
	struct debug_tree_data *node_data = (struct debug_tree_data *) data;
	g_free (node_data);
}


/* initializes an existing debug tree object. */
static void
debug_tree_init (DebugTree * d_tree)
{
	d_tree->root = NULL;
	d_tree->subtree = NULL;
	d_tree->current_frame = NULL;
}

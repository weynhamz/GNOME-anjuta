#ifndef _DEBUG_TREE_H_
#define _DEBUG_TREE_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum { LEAF = 0 , SUBTREE = 1 , POINTER = 2 };

/* The debug tree object */
typedef struct _DebugTree {
	GtkWidget* tree;			/* the tree widget */
	GtkCTreeNode* root;			/* root node of the tree */
	GtkCTreeNode* subtree;		/* saves the current context between async calls to the debugger */
	gchar* current_frame;		/* current stack frame */
} DebugTree;



DebugTree* debug_tree_create(GtkWidget* container, gchar* title);
void debug_tree_destroy(DebugTree* d_tree);
void debug_tree_clear(DebugTree* tree);
void debug_tree_parse_variables(DebugTree* tree, GList* list, GtkCTreeNode* root, gboolean parse_pointer);

#ifdef __cplusplus
}
#endif

#endif

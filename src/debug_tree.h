#ifndef _DEBUG_TREE_H_
#define _DEBUG_TREE_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum { LEAF = 0 , SUBTREE = 1 , POINTER = 2 };
typedef struct _DebugTree {
	GtkWidget* tree;
	GtkCTreeNode* root;
	GtkCTreeNode* subtree;
} DebugTree;


DebugTree* debug_tree_create(GtkWidget* container, gchar* title);
void debug_tree_clear(DebugTree* tree);
void debug_tree_parse_variables(DebugTree* tree, GList* list, GtkCTreeNode* root);

#ifdef __cplusplus
}
#endif

#endif

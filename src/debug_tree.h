#ifndef _DEBUG_TREE_H_
#define _DEBUG_TREE_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* The debug tree object */
typedef struct _DebugTree {
	
	GtkWidget* tree;        /* the tree widget */
	// GtkTreeIter* cur_node;
	// GtkWidget* middle_click_menu;

} DebugTree;

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

struct _TrimmableItem
{
  DataType dataType;
  gchar *name;
  gchar *value;
  gboolean expandable;
  gboolean expanded;
  gboolean analyzed;
  gint display_type;	
};

typedef struct _Parsepointer Parsepointer;

struct _Parsepointer
{
  GtkTreeView *tree;
  GtkTreeIter *node;
  GList *next;
  gboolean is_pointer;
};

DebugTree* debug_tree_create (GtkWidget* container);
void debug_tree_destroy (DebugTree* d_tree);
void debug_tree_clear (DebugTree* tree);
void debug_tree_parse_variables (DebugTree* tree, GList* list);

#ifdef __cplusplus
}
#endif

#endif

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

#ifndef _DEBUG_TREE_H_
#define _DEBUG_TREE_H_

#include <gtk/gtk.h>
#include "debugger.h"

G_BEGIN_DECLS

/* The debug tree object */
typedef struct _DebugTree {
	Debugger *debugger;
	GtkWidget* tree;        /* the tree widget */
	GtkTreeIter* cur_node;
	GtkWidget* middle_click_menu;

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
	gboolean modified;
	gint display_type;	
};

typedef struct _Parsepointer Parsepointer;

struct _Parsepointer
{
	DebugTree *d_tree;
	GtkTreeView *tree;
	GtkTreeIter *node;
	GList *next;
	gboolean is_pointer;
};

DebugTree* debug_tree_create (Debugger *debugger);
void debug_tree_destroy (DebugTree* d_tree);
void debug_tree_clear (DebugTree* tree);
void debug_tree_parse_variables (DebugTree* tree, const GList* list);

G_END_DECLS

#endif

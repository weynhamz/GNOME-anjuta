/*
    stack_trace.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>
#include "resources.h"
#include "debugger.h"
#include "utilities.h"
#include "pixmaps.h"

/* Including pixmaps at compile time */
/* So that these at least work when gnome pixmaps are not found */
#include "../pixmaps/pointer.xpm"
#include "../pixmaps/blank.xpm"

static void add_frame (StackTrace * st, gchar *);

GdkPixbuf *pointer_pix;


StackTrace *
stack_trace_new ()
{
	StackTrace *st;

	st = g_malloc (sizeof (StackTrace));
	if (st)
	{
		create_stack_trace_gui (st);
		st->current_frame = 0;
		st->current_index = 0;
		st->current_frame_iter = NULL;
		st->current_index_iter = NULL;
		
		pointer_pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_POINTER);
	}
	return st;
}

void
stack_trace_clear (StackTrace * st)
{
	GtkTreeModel* model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));

	gtk_list_store_clear (GTK_LIST_STORE (model));

	st->current_frame = 0;
	st->current_index = 0;
	st->current_frame_iter = NULL;
	st->current_index_iter = NULL;
}

void
stack_trace_update (GList * outputs, gpointer data)
{
	StackTrace *st;
	gchar *ptr;
	gfloat adj_value;
	GtkAdjustment *adj;
	GList *list, *node;

	st = (StackTrace *) data;
	list = remove_blank_lines (outputs);
	stack_trace_clear (st);

	if (g_list_length (list) < 1)
	{
		g_list_free (list);
		return;
	}

	node = list->next;
	ptr = g_strdup ((gchar *) list->data);
	while (node)
	{
		gchar *line = (gchar *) node->data;
		node = g_list_next (node);
		if (isspace (line[0]))	/* line break */
		{
			gchar *tmp;
			tmp = ptr;
			ptr = g_strconcat (tmp, line, NULL);
			g_free (tmp);
		}
		else
		{
			add_frame (st, ptr);
			g_free (ptr);
			ptr = g_strdup (line);
		}
	}
	if (ptr)
	{
		add_frame (st, ptr);
		g_free (ptr);
		ptr = NULL;
	}

	g_list_free (list);
}

static void
add_frame (StackTrace * st, gchar * line)
{
	gchar frame_no[10], *dummy_fn;
	gint count;
	GdkColor blue = { 16, 0, 0, -1 };
	GdkColor red = { 16, -1, 0, 0 };
	GtkTreeModel *model;
	GtkTreeIter iter;
	GdkPixbuf *pic;

	count = sscanf (line, "#%s", frame_no);
	if (count != 1)
		return;
	while (!isspace (*line))
		line++;
	while (isspace (*line))
		line++;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));	

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	count = atoi(frame_no);

	/* if we are on the current frame set iterator and pixmap correctly */
	if (count == st->current_frame) {
		if (st->current_frame_iter) 
			gtk_tree_iter_free (st->current_frame_iter);
		st->current_frame_iter = gtk_tree_iter_copy (&iter);
		pic = pointer_pix;
	}
	else
		pic = NULL;

	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
					   STACK_TRACE_ACTIVE_COLUMN, pic,
					   STACK_TRACE_COUNT_COLUMN, g_strdup (frame_no), 
					   STACK_TRACE_FRAME_COLUMN, g_strdup (line), -1);
}


void
stack_trace_destroy (StackTrace * st)
{
	if (st)
	{
		stack_trace_clear (st);
		gtk_widget_unref (st->widgets.clist);
		gtk_widget_unref (st->widgets.menu);
		gtk_widget_unref (st->widgets.menu_set);
		gtk_widget_unref (st->widgets.menu_info);
		gtk_widget_unref (st->widgets.menu_update);
		gtk_widget_unref (st->widgets.menu_view);

		g_free (st);
	}
}

void
stack_trace_update_controls (StackTrace * st)
{
	gboolean A, R;

	A = debugger_is_active ();
	R = debugger_is_ready ();

	gtk_widget_set_sensitive (st->widgets.menu_set, A && R);
	gtk_widget_set_sensitive (st->widgets.menu_info, A && R);
	gtk_widget_set_sensitive (st->widgets.menu_update, A && R);
	gtk_widget_set_sensitive (st->widgets.menu_view, A && R);
}

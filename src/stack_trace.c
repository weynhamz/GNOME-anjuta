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

typedef struct _StackTraceGui StackTraceGui;
struct _StackTraceGui
{
  GtkWidget *clist;
  GtkWidget *menu;
  GtkWidget *menu_set;
  GtkWidget *menu_info;
  GtkWidget *menu_update;
  GtkWidget *menu_view;
};

struct _StackTrace
{
  StackTraceGui widgets;
  gint current_frame;
  gint current_index;
  GtkTreeIter *current_frame_iter;
  GtkTreeIter *current_index_iter;
};

enum {
	STACK_TRACE_ACTIVE_COLUMN,
	STACK_TRACE_COUNT_COLUMN,
	STACK_TRACE_FRAME_COLUMN,
	STACK_TRACE_N_COLUMNS
};

/* Pointer pixbuf */
GdkPixbuf *pointer_pix;

/*
 * returns the current stack frame or -1 on error
 */
static gint
get_index_from_iter (StackTrace* st, GtkTreeIter* iter)
{
	GtkTreeModel *model;		
	gchar* count = NULL;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));		

	gtk_tree_model_get (model, iter, STACK_TRACE_COUNT_COLUMN, &count, -1);
	
	return atoi(count);
}

static gboolean 
get_current_iter (StackTrace* st, GtkTreeIter* iter)
{
	GtkTreeView *view;
	GtkTreeSelection *selection;
	gchar* count = NULL;
	
	view = GTK_TREE_VIEW (st->widgets.clist);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, iter))
	{
		g_warning("Error getting selection\n");
		return FALSE;
	}

	return TRUE;
}

/*
 * extract the source file and line number from a stack frame
 */
static gboolean
get_file_and_line_from_frame (gchar* frame, gchar** file, glong* lineno)
{   
	int line;
	char* end;
	char* colon;

	/* sanity check */
	if (!frame)
		return FALSE;
    
	/* we want the end of the frame; init to the beginning */
	end = frame;
	
	/* search for the end of the string */
	while (*end)
    		end++;
	
	/* end reached - take one step back to the last char */
	end--;

	/* skip any spaces that come after the actual text */
	while (isspace (*end))
		end--;
	
	/* make sure we hit the end of the line number */
	if (!isdigit (*end)) 
		return FALSE;

	/* find the beginning of the line number */
	while (isdigit (*end))
		end--;

	/* between the file name and line number must be a colon */
	if (*end != ':') 
		return FALSE;
    
	/* save the colon's position */
	colon = end;
	
	/* extract line number */
	*lineno = atol (end+1);

	/* find beginning of file name */

	while (!isspace (*end))
		end--;

	/* one step forward to be on the beginning of the file name */
	end++;
	
	/* extract file name */
	*file = g_strndup (end, colon-end);
	
	return TRUE;
}

static void
stack_trace_args_cbs (GList *outputs, gpointer data)
{
	char* cmd = "info args";
	
	/* we get here after we set the correct frame */
	
	debugger_put_cmd_in_queqe (cmd, 0, debugger_dialog_message, NULL);
	debugger_execute_cmd_in_queqe ();	
}

static void 
stack_trace_set_active_cbs (GList * outputs, gpointer data)
{
	GtkTreeModel *model;	

	StackTrace *st = (StackTrace*) data;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));	

	/* set pointer on this frame */
	gtk_list_store_set (GTK_LIST_STORE(model), st->current_frame_iter, 
					   STACK_TRACE_ACTIVE_COLUMN, pointer_pix, -1);		
}

static gboolean
on_stack_trace_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	StackTrace *st = (StackTrace*) user_data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		/* update the current index iterator and number in the stack
		   trace object */
		GtkTreeIter iter;
		GdkEventButton *bevent;
		
		if (!get_current_iter (st,&iter))
			return FALSE;
		
		gtk_tree_iter_free (st->current_index_iter);
		st->current_index_iter = gtk_tree_iter_copy (&iter);		
		st->current_index = get_index_from_iter (st,&iter);
				
		bevent = (GdkEventButton *) event;
		if (bevent->button != 3)
			return FALSE;
		bevent->button = 1;
		stack_trace_update_controls (st);
		gtk_menu_popup (GTK_MENU (st->widgets.menu), NULL, NULL, NULL, NULL,
				bevent->button, bevent->time);
		return TRUE;
	}
	else
		return FALSE;
}

static void
on_stack_frame_set_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace *st;
	gchar* cmd = "frame ";
	gchar current_index_no[16];
	GtkTreeModel *model;
	GtkTreeView *view;
	
	st = (StackTrace*) user_data;
	
	/* current frame is already active */
	if (st->current_index == st->current_frame)
		return;

	view = GTK_TREE_VIEW (st->widgets.clist);
	model = gtk_tree_view_get_model (view);	
	
	/* clear pixmap on the previous active frame */
	gtk_list_store_set(GTK_LIST_STORE (model), st->current_frame_iter, 
					   STACK_TRACE_ACTIVE_COLUMN, NULL, -1);			
	
	/* issue a command to switch active frame to new location */
	sprintf (current_index_no, "%d", st->current_index);	
	cmd = g_strconcat (cmd,current_index_no, NULL);	
	g_print ("Command: %s\n", cmd);
	
	/* make current index and frame the same in the stack trace object */
	st->current_frame = st->current_index;
	gtk_tree_iter_free (st->current_frame_iter);
	st->current_frame_iter = gtk_tree_iter_copy (st->current_index_iter);

	debugger_put_cmd_in_queqe (cmd, 0, stack_trace_set_active_cbs , st);
	debugger_execute_cmd_in_queqe ();
}

static void
on_stack_frame_info_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace *st;	
	gchar* cmd = "info frame ";
	gchar frame_no[16];
	
	st = (StackTrace*) user_data;
	
	sprintf (frame_no,"%d",st->current_index);
	cmd = g_strconcat (cmd, frame_no, NULL);
	
	g_print ("Command: %s\n",cmd);
	
	debugger_put_cmd_in_queqe (cmd, 0, debugger_dialog_message, NULL);
	debugger_execute_cmd_in_queqe ();
	g_free (cmd);
}

static void
on_stack_frame_args_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	StackTrace *st;
	gchar* cmd = "frame ";
	gchar frame_no[16];
	
	st = (StackTrace*) user_data;

	sprintf (frame_no, "%d", st->current_index);	
	cmd = g_strconcat (cmd, frame_no, NULL);
	
	g_print ("Command: %s\n", cmd);
	
	debugger_put_cmd_in_queqe (cmd, 0, stack_trace_args_cbs, st);
	debugger_execute_cmd_in_queqe ();
	g_free (cmd);
}

static void
on_stack_update_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace* st = (StackTrace*) user_data;
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE, stack_trace_update, st);
	debugger_execute_cmd_in_queqe ();
}

static void
on_stack_view_src_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar* frame = NULL;
	gchar* file;
	glong lineno;
	gboolean ret;
	
	StackTrace* st = (StackTrace*) user_data;		

	view = GTK_TREE_VIEW (st->widgets.clist);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		g_warning(_("Error getting selection\n"));
		return;
	}

	/* get the frame info */
	gtk_tree_model_get (model, &iter, STACK_TRACE_FRAME_COLUMN, &frame, -1);
	
	ret = get_file_and_line_from_frame (frame, &file, &lineno);

	if (!ret)
	{
		g_warning(_("Error getting source location from stack frame"));
		return;
	}
	
	anjuta_goto_file_line (file, lineno);
}

static void
on_stack_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}

/* Context pop up menu */
static GnomeUIInfo stack_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Frame set"),
    NULL,
    on_stack_frame_set_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Frame info"),
    NULL,
    on_stack_frame_info_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Frame args"),
    NULL,
    on_stack_frame_args_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_stack_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("View Source"),
    NULL,
    on_stack_view_src_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Help"),
    NULL,
    on_stack_help_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GtkWidget*
create_stack_menu (StackTrace *st)
{
	GtkWidget *menu4;
	int i;
	int entries = sizeof (stack_menu_uiinfo) / sizeof (GnomeUIInfo);
	
	/* Set all user data in the stack trace menu to the StackTrace
	 * struct parameter
	 */
	for (i = 0; i < entries; i++)
		stack_menu_uiinfo[i].user_data = st;
	
	menu4 = gtk_menu_new ();
	gnome_app_fill_menu (GTK_MENU_SHELL (menu4), stack_menu_uiinfo,
						NULL, FALSE, 0);
	return menu4;
}

static void
on_stack_trace_row_activated           (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        StackTrace      *st)
{
	on_stack_frame_set_activate (NULL, st);
	on_stack_view_src_activate (NULL, st);
}

static void
create_stack_trace_gui(StackTrace *st)
{
	GtkTreeModel* model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;	
	
	model = GTK_TREE_MODEL(gtk_list_store_new (STACK_TRACE_N_COLUMNS,
											   GDK_TYPE_PIXBUF,
											   GTK_TYPE_STRING,
											   GTK_TYPE_STRING));
	
	st->widgets.clist = gtk_tree_view_new_with_model (model);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (st->widgets.clist));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (model));
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Active"));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										STACK_TRACE_ACTIVE_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (st->widgets.clist),
									   column);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Count"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_COUNT_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (st->widgets.clist),
									   column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FRAME_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Frame"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	
	g_signal_connect (st->widgets.clist, "event",
					  G_CALLBACK (on_stack_trace_event), st);  
	g_signal_connect (st->widgets.clist, "row_activated",
					  G_CALLBACK (on_stack_trace_row_activated), st);  
	
	st->widgets.menu = create_stack_menu(st);
	st->widgets.menu_set = stack_menu_uiinfo[0].widget;
	st->widgets.menu_info = stack_menu_uiinfo[1].widget; 
	st->widgets.menu_update = stack_menu_uiinfo[2].widget;
	st->widgets.menu_view = stack_menu_uiinfo[3].widget;
	
	gtk_widget_ref(st->widgets.clist);
	gtk_widget_ref(st->widgets.menu);
	gtk_widget_ref(st->widgets.menu_set);
	gtk_widget_ref(st->widgets.menu_info);
	gtk_widget_ref(st->widgets.menu_update);
	gtk_widget_ref(st->widgets.menu_view);
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

void
stack_trace_set_frame (StackTrace *st, gint frame)
{
	st->current_frame = frame;
}

GtkWidget*
stack_trace_get_treeview (StackTrace *st)
{
	return st->widgets.clist;
}

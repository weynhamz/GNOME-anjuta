/*
    stack_trace_cbs.c
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

#include <gnome.h>
#include "debugger.h"
#include "stack_trace_cbs.h"

static gboolean get_file_and_line_from_frame (gchar* frame, gchar** file, glong* lineno);
static void stack_trace_args_cbs (GList * outputs, gpointer data);
static void stack_trace_set_active_cbs (GList * outputs, gpointer data);
static gint get_index_from_iter (StackTrace* st, GtkTreeIter* iter);
static gboolean get_current_iter (StackTrace* st, GtkTreeIter* iter);

extern GdkPixbuf *pointer_pix;


gboolean
on_stack_trace_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	StackTrace *st = (StackTrace*) user_data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		/* update the current index iterator and number in the stack trace object */
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


void
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


void
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


void
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

void
on_stack_update_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace* st = (StackTrace*) user_data;
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE, stack_trace_update, st);
	debugger_execute_cmd_in_queqe ();
}

void
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

void
on_stack_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{

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

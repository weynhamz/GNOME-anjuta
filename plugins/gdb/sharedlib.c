/*
    sharedlibs.c
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

#include <gnome.h>

#include <libanjuta/resources.h>

#include "debugger.h"
#include "utilities.h"

Sharedlibs* sharedlibs_new()
{
  Sharedlibs* ew;
  ew = g_malloc(sizeof(Sharedlibs));
  if(ew)
  {
     ew->is_showing = FALSE;
	 ew->win_width = 410;
     ew->win_height = 370;
     ew->win_pos_x = 120;
     ew->win_pos_y = 140;
     create_sharedlibs_gui(ew);
  }
  return ew;
}

void
sharedlibs_clear(Sharedlibs *sg)
{
   if(GTK_IS_CLIST(sg->widgets.clist)) gtk_clist_clear(GTK_CLIST(sg->widgets.clist));
}

void
sharedlibs_update( GList *lines, gpointer data)
{
    Sharedlibs *sl;
    gchar obj[512], from[32], to[32], read[32];
    gchar *row[4];
    gint count;
    GList *list, *node;

    sl = (Sharedlibs*)data;

    list = remove_blank_lines(lines);
    sharedlibs_clear(sl);
    if(g_list_length(list) < 2 ){g_list_free(list); return;}
    node = list->next;
    while(node)
    {
        count = sscanf((char*)node->data, "%s %s %s %s", from,to,read,obj);
        node = g_list_next(node);
        if(count != 4) continue;
        row[0]=(gchar*) extract_filename(obj);
        row[1]=from;
        row[2]=to;
        row[3]=read;
        gtk_clist_append(GTK_CLIST(sl->widgets.clist), row);
     }
     g_list_free(list);
}

void
sharedlibs_show(Sharedlibs* ew)
{
  if(ew)
  {
     if(ew->is_showing)
     {
         gdk_window_raise(ew->widgets.window->window);
		 return;
	 }
     gtk_widget_set_uposition(ew->widgets.window, ew->win_pos_x, ew->win_pos_y);
     gtk_window_set_default_size(GTK_WINDOW(ew->widgets.window), ew->win_width, ew->win_height);
     gtk_widget_show(ew->widgets.window);
     ew->is_showing = TRUE;
  }
}

void
sharedlibs_hide(Sharedlibs* ew)
{
  if(ew)
  {
     if(ew->is_showing == FALSE) return;
     gdk_window_get_root_origin(ew ->widgets.window->window, &ew->win_pos_x, &ew->win_pos_y);
     gdk_window_get_size(ew ->widgets.window->window, &ew->win_width, &ew->win_height);
     gtk_widget_hide(ew->widgets.window);
     ew->is_showing = FALSE;
  }
}

gboolean
sharedlibs_save_yourself(Sharedlibs* ew, FILE* stream)
{
	if (!ew) return FALSE;

	if (ew->is_showing)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
		      &ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width, &ew->win_height);
	}
	fprintf(stream, "sharedlibs.win.pos.x=%d\n", ew->win_pos_x);
	fprintf(stream, "sharedlibs.win.pos.y=%d\n", ew->win_pos_y);
	fprintf(stream, "sharedlibs.win.width=%d\n", ew->win_width);
	fprintf(stream, "sharedlibs.win.height=%d\n", ew->win_height);
	return TRUE;
}

/* TODO
gboolean
sharedlibs_load_yourself(Sharedlibs* ew, PropsID props)
{
	if (!ew) return FALSE;
	
	ew->win_pos_x = prop_get_int (props, "sharedlibs.win.pos.x", 120);
	ew->win_pos_y = prop_get_int (props, "sharedlibs.win.pos.y", 140);
	ew->win_width = prop_get_int (props, "sharedlibs.win.width", 410);
	ew->win_height = prop_get_int (props, "sharedlibs.win.height", 370);
	return TRUE;
}
*/

void
sharedlibs_destroy(Sharedlibs* sg)
{
  if(sg)
  {
     sharedlibs_clear(sg);
     gtk_widget_unref(sg->widgets.window);
     gtk_widget_unref(sg->widgets.clist);
     gtk_widget_unref(sg->widgets.menu);
     gtk_widget_unref(sg->widgets.menu_update);
     if(GTK_IS_WIDGET(sg->widgets.window))
              gtk_widget_destroy(sg->widgets.window);
     g_free(sg);
  }
}

void
sharedlibs_update_controls(Sharedlibs* ew)
{
     gboolean A, R;

     A = debugger_is_active();
     R = debugger_is_ready();

     gtk_widget_set_sensitive(ew->widgets.menu_update, A && R);
}

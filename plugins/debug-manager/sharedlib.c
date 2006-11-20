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

#include "utilities.h"
#include "sharedlib.h"

static gint
on_sharedlibs_delete_event (GtkWidget* w, GdkEvent *event, gpointer data)
{
  Sharedlibs* sl = data;
  sharedlibs_hide(sl);
  return TRUE;
}

static gboolean
on_sharedlibs_key_press_event (GtkWidget *widget, GdkEventKey *event,
							  gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		Sharedlibs* sl = data;
  		sharedlibs_hide(sl);
		return TRUE;
	}
	return FALSE;
}


static void
sharedlibs_update (const GList *lines, gpointer data)
{
    Sharedlibs *sl;
    gchar obj[512], from[32], to[32], read[32];
    gchar *row[4];
    gint count;
    GList *list, *node;

    sl = (Sharedlibs*)data;

    list = gdb_util_remove_blank_lines(lines);
    sharedlibs_clear(sl);
    if(g_list_length(list) < 2 ){g_list_free(list); return;}
    node = list->next;
    while(node)
    {
        count = sscanf((char*)node->data, "%s %s %s %s", from,to,read,obj);
        node = g_list_next(node);
        if(count != 4) continue;
        row[0]=(gchar*) g_path_get_basename (obj);
        row[1]=from;
        row[2]=to;
        row[3]=read;
        gtk_clist_append(GTK_CLIST(sl->widgets.clist), row);
     }
     g_list_free(list);
}

static void
on_sharedlibs_update_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	Sharedlibs *sl = (Sharedlibs *)user_data;
	
	ianjuta_debugger_info_sharedlib (sl->debugger, sharedlibs_update, sl, NULL);
}

static void
sharedlibs_update_controls (Sharedlibs* ew)
{
     gboolean R;

     R = ianjuta_debugger_get_status (ew->debugger, NULL) == IANJUTA_DEBUGGER_OK;

     gtk_widget_set_sensitive(ew->widgets.menu_update, R);
}

static gboolean
on_sharedlibs_event (GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	GdkEventButton *bevent;
	Sharedlibs *ew = (Sharedlibs*)user_data;
	
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *)event)->button != 3)
		return FALSE;
	bevent =  (GdkEventButton *)event;
	bevent->button = 1;
	sharedlibs_update_controls(ew);
	gtk_menu_popup (GTK_MENU(ew->widgets.menu), NULL,
					NULL, NULL, NULL,
					bevent->button, bevent->time);
	return TRUE;
}

static GnomeUIInfo sharedlibs_menu_uiinfo[] =
{
	{
		GNOME_APP_UI_ITEM, N_("Update"),
		NULL,
		on_sharedlibs_update_activate, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
	GNOMEUIINFO_END
};

static GtkWidget*
create_sharedlibs_menu (Sharedlibs *sl)
{
	GtkWidget *sharedlibs_menu;

	sharedlibs_menu = gtk_menu_new ();
	sharedlibs_menu_uiinfo[0].user_data = sl;
	gnome_app_fill_menu (GTK_MENU_SHELL (sharedlibs_menu), sharedlibs_menu_uiinfo,
						 NULL, FALSE, 0);
	return sharedlibs_menu;
}

static void
create_sharedlibs_gui (Sharedlibs *sl)
{
	GtkWidget *window3;
	GtkWidget *scrolledwindow4;
	GtkWidget *clist4;
	GtkWidget *label6, *label7, *label8, *label9;
	
	window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize (window3, 170, -2);
	gtk_window_set_title (GTK_WINDOW (window3), _("Shared libraries"));
	gtk_window_set_wmclass (GTK_WINDOW (window3), "sharedlibs", "Anjuta");
	gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);
	gnome_window_icon_set_from_default(GTK_WINDOW(window3));
	
	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow4);
	gtk_container_add (GTK_CONTAINER (window3), scrolledwindow4);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	clist4 = gtk_clist_new (4);
	gtk_widget_show (clist4);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), clist4);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 0, 110);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 1, 90);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 2, 90);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 3, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (clist4), GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist4));
	gtk_clist_set_column_auto_resize (GTK_CLIST(clist4), 0, TRUE);
	
	label6 = gtk_label_new (_("  Shared Object  "));
	gtk_widget_show (label6);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 0, label6);
	
	label7 = gtk_label_new (_("From"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 1, label7);
	
	label8 = gtk_label_new (_("To"));
	gtk_widget_show (label8);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 2, label8);
	
	label9 = gtk_label_new (_("Sysm Read"));
	gtk_widget_show (label9);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 3, label9);
	
	gtk_signal_connect (GTK_OBJECT (window3), "delete_event",
						GTK_SIGNAL_FUNC (on_sharedlibs_delete_event), sl);
	gtk_signal_connect (GTK_OBJECT (window3), "key-press-event",
						GTK_SIGNAL_FUNC (on_sharedlibs_key_press_event), sl);							 
	gtk_signal_connect (GTK_OBJECT (clist4), "event",
						GTK_SIGNAL_FUNC (on_sharedlibs_event),
						sl);
	
	sl->widgets.window = window3;
	sl->widgets.clist = clist4;
	sl->widgets.menu = create_sharedlibs_menu (sl);
	sl->widgets.menu_update = sharedlibs_menu_uiinfo[0].widget;
}

Sharedlibs*
sharedlibs_new (IAnjutaDebugger *debugger)
{
	Sharedlibs* ew;
	ew = g_malloc(sizeof(Sharedlibs));
	if(ew)
	{
		ew->debugger = debugger;
		g_object_ref (debugger);
		
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
sharedlibs_clear (Sharedlibs *sg)
{
	if(GTK_IS_CLIST(sg->widgets.clist))
		gtk_clist_clear(GTK_CLIST(sg->widgets.clist));
}

void
sharedlibs_show (Sharedlibs* ew)
{
	if(ew)
	{
		if(ew->is_showing)
		{
			gdk_window_raise(ew->widgets.window->window);
		}
		else
		{
			gtk_widget_set_uposition(ew->widgets.window, ew->win_pos_x,
									 ew->win_pos_y);
			gtk_window_set_default_size(GTK_WINDOW(ew->widgets.window),
										ew->win_width, ew->win_height);
			gtk_widget_show(ew->widgets.window);
			ew->is_showing = TRUE;
			ianjuta_debugger_info_sharedlib (ew->debugger, sharedlibs_update, ew, NULL);
		}
	}
}

void
sharedlibs_hide (Sharedlibs* ew)
{
	if(ew)
	{
		if(ew->is_showing == FALSE) return;
			gdk_window_get_root_origin(ew ->widgets.window->window,
									   &ew->win_pos_x, &ew->win_pos_y);
		gdk_window_get_size(ew ->widgets.window->window, &ew->win_width,
							&ew->win_height);
		gtk_widget_hide(ew->widgets.window);
		ew->is_showing = FALSE;
	}
}

void
sharedlibs_free(Sharedlibs* sg)
{
	if(sg)
	{
		sharedlibs_clear(sg);
		g_object_unref (sg->debugger);
		gtk_widget_destroy(sg->widgets.window);
		gtk_widget_destroy(sg->widgets.menu);
		g_free(sg);
	}
}

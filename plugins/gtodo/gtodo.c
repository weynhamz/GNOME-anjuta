/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    gtodo.c
    Copyright (C) 2000 QBall, Naba Kumar

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

#include <config.h>
#include "main.h"
#include "debug_printf.h"
#include "tray-icon.h"

gulong shand;
GdkEventMotion *ovent;

static char *
item_factory_translate_func (const char *path, gpointer func_data)
{
	return _(path);
}

/************************
  save the xml file and quit
  saving xml isnt needed anymore, this is done on the fly
 ************************/
void quit_program()
{
	debug_printf(DEBUG_INFO, "Quiting....");
	/* calling a window moved, so the position is saved */
	if(mw.window)windows_moved(mw.window);
	gtk_main_quit();
}

void set_sorting_menu_item()
{
	int i = gconf_client_get_int(client, "/apps/gtodo/prefs/sort-type",NULL);
	if(i == 0)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Done, Date, Priority"))), TRUE);
	}
	else if (i==1)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Done, Priority, Date"))), TRUE);	
	}
	else if(i==2)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Priority, Date, Done"))), TRUE);		
	}
	else if(i==3)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Priority, Done, Date"))), TRUE);		
	}    
	else if(i==4)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Date, Priority, Done"))), TRUE);		
	}
	else if(i==5)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Date, Done, Priority"))), TRUE);		
	}    
}

static void sorting_order_callback(GtkWidget *widget,
							int data, GtkWidget *wid2)
{
	gboolean sortorder = !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(wid2));
	gtodo_set_sorting_order (sortorder);
}

static void sorting_callback(GtkWidget *widget, int data, GtkWidget *wid2)
{
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(wid2)))
	{
		gint sorttype = data;
		gtodo_set_sorting_type (sorttype);
	}
}

static void gconf_set_hide_done()
{
	gtodo_set_hide_done (gtk_check_menu_item_get_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(mw.item_factory, "/View/Hide Completed Items")));
}

static void gconf_set_hide_nodate()
{
	gtodo_set_hide_nodate (gtk_check_menu_item_get_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(mw.item_factory, "/View/Hide Items Without an End Date")));
}

static void gconf_set_hide_due()
{
	gtodo_set_hide_due (gtk_check_menu_item_get_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(mw.item_factory, "/View/Hide Items that Are Past Due Date")));
}

void windows_moved(GtkWidget *window)
{
	gint width, height;
	gint x, y;
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	gconf_client_set_int(client, "/apps/gtodo/prefs/pos-x",x,NULL);
	gconf_client_set_int(client, "/apps/gtodo/prefs/pos-y",y,NULL);
	gconf_client_set_int(client, "/apps/gtodo/prefs/size-x",width,NULL);
	gconf_client_set_int(client, "/apps/gtodo/prefs/size-y",height,NULL);
}

static GtkItemFactoryEntry menu_items[] =
{
	{ N_("/_ToDo"),					NULL,			0,		0,		"<Branch>" },
	{ N_("/ToDo/_New Task List"),			"<control>n",	create_playlist,		0,		"<StockItem>",GTK_STOCK_NEW},
	{ N_("/ToDo/_Open Task List"),			"<control>o",	open_playlist,		0,		"<StockItem>",GTK_STOCK_OPEN},
	{ N_("/ToDo/_Export to"), 			NULL,		0,		0,		"<Branch>"},
	{ N_("/ToDo/Export to/html"),			"",	export_gui,		0,		"<StockItem>",GTK_STOCK_SAVE},
	{ N_("/ToDo/Export to/task list"),			"",	export_backup_xml,		0,		"<StockItem>",GTK_STOCK_SAVE},
	{ "/ToDo/Sep0",					0,			0,		0,"<Separator>",	0},
	{ N_("/ToDo/_Hide or Show Window"), 		"<control>h",		tray_hide_show_window,		0,		"<StockItem>", GTK_STOCK_CONVERT },
	{ "/ToDo/Sep1",					0,			0,		0,"<Separator>",	0},
	{ N_("/ToDo/_Preferences"),			"<control>p",	gui_preferences,		0,		"<StockItem>",GTK_STOCK_PREFERENCES},
	{ N_("/ToDo/_Edit Categories"),		"<control>e",	category_manager,		0,"<StockItem>", "gtodo-edit"},
	{ "/ToDo/Sep1",					0,			0,		0,"<Separator>",	0},
	{ N_("/ToDo/_Quit"),				"<control>q",	quit_program,			0,"<StockItem>",	GTK_STOCK_QUIT},
	{ N_("/_Item"),					NULL,			0,		0,"<Branch>"},
	{ N_("/Item/_Add"),			"<alt>a",		gui_add_todo_item, (int)GINT_TO_POINTER(0), 	"<StockItem>", GTK_STOCK_ADD},
	{ N_("/Item/_Edit"),				"<alt>e",	gui_add_todo_item, (int)GINT_TO_POINTER(1), 	"<StockItem>", "gtodo-edit"},  
	{ N_("/Item/_Remove"),			"<alt>r",	remove_todo_item,		FALSE,		"<StockItem>",	GTK_STOCK_REMOVE},  
	{ N_("/Item/Remove _Completed Items"),		NULL,	purge_category,		0,	0},
	{ N_("/_View"),					NULL,	0,			0,	"<Branch>"},
	{ N_("/_View/_Sorting"),				NULL,	0,			0,	"<Branch>"},
	{ N_("/View/Sorting/Done, Date, Priority"),		NULL,	sorting_callback,	0, 	"<RadioItem>", 0},
	{ N_("/View/Sorting/Done, Priority, Date"),		NULL,	sorting_callback,	1, 	N_("/View/Sorting/Done, Date, Priority"),0},
	{ N_("/View/Sorting/Priority, Date, Done"),		NULL,	sorting_callback,	2, 	N_("/View/Sorting/Done, Date, Priority"),0},
	{ N_("/View/Sorting/Priority, Done, Date"),		NULL,	sorting_callback,	3, 	N_("/View/Sorting/Done, Date, Priority"),0},
	{ N_("/View/Sorting/Date, Priority, Done"),		NULL,	sorting_callback,	4, 	N_("/View/Sorting/Done, Date, Priority"),0},
	{ N_("/View/Sorting/Date, Done, Priority"),		NULL,	sorting_callback,	5, 	N_("/View/Sorting/Done, Date, Priority"),0},
	{ "/View/Sorting/Sepsort",				0,			0,	0,	"<Separator>",	0},
	{ N_("/View/Sorting/Sort Ascending"),			NULL,	sorting_order_callback,	0,	"<RadioItem>",0},  
	{ N_("/View/Sorting/Sort Descending"),		NULL,	NULL,		 	0,	N_("/View/Sorting/Sort Ascending"),0},    
	{ N_("/View/Hide Completed Items"),			NULL,	gconf_set_hide_done,	0, 	"<ToggleItem>", 0},
	{ N_("/View/Hide Items that Are Past Due Date"),		NULL,	gconf_set_hide_due,	0, 	"<ToggleItem>", 0},
	{ N_("/View/Hide Items Without an End Date"),	NULL,	gconf_set_hide_nodate,		0, 	"<ToggleItem>", 0},
	{ N_("/_Help"),					NULL,			0,	0,	"<Branch>"},
	{ N_("/Help/_About"),				NULL,			about_window,		0, "<StockItem>", "gnome-stock-about"},
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

GtkWidget* gui_create_main_window() {

    	GtkWidget *window;
	GtkWidget *gtodo;
	GtkIconSet *set;
	GdkPixbuf *pixbuf;

	/* Create todo widget */
	gtodo = gui_create_todo_widget();
	
	set = gtk_icon_factory_lookup_default("gtodo");

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    	gtk_container_add(GTK_CONTAINER(window), gtodo);
	gtk_window_set_title(GTK_WINDOW(window), _("Todo List"));
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 400);

	pixbuf = gtk_icon_set_render_icon(set, window->style, GTK_TEXT_DIR_NONE, GTK_STATE_NORMAL, GTK_ICON_SIZE_LARGE_TOOLBAR, window, NULL);
	gtk_window_set_icon(GTK_WINDOW(window), pixbuf);
	g_object_unref(pixbuf);

	/* drop down menu's */
	mw.accel_group = gtk_accel_group_new();
	mw.item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", mw.accel_group);
	g_object_set_data_full (G_OBJECT (window), "<main>", mw.item_factory, (GDestroyNotify) g_object_unref);
	gtk_item_factory_set_translate_func(mw.item_factory, item_factory_translate_func,
			NULL, NULL);
	gtk_item_factory_create_items (mw.item_factory, nmenu_items, menu_items, NULL);


	gtk_box_pack_start(GTK_BOX(mw.vbox), gtk_item_factory_get_widget(mw.item_factory, "<main>"), FALSE, TRUE, 0);    

	/* add a Hbuttonbox (droby) with a category selector and a new/delete button */

	g_signal_connect(window, "delete-event", G_CALLBACK(quit_program), NULL);
	g_signal_connect(window, "destroy-event", G_CALLBACK(quit_program), NULL);
	gtk_window_add_accel_group(GTK_WINDOW(window), mw.accel_group);

	gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(mw.item_factory, N_("/View/Hide Completed Items")),settings.hide_done);
	gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(mw.item_factory, N_("/View/Hide Items that Are Past Due Date")),settings.hide_due);
	gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(mw.item_factory, N_("/View/Hide Items Without an End Date")), settings.hide_nodate);

	if(settings.sortorder == 0)gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Sort Ascending"))), TRUE);		
	else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(mw.item_factory, N_("/View/Sorting/Sort Descending"))), TRUE);		

	set_sorting_menu_item();
	return window;
}

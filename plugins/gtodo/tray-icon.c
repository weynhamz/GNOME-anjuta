#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include "main.h"
#include "eggtrayicon.h"
#include "tray-icon.h"
#include "debug_printf.h"

EggTrayIcon *tray_icon = NULL;
GtkWidget *image = NULL;
GtkWidget *tip = NULL;
PangoLayout *tray_layout_tooltip = NULL;


/****************************************
 * The table mouse menu 		*
 ***************************************/

static GtkItemFactoryEntry tray_icon_menu[] =
{
	{N_("/_New"),	"",	gui_add_todo_item,	0, "<StockItem>", GTK_STOCK_NEW},
	{N_("/_Hide"),	"",	tray_hide_show_window,	0, "<StockItem>", GTK_STOCK_CONVERT},
	{N_("/_Show"),	"",	tray_hide_show_window,	0, "<StockItem>", GTK_STOCK_CONVERT},
	{N_("/sep1"),	NULL,	0,	      		0, "<Separator>" 	},
	{N_("/_Quit"),	"",	gtk_main_quit, 		0, "<StockItem>", GTK_STOCK_QUIT}
};

static int ntray_icon_menu = sizeof (tray_icon_menu) / sizeof (tray_icon_menu[0]);


void tray_paint_tip(GtkWidget *widget, GdkEventExpose *event)
{
	int width = 0, height = 0;
	int number_rows = 0;
	GTodoList *list = NULL;

	gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
			NULL, widget, "tooltip", 0, 0, -1, -1);                     	



	list = gtodo_client_get_todo_item_list(cl, NULL);
	if(list != NULL)
	{
		do
		{
			int m_height =0 , m_width = 0;
			gchar *string = NULL;
			GTodoItem *item = gtodo_client_get_todo_item_from_list(list); 

			string = gtodo_todo_item_get_summary(item);	

			pango_layout_set_markup(tray_layout_tooltip, string, strlen(string));
			pango_layout_get_size(tray_layout_tooltip, &m_width, &m_height);
			if(!gtodo_todo_item_get_done(item))
			{
				gtk_paint_arrow(widget->style, widget->window, GTK_STATE_NORMAL, 
						GTK_SHADOW_IN,NULL, widget, "tooltip",GTK_ARROW_RIGHT, TRUE, 4, 4+height, PANGO_PIXELS(m_height),
						PANGO_PIXELS(m_height)); 

				gtk_paint_layout (widget->style, widget->window, GTK_STATE_NORMAL, TRUE,
						NULL, widget, "tooltip", 18,4+height, tray_layout_tooltip);

				width = MAX(PANGO_PIXELS(width), m_width);
				height = height + PANGO_PIXELS(m_height);
			}

		}while(gtodo_client_get_list_next(list));
		gtodo_client_free_todo_item_list(cl, list);
	}
	if(height == 0)
	{
		pango_layout_set_markup(tray_layout_tooltip, _("Todo List"), strlen(_("Todo List")));
		pango_layout_get_size(tray_layout_tooltip, &width, &height);


		gtk_paint_layout (widget->style, widget->window, GTK_STATE_NORMAL, TRUE,
				NULL, widget, "tooltip", 4,4, tray_layout_tooltip);
	}
}

gboolean tray_motion_cb (GtkWidget *tv, GdkEventCrossing *event, gpointer n)
{
	int width = 0,height = 0;
	GdkRectangle msize;
	int x,y;
	char *tooltiptext = NULL;
	int number_rows = 0;
	GTodoList *list = NULL;
	int monitor = gdk_screen_get_monitor_at_window(
			gtk_widget_get_screen(tv), tv->window);
	if(tip != NULL) return FALSE;
	tooltiptext = g_strdup("getting height");



	tip = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_app_paintable(tip, TRUE);
	gtk_window_set_resizable(GTK_WINDOW(tip), FALSE);
	gtk_widget_set_name(tip, "gtk-tooltips");
	gtk_widget_ensure_style (tip);

	tray_layout_tooltip = gtk_widget_create_pango_layout (tip, NULL);
	pango_layout_set_wrap(tray_layout_tooltip, PANGO_WRAP_WORD);	

	list = gtodo_client_get_todo_item_list(cl, NULL);
	if(list != NULL)
	{
		do
		{
			int m_height =0 , m_width = 0;
			gchar *string = NULL;
			GTodoItem *item = gtodo_client_get_todo_item_from_list(list); 

			string = gtodo_todo_item_get_summary(item);	

			pango_layout_set_markup(tray_layout_tooltip, string, strlen(string));
			pango_layout_get_size(tray_layout_tooltip, &m_width, &m_height);

			if(!gtodo_todo_item_get_done(item))
			{
				width = MAX(width, m_width);
				height = height + m_height;
			}

		}while(gtodo_client_get_list_next(list));
		gtodo_client_free_todo_item_list(cl, list);
	}
	if(height == 0)
	{
		pango_layout_set_markup(tray_layout_tooltip, _("Todo List"), strlen(_("Todo List")));
		pango_layout_get_size(tray_layout_tooltip, &width, &height);
		width = width - 18*PANGO_SCALE;
	}

	gdk_screen_get_monitor_geometry(gtk_widget_get_screen(tv), monitor, &msize);

	g_signal_connect(G_OBJECT(tip), "expose_event",
			G_CALLBACK(tray_paint_tip), NULL);


	width= PANGO_PIXELS(width) +26;
	height= PANGO_PIXELS(height)+8;



	/* set size */
	gtk_widget_set_usize(tip, width,height);                                       	

	/* calculate position */
	x = (int)event->x_root - event->x+tv->allocation.width/2 - width/2;
	y = (int)event->y_root+(tv->allocation.height - event->y) +5;	

	/* check borders left, right*/	
	if((x+width) > msize.width+msize.x)
	{	
		x = msize.x+msize.width-(width);
	}
	else if(x < 0)
	{                                                                  	
		x= 0;
	}
	/* check up down.. if can't place it below, place it above */
	if( y+height > msize.height+msize.y) 
	{
		y = event->y_root - event->y -5-(height);
	}
	/* place the window */
	gtk_window_move(GTK_WINDOW(tip), x, y);                            	

	gtk_widget_show_all(tip);	


	g_free(tooltiptext);
	gtk_widget_queue_draw(tip);
	return TRUE;


}

void tray_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n)
{
	//	if(tray_timeout != -1) g_source_remove(tray_timeout);
	//	tray_timeout = -1;
	if(tip != NULL)
	{
		gtk_widget_destroy(tip);
		g_object_unref(tray_layout_tooltip);
	}

	tip = NULL;
}

void tray_icon_remove()
{
	gtk_widget_destroy(GTK_WIDGET(tray_icon));
	tray_icon = NULL;

}

/* if the tray is destroyed, recreate it again */
void tray_icon_destroy(GtkWidget *ticon, GtkWidget *window)
{
	if(gconf_client_get_bool(client, "/apps/gtodo/view/enable-tray",NULL))
	{
		tray_init(window);
	}
}

void tray_hide_show_window()
{
	GtkWidget *window;
    
    	window = g_object_get_data (G_OBJECT(tray_icon), "window");

	if(GTK_WIDGET_VISIBLE(window))
	{
		debug_printf(DEBUG_INFO, "Hiding window");
		gtk_widget_hide(window);
	}
	else
	{
		debug_printf(DEBUG_INFO, "Showing window");
		gtk_widget_show(window);
	}
}

int tray_mouse_click(GtkWidget *wid, GdkEventButton *event)
{
	GtkWidget *window;
    
    	window = g_object_get_data (G_OBJECT(tray_icon), "window");
    
	/* left mouse button */
	/* Show/hide the window*/
	if(event->button == 1)
	{
		tray_hide_show_window();
	}
	/* right mouse menu */
	else if (event->button == 3)
	{
		GtkItemFactory *item = gtk_item_factory_new(GTK_TYPE_MENU, "<tablepop>", NULL);
		gtk_item_factory_create_items (item, ntray_icon_menu, tray_icon_menu, NULL);
		/* add it */
		if(GTK_WIDGET_VISIBLE(window))
		{
			gtk_widget_hide(gtk_item_factory_get_widget(item, "/Show"));
		}
		else
		{
			gtk_widget_hide(gtk_item_factory_get_widget(item, "/Hide"));
		}
		gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(GTK_ITEM_FACTORY(item), "<tablepop>")),
				NULL,NULL,NULL, NULL, event->button, event->time);
	}
}


/* create the tray icon */
void tray_init(GtkWidget *window)
{
	GtkWidget *eventbox = NULL;
	GdkPixbuf *pixbuf = NULL;

	if(!gconf_client_get_bool(client, "/apps/gtodo/view/enable-tray",NULL))
	{
		return;
	}
	if(tray_icon != NULL) return;

	
	debug_printf(DEBUG_INFO,"Creating Tray Icon\n");

	/* setup the tray icon */
	tray_icon = egg_tray_icon_new(_("Todo List Manager"));

	/* event box */
	eventbox = gtk_event_box_new();

	/* add a image (in an event box) to the tray icon */

	/* 
	 * the icon is a fixed size, I have no idea (yet) how to get the size of the tray widget, 
	 * Withouth doing the drawing myself.
	 */
	pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAP_PATH"/gtodo_tray.png", 20,20,NULL); 
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	/* add everything */
	gtk_container_add(GTK_CONTAINER(eventbox), image);
	gtk_container_add(GTK_CONTAINER(tray_icon), eventbox);


	/* connect signals to the tray icon */
	g_signal_connect(G_OBJECT(tray_icon), "destroy", 		G_CALLBACK(tray_icon_destroy), window);
	g_signal_connect(G_OBJECT(tray_icon), "button-release-event", 	G_CALLBACK(tray_mouse_click), NULL);
	g_signal_connect(G_OBJECT(eventbox), "enter-notify-event",	G_CALLBACK(tray_motion_cb), NULL);
	g_signal_connect(G_OBJECT(eventbox), "leave-notify-event",	G_CALLBACK(tray_leave_cb), NULL);

	/* show the tray icon */
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	g_object_set_data (G_OBJECT(tray_icon), "window", window);
}

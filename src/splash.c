/*
    splash.c
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
#include "pixmaps.h"
#include "splash.h"
#include "resources.h"

// this is the callback which will destroy the splash screen window.
static gint
splash_screen_cb(gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
        gtk_object_destroy(GTK_OBJECT(data));
        return FALSE;
}

// if user presses a key, hide the splash screen.
static gboolean
splash_key_pressed(GtkWidget *widget,
                      GdkEventKey * event,
                      gpointer user_data)
{
        gtk_widget_hide(widget);
        return TRUE;
}

// if user presses a mouse button, hide splash screen.
static gboolean
splash_button_pressed(GtkWidget *widget,
                      GdkEventButton * event,
                      gpointer user_data)
{
        gtk_widget_hide(widget);
        return TRUE;
}

static GtkWidget*
load_image(char* file)
{
    int imw, imh;
    GdkImlibImage* im;
    GtkWidget* ret;
    im = gdk_imlib_load_image(file);
    if (!im)
        return NULL;
    imw = im->rgb_width;
    imh=im->rgb_height;
    ret = gnome_pixmap_new_from_imlib_at_size(im,imw,imh);
    gdk_imlib_destroy_image(im);
    return ret;
}

// show the splash screen
void
splash_screen()
{
	gchar* im_file;
	GtkWidget* pixmap;
	GtkWidget *MainFrame;
	// make the image for the splash screen
	im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
	if(im_file == NULL) return;
	pixmap = load_image(im_file);
	g_free(im_file);
	if(pixmap == NULL) return;

	// make a window for splash screen, make it a popup so it will stay on
	// top of everything.
	MainFrame = gtk_window_new (GTK_WINDOW_POPUP);

	// set the size to that of our splash image.
        gtk_window_set_title (GTK_WINDOW (MainFrame), _("Anjuta"));
	// center it on the screen
        gtk_window_set_position(GTK_WINDOW (MainFrame), GTK_WIN_POS_CENTER);

	// set up key and mound button press to hide splash screen
        gtk_signal_connect(GTK_OBJECT(MainFrame),"button_press_event",
                GTK_SIGNAL_FUNC(splash_button_pressed),NULL);
        gtk_signal_connect(GTK_OBJECT(MainFrame),"key_press_event",
                GTK_SIGNAL_FUNC(splash_key_pressed),NULL);
        gtk_widget_add_events(MainFrame,
                              GDK_BUTTON_PRESS_MASK|
                              GDK_BUTTON_RELEASE_MASK|
                              GDK_KEY_PRESS_MASK);


        gtk_container_add (GTK_CONTAINER (MainFrame), pixmap);
        gtk_widget_show(pixmap);
        gtk_widget_show(MainFrame);

	// force it to draw now.
	gdk_flush();

	// go into main loop, processing events.
        while(gtk_events_pending() || !GTK_WIDGET_REALIZED(pixmap))
                gtk_main_iteration();

	// after 3 seconds, destroy the splash screen.
        gtk_timeout_add( 3000, splash_screen_cb, MainFrame );
}

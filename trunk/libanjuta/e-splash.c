/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* e-splash.c
 *
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Ettore Perazzoli
 */

/**
 * SECTION:e-splash
 * @short_description: Splash screen
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/e-splash.h
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <gtk/gtk.h>

#include "e-splash.h"

struct _ESplashPrivate {
	GnomeCanvas *canvas;
	GnomeCanvasItem *canvas_icon;
	GnomeCanvasItem *canvas_text;
	GnomeCanvasItem *canvas_line;
	GnomeCanvasItem *canvas_line_back;
	GdkPixbuf *splash_image_pixbuf;
	gint progressbar_position;
};

G_DEFINE_TYPE(ESplash, e_splash, GTK_TYPE_WINDOW)

/* Layout constants.  These need to be changed if the splash changes.  */

#define ICON_Y    priv->progressbar_position
#define ICON_X    15
#define ICON_SIZE 48
#define PROGRESS_SIZE 5

/* GtkObject methods.  */

static void
impl_destroy (GtkObject *object)
{
	ESplash *splash;
	ESplashPrivate *priv;

	splash = E_SPLASH (object);
	priv = splash->priv;

	if (priv->splash_image_pixbuf != NULL)
		gdk_pixbuf_unref (priv->splash_image_pixbuf);
	g_free (priv);
}

static void
e_splash_finalize (GObject *obj)
{	
	G_OBJECT_CLASS (e_splash_parent_class)->finalize (obj);
}

static void
e_splash_class_init (ESplashClass *klass)
{
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	gtkobject_class->destroy = impl_destroy;
	
	object_class->finalize = e_splash_finalize;
}

static void
e_splash_init (ESplash *splash)
{
	ESplashPrivate *priv;

	priv = g_new (ESplashPrivate, 1);
	priv->canvas              = NULL;
	priv->splash_image_pixbuf = NULL;
	priv->progressbar_position = 100;
	splash->priv = priv;
}

static gboolean
button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	ESplash *splash = (ESplash *) data;
	
	gtk_widget_hide (GTK_WIDGET (splash));
	
	return TRUE;
}

/**
 * e_splash_construct:
 * @splash: A pointer to an ESplash widget
 * @splash_image_pixbuf: The pixbuf for the image to appear in the splash dialog
 * 
 * Construct @splash with @splash_image_pixbuf as the splash image.
 **/
void
e_splash_construct (ESplash *splash,
					GdkPixbuf *splash_image_pixbuf,
					gint progressbar_position)
{
	ESplashPrivate *priv;
	GtkWidget *canvas; /*, *frame; */
	int image_width, image_height;

	g_return_if_fail (splash != NULL);
	g_return_if_fail (E_IS_SPLASH (splash));
	g_return_if_fail (splash_image_pixbuf != NULL);

	priv = splash->priv;
	priv->progressbar_position = progressbar_position;
	priv->splash_image_pixbuf = gdk_pixbuf_ref (splash_image_pixbuf);

	canvas = gnome_canvas_new_aa ();
	priv->canvas = GNOME_CANVAS (canvas);

	image_width = gdk_pixbuf_get_width (splash_image_pixbuf);
	image_height = gdk_pixbuf_get_height (splash_image_pixbuf);

	gtk_widget_set_size_request (canvas, image_width, image_height);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), 0, 0, image_width, image_height);
	gtk_widget_show (canvas);

	gtk_container_add (GTK_CONTAINER (splash), canvas);

	gnome_canvas_item_new (GNOME_CANVAS_GROUP (priv->canvas->root),
			       GNOME_TYPE_CANVAS_PIXBUF,
			       "pixbuf", splash_image_pixbuf,
			       NULL);
	priv->canvas_icon = gnome_canvas_item_new (GNOME_CANVAS_GROUP (priv->canvas->root),
						   GNOME_TYPE_CANVAS_PIXBUF,
						   "x", (double) (ICON_X),
						   "y", (double) (ICON_Y),
						   NULL);
	priv->canvas_text  = gnome_canvas_item_new (GNOME_CANVAS_GROUP (priv->canvas->root),
						   GNOME_TYPE_CANVAS_TEXT,
						   "fill_color", "black",
						   "size_points", (double)12,
						   "anchor", GTK_ANCHOR_SOUTH_WEST,
						   "x", (double)(ICON_X + ICON_SIZE + 10),
						   "y", (double)(ICON_Y + ICON_SIZE - PROGRESS_SIZE),
						   NULL);
	priv->canvas_line  = gnome_canvas_item_new (GNOME_CANVAS_GROUP (priv->canvas->root),
						   GNOME_TYPE_CANVAS_LINE,
						   "fill_color", "black",
						   "width_pixels", PROGRESS_SIZE,
						   NULL);
	priv->canvas_line_back  = gnome_canvas_item_new (GNOME_CANVAS_GROUP (priv->canvas->root),
						   GNOME_TYPE_CANVAS_LINE,
						   "fill_color", "blue",
						   "width_pixels", PROGRESS_SIZE,
						   NULL);
	g_signal_connect (G_OBJECT (splash), "button-press-event",
			  G_CALLBACK (button_press_event), splash);
	
	gtk_window_set_decorated(GTK_WINDOW(splash), FALSE);
	gtk_window_set_position (GTK_WINDOW (splash), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable (GTK_WINDOW (splash), FALSE);
	gtk_window_set_default_size (GTK_WINDOW (splash), image_width, image_height);
	gtk_window_set_title (GTK_WINDOW (splash), "Anjuta");

}

/**
 * e_splash_new:
 * @image_file: Splash image file
 *
 * Create a new ESplash widget.
 * 
 * Return value: A pointer to the newly created ESplash widget.
 **/
GtkWidget *
e_splash_new (const char *image_file, gint progressbar_position)
{
	ESplash *splash;
	GdkPixbuf *splash_image_pixbuf;

	splash_image_pixbuf = gdk_pixbuf_new_from_file (image_file, NULL);
	g_return_val_if_fail (splash_image_pixbuf != NULL, NULL);

	splash = g_object_new (e_splash_get_type (), "type", GTK_WINDOW_TOPLEVEL, NULL);
	e_splash_construct (splash, splash_image_pixbuf, progressbar_position);

	/* gdk_pixbuf_unref (splash_image_pixbuf); */

	return GTK_WIDGET (splash);
}

/**
 * e_splash_set:
 * @splash: A pointer to an ESplash widget
 * @icon_pixbuf: Pixbuf for the icon to be added
 * @title: Title.
 * @desc: Description.
 * @progress_percentage: percentage of progress.
 * 
 * Set the current progress/icon/text.
 **/
void
e_splash_set  (ESplash *splash, GdkPixbuf *icon_pixbuf,
			   const gchar *title, const gchar *desc,
			   gfloat progress_percentage)
{
	ESplashPrivate *priv;
	GnomeCanvasPoints *points;
	gint inc_width, progress_width;
	
	g_return_if_fail (splash != NULL);
	g_return_if_fail (E_IS_SPLASH (splash));

#ifdef GNOME2_CONVERSION_COMPLETE
	if (GTK_OBJECT_DESTROYED (splash))
		return;
#endif
	
	priv = splash->priv;
	
	if (icon_pixbuf)
	{
		GdkPixbuf *scaled_pixbuf;
		
		scaled_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE,
										8, ICON_SIZE, ICON_SIZE);
		gdk_pixbuf_scale (icon_pixbuf, scaled_pixbuf,
						  0, 0,
						  ICON_SIZE, ICON_SIZE,
						  0, 0,
						  (double) ICON_SIZE / gdk_pixbuf_get_width (icon_pixbuf),
						  (double) ICON_SIZE / gdk_pixbuf_get_height (icon_pixbuf),
						  GDK_INTERP_HYPER);
		g_object_set (G_OBJECT (priv->canvas_icon),
					  "pixbuf", scaled_pixbuf, NULL);
		gdk_pixbuf_unref (scaled_pixbuf);
	}
	
	if (title)
	{
		g_object_set (G_OBJECT (priv->canvas_text),
					  "markup", title, NULL);
	}
	
	inc_width = gdk_pixbuf_get_width (priv->splash_image_pixbuf);
	inc_width -= (ICON_X + ICON_SIZE + 20);
	progress_width = inc_width;
	
	points = gnome_canvas_points_new (2);
	points->coords[0] = ICON_X + ICON_SIZE + 10;
	points->coords[1] = ICON_Y + ICON_SIZE;
	points->coords[2] = ICON_X + ICON_SIZE + 10 + (progress_percentage * inc_width);
	points->coords[3] = ICON_Y + ICON_SIZE;
	
	g_object_set (G_OBJECT (priv->canvas_line),
				  "points", points, NULL);
	gnome_canvas_points_unref (points);
	
	points = gnome_canvas_points_new (2);
	points->coords[0] = ICON_X + ICON_SIZE + 10 + (progress_percentage * inc_width);
	points->coords[1] = ICON_Y + ICON_SIZE;
	points->coords[2] = ICON_X + ICON_SIZE + 10 + progress_width;
	points->coords[3] = ICON_Y + ICON_SIZE;
	
	g_object_set (G_OBJECT (priv->canvas_line_back),
				  "points", points, NULL);
	gnome_canvas_points_unref (points);
}

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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Ettore Perazzoli
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnome/gnome-macros.h>
#include <gtk/gtk.h>

#include "e-splash.h"

struct _Icon {
	GdkPixbuf *pixbuf;
	gchar *title, *description;
};
typedef struct _Icon Icon;

struct _ESplashPrivate {
	GnomeCanvas *canvas;
	GnomeCanvasItem *canvas_icon;
	GnomeCanvasItem *canvas_text;
	GnomeCanvasItem *canvas_line;
	GnomeCanvasItem *canvas_line_back;
	GdkPixbuf *splash_image_pixbuf;
	gint progressbar_position;
	
	GList *icons;		/* (Icon *) */
	int num_icons;
};

GNOME_CLASS_BOILERPLATE (ESplash, e_splash, GtkWindow, GTK_TYPE_WINDOW);

/* Layout constants.  These need to be changed if the splash changes.  */

#define ICON_Y    priv->progressbar_position
#define ICON_X    15
#define ICON_SIZE 48
#define PROGRESS_SIZE 5

/* Icon management.  */

#if 0
static GdkPixbuf *
create_darkened_pixbuf (GdkPixbuf *pixbuf)
{
	GdkPixbuf *new;
	unsigned char *rowp;
	int width, height;
	int rowstride;
	int i, j;

	new = gdk_pixbuf_copy (pixbuf);
	
	if (! gdk_pixbuf_get_has_alpha (new))
		return new;

	width     = gdk_pixbuf_get_width (new);
	height    = gdk_pixbuf_get_height (new);
	rowstride = gdk_pixbuf_get_rowstride (new);

	rowp = gdk_pixbuf_get_pixels (new);
	for (i = 0; i < height; i ++) {
		unsigned char *p;

		p = rowp;
		for (j = 0; j < width; j++) {
			p[3] *= .25;
			p += 4;
		}

		rowp += rowstride;
	}

	return new;
}
#endif

static Icon *
icon_new (ESplash *splash,
	  GdkPixbuf *image_pixbuf, const gchar *title, const gchar *desc)
{
	ESplashPrivate *priv;
	Icon *icon;

	priv = splash->priv;

	icon = g_new (Icon, 1);

	icon->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE,
								   8, ICON_SIZE, ICON_SIZE);
	gdk_pixbuf_scale (image_pixbuf, icon->pixbuf,
			  0, 0,
			  ICON_SIZE, ICON_SIZE,
			  0, 0,
			  (double) ICON_SIZE / gdk_pixbuf_get_width (image_pixbuf),
			  (double) ICON_SIZE / gdk_pixbuf_get_height (image_pixbuf),
			  GDK_INTERP_HYPER);

	icon->title = g_strdup (title);
	icon->description = g_strdup (desc);
	/* Set up the canvas item to point to the dark pixbuf initially.  */

	return icon;
}

static void
icon_free (Icon *icon)
{
	gdk_pixbuf_unref (icon->pixbuf);
/*  	gtk_object_unref (GTK_OBJECT (icon->canvas_item)); */
	g_free (icon->title);
	g_free (icon->description);
	g_free (icon);
}

/* GtkObject methods.  */

static void
impl_destroy (GtkObject *object)
{
	ESplash *splash;
	ESplashPrivate *priv;
	GList *p;

	splash = E_SPLASH (object);
	priv = splash->priv;

	if (priv->splash_image_pixbuf != NULL)
		gdk_pixbuf_unref (priv->splash_image_pixbuf);

	for (p = priv->icons; p != NULL; p = p->next) {
		Icon *icon;

		icon = (Icon *) p->data;
		icon_free (icon);
	}

	g_list_free (priv->icons);

	g_free (priv);
}

static void
e_splash_class_init (ESplashClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy = impl_destroy;

	parent_class = gtk_type_class (gtk_window_get_type ());
}

static void
e_splash_instance_init (ESplash *splash)
{
	ESplashPrivate *priv;

	priv = g_new (ESplashPrivate, 1);
	priv->canvas              = NULL;
	priv->splash_image_pixbuf = NULL;
	priv->icons               = NULL;
	priv->num_icons           = 0;
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

	/*frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (splash), frame);
	*/
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
 * e_splash_add_icon:
 * @splash: A pointer to an ESplash widget
 * @icon_pixbuf: Pixbuf for the icon to be added
 * @title: Title.
 * @desc: Description.
 *
 * Add @icon_pixbuf to the @splash.
 * 
 * Return value: The total number of icons in the splash after the new icon has
 * been added.
 **/
int
e_splash_add_icon (ESplash *splash,
		   GdkPixbuf *icon_pixbuf, const gchar *title, const gchar *desc)
{
	ESplashPrivate *priv;
	Icon *icon;

	g_return_val_if_fail (splash != NULL, 0);
	g_return_val_if_fail (E_IS_SPLASH (splash), 0);
	g_return_val_if_fail (icon_pixbuf != NULL, 0);

#ifdef GNOME2_CONVERSION_COMPLETE
	if (GTK_OBJECT_DESTROYED (splash))
		return 0;
#endif
	
	priv = splash->priv;

	icon = icon_new (splash, icon_pixbuf, title, desc);
	priv->icons = g_list_append (priv->icons, icon);

	priv->num_icons ++;
	return priv->num_icons;
}

/**
 * e_splash_set_icon_highlight:
 * @splash: A pointer to an ESplash widget
 * @num: Number of the icon whose highlight state must be changed
 * @highlight: Whether the icon must be highlit or not
 * 
 * Change the highlight state of the @num-th icon.
 **/
void
e_splash_set_icon_highlight  (ESplash *splash,
			      int num,
			      gboolean highlight)
{
	ESplashPrivate *priv;
	Icon *icon;
	GList *node;
	gchar *label;
	GnomeCanvasPoints *points;
	gint inc_width, progress_width;
	
	g_return_if_fail (splash != NULL);
	g_return_if_fail (E_IS_SPLASH (splash));

#ifdef GNOME2_CONVERSION_COMPLETE
	if (GTK_OBJECT_DESTROYED (splash))
		return;
#endif
	
	priv = splash->priv;
	
	node = priv->icons;
	icon = (Icon *) g_list_nth (priv->icons, num)->data;

	g_object_set (G_OBJECT (priv->canvas_icon),
		      "pixbuf", icon->pixbuf,
		      NULL);
	label = g_strdup_printf ("<b>Loading:</b> %s", icon->title);
	g_object_set (G_OBJECT (priv->canvas_text),
		      "markup", label,
		      NULL);
	g_free (label);
	
	inc_width = gdk_pixbuf_get_width (priv->splash_image_pixbuf);
	inc_width -= (ICON_X + ICON_SIZE + 20);
	progress_width = inc_width;
	inc_width /= priv->num_icons;
	
	points = gnome_canvas_points_new (2);
	points->coords[0] = ICON_X + ICON_SIZE + 10;
	points->coords[1] = ICON_Y + ICON_SIZE;
	points->coords[2] = ICON_X + ICON_SIZE + 10 + ((num + 1) * inc_width);
	points->coords[3] = ICON_Y + ICON_SIZE;
	
	g_object_set (G_OBJECT (priv->canvas_line),
		      "points", points, NULL);
	gnome_canvas_points_unref (points);
	
	points = gnome_canvas_points_new (2);
	points->coords[0] = ICON_X + ICON_SIZE + 10 + ((num + 1) * inc_width);
	points->coords[1] = ICON_Y + ICON_SIZE;
	points->coords[2] = ICON_X + ICON_SIZE + 10 + progress_width;
	points->coords[3] = ICON_Y + ICON_SIZE;
	
	g_object_set (G_OBJECT (priv->canvas_line_back),
		      "points", points, NULL);
	gnome_canvas_points_unref (points);

}

#if 0

E_MAKE_TYPE (e_splash, "ESplash", ESplash, class_init, init, PARENT_TYPE)
#endif

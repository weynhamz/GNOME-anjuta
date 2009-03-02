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

#include <gtk/gtk.h>

#include "e-splash.h"

struct _ESplashPrivate {
	GdkPixbuf *splash_image_pixbuf;
	GdkPixbuf *icon_pixbuf;
	gchar *title;
	gint progressbar_position;
	gfloat progress_percentage;
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
		g_object_unref (priv->splash_image_pixbuf);
	if (priv->icon_pixbuf != NULL)
		g_object_unref (priv->icon_pixbuf);
	g_free (priv->title);
	
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
	priv->splash_image_pixbuf = NULL;
	priv->icon_pixbuf = NULL;
	priv->title = NULL;
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

static gboolean
on_expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
                    ESplash *splash)
{
	ESplashPrivate *priv;
	cairo_t *cr;
	gint inc_width;

	priv = splash->priv;

	/* draw the background pixbuf */
	cr = gdk_cairo_create (widget->window);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	gdk_cairo_set_source_pixbuf (cr, priv->splash_image_pixbuf, 0, 0);

	cairo_paint (cr);

	/* draw the plugin icon */
	if (priv->icon_pixbuf)
	{
		cr = gdk_cairo_create (widget->window);
		gdk_cairo_set_source_pixbuf (cr, priv->icon_pixbuf, ICON_X, ICON_Y);

		cairo_paint (cr);
	}

	/* draw the plugin text */
	if (priv->title)
	{
		PangoContext *pc;
		PangoLayout *layout;
		gint layout_height;
		
		pc = gtk_widget_get_pango_context (widget);
		layout = pango_layout_new (pc);
		pango_layout_set_markup (layout, priv->title, -1);
		pango_layout_get_size (layout, NULL, &layout_height);

		cr = gdk_cairo_create (widget->window);
		cairo_move_to (cr, ICON_X + ICON_SIZE + 10,
		               ICON_Y + ICON_SIZE - PROGRESS_SIZE - PANGO_PIXELS (layout_height));
		
		pango_cairo_show_layout (cr, layout);

		g_object_unref (layout);
	}
	
	/* draw the progress bar */
	inc_width = gdk_pixbuf_get_width (priv->splash_image_pixbuf);
	inc_width -= (ICON_X + ICON_SIZE + 20);	
	
	cr = gdk_cairo_create (widget->window);
	cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
	cairo_rectangle (cr, ICON_X + ICON_SIZE + 10, ICON_Y + ICON_SIZE,
	                 inc_width, PROGRESS_SIZE);

	cairo_fill (cr);

	cr = gdk_cairo_create (widget->window);
	cairo_rectangle (cr, ICON_X + ICON_SIZE + 10, ICON_Y + ICON_SIZE,
	                 (priv->progress_percentage * inc_width), PROGRESS_SIZE);

	cairo_fill (cr);

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
	int image_width, image_height;

	g_return_if_fail (splash != NULL);
	g_return_if_fail (E_IS_SPLASH (splash));
	g_return_if_fail (splash_image_pixbuf != NULL);

	priv = splash->priv;
	priv->progressbar_position = progressbar_position;
	priv->splash_image_pixbuf = g_object_ref (splash_image_pixbuf);

	image_width = gdk_pixbuf_get_width (splash_image_pixbuf);
	image_height = gdk_pixbuf_get_height (splash_image_pixbuf);

	gtk_widget_set_size_request (GTK_WIDGET (splash), image_width, image_height);

	g_signal_connect (G_OBJECT (splash), "expose-event",
	                  G_CALLBACK (on_expose_event_cb), splash);
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

	splash = g_object_new (E_TYPE_SPLASH, "type", GTK_WINDOW_TOPLEVEL, NULL);
	e_splash_construct (splash, splash_image_pixbuf, progressbar_position);

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
	
	g_return_if_fail (splash != NULL);
	g_return_if_fail (E_IS_SPLASH (splash));
	
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

		g_object_unref (priv->icon_pixbuf);
		priv->icon_pixbuf = scaled_pixbuf;
	}

	if (title)
	{
		g_free (priv->title);
		priv->title = g_strdup (title);
	}

	priv->progress_percentage = progress_percentage;

	gtk_widget_queue_draw (GTK_WIDGET (splash));
}

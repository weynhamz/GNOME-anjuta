/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2002 Dave Camp
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:cell-renderer-captioned-image
 * @short_description: Captioned image cell renderer
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-cell-renderer-captioned-image.h
 * 
 */

#include <stdlib.h>
#include <glib/gi18n.h>

#include "anjuta-cell-renderer-captioned-image.h"

enum {
	PROP_0,
	
	PROP_TEXT,
	PROP_PIXBUF
};

#define PAD 3
#define SPACE 5
			 
G_DEFINE_TYPE(AnjutaCellRendererCaptionedImage, anjuta_cell_renderer_captioned_image, GTK_TYPE_CELL_RENDERER)

static void
anjuta_cell_renderer_captioned_image_get_property (GObject *object,
						   guint param_id,
						   GValue *value,
						   GParamSpec *pspec)
{
	AnjutaCellRendererCaptionedImage *cell = ANJUTA_CELL_RENDERER_CAPTIONED_IMAGE (object);
	
	switch (param_id) {
	case PROP_TEXT:
		g_object_get_property (G_OBJECT (cell->caption), 
				       "text", value);
		break;
	case PROP_PIXBUF:
		g_object_get_property (G_OBJECT (cell->image), 
				       "pixbuf", value);
		break;
	default :
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
anjuta_cell_renderer_captioned_image_set_property (GObject *object,
						   guint param_id,
						   const GValue *value,
						   GParamSpec *pspec)
{
	AnjutaCellRendererCaptionedImage *cell = ANJUTA_CELL_RENDERER_CAPTIONED_IMAGE (object);
	
	switch (param_id) {
	case PROP_TEXT:
		g_object_set_property (G_OBJECT (cell->caption), "text", value);
		break;
		
	case PROP_PIXBUF:
		g_object_set_property (G_OBJECT (cell->image), "pixbuf", 
				       value);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GtkCellRenderer *
anjuta_cell_renderer_captioned_image_new (void)
{
	return GTK_CELL_RENDERER (g_object_new (anjuta_cell_renderer_captioned_image_get_type (), NULL));
}

static void
anjuta_cell_renderer_captioned_image_get_preferred_height (GtkCellRenderer *gtk_cell,
                                                           GtkWidget *widget,
                                                           gint *minimum,
                                                           gint *natural)
{
	gint text_min;
	gint text_nat;
	gint image_min;
	gint image_nat;
    
	AnjutaCellRendererCaptionedImage *cell = ANJUTA_CELL_RENDERER_CAPTIONED_IMAGE (gtk_cell);
	
	gtk_cell_renderer_get_preferred_height (cell->image, widget, &image_min, &image_nat);
	
	gtk_cell_renderer_get_preferred_height (cell->caption, widget,
	                                        &text_min, &text_nat);


	if (minimum) {
		*minimum = image_min + text_min + PAD;
	}
	
	if (natural) {
		*natural = image_nat + text_nat + PAD;
	}
}

static void
anjuta_cell_renderer_captioned_image_get_preferred_width (GtkCellRenderer *gtk_cell,
                                                          GtkWidget *widget,
                                                          gint *minimum,
                                                          gint *natural)
{
	gint text_min;
	gint text_nat;
	gint image_min;
	gint image_nat;
    
	AnjutaCellRendererCaptionedImage *cell = ANJUTA_CELL_RENDERER_CAPTIONED_IMAGE (gtk_cell);
	
	gtk_cell_renderer_get_preferred_width (cell->image, widget, &image_min, &image_nat);
	
	gtk_cell_renderer_get_preferred_width (cell->caption, widget,
	                                      &text_min, &text_nat);


	if (minimum) {
		*minimum = MAX (image_min, text_min);
		*minimum += SPACE * 2;
	}
	
	if (natural) {
		*natural = MAX (image_nat, text_nat);
		*natural += SPACE * 2;
	}
}

static void
anjuta_cell_renderer_captioned_image_render (GtkCellRenderer *gtk_cell,
					     cairo_t *cr,
					     GtkWidget *widget,
					     const GdkRectangle *background_area,
					     const GdkRectangle *cell_area,
					     GtkCellRendererState flags)

{
	AnjutaCellRendererCaptionedImage *cell = ANJUTA_CELL_RENDERER_CAPTIONED_IMAGE (gtk_cell);
	GdkRectangle text_area;
	GdkRectangle pixbuf_area;
	gint width, height;
	
	gtk_cell_renderer_get_preferred_width (cell->image, widget, NULL, &width);
	gtk_cell_renderer_get_preferred_height (cell->image, widget, NULL, &height);
	
	pixbuf_area.y = cell_area->y;
	pixbuf_area.x = cell_area->x;
	pixbuf_area.height = height;
	pixbuf_area.width = cell_area->width;
    
	gtk_cell_renderer_get_preferred_width (cell->caption, widget, NULL, &width);
	gtk_cell_renderer_get_preferred_height (cell->caption, widget, NULL, &height);

	text_area.x = cell_area->x + (cell_area->width - width) / 2;
	text_area.y = cell_area->y + (pixbuf_area.height + PAD);
	text_area.height = height;
	text_area.width = width;

	gtk_cell_renderer_render (cell->image, cr, widget, 
				  background_area, &pixbuf_area, flags);

	gtk_cell_renderer_render (cell->caption, cr, widget,
				  background_area, &text_area, flags);
}

static void
anjuta_cell_renderer_captioned_image_dispose (GObject *obj)
{
	AnjutaCellRendererCaptionedImage *cell = ANJUTA_CELL_RENDERER_CAPTIONED_IMAGE (obj);
	
	g_object_unref (cell->image);
	g_object_unref (cell->caption);
	
	G_OBJECT_CLASS (anjuta_cell_renderer_captioned_image_parent_class)->dispose (obj);
}

static void
anjuta_cell_renderer_captioned_image_finalize (GObject *obj)
{	
	G_OBJECT_CLASS (anjuta_cell_renderer_captioned_image_parent_class)->finalize (obj);
}

static void
anjuta_cell_renderer_captioned_image_init (AnjutaCellRendererCaptionedImage *cell)
{
	cell->image = gtk_cell_renderer_pixbuf_new ();
	cell->caption = gtk_cell_renderer_text_new ();
}

static void
anjuta_cell_renderer_captioned_image_class_init (AnjutaCellRendererCaptionedImageClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	
	object_class->dispose = anjuta_cell_renderer_captioned_image_dispose;
	object_class->finalize = anjuta_cell_renderer_captioned_image_finalize;
	
	object_class->get_property = anjuta_cell_renderer_captioned_image_get_property;
	object_class->set_property = anjuta_cell_renderer_captioned_image_set_property;
	
	cell_class->get_preferred_height = anjuta_cell_renderer_captioned_image_get_preferred_height;
	cell_class->get_preferred_width = anjuta_cell_renderer_captioned_image_get_preferred_width;
    
	cell_class->render = anjuta_cell_renderer_captioned_image_render;

	g_object_class_install_property (object_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
							      _("Text"),
							      _("Text to render"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_PIXBUF,
					 g_param_spec_object ("pixbuf",
							      _("Pixbuf Object"),
							      _("The pixbuf to render."),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_READWRITE));
}

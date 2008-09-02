/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-message-area.c
 * This file is part of anjuta
 *
 * Copyright (C) 2005 - Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the anjuta Team, 2005. See the AUTHORS file for a
 * list of people on the anjuta Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

/* TODO: Style properties */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "anjuta-message-area.h"
#include "anjuta-utils.h"

#define ANJUTA_MESSAGE_AREA_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
                                                ANJUTA_TYPE_MESSAGE_AREA, \
                                                AnjutaMessageAreaPrivate))

struct _AnjutaMessageAreaPrivate
{
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *action_area;

	gboolean changing_style;
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
	gint response_id;
};

enum {
	RESPONSE,
	CLOSE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE(AnjutaMessageArea, anjuta_message_area, GTK_TYPE_HBOX)

static void
anjuta_message_area_finalize (GObject *object)
{
	G_OBJECT_CLASS (anjuta_message_area_parent_class)->finalize (object);
}

static ResponseData *
get_response_data (GtkWidget *widget, gboolean create)
{
	ResponseData *ad = g_object_get_data (G_OBJECT (widget),
	                                      "anjuta-message-area-response-data");

	if (ad == NULL && create)
	{
		ad = g_new (ResponseData, 1);

		g_object_set_data_full (G_OBJECT (widget),
		                        "anjuta-message-area-response-data",
		                        ad,
		                        g_free);
		}

	return ad;
}

static gboolean
paint_message_area (GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	gtk_paint_flat_box (widget->style,
	                    widget->window,
	                    GTK_STATE_NORMAL,
	                    GTK_SHADOW_OUT,
	                    NULL,
	                    widget,
	                    "tooltip",
	                    widget->allocation.x + 1,
	                    widget->allocation.y + 1,
	                    widget->allocation.width - 2,
	                    widget->allocation.height - 2);

	return FALSE;
}

static void
anjuta_message_area_class_init (AnjutaMessageAreaClass *klass)
{
	GObjectClass *object_class;
	GtkBindingSet *binding_set;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = anjuta_message_area_finalize;

	//klass->close = anjuta_message_area_close;

	g_type_class_add_private (object_class, sizeof(AnjutaMessageAreaPrivate));

	signals[RESPONSE] = g_signal_new ("response",
					  G_OBJECT_CLASS_TYPE (klass),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (AnjutaMessageAreaClass, response),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__INT,
					  G_TYPE_NONE, 1,
					  G_TYPE_INT);

	signals[CLOSE] =  g_signal_new ("close",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (AnjutaMessageAreaClass, close),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);

	binding_set = gtk_binding_set_by_class (klass);

	gtk_binding_entry_add_signal (binding_set, GDK_Escape, 0, "close", 0);
}

static void
style_set (GtkWidget *widget, GtkStyle *prev_style, AnjutaMessageArea *message_area)
{
	GtkWidget *window;
	GtkStyle *style;

	if (message_area->priv->changing_style)
		return;

	/* This is a hack needed to use the tooltip background color */
	window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_widget_set_name (window, "gtk-tooltip");
	gtk_widget_ensure_style (window);
	style = gtk_widget_get_style (window);

	message_area->priv->changing_style = TRUE;
	gtk_widget_set_style (GTK_WIDGET (message_area), style);
	message_area->priv->changing_style = FALSE;

	gtk_widget_destroy (window);

	gtk_widget_queue_draw (GTK_WIDGET (message_area));
}

static void
anjuta_message_area_init (AnjutaMessageArea *message_area)
{
	GtkWidget *main_hbox;
	GtkWidget *image;
	GtkWidget *label;
	
	message_area->priv = ANJUTA_MESSAGE_AREA_GET_PRIVATE (message_area);

	main_hbox = gtk_hbox_new (FALSE, 16);
	gtk_widget_show (main_hbox);

	image = gtk_image_new ();
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (main_hbox), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
	message_area->priv->image = image;

	label = gtk_label_new (NULL);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (main_hbox), label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	GTK_WIDGET_SET_FLAGS (label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	message_area->priv->label = label;

	gtk_widget_show (main_hbox);
	gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 8);

	message_area->priv->action_area = gtk_vbox_new (TRUE, 8);
	gtk_widget_show (message_area->priv->action_area);
	gtk_box_pack_end (GTK_BOX (main_hbox),
	                  message_area->priv->action_area,
	                  FALSE,
	                  FALSE,
	                  0);

	gtk_box_pack_start (GTK_BOX (message_area),
	                    main_hbox,
	                    TRUE,
	                    TRUE,
	                    0);

	gtk_widget_set_app_paintable (GTK_WIDGET (message_area), TRUE);

	g_signal_connect (message_area,
	                  "expose-event",
	                  G_CALLBACK (paint_message_area),
	                  NULL);

	/* Note that we connect to style-set on one of the internal
	 * widgets, not on the message area itself, since gtk does
	 * not deliver any further style-set signals for a widget on
	 * which the style has been forced with gtk_widget_set_style() */
	g_signal_connect (main_hbox,
	                  "style-set",
	                  G_CALLBACK (style_set),
	                  message_area);
}

static gint
get_response_for_widget (AnjutaMessageArea *message_area, GtkWidget *widget)
{
	ResponseData *rd;

	rd = get_response_data (widget, FALSE);
	if (!rd)
		return GTK_RESPONSE_NONE;
	else
		return rd->response_id;
}

static void
action_widget_activated (GtkWidget *widget, AnjutaMessageArea *message_area)
{
	gint response_id;

	response_id = get_response_for_widget (message_area, widget);
	anjuta_message_area_response (message_area, response_id);
}

/**
 * anjuta_message_area_set_text:
 * @message_area: an #AnjutaMessageArea
 * @text: the text to set in the message area
 *
 * Sets the visualized text of the #AnjutaMessageArea.
 */
void
anjuta_message_area_set_text (AnjutaMessageArea *message_area,
                              const gchar *text)
{
	gchar *markup;

	if (text != NULL)
	{
		markup = g_strdup_printf ("<b>%s</b>", text);
		gtk_label_set_markup (GTK_LABEL (message_area->priv->label), markup);
		g_free (markup);
	}
}

/**
 * anjuta_message_area_set_image:
 * @message_area: an #AnjutaMessageArea
 * @icon_stock_id: a stock image ID
 *
 * Sets the visualized icon of the #AnjutaMessageArea.
 */
void
anjuta_message_area_set_image (AnjutaMessageArea *message_area,
                               const gchar *icon_stock_id)
{
	if (icon_stock_id != NULL)
	{
		gtk_image_set_from_stock (GTK_IMAGE (message_area->priv->image),
		                          icon_stock_id, GTK_ICON_SIZE_DIALOG);
	}
}

static void
anjuta_message_area_add_action_widget (AnjutaMessageArea *message_area, 
									   GtkWidget *child, gint response_id)
{
	ResponseData *ad;
	guint signal_id;
	GClosure *closure;

	g_return_if_fail (ANJUTA_IS_MESSAGE_AREA (message_area));
	g_return_if_fail (GTK_IS_BUTTON (child));

	ad = get_response_data (child, TRUE);

	ad->response_id = response_id;

	signal_id = g_signal_lookup ("clicked", GTK_TYPE_BUTTON);
	closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
					 G_OBJECT (message_area));

	g_signal_connect_closure_by_id (child,
	                                signal_id,
	                                0,
	                                closure,
	                                FALSE);

	gtk_box_pack_start (GTK_BOX (message_area->priv->action_area),
	                             child,
	                             FALSE,
	                             FALSE,
	                             0);
}

/**
 * anjuta_message_area_new:
 * 
 * Creates a new #AnjutaMessageArea object.
 * 
 * Returns: a new #AnjutaMessageArea object
 */
GtkWidget *
anjuta_message_area_new (const gchar    *text,
                         const gchar    *icon_stock_id)
{
	AnjutaMessageArea *message_area;
	
	message_area = g_object_new (ANJUTA_TYPE_MESSAGE_AREA, NULL);
	anjuta_message_area_set_text (message_area, text);
	anjuta_message_area_set_image (message_area, icon_stock_id);
	
	return GTK_WIDGET (message_area);
}

/**
 * anjuta_message_area_response:
 * @message_area: an #AnjutaMessageArea
 * @response_id: a response ID
 *
 * Emits the 'response' signal with the given @response_id.
 */
void
anjuta_message_area_response (AnjutaMessageArea *message_area,
							  gint			   response_id)
{
	g_return_if_fail (ANJUTA_IS_MESSAGE_AREA (message_area));

	g_signal_emit (message_area,
				   signals[RESPONSE],
				   0,
				   response_id);
}

/**
 * anjuta_message_area_add_button:
 * @message_area: an #AnjutaMessageArea
 * @button_text: text of button, or stock ID
 * @response_id: response ID for the button
 * 
 * Adds a button with the given text (or a stock button, if button_text is a stock ID)
 * and sets things up so that clicking the button will emit the "response" signal
 * with the given response_id. The button is appended to the end of the message area's
 * action area. The button widget is returned, but usually you don't need it.
 *
 * Returns: the button widget that was added
 */
GtkWidget*
anjuta_message_area_add_button (AnjutaMessageArea *message_area,
								const gchar       *button_text,
								gint               response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (ANJUTA_IS_MESSAGE_AREA (message_area), NULL);
	g_return_val_if_fail (button_text != NULL, NULL);

	button = gtk_button_new_from_stock (button_text);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	anjuta_message_area_add_action_widget (message_area,
	                                       button,
	                                       response_id);

	return button;
}

/**
 * anjuta_message_area_add_button_with_stock_image:
 * @message_area: an #AnjutaMessageArea
 * @text: the text to visualize in the button
 * @stock_id: the stock ID of the button
 * @response_id: a response ID
 *
 * Same as anjuta_message_area_add_button() but with a specific icon.
 */
GtkWidget *
anjuta_message_area_add_button_with_stock_image (AnjutaMessageArea *message_area,
                                                 const gchar	   *text,
                                                 const gchar	   *stock_id,
                                                 gint			   response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (ANJUTA_IS_MESSAGE_AREA (message_area), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = anjuta_util_button_new_with_stock_image (text, stock_id);
	gtk_widget_show (button);

	anjuta_message_area_add_action_widget (message_area,
										   button,
										   response_id);

	return button;
}


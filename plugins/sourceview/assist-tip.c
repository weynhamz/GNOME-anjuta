/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta-trunk.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "assist-tip.h"
#include <gtk/gtklabel.h>

#include <libanjuta/anjuta-debug.h>

G_DEFINE_TYPE (AssistTip, assist_tip, GTK_TYPE_WINDOW);

enum
{
	COLUMN_TIP,
	N_COLUMNS
};

static void
assist_tip_init (AssistTip *object)
{
	object->label = gtk_label_new ("Tip");
	
	gtk_container_add (GTK_CONTAINER(object), object->label);
}

static void
assist_tip_finalize (GObject *object)
{
	G_OBJECT_CLASS (assist_tip_parent_class)->finalize (object);
}

static void
assist_tip_class_init (AssistTipClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = assist_tip_finalize;
}

void
assist_tip_set_tips (AssistTip* tip, GList* tips)
{
	GList* cur_tip;
	gchar* text = NULL;
	gchar* tip_text;
	for (cur_tip = tips; cur_tip != NULL; cur_tip = g_list_next (cur_tip))
	{
		if (!text)
		{
			text = g_strdup(cur_tip->data);
			continue;
		}
		gchar* new_text =
			g_strconcat(text, "\n", cur_tip->data, NULL);
		g_free(text);
		text = new_text;
	}
	tip_text = g_strdup_printf("<tt>%s</tt>", text);
	gtk_label_set_markup(GTK_LABEL(tip->label), tip_text);
	gtk_widget_show (tip->label);
	g_free(text);
	g_free(tip_text);
}

/* Return a tuple containing the (x, y) position of the cursor + 1 line */
static void
assist_tip_get_coordinates(GtkWidget* view, int offset, int* x, int* y, GtkWidget* entry)
{
	int xor, yor;
	/* We need to Rectangles because if we step to the next line
	the x position is lost */
	GtkRequisition entry_req;
	GdkRectangle rectx;
	GdkRectangle recty;
	gint view_width;
	gint width_left;
	GdkWindow* window;
	GtkTextIter iter;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 
									   offset); 
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &iter, &rectx);
	
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &iter, &recty);
	window = gtk_text_view_get_window(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT, 
		rectx.x + rectx.width, recty.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
	

	/* Compute entry width/height */
	gtk_widget_size_request(entry, &entry_req);
	
	/* ensure that the tip is inside the text_view */
	gdk_drawable_get_size (GDK_DRAWABLE(window), &view_width, NULL);
	width_left = (xor + view_width) - (*x + entry_req.width);
	DEBUG_PRINT ("width_left: %d", width_left);
	if (width_left < 0)
	{
		*x += width_left;
	}
	
	*y -= entry_req.height;
}

void
assist_tip_move(AssistTip* assist_tip, GtkTextView* text_view, int offset)
{
	int x,y;
	assist_tip_get_coordinates(GTK_WIDGET(text_view), offset, &x, &y, assist_tip->label);	
	gtk_window_move(GTK_WINDOW(assist_tip), x, y);
}

gint assist_tip_get_position (AssistTip* tip)
{	
	return tip->position;
}

GtkWidget*
assist_tip_new (GtkTextView* view, GList* tips)
{
	GtkTextBuffer* buffer;
	GtkTextIter iter;
	GtkTextMark* mark;
	AssistTip* assist_tip;
	GObject* object = 
		g_object_new (ASSIST_TYPE_TIP, "type", GTK_WINDOW_POPUP, NULL);
	
	assist_tip = ASSIST_TIP (object);
	
	assist_tip_set_tips (assist_tip, tips);
	
	buffer = gtk_text_view_get_buffer (view);
	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);
	/* Position is off-by-one because we want the '(' as first position */
	assist_tip->position = gtk_text_iter_get_offset (&iter) - 1;
	assist_tip_move (assist_tip, view, assist_tip->position);
	
	return GTK_WIDGET(object);
}

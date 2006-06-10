/*
    data_view.c
    Copyright (C) 2006 Sebastien Granjoux

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

#include "data_view.h"

#include "chunk_view.h"

#include "anjuta-marshal.h"

#include <gdk/gdkkeysyms.h>

#include <stdlib.h>

/* Size in pixel of border between text view widget */
#define ADDRESS_BORDER 4
#define ASCII_BORDER 2
#define SCROLLBAR_SPACING 4

struct _DmaDataView
{
	GtkContainer parent;
	
	GtkWidget *address;
	GtkWidget *data;
	GtkWidget *ascii;
	GtkWidget *range;
	
	GtkWidget *goto_window;
	GtkWidget *goto_entry;
	
	guint16 shadow_type;
	GtkAllocation frame;

	GtkTextBuffer *adr_buffer;
	GtkTextBuffer *data_buffer;
	GtkTextBuffer *ascii_buffer;

	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;
	
	GtkAdjustment* view_range;
	GtkAdjustment* buffer_range;
	
	DmaDataBuffer *buffer;
	
	gulong start;
	
	guint bytes_by_line;
	guint line_by_page;
	guint char_by_byte;
	guint char_height;
	guint char_width;
};

struct _DmaDataViewClass
{
	GtkContainerClass parent_class;
};

static GtkWidgetClass *parent_class = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
get_widget_char_size (GtkWidget *widget, gint *width, gint *height)
{
	PangoFontMetrics *metrics;
	PangoContext *context;
	PangoFontDescription *font_desc;

	font_desc = pango_font_description_from_string ("Monospace 10");
	
	context = gtk_widget_get_pango_context (widget);
	metrics = pango_context_get_metrics (context,
				       /*widget->style->font_desc,*/
					   font_desc,
				       pango_context_get_language (context));

	*height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
							   pango_font_metrics_get_descent (metrics));
	*width = (pango_font_metrics_get_approximate_char_width (metrics) + PANGO_SCALE - 1) / PANGO_SCALE;
	pango_font_metrics_unref (metrics);
	pango_font_description_free (font_desc);
}

/* Goto address window
 *---------------------------------------------------------------------------*/

static void
dma_data_view_goto_window_hide (DmaDataView *view)
{
  gtk_widget_hide (view->goto_window);
  gtk_entry_set_text (GTK_ENTRY (view->goto_entry), "");
}

static gboolean
dma_data_view_goto_delete_event (GtkWidget *widget,
								 GdkEventAny *event,
								 DmaDataView *view)
{
	g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

	dma_data_view_goto_window_hide (view);

	return TRUE;
}

static gboolean
dma_data_view_goto_key_press_event (GtkWidget *widget,
									GdkEventKey *event,
									DmaDataView *view)
{
	g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
	g_return_val_if_fail (IS_DMA_DATA_VIEW (view), FALSE);

	/* Close window */
	if (event->keyval == GDK_Escape ||
		event->keyval == GDK_Tab ||
		event->keyval == GDK_KP_Tab ||
		event->keyval == GDK_ISO_Left_Tab)
    {
		dma_data_view_goto_window_hide (view);
		return TRUE;
    }

	/* Goto to address and close window */
	if (event->keyval == GDK_Return ||
		event->keyval == GDK_ISO_Enter ||
		event->keyval == GDK_KP_Enter)
	{
		gulong adr;
		const gchar *text;
		gchar *end;
		
		text = gtk_entry_get_text (GTK_ENTRY (view->goto_entry));
		adr = strtoul (text, &end, 0);
		
		if ((*text != '\0') && (*end == '\0'))
		{
			/* Valid input goto to address */
			gtk_adjustment_set_value (view->buffer_range, adr);
		}
		
		dma_data_view_goto_window_hide (view);
		return TRUE;
	}
	
	return FALSE;
}

static void
dma_data_view_goto_position_func (DmaDataView *view)
{
	gint x, y;
	gint win_x, win_y;
	gint win_width, win_height;
	GdkWindow *window = GTK_WIDGET (view)->window;
	GdkScreen *screen = gdk_drawable_get_screen (window);
	GtkRequisition requisition;
	gint monitor_num;
	GdkRectangle monitor;

	monitor_num = gdk_screen_get_monitor_at_window (screen, window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	gtk_widget_realize (view->goto_window);

	gdk_window_get_origin (window, &win_x, &win_y);
	gdk_drawable_get_size (window,
						   &win_width,
						   &win_height);
	gtk_widget_size_request (view->goto_window, &requisition);
	
	if (win_x + win_width - requisition.width > gdk_screen_get_width (screen))
		x = gdk_screen_get_width (screen) - requisition.width;
	else if (win_x + win_width - requisition.width < 0)
		x = 0;
	else
		x = win_x + win_width - requisition.width;

	if (win_y + win_height > gdk_screen_get_height (screen))
		y = gdk_screen_get_height (screen) - requisition.height;
	else if (win_y + win_height < 0) /* isn't really possible ... */
		y = 0;
	else
		y = win_y + win_height;
	
	gtk_window_move (GTK_WINDOW (view->goto_window), x, y);
}

static void
dma_data_view_goto_activate (GtkWidget   *menu_item,
		       DmaDataView *view)
{
	GtkWidget *toplevel;
	GtkWidget *frame;
	GtkWidget *vbox;
	
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
	
	if (view->goto_window != NULL)
	{
		if (GTK_WINDOW (toplevel)->group)
			gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
										 GTK_WINDOW (view->goto_window)); 
		else if (GTK_WINDOW (view->goto_window)->group)
			gtk_window_group_remove_window (GTK_WINDOW (view->goto_window)->group,
											GTK_WINDOW (view->goto_window)); 
	
		dma_data_view_goto_position_func (view);	
		gtk_widget_show (view->goto_window);
		return;
	}

	view->goto_window = gtk_window_new (GTK_WINDOW_POPUP);

	if (GTK_WINDOW (toplevel)->group)
		gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
									 GTK_WINDOW (view->goto_window));

	gtk_window_set_modal (GTK_WINDOW (view->goto_window), TRUE);
	g_signal_connect (view->goto_window, "delete_event",
					  G_CALLBACK (dma_data_view_goto_delete_event),
					  view);
	g_signal_connect (view->goto_window, "key_press_event",
					  G_CALLBACK (dma_data_view_goto_key_press_event),
					  view);
  
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (view->goto_window), frame);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	/* add entry */
	view->goto_entry = gtk_entry_new ();
	gtk_widget_show (view->goto_entry);
	gtk_container_add (GTK_CONTAINER (vbox),
					   view->goto_entry);
					   
	gtk_widget_realize (view->goto_entry);
	dma_data_view_goto_position_func (view);	
	gtk_widget_show (view->goto_window);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
dma_data_view_address_size_request (DmaDataView *view,
									GtkRequisition *requisition)
{
	PangoLayout *layout;
	char text[] = "0";
	int width;
	int height;
	
	layout = gtk_widget_create_pango_layout(view->address, text);
	pango_layout_get_pixel_size(layout, &(requisition->width), &(requisition->height));
	do
	{
		text[0]++;
		pango_layout_get_pixel_size(layout, &width, &height);
		if (width > requisition->width) requisition->width = width;
		if (height > requisition->height) requisition->height = height;
	} while (text[0] == '9');
	for (text[0] = 'A'; text[0] != 'G'; text[0]++)
	{
		pango_layout_get_pixel_size(layout, &width, &height);
		if (width > requisition->width) requisition->width = width;
		if (height > requisition->height) requisition->height = height;
	}
	g_object_unref(G_OBJECT(layout));
	
	requisition->width *= sizeof(view->start) * 2;
}

static void
dma_data_view_data_size_request (DmaDataView *view,
									GtkRequisition *requisition)
{
	PangoFontMetrics *metrics;
	PangoContext *context;

	context = gtk_widget_get_pango_context (view->data);
	metrics = pango_context_get_metrics (context,
				       view->data->style->font_desc,
				       pango_context_get_language (context));

	requisition->height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
							   pango_font_metrics_get_descent (metrics));
	requisition->width = (pango_font_metrics_get_approximate_char_width (metrics) + PANGO_SCALE - 1) / PANGO_SCALE;
	pango_font_metrics_unref (metrics);
}

static void
dma_data_view_ascii_size_request (DmaDataView *view,
									GtkRequisition *requisition)
{
	PangoFontMetrics *metrics;
	PangoContext *context;

	context = gtk_widget_get_pango_context (view->ascii);
	metrics = pango_context_get_metrics (context,
				       view->ascii->style->font_desc,
				       pango_context_get_language (context));

	requisition->height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
							   pango_font_metrics_get_descent (metrics));
	requisition->width = (pango_font_metrics_get_approximate_char_width (metrics) + PANGO_SCALE - 1) / PANGO_SCALE;
	
	pango_font_metrics_unref (metrics);
}

static void
dma_data_view_value_changed (GtkAdjustment *adj,
                             DmaDataView   *view)
{
	view->start = ((gulong)adj->value) - (((gulong)adj->value) % view->bytes_by_line);
	dma_data_view_refresh (view);
}

static void
dma_data_view_populate_popup (GtkTextView *widget,
							  GtkMenu *menu,
							  DmaDataView *view)
{
	GtkWidget *menu_item;
	
	/* separator */
	menu_item = gtk_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	/* create goto menu_item. */
	menu_item = gtk_menu_item_new_with_mnemonic ("_Goto address");
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (dma_data_view_goto_activate), view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
}

static void
dma_data_view_size_request (GtkWidget *widget,
                       GtkRequisition *requisition)
{
	DmaDataView *view = (DmaDataView *)widget;
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (view->range, requisition);
	
	dma_data_view_address_size_request (view, &child_requisition);
	if (child_requisition.height > requisition->height) requisition->height = child_requisition.height;
	requisition->width += child_requisition.width;
	requisition->width += ADDRESS_BORDER;
	
	dma_data_view_data_size_request (view, &child_requisition);
	if (child_requisition.height > requisition->height) requisition->height = child_requisition.height;
	requisition->width += child_requisition.width * view->char_by_byte;
	requisition->width += ASCII_BORDER;
	
	dma_data_view_ascii_size_request (view, &child_requisition);
	if (child_requisition.height > requisition->height) requisition->height = child_requisition.height;
	requisition->width += child_requisition.width;
	
	if (view->shadow_type != GTK_SHADOW_NONE)
	{
		requisition->width += 2 * widget->style->xthickness;
		requisition->height += 2 * widget->style->ythickness;
	}
	requisition->width += SCROLLBAR_SPACING;
}

static void
dma_data_view_size_allocate (GtkWidget *widget,
                             GtkAllocation *allocation)
{
	DmaDataView *view = (DmaDataView *)widget;
	GtkAllocation child_allocation;
	GtkRequisition range_requisition;
	GtkRequisition address_requisition;
	GtkRequisition data_requisition;
	GtkRequisition ascii_requisition;
	gint width;
	gint height;
	gint bytes_by_line;
	gint step;
	gboolean need_fill = FALSE;
	
 	gtk_widget_get_child_requisition (view->range, &range_requisition);
	dma_data_view_address_size_request (view, &address_requisition);
	dma_data_view_data_size_request (view, &data_requisition);
	dma_data_view_ascii_size_request (view, &ascii_requisition);
	
	/* Compute number of byte per line */
	width = allocation->width
	        - 2 * GTK_CONTAINER (view)->border_width
	        - (view->shadow_type == GTK_SHADOW_NONE ? 0 : 2 * widget->style->xthickness)
	        - ADDRESS_BORDER
	        - ASCII_BORDER
	        - SCROLLBAR_SPACING
	        - range_requisition.width
	        - address_requisition.width
	        - data_requisition.width * view->char_by_byte
	        - ascii_requisition.width;

	step = (ascii_requisition.width + data_requisition.width * (view->char_by_byte + 1));
	for (bytes_by_line = 1; width >= step * bytes_by_line; bytes_by_line *= 2)
	{
		width -=  step * bytes_by_line;
	}
	
	if (bytes_by_line != view->bytes_by_line)
	{
		need_fill = TRUE;
		view->bytes_by_line = bytes_by_line;
	}

	/* Compute number of line by page */
	height = allocation->height
	        - 2 * GTK_CONTAINER (view)->border_width
	        - (view->shadow_type == GTK_SHADOW_NONE ? 0 : 2 * widget->style->ythickness);
	
	if (view->line_by_page != (height / address_requisition.height))
	{
		need_fill = TRUE;
		view->line_by_page = (height / address_requisition.height);
	}
	
	child_allocation.y = allocation->y + GTK_CONTAINER (view)->border_width;
	child_allocation.height = MAX (1, (gint) allocation->height - (gint) GTK_CONTAINER (view)->border_width * 2);

	/* Scroll bar */
	child_allocation.x = allocation->x + allocation->width - GTK_CONTAINER (view)->border_width - range_requisition.width;
	child_allocation.width = range_requisition.width;
	gtk_widget_size_allocate (view->range, &child_allocation);

	child_allocation.x = allocation->x + GTK_CONTAINER (view)->border_width;
	
	/* Frame */
	if (view->shadow_type != GTK_SHADOW_NONE)
	{
		view->frame.x = allocation->x + GTK_CONTAINER (view)->border_width;
		view->frame.y = allocation->y + GTK_CONTAINER (view)->border_width;
		view->frame.width = allocation->width - range_requisition.width - SCROLLBAR_SPACING - 2 * (GTK_CONTAINER (view)->border_width);
		view->frame.height = allocation->height - 2 * (GTK_CONTAINER (view)->border_width);
		child_allocation.x += widget->style->xthickness;
		child_allocation.y += widget->style->ythickness;
		child_allocation.height -= 2 * widget->style->ythickness;
	}
	
	/* Address */
	child_allocation.width = address_requisition.width;
	gtk_widget_size_allocate (view->address, &child_allocation);
	child_allocation.x += child_allocation.width;

	child_allocation.x += ADDRESS_BORDER;

	/* Data */

	child_allocation.width = (view->bytes_by_line * (view->char_by_byte + 1) - 1) * data_requisition.width;
	gtk_widget_size_allocate (view->data, &child_allocation);
	child_allocation.x += child_allocation.width;

	child_allocation.x += ASCII_BORDER;

	/* Ascii */
	child_allocation.width = view->bytes_by_line * ascii_requisition.width;
	gtk_widget_size_allocate (view->ascii, &child_allocation);
	child_allocation.x += child_allocation.width;

	if (need_fill)
	{
		view->buffer_range->step_increment = view->bytes_by_line;
		view->buffer_range->page_increment = view->bytes_by_line * (view->line_by_page - 1);
		view->buffer_range->page_size = (gulong)(view->buffer_range->upper) % (view->bytes_by_line) + view->buffer_range->page_increment;

		if (view->start + view->buffer_range->page_size > view->buffer_range->upper)
		{
			view->start = view->buffer_range->upper - view->buffer_range->page_size + view->bytes_by_line - 1;
			view->start -= view->start % view->bytes_by_line;
		}
		dma_data_view_refresh (view);
	}

}

static void
dma_data_view_paint (GtkWidget    *widget,
                     GdkRectangle *area)
{
	DmaDataView *view = DMA_DATA_VIEW (widget);

	if (view->shadow_type != GTK_SHADOW_NONE)
    {
		gtk_paint_shadow (widget->style, widget->window,
                        GTK_STATE_NORMAL, view->shadow_type,
                        area, widget, "dma_data_view",
                        view->frame.x,
                        view->frame.y,
                        view->frame.width,
                        view->frame.height);
	}
}

static gint
dma_data_view_expose (GtkWidget *widget,
                      GdkEventExpose *event)
{
	if (GTK_WIDGET_DRAWABLE (widget))
	{
		dma_data_view_paint (widget, &event->area);

		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);
    }

	return FALSE;
}

static void
dma_data_view_create_widget (DmaDataView *view)
{
	GtkAdjustment *adj;
	GtkWidget* wid;
	PangoFontDescription *font_desc;

	wid = GTK_WIDGET (view);
	
	GTK_WIDGET_SET_FLAGS (wid, GTK_NO_WINDOW);
	GTK_WIDGET_SET_FLAGS (wid, GTK_CAN_FOCUS);
	gtk_widget_set_redraw_on_allocate (wid, FALSE); 	
	
	view->char_by_byte = 2;
	view->bytes_by_line = 16;
	view->line_by_page = 16;
	
	view->hadjustment = NULL;
	view->vadjustment = NULL;
	
	view->shadow_type = GTK_SHADOW_IN;

	view->goto_window = NULL;
	view->goto_entry = NULL;
	
	font_desc = pango_font_description_from_string ("Monospace 10");
	
	view->buffer_range = GTK_ADJUSTMENT (gtk_adjustment_new (0,
											 dma_data_buffer_get_lower (view->buffer),
											 dma_data_buffer_get_upper (view->buffer)
											 ,1,4,4));
	g_signal_connect (view->buffer_range, "value_changed",
                        G_CALLBACK (dma_data_view_value_changed), view);
	
	gtk_widget_push_composite_child ();
	
	wid = gtk_vscrollbar_new (view->buffer_range);
	g_object_ref (wid);
	view->range = wid;
	gtk_widget_set_parent (wid, GTK_WIDGET (view));
	adj = view->view_range;
	gtk_widget_show (wid);

	wid = dma_chunk_view_new ();
	g_object_ref (wid);
	gtk_widget_modify_font (wid, font_desc);
	gtk_widget_set_parent (wid, GTK_WIDGET (view));
	gtk_widget_set_size_request (wid, -1, 0);
	gtk_widget_show (wid);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (wid), FALSE);
	view->ascii = wid;
	view->ascii_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wid));
	dma_chunk_view_set_scroll_adjustment (DMA_CHUNK_VIEW (wid), view->buffer_range);
	g_signal_connect (wid, "populate_popup",
                        G_CALLBACK (dma_data_view_populate_popup), view);
	
	wid = dma_chunk_view_new ();
	g_object_ref (wid);
	gtk_widget_modify_font (wid, font_desc);
	gtk_widget_set_parent (wid, GTK_WIDGET (view));
	gtk_widget_set_size_request (wid, -1, 0);
	gtk_widget_show (wid);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (wid), FALSE);
	view->data = wid;
	view->data_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wid));
	dma_chunk_view_set_scroll_adjustment (DMA_CHUNK_VIEW (wid), view->buffer_range);
	g_signal_connect (wid, "populate_popup",
                        G_CALLBACK (dma_data_view_populate_popup), view);

	wid = dma_chunk_view_new ();
	g_object_ref (wid);
	gtk_widget_modify_font (wid, font_desc);
	gtk_widget_set_parent (wid, GTK_WIDGET (view));
	gtk_widget_set_size_request (wid, -1, 0);
	gtk_widget_show (wid);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (wid), FALSE);
	view->address = wid;
	view->adr_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wid));
	dma_chunk_view_set_scroll_adjustment (DMA_CHUNK_VIEW (wid), view->buffer_range);
	g_signal_connect (wid, "populate_popup",
                        G_CALLBACK (dma_data_view_populate_popup), view);
	
	gtk_widget_pop_composite_child ();
	pango_font_description_free (font_desc);
}

static void
dma_data_view_changed_notify (DmaDataBuffer* buffer, gulong lower, gulong upper, DmaDataView *view)
{
	if ((upper >= view->start) && (lower < (view->start + view->line_by_page * view->bytes_by_line)))
	{
		dma_data_view_refresh (view);
	}
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
dma_data_view_goto_address (DmaDataView *view, const void *address)
{
	gtk_adjustment_set_value (view->view_range, (gdouble)((gulong)address));
}

GtkWidget *
dma_data_view_get_address (DmaDataView *view)
{
	return view->address;
}
	
GtkWidget *
dma_data_view_get_data (DmaDataView *view)
{
	return view->data;
}

GtkWidget *
dma_data_view_get_ascii (DmaDataView *view)
{
	return view->ascii;
}

void
dma_data_view_refresh (DmaDataView *view)
{
	gchar *data = "";
	gint offset;
	GtkTextIter cur;
	GtkTextMark *mark;
	GtkTextBuffer *buffer;

	/* Save all cursor offset */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->address));
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
    offset = gtk_text_iter_get_offset (&cur);

	data = dma_data_buffer_get_address (view->buffer, view->start, view->line_by_page * view->bytes_by_line, view->bytes_by_line, sizeof(view->start) * 2);
	gtk_text_buffer_set_text (buffer, data, -1);
	g_free (data);
	
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	gtk_text_iter_set_offset (&cur, offset);
	gtk_text_buffer_move_mark_by_name (buffer, "insert", &cur);
	gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &cur);

	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->data));
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
    offset = gtk_text_iter_get_offset (&cur);

	data = dma_data_buffer_get_data (view->buffer, view->start, view->line_by_page * view->bytes_by_line, view->bytes_by_line, DMA_HEXADECIMAL_BASE);
	gtk_text_buffer_set_text (buffer, data, -1);
	g_free (data);
	
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	gtk_text_iter_set_offset (&cur, offset);
	gtk_text_buffer_move_mark_by_name (buffer, "insert", &cur);
	gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &cur);
	
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->ascii));
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
    offset = gtk_text_iter_get_offset (&cur);

	data = dma_data_buffer_get_data (view->buffer, view->start, view->line_by_page * view->bytes_by_line, view->bytes_by_line, DMA_ASCII_BASE);
	gtk_text_buffer_set_text (buffer, data, -1);
	g_free (data);
	
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	gtk_text_iter_set_offset (&cur, offset);
	gtk_text_buffer_move_mark_by_name (buffer, "insert", &cur);
	gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &cur);
	
}

/* GtkContainer functions
 *---------------------------------------------------------------------------*/

static GType
dma_data_view_child_type (GtkContainer *container)
{
	return GTK_TYPE_NONE;
}

static void
dma_data_view_forall (GtkContainer *container,
                gboolean      include_internals,
                GtkCallback   callback,
                gpointer      callback_data)
{
	DmaDataView *view = DMA_DATA_VIEW (container);

	g_return_if_fail (callback != NULL);

	if (include_internals)
	{
		callback (view->address, callback_data);
		callback (view->data, callback_data);
		callback (view->ascii, callback_data);
		callback (view->range, callback_data);
	}
		
}

/* GtkObject functions
 *---------------------------------------------------------------------------*/

static void
dma_data_view_destroy (GtkObject *object)
{
	DmaDataView *view;

	g_return_if_fail (IS_DMA_DATA_VIEW (object));

	view = DMA_DATA_VIEW (object);

	gtk_widget_unparent (view->address);
	gtk_widget_destroy (view->address);
	gtk_widget_unparent (view->data);
	gtk_widget_destroy (view->data);
	gtk_widget_unparent (view->ascii);
	gtk_widget_destroy (view->ascii);
	gtk_widget_unparent (view->range);
	gtk_widget_destroy (view->range);

	if (view->goto_window)
	{
		gtk_widget_destroy (view->goto_window);
		view->goto_window = NULL;
		view->goto_entry = NULL;
	}
	
	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/* GObject functions
 *---------------------------------------------------------------------------*/

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_data_view_dispose (GObject *object)
{
	DmaDataView *view = DMA_DATA_VIEW (object);

	if (view->buffer != NULL)
	{
		g_signal_handlers_disconnect_by_func (view->buffer,
						      dma_data_view_changed_notify,
						      view);
		g_object_unref (view->buffer);
	}
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_data_view_finalize (GObject *object)
{
	DmaDataView *view = DMA_DATA_VIEW (object);
	
	g_object_unref (view->address);
	g_object_unref (view->data);
	g_object_unref (view->ascii);
	g_object_unref (view->range);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_data_view_instance_init (DmaDataView *view)
{
	view->buffer = NULL;
}

/* class_init intialize the class itself not the instance */

static void
dma_data_view_class_init (DmaDataViewClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GtkWidgetClass   *widget_class;
	GtkContainerClass *container_class;

	g_return_if_fail (klass != NULL);
	
	gobject_class = (GObjectClass *) klass;
	object_class = (GtkObjectClass *) klass;
	widget_class = GTK_WIDGET_CLASS (klass);
	container_class = GTK_CONTAINER_CLASS (klass);
	parent_class = (GtkWidgetClass*) g_type_class_peek_parent (klass);
	
	gobject_class->dispose = dma_data_view_dispose;
	gobject_class->finalize = dma_data_view_finalize;

	object_class->destroy = dma_data_view_destroy;
	
	widget_class->size_request = dma_data_view_size_request;
	widget_class->size_allocate = dma_data_view_size_allocate;
	widget_class->expose_event = dma_data_view_expose;
	
	container_class->forall = dma_data_view_forall;
	container_class->child_type = dma_data_view_child_type;

}

GType
dma_data_view_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaDataViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_data_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaDataView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) dma_data_view_instance_init,
			NULL            /* value_table */
		};

		type = g_type_register_static (GTK_TYPE_CONTAINER,
		                            "DmaDataView", &type_info, 0);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

GtkWidget*
dma_data_view_new_with_buffer (DmaDataBuffer* buffer)
{
	DmaDataView *view;

	view = g_object_new (DMA_DATA_VIEW_TYPE, NULL);
	g_assert (view != NULL);

	view->buffer = buffer;
	g_object_ref (buffer);
	
	g_signal_connect (G_OBJECT (buffer), 
			  "changed_notify", 
			  G_CALLBACK (dma_data_view_changed_notify), 
			  view);
	
	dma_data_view_create_widget (view);
	
	return GTK_WIDGET (view);
}
	
void
dma_data_view_free (DmaDataView *view)
{
	g_object_unref (view);
}


/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    sparse_view.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * It is a GtkTextView with an associated GtkAjustment, allowing to put in the
 * GtkTextView only a part of a big amount of text. Cursor movements are not
 * limited by the GtkTextView content but by the GtkAjustment.
 *---------------------------------------------------------------------------*/
 
#include "sparse_view.h"

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-markable.h>

#include "sexy-icon-entry.h"

#include <libgnome/gnome-i18n.h>

#include <gdk/gdkkeysyms.h>

#include <stdlib.h>
#include <string.h>

/* Constants
 *---------------------------------------------------------------------------*/

/* Minimum size left window showing address */
#define MIN_NUMBER_WINDOW_WIDTH		20
#define GUTTER_PIXMAP 			16
#define COMPOSITE_ALPHA                 225

#define MAX_MARKER		32	/* Must be smaller than the number of bits in an int */

/* Order is important, as marker with the lowest number is drawn first */
#define SPARSE_VIEW_BOOKMARK                0
#define SPARSE_VIEW_BREAKPOINT_DISABLED     1
#define SPARSE_VIEW_BREAKPOINT_ENABLED      2
#define SPARSE_VIEW_PROGRAM_COUNTER         3
#define SPARSE_VIEW_LINEMARKER              4

#define MARKER_PIXMAP_BOOKMARK "anjuta-bookmark-16.png"
#define MARKER_PIXMAP_LINEMARKER "anjuta-linemark-16.png"
#define MARKER_PIXMAP_PROGRAM_COUNTER "anjuta-pcmark-16.png"
#define MARKER_PIXMAP_BREAKPOINT_DISABLED "anjuta-breakpoint-disabled-16.png"
#define MARKER_PIXMAP_BREAKPOINT_ENABLED "anjuta-breakpoint-enabled-16.png"

/* Types
 *---------------------------------------------------------------------------*/

enum {
	PROP_0,
	PROP_SHOW_LINE_NUMBERS,
	PROP_SHOW_LINE_MARKERS,
};

struct _DmaSparseViewPrivate
{
	GtkTextView parent;
	
	gboolean 	 show_line_numbers;
	gboolean	 show_line_markers;
	
	DmaSparseBuffer* buffer;
	DmaSparseIter start;
	GtkAdjustment *vadjustment;

	GtkWidget *goto_window;
	GtkWidget *goto_entry;

	gint line_by_page;
	gint char_by_line;
	
	guint stamp;
	
	GdkPixbuf *marker_pixbuf[MAX_MARKER];
};

/* Used in dispose and finalize */
static GtkTextViewClass *parent_class = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Cut and paste from gtkwindow.c */
static void
send_focus_change (GtkWidget *widget,
                   gboolean   in)
{
        GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

        g_object_ref (widget);

        if (in)
                GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
        else
                GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

        fevent->focus_change.type = GDK_FOCUS_CHANGE;
        fevent->focus_change.window = g_object_ref (widget->window);
        fevent->focus_change.in = in;

        gtk_widget_event (widget, fevent);

        g_object_notify (G_OBJECT (widget), "has-focus");

        g_object_unref (widget);
        gdk_event_free (fevent);
}

/* Goto address window
 *---------------------------------------------------------------------------*/

static void
dma_sparse_view_goto_window_hide (DmaSparseView *view)
{
  gtk_widget_hide (view->priv->goto_window);
}

static gboolean
dma_sparse_view_goto_delete_event (GtkWidget *widget,
								 GdkEventAny *event,
								 DmaSparseView *view)
{
	g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

	dma_sparse_view_goto_window_hide (view);

	return TRUE;
}

static gboolean
dma_sparse_view_goto_key_press_event (GtkWidget *widget,
									GdkEventKey *event,
									DmaSparseView *view)
{
	g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
	g_return_val_if_fail (DMA_IS_SPARSE_VIEW (view), FALSE);

	/* Close window */
	if (event->keyval == GDK_Escape ||
		event->keyval == GDK_Tab ||
		event->keyval == GDK_KP_Tab ||
		event->keyval == GDK_ISO_Left_Tab)
    {
		dma_sparse_view_goto_window_hide (view);
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
		
		text = gtk_entry_get_text (GTK_ENTRY (view->priv->goto_entry));
		adr = strtoul (text, &end, 0);
		
		if ((*text != '\0') && (*end == '\0'))
		{
			/* Valid input goto to address */
			dma_sparse_view_goto (view, adr);
		}
		
		dma_sparse_view_goto_window_hide (view);
		return TRUE;
	}
	
	return FALSE;
}

static void
dma_sparse_view_goto_position_func (DmaSparseView *view)
{
	gint x, y;
	gint win_x, win_y;
	GdkWindow *window = GTK_WIDGET (view)->window;
	GdkScreen *screen = gdk_drawable_get_screen (window);
	gint monitor_num;
	GdkRectangle monitor;

	monitor_num = gdk_screen_get_monitor_at_window (screen, window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	gtk_widget_realize (view->priv->goto_window);

	gdk_window_get_origin (window, &win_x, &win_y);
	x = MAX(12, win_x + 12);
	y = MAX(12, win_y + 12);
	
	gtk_window_move (GTK_WINDOW (view->priv->goto_window), x, y);
}

static void
dma_sparse_view_goto_activate (GtkWidget   *menu_item,
		       DmaSparseView *view)
{
	GtkWidget *toplevel;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *icon;
	
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
	
	if (view->priv->goto_window != NULL)
	{
		if (GTK_WINDOW (toplevel)->group)
			gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
										 GTK_WINDOW (view->priv->goto_window)); 
		else if (GTK_WINDOW (view->priv->goto_window)->group)
			gtk_window_group_remove_window (GTK_WINDOW (view->priv->goto_window)->group,
											GTK_WINDOW (view->priv->goto_window)); 
	
	}
	else
	{
		view->priv->goto_window = gtk_window_new (GTK_WINDOW_POPUP);

		if (GTK_WINDOW (toplevel)->group)
			gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
										 GTK_WINDOW (view->priv->goto_window));

		gtk_window_set_modal (GTK_WINDOW (view->priv->goto_window), TRUE);
		g_signal_connect (view->priv->goto_window, "delete_event",
						  G_CALLBACK (dma_sparse_view_goto_delete_event),
						  view);
		g_signal_connect (view->priv->goto_window, "key_press_event",
						  G_CALLBACK (dma_sparse_view_goto_key_press_event),
						  view);
  
		frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
		gtk_widget_show (frame);
		gtk_container_add (GTK_CONTAINER (view->priv->goto_window), frame);
	
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vbox);
		gtk_container_add (GTK_CONTAINER (frame), vbox);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

		/* add entry */
		view->priv->goto_entry = sexy_icon_entry_new ();
		icon = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
		sexy_icon_entry_set_icon (SEXY_ICON_ENTRY(view->priv->goto_entry),
    	                          SEXY_ICON_ENTRY_PRIMARY,
                                  GTK_IMAGE (icon));
		gtk_widget_show (view->priv->goto_entry);
		gtk_container_add (GTK_CONTAINER (vbox),
						   view->priv->goto_entry);
					   
		gtk_widget_realize (view->priv->goto_entry);
	}
	
	dma_sparse_view_goto_position_func (view);	
	gtk_entry_set_text (GTK_ENTRY (view->priv->goto_entry), "0x");
	gtk_widget_show (view->priv->goto_window);
	
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
    gtk_widget_grab_focus (view->priv->goto_entry);
    send_focus_change (view->priv->goto_entry, TRUE);
	gtk_editable_set_position (GTK_EDITABLE (view->priv->goto_entry), -1);
}

/* Properties functions
 *---------------------------------------------------------------------------*/

/**
 * dma_sparse_view_get_show_line_numbers:
 * @view: a #DmaSparseView.
 *
 * Returns whether line numbers are displayed beside the text.
 *
 * Return value: %TRUE if the line numbers are displayed.
 **/
gboolean
dma_sparse_view_get_show_line_numbers (DmaSparseView *view)
{
	g_return_val_if_fail (view != NULL, FALSE);
	g_return_val_if_fail (DMA_IS_SPARSE_VIEW (view), FALSE);

	return view->priv->show_line_numbers;
}

/**
 * dma_sparse_view_set_show_line_numbers:
 * @view: a #DmaSparseView.
 * @show: whether line numbers should be displayed.
 *
 * If %TRUE line numbers will be displayed beside the text.
 *
 **/
void
dma_sparse_view_set_show_line_numbers (DmaSparseView *view, gboolean show)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (DMA_IS_SPARSE_VIEW (view));

	show = (show != FALSE);

	if (show) 
	{
		if (!view->priv->show_line_numbers) 
		{
			/* Set left margin to minimum width if no margin is 
			   visible yet. Otherwise, just queue a redraw, so the
			   expose handler will automatically adjust the margin. */
			if (!view->priv->show_line_markers)
				gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (view),
								      GTK_TEXT_WINDOW_LEFT,
								      MIN_NUMBER_WINDOW_WIDTH);
			else
				gtk_widget_queue_draw (GTK_WIDGET (view));

			view->priv->show_line_numbers = show;

			g_object_notify (G_OBJECT (view), "show_line_numbers");
		}
	}
	else 
	{
		if (view->priv->show_line_numbers) 
		{
			view->priv->show_line_numbers = show;

			/* force expose event, which will adjust margin. */
			gtk_widget_queue_draw (GTK_WIDGET (view));

			g_object_notify (G_OBJECT (view), "show_line_numbers");
		}
	}
}

/**
 * dma_sparse_view_get_show_line_markers:
 * @view: a #DmaSparseView.
 *
 * Returns whether line markers are displayed beside the text.
 *
 * Return value: %TRUE if the line markers are displayed.
 **/
gboolean
dma_sparse_view_get_show_line_markers (DmaSparseView *view)
{
	g_return_val_if_fail (view != NULL, FALSE);
	g_return_val_if_fail (DMA_IS_SPARSE_VIEW (view), FALSE);

	return view->priv->show_line_markers;
}

/**
 * dma_sparse_view_set_show_line_markers:
 * @view: a #DmaSparseView.
 * @show: whether line markers should be displayed.
 *
 * If %TRUE line markers will be displayed beside the text.
 *
 **/
void
dma_sparse_view_set_show_line_markers (DmaSparseView *view, gboolean show)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (DMA_IS_SPARSE_VIEW (view));

	show = (show != FALSE);

	if (show) 
	{
		if (!view->priv->show_line_markers) 
		{
			/* Set left margin to minimum width if no margin is 
			   visible yet. Otherwise, just queue a redraw, so the
			   expose handler will automatically adjust the margin. */
			if (!view->priv->show_line_numbers)
				gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (view),
								      GTK_TEXT_WINDOW_LEFT,
								      MIN_NUMBER_WINDOW_WIDTH);
			else
				gtk_widget_queue_draw (GTK_WIDGET (view));

			view->priv->show_line_markers = show;

			g_object_notify (G_OBJECT (view), "show_line_markers");
		}
	} 
	else 
	{
		if (view->priv->show_line_markers) 
		{
			view->priv->show_line_markers = show;

			/* force expose event, which will adjust margin. */
			gtk_widget_queue_draw (GTK_WIDGET (view));

			g_object_notify (G_OBJECT (view), "show_line_markers");
		}
	}
}

/* Markers private functions
 *---------------------------------------------------------------------------*/

static gint
marker_ianjuta_to_view (IAnjutaMarkableMarker marker)
{
        gint mark;
        switch (marker)
        {
                case IANJUTA_MARKABLE_LINEMARKER:
                        mark = SPARSE_VIEW_LINEMARKER;
                        break;
                case IANJUTA_MARKABLE_BOOKMARK:
                        mark = SPARSE_VIEW_BOOKMARK;
                        break;
                case IANJUTA_MARKABLE_BREAKPOINT_DISABLED:
                        mark = SPARSE_VIEW_BREAKPOINT_DISABLED;
                        break;
                case IANJUTA_MARKABLE_BREAKPOINT_ENABLED:
                        mark = SPARSE_VIEW_BREAKPOINT_ENABLED;
                        break;
                case IANJUTA_MARKABLE_PROGRAM_COUNTER:
                        mark = SPARSE_VIEW_PROGRAM_COUNTER;
                        break;
                default:
                        mark = SPARSE_VIEW_LINEMARKER;
        }
	
        return mark;
}

static void
dma_sparse_view_initialize_marker (DmaSparseView *view)
{
	view->priv->marker_pixbuf[SPARSE_VIEW_BOOKMARK] = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BOOKMARK, NULL);
	view->priv->marker_pixbuf[SPARSE_VIEW_BREAKPOINT_DISABLED] = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_DISABLED, NULL);
	view->priv->marker_pixbuf[SPARSE_VIEW_BREAKPOINT_ENABLED] = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_ENABLED, NULL);
	view->priv->marker_pixbuf[SPARSE_VIEW_PROGRAM_COUNTER] = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_PROGRAM_COUNTER, NULL);
	view->priv->marker_pixbuf[SPARSE_VIEW_LINEMARKER] = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_LINEMARKER, NULL);
}

static void
dma_sparse_view_free_marker (DmaSparseView *view)
{
	gint i;
	
	for (i = 0; i < MAX_MARKER; i++)
	{
		if (view->priv->marker_pixbuf[i] != NULL)
		{
			g_object_unref (view->priv->marker_pixbuf[i]);
			view->priv->marker_pixbuf[i] = NULL;
		}
	}
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
dma_sparse_view_populate_popup (DmaSparseView *view,
							  GtkMenu *menu,
							  DmaSparseView *user_data)
{
	GtkWidget *menu_item;
	
	/* separator */
	menu_item = gtk_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	/* create goto menu_item. */
	menu_item = gtk_menu_item_new_with_mnemonic (_("_Goto address"));
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (dma_sparse_view_goto_activate), view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
}


static void
dma_sparse_view_move_cursor (GtkTextView *text_view,
			     GtkMovementStep step,
			     gint            count,
			     gboolean        extend_selection)
{
	DmaSparseView *view = DMA_SPARSE_VIEW (text_view);
	
	switch (step)
    {
    case GTK_MOVEMENT_LOGICAL_POSITIONS:
    case GTK_MOVEMENT_VISUAL_POSITIONS:
	case GTK_MOVEMENT_WORDS:
    case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
    case GTK_MOVEMENT_HORIZONTAL_PAGES:
		break;
    case GTK_MOVEMENT_DISPLAY_LINES:
    case GTK_MOVEMENT_PARAGRAPHS:
    case GTK_MOVEMENT_PARAGRAPH_ENDS:

       	dma_sparse_iter_forward_lines (&view->priv->start, count);
		gtk_adjustment_set_value (view->priv->vadjustment, (gdouble)dma_sparse_iter_get_address (&view->priv->start));
		return;
	case GTK_MOVEMENT_PAGES:
       	dma_sparse_iter_forward_lines (&view->priv->start, count * (view->priv->line_by_page > 1 ? view->priv->line_by_page - 1 : view->priv->line_by_page));
		gtk_adjustment_set_value (view->priv->vadjustment, (gdouble)dma_sparse_iter_get_address (&view->priv->start));
		return;
	case GTK_MOVEMENT_BUFFER_ENDS:
		break;
    default:
        break;
    }
	
	GTK_TEXT_VIEW_CLASS (parent_class)->move_cursor (text_view,
								 step, count,
								 extend_selection);
}

static void
dma_sparse_view_synchronize_iter (DmaSparseView *view, DmaSparseIter *iter)
{
	gdouble dist;
	gdouble pos;
	/* Need to change iterator according to adjustment */
	
	pos = gtk_adjustment_get_value (view->priv->vadjustment);
	dist = pos - (gdouble)dma_sparse_iter_get_address (iter);

	if (dist != 0)
	{
		if ((dist < 4.0 * (view->priv->vadjustment->page_size))
			 && (dist > -4.0 * (view->priv->vadjustment->page_size)))
        {
			gint count = (gint)(dist / view->priv->vadjustment->step_increment);
			
        	dma_sparse_iter_forward_lines (iter, count);
        }
        else
		{
        	dma_sparse_iter_move_at (iter, pos);
            dma_sparse_iter_round (iter, FALSE);
        }
		gtk_adjustment_set_value (view->priv->vadjustment, (gdouble)dma_sparse_iter_get_address (iter));
	}
}

static void
draw_line_markers (DmaSparseView *view,
		   gint		current_marker,
		   gint     x,
		   gint     y)
{
	GdkPixbuf *composite;
	gint width, height;
	gint i;
	
	composite = NULL;
	width = height = 0;

	/* composite all the pixbufs for the markers present at the line */
	for (i = 0; i < MAX_MARKER; i++)
	{
		if (current_marker & (1 << i))
		{
			GdkPixbuf *pixbuf = view->priv->marker_pixbuf[i];
			
			if (pixbuf)
			{
				if (!composite)
				{
					composite = gdk_pixbuf_copy (pixbuf);
					width = gdk_pixbuf_get_width (composite);
					height = gdk_pixbuf_get_height (composite);
				}
				else
				{
					gint pixbuf_w;
					gint pixbuf_h;

					pixbuf_w = gdk_pixbuf_get_width (pixbuf);
					pixbuf_h = gdk_pixbuf_get_height (pixbuf);
					gdk_pixbuf_composite (pixbuf,
							      composite,
							      0, 0,
						    	  width, height,
							      	0, 0,
							      	(double) pixbuf_w / width,
							      (double) pixbuf_h / height,
							      GDK_INTERP_BILINEAR,
							      COMPOSITE_ALPHA);
				}
				//g_object_unref (pixbuf);
			}
			else
			{
				g_warning ("Unknown marker %d used", i);
			}
			current_marker &= ~ (1 << i);
			if (current_marker == 0) break;
		}
	}
	
	/* render the result to the left window */
	if (composite)
	{
		GdkWindow *window;

		window = gtk_text_view_get_window (GTK_TEXT_VIEW (view),
						   GTK_TEXT_WINDOW_LEFT);

		gdk_draw_pixbuf (GDK_DRAWABLE (window), NULL, composite,
					 0, 0, x, y,
					 width, height,
				 	GDK_RGB_DITHER_NORMAL, 0, 0);
		g_object_unref (composite);
	}
}

static void
dma_sparse_view_paint_margin (DmaSparseView *view,
			      GdkEventExpose *event)
{
	GtkTextView *text_view;
	GdkWindow *win;
	PangoLayout *layout;
	gint y1, y2;
	gint y, height;
	gchar str [16];
	gint margin_width;
	gint margin_length;
	gint text_width;
	DmaSparseIter buf_iter;
	GtkTextIter text_iter;
	guint prev_address = G_MAXUINT;

	
	text_view = GTK_TEXT_VIEW (view);

	if (!view->priv->show_line_numbers && !view->priv->show_line_markers)
	{
		gtk_text_view_set_border_window_size (text_view,
						      GTK_TEXT_WINDOW_LEFT,
						      0);

		return;
	}
	
	win = gtk_text_view_get_window (text_view,
					GTK_TEXT_WINDOW_LEFT);	

	y1 = event->area.y;
	y2 = y1 + event->area.height;

	/* get the extents of the line printing */
	gtk_text_view_window_to_buffer_coords (text_view,
					       GTK_TEXT_WINDOW_LEFT,
					       0,
					       y1,
					       NULL,
					       &y1);

	gtk_text_view_window_to_buffer_coords (text_view,
					       GTK_TEXT_WINDOW_LEFT,
					       0,
					       y2,
					       NULL,
					       &y2);

	/* set size. */
	g_snprintf (str, sizeof (str), "0x%X", G_MAXUINT);
	margin_length = strlen(str) - 2;
	layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), str);

	pango_layout_get_pixel_size (layout, &text_width, NULL);
	
	pango_layout_set_width (layout, text_width);
	pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);

	/* determine the width of the left margin. */
	if (view->priv->show_line_numbers)
		margin_width = text_width + 4;
	else
		margin_width = 0;
	
	if (view->priv->show_line_markers)
		margin_width += GUTTER_PIXMAP;

	g_return_if_fail (margin_width != 0);
	
	gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text_view),
					      GTK_TEXT_WINDOW_LEFT,
					      margin_width);

	/* Display all addresses */
	dma_sparse_iter_copy (&buf_iter, &view->priv->start);
	gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer (text_view), &text_iter);
	

	
	/* Skip line while position doesn't need to be repaint */
	gtk_text_view_get_line_yrange (text_view, &text_iter, &y, &height);
	if (y < y1)
	{
		do
		{
			if (!dma_sparse_iter_forward_lines (&buf_iter, 1)) return;
			if (!gtk_text_iter_forward_line (&text_iter)) return;
			gtk_text_view_get_line_yrange (text_view, &text_iter, &y, &height);
		} while (y < y1);
	}
		

	/* Display address */
	do
	{
		gint pos;
		guint address;
		
		gtk_text_view_buffer_to_window_coords (text_view,
						       GTK_TEXT_WINDOW_LEFT,
						       0,
						       y,
						       NULL,
						       &pos);

		address = dma_sparse_iter_get_address (&buf_iter);
		
		if (view->priv->show_line_numbers) 
		{
			g_snprintf (str, sizeof (str),"0x%0*lX", margin_length, (long unsigned int)address);
			pango_layout_set_markup (layout, str, -1);

			gtk_paint_layout (GTK_WIDGET (view)->style,
					  win,
					  GTK_WIDGET_STATE (view),
					  FALSE,
					  NULL,
					  GTK_WIDGET (view),
					  NULL,
					  text_width + 2, 
					  pos,
					  layout);
		}

		/* Display marker */
		if ((prev_address != address) && (view->priv->show_line_markers)) 
		{
			gint current_marker = dma_sparse_buffer_get_marks (view->priv->buffer, address);

			if (current_marker)
			{
				gint x;
				
				if (view->priv->show_line_numbers)
					x = text_width + 4;
				else
					x = 0;
				
				draw_line_markers (view, current_marker, x, pos);
				prev_address = address;
			}
		}
		
		if (!dma_sparse_iter_forward_lines (&buf_iter, 1)) return;
		if (!gtk_text_iter_forward_line (&text_iter)) return;
		gtk_text_view_get_line_yrange (text_view, &text_iter, &y, &height);
		
	} while (y < y2);
		
	g_object_unref (G_OBJECT (layout));	
}

static void
dma_sparse_view_update_adjustement (DmaSparseView *view)
{
	PangoLayout *layout;
	GdkRectangle text_area;
	int height;

	gtk_text_view_get_visible_rect(GTK_TEXT_VIEW (view), &text_area);
	layout = gtk_widget_create_pango_layout (GTK_WIDGET(view), "0123456789ABCDEFGHIJKLMNOPQRSTUVWWYZ,");
	pango_layout_get_pixel_size(layout, NULL, &height);
	g_object_unref (G_OBJECT (layout));
	
	view->priv->line_by_page = text_area.height / height;
	view->priv->char_by_line = 8;	

	if (view->priv->vadjustment != NULL)
	{
		GtkAdjustment *vadj = view->priv->vadjustment;
		
		vadj->step_increment = view->priv->char_by_line;
		vadj->page_size = (view->priv->line_by_page - 1) * vadj->step_increment;
		vadj->page_increment = vadj->page_size * 0.9;
		gtk_adjustment_changed (vadj); 
	}
}

static void
dma_sparse_view_value_changed (GtkAdjustment *adj,
                             DmaSparseView   *view)
{
	dma_sparse_view_synchronize_iter (view, &view->priv->start);
	dma_sparse_view_refresh (view);
}

static void
dma_sparse_view_set_scroll_adjustments (GtkTextView *text_view,
                                      GtkAdjustment *hadj,
                                      GtkAdjustment *vadj)
{
	DmaSparseView *view = DMA_SPARSE_VIEW (text_view);

	if (vadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
	
	if (view->priv->vadjustment && (view->priv->vadjustment != vadj))
    {
		g_signal_handlers_disconnect_by_func (view->priv->vadjustment,
									dma_sparse_view_value_changed,
					    			view);
     	g_object_unref (view->priv->vadjustment);
	}
	
	if (view->priv->vadjustment != vadj)
	{
		
		GTK_TEXT_VIEW_CLASS (parent_class)->set_scroll_adjustments  (GTK_TEXT_VIEW (view), hadj, NULL);
		
		if (vadj != NULL)
		{
			g_object_ref_sink (vadj);
      
			g_signal_connect (vadj, "value_changed",
                        G_CALLBACK (dma_sparse_view_value_changed),
						view);
			
			vadj->upper = dma_sparse_buffer_get_upper (view->priv->buffer);
			vadj->lower = dma_sparse_buffer_get_lower (view->priv->buffer);
			vadj->value = 0;
		}
		view->priv->vadjustment = vadj;
		dma_sparse_view_update_adjustement (view);
	}
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
dma_sparse_view_set_sparse_buffer (DmaSparseView *view, DmaSparseBuffer *buffer)
{
	view->priv->buffer = buffer;
	dma_sparse_buffer_get_iterator_at_address (buffer, &view->priv->start, 0);
	dma_sparse_view_refresh (view);
}

DmaSparseBuffer * 
dma_sparse_view_get_sparse_buffer (DmaSparseView *view)
{
	return view->priv->buffer;
}

void
dma_sparse_view_refresh (DmaSparseView *view)
{
	gint offset;
	GtkTextIter cur;
	GtkTextMark *mark;
	GtkTextIter start, end;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	/* Save all cursor offset */
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
    offset = gtk_text_iter_get_offset (&cur);

	/* Remove old data */
	view->priv->stamp++;
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 0);
	
	/* Get data */
	dma_sparse_iter_insert_lines (&view->priv->start, &end, view->priv->line_by_page);

	/* Restore cursor */
    mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &cur, mark);
	gtk_text_iter_set_offset (&cur, offset);
	gtk_text_buffer_move_mark_by_name (buffer, "insert", &cur);
	gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &cur);
}

/* Markers functions
 *---------------------------------------------------------------------------*/

gint
dma_sparse_view_mark (DmaSparseView *view, guint location, gint marker)
{
	dma_sparse_buffer_add_mark (view->priv->buffer, location, marker_ianjuta_to_view (marker));
	gtk_widget_queue_draw (GTK_WIDGET (view));
	
	return (gint)location;
}

void
dma_sparse_view_unmark (DmaSparseView *view, guint location, gint marker)
{
	dma_sparse_buffer_remove_mark (view->priv->buffer, location, marker_ianjuta_to_view (marker));
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
dma_sparse_view_delete_all_markers (DmaSparseView *view, gint marker)
{
	dma_sparse_buffer_remove_all_mark (view->priv->buffer, marker_ianjuta_to_view(marker));
}

void
dma_sparse_view_goto (DmaSparseView *view, guint location)
{
	dma_sparse_iter_move_at (&view->priv->start, location);
	gtk_adjustment_set_value (view->priv->vadjustment, (gdouble)location);
	gtk_adjustment_value_changed (view->priv->vadjustment);
}

guint
dma_sparse_view_get_location (DmaSparseView *view)
{
	GtkTextMark *mark;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	DmaSparseIter buf_iter;
	gint line;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(view));
	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);
	line = gtk_text_iter_get_line (&iter);
		
	dma_sparse_iter_copy (&buf_iter, &view->priv->start);
	dma_sparse_iter_forward_lines (&buf_iter, line);
	
	return dma_sparse_iter_get_address (&buf_iter);
}

/* GtkWidget functions
 *---------------------------------------------------------------------------*/

static gint
dma_sparse_view_expose (GtkWidget      *widget,
			GdkEventExpose *event)
{
	DmaSparseView *view;
	GtkTextView *text_view;
	gboolean event_handled;
	
	view = DMA_SPARSE_VIEW (widget);
	text_view = GTK_TEXT_VIEW (widget);

	event_handled = FALSE;
	
	/* now check for the left window, which contains the margin */
	if (event->window == gtk_text_view_get_window (text_view,
						       GTK_TEXT_WINDOW_LEFT)) 
	{
		dma_sparse_view_paint_margin (view, event);
		event_handled = TRUE;
	} 
	else
	{
		event_handled = GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
	}
	
	return event_handled;
}

static void
dma_sparse_view_size_allocate (GtkWidget *widget,
                             GtkAllocation *allocation)
{
	DmaSparseView *view;

	view = DMA_SPARSE_VIEW (widget);
	
	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

	dma_sparse_view_update_adjustement (view);
	dma_sparse_view_refresh (view);
}

/* GtkObject functions
 *---------------------------------------------------------------------------*/

static void
dma_sparse_view_destroy (GtkObject *object)
{
	DmaSparseView *view;

	g_return_if_fail (DMA_IS_SPARSE_VIEW (object));

	view = DMA_SPARSE_VIEW (object);

	if (view->priv->goto_window)
	{
		gtk_widget_destroy (view->priv->goto_window);
		view->priv->goto_window = NULL;
		view->priv->goto_entry = NULL;
	}
	
	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

/* GObject functions
 *---------------------------------------------------------------------------*/

static void 
dma_sparse_view_set_property (GObject *object,guint prop_id, const GValue *value, GParamSpec *pspec)
{
	DmaSparseView *view;
	
	g_return_if_fail (DMA_IS_SPARSE_VIEW (object));

	view = DMA_SPARSE_VIEW (object);
    
	switch (prop_id)
	{
		case PROP_SHOW_LINE_NUMBERS:
			dma_sparse_view_set_show_line_numbers (view, g_value_get_boolean (value));
			break;
		case PROP_SHOW_LINE_MARKERS:
			dma_sparse_view_set_show_line_markers (view, g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void 
dma_sparse_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	DmaSparseView *view;
	
	g_return_if_fail (DMA_IS_SPARSE_VIEW (object));

	view = DMA_SPARSE_VIEW (object);
    
	switch (prop_id)
	{
		case PROP_SHOW_LINE_NUMBERS:
			g_value_set_boolean (value, dma_sparse_view_get_show_line_numbers (view));
			break;
		case PROP_SHOW_LINE_MARKERS:
			g_value_set_boolean (value, dma_sparse_view_get_show_line_markers (view));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_sparse_view_dispose (GObject *object)
{
	DmaSparseView *view;

	g_return_if_fail (object != NULL);
	g_return_if_fail (DMA_IS_SPARSE_VIEW (object));
	
	view = DMA_SPARSE_VIEW (object);
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_sparse_view_finalize (GObject *object)
{
	DmaSparseView *view;

	g_return_if_fail (object != NULL);
	g_return_if_fail (DMA_IS_SPARSE_VIEW (object));

	view = DMA_SPARSE_VIEW (object);
	
	dma_sparse_view_free_marker (view);

	g_free (view->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_sparse_view_instance_init (DmaSparseView *view)
{
	PangoFontDescription *font_desc;
	
	view->priv = g_new0 (DmaSparseViewPrivate, 1);	
	
	view->priv->buffer = NULL;

	view->priv->goto_window = NULL;
	view->priv->goto_entry = NULL;

	view->priv->show_line_numbers = TRUE;
	view->priv->show_line_markers = TRUE;
	
	view->priv->stamp = 0;

	memset (view->priv->marker_pixbuf, 0, sizeof (view->priv->marker_pixbuf));

	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 2);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 2);	
	
	g_signal_connect (view, "populate_popup",
                        G_CALLBACK (dma_sparse_view_populate_popup), view);
	
	gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (view),
								      GTK_TEXT_WINDOW_LEFT,
								      MIN_NUMBER_WINDOW_WIDTH);
	
	font_desc = pango_font_description_from_string ("Monospace 10");
	gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
	pango_font_description_free (font_desc);
	
	dma_sparse_view_initialize_marker (view);
}

/* class_init intialize the class itself not the instance */

static void
dma_sparse_view_class_init (DmaSparseViewClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GtkWidgetClass   *widget_class;
	GtkTextViewClass *text_view_class;

	g_return_if_fail (klass != NULL);
	
	gobject_class = (GObjectClass *) klass;
	object_class = (GtkObjectClass *) klass;
	widget_class = GTK_WIDGET_CLASS (klass);
	text_view_class = GTK_TEXT_VIEW_CLASS (klass);
	parent_class = (GtkTextViewClass*) g_type_class_peek_parent (klass);
	
	gobject_class->dispose = dma_sparse_view_dispose;
	gobject_class->finalize = dma_sparse_view_finalize;
	gobject_class->get_property = dma_sparse_view_get_property;
	gobject_class->set_property = dma_sparse_view_set_property;	

	object_class->destroy = dma_sparse_view_destroy;

	widget_class->size_allocate = dma_sparse_view_size_allocate;
	widget_class->expose_event = dma_sparse_view_expose;	
	
	text_view_class->move_cursor = dma_sparse_view_move_cursor;
	text_view_class->set_scroll_adjustments = dma_sparse_view_set_scroll_adjustments;
	
	g_object_class_install_property (gobject_class,
					 PROP_SHOW_LINE_NUMBERS,
					 g_param_spec_boolean ("show_line_numbers",
							       _("Show Line Numbers"),
							       _("Whether to display line numbers"),
							       FALSE,
							       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
					 PROP_SHOW_LINE_MARKERS,
					 g_param_spec_boolean ("show_line_markers",
							       _("Show Line Markers"),
							       _("Whether to display line marker pixbufs"),
							       FALSE,
							       G_PARAM_READWRITE));	
}

GType
dma_sparse_view_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaSparseViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_sparse_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaSparseView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) dma_sparse_view_instance_init,
			NULL            /* value_table */
		};

		type = g_type_register_static (GTK_TYPE_TEXT_VIEW,
		                            "DmaSparseView", &type_info, 0);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

GtkWidget*
dma_sparse_view_new_with_buffer (DmaSparseBuffer *buffer)
{
	GtkWidget *view;

	view = g_object_new (DMA_SPARSE_VIEW_TYPE, NULL);
	g_assert (view != NULL);
	
	DMA_SPARSE_VIEW(view)->priv->buffer = buffer;
	dma_sparse_buffer_get_iterator_at_address (buffer, &(DMA_SPARSE_VIEW (view))->priv->start, 0);

	return view;
}
	
void
dma_sparse_view_free (DmaSparseView *view)
{
	g_object_unref (view);
}


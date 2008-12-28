/*
 * anjuta-view.c
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2003-2005 Paolo Maggi
 * Copyright (C) 2006 Johannes Schmid
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301, USA. 
 */
 
/*
 * Modified by the anjuta Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$ 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <glib.h>

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>

#include "anjuta-view.h"
#include "sourceview.h"
#include "sourceview-private.h"
#include "anjuta-marshal.h"

#define ANJUTA_VIEW_SCROLL_MARGIN 0.02

#define ANJUTA_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ANJUTA_TYPE_VIEW, AnjutaViewPrivate))

enum
{
	ANJUTA_VIEW_POPUP = 1
};

struct _AnjutaViewPrivate
{
	GtkWidget* popup;
	guint scroll_idle;
  Sourceview* sv;
};

static void	anjuta_view_dispose		(GObject       *object);
static void	anjuta_view_finalize		(GObject         *object);
static void	anjuta_view_move_cursor		(GtkTextView     *text_view,
						 GtkMovementStep  step,
						 gint             count,
						 gboolean         extend_selection);
static gint     anjuta_view_focus_out		(GtkWidget       *widget,
						 GdkEventFocus   *event);

static gint	anjuta_view_expose	 	(GtkWidget       *widget,
						 GdkEventExpose  *event);
					
static gboolean	anjuta_view_key_press_event		(GtkWidget         *widget,
							 GdkEventKey       *event);
static gboolean	anjuta_view_button_press_event		(GtkWidget         *widget,
							 GdkEventButton       *event);

G_DEFINE_TYPE(AnjutaView, anjuta_view, GTK_TYPE_SOURCE_VIEW)

static gboolean
scroll_to_cursor_real (AnjutaView *view)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_val_if_fail (buffer != NULL, FALSE);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      0.25,
				      FALSE,
				      0.0,
				      0.0);
	
	view->priv->scroll_idle = 0;
	return FALSE;
}

static void
anjuta_view_set_property (GObject * object,
			   guint property_id,
			   const GValue * value, GParamSpec * pspec)
{
	AnjutaView *self = ANJUTA_VIEW (object);
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case ANJUTA_VIEW_POPUP:
		{
			GtkWidget* widget;
			self->priv->popup = g_value_get_object (value);
			widget = gtk_menu_get_attach_widget(GTK_MENU(self->priv->popup));
			/* This is a very ugly hack, maybe somebody more familiar with gtk menus
			can fix this */
			if (widget != NULL)
				gtk_menu_detach(GTK_MENU(self->priv->popup));
			gtk_menu_attach_to_widget(GTK_MENU(self->priv->popup), GTK_WIDGET(self), NULL);
			break;
		}
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
		}
	}
}

static void
anjuta_view_get_property (GObject * object,
			   guint property_id,
			   GValue * value, GParamSpec * pspec)
{
	AnjutaView *self = ANJUTA_VIEW (object);
	
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case ANJUTA_VIEW_POPUP:
		{
			g_value_set_object (value, self->priv->popup);
			break;
		}
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
		}
	}
}

static void
anjuta_view_class_init (AnjutaViewClass *klass)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass   *gtkobject_class = GTK_OBJECT_CLASS (klass);
	GtkTextViewClass *textview_class = GTK_TEXT_VIEW_CLASS (klass);
	GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
	GtkBindingSet    *binding_set;
	GParamSpec *anjuta_view_spec_popup;

	object_class->dispose = anjuta_view_dispose;
	object_class->finalize = anjuta_view_finalize;
	object_class->set_property = anjuta_view_set_property;
	object_class->get_property = anjuta_view_get_property;	

	widget_class->focus_out_event = anjuta_view_focus_out;
	widget_class->expose_event = anjuta_view_expose;
	widget_class->key_press_event = anjuta_view_key_press_event;
	widget_class->button_press_event = anjuta_view_button_press_event;

	textview_class->move_cursor = anjuta_view_move_cursor;

	g_type_class_add_private (klass, sizeof (AnjutaViewPrivate));
	
	anjuta_view_spec_popup = g_param_spec_object ("popup",
						       "Popup menu",
						       "The popup-menu to show",
						       GTK_TYPE_WIDGET,
						       G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 ANJUTA_VIEW_POPUP,
					 anjuta_view_spec_popup);
	
	binding_set = gtk_binding_set_by_class (klass);	
}

static void
move_cursor (GtkTextView       *text_view,
	     const GtkTextIter *new_location,
	     gboolean           extend_selection)
{
	GtkTextBuffer *buffer = text_view->buffer;

	if (extend_selection)
		gtk_text_buffer_move_mark_by_name (buffer,
						   "insert",
						   new_location);
	else
		gtk_text_buffer_place_cursor (buffer, new_location);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

static void
anjuta_view_move_cursor (GtkTextView    *text_view,
			GtkMovementStep step,
			gint            count,
			gboolean        extend_selection)
{
	GtkTextBuffer *buffer = text_view->buffer;
	GtkTextMark *mark;
	GtkTextIter cur, iter;

	/* really make sure gtksourceview's home/end is disabled */
	g_return_if_fail (!gtk_source_view_get_smart_home_end (
						GTK_SOURCE_VIEW (text_view)));

	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	iter = cur;

	if (step == GTK_MOVEMENT_DISPLAY_LINE_ENDS &&
	    (count == -1) && gtk_text_iter_starts_line (&iter))
	{
		/* Find the iter of the first character on the line. */
		while (!gtk_text_iter_ends_line (&cur))
		{
			gunichar c;

			c = gtk_text_iter_get_char (&cur);
			if (g_unichar_isspace (c))
				gtk_text_iter_forward_char (&cur);
			else
				break;
		}

		/* if we are clearing selection, we need to move_cursor even
		 * if we are at proper iter because selection_bound may need
		 * to be moved */
		if (!gtk_text_iter_equal (&cur, &iter) || !extend_selection)
			move_cursor (text_view, &cur, extend_selection);
	}
	else if (step == GTK_MOVEMENT_DISPLAY_LINE_ENDS &&
	         (count == 1) && gtk_text_iter_ends_line (&iter))
	{
		/* Find the iter of the last character on the line. */
		while (!gtk_text_iter_starts_line (&cur))
		{
			gunichar c;

			gtk_text_iter_backward_char (&cur);
			c = gtk_text_iter_get_char (&cur);
			if (!g_unichar_isspace (c))
			{
				/* We've gone one character too far. */
				gtk_text_iter_forward_char (&cur);
				break;
			}
		}
		
		/* if we are clearing selection, we need to move_cursor even
		 * if we are at proper iter because selection_bound may need
		 * to be moved */
		if (!gtk_text_iter_equal (&cur, &iter) || !extend_selection)
			move_cursor (text_view, &cur, extend_selection);
	}
	else
	{
		/* note that we chain up to GtkTextView skipping GtkSourceView */
		(* GTK_TEXT_VIEW_CLASS (anjuta_view_parent_class)->move_cursor) (text_view,
										step,
										count,
										extend_selection);
	}
}

static void 
anjuta_view_init (AnjutaView *view)
{	
	view->priv = ANJUTA_VIEW_GET_PRIVATE (view);

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
		
	g_object_set (G_OBJECT (view), 
		      "wrap-mode", FALSE,
		      "show-line-numbers", TRUE,
		      "auto-indent", TRUE,
		      "tab-width", 4,
		      "insert-spaces-instead-of-tabs", FALSE,
		      "highlight-current-line", TRUE, 
          "indent-on-tab", TRUE, /* Fix #388727 */
		      "smart-home-end", FALSE, /* Never changes this */
		      NULL);
}

static void
anjuta_view_dispose (GObject *object)
{
	AnjutaView *view;

	view = ANJUTA_VIEW (object);

	g_source_remove (view->priv->scroll_idle);
		
	(* G_OBJECT_CLASS (anjuta_view_parent_class)->dispose) (object);
}

static void
anjuta_view_finalize (GObject *object)
{
	AnjutaView *view;

	view = ANJUTA_VIEW (object);

	if (view->priv->popup != NULL) 
	{
	    GtkWidget* widget = gtk_menu_get_attach_widget (GTK_MENU (view->priv->popup));
	    if (widget != NULL)
		gtk_menu_detach (GTK_MENU (view->priv->popup));
	}
		
	(* G_OBJECT_CLASS (anjuta_view_parent_class)->finalize) (object);
}

static gint
anjuta_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	AnjutaView *view = ANJUTA_VIEW (widget);
	AssistWindow* assist_win = view->priv->sv->priv->assist_win;
	AssistTip* assist_tip = view->priv->sv->priv->assist_tip;
	
	if (assist_win)
		gtk_widget_destroy(GTK_WIDGET(assist_win));
	
	if (assist_tip)
		gtk_widget_destroy(GTK_WIDGET(assist_tip));
	
	gtk_widget_queue_draw (widget);
	
	
	(* GTK_WIDGET_CLASS (anjuta_view_parent_class)->focus_out_event) (widget, event);
	
	return FALSE;
}


/**
 * anjuta_view_new:
 * @sv: a #Sourceview
 * 
 * Creates a new #AnjutaView object displaying the @sv->priv->doc document. 
 * @doc cannot be NULL.
 *
 * Return value: a new #AnjutaView
 **/
GtkWidget *
anjuta_view_new (Sourceview *sv)
{
	GtkWidget *view;

	view = GTK_WIDGET (g_object_new (ANJUTA_TYPE_VIEW, NULL));
	
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view),
				  GTK_TEXT_BUFFER (sv->priv->document));

	gtk_widget_show_all (view);

  ANJUTA_VIEW(view)->priv->sv = sv;
  
	return view;
}

void
anjuta_view_cut_clipboard (AnjutaView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	if (!gtk_text_view_get_editable (GTK_TEXT_VIEW(view)))
		return;
	
	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_cut_clipboard (buffer,
  				       clipboard,
				       TRUE);
  	
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
anjuta_view_copy_clipboard (AnjutaView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

  	gtk_text_buffer_copy_clipboard (buffer, clipboard);

	/* on copy do not scroll, we are already on screen */
}

void
anjuta_view_paste_clipboard (AnjutaView *view)
{
  	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_paste_clipboard (buffer,
					 clipboard,
					 NULL,
					 TRUE);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
anjuta_view_delete_selection (AnjutaView *view)
{
  	GtkTextBuffer *buffer = NULL;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer,
					  TRUE,
					  TRUE);
						
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
anjuta_view_select_all (AnjutaView *view)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_select_range (buffer, &start, &end);
}

void
anjuta_view_scroll_to_cursor (AnjutaView *view)
{
	g_return_if_fail (ANJUTA_IS_VIEW (view));
	
	view->priv->scroll_idle = g_idle_add ((GSourceFunc) scroll_to_cursor_real, view);
}

void
anjuta_view_set_font (AnjutaView   *view, 
		     gboolean     def, 
		     const gchar *font_name)
{

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	if (!def)
	{
		PangoFontDescription *font_desc = NULL;

		g_return_if_fail (font_name != NULL);
		
		font_desc = pango_font_description_from_string (font_name);
		g_return_if_fail (font_desc != NULL);

		gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
		
		pango_font_description_free (font_desc);		
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		if (rc_style->font_desc)
			pango_font_description_free (rc_style->font_desc);

		rc_style->font_desc = NULL;
		
		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);
	}
}

static gint
anjuta_view_expose (GtkWidget      *widget,
                   GdkEventExpose *event)
{
	GtkTextView *text_view;
	GtkTextBuffer *doc;
	
	text_view = GTK_TEXT_VIEW (widget);
	
	doc = gtk_text_view_get_buffer (text_view);
	
	if ((event->window == gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT)))
	{
		GdkRectangle visible_rect;
		GtkTextIter iter1, iter2;
		
		gtk_text_view_get_visible_rect (text_view, &visible_rect);
		gtk_text_view_get_line_at_y (text_view, &iter1,
					     visible_rect.y, NULL);
		gtk_text_view_get_line_at_y (text_view, &iter2,
					     visible_rect.y
					     + visible_rect.height, NULL);
		gtk_text_iter_forward_line (&iter2);
	}

	return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->expose_event)(widget, event);
}

static gboolean
anjuta_view_key_press_event		(GtkWidget *widget, GdkEventKey       *event)
{
	GtkTextBuffer *buffer;
	AnjutaView* view = ANJUTA_VIEW(widget);
	AssistWindow* assist_win;
	AssistTip* assist_tip;
	
	buffer  = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	
	assist_win = view->priv->sv->priv->assist_win;
	if (assist_win)
	{
		if (assist_window_filter_keypress(assist_win, event->keyval))
		{
			DEBUG_PRINT("key filtered: %d", event->keyval);
			return TRUE;
		}
	}
	assist_tip = view->priv->sv->priv->assist_tip;
	if (assist_tip)
	{
    switch (event->keyval)
    {
      case GDK_Escape:
      case GDK_Up:
      case GDK_Down:
      case GDK_Page_Up:
      case GDK_Page_Down:
        gtk_widget_destroy (GTK_WIDGET(assist_tip));
        break;
		}
	}
	return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
}

static gboolean	
anjuta_view_button_press_event	(GtkWidget *widget, GdkEventButton *event)
{
	AnjutaView* view = ANJUTA_VIEW(widget);
	
  /* If we have a calltip shown - hide it */
  AssistTip* assist_tip = view->priv->sv->priv->assist_tip;
  AssistWindow* assist_win = view->priv->sv->priv->assist_win;
  if (assist_win)
  {
    gtk_widget_destroy (GTK_WIDGET (assist_win));
  }
	if (assist_tip)
	{
    gtk_widget_destroy (GTK_WIDGET (assist_tip));
  }
  
	switch(event->button)
	{
		case 3: /* Right Button */
		{
			GtkTextBuffer* buffer = GTK_TEXT_BUFFER (view->priv->sv->priv->document);
			if (!gtk_text_buffer_get_has_selection (buffer))
			{
				/* Move cursor to set breakpoints at correct line (#530689) */
				GtkTextIter iter;
				gint buffer_x, buffer_y;
				GtkTextWindowType type =  gtk_text_view_get_window_type (GTK_TEXT_VIEW (view),
																																 event->window);
				gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (view),
																							 type,
																							 event->x,
																							 event->y,
																							 &buffer_x,
																							 &buffer_y);
				gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (view),
																						&iter, buffer_x, buffer_y);
				gtk_text_buffer_place_cursor (buffer, &iter);
			}												 
			gtk_menu_popup (GTK_MENU (view->priv->popup), NULL, NULL, NULL, NULL, 
                  event->button, event->time);
			return TRUE;
		}
		default:
			return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->button_press_event)(widget, event);
	}
}

/*
    chunk_view.c
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

/*
 * It is a GtkTextView with an associated GtkAjustment, allowing to put in the
 * GtkTextView only a part of a big amount of text. Cursor movements are not
 * limited by the GtkTextView content but by the GtkAjustment.
 *---------------------------------------------------------------------------*/
 
#include "chunk_view.h"

struct _DmaChunkView
{
	GtkTextView parent;
	
	GtkAdjustment *vadjustment;
};

struct _DmaChunkViewClass
{
	GtkTextViewClass parent_class;
};

/* Used in dispose and finalize */
static GtkWidgetClass *parent_class = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
set_adjustment_clamped (GtkAdjustment *adj, gdouble value)
{
	if (value < adj->lower)
	{
		value = adj->lower;
	}
	if (value > (adj->upper - adj->page_size))
	{
		value = adj->upper - adj->page_size;
	}
	gtk_adjustment_set_value (adj, value);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
dma_chunk_view_move_cursor (GtkTextView *text_view,
			     GtkMovementStep step,
			     gint            count,
			     gboolean        extend_selection)
{
	DmaChunkView *view = DMA_CHUNK_VIEW (text_view);
	gdouble value = view->vadjustment->value;
	GtkTextMark *mark;
	GtkTextBuffer *buffer;
	GtkTextIter cur;
	gint line;
	
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
		buffer = gtk_text_view_get_buffer (text_view);
	    mark = gtk_text_buffer_get_insert (buffer);
	    gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	    line = gtk_text_iter_get_line (&cur);
	
		if ((count < 0) && (line == 0))
		{
			value += count * view->vadjustment->step_increment;
			set_adjustment_clamped (view->vadjustment, value);
			return;
		}
		else if ((count > 0) && (line == gtk_text_buffer_get_line_count(buffer) - 1))
		{
			value += count * view->vadjustment->step_increment;
			set_adjustment_clamped (view->vadjustment, value);
			return;
		}
		break;
	case GTK_MOVEMENT_PAGES:
		value += count * view->vadjustment->page_increment;
		set_adjustment_clamped (view->vadjustment, value);
		return;
	case GTK_MOVEMENT_BUFFER_ENDS:
		set_adjustment_clamped (view->vadjustment, count < 0 ? view->vadjustment->lower : view->vadjustment->upper);
		return;
    default:
        break;
    }
	
	GTK_TEXT_VIEW_CLASS (parent_class)->move_cursor (text_view,
								 step, count,
								 extend_selection);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void dma_chunk_view_set_scroll_adjustment (DmaChunkView *this, GtkAdjustment* vadjustment)
{
	this->vadjustment = vadjustment;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_chunk_view_dispose (GObject *object)
{
	/*DmaChunkView *this = DMA_CHUNK_VIEW (obj);*/
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_chunk_view_finalize (GObject *object)
{
	/*DmaChunkView *view = DMA_CHUNK_VIEW (object);*/
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_chunk_view_instance_init (DmaChunkView *view)
{
	// GtkAdjustment *adj;
	// GtkWidget* wid;
	// PangoFontDescription *font_desc;

}

/* class_init intialize the class itself not the instance */

static void
dma_chunk_view_class_init (DmaChunkViewClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GtkWidgetClass   *widget_class;
	GtkTextViewClass *text_view_class;

	g_return_if_fail (klass != NULL);
	
	gobject_class = G_OBJECT_CLASS (klass);
	object_class = GTK_OBJECT_CLASS (klass);
	widget_class = GTK_WIDGET_CLASS (klass);
	text_view_class = GTK_TEXT_VIEW_CLASS (klass);
	parent_class = GTK_WIDGET_CLASS (g_type_class_peek_parent (klass));
	
	gobject_class->dispose = dma_chunk_view_dispose;
	gobject_class->finalize = dma_chunk_view_finalize;
	
	text_view_class->move_cursor = dma_chunk_view_move_cursor;
}

GType
dma_chunk_view_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaChunkViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_chunk_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaChunkView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) dma_chunk_view_instance_init,
			NULL            /* value_table */
		};

		type = g_type_register_static (GTK_TYPE_TEXT_VIEW,
		                            "DmaChunkView", &type_info, 0);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

GtkWidget*
dma_chunk_view_new (void)
{
	GtkWidget *this;

	this = g_object_new (DMA_CHUNK_VIEW_TYPE, NULL);
	g_assert (this != NULL);
	
	return this;
}
	
void
dma_chunk_view_free (DmaChunkView *this)
{
	g_object_unref (this);
}


/*
    chunk_view.h
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

#ifndef _CHUNK_VIEW_H
#define _CHUNK_VIEW_H

#include <gtk/gtk.h>

#define DMA_CHUNK_VIEW_TYPE              (dma_chunk_view_get_type ())
#define DMA_CHUNK_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_CHUNK_VIEW_TYPE, DmaChunkView))
#define DMA_CHUNK_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_CHUNK_VIEW_TYPE, DmaChunkViewClass))
#define IS_DMA_CHUNK_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_CHUNK_VIEW_TYPE))
#define IS_DMA_CHUNK_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_CHUNK_VIEW_TYPE))
#define GET_DMA_CHUNK_VIEW_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_CHUNK_VIEW_TYPE, DmaChunkViewClass))

typedef struct _DmaChunkView DmaChunkView;
typedef struct _DmaChunkViewClass DmaChunkViewClass;

GType dma_chunk_view_get_type (void);

GtkWidget *dma_chunk_view_new (void);
void dma_chunk_view_free (DmaChunkView *this);

void dma_chunk_view_set_scroll_adjustment (DmaChunkView *this, GtkAdjustment* hadjustment);

#endif /* _CHUNK_VIEW_H */

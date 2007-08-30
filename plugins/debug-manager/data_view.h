/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    data_view.h
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

#ifndef _DATA_VIEW_H
#define _DATA_VIEW_H

#include "data_buffer.h"

#include <gtk/gtk.h>

#define DMA_DATA_VIEW_TYPE              (dma_data_view_get_type ())
#define DMA_DATA_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_DATA_VIEW_TYPE, DmaDataView))
#define DMA_DATA_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_DATA_VIEW_TYPE, DmaDataViewClass))
#define IS_DMA_DATA_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_DATA_VIEW_TYPE))
#define IS_DMA_DATA_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_DATA_VIEW_TYPE))
#define GET_DMA_DATA_VIEW_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_DATA_VIEW_TYPE, DmaDataViewClass))

typedef struct _DmaDataView DmaDataView;
typedef struct _DmaDataViewClass DmaDataViewClass;

GType dma_data_view_get_type (void);

GtkWidget *dma_data_view_new_with_buffer (DmaDataBuffer *buffer);
void dma_data_view_free (DmaDataView *this);

void dma_data_view_goto_address (DmaDataView *view, const void *address);
void dma_data_view_refresh (DmaDataView *view);

GtkWidget *dma_data_view_get_address (DmaDataView *view);
GtkWidget *dma_data_view_get_data (DmaDataView *view);
GtkWidget *dma_data_view_get_ascii (DmaDataView *view);

#endif /* _DATA_VIEW_H */

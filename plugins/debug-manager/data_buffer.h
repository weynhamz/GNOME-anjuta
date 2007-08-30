/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    data_buffer.h
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

#ifndef _DATA_BUFFER_H
#define _DATA_BUFFER_H

#include <gtk/gtk.h>

#define DMA_DATA_BUFFER_TYPE              (dma_data_buffer_get_type ())
#define DMA_DATA_BUFFER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_DATA_BUFFER_TYPE, DmaDataBuffer))
#define DMA_DATA_BUFFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_DATA_BUFFER_TYPE, DmaDataBufferClass))
#define IS_DMA_DATA_BUFFER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_DATA_BUFFER_TYPE))
#define IS_DMA_DATA_BUFFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_DATA_BUFFER_TYPE))
#define GET_DMA_DATA_BUFFER_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_DATA_BUFFER_TYPE, DmaDataBufferClass))

typedef struct _DmaDataBuffer DmaDataBuffer;
typedef struct _DmaDataBufferClass DmaDataBufferClass;

GType dma_data_buffer_get_type (void);

typedef enum
{
	DMA_OCTAL_BASE = 0,
	DMA_DECIMAL_BASE,
	DMA_HEXADECIMAL_BASE,
	DMA_ASCII_BASE,
	DMA_DATA_BASE = 0xFF,
	DMA_BYTE_DATA = 0x100,
	DMA_WORD_DATA,
	DMA_DWORD_DATA,
	DMA_QWORD_DATA,
	DMA_DATA_SIZE = 0xFF00
} DmaFormat;

typedef void (*DmaDataBufferReadFunc) (gulong address, gulong length, gpointer user_data);
typedef void (*DmaDataBufferWriteFunc) (gulong address, gulong length, const gchar *data, gpointer user_data);

DmaDataBuffer *dma_data_buffer_new (gulong lower, gulong upper, DmaDataBufferReadFunc read, DmaDataBufferWriteFunc write, gpointer user_data);
void dma_data_buffer_free (DmaDataBuffer *buffer);

void dma_data_buffer_remove_all_page (DmaDataBuffer *buffer);

gulong dma_data_buffer_get_lower (const DmaDataBuffer *buffer);
gulong dma_data_buffer_get_upper (const DmaDataBuffer *buffer);

gchar *dma_data_buffer_get_address (DmaDataBuffer *buffer, gulong lower, guint length, guint step, guint size);
gchar *dma_data_buffer_get_data (DmaDataBuffer *buffer, gulong lower, guint length, guint step, gint base);

void dma_data_buffer_invalidate (DmaDataBuffer *buffer);
void dma_data_buffer_set_data (DmaDataBuffer *buffer, gulong address, gulong length, const gchar *data);

#endif /* _DATA_BUFFER_H */

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    sparse_buffer.h
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

#ifndef _SPARSE_BUFFER_H
#define _SPARSE_BUFFER_H

#include <gtk/gtk.h>

#define DMA_SPARSE_BUFFER_TYPE              (dma_sparse_buffer_get_type ())
#define DMA_SPARSE_BUFFER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_SPARSE_BUFFER_TYPE, DmaSparseBuffer))
#define DMA_SPARSE_BUFFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_SPARSE_BUFFER_TYPE, DmaSparseBufferClass))
#define DMA_IS_SPARSE_BUFFER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_SPARSE_BUFFER_TYPE))
#define DMA_IS_SPARSE_BUFFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_SPARSE_BUFFER_TYPE))
#define DMA_GET_SPARSE_BUFFER_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_SPARSE_BUFFER_TYPE, DmaSparseBufferClass))

typedef struct _DmaSparseBuffer DmaSparseBuffer;
typedef struct _DmaSparseBufferClass DmaSparseBufferClass;

typedef struct _DmaSparseBufferNode DmaSparseBufferNode;
typedef struct _DmaSparseIter DmaSparseIter;
typedef struct _DmaSparseBufferTransport DmaSparseBufferTransport;

GType dma_sparse_buffer_get_type (void);

DmaSparseBuffer *dma_sparse_buffer_new (guint lower, guint upper);
void dma_sparse_buffer_free (DmaSparseBuffer *buffer);

void dma_sparse_buffer_insert (DmaSparseBuffer *buffer, DmaSparseBufferNode *node);
void dma_sparse_buffer_remove (DmaSparseBuffer *buffer, DmaSparseBufferNode *node);
void dma_sparse_buffer_remove_all (DmaSparseBuffer *buffer);
DmaSparseBufferNode *dma_sparse_buffer_lookup (DmaSparseBuffer *self, guint address);
DmaSparseBufferNode *dma_sparse_buffer_first (DmaSparseBuffer *self);

guint dma_sparse_buffer_get_lower (const DmaSparseBuffer *buffer);
guint dma_sparse_buffer_get_upper (const DmaSparseBuffer *buffer);

void dma_sparse_buffer_changed (const DmaSparseBuffer *buffer);

void dma_sparse_buffer_add_mark (DmaSparseBuffer *buffer, guint address, gint mark);
void dma_sparse_buffer_remove_mark (DmaSparseBuffer *buffer, guint address, gint mark);
void dma_sparse_buffer_remove_all_mark (DmaSparseBuffer *buffer, gint mark);
gint dma_sparse_buffer_get_marks (DmaSparseBuffer *buffer, guint address);

void dma_sparse_buffer_get_iterator_at_address (DmaSparseBuffer *buffer, DmaSparseIter *iter, guint address);
void dma_sparse_buffer_get_iterator_near_address (DmaSparseBuffer *buffer, DmaSparseIter *iter, guint address);
void dma_sparse_iter_copy (DmaSparseIter *dst, const DmaSparseIter *src);
void dma_sparse_iter_move_at (DmaSparseIter *iter, guint address);
void dma_sparse_iter_move_near (DmaSparseIter *iter, guint address);
void dma_sparse_iter_refresh (DmaSparseIter *iter);
void dma_sparse_iter_round (DmaSparseIter *iter, gboolean round_up);
void dma_sparse_iter_insert_lines (DmaSparseIter *iter, GtkTextIter *dst, guint count);
gboolean dma_sparse_iter_forward_lines (DmaSparseIter *iter, gint count);
gulong dma_sparse_iter_get_address (DmaSparseIter *iter);

DmaSparseBufferTransport* dma_sparse_buffer_alloc_transport (DmaSparseBuffer *buffer, guint lines, guint chars);
void dma_sparse_buffer_free_transport (DmaSparseBufferTransport *trans);

struct _DmaSparseBuffer
{
	GObject parent;
	
	guint lower;
	guint upper;

	struct
	{
		DmaSparseBufferNode *head;
		DmaSparseBufferNode *tail;
	} cache;
	DmaSparseBufferNode *head;
	
	gint stamp;
	DmaSparseBufferTransport *pending;
	
	GHashTable* mark;
};

struct _DmaSparseBufferClass
{
	GObjectClass parent;
	
	void (*changed) (const DmaSparseBuffer *buffer);	
	
	void (*insert_line) (DmaSparseIter *iter, GtkTextIter *dst);
	gboolean (*refresh_iter) (DmaSparseIter *iter);
	void (*round_iter) (DmaSparseIter *iter, gboolean round_up);
	gboolean (*forward_line) (DmaSparseIter *iter);
	gboolean (*backward_line) (DmaSparseIter *iter);
	gulong (*get_address) (DmaSparseIter *iter);
};

struct _DmaSparseBufferNode
{
	struct
	{
		DmaSparseBufferNode *prev;
		DmaSparseBufferNode *next;
	} cache;
	DmaSparseBufferNode *prev;
	DmaSparseBufferNode *next;
	
	guint lower;		/* Lowest address of block */
	guint upper;		/* Highest address in the block (avoid overflow) */
};

struct _DmaSparseIter
{
	DmaSparseBuffer *buffer;
	gint stamp;
	DmaSparseBufferNode *node;
	gulong base;
	glong offset;
	gint line;
};

struct _DmaSparseBufferTransport
{
	DmaSparseBuffer *buffer;
	gulong start;
	gulong length;
	guint lines;
	guint chars;
	guint stamp;
	gint tag;
	DmaSparseBufferTransport *next;
};

#endif /* _SPARSE_BUFFER_H */

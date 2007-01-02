/*
    sparse_view.h
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

#ifndef _SPARSE_VIEW_H
#define _SPARSE_VIEW_H

#include "sparse_buffer.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DMA_SPARSE_VIEW_TYPE              (dma_sparse_view_get_type ())
#define DMA_SPARSE_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_SPARSE_VIEW_TYPE, DmaSparseView))
#define DMA_SPARSE_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_SPARSE_VIEW_TYPE, DmaSparseViewClass))
#define DMA_IS_SPARSE_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_SPARSE_VIEW_TYPE))
#define DMA_IS_SPARSE_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_SPARSE_VIEW_TYPE))
#define DMA_GET_SPARSE_VIEW_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_SPARSE_VIEW_TYPE, DmaSparseViewClass))

typedef struct _DmaSparseView DmaSparseView;
typedef struct _DmaSparseViewClass DmaSparseViewClass;

typedef struct _DmaSparseViewPrivate DmaSparseViewPrivate;

struct _DmaSparseView
{
	GtkTextView parent;

	DmaSparseViewPrivate *priv;
};

struct _DmaSparseViewClass
{
	GtkTextViewClass parent_class;
	
	void (*insert_lines) (DmaSparseView *view, DmaSparseIter *src, GtkTextIter *dst, guint count);
	gboolean (*forward_lines) (DmaSparseView *view, DmaSparseIter *iter, gint count);
};

GType dma_sparse_view_get_type (void);

GtkWidget *dma_sparse_view_new_with_buffer (DmaSparseBuffer *buffer);
void dma_sparse_view_free (DmaSparseView *view);

void dma_sparse_view_set_show_line_numbers (DmaSparseView *view, gboolean show);
gboolean dma_sparse_view_get_show_line_numbers (DmaSparseView *view);
void dma_sparse_view_set_show_line_markers (DmaSparseView *view, gboolean show);
gboolean dma_sparse_view_get_show_line_markers (DmaSparseView *view);

void dma_sparse_view_refresh (DmaSparseView *view);

DmaSparseBuffer * dma_sparse_view_get_sparse_buffer (DmaSparseView *view);
void dma_sparse_view_set_sparse_buffer (DmaSparseView *view, DmaSparseBuffer *buffer);

G_END_DECLS
#endif /* _SPARSE_VIEW_H */

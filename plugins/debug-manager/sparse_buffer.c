/*
    sparse_buffer.c
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

#include "sparse_buffer.h"

#include "anjuta-marshal.h"

#include <string.h>

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>

/*
 * This object works with the DmaSparseView which is a text view window allowing
 * to display very big amount of data. Only the part of the data currently 
 * displayed is kept in memory.
 *
 * This object doesn't replace the GtkTextBuffer in the corresponding
 * DmaSparseView. The GtkTextBuffer of each DmaSparseView contains only the data
 * displayed by the view. So several view displaying the same DmaSparseBuffer 
 * will probably have GtkTextBuffer containing different data.
 *
 * The DmaSparseBuffer object contains the data necessary for each view and
 * try to maintain a cache of recently used data. The data should be split in
 * blocks having a address and a size. The buffer does not care about the 
 * exact content of each block.
 *
 * The DmaSparseBuffer does not have any graphical knowledge, like which data
 * correspond to one line in the GtkTextBuffer.
 *---------------------------------------------------------------------------*/

enum
{
	DMA_SPARSE_BUFFER_NODE_SIZE = 512,
	DMA_SPARSE_BUFFER_MAX_PAGE = 60,
};

static GObjectClass *parent_class = NULL;

enum
{
	CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

/* Helper functions
 *---------------------------------------------------------------------------*/

/* DmaBufferNode functions
 *---------------------------------------------------------------------------*/

/* Transport functions
 *---------------------------------------------------------------------------*/

DmaSparseBufferTransport* 
dma_sparse_buffer_alloc_transport (DmaSparseBuffer *buffer, guint lines, guint chars)
{
	DmaSparseBufferTransport *trans;
	
	trans = g_slice_new0 (DmaSparseBufferTransport);

	trans->buffer = buffer;
	trans->lines = lines;
	trans->chars = chars;
	trans->next = buffer->pending;
	buffer->pending = trans;
	
	return trans;
}

void
dma_sparse_buffer_free_transport (DmaSparseBufferTransport *trans)
{
	DmaSparseBufferTransport **prev;
	
	g_return_if_fail (trans != NULL);
	
	for (prev = &trans->buffer->pending; *prev != trans; prev = &(*prev)->next)
	{
		if (*prev == NULL)
		{
			g_warning ("transport structure is missing");
			return;
		}
	}
	
	/* Remove transport structure and free it */
	*prev = trans->next;
	
	g_slice_free (DmaSparseBufferTransport, trans);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static DmaSparseBufferNode*
dma_sparse_buffer_find (DmaSparseBuffer *buffer, guint address)
{
	DmaSparseBufferNode *node = NULL;
	DmaSparseBufferNode *next;
	
	/* Look in last node */
	if (buffer->cache.head != NULL)
	{
		gint gap = buffer->cache.head->lower - address + DMA_SPARSE_BUFFER_NODE_SIZE * 4;
		
		if (gap < DMA_SPARSE_BUFFER_NODE_SIZE * 9)
		{
			/* node should be quite near */
			node = buffer->cache.head;
		}
		else
		{
			node = buffer->head;
		}
	}
	else
	{
		node = buffer->head;
	}

	for (;;)
	{
		if (node == NULL) break;
		
		if (node->lower > address)
		{
			/* Search backward */
			node = node->prev;
		}
		else if (node->upper < address)
		{
			/* Search forward */
			next = node->next;
			
			if ((next == NULL) || (next->lower > address))
			{
				/* Corresponding node doesn't exist */
				break;
			}
			node = next;
		}
		else
		{
			/* Find current node */
			break;
		}
	}
	
	return node;
}

static void
on_dma_sparse_buffer_changed (const DmaSparseBuffer *buffer)
{
}

/* Public functions
 *---------------------------------------------------------------------------*/

DmaSparseBufferNode*
dma_sparse_buffer_lookup (DmaSparseBuffer *buffer, guint address)
{
	return dma_sparse_buffer_find (buffer, address);
}

DmaSparseBufferNode*
dma_sparse_buffer_first (DmaSparseBuffer *buffer)
{
	return buffer->head;
}

void
dma_sparse_buffer_insert (DmaSparseBuffer *buffer, DmaSparseBufferNode *node)
{
	/* New node should have been allocated by caller with g_new */
	DmaSparseBufferNode *prev;

	DEBUG_PRINT ("insert block %x %x", node->lower, node->upper);
	/* Look for previous node */
	prev = dma_sparse_buffer_find (buffer, node->lower);
	while ((prev != NULL) && (node->lower <= prev->upper))
	{
		DmaSparseBufferNode *tmp;

		DEBUG_PRINT ("remove previous block %x %x", prev->lower, prev->upper);
		/* node overlap, remove it */
		tmp = prev->prev;
		dma_sparse_buffer_remove (buffer, prev);
		prev = tmp;
	}

	/* Insert node just after prev */
	if (prev == NULL)
	{
		/* Insert at the beginning */
		node->prev = NULL;
		node->next = buffer->head;
		buffer->head = node;
	}
	else
	{
		node->prev = prev;
		node->next = prev->next;
		prev->next = node;
		if (node->next != NULL)
		{
			node->next->prev = node;
		}
	}

	/* Check if new node overlap next one */
	while ((node->next != NULL) && (node->upper >= node->next->lower))
	{
		DEBUG_PRINT ("remove next block %x %x", node->next->lower, node->next->upper);
		/* node overlap, remove it */
		dma_sparse_buffer_remove (buffer, node->next);
	}
		
	/* Insert node at the beginning of cache list */
	node->cache.prev = NULL;
	node->cache.next = buffer->cache.head;
	if (buffer->cache.head != NULL)
	{
		buffer->cache.head->prev = node;
	}
	buffer->stamp++;
}

void
dma_sparse_buffer_remove (DmaSparseBuffer *buffer, DmaSparseBufferNode *node)
{
	/* Remove node from node list */
	if (node->next != NULL)
	{
		node->next->prev = node->prev;
	}
	if (node->prev != NULL)
	{
		node->prev->next = node->next;
	}
	if (buffer->head == node)
	{
		buffer->head = node->next;
	}
	
	/* Remove node from cache list */
	if (node->cache.next != NULL)
	{
		node->cache.next->prev = node->cache.prev;
	}
	if (node->cache.prev != NULL)
	{
		node->cache.prev->next = node->cache.next;
	}
	if (buffer->cache.head == node)
	{
		buffer->cache.head = node->cache.next;
	}
	if (buffer->cache.tail == node)
	{
		buffer->cache.tail = node->cache.prev;
	}
	
	g_free (node);
	
	buffer->stamp++;
}

void
dma_sparse_buffer_remove_all (DmaSparseBuffer *buffer)
{
	DmaSparseBufferNode *node;
	DmaSparseBufferNode *next;
	
	for (node = buffer->head; node != NULL; node = next)
	{
		next = node->next;
		g_free (node);
	}
	buffer->cache.head = NULL;
	buffer->cache.tail = NULL;
	buffer->head = NULL;
	buffer->stamp++;
}

guint
dma_sparse_buffer_get_lower (const DmaSparseBuffer *buffer)
{
	return buffer->lower;
}

guint
dma_sparse_buffer_get_upper (const DmaSparseBuffer *buffer)
{
	return buffer->upper;
}

void
dma_sparse_buffer_changed (const DmaSparseBuffer *buffer)
{
		g_signal_emit (G_OBJECT (buffer), signals[CHANGED], 0);
}

/* Iterator private functions
 *---------------------------------------------------------------------------*/

static gboolean
dma_sparse_iter_forward_line (DmaSparseIter *iter)
{
	return DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->forward_line (iter);
}

static gboolean
dma_sparse_iter_backward_line (DmaSparseIter *iter)
{
	return DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->backward_line (iter);
}

static void
dma_sparse_iter_insert_line (DmaSparseIter *iter, GtkTextIter *dst)
{
	DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->insert_line (iter, dst);
}

/* Iterator public functions
 *---------------------------------------------------------------------------*/

void
dma_sparse_buffer_get_iterator_at_address (DmaSparseBuffer *buffer, DmaSparseIter *iter, guint address)
{
	g_return_if_fail (iter != NULL);
 	g_return_if_fail (DMA_IS_SPARSE_BUFFER (buffer));

	iter->buffer = buffer;
	iter->node = dma_sparse_buffer_find (buffer, address);
	iter->base = address;
	iter->offset = 0;
	iter->stamp = buffer->stamp;
	iter->line = 0;
	
    DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->refresh_iter (iter);
}

void
dma_sparse_buffer_get_iterator_near_address (DmaSparseBuffer *buffer, DmaSparseIter *iter, guint address)
{
	g_return_if_fail (iter != NULL);
 	g_return_if_fail (DMA_IS_SPARSE_BUFFER (buffer));
	
	iter->buffer = buffer;
	iter->node = dma_sparse_buffer_find (buffer, address);
	iter->base = address;
	iter->offset = 1;
	iter->line = 0;
	iter->stamp = buffer->stamp;
	
    DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->refresh_iter (iter);
}

void
dma_sparse_iter_copy (DmaSparseIter *dst, const DmaSparseIter *src)
{
	memcpy(dst, src, sizeof (DmaSparseIter));
}

void
dma_sparse_iter_move_at (DmaSparseIter *iter, guint address)
{
	dma_sparse_buffer_get_iterator_at_address (iter->buffer, iter, address);
}

void
dma_sparse_iter_move_near (DmaSparseIter *iter, guint address)
{
	dma_sparse_buffer_get_iterator_near_address (iter->buffer, iter, address);
}

void
dma_sparse_iter_refresh (DmaSparseIter *iter)
{
	if (iter->buffer->stamp != iter->stamp)
	{
		iter->node = dma_sparse_buffer_find (iter->buffer, iter->base);
		iter->stamp = iter->buffer->stamp;
       	DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->refresh_iter (iter);
	}
}

void
dma_sparse_iter_round (DmaSparseIter *iter, gboolean round_up)
{
	if (iter->buffer->stamp != iter->stamp)
	{
		iter->node = dma_sparse_buffer_find (iter->buffer, iter->base);
		iter->stamp = iter->buffer->stamp;
	}
    DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->round_iter (iter, round_up);
}

gulong
dma_sparse_iter_get_address (DmaSparseIter *iter)
{
	return DMA_GET_SPARSE_BUFFER_CLASS (iter->buffer)->get_address (iter);
}

gboolean
dma_sparse_iter_forward_lines (DmaSparseIter *iter, gint count)
{
	gint i;

	dma_sparse_iter_refresh (iter);
	
	if (count < 0)
	{
		for (i = 0; i > count; --i)
		{
			if (!dma_sparse_iter_backward_line (iter)) return FALSE;
		}
	}
	else if (count > 0)
	{
		for (i = 0; i < count; i++)
		{
			if (!dma_sparse_iter_forward_line (iter)) return FALSE;
		}
	}
	
	return TRUE;
}

void
dma_sparse_iter_insert_lines (DmaSparseIter *src, GtkTextIter *dst, guint count)
{
	DmaSparseIter iter;
	guint line = 0;
	GtkTextBuffer *buffer;
	
	buffer = gtk_text_iter_get_buffer (dst);

	/* It is possible to get an iterator that doesn't point to any node
	 * if it has been move to a fixed address without rounding it to
	 * the nearest line */
	 
	dma_sparse_iter_copy (&iter, src);
	dma_sparse_iter_refresh (&iter);

	/* Fill with data */
	for (; line < count; line++)
	{
		dma_sparse_iter_insert_line (&iter, dst);
		if (!dma_sparse_iter_forward_line (&iter))
		{
			/* no more data */
			break;
		}
		if (line != count - 1) gtk_text_buffer_insert (buffer, dst, "\n", 1);
	}	
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_sparse_buffer_dispose (GObject *object)
{
	/*DmaSparseBuffer *self = DMA_DATA_BUFFER (object);*/
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_sparse_buffer_finalize (GObject *object)
{
	DmaSparseBuffer *buffer = DMA_SPARSE_BUFFER (object);
	DmaSparseBufferTransport *trans;

	dma_sparse_buffer_remove_all (buffer);

	/* Free all remaining transport structure */
	for (trans = buffer->pending; trans != NULL;)
	{
		DmaSparseBufferTransport *next = trans->next;
		
		g_slice_free (DmaSparseBufferTransport, trans);
		trans = next;
	}
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_sparse_buffer_instance_init (DmaSparseBuffer *buffer)
{
	buffer->lower = 0;
	buffer->upper = 0;
	
	buffer->cache.head = NULL;
	buffer->cache.tail = NULL;
	buffer->head = NULL;
	buffer->stamp = 0;
	buffer->pending = NULL;
}

/* class_init intialize the class itself not the instance */

static void
dma_sparse_buffer_class_init (DmaSparseBufferClass * klass)
{
	GObjectClass *object_class;

	g_return_if_fail (klass != NULL);
	
	parent_class = g_type_class_peek_parent (klass);
	
	object_class = G_OBJECT_CLASS (klass);
	
	object_class->dispose = dma_sparse_buffer_dispose;
	object_class->finalize = dma_sparse_buffer_finalize;
	
	klass->changed = on_dma_sparse_buffer_changed;
	
	signals[CHANGED] = g_signal_new ("changed",
									 G_OBJECT_CLASS_TYPE (object_class),
									 G_SIGNAL_RUN_LAST,
									 G_STRUCT_OFFSET (DmaSparseBufferClass, changed),
									 NULL, NULL,
									 anjuta_marshal_VOID__VOID,
									 G_TYPE_NONE,
									 0);
}

GType
dma_sparse_buffer_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaSparseBufferClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_sparse_buffer_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaSparseBuffer),
			0,              /* n_preallocs */
			(GInstanceInitFunc) dma_sparse_buffer_instance_init,
			NULL            /* value_table */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
		                            "DmaSparseBuffer", &type_info, 0);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

DmaSparseBuffer*
dma_sparse_buffer_new (guint lower, guint upper)
{
	DmaSparseBuffer *buffer;

	buffer = g_object_new (DMA_SPARSE_BUFFER_TYPE, NULL);
	g_assert (buffer != NULL);

	buffer->lower = lower;
	buffer->upper = upper;
	
	return buffer;
}
	
void
dma_sparse_buffer_free (DmaSparseBuffer *buffer)
{
	g_object_unref (buffer);
}


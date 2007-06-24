/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    disassemble.c
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
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-markable.h>


#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "disassemble.h"

#include "sparse_buffer.h"
#include "sparse_view.h"

#include "plugin.h"

/* Constants
 *---------------------------------------------------------------------------*/

enum {DMA_DISASSEMBLY_BUFFER_BLOCK_SIZE = 256,
	DMA_DISASSEMBLY_SKIP_BEGINNING_LINE = 4,
	DMA_DISASSEMBLY_TAB_LENGTH = 4,
	DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH = 8,
	DMA_DISASSEMBLY_PAGE_DISTANCE = 4 * 60,
	DMA_DISASSEMBLY_VALID_ADDRESS = 0,
	DMA_DISASSEMBLY_KNOW_ADDRESS = -1,
	DMA_DISASSEMBLY_UNKNOWN_ADDRESS = -2};

enum {DMA_DISASSEMBLY_KEEP_ALL,
	DMA_DISASSEMBLY_SKIP_BEGINNING};

/* Types
 *---------------------------------------------------------------------------*/

typedef struct _DmaDisassemblyLine DmaDisassemblyLine;
typedef struct _DmaDisassemblyBufferNode DmaDisassemblyBufferNode;

typedef struct _DmaDisassemblyBuffer DmaDisassemblyBuffer;
typedef struct _DmaDisassemblyBufferClass DmaDisassemblyBufferClass;

typedef struct _DmaDisassemblyView DmaDisassemblyView;
typedef struct _DmaDisassemblyViewClass DmaDisassemblyViewClass;

struct _DmaDisassemble
{
	IAnjutaDebugger *debugger;
	DebugManagerPlugin *plugin;
	GtkWidget *window;
	GtkWidget *menu;
	DmaSparseBuffer* buffer;
	DmaSparseView* view;
};

/* Disassembly buffer object
 *---------------------------------------------------------------------------*/

struct _DmaDisassemblyBuffer
{
	DmaSparseBuffer parent;
	IAnjutaDebugger *debugger;
	gboolean pending;
};

struct _DmaDisassemblyBufferClass
{
	DmaSparseBufferClass parent;
};

struct _DmaDisassemblyLine
{
	guint address;
	gchar* text;
};

struct _DmaDisassemblyBufferNode
{
	DmaSparseBufferNode parent;
	guint size;
	
	DmaDisassemblyLine data[];
};

#define DMA_DISASSEMBLY_BUFFER_TYPE              (dma_disassembly_buffer_get_type ())
#define DMA_DISASSEMBLY_BUFFER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_DISASSEMBLY_BUFFER_TYPE, DmaDisassemblyBuffer))
#define DMA_DISASSEMBLY_BUFFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_DISASSEMBLY_BUFFER_TYPE, DmaDisassemblyBufferClass))
#define IS_DMA_DISASSEMBLY_BUFFER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_DISASSEMBLY_BUFFER_TYPE))
#define IS_DMA_DISASSEMBLY_BUFFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_DISASSEMBLY_BUFFER_TYPE))
#define GET_DMA_DISASSEMBLY_BUFFER_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_DISASSEMBLY_BUFFER_TYPE, DmaDisassemblyBufferClass))

static DmaSparseBufferClass *parent_buffer_class = NULL;

static GType dma_disassembly_buffer_get_type (void);

/* Disassembly iterator function
 *---------------------------------------------------------------------------*/

/* 
 * Iterator can be divided in 3 cases:
 * 1. Iterator on valid address:
 *		node != NULL
 *		base = address
 *		offset = 0
 *		line = line number in node
 * 2. Iterator on know address:
 *		base = address
 *		offset = 0
 *		line = -1
 * 3. Iterator on unknown address:
 *		base = address (often known)
 *	 	offset != 0 (could be 0 too)
 *		line = -2
 *---------------------------------------------------------------------------*/

static gulong
dma_disassembly_get_address (DmaSparseIter *iter)
{
	return iter->base + iter->offset;
}

static gboolean
dma_disassembly_iter_refresh (DmaSparseIter *iter)
{
	/* Call this after updating node according to base */
	gint line = -1;
	DmaDisassemblyBufferNode *node = (DmaDisassemblyBufferNode *)iter->node;
	
	DEBUG_PRINT("iter_refresh iter->node %p (%x, %d), base %lx offset %ld", iter->node, iter->node != NULL ? iter->node->lower : 0, iter->node != NULL ? ((DmaDisassemblyBufferNode *)iter->node)->size : 0, iter->base, iter->offset);
	if (iter->node != NULL)
	{
		/* Find line corresponding to base */
		if ((iter->node->lower <= iter->base) && (iter->base <= iter->node->upper))
		{
			/* Iterator in current node */
			if ((iter->line >= 0) && (iter->line < ((DmaDisassemblyBufferNode *)iter->node)->size)
				&& (((DmaDisassemblyBufferNode *)iter->node)->data[iter->line].address == iter->base))
			{
				/* Already get the right node */
				line = iter->line;
			}
			else
			{
				/* Search for correct line */
			
				if (iter->offset >= 0)
				{
					for (line = 0; line < node->size; line++)
					{
						if (node->data[line].address >= iter->base) break;
					}
				}
				else
				{
					for (line = node->size - 1; line >= 0; line--)
					{
						if (node->data[line].address <= iter->base) break;
					}
				}

				if (node->data[line].address == iter->base)
				{
					iter->line = line;
				}
				else if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS)
				{
					iter->line = iter->offset == 0  ? DMA_DISASSEMBLY_KNOW_ADDRESS : DMA_DISASSEMBLY_UNKNOWN_ADDRESS;
				}
			}
		}
		else if (iter->base == iter->node->upper + 1)
		{
			line = node->size;
			if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS)
			{
				iter->line = iter->offset == 0  ? DMA_DISASSEMBLY_KNOW_ADDRESS : DMA_DISASSEMBLY_UNKNOWN_ADDRESS;
			}
		}
		else
		{
			/* Invalid base address */
			if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS)
			{
				iter->line = iter->offset == 0  ? DMA_DISASSEMBLY_KNOW_ADDRESS : DMA_DISASSEMBLY_UNKNOWN_ADDRESS;
			}
		}
	}
	else
	{
		/* Invalid base address */
		if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS)
		{
			iter->line = iter->offset == 0  ? DMA_DISASSEMBLY_KNOW_ADDRESS : DMA_DISASSEMBLY_UNKNOWN_ADDRESS;
		}
	}

	/* Try to reduce offset */
	DEBUG_PRINT("iter_refresh offset iter->node %p, base %lx offset %ld line %d line %d", iter->node, iter->base, iter->offset, iter->line, line);
	if (line != -1)
	{
		if (iter->offset > 0)
		{
			/* Need to go upper if possible */
			guint up = (DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH + iter->offset - 1)/ DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
			
			for (;;)
			{
				gint len = node->size - line;

				if (up < len)
				{
					iter->node = (DmaSparseBufferNode *)node;
					iter->line = line + up;
					iter->base = node->data[iter->line].address;
					iter->offset = 0;
					
					return TRUE;
				}

				if (iter->node->upper == dma_sparse_buffer_get_upper (iter->buffer))
				{
					gboolean move = iter->line != node->size - 1;
					
					iter->node = (DmaSparseBufferNode *)node;
					iter->line = node->size - 1;
					iter->base = node->data[iter->line].address;
					iter->offset = 0;
					
					return move;
				}

				up -= len;
				
				if ((node->parent.next == NULL) || (node->parent.upper != node->parent.next->lower - 1))
				{
					/* No following node */
					
					iter->node = (DmaSparseBufferNode *)node;
					iter->base = node->parent.upper + 1;
					iter->offset = up * DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
					
					if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS)
					{
						iter->line = iter->offset == 0  ? DMA_DISASSEMBLY_KNOW_ADDRESS : DMA_DISASSEMBLY_UNKNOWN_ADDRESS;
					}
					
					break;
				}

				node = (DmaDisassemblyBufferNode *)node->parent.next;
				line = 0;
			}
		}
		else if (iter->offset < 0)
		{
			/* Need to go down if possible */
			gint down = (- iter->offset) / DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;

			for (;;)
			{
				guint len = line;
				
				if (down <= len)
				{
					iter->node = (DmaSparseBufferNode *)node;
					iter->line = line - down;
					iter->base = node->data[iter->line].address;
					iter->offset = 0;
					
					return TRUE;
				}

				if (iter->node->lower == dma_sparse_buffer_get_lower (iter->buffer))
				{
					gboolean move = iter->line != 0;
					
					iter->node = (DmaSparseBufferNode *)node;
					iter->line = 0;
					iter->base = node->data[0].address;
					iter->offset = 0;
					
					return move;
				}

				down -= len;
				
				if ((node->parent.prev == NULL) || (node->parent.lower != node->parent.prev->upper + 1))
				{
					/* No following node */
						
					iter->node = (DmaSparseBufferNode *)node;
					iter->base = node->parent.lower;
					iter->offset = -down * DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
					if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS)
					{
						iter->line = iter->offset == 0  ? DMA_DISASSEMBLY_KNOW_ADDRESS : DMA_DISASSEMBLY_UNKNOWN_ADDRESS;
					}
					break;
				}
				
				node = (DmaDisassemblyBufferNode *)node->parent.prev;
				line = node->size;
			}
		}
	}
	
	/* Round offset */
	DEBUG_PRINT("iter_refresh round iter->node %p, base %lx offset %ld line %d line %d", iter->node, iter->base, iter->offset, iter->line, line);
	if (iter->offset < 0)
	{
		gulong address;
		gboolean move = TRUE;
					
		address = iter->offset + iter->base;
		if ((address < dma_sparse_buffer_get_lower (iter->buffer)) || (address > iter->base))
		{
			address = dma_sparse_buffer_get_lower (iter->buffer);
			move = FALSE;
		}
		address -= address % DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
		iter->offset = address - iter->base;
		
		return move;
	}		
	else if ((iter->offset > 0) || (iter->line == DMA_DISASSEMBLY_UNKNOWN_ADDRESS))
	{
		gulong address;
		gboolean move = TRUE;
					
		address = iter->offset + iter->base;
		if ((address > dma_sparse_buffer_get_upper (iter->buffer)) || (address < iter->base))
		{
			address = dma_sparse_buffer_get_upper (iter->buffer);
			move = FALSE;
		}
		address -= address % DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
		iter->offset = address - iter->base;
		
		return move;
	}
	
	/* return FALSE if iterator reach the lower or upper limit */
	return TRUE;
}

static gboolean
dma_disassembly_iter_backward_line (DmaSparseIter *iter)
{
	iter->offset -= DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
	return dma_disassembly_iter_refresh (iter);
}

static gboolean
dma_disassembly_iter_forward_line (DmaSparseIter *iter)
{
	iter->offset += DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
	return dma_disassembly_iter_refresh (iter);
}


static void
dma_disassembly_iter_round (DmaSparseIter *iter, gboolean round_up)
{
	iter->offset += round_up ? 1 : -1;
	dma_disassembly_iter_refresh (iter);
}

static void
on_disassemble (const IAnjutaDebuggerDisassembly *block, DmaSparseBufferTransport *trans, GError *err)
{
	DmaDisassemblyBufferNode *node;
	DmaDisassemblyBuffer *buffer = (DmaDisassemblyBuffer *)trans->buffer;
	DmaSparseBufferNode *next;
	guint i;
	char *dst;
	
	DEBUG_PRINT ("on disassemble %p", block);
	
	if ((err != NULL) && !g_error_matches (err, IANJUTA_DEBUGGER_ERROR, IANJUTA_DEBUGGER_UNABLE_TO_ACCESS_MEMORY))
		
	{
		/* Command has been cancelled */
		dma_sparse_buffer_free_transport (trans);
		
		return;
	}
	
	/* Find following block */
	next = dma_sparse_buffer_lookup (DMA_SPARSE_BUFFER (buffer), trans->start + trans->length - 1);
	if ((next != NULL) && (next->upper <= trans->start)) next = NULL;
	
	if (err != NULL)
	{
		gulong address = trans->start;
		gint len;
			
		DEBUG_PRINT ("Create a dummy disassembly node");
			
		/* Create a dummy node */
		len = (trans->length + DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH - 1) / DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
		node = (DmaDisassemblyBufferNode *)g_malloc0 (sizeof(DmaDisassemblyBufferNode) + sizeof(DmaDisassemblyLine) * len);
		node->parent.lower = address;
		for (i = 0; i < len; i++)
		{
			if ((next != NULL) && (address >= next->lower)) break;
			node->data[i].address = address;
			node->data[i].text = "????????";
			address += DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
			address -= address % DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH;
		}
		node->size = i;
		if ((next != NULL) && (address >= next->lower))
		{
			address = next->lower -1;
		}
		else
		{	
			address = trans->start + trans->length - 1;
		}
		node->parent.upper = address;
	}
	else
	{
		guint size = 0;
		guint line = 0;
		
		DEBUG_PRINT ("on disassemble size %d", block->size);
		/* Compute size of all data */
		/* use size -1 because last block has no data (NULL) */
		for (i = trans->tag == DMA_DISASSEMBLY_KEEP_ALL ? 0 : 4; i < block->size - 1; i++)
		{	
			if (block->data[i].label)
			{
				size += strlen(block->data[i].label) + 2;
				line++;
			}
			size += strlen(block->data[i].text) + 1 + DMA_DISASSEMBLY_TAB_LENGTH;
			line++;
		}
	
		node = (DmaDisassemblyBufferNode *)g_malloc0 (sizeof(DmaDisassemblyBufferNode) + sizeof(DmaDisassemblyLine) * line + size);
	
		/* Copy all data */
		dst = (gchar *)&(node->data[line]);
		line = 0;
		for (i = trans->tag == DMA_DISASSEMBLY_KEEP_ALL ? 0 : DMA_DISASSEMBLY_SKIP_BEGINNING_LINE; i < block->size - 1; i++)
		{
			gsize len;

			if ((next != NULL) && (block->data[i].address == next->lower)) break;

			/* Add label if exist */
			if (block->data[i].label != NULL)
			{
				len = strlen(block->data[i].label);
				
				node->data[line].address = block->data[i].address;
				node->data[line].text = dst;
				
				memcpy(dst, block->data[i].label, len);
				dst[len] = ':';
				dst[len + 1] = '\0';
				
				dst += len + 2;
				line++;
			}
				
			/* Add disassembled instruction */
			len = strlen(block->data[i].text) + 1;

			node->data[line].address = block->data[i].address;
			node->data[line].text = dst;
		
			memset (dst, ' ', DMA_DISASSEMBLY_TAB_LENGTH);
			memcpy (dst + DMA_DISASSEMBLY_TAB_LENGTH, block->data[i].text, len);
			dst += len + DMA_DISASSEMBLY_TAB_LENGTH;
			line++;
		}

		/* fill last block */
		node->size = line;
		node->parent.lower = node->data[0].address;
		node->parent.upper = block->data[i].address - 1;
	
	}
	
	dma_sparse_buffer_insert (DMA_SPARSE_BUFFER (buffer), (DmaSparseBufferNode *)node);
	dma_sparse_buffer_free_transport (trans);
	dma_sparse_buffer_changed (DMA_SPARSE_BUFFER (buffer));
}

static void
dma_disassembly_buffer_insert_line (DmaSparseIter *iter, GtkTextIter *dst)
{
	DmaDisassemblyBuffer * dis = (DmaDisassemblyBuffer *)iter->buffer;
	GtkTextBuffer *buffer = gtk_text_iter_get_buffer (dst);

	if (dis->debugger != NULL)
	{	
		dma_sparse_iter_refresh (iter);
		if (iter->line < DMA_DISASSEMBLY_VALID_ADDRESS)
		{
			if (iter->buffer->pending == NULL)
			{
				DmaSparseIter end;
				DmaSparseBufferTransport *trans;
				gint i, j;
				gulong start_adr;
				gulong end_adr;
				gint margin;
			
				DEBUG_PRINT("no data at address %lx + %ld", iter->base, iter->offset);
				
				/* If following line is define, get a block stopping here */
				dma_sparse_iter_copy (&end, iter);
				margin = 0;
				for (j = 0; j < DMA_DISASSEMBLY_BUFFER_BLOCK_SIZE / DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH; j++)
				{
					if (!dma_disassembly_iter_forward_line (&end))
					{
						end.offset = 0;
						end.base = dma_sparse_buffer_get_upper (end.buffer);
						break;
					}
					if (margin > DMA_DISASSEMBLY_SKIP_BEGINNING_LINE) break;
					if ((margin != 0) || (end.line >= DMA_DISASSEMBLY_VALID_ADDRESS)) margin++;
				}
				i = j;
				if (iter->line == DMA_DISASSEMBLY_UNKNOWN_ADDRESS)
				{
					for (i = j; i < DMA_DISASSEMBLY_BUFFER_BLOCK_SIZE / DMA_DISASSEMBLY_DEFAULT_LINE_LENGTH; i++)
					{
						if (!dma_disassembly_iter_backward_line (iter)) break;
						if (iter->line >= DMA_DISASSEMBLY_VALID_ADDRESS) break;
					}
				}
				start_adr = dma_sparse_iter_get_address (iter);
				end_adr = dma_sparse_iter_get_address (&end);
				trans = dma_sparse_buffer_alloc_transport (DMA_SPARSE_BUFFER (dis), i, 0);
				trans->tag = i != j ? DMA_DISASSEMBLY_SKIP_BEGINNING : DMA_DISASSEMBLY_KEEP_ALL;
				trans->start = start_adr;
				trans->length = end_adr - start_adr;
				if (end_adr == dma_sparse_buffer_get_upper (DMA_SPARSE_BUFFER (dis)))
				{
					trans->length++;
				}
				DEBUG_PRINT("get disassemble %lx %lx %ld", start_adr, end_adr, trans->length);
				ianjuta_cpu_debugger_disassemble ((IAnjutaCpuDebugger *)dis->debugger, start_adr, end_adr + 1 - start_adr, (IAnjutaDebuggerCallback)on_disassemble, trans, NULL);
			}
		}
		else
		{
			/* Fill with known data */
			gtk_text_buffer_insert (buffer, dst, ((DmaDisassemblyBufferNode *)(iter->node))->data[iter->line].text, -1);
			
			return;
		}
	}
	
	/* Fill with unknow data */
	gtk_text_buffer_insert (buffer, dst, "??", 2);
}

static void
dma_disassembly_buffer_class_init (DmaDisassemblyBufferClass *klass)
{
	DmaSparseBufferClass* buffer_class;
	
	g_return_if_fail (klass != NULL);
	
	parent_buffer_class = (DmaSparseBufferClass*) g_type_class_peek_parent (klass);
	
	buffer_class = DMA_SPARSE_BUFFER_CLASS (klass);

	buffer_class->refresh_iter = dma_disassembly_iter_refresh;
	buffer_class->round_iter = dma_disassembly_iter_round;
	buffer_class->insert_line = dma_disassembly_buffer_insert_line;
	buffer_class->forward_line = dma_disassembly_iter_forward_line;
	buffer_class->backward_line = dma_disassembly_iter_backward_line;
	buffer_class->get_address = dma_disassembly_get_address;
}

static GType
dma_disassembly_buffer_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaDisassemblyBufferClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_disassembly_buffer_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaDisassemblyBuffer),
			0,              /* n_preallocs */
			(GInstanceInitFunc) NULL,
			NULL            /* value_table */
		};

		type = g_type_register_static (DMA_SPARSE_BUFFER_TYPE,
		                            "DmaDisassemblyBuffer", &type_info, 0);
	}
	
	return type;
}


static DmaDisassemblyBuffer*
dma_disassembly_buffer_new (IAnjutaDebugger *debugger, gulong lower, gulong upper)
{
	DmaDisassemblyBuffer *buffer;

	buffer = g_object_new (DMA_DISASSEMBLY_BUFFER_TYPE, NULL);
	g_assert (buffer != NULL);

	buffer->debugger = debugger;

	DMA_SPARSE_BUFFER (buffer)->lower = lower;
	DMA_SPARSE_BUFFER (buffer)->upper = upper;
	
	return buffer;
}
	
static void
dma_disassembly_buffer_free (DmaDisassemblyBuffer *buffer)
{
	g_object_unref (buffer);
}


/* Disassembly view object
 *---------------------------------------------------------------------------*/

struct _DmaDisassemblyView
{
	DmaSparseView parent;
	IAnjutaDebugger *debugger;
	gboolean pending;
};

struct _DmaDisassemblyViewClass
{
	DmaSparseViewClass parent;
};

#define DMA_DISASSEMBLY_VIEW_TYPE              (dma_disassembly_view_get_type ())
#define DMA_DISASSEMBLY_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_DISASSEMBLY_VIEW_TYPE, DmaDisassemblyView))
#define DMA_DISASSEMBLY_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  DMA_DISASSEMBLY_VIEW_TYPE, DmaDisassemblyViewClass))
#define IS_DMA_DISASSEMBLY_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_DISASSEMBLY_VIEW_TYPE))
#define IS_DMA_DISASSEMBLY_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  DMA_DISASSEMBLY_VIEW_TYPE))
#define GET_DMA_DISASSEMBLY_VIEW_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  DMA_DISASSEMBLY_VIEW_TYPE, DmaDisassemblyViewClass))

static DmaSparseViewClass *parent_class = NULL;

static GType dma_disassembly_view_get_type (void);

static void
dma_disassembly_view_class_init (DmaDisassemblyViewClass *klass)
{
	DmaSparseViewClass* view_class;
	
	g_return_if_fail (klass != NULL);
	
	parent_class = (DmaSparseViewClass*) g_type_class_peek_parent (klass);
	
	view_class = DMA_SPARSE_VIEW_CLASS (klass);
}

static GType
dma_disassembly_view_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaDisassemblyViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_disassembly_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaDisassemblyView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) NULL,
			NULL            /* value_table */
		};

		type = g_type_register_static (DMA_SPARSE_VIEW_TYPE,
		                            "DmaDisassemblyView", &type_info, 0);
	}
	
	return type;
}


static DmaDisassemblyView*
dma_disassembly_view_new_with_buffer (IAnjutaDebugger *debugger, DmaSparseBuffer *buffer)
{
	DmaDisassemblyView *view;

	view = g_object_new (DMA_DISASSEMBLY_VIEW_TYPE, NULL);
	g_assert (view != NULL);

	view->debugger = debugger;
	
	dma_sparse_view_set_sparse_buffer (DMA_SPARSE_VIEW(view), buffer);	
		
	return view;
}
	
static void
dma_disassembly_view_free (DmaDisassemblyView *view)
{
	g_object_unref (view);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
on_disassembly_buffer_changed (DmaDisassemblyBuffer *buffer, DmaSparseView *view)
{
	dma_sparse_view_refresh (view);
}

static void
on_breakpoint_changed (DmaDisassemble *self, IAnjutaDebuggerBreakpoint *bp)
{
	g_return_if_fail (bp != NULL);
	
	dma_disassemble_unmark (self, bp->address, IANJUTA_MARKABLE_BREAKPOINT_DISABLED);
	dma_disassemble_unmark (self, bp->address, IANJUTA_MARKABLE_BREAKPOINT_ENABLED);
	if (bp->type != IANJUTA_DEBUGGER_BREAK_REMOVED)
	{
		dma_disassemble_mark (self, bp->address, bp->enable ? IANJUTA_MARKABLE_BREAKPOINT_ENABLED : IANJUTA_MARKABLE_BREAKPOINT_DISABLED);
	}
}

static GtkWidget*
create_disassemble_gui (DmaDisassemble *self)
{
	GtkWidget *dataview;

	dataview = GTK_WIDGET (dma_disassembly_view_new_with_buffer (self->debugger, self->buffer));

	/* Add register window */
	self->window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->window),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->window),
									 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (self->window), GTK_WIDGET (dataview));
	
	anjuta_shell_add_widget (ANJUTA_PLUGIN (self->plugin)->shell,
							 self->window,
                             "AnjutaDebuggerDisassemble", _("Disassembly"),
                             NULL, ANJUTA_SHELL_PLACEMENT_LEFT,
							 NULL);


	DMA_DISASSEMBLY_VIEW (dataview)->pending = FALSE;
	
	return dataview;
}

static void
on_debugger_started (DmaDisassemble *self)
{
	self->buffer = DMA_SPARSE_BUFFER (dma_disassembly_buffer_new (self->debugger, 0x00000000U,0xFFFFFFFFU));

	self->view = DMA_SPARSE_VIEW (create_disassemble_gui (self));

	g_signal_connect_swapped (self->plugin, "breakpoint-changed", G_CALLBACK (on_breakpoint_changed), self);
	
	g_signal_connect (G_OBJECT (self->buffer), 
			"changed", 
			G_CALLBACK (on_disassembly_buffer_changed), 
			self->view);
}

static void
destroy_disassemble_gui (DmaDisassemble *self)
{
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_breakpoint_changed), self);

	/* Destroy menu */
	if (self->menu != NULL)
	{
		gtk_widget_destroy (self->menu);
		self->menu = NULL;
	}

	if (self->window != NULL)
	{
		gtk_widget_destroy (self->window);
		self->window = NULL;
		self->view = NULL;
	}
}

static void
on_debugger_stopped (DmaDisassemble *self)
{
	g_signal_handlers_disconnect_by_func (self->buffer,
			on_disassembly_buffer_changed,
			self->view);

	dma_sparse_buffer_free (DMA_SPARSE_BUFFER (self->buffer));
	self->buffer = NULL;
	
	destroy_disassemble_gui (self);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
dma_disassemble_mark (DmaDisassemble *self, guint address, gint marker)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (self->view != NULL);
	
	dma_sparse_view_mark (self->view, address, marker);
}

void
dma_disassemble_unmark (DmaDisassemble *self, guint address, gint marker)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (self->view != NULL);
	
	dma_sparse_view_unmark (self->view, address, marker);
}

void
dma_disassemble_clear_all_mark (DmaDisassemble *self, gint marker)
{
	g_return_if_fail (self != NULL);
	/* Accept to clear mark without a view, used at initialization */
	if (self->view != NULL)
		dma_sparse_view_delete_all_markers (self->view, marker);
}

void
dma_disassemble_goto_address (DmaDisassemble *self, guint address)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (self->view != NULL);
	
	dma_sparse_view_goto (self->view, address);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

DmaDisassemble*
dma_disassemble_new(AnjutaPlugin *plugin, IAnjutaDebugger *debugger)
{
	DmaDisassemble* self;
	
	self = g_new0 (DmaDisassemble, 1);
	
	self->debugger = debugger;
	if (debugger != NULL) g_object_ref (debugger);
	self->plugin = ANJUTA_PLUGIN_DEBUG_MANAGER (plugin);
	self->buffer = NULL;
	self->view = NULL;

	g_signal_connect_swapped (self->debugger, "debugger-started", G_CALLBACK (on_debugger_started), self);
	g_signal_connect_swapped (self->debugger, "debugger-stopped", G_CALLBACK (on_debugger_stopped), self);
	
	return self;
}

void
dma_disassemble_free(DmaDisassemble* self)
{
	g_return_if_fail (self != NULL);

	destroy_disassemble_gui (self);

	if (self->debugger != NULL)	g_object_unref (self->debugger);
	
	g_free(self);
}



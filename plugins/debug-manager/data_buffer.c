/*
    data_buffer.c
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

#include "data_buffer.h"

#include "anjuta-marshal.h"

#include <string.h>
#include <glib/gprintf.h>

enum
{
	DMA_DATA_BUFFER_PAGE_SIZE = 512,
	DMA_DATA_BUFFER_LAST_LEVEL_SIZE = 8,
	DMA_DATA_BUFFER_LEVEL_SIZE = 16,
	DMA_DATA_BUFFER_LEVEL = 6
};

enum
{
	DMA_DATA_UNDEFINED = 0,
	DMA_DATA_MODIFIED = 1,
	DMA_DATA_SAME = 2
};

typedef struct _DmaDataBufferPage DmaDataBufferPage;
typedef struct _DmaDataBufferNode DmaDataBufferNode;
typedef struct _DmaDataBufferLastNode DmaDataBufferLastNode;

struct _DmaDataBufferPage
{
	gchar data[DMA_DATA_BUFFER_PAGE_SIZE];
	gchar tag[DMA_DATA_BUFFER_PAGE_SIZE];
	guint validation;
};

struct _DmaDataBufferNode
{
	DmaDataBufferNode *child[DMA_DATA_BUFFER_LEVEL_SIZE];
};

struct _DmaDataBufferLastNode
{
	DmaDataBufferPage *child[DMA_DATA_BUFFER_LAST_LEVEL_SIZE];
};

struct _DmaDataBuffer
{
	GObject parent;
	
	gulong lower;
	gulong upper;
	
	DmaDataBufferReadFunc read;
	DmaDataBufferWriteFunc write;
	gpointer user_data;
	
	guint validation;
	DmaDataBufferNode *top;
};

struct _DmaDataBufferClass
{
	GObjectClass parent;
	
	void (*changed_notify) (DmaDataBuffer *buffer, gulong lower, gulong upper);	
};

static GObjectClass *parent_class = NULL;

typedef gchar * (*DmaDisplayDataFunc) (gchar *string, const char *data, const char *tag);

enum
{
	CHANGED_NOTIFY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};


/* Helper functions
 *---------------------------------------------------------------------------*/

static gchar *
display_in_octal (gchar *string, const gchar *data, const gchar *tag)
{
	if ((data == NULL) || (*tag == DMA_DATA_UNDEFINED))
	{
		memcpy(string, "??? ", 4);
	}
	else
	{
		g_sprintf (string, "%03o ", ((unsigned)*data) & 0xFFU);
	}
	
	return string + 4;
}

static gchar *
display_in_decimal (gchar *string, const gchar *data, const gchar *tag)
{
	if ((data == NULL) || (*tag == DMA_DATA_UNDEFINED))
	{
		memcpy(string, "??? ", 4);
	}
	else
	{
		g_sprintf (string, "% 3u ", ((unsigned)*data) & 0xFFU);
	}
	
	return string + 4;
}

static gchar *
display_in_hexadecimal (gchar *string, const gchar *data, const gchar *tag)
{
	if ((data == NULL) || (*tag == DMA_DATA_UNDEFINED))
	{
		memcpy(string, "?? ", 3);
	}
	else
	{
		g_sprintf (string, "%02X ", ((unsigned)*data) & 0xFFU);
	}
	
	return string + 3;
}

static gchar *
display_in_ascii (gchar *string, const gchar *data, const gchar *tag)
{
	if ((data == NULL) || (*tag == DMA_DATA_UNDEFINED))
	{
		*string = '?';
	}
	else if (g_ascii_isprint (*data))
	{
		*string = *data;
	}
	else
	{
		*string = '.';
	}
	
	return string + 1;
}

/* DmaBufferNode functions
 *---------------------------------------------------------------------------*/

static DmaDataBufferNode** 
dma_data_buffer_find_node (DmaDataBuffer *buffer, gulong address)
{
	DmaDataBufferNode **node;
	gint i;
	
	node = &buffer->top;
	address /= DMA_DATA_BUFFER_PAGE_SIZE;
	for (i = DMA_DATA_BUFFER_LEVEL - 1; i >= 0; --i)
	{
		if (*node == NULL)
		{
			if (i == 0)
			{
				*node = (DmaDataBufferNode *)g_new0 (DmaDataBufferLastNode, 1);
			}
			else
			{
				*node = g_new0 (DmaDataBufferNode, 1);
			}
		}
		node = &((*node)->child[address % (i == 0 ? DMA_DATA_BUFFER_LAST_LEVEL_SIZE : DMA_DATA_BUFFER_LEVEL_SIZE)]);
		address /= DMA_DATA_BUFFER_LEVEL_SIZE;
	}

	return node;
}

#if 0
static DmaDataBufferPage* 
dma_data_buffer_get_page (DmaDataBuffer *buffer, gulong address)
{
	DmaDataBufferNode **node;

	node = dma_data_buffer_find_node (buffer, address);

	return (DmaDataBufferPage *)*node;
}
#endif

static DmaDataBufferPage* 
dma_data_buffer_add_page (DmaDataBuffer *buffer, gulong address)
{
	DmaDataBufferNode **node;
	DmaDataBufferPage *page;

	node = dma_data_buffer_find_node (buffer, address);
	
	if (*node == NULL)
	{
		*node = (DmaDataBufferNode *)g_new0 (DmaDataBufferPage, 1);
		page = (DmaDataBufferPage *)*node;
		page->validation = buffer->validation - 1;
	}
	else
	{
		page = (DmaDataBufferPage *)*node;
	}
	
	return page;
}

static DmaDataBufferPage* 
dma_data_buffer_read_page (DmaDataBuffer *buffer, gulong address)
{
	DmaDataBufferPage *page;

	page = dma_data_buffer_add_page (buffer, address);

	if ((page == NULL) || (page->validation != buffer->validation))
	{
		if (page != NULL) page->validation = buffer->validation;
		/* Data need to be refresh */
		if (buffer->read != NULL);
			buffer->read (address - (address % DMA_DATA_BUFFER_PAGE_SIZE), DMA_DATA_BUFFER_PAGE_SIZE, buffer->user_data);
		
		return page;
	}
	
	
	return page;
}

static void
dma_data_buffer_free_node (DmaDataBufferNode *node, gint level)
{
	gint i;

	for (i = (level == 0 ? DMA_DATA_BUFFER_LAST_LEVEL_SIZE : DMA_DATA_BUFFER_LEVEL_SIZE) - 1; i >= 0; --i)
	{
		if (node->child[i] != NULL)
		{
			if (level != 0)
			{
				dma_data_buffer_free_node (node->child[i], level - 1);
			}
			g_free (node->child[i]);
		}
	}
}

/* Private functions
 *---------------------------------------------------------------------------*/


static void
dma_data_buffer_changed_notify (DmaDataBuffer *buffer, gulong lower, gulong upper)
{
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
dma_data_buffer_remove_all_page (DmaDataBuffer *buffer)
{
	if (buffer->top != NULL)
	{
		dma_data_buffer_free_node (buffer->top, DMA_DATA_BUFFER_LEVEL - 1);
		g_free (buffer->top);
		buffer->top = NULL;
	}
}

gulong
dma_data_buffer_get_lower (const DmaDataBuffer *buffer)
{
	return buffer->lower;
}

gulong
dma_data_buffer_get_upper (const DmaDataBuffer *buffer)
{
	return buffer->upper;
}

gchar *
dma_data_buffer_get_address (DmaDataBuffer *buffer, gulong lower, guint length, guint step, guint size)
{
	gchar *data;
	gchar *ptr;
	guint line;
	
	line = (length + step - 1) / step;
	
	data = g_strnfill (line * (size + 1), ' ');
	for (ptr = data; line != 0; --line)
	{
		g_sprintf (ptr, "%0*lx\n", size, lower);
		lower += step;
		ptr += size + 1;
	}
	*(ptr - 1) = '\0';
	
	return data;
}
		
gchar *
dma_data_buffer_get_data (DmaDataBuffer *buffer, gulong lower, guint length, guint step, gint base)
{
	static const DmaDisplayDataFunc format_table[] = {display_in_octal,
	                                          display_in_decimal,
                                              display_in_hexadecimal,
                                              display_in_ascii};
	gchar *text;
	gchar *ptr;
	guint line;
	guint col;
	DmaDisplayDataFunc display;
	guint inc;
    gchar dummy[16];
    gchar *data;
	gchar *tag;
	guint len;

	line = (length + step - 1) / step;
											  
	switch (base & DMA_DATA_BASE)
	{
	case DMA_OCTAL_BASE:
	case DMA_DECIMAL_BASE:
	case DMA_HEXADECIMAL_BASE:
	case DMA_ASCII_BASE:
		display = format_table[base & DMA_DATA_BASE];
	    break;
    default:
		display = format_table[DMA_HEXADECIMAL_BASE];
	    break;
    }
	
	/* Dummy call of display function to know the increment */
	ptr = display (dummy, NULL, NULL);
	inc = ptr - dummy;
	
	text = g_strnfill (line * (step * inc + 1), ' ');
	len = 0;
	for (ptr = text; line != 0; --line)
	{
		for (col = step; col != 0; --col)
		{
			if (len == 0)
			{
				DmaDataBufferPage *node;
				/* Get new data */
				node = dma_data_buffer_read_page (buffer, lower);
				if (node == NULL)
				{
					data = NULL;
					tag = NULL;
				}
				else
				{
					data = &(node->data[lower % DMA_DATA_BUFFER_PAGE_SIZE]);
					tag = &(node->tag[lower % DMA_DATA_BUFFER_PAGE_SIZE]);
				}
				len = DMA_DATA_BUFFER_PAGE_SIZE - (lower % DMA_DATA_BUFFER_PAGE_SIZE);
			}
			ptr = display (ptr, data, tag);
			if (data != NULL)
			{
				data++;
				tag++;
			}
			lower++;
			len--;
		}
		if (inc != 1) --ptr; /* Remove last space */
		*ptr++ = '\n';
	}
	*(ptr - 1) = '\0'; /* Remove last carriage return */
	
	return text;
}

void
dma_data_buffer_invalidate (DmaDataBuffer *buffer)
{
	buffer->validation++;
}

void
dma_data_buffer_set_data (DmaDataBuffer *buffer, gulong address, gulong length, const gchar *data)
{
	gulong upper;
	gulong lower;
	
	if (length == 0) return;
	
	upper = address + length -1UL;
	lower = address;
	
	while (length)
	{
		DmaDataBufferPage *page;
		guint len;
		
		page = dma_data_buffer_add_page (buffer, address);
		len = DMA_DATA_BUFFER_PAGE_SIZE - (address % DMA_DATA_BUFFER_PAGE_SIZE);
		if (len > length) len = length;
		memcpy (&page->data[address % DMA_DATA_BUFFER_PAGE_SIZE], data, len);
		memset (&page->tag[address % DMA_DATA_BUFFER_PAGE_SIZE], DMA_DATA_MODIFIED, len);
		page->validation = buffer->validation;
		
		length -= len;
		address += len;
	}
	g_signal_emit (buffer, signals[CHANGED_NOTIFY], 0, lower, upper);
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_data_buffer_dispose (GObject *object)
{
	/*DmaDataBuffer *this = DMA_DATA_BUFFER (object);*/
	
	(* parent_class->dispose) (object);
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_data_buffer_finalize (GObject *object)
{
	DmaDataBuffer *buffer = DMA_DATA_BUFFER (object);

	dma_data_buffer_remove_all_page (buffer);
	
	(* parent_class->finalize) (object);
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_data_buffer_instance_init (DmaDataBuffer *this)
{
	this->lower = 0;
	this->upper = 0;
}

/* class_init intialize the class itself not the instance */

static void
dma_data_buffer_class_init (DmaDataBufferClass * klass)
{
	GObjectClass *object_class;

	g_return_if_fail (klass != NULL);
	
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	
	object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = dma_data_buffer_dispose;
	object_class->finalize = dma_data_buffer_finalize;

	klass->changed_notify = dma_data_buffer_changed_notify;
	
	signals[CHANGED_NOTIFY] = g_signal_new ("changed_notify",
									 G_OBJECT_CLASS_TYPE (object_class),
									 G_SIGNAL_RUN_LAST,
									 G_STRUCT_OFFSET (DmaDataBufferClass, changed_notify),
									 NULL, NULL,
									 anjuta_marshal_VOID__ULONG_ULONG,
									 G_TYPE_NONE,
									 2,
									 G_TYPE_ULONG,
									 G_TYPE_ULONG);	
}

GType
dma_data_buffer_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaDataBufferClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_data_buffer_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaDataBuffer),
			0,              /* n_preallocs */
			(GInstanceInitFunc) dma_data_buffer_instance_init,
			NULL            /* value_table */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
		                            "DmaDataBuffer", &type_info, 0);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

DmaDataBuffer*
dma_data_buffer_new (gulong lower, gulong upper, DmaDataBufferReadFunc read, DmaDataBufferWriteFunc write, gpointer user_data)
{
	DmaDataBuffer *buffer;

	buffer = g_object_new (DMA_DATA_BUFFER_TYPE, NULL);
	g_assert (buffer != NULL);

	buffer->lower = lower;
	buffer->upper = upper;
	buffer->read = read;
	buffer->write = write;
	buffer->user_data = user_data;
	
	return buffer;
}
	
void
dma_data_buffer_free (DmaDataBuffer *buffer)
{
	g_object_unref (buffer);
}


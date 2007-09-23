/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    memory.c
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
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include <libanjuta/anjuta-debug.h>

#include "memory.h"

#include "data_view.h"
#include "plugin.h"
#include "queue.h"

/* Constants
 *---------------------------------------------------------------------------*/

/* Types
 *---------------------------------------------------------------------------*/

struct _DmaMemory
{
	DmaDebuggerQueue *debugger;
	AnjutaPlugin *plugin;
	GtkWidget *window;
	DmaDataBuffer *buffer;
	GtkWidget *menu;
};

/* Private functions
 *---------------------------------------------------------------------------*/

static void
on_memory_block_read (const IAnjutaDebuggerMemory *block, DmaMemory *mem, GError *err)
{
	const gchar* tag;
	
	if (block == NULL)
	{
		/* Command has been cancelled */
		return;
	}
	
	guint length = block->length;
	gchar *data = block->data;
	guint address = block->address;
	
	tag = data + length;
	while (length != 0)
	{
		const gchar* start;
		
		/* Skip undefined data */
		for (start = tag; *tag == 0; tag++)
		{
			length--;
			if (length == 0) return;
		}
		data += tag - start;
		address += tag - start;
		
		/* Compute length of defined data */
		for (start = tag; (length != 0) && (*tag != 0); tag++)
		{
			length--;
		}
		
		dma_data_buffer_set_data (mem->buffer, address, tag - start, data);
		data += tag - start;
		address += tag - start;
	}
}

static void
read_memory_block (gulong address, gulong length, gpointer user_data)
{
	DmaMemory *mem = (DmaMemory *)user_data;
	
	if (mem->debugger != NULL)
	{	
		dma_queue_inspect_memory (
				mem->debugger,
				(guint)address,
				(guint)length,
				(IAnjutaDebuggerCallback)on_memory_block_read,
				mem);
	}
}

static void
dma_memory_update (DmaMemory *mem)
{
	dma_data_buffer_invalidate(mem->buffer);
	dma_data_view_refresh(DMA_DATA_VIEW (mem->window));
}

static void
on_program_stopped (DmaMemory *mem)
{
	dma_memory_update (mem);
}

static void
create_memory_gui (DmaMemory *mem)
{
	GtkWidget *dataview;
	
	dataview = dma_data_view_new_with_buffer (mem->buffer);
	mem->window = dataview;
	
	anjuta_shell_add_widget (mem->plugin->shell,
							 mem->window,
                             "AnjutaDebuggerMemory", _("Memory"),
                             NULL, ANJUTA_SHELL_PLACEMENT_CENTER,
							 NULL);

}

static void
on_debugger_started (DmaMemory *mem)
{
	if (mem->buffer == NULL)
	{
		mem->buffer = dma_data_buffer_new (0x0000, 0xFFFFFFFFU, read_memory_block, NULL, mem);
	}
	create_memory_gui (mem);
}

static void
destroy_memory_gui (DmaMemory *mem)
{
	/* Destroy menu */
	if (mem->menu != NULL)
	{
		gtk_widget_destroy (mem->menu);
	}

	if (mem->window != NULL)
	{
		gtk_widget_destroy (mem->window);
		mem->window = NULL;
		dma_data_buffer_remove_all_page (mem->buffer);
	}
}

static void
on_debugger_stopped (DmaMemory *mem)
{
	destroy_memory_gui (mem);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

DmaMemory*
dma_memory_new(DebugManagerPlugin *plugin)
{
	DmaMemory* mem;
	
	mem = g_new0 (DmaMemory, 1);
	
	mem->debugger = dma_debug_manager_get_queue (plugin);
	mem->plugin = ANJUTA_PLUGIN (plugin);
	mem->buffer = NULL;

	g_signal_connect_swapped (mem->plugin, "debugger-started", G_CALLBACK (on_debugger_started), mem);
	g_signal_connect_swapped (mem->plugin, "debugger-stopped", G_CALLBACK (on_debugger_stopped), mem);
	g_signal_connect_swapped (mem->plugin, "program-stopped", G_CALLBACK (on_program_stopped), mem);

	return mem;
}

void
dma_memory_free(DmaMemory* mem)
{
	g_return_if_fail (mem != NULL);

	destroy_memory_gui (mem);
	
	if (mem->buffer != NULL) g_object_unref (mem->buffer);

	g_free(mem);
}



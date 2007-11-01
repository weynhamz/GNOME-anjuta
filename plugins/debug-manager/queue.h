/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    queue.h
    Copyright (C) 2005 Sébastien Granjoux

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

#ifndef QUEUE_H
#define QUEUE_H

/*---------------------------------------------------------------------------*/

#include "plugin.h"

#include "command.h"

#include <libanjuta/anjuta-plugin.h>

#include <glib.h>

typedef enum
{
	HAS_CPU = 1 << 0,
	HAS_BREAKPOINT = 1 << 1,
	HAS_VARIABLE = 1 << 2
} DmaDebuggerFeature;

typedef struct _DmaDebuggerQueueClass   DmaDebuggerQueueClass;

#define DMA_DEBUGGER_QUEUE_TYPE            (dma_debugger_queue_get_type ())
#define DMA_DEBUGGER_QUEUE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_DEBUGGER_QUEUE_TYPE, DmaDebuggerQueue))
#define DMA_DEBUGGER_QUEUE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DMA_DEBUGGER_QUEUE_TYPE, DmaDebuggerQueueClass))
#define IS_DMA_DEBUGGER_QUEUE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_DEBUGGER_QUEUE_TYPE))
#define IS_DMA_DEBUGGER_QUEUE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DMA_DEBUGGER_QUEUE_TYPE))

GType dma_debugger_queue_get_type (void);

DmaDebuggerQueue *dma_debugger_queue_new (AnjutaPlugin *plugin);
void dma_debugger_queue_free (DmaDebuggerQueue *this);

gboolean dma_debugger_queue_append (DmaDebuggerQueue *self, DmaQueueCommand *cmd);

gboolean dma_debugger_queue_start (DmaDebuggerQueue *self, const gchar *mime_type);
void dma_debugger_queue_stop (DmaDebuggerQueue *self);
void dma_debugger_queue_enable_log (DmaDebuggerQueue *self, IAnjutaMessageView *log);
void dma_debugger_queue_disable_log (DmaDebuggerQueue *self);
gint dma_debugger_queue_get_feature (DmaDebuggerQueue *self);

IAnjutaDebuggerState dma_debugger_queue_get_state (DmaDebuggerQueue *self);

#endif

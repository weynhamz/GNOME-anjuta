/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    debugger.h
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DEBUGGER_H
#define DEBUGGER_H

/*---------------------------------------------------------------------------*/

#include "plugin.h"

#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-cpu-debugger.h>
#include <libanjuta/anjuta-plugin.h>

#include <glib.h>

typedef struct _DmaDebuggerQueue        DmaDebuggerQueue;
typedef struct _DmaDebuggerQueueClass   DmaDebuggerQueueClass;

#define DMA_DEBUGGER_QUEUE_TYPE            (dma_debugger_queue_get_type ())
#define DMA_DEBUGGER_QUEUE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DMA_DEBUGGER_QUEUE_TYPE, DmaDebuggerQueue))
#define DMA_DEBUGGER_QUEUE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DMA_DEBUGGER_QUEUE_TYPE, DmaDebuggerQueueClass))
#define IS_DMA_DEBUGGER_QUEUE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DMA_DEBUGGER_QUEUE_TYPE))
#define IS_DMA_DEBUGGER_QUEUE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DMA_DEBUGGER_QUEUE_TYPE))

GType dma_debugger_queue_get_type (void);

DmaDebuggerQueue *dma_debugger_queue_new (AnjutaPlugin *plugin);
void dma_debugger_queue_free (DmaDebuggerQueue *this);

void dma_debugger_message (DmaDebuggerQueue *this, const gchar* message);
void dma_debugger_info (DmaDebuggerQueue *this, const gchar* message);
void dma_debugger_warning (DmaDebuggerQueue *this, const gchar* message);
void dma_debugger_error (DmaDebuggerQueue *this, const gchar* message);

#endif

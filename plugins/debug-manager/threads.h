/*
    threads.h
    Copyright (C) 2007 Sébastien Granjoux

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

#ifndef _THREADS_H_
#define _THREADS_H_

#include "plugin.h"

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <gtk/gtkwidget.h>

#include <glib.h>

typedef struct _DmaThreads DmaThreads;

DmaThreads *dma_threads_new (IAnjutaDebugger *debugger, DebugManagerPlugin *plugin);
void dma_threads_free (DmaThreads *self);

#endif

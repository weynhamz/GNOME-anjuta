/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    disassemble.h
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

#ifndef _DISASSEMBLE_H
#define _DISASSEMBLE_H

G_BEGIN_DECLS

#include "plugin.h"

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-cpu-debugger.h>

typedef struct _DmaDisassemble DmaDisassemble;

DmaDisassemble* dma_disassemble_new (DebugManagerPlugin *plugin);
void dma_disassemble_free(DmaDisassemble *self);

gboolean dma_disassemble_is_focus (DmaDisassemble *self);
guint dma_disassemble_get_current_address (DmaDisassemble *self);

G_END_DECLS

#endif /* _DISASSEMBLE_H */

/*
    locals.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#ifndef _LOCALS_H_
#define _LOCALS_H_

#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/anjuta-plugin.h>

#include <gtk/gtkwidget.h>

typedef struct _Locals Locals;

Locals *locals_new (AnjutaPlugin* plugin, IAnjutaDebugger* debugger);
void locals_connect (Locals *l, IAnjutaDebugger *debugger);
void locals_free (Locals *l);

void locals_update (Locals *l);
void locals_clear (Locals *l);

#endif

/*
    breakpoints.h
    Copyright (C) 2000  Naba Kumar <naba@gnome.org>

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

#ifndef _BREAKPOINTS_DBASE_H_
#define _BREAKPOINTS_DBASE_H_

#include <glade/glade.h>
#include <stdio.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

G_BEGIN_DECLS

typedef struct _BreakpointsDBase BreakpointsDBase;

BreakpointsDBase *breakpoints_dbase_new (AnjutaPlugin *plugin);
void breakpoints_dbase_destroy (BreakpointsDBase * bd);

void breakpoints_dbase_connect (BreakpointsDBase *bd, IAnjutaDebugger *debugger);
void breakpoints_dbase_disconnect (BreakpointsDBase *bd);

void breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, IAnjutaEditor* te);
void breakpoints_dbase_clear_all_in_editor (BreakpointsDBase* bd, IAnjutaEditor* te);

G_END_DECLS
											
#endif

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
#include "properties.h"
#include "text_editor.h"
#include "project_dbase.h"

typedef struct _BreakpointsDBase BreakpointsDBase;
typedef struct _BreakpointsDBasePriv BreakpointsDBasePriv;

struct _BreakpointsDBase
{
	BreakpointsDBasePriv *priv;
};


BreakpointsDBase *breakpoints_dbase_new (void);

void breakpoints_dbase_save (BreakpointsDBase * bd, ProjectDBase * pdb );

void breakpoints_dbase_show (BreakpointsDBase * bd);

void breakpoints_dbase_hide (BreakpointsDBase * bd);

void breakpoints_dbase_attach (BreakpointsDBase * bd);

void breakpoints_dbase_detach (BreakpointsDBase * bd);

void breakpoints_dbase_dock (BreakpointsDBase * bd);

void breakpoints_dbase_undock (BreakpointsDBase * bd);

void breakpoints_dbase_update (GList * outputs, gpointer p);

void breakpoints_dbase_add_bp (BreakpointsDBase * bd);

void breakpoints_dbase_append (BreakpointsDBase * bd, gchar * string);

void breakpoints_dbase_clear (BreakpointsDBase * bd);

void breakpoints_dbase_destroy (BreakpointsDBase * bd);

void breakpoints_dbase_add (BreakpointsDBase *bd);

gboolean breakpoints_dbase_toggle_breakpoint (BreakpointsDBase* bd, guint l);

gboolean breakpoints_dbase_toggle_doubleclick (guint line);

void breakpoints_dbase_toggle_singleclick (guint line);

void breakpoints_dbase_disable_all (BreakpointsDBase *bd);

void breakpoints_dbase_remove_all (BreakpointsDBase *bd);

gboolean breakpoints_dbase_save_yourself (BreakpointsDBase * bd, FILE * stream);

gboolean breakpoints_dbase_load_yourself (BreakpointsDBase * bd, PropsID props);

void breakpoints_dbase_load (BreakpointsDBase * bd, ProjectDBase *p );

void breakpoints_dbase_set_all (BreakpointsDBase * bd);

void breakpoints_dbase_update_controls (BreakpointsDBase * bd);

void breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, TextEditor* te);

void breakpoints_dbase_clear_all_in_editor (BreakpointsDBase* bd, TextEditor* te);

#endif

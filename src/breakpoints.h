/*
    breakpoints.h
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

#ifndef _BREAKPOINTS_DBASE_H_
#define _BREAKPOINTS_DBASE_H_

#include <glade/glade.h>
#include "properties.h"
#include "text_editor.h"
#include "project_dbase.h"

typedef struct _BreakpointItem BreakpointItem;
typedef struct _BreakpointsDBase BreakpointsDBase;
typedef struct _BreakpointsDBaseGui BreakpointsDBaseGui;

enum _BreakpointType
{ breakpoint, watchpoint, catchpoint };
typedef enum _BreakpointType BreakpointType;

struct _BreakpointItem
{
	gint id;
	gchar *disp;
	gboolean enable;
	gulong addr;
	gint pass;
	gchar *condition;
	gchar *file;
	guint line;
	gint handle;
	gboolean handle_invalid;
	gchar *function;
	gchar *info;
	time_t time;
};

struct _BreakpointsDBaseGui
{
	GtkWidget *window;
	GtkWidget *treeview;
	GtkWidget *remove_button;
	GtkWidget *toggle_button;
	GtkWidget *jumpto_button;
	GtkWidget *properties_button;
	GtkWidget *add_button;
	GtkWidget *removeall_button;
	GtkWidget *enableall_button;
	GtkWidget *disableall_button;
};

struct _BreakpointsDBase
{
	GladeXML *gxml;
	GladeXML *gxml_prop;

	BreakpointsDBaseGui widgets;
	gchar *cond_history, *loc_history, *pass_history;

	/* Breakpoints set in the debugger */
	GList *breakpoints;

	/* Private */
	gint current_index;
	gint edit_index;
	gboolean is_showing;
	gboolean is_docked;
	gint win_pos_x, win_pos_y, win_width, win_height;
};

struct BkItemData
{
	GtkWidget *dialog;
	GtkWidget *loc, *cond, *pass;
	gchar *loc_text, *cond_text, *pass_text;
	BreakpointsDBase *bd;
};

BreakpointItem *breakpoint_item_new (void);

void breakpoint_item_destroy (BreakpointItem * bi);

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

gboolean
breakpoints_dbase_save_yourself (BreakpointsDBase * bd, FILE * stream);

/*gboolean breakpoints_dbase_load_project (BreakpointsDBase * bd);*/

void breakpoints_dbase_load (BreakpointsDBase * bd, ProjectDBase *p );

void breakpoints_dbase_set_all (BreakpointsDBase * bd);

gboolean
breakpoints_dbase_load_yourself (BreakpointsDBase * bd, PropsID props);

/* Private */

void create_breakpoints_dbase_gui (BreakpointsDBase * bd);

GtkWidget *create_bk_add_dialog (BreakpointsDBase * bd);

GtkWidget *create_bk_edit_dialog (BreakpointsDBase * bd);

void breakpoints_dbase_update_controls (BreakpointsDBase * bd);

void breakpoints_dbase_add_brkpnt (BreakpointsDBase * bd, gchar * line);
void breakpoints_dbase_toggle_breakpoint (BreakpointsDBase* b);

void breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, TextEditor* te);
void breakpoints_dbase_clear_all_in_editor (BreakpointsDBase* bd, TextEditor* te);

#endif

/*
    find_text.h
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
#ifndef _FIND_TEXT_H_
#define _FIND_TEXT_H_

#include <gnome.h>
#include <glade/glade.h>
#include "project_dbase.h"
#define FR_CENTRE     -1

typedef struct _FindTextGui FindTextGui;
typedef struct _FindText FindText;

struct _FindTextGui
{
	GtkWidget *GUI;
	GtkWidget *combo;
	GtkWidget *entry;
	GtkWidget *from_cur_loc_radio;
	GtkWidget *from_begin_radio;
	GtkWidget *forward_radio;
	GtkWidget *backward_radio;
	GtkWidget *regexp_radio;
	GtkWidget *string_radio;
	GtkWidget *whole_word_check;
	GtkWidget *ignore_case_check;
};

struct _FindText
{
	GladeXML *gxml;
	FindTextGui f_gui;
	GList *find_history;
	
	/*
	* area = TEXT_EDITOR_FIND_SCOPE_WHOLE		for whole document
	* area = TEXT_EDITOR_FIND_SCOPE_CURRENT	for current location.
	* area = TEXT_EDITOR_FIND_SCOPE_SELECTION	for selected text.
	*/
	gint area;
	
	gboolean forward;
	gboolean regexp;
	gboolean whole_word;
	gboolean ignore_case;
	gboolean is_showing;
	glong incremental_pos;
	gboolean incremental_wrap;
	gint pos_x;
	gint pos_y;
};

FindText *find_text_new (void);
void find_text_save_settings (FindText * ft);
void find_text_show (FindText * ft);
void find_text_hide (FindText * ft);
void find_text_destroy (FindText * ft);

gboolean find_text_save_session ( FindText * ft, ProjectDBase *p );
void find_text_load_session( FindText * ft, ProjectDBase *p );
void enter_selection_as_search_target(void);

#endif

/*
    find_replace.h
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
#ifndef _FIND_REPLACE_H_
#define _FIND_REPLACE_H_

#include <gnome.h>
#include <glade/glade.h>
#include "find_text.h"
#include "properties.h"

#define FR_CENTRE     -1

typedef  struct  _FindAndReplaceGui                        FindAndReplaceGui;
typedef  struct  _FindAndReplace                                FindAndReplace;

struct  _FindAndReplaceGui
{
  GtkWidget *GUI;
  GtkWidget *find_combo;
  GtkWidget *find_entry;
  GtkWidget *replace_combo;
  GtkWidget *replace_entry;
  GtkWidget *from_cur_loc_radio;
  GtkWidget *from_begin_radio;
  GtkWidget *forward_radio;
  GtkWidget *backward_radio;
  GtkWidget *regexp_radio;
  GtkWidget *string_radio;
  GtkWidget *ignore_case_check;
  GtkWidget *whole_word_check;
  GtkWidget *replace_prompt_check;
};

struct _FindAndReplace
{
  GladeXML *gxml;
  GladeXML *gxml_prompt;
  FindAndReplaceGui    r_gui;
  FindText                          *find_text;
  GList                                 *replace_history;
  gboolean                             replace_prompt;
  gboolean                            is_showing;
  gint                                    pos_x;
  gint                                    pos_y;
};


FindAndReplace*
find_replace_new(void);

void
find_replace_show(FindAndReplace* fr);

void
find_replace_hide(FindAndReplace* fr);

void
find_replace_destroy(FindAndReplace* fr);

gboolean
find_replace_save_yourself(FindAndReplace* fr, FILE* stream);

gboolean
find_replace_load_yourself(FindAndReplace* fr, PropsID props);

gboolean 
find_replace_save_project ( FindAndReplace * fr, ProjectDBase *p );
void
find_replace_save_session ( FindAndReplace * fr, ProjectDBase *p );
void
find_replace_load_session ( FindAndReplace * fr, ProjectDBase *p );

#endif

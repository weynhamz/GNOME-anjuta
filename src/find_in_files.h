/*
    find_in_files.h
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
#ifndef _FIND_IN_FILES_H_
#define _FIND_IN_FILES_H_

#include <gnome.h>
#include "project_dbase.h"

#define FR_CENTRE     -1


typedef  struct  _FindInFilesGui                        FindInFilesGui;
typedef  struct  _FindInFiles                                FindInFiles;

struct  _FindInFilesGui
{
  GtkWidget *window;
  GtkWidget *file_entry;
  GtkWidget *file_combo;
  GtkWidget *clist;
  GtkWidget *case_sensitive_check;
  GtkWidget *ignore_binary;
  GtkWidget *add;
  GtkWidget *remove;
  GtkWidget *clear;
  GtkWidget *regexp_entry;
  GtkWidget *regexp_combo;
  GtkWidget *help;
  GtkWidget *ok;
  GtkWidget *cancel;
};

struct _FindInFiles
{
  FindInFilesGui                widgets;
  GList                                *regexp_history;
  gint                                   cur_row;
  gint                                   length;
  gboolean                           is_showing;
  gint                                   win_pos_x;
  gint                                   win_pos_y;
};


FindInFiles*
find_in_files_new(void);

void
create_find_in_files_gui (FindInFiles *fr)  ;

void
find_in_files_show(FindInFiles* fr);

void
find_in_files_hide(FindInFiles* fr);

void
find_in_files_destroy(FindInFiles* fr);

gboolean
find_in_files_save_yourself(FindInFiles* fr, FILE* stream);

gboolean
find_in_files_load_yourself(FindInFiles* fr, FILE* stream);

/* private */

void
find_in_files_terminated(int status, time_t t);

void
find_in_files_mesg_arrived(gchar *mesg);

void
find_in_files_process(FindInFiles* ff);

void
find_in_files_save_session ( FindInFiles* ff, ProjectDBase *p );

void
find_in_files_load_session( FindInFiles * ff, ProjectDBase *p );


#endif

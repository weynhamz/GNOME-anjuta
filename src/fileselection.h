/*
    fileselection.h
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
#ifndef _FILE_SELECTION_H_
#define _FILE_SELECTION_H_

#include "global.h"

/* This structure must be declared static where ever you use it */
typedef struct _FileSelData FileSelData;
struct _FileSelData
{
  char title[MAX_NAME_LENGTH];
  gpointer data;
  void (*click_ok_callback)(GtkDialog*, gpointer);
  void (*click_cancel_callback)(GtkDialog*, gpointer);
  GtkWidget *filesel;
  
};
  
void fileselection_update_dir(GtkWidget *filesel);

GtkWidget* create_fileselection_gui (FileSelData *fd);

/* Free the return */
gchar* fileselection_get_filename (GtkWidget* filesel);

/* Free the return */
gchar* fileselection_get_path (GtkWidget* filesel);

void fileselection_set_title (GtkWidget* filesel, gchar* title);

/* Sets the directory */
gboolean fileselection_set_dir (GtkWidget* filesel, gchar* dir);

/* Sets the filename path */
gboolean
fileselection_set_filename (GtkWidget* filesel, gchar* fname);

GList *
fileselection_get_filelist(GtkWidget * filesel);

void 
fileselection_hide_widget(GtkWidget * widget);

GtkWidget * 
fileselection_storetypes(GtkWidget *filesel, GList *filetypes);

GList * 
fileselection_addtype_f(GList *filetypes, gchar *description, ...);

GList * 
fileselection_addtype(GList *filetypes, gchar *description, GList *extentions);

GList * 
fileselection_getcombolist(GtkWidget * filesel, GList *filetypes);

void 
fileselection_set_combolist(GtkWidget* filesel, GList *combolist);

GtkWidget*
fileselection_clearfiletypes(GtkWidget* filesel);

#endif

/* 
    text_editor_menu.h
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
#ifndef _TEXT_EDITOR_MENU_H_
#define _TEXT_EDITOR_MENU_H_

#include <gtk/gtk.h>

typedef struct _TextEditorMenu TextEditorMenu;
struct _TextEditorMenu
{
	GtkWidget *GUI;
	GtkWidget *copy;
	GtkWidget *cut;
	GtkWidget *autoformat;
	GtkWidget *swap;
	GtkWidget *functions;
	GtkWidget *debug;
};

void create_text_editor_menu_gui (TextEditorMenu *);

TextEditorMenu *text_editor_menu_new (void);

void text_editor_menu_destroy (TextEditorMenu *);

void text_editor_menu_popup (TextEditorMenu * menu, GdkEventButton * bevent);

void
on_text_editor_menu_tags_activate (GtkMenuItem * menuitem,
				       gpointer user_data);

void
on_text_editor_menu_swap_activate (GtkMenuItem * menuitem,
				   gpointer user_data);

#endif

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _TEXT_EDITOR_MENU_H_
#define _TEXT_EDITOR_MENU_H_

#include <gtk/gtk.h>

#if 0
typedef struct _TextEditorMenu TextEditorMenu;
struct _TextEditorMenu
{
	GtkWidget *GUI;
	GtkWidget *cut;
	GtkWidget *copy;
	GtkWidget *paste;
	GtkWidget *context_help;
	GtkWidget *autoformat;
	GtkWidget *swap;
	GtkWidget *functions;
	GtkWidget *debug;
	GtkWidget *docked;
};

//void create_text_editor_menu_gui (TextEditorMenu *);
TextEditorMenu *text_editor_menu_new (void);
void text_editor_menu_destroy (TextEditorMenu *);
void text_editor_menu_popup (TextEditorMenu * menu, GdkEventButton * bevent);
void on_text_editor_menu_tags_activate (GtkMenuItem * menuitem, gpointer data);
void on_text_editor_menu_swap_activate (GtkMenuItem * menuitem, gpointer data);
#endif
#endif

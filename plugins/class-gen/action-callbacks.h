/*
 *  Copyright (C) 2005 Massimo Cora'
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef __CLASS_GEN_CALLBACKS__
#define __CLASS_GEN_CALLBACKS__


#include <gtk/gtk.h>
#include <glib.h>
#include "plugin.h"

gint on_delete_event (GtkWidget* widget, GdkEvent* event, AnjutaClassGenPlugin *plugin);
gboolean on_class_gen_key_press_event(GtkWidget *widget, GdkEventKey *event,
                                           AnjutaClassGenPlugin *plugin);
void on_header_browse_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin);
void on_source_browse_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin);
void on_class_name_changed (GtkEditable* editable, AnjutaClassGenPlugin *plugin);
void on_class_type_changed (GtkEditable* editable, AnjutaClassGenPlugin *plugin);
void on_finish_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin);
void on_cancel_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin);
void on_help_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin);
void on_inline_toggled (GtkToggleButton* button, AnjutaClassGenPlugin *plugin);
void on_header_file_selection_cancel (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin);
void on_source_file_selection_cancel (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin);
void on_header_file_selection (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin); 
void on_source_file_selection (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin); 


#endif

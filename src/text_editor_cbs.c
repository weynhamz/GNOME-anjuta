/*
 * text_editor_cbs.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <ctype.h>

#include "anjuta.h"
#include "controls.h"
#include "text_editor.h"
#include "text_editor_cbs.h"
#include "text_editor_gui.h"
#include "mainmenu_callbacks.h"
#include "resources.h"
#include "Scintilla.h"

void
on_text_editor_window_realize (GtkWidget * widget, gpointer user_data)
{
	TextEditor *te = user_data;
	te->widgets.window = widget;
}

void
on_text_editor_window_size_allocate (GtkWidget * widget,
				     GtkAllocation * allocation,
				     gpointer user_data)
{
	TextEditor *te = user_data;
	te->geom.x = allocation->x;
	te->geom.y = allocation->y;
	te->geom.width = allocation->width;
	te->geom.height = allocation->height;
}

void
on_text_editor_client_realize (GtkWidget * widget, gpointer user_data)
{
	TextEditor *te = user_data;
	te->widgets.client = widget;
}

gboolean
on_text_editor_text_buttonpress_event (GtkWidget * widget,
				       GdkEventButton * event,
				       gpointer user_data)
{
	TextEditor *te = user_data;
	text_editor_check_disk_status ( te, FALSE );
	return FALSE;
}

gboolean
on_text_editor_text_event (GtkWidget * widget,
			   GdkEvent * event, gpointer user_data)
{
	GdkEventButton *bevent;
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *) event)->button != 3)
		return FALSE;
	bevent = (GdkEventButton *) event;
	bevent->button = 1;
	text_editor_menu_popup (((TextEditor *) user_data)->menu, bevent);
	return TRUE;
}

gboolean
on_text_editor_window_delete (GtkWidget * widget,
			      GdkEventFocus * event, gpointer user_data)
{
	on_close_file1_activate (NULL, NULL);
	return TRUE;
}

void
on_text_editor_notebook_close_page (GtkNotebook * notebook,
				GtkNotebookPage * page,
				gint page_num, gpointer user_data)
{
	 gtk_signal_emit_by_name (GTK_OBJECT
                             (app->widgets.menubar.file.close_file),
                             "activate");
}

gboolean
on_text_editor_window_focus_in_event (GtkWidget * widget,
				      GdkEventFocus * event,
				      gpointer user_data)
{
	TextEditor *te = user_data;
	anjuta_set_current_text_editor (te);
	return FALSE;
}

void
on_text_editor_dock_activate (GtkButton * button, gpointer user_data)
{
	GtkWidget *label, *eventbox;
	TextEditor *te = anjuta_get_current_text_editor ();
	label = gtk_label_new (te->filename);
	te->widgets.tab_label = label;
	gtk_widget_show (label);
	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	text_editor_dock (te, eventbox);

	/* For your kind info, this same data is also set in */
	/* the function anjuta_append_text_editor() */
	gtk_object_set_data (GTK_OBJECT (eventbox), "TextEditor", te);

	gtk_notebook_prepend_page (GTK_NOTEBOOK (app->widgets.notebook),
				   eventbox, label);
	gtk_window_set_title (GTK_WINDOW (app->widgets.window),
			      te->full_filename);
	gtk_notebook_set_page (GTK_NOTEBOOK (app->widgets.notebook), 0);
	anjuta_update_page_label(te);
}

void
on_text_editor_text_changed (GtkEditable * editable, gpointer user_data)
{
	TextEditor *te = (TextEditor *) user_data;
	if (text_editor_is_saved (te))
	{
		anjuta_update_title ();
		update_main_menubar ();
		text_editor_update_controls (te);
	}
}

void
on_text_editor_insert_text (GtkEditable * text,
			    const gchar * insertion_text,
			    gint length, gint * pos, TextEditor * te)
{
}

void
on_text_editor_check_yes_clicked (GtkButton * button, TextEditor * te)
{
	text_editor_load_file (te);
}

void
on_text_editor_check_no_clicked (GtkButton * button, TextEditor * te)
{
	te->modified_time = time (NULL);
}

gboolean on_text_editor_auto_save (gpointer data)
{
	TextEditor *te = data;

	if (!te)
		return FALSE;
	if (preferences_get_int (te->preferences, SAVE_AUTOMATIC) == FALSE)
	{
		te->autosave_on = FALSE;
		return FALSE;
	}
	if (te->full_filename == NULL)
		return TRUE;
	if (text_editor_is_saved (te))
		return TRUE;
	if (text_editor_save_file (te))
	{
		anjuta_status (_("Autosaved \"%s\""), te->filename);
	}
	else
	{
		anjuta_warning (_("Autosave \"%s\" -- Failed"), te->filename);
	}
	return TRUE;
}

void
on_text_editor_scintilla_notify (GtkWidget * sci,
				 gint wParam, gpointer lParam, gpointer data)
{
	TextEditor *te;
	struct SCNotification *nt;

	te = data;
	if (te->freeze_count != 0)
		return;
	nt = lParam;
	switch (nt->nmhdr.code)
	{
	case SCN_URIDROPPED:
		scintilla_uri_dropped(nt->text);
		break;
	case SCN_SAVEPOINTREACHED:
	case SCN_SAVEPOINTLEFT:
		anjuta_update_page_label(te);
		anjuta_update_title ();
		update_main_menubar ();
		text_editor_update_controls (te);
		return;
	case SCN_UPDATEUI:
		anjuta_update_app_status (FALSE, NULL);
		te->current_line = text_editor_get_current_lineno (te);
		
/*	case SCEN_SETFOCUS:
	case SCEN_KILLFOCUS:
	case SCN_STYLENEEDED:
	case SCN_CHARADDED:
	case SCN_DOUBLECLICK:
	case SCN_MODIFIED:
	case SCN_MARGINCLICK:
	case SCN_NEEDSHOWN:
*/
	default:
		return;
	}
}

void
on_text_editor_scintilla_size_allocate (GtkWidget * widget,
					GtkAllocation * allocation,
					gpointer data)
{
	TextEditor *te;
	te = data;
	g_return_if_fail (te != NULL);

	if (te->first_time_expose == FALSE)
		return;

	te->first_time_expose = FALSE;
	text_editor_goto_line (te, te->current_line, FALSE);
}

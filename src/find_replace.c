/*
    find_replace.c
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "anjuta.h"
#include "resources.h"
#include "find_replace.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "ScintillaWidget.h"


static const gchar SECTION_REPLACE [] =  {"replace_text"};

static void create_find_replace_gui (FindAndReplace * fr);
static GtkWidget *create_replace_messagebox (FindAndReplace *fr);
static void on_replace_dialog_response (GtkDialog *dialog, gint response,
                                        gpointer user_data);

/*
static gboolean
on_replace_text_close (GtkWidget * widget,
				  gpointer user_data);
*/

static gboolean
on_find_replace_delete_event (GtkDialog *dialog, GdkEvent *event,
							  FindAndReplace *ft)
{
	find_replace_hide (ft);
	return TRUE;
}

FindAndReplace *
find_replace_new ()
{
	FindAndReplace *fr;
	fr = g_malloc (sizeof (FindAndReplace));
	if (fr)
	{
		fr->find_text = find_text_new ();
		fr->replace_history = NULL;
		fr->replace_prompt = TRUE;
		fr->is_showing = FALSE;
		fr->pos_x = 100;
		fr->pos_y = 100;
		create_find_replace_gui (fr);
	}
	return fr;
}

void
find_replace_destroy (FindAndReplace * fr)
{
	gint i;
	if (fr)
	{
		if (fr->find_text)
			find_text_destroy (fr->find_text);

		if (GTK_IS_WIDGET (fr->r_gui.GUI))
			gtk_widget_destroy (fr->r_gui.GUI);
		for (i = 0; i < g_list_length (fr->replace_history); i++)
			g_free (g_list_nth (fr->replace_history, i)->data);
		if (fr->replace_history)
			g_list_free (fr->replace_history);
		g_object_unref (fr->gxml);
		g_free (fr);
	}
}

gboolean find_replace_save_yourself (FindAndReplace * fr, FILE * stream)
{
	find_text_save_settings (fr->find_text);
	return TRUE;
}

void
find_replace_save_session ( FindAndReplace * fr, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != fr );
	session_save_strings( p, SECSTR(SECTION_REPLACETEXT), fr->replace_history );
	if( fr->find_text )
		find_text_save_session ( fr->find_text, p );
}

void
find_replace_load_session ( FindAndReplace * fr, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != fr );
	fr->replace_history	= session_load_strings(p,
											   SECSTR(SECTION_REPLACETEXT),
											   fr->replace_history );
	if( fr->find_text )
		find_text_load_session ( fr->find_text, p );
}

/*
gboolean find_replace_save_project ( FindAndReplace * fr, ProjectDBase *p )
{
	g_return_val_if_fail( NULL != p, FALSE );
	
	session_clear_section( p, SECTION_SEARCH );
	session_clear_section( p, SECTION_REPLACE );
	if( fr->find_text->find_history )
	{
		const gchar *szFnd ;
		int 		i;
		int			nLen = g_list_length (fr->find_text->find_history); 
		for (i = 0; i < nLen ; i++)
		{
			szFnd = (gchar*)g_list_nth (ft->find_history, i)->data ;
			if( szFnd && szFnd[0] )
				session_save_string( p, SECTION_SEARCH, i, szFnd );
		}
	}

	return TRUE;
}
*/

gboolean find_replace_load_yourself (FindAndReplace * fr, PropsID props)
{
	return TRUE;
}

void
find_replace_show (FindAndReplace * fr)
{
	if (fr->find_text->find_history)
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (fr->r_gui.find_combo),
					       fr->find_text->find_history);
	if (fr->replace_history)
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (fr->r_gui.replace_combo),
					       fr->replace_history);
	switch (fr->find_text->area)
	{
	case TEXT_EDITOR_FIND_SCOPE_WHOLE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.from_begin_radio),
					      TRUE);
		break;
	default:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.from_cur_loc_radio),
					      TRUE);
	}
	if (fr->find_text->forward)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.forward_radio),
					      TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.backward_radio),
					      TRUE);
	if (fr->find_text->regexp)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.regexp_radio), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.string_radio), TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (fr->r_gui.ignore_case_check),
				      fr->find_text->ignore_case);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (fr->r_gui.whole_word_check),
				      fr->find_text->whole_word);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (fr->r_gui.replace_prompt_check),
				      fr->replace_prompt);

	gtk_widget_grab_focus (fr->r_gui.find_entry);
	gtk_dialog_set_default_response (GTK_DIALOG (fr->r_gui.GUI),
									 GTK_RESPONSE_OK);
	entry_set_text_n_select (fr->r_gui.find_entry, NULL, TRUE);
	entry_set_text_n_select (fr->r_gui.replace_entry, NULL, FALSE);

	if (fr->is_showing)
	{
		gtk_widget_show (fr->r_gui.GUI);
		gdk_window_raise (fr->r_gui.GUI->window);
		return;
	}
	gtk_widget_set_uposition (fr->r_gui.GUI, fr->pos_x, fr->pos_y);
	gtk_widget_show (fr->r_gui.GUI);
	fr->is_showing = TRUE;
}

void
find_replace_hide (FindAndReplace * fr)
{
	gboolean radio0, radio1, state;
	if (!fr)
		return;
	if (fr->is_showing == FALSE)
		return;
	
	gdk_window_get_root_origin (fr->r_gui.GUI->window, &fr->pos_x,
				    &fr->pos_y);
	gtk_widget_hide (fr->r_gui.GUI);
	fr->is_showing = FALSE;

	fr->find_text->ignore_case =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.ignore_case_check));
	fr->find_text->whole_word =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.whole_word_check));

	radio0 =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.from_begin_radio));
	radio1 =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.from_cur_loc_radio));

	if (radio0)
		fr->find_text->area = TEXT_EDITOR_FIND_SCOPE_WHOLE;
	else
		fr->find_text->area = TEXT_EDITOR_FIND_SCOPE_CURRENT;

	state =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.forward_radio));
	if (state)
		fr->find_text->forward = TRUE;
	else
		fr->find_text->forward = FALSE;

	state =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.regexp_radio));
	if (state)
		fr->find_text->regexp = TRUE;
	else
		fr->find_text->regexp = FALSE;

	fr->find_text->find_history =
		update_string_list (fr->find_text->find_history,
				    gtk_entry_get_text (GTK_ENTRY
							(fr->r_gui.
							 find_entry)),
				    COMBO_LIST_LENGTH);
	if (fr->find_text->find_history)
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (app->widgets.toolbar.
						main_toolbar.find_combo),
					       fr->find_text->find_history);
	fr->replace_history =
		update_string_list (fr->replace_history,
				    gtk_entry_get_text (GTK_ENTRY
							(fr->r_gui.
							 replace_entry)),
				    COMBO_LIST_LENGTH);
	fr->replace_prompt =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (fr->r_gui.
					       replace_prompt_check));
}

static void
create_find_replace_gui (FindAndReplace * fr)
{
	fr->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "find_replace_dialog", NULL);
	fr->r_gui.GUI = glade_xml_get_widget (fr->gxml, "find_replace_dialog");
	gtk_widget_hide (fr->r_gui.GUI);

	gtk_window_set_transient_for (GTK_WINDOW(fr->r_gui.GUI),
                                  GTK_WINDOW(app->widgets.window));
	
	fr->r_gui.find_combo = glade_xml_get_widget (fr->gxml, "find_replace_find_combo");
	fr->r_gui.find_entry = glade_xml_get_widget (fr->gxml, "find_replace_find_entry");
	fr->r_gui.replace_combo = glade_xml_get_widget (fr->gxml, "find_replace_replace_combo");
	fr->r_gui.replace_entry = glade_xml_get_widget (fr->gxml, "find_replace_replace_entry");
	fr->r_gui.from_begin_radio = glade_xml_get_widget (fr->gxml, "find_replace_whole_document");
	fr->r_gui.from_cur_loc_radio = glade_xml_get_widget (fr->gxml, "find_replace_from_cursor");
	fr->r_gui.forward_radio = glade_xml_get_widget (fr->gxml, "find_replace_forwards");
	fr->r_gui.backward_radio = glade_xml_get_widget (fr->gxml, "find_replace_backwards");
	fr->r_gui.regexp_radio = glade_xml_get_widget (fr->gxml, "find_replace_regexp");
	fr->r_gui.string_radio = glade_xml_get_widget (fr->gxml, "find_replace_string");
	fr->r_gui.ignore_case_check = glade_xml_get_widget (fr->gxml, "find_replace_ignore_case");
	fr->r_gui.replace_prompt_check = glade_xml_get_widget (fr->gxml, "find_replace_prompt_replace");
	fr->r_gui.whole_word_check = glade_xml_get_widget (fr->gxml, "find_replace_whole_word");
	
	gtk_combo_disable_activate (GTK_COMBO (fr->r_gui.find_combo));
	gtk_combo_disable_activate (GTK_COMBO (fr->r_gui.replace_combo));
	
	g_signal_connect (G_OBJECT (fr->r_gui.GUI), "response",
	                  G_CALLBACK (on_replace_dialog_response), fr);
	g_signal_connect (G_OBJECT (fr->r_gui.GUI), "delete_event",
	                  G_CALLBACK (on_replace_dialog_response), fr);

	gtk_widget_grab_focus (fr->r_gui.find_entry);
}

static GtkWidget*
create_replace_messagebox (FindAndReplace *fr)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new (GTK_WINDOW (fr->r_gui.GUI), 
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_NONE,
									 _("Do you want to replace this?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							GTK_STOCK_NO, GTK_RESPONSE_NO,
							GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	return dialog;
}

static void
on_replace_dialog_response (GtkDialog *dialog, gint response,
                            gpointer user_data)
{
	TextEditor *te;
	const gchar *f_string, *r_string;
	gchar buff[256];
	FindAndReplace *fr = user_data;
	gint ret, count;

	if (response == GTK_RESPONSE_HELP)
		return;
	find_replace_hide(fr);
	if (response == GTK_RESPONSE_CLOSE)
		return;

	te = anjuta_get_current_text_editor ();
	if (!te)
		return;
	f_string = gtk_entry_get_text (GTK_ENTRY (fr->r_gui.find_entry));
	r_string = gtk_entry_get_text (GTK_ENTRY (fr->r_gui.replace_entry));

	if (!f_string || strlen (f_string) < 1)
		return;
	switch (fr->find_text->area)
	{
	case TEXT_EDITOR_FIND_SCOPE_WHOLE:
		sprintf (buff,
				 "The string \"%s\" was not found in the current file",
				 f_string);
		break;
	case TEXT_EDITOR_FIND_SCOPE_SELECTION:
		sprintf (buff,
				 _("The string \"%s\" was not found in the selected text"),
				 f_string);
		break;
	default:
		sprintf (buff,
				 _("The string \"%s\" was not found from the current location"),
				 f_string);
		break;
	}
	count = 0;
	while (1)
	{
		/* Only the first search */
		if (count == 0)
		{
			ret = text_editor_find (te, f_string,
									fr->find_text->area,
									fr->find_text->forward,
									fr->find_text->regexp,
									fr->find_text->ignore_case,
									fr->find_text->whole_word);
		}
		else
		{
			ret = text_editor_find (te, f_string,
									TEXT_EDITOR_FIND_SCOPE_CURRENT,
									fr->find_text->forward,
									fr->find_text->regexp,
									fr->find_text->ignore_case,
									fr->find_text->whole_word);
		}
		if (ret < 0)
		{
			if (count == 0)
				anjuta_error (buff);
			return;
		}
		else
		{
			GtkWidget *dialog;
			glong sel_start;
			gint but;
			
			dialog = create_replace_messagebox (fr);
			if (fr->replace_prompt)
				but = gtk_dialog_run (GTK_DIALOG (dialog));
			else
				but = 1;
			gtk_widget_destroy (dialog);
			
			switch (but)
			{
			case GTK_RESPONSE_YES:
				sel_start =
					scintilla_send_message (SCINTILLA (te->widgets.editor),
											SCI_GETSELECTIONSTART, 0, 0);
				text_editor_replace_selection (te, r_string);
				if (!fr->find_text->forward)
					scintilla_send_message (SCINTILLA(te->widgets.editor),
											SCI_SETCURRENTPOS, sel_start, 0);
				break;
			case GTK_RESPONSE_NO:
				break;
			default:
				return;
			}
		}
		count++;
	}
}

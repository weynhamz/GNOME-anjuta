/*
    find_text.c
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
#include "find_text.h"
#include "utilities.h"

static const gchar SecFT [] = {"FindText"};
/*static const gchar SECTION_SEARCH [] =  {"search_text"};*/

static void
on_find_text_dialog_response (GtkDialog *dialog, gint response,
							  gpointer user_data);

static gboolean
on_find_text_delete_event (GtkDialog *dialog, GdkEvent *event,
						   FindText *ft)
{
	find_text_hide (ft);
	return TRUE;
}

static void
on_find_text_start_over (void)
{
	TextEditor *te = anjuta_get_current_text_editor();
	long length;

	length = aneditor_command(te->editor_id, ANE_GETLENGTH, 0, 0);
	
	if (app->find_replace->find_text->forward == TRUE)
		// search from doc start
		aneditor_command (te->editor_id, ANE_GOTOLINE, 0, 0);
	else
		// search from doc end
		aneditor_command (te->editor_id, ANE_GOTOLINE, length, 0);
	on_find_text_dialog_response (NULL, GTK_RESPONSE_OK,
								  app->find_replace->find_text);
}

static gboolean
on_find_text_dialog_key_press (GtkWidget *widget, GdkEventKey *event,
                               gpointer user_data)
{
	if (event->keyval == GDK_Escape)
	{
		if (user_data)
		{
			/* Escape pressed in Find window */
			find_text_hide ((FindText *) user_data);
		}
		else
		{
			/* Escape pressed in wrap yes/no window */
			gtk_dialog_response (GTK_DIALOG (widget), GTK_RESPONSE_NO);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void
on_find_text_dialog_response (GtkDialog *dialog, gint response,
                              gpointer user_data)
{
	TextEditor *te;
	gchar *string;
	const gchar *str;
	gchar buff[512];
	gint ret;
	FindText *ft = user_data;
	gboolean radio0, radio1, state;

	if (response == GTK_RESPONSE_HELP)
		return;
	if (response == GTK_RESPONSE_CLOSE)
	{
		find_text_hide (ft);
		return;
	}

	state =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.ignore_case_check));
	if (state == TRUE)
		ft->ignore_case = TRUE;
	else
		ft->ignore_case = FALSE;

	state =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.whole_word_check));
	if (state == TRUE)
		ft->whole_word = TRUE;
	else
		ft->whole_word = FALSE;


	radio0 =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.from_begin_radio));
	radio1 =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.from_cur_loc_radio));

	if (radio0)
		ft->area = TEXT_EDITOR_FIND_SCOPE_WHOLE;
	else
		ft->area = TEXT_EDITOR_FIND_SCOPE_CURRENT;

	state =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.forward_radio));
	if (state == TRUE)
		ft->forward = TRUE;
	else
		ft->forward = FALSE;

	state =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.regexp_radio));
	if (state == TRUE)
		ft->regexp = TRUE;
	else
		ft->regexp = FALSE;


	te = anjuta_get_current_text_editor ();
	if (!te) return;
	str = gtk_entry_get_text (GTK_ENTRY (ft->f_gui.entry));
	if (!str || strlen (str) < 1)
		return;
	/* Object is going to be destroyed. so, strdup */
	string = g_strdup (str);
	switch (ft->area)
	{
	case TEXT_EDITOR_FIND_SCOPE_WHOLE:
		sprintf (buff,
				 _("The match \"%s\" was not found in the whole document"),
				 string);
		break;
	case TEXT_EDITOR_FIND_SCOPE_SELECTION:
		sprintf (buff,
				 _("The match \"%s\" was not found in the selected text"),
				 string);
		break;
	default:
		sprintf (buff,
				 _("The match \"%s\" was not found from the current location"),
				 string);
		break;
	}
	ret = text_editor_find (te, string, ft->area,
							ft->forward, ft->regexp,
							ft->ignore_case,
							ft->whole_word);

	gtk_entry_set_text(GTK_ENTRY(app->widgets.toolbar.main_toolbar.find_entry),
					   string);

	g_free (string);
	if (ret < 0)
	{
		if (ft->area == TEXT_EDITOR_FIND_SCOPE_CURRENT)
		{
			GtkWidget *dialog;
			// Ask if user wants to wrap around the doc
			dialog = gtk_message_dialog_new (GTK_WINDOW (ft->f_gui.GUI),
											 GTK_DIALOG_DESTROY_WITH_PARENT,
											 GTK_MESSAGE_QUESTION,
											 GTK_BUTTONS_YES_NO,
						 _("No matches. Wrap search around the document?"));
			gtk_dialog_set_default_response (GTK_DIALOG (dialog),
											 GTK_RESPONSE_YES);
			g_signal_connect (G_OBJECT (dialog), "key-press-event",
							  G_CALLBACK (on_find_text_dialog_key_press), NULL);
			if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			{
				on_find_text_start_over ();
			}
			gtk_widget_destroy (dialog);
		}
		else
			anjuta_error (buff);
	}
}

static void
create_find_text_gui (FindText * ft)
{
	ft->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "find_text_dialog", NULL);
	glade_xml_signal_autoconnect (ft->gxml);
	ft->f_gui.GUI = glade_xml_get_widget (ft->gxml, "find_text_dialog");
	gtk_widget_hide (ft->f_gui.GUI);
	gtk_window_set_transient_for (GTK_WINDOW(ft->f_gui.GUI),
                                  GTK_WINDOW(app->widgets.window));
	
	ft->f_gui.combo = glade_xml_get_widget (ft->gxml, "find_text_combo");
	ft->f_gui.entry = glade_xml_get_widget (ft->gxml, "find_text_entry");
	ft->f_gui.from_begin_radio = glade_xml_get_widget (ft->gxml, "find_text_whole_document");
	ft->f_gui.from_cur_loc_radio = glade_xml_get_widget (ft->gxml, "find_text_from_cursor");
	ft->f_gui.forward_radio = glade_xml_get_widget (ft->gxml, "find_text_forwards");
	ft->f_gui.backward_radio = glade_xml_get_widget (ft->gxml, "find_text_backwards");
	ft->f_gui.regexp_radio = glade_xml_get_widget (ft->gxml, "find_text_regexp");
	ft->f_gui.string_radio = glade_xml_get_widget (ft->gxml, "find_text_string");
	ft->f_gui.ignore_case_check = glade_xml_get_widget (ft->gxml, "find_text_ignore_case");
	ft->f_gui.whole_word_check = glade_xml_get_widget (ft->gxml, "find_text_whole_word");
	
	gtk_combo_disable_activate (GTK_COMBO (ft->f_gui.combo));
	
	g_signal_connect (G_OBJECT (ft->f_gui.GUI), "delete_event",
	                  G_CALLBACK (on_find_text_delete_event), ft);
	g_signal_connect (G_OBJECT (ft->f_gui.GUI), "response",
	                  G_CALLBACK (on_find_text_dialog_response), ft);
	g_signal_connect (G_OBJECT (ft->f_gui.GUI), "key-press-event",
	                  G_CALLBACK (on_find_text_dialog_key_press), ft);

	gtk_widget_grab_focus (ft->f_gui.entry);
}

FindText *
find_text_new ()
{
	FindText *ft;
	ft = g_malloc (sizeof (FindText));
	if (ft)
	{
		ft->find_history = NULL;
		/*ft->area = 1;
		ft->forward = TRUE;
		ft->regexp = FALSE;
		ft->ignore_case = FALSE;
		ft->whole_word = FALSE;

		ft->is_showing = FALSE;*/
		
		ft->area		= GetProfileInt( SecFT, "area", 1) ;
		ft->forward		= GetProfileBool( SecFT, "forward", TRUE ) ;
		ft->regexp		= GetProfileBool( SecFT, "regexp", FALSE ) ;
		ft->ignore_case = GetProfileBool( SecFT, "ignore_case", FALSE ) ;
		ft->whole_word	= GetProfileBool( SecFT, "whole_word", FALSE ) ;
		ft->incremental_pos = -1;
		ft->incremental_wrap = FALSE;

		ft->is_showing = FALSE;
		
		ft->pos_x = 100;
		ft->pos_y = 100;
		create_find_text_gui (ft);
	}
	return ft;
}

void
find_text_save_settings (FindText * ft)
{
	if (ft)
	{
		/* Passivation */
		WriteProfileInt( SecFT, "area", ft->area ) ;
		WriteProfileBool( SecFT, "forward", ft->forward ) ;
		WriteProfileBool( SecFT, "regexp", ft->regexp ) ;
		WriteProfileBool( SecFT, "ignore_case", ft->ignore_case ) ;
		WriteProfileBool( SecFT, "whole_word", ft->whole_word ) ;
	}
}


void
find_text_destroy (FindText * ft)
{
	gint i;
	if (ft)
	{
		gtk_widget_unref (ft->f_gui.GUI);
		gtk_widget_unref (ft->f_gui.combo);
		gtk_widget_unref (ft->f_gui.entry);
		gtk_widget_unref (ft->f_gui.from_cur_loc_radio);
		gtk_widget_unref (ft->f_gui.from_begin_radio);
		gtk_widget_unref (ft->f_gui.forward_radio);
		gtk_widget_unref (ft->f_gui.backward_radio);
		gtk_widget_unref (ft->f_gui.regexp_radio);
		gtk_widget_unref (ft->f_gui.string_radio);
		gtk_widget_unref (ft->f_gui.ignore_case_check);
		gtk_widget_unref (ft->f_gui.whole_word_check);
		if (ft->f_gui.GUI)
			gtk_widget_destroy (ft->f_gui.GUI);
		for (i = 0; i < g_list_length (ft->find_history); i++)
			g_free (g_list_nth (ft->find_history, i)->data);

		if (ft->find_history)
			g_list_free (ft->find_history);
		g_object_unref (ft->gxml);
		g_free (ft);
		ft = NULL;
	}
}

void
find_text_load_session( FindText * ft, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != ft );

	ft->find_history = session_load_strings(p,
											SECSTR(SECTION_FINDTEXT),
											ft->find_history );
}

void
find_text_show (FindText * ft)
{
	if (ft->find_history)
		gtk_combo_set_popdown_strings (GTK_COMBO (ft->f_gui.combo),
					       ft->find_history);
	switch (ft->area)
	{
	case TEXT_EDITOR_FIND_SCOPE_WHOLE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.from_begin_radio),
					      TRUE);
		break;
	default:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.from_cur_loc_radio),
					      TRUE);
	}
	if (ft->forward)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.forward_radio),
					      TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.backward_radio),
					      TRUE);

	if (ft->regexp)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.regexp_radio), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (ft->f_gui.string_radio), TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (ft->f_gui.ignore_case_check),
				      ft->ignore_case);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (ft->f_gui.whole_word_check),
				      ft->whole_word);

	gtk_widget_grab_focus (ft->f_gui.entry);
	gtk_dialog_set_default_response (GTK_DIALOG (ft->f_gui.GUI),
									 GTK_RESPONSE_OK);
	entry_set_text_n_select (ft->f_gui.entry, NULL, TRUE);
	if (ft->is_showing)
	{
		gtk_widget_show (ft->f_gui.GUI);
		gdk_window_raise (ft->f_gui.GUI->window);
		return;
	}
	gtk_widget_set_uposition (ft->f_gui.GUI, ft->pos_x, ft->pos_y);
	gtk_widget_show (ft->f_gui.GUI);
	ft->is_showing = TRUE;
}

void
find_text_hide (FindText * ft)
{
	if (!ft)
		return;
	if (ft->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (ft->f_gui.GUI->window, &ft->pos_x,
				    &ft->pos_y);
	gtk_widget_hide (ft->f_gui.GUI);
	ft->is_showing = FALSE;


	ft->find_history = update_string_list (ft->find_history,
					       gtk_entry_get_text (GTK_ENTRY
								   (ft->f_gui.
								    entry)),
					       COMBO_LIST_LENGTH);
	if (ft->find_history)
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (app->widgets.toolbar.
						main_toolbar.find_combo),
					       ft->find_history);
	/*if( app->project_dbase )
		find_text_save_project( ft, app->project_dbase );*/
}

gboolean 
find_text_save_session ( FindText * ft, ProjectDBase *p )
{
	g_return_val_if_fail( NULL != p, FALSE );
	session_save_strings( p, SECSTR(SECTION_FINDTEXT), ft->find_history );
	/*session_clear_section( p, SECTION_SEARCH );
	if( ft->find_history )
	{
		const gchar *szFnd ;
		int 		i;
		int			nLen = g_list_length (ft->find_history); 
		for (i = 0; i < nLen ; i++)
		{
			szFnd = (gchar*)g_list_nth (ft->find_history, i)->data ;
			if( szFnd && szFnd[0] )
				session_save_string_n( p, SECTION_SEARCH, i, szFnd );
		}
	}*/
	return TRUE;
}

void
enter_selection_as_search_target(void)
{
	gchar *selectionText = NULL;
	
	selectionText = anjuta_get_current_selection ();
	if (selectionText != NULL && selectionText[0] != '\0')
	{
		GList   *updatedHistory;
		updatedHistory =
			update_string_list (app->find_replace->find_text->find_history,
								selectionText, COMBO_LIST_LENGTH);
		app->find_replace->find_text->find_history = updatedHistory;
		entry_set_text_n_select (app->widgets.toolbar.main_toolbar.find_entry,
								 selectionText, FALSE);
	}
	
	if (selectionText != NULL)
		g_free (selectionText);
}

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

gboolean
on_find_text_close (GtkWidget * widget,
			   gpointer user_data);
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
		g_free (ft);
		ft = NULL;
	}
}

void
find_text_load_session( FindText * ft, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != ft );

	ft->find_history = session_load_strings( p, SECSTR(SECTION_FINDTEXT), ft->find_history );
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
	gnome_dialog_set_default (GNOME_DIALOG (ft->f_gui.GUI), 3);
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
create_find_text_gui (FindText * ft)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *frame1;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *frame2;
	GtkWidget *table1;
	GtkWidget *frame3;
	GtkWidget *vbox1;
	GSList *vbox1_group = NULL;
	GtkWidget *radiobutton1;
	GtkWidget *radiobutton2;
	GtkWidget *frame4;
	GtkWidget *vbox2;
	GSList *vbox2_group = NULL;
	GtkWidget *radiobutton4;
	GtkWidget *radiobutton5;
	GtkWidget *frame5;
	GtkWidget *vbox3;
	GSList *vbox3_group = NULL;
	GtkWidget *radiobutton6;
	GtkWidget *radiobutton7;
	GtkWidget *checkbutton1;
	GtkWidget *checkbutton2;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;

	dialog1 = gnome_dialog_new (_("Find"), NULL);
	gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "find", "Anjuta");
	gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);

	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);

	frame1 = gtk_frame_new (_(" RegExp/String to search "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame1, TRUE, TRUE, 0);

	combo1 = gtk_combo_new ();
	gtk_widget_show (combo1);
	gtk_container_add (GTK_CONTAINER (frame1), combo1);
	gtk_container_set_border_width (GTK_CONTAINER (combo1), 5);
	gtk_combo_disable_activate (GTK_COMBO (combo1));
	gtk_combo_set_case_sensitive    (GTK_COMBO (combo1), TRUE);
	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_show (combo_entry1);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame2, TRUE, TRUE, 0);

	table1 = gtk_table_new (2, 3, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame2), table1);

	frame3 = gtk_frame_new (_(" Scope "));
	gtk_widget_show (frame3);
	gtk_table_attach (GTK_TABLE (table1), frame3, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 3);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame3), vbox1);

	radiobutton1 =
		gtk_radio_button_new_with_label (vbox1_group,
						 _("Whole document"));
	vbox1_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
	gtk_widget_show (radiobutton1);

	radiobutton2 =
		gtk_radio_button_new_with_label (vbox1_group,
						 _("From cursor"));
	vbox1_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
	gtk_widget_show (radiobutton2);
	gtk_box_pack_start (GTK_BOX (vbox1), radiobutton2, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), radiobutton1, FALSE, TRUE, 0);

	frame4 = gtk_frame_new (_(" Direction "));
	gtk_widget_show (frame4);
	gtk_table_attach (GTK_TABLE (table1), frame4, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame4), 3);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame4), vbox2);

	radiobutton4 =
		gtk_radio_button_new_with_label (vbox2_group, _("Forwards"));
	vbox2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
	gtk_widget_show (radiobutton4);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton4, FALSE, TRUE, 0);

	radiobutton5 =
		gtk_radio_button_new_with_label (vbox2_group, _("Backwards"));
	vbox2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton5));
	gtk_widget_show (radiobutton5);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton5, FALSE, TRUE, 0);

	frame5 = gtk_frame_new (_("Type"));
	gtk_widget_show (frame5);
	gtk_table_attach (GTK_TABLE (table1), frame5, 2, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame5), 3);

	vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame5), vbox3);

	radiobutton6 =
		gtk_radio_button_new_with_label (vbox3_group, _("RegExp"));
	vbox3_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton6));
	gtk_widget_show (radiobutton6);
	gtk_box_pack_start (GTK_BOX (vbox3), radiobutton6, FALSE, TRUE, 0);

	radiobutton7 =
		gtk_radio_button_new_with_label (vbox3_group, _("String"));
	vbox3_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton7));
	gtk_widget_show (radiobutton7);
	gtk_box_pack_start (GTK_BOX (vbox3), radiobutton7, FALSE, TRUE, 0);

	checkbutton1 =
		gtk_check_button_new_with_label (_
						 ("Ignore case while searching"));
	gtk_widget_show (checkbutton1);
	gtk_table_attach (GTK_TABLE (table1), checkbutton1, 1, 3, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	checkbutton2 =
		gtk_check_button_new_with_label (_("Whole word"));
	gtk_widget_show (checkbutton2);
	gtk_table_attach (GTK_TABLE (table1), checkbutton2, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_HELP);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CLOSE);
	button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button2);
	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    _("Find"));
	button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (dialog1));

	gtk_signal_connect (GTK_OBJECT (dialog1), "close",
			    GTK_SIGNAL_FUNC (on_find_text_close), ft);
	gtk_signal_connect (GTK_OBJECT (combo_entry1), "activate",
			    GTK_SIGNAL_FUNC (on_find_text_ok_clicked), ft);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_find_text_help_clicked), ft);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_find_text_cancel_clicked),
			    ft);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_find_text_ok_clicked), ft);

	ft->f_gui.GUI = dialog1;
	ft->f_gui.combo = combo1;
	ft->f_gui.entry = combo_entry1;
	ft->f_gui.from_begin_radio = radiobutton1;
	ft->f_gui.from_cur_loc_radio = radiobutton2;
	ft->f_gui.forward_radio = radiobutton4;
	ft->f_gui.backward_radio = radiobutton5;
	ft->f_gui.regexp_radio = radiobutton6;
	ft->f_gui.string_radio = radiobutton7;
	ft->f_gui.ignore_case_check = checkbutton1;
	ft->f_gui.whole_word_check = checkbutton2;

	gtk_widget_ref (ft->f_gui.GUI);
	gtk_widget_ref (ft->f_gui.combo);
	gtk_widget_ref (ft->f_gui.entry);
	gtk_widget_ref (ft->f_gui.from_cur_loc_radio);
	gtk_widget_ref (ft->f_gui.from_begin_radio);
	gtk_widget_ref (ft->f_gui.forward_radio);
	gtk_widget_ref (ft->f_gui.backward_radio);
	gtk_widget_ref (ft->f_gui.regexp_radio);
	gtk_widget_ref (ft->f_gui.string_radio);
	gtk_widget_ref (ft->f_gui.ignore_case_check);
	gtk_widget_ref (ft->f_gui.whole_word_check);

	gtk_widget_grab_focus (combo_entry1);
	gtk_window_set_transient_for(GTK_WINDOW(ft->f_gui.GUI), GTK_WINDOW(app->widgets.window));
}

void
on_find_text_ok_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	gchar *string, *str;
	gchar buff[512];
	gint ret;
	FindText *ft = user_data;
	gboolean radio0, radio1, state;

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
		sprintf (buff, _("The match \"%s\" was not found in the whole document"), string);
		break;
	case TEXT_EDITOR_FIND_SCOPE_SELECTION:
		sprintf (buff, _("The match \"%s\" was not found in the selected text"), string);
		break;
	default:
		sprintf (buff, _("The match \"%s\" was not found from the current location"), string);
		break;
	}
	ret = text_editor_find (te, string, ft->area, ft->forward, ft->regexp, ft->ignore_case, ft->whole_word);

	gtk_entry_set_text(GTK_ENTRY(app->widgets.toolbar.main_toolbar.find_entry), string);

	g_free (string);
	if (ret < 0)
	{
		if (ft->area == TEXT_EDITOR_FIND_SCOPE_CURRENT)
		{
			// Ask if user wants to wrap around the doc
			messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
					_("No matches. Wrap search around the document?"),
					GNOME_STOCK_BUTTON_NO,
					GNOME_STOCK_BUTTON_YES,
					GTK_SIGNAL_FUNC(on_find_text_cancel_clicked),
					GTK_SIGNAL_FUNC(on_find_text_start_over), 
					user_data);
		}
		else
			anjuta_error (buff);
	}
}

void
on_find_text_cancel_clicked (GtkButton * button, gpointer user_data)
{
	FindText *ft = user_data;
	if (NULL != ft)
		gnome_dialog_close(GNOME_DIALOG(ft->f_gui.GUI));
}

void
on_find_text_help_clicked (GtkButton * button, gpointer user_data)
{
}

gboolean
on_find_text_close (GtkWidget * widget,
			   gpointer user_data)
{
	FindText *ft = (FindText *) user_data;
	find_text_hide (ft);
	return FALSE;
}

void
on_find_text_start_over (GtkButton * button, gpointer user_data)
{
	TextEditor *te = anjuta_get_current_text_editor();
	long length;

	length = aneditor_command(te->editor_id, ANE_GETLENGTH, 0, 0);
	
	if (app->find_replace->find_text->forward == TRUE)		
		aneditor_command (te->editor_id, ANE_GOTOLINE, 0, 0); // search from doc start
	else
		aneditor_command (te->editor_id, ANE_GOTOLINE, length, 0); // search from doc end

	on_find_text_ok_clicked(NULL, app->find_replace->find_text);
}





void
enter_selection_as_search_target(void)
{
gchar *selectionText = NULL;

       selectionText = anjuta_get_current_selection ();
       
       if (selectionText != NULL && selectionText[0] != '\0')
       {
       GList   *updatedHistory;
               
               updatedHistory = update_string_list (app->find_replace->find_text->find_history, selectionText, COMBO_LIST_LENGTH);
               
               app->find_replace->find_text->find_history = updatedHistory;
               
               
               entry_set_text_n_select (app->widgets.toolbar.main_toolbar.find_entry, selectionText, FALSE);
       }
       
       
       if (selectionText != NULL)
       {
               g_free (selectionText);
       }       
}


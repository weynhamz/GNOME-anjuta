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

static GtkWidget *create_replace_messagebox (void);

static void
on_replace_text_ok_clicked (GtkButton * button, gpointer user_data);

static void
on_replace_text_cancel_clicked (GtkButton * button, gpointer user_data);
static void
on_replace_text_help_clicked (GtkButton * button, gpointer user_data);

static gboolean
on_replace_text_delete_event (GtkWidget * widget,
			      GdkEvent * event, gpointer user_data);

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
		gtk_widget_unref (fr->r_gui.GUI);
		gtk_widget_unref (fr->r_gui.find_combo);
		gtk_widget_unref (fr->r_gui.find_entry);
		gtk_widget_unref (fr->r_gui.replace_combo);
		gtk_widget_unref (fr->r_gui.replace_entry);
		gtk_widget_unref (fr->r_gui.from_cur_loc_radio);
		gtk_widget_unref (fr->r_gui.from_begin_radio);
		gtk_widget_unref (fr->r_gui.forward_radio);
		gtk_widget_unref (fr->r_gui.backward_radio);
		gtk_widget_unref (fr->r_gui.regexp_radio);
		gtk_widget_unref (fr->r_gui.string_radio);
		gtk_widget_unref (fr->r_gui.ignore_case_check);
		gtk_widget_unref (fr->r_gui.whole_word_check);
		gtk_widget_unref (fr->r_gui.replace_prompt_check);

		if (GTK_IS_WIDGET (fr->r_gui.GUI))
			gtk_widget_destroy (fr->r_gui.GUI);
		for (i = 0; i < g_list_length (fr->replace_history); i++)
			g_free (g_list_nth (fr->replace_history, i)->data);
		if (fr->replace_history)
			g_list_free (fr->replace_history);
		g_free (fr);
		fr = NULL;
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
	save_session_strings( p, SECSTR(SECTION_REPLACETEXT), fr->replace_history );
	if( fr->find_text )
		find_text_save_session ( fr->find_text, p );
}

void
find_replace_load_session ( FindAndReplace * fr, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != fr );
	fr->replace_history	= load_session_strings( p, SECSTR(SECTION_REPLACETEXT), fr->replace_history );
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
	gnome_dialog_set_default (GNOME_DIALOG (fr->r_gui.GUI), 3);
	entry_set_text_n_select (fr->r_gui.find_entry, NULL, TRUE);
	entry_set_text_n_select (fr->r_gui.replace_entry, NULL, FALSE);

	if (fr->is_showing)
	{
		gtk_widget_show (fr->r_gui.GUI->window);
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
	GtkWidget *dialog2;
	GtkWidget *dialog_vbox2;
	GtkWidget *frame8;
	GtkWidget *combo2;
	GtkWidget *combo3;
	GtkWidget *combo_entry2;
	GtkWidget *combo_entry3;
	GtkWidget *frame9;
	GtkWidget *frame10;
	GtkWidget *table2;
	GtkWidget *frame11;
	GtkWidget *vbox5;
	GSList *vbox5_group = NULL;
	GtkWidget *radiobutton15;
	GtkWidget *radiobutton16;
	GtkWidget *frame12;
	GtkWidget *vbox6;
	GSList *vbox6_group = NULL;
	GtkWidget *radiobutton18;
	GtkWidget *radiobutton19;
	GtkWidget *frame13;
	GtkWidget *vbox7;
	GSList *vbox7_group = NULL;
	GtkWidget *radiobutton20;
	GtkWidget *radiobutton21;
	GtkWidget *hseparator1;
	GtkWidget *checkbutton2;
	GtkWidget *checkbutton3;
	GtkWidget *checkbutton4;
	GtkWidget *dialog_action_area3;
	GtkWidget *button9;
	GtkWidget *button10;
	GtkWidget *button11;

	dialog2 = gnome_dialog_new (_("Find and Replace"), NULL);
	gtk_window_set_policy (GTK_WINDOW (dialog2), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog2), "find_repl", "Anjuta");
	gnome_dialog_close_hides (GNOME_DIALOG (dialog2), TRUE);

	dialog_vbox2 = GNOME_DIALOG (dialog2)->vbox;
	gtk_widget_show (dialog_vbox2);

	frame8 = gtk_frame_new (_(" RegExp/String to search "));
	gtk_widget_show (frame8);
	gtk_box_pack_start (GTK_BOX (dialog_vbox2), frame8, TRUE, TRUE, 0);

	combo2 = gtk_combo_new ();
	gtk_widget_show (combo2);
	gtk_combo_set_case_sensitive    (GTK_COMBO (combo2), TRUE);
	gtk_combo_disable_activate (GTK_COMBO (combo2));
	gtk_container_add (GTK_CONTAINER (frame8), combo2);
	gtk_container_set_border_width (GTK_CONTAINER (combo2), 5);

	combo_entry2 = GTK_COMBO (combo2)->entry;
	gtk_widget_show (combo_entry2);

	frame9 = gtk_frame_new (_(" String to replace "));
	gtk_widget_show (frame9);
	gtk_box_pack_start (GTK_BOX (dialog_vbox2), frame9, TRUE, TRUE, 0);

	combo3 = gtk_combo_new ();
	gtk_widget_show (combo3);
	gtk_combo_set_case_sensitive    (GTK_COMBO (combo3), TRUE);
	gtk_combo_disable_activate (GTK_COMBO (combo3));
	gtk_container_add (GTK_CONTAINER (frame9), combo3);
	gtk_container_set_border_width (GTK_CONTAINER (combo3), 5);
	gtk_combo_disable_activate (GTK_COMBO (combo3));

	combo_entry3 = GTK_COMBO (combo3)->entry;
	gtk_widget_show (combo_entry3);

	frame10 = gtk_frame_new (NULL);
	gtk_widget_show (frame10);
	gtk_box_pack_start (GTK_BOX (dialog_vbox2), frame10, TRUE, TRUE, 0);

	table2 = gtk_table_new (4, 3, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame10), table2);

	frame11 = gtk_frame_new (_(" Scope "));
	gtk_widget_show (frame11);
	gtk_table_attach (GTK_TABLE (table2), frame11, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame11), 3);

	vbox5 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox5);
	gtk_container_add (GTK_CONTAINER (frame11), vbox5);

	radiobutton15 =
		gtk_radio_button_new_with_label (vbox5_group,
						 _("Whole Document"));
	vbox5_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton15));
	gtk_widget_show (radiobutton15);
	gtk_box_pack_start (GTK_BOX (vbox5), radiobutton15, FALSE, TRUE, 0);

	radiobutton16 =
		gtk_radio_button_new_with_label (vbox5_group,
						 _("From Cursor"));
	vbox5_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton16));
	gtk_widget_show (radiobutton16);
	gtk_box_pack_start (GTK_BOX (vbox5), radiobutton16, FALSE, TRUE, 0);

	frame12 = gtk_frame_new (_(" Direction "));
	gtk_widget_show (frame12);
	gtk_table_attach (GTK_TABLE (table2), frame12, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame12), 3);

	vbox6 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox6);
	gtk_container_add (GTK_CONTAINER (frame12), vbox6);

	radiobutton18 =
		gtk_radio_button_new_with_label (vbox6_group, _("Forward"));
	vbox6_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton18));
	gtk_widget_show (radiobutton18);
	gtk_box_pack_start (GTK_BOX (vbox6), radiobutton18, FALSE, TRUE, 0);

	radiobutton19 =
		gtk_radio_button_new_with_label (vbox6_group, _("Backward"));
	vbox6_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton19));
	gtk_widget_show (radiobutton19);
	gtk_box_pack_start (GTK_BOX (vbox6), radiobutton19, FALSE, TRUE, 0);

	frame13 = gtk_frame_new (_("Type"));
	gtk_widget_show (frame13);
	gtk_table_attach (GTK_TABLE (table2), frame13, 2, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame13), 3);

	vbox7 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox7);
	gtk_container_add (GTK_CONTAINER (frame13), vbox7);

	radiobutton20 =
		gtk_radio_button_new_with_label (vbox7_group, _("RegExp"));
	vbox7_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton20));
	gtk_widget_show (radiobutton20);
	gtk_box_pack_start (GTK_BOX (vbox7), radiobutton20, FALSE, TRUE, 0);

	radiobutton21 =
		gtk_radio_button_new_with_label (vbox7_group, _("String"));
	vbox7_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton21));
	gtk_widget_show (radiobutton21);
	gtk_box_pack_start (GTK_BOX (vbox7), radiobutton21, FALSE, TRUE, 0);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_table_attach (GTK_TABLE (table2), hseparator1, 0, 3, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

	checkbutton2 =
		gtk_check_button_new_with_label (_
						 ("Ignore case while searching"));
	gtk_widget_show (checkbutton2);
	gtk_table_attach (GTK_TABLE (table2), checkbutton2, 1, 3, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	checkbutton3 =
		gtk_check_button_new_with_label (_("Prompt before Replace"));
	gtk_widget_show (checkbutton3);
	gtk_table_attach (GTK_TABLE (table2), checkbutton3, 0, 3, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	checkbutton4 =
		gtk_check_button_new_with_label (_("Whole word"));
	gtk_widget_show (checkbutton4);
	gtk_table_attach (GTK_TABLE (table2), checkbutton4, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	dialog_action_area3 = GNOME_DIALOG (dialog2)->action_area;
	gtk_widget_show (dialog_action_area3);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area3),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area3), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog2),
				    GNOME_STOCK_BUTTON_HELP);
	button9 = g_list_last (GNOME_DIALOG (dialog2)->buttons)->data;
	gtk_widget_show (button9);
	GTK_WIDGET_SET_FLAGS (button9, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog2),
				    GNOME_STOCK_BUTTON_CANCEL);
	button10 = g_list_last (GNOME_DIALOG (dialog2)->buttons)->data;
	gtk_widget_show (button10);
	GTK_WIDGET_SET_FLAGS (button10, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog2),
				    GNOME_STOCK_BUTTON_OK);
	button11 = g_list_last (GNOME_DIALOG (dialog2)->buttons)->data;
	gtk_widget_show (button11);
	GTK_WIDGET_SET_FLAGS (button11, GTK_CAN_DEFAULT);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (dialog2));


	gtk_signal_connect (GTK_OBJECT (dialog2), "delete_event",
			    GTK_SIGNAL_FUNC (on_replace_text_delete_event),
			    fr);
	gtk_signal_connect (GTK_OBJECT (combo_entry2), "activate",
			    GTK_SIGNAL_FUNC (on_replace_text_ok_clicked), fr);
	gtk_signal_connect (GTK_OBJECT (combo_entry3), "activate",
			    GTK_SIGNAL_FUNC (on_replace_text_ok_clicked), fr);

	gtk_signal_connect (GTK_OBJECT (button9), "clicked",
			    GTK_SIGNAL_FUNC (on_replace_text_help_clicked),
			    fr);
	gtk_signal_connect (GTK_OBJECT (button10), "clicked",
			    GTK_SIGNAL_FUNC (on_replace_text_cancel_clicked),
			    fr);
	gtk_signal_connect (GTK_OBJECT (button11), "clicked",
			    GTK_SIGNAL_FUNC (on_replace_text_ok_clicked), fr);

	fr->r_gui.GUI = dialog2;
	fr->r_gui.find_combo = combo2;
	fr->r_gui.find_entry = combo_entry2;
	fr->r_gui.replace_combo = combo3;
	fr->r_gui.replace_entry = combo_entry3;
	fr->r_gui.from_begin_radio = radiobutton15;
	fr->r_gui.from_cur_loc_radio = radiobutton16;
	fr->r_gui.forward_radio = radiobutton18;
	fr->r_gui.backward_radio = radiobutton19;
	fr->r_gui.regexp_radio = radiobutton20;
	fr->r_gui.string_radio = radiobutton21;
	fr->r_gui.ignore_case_check = checkbutton2;
	fr->r_gui.replace_prompt_check = checkbutton3;
	fr->r_gui.whole_word_check = checkbutton4;

	gtk_widget_ref (fr->r_gui.GUI);
	gtk_widget_ref (fr->r_gui.find_combo);
	gtk_widget_ref (fr->r_gui.find_entry);
	gtk_widget_ref (fr->r_gui.replace_combo);
	gtk_widget_ref (fr->r_gui.replace_entry);
	gtk_widget_ref (fr->r_gui.from_cur_loc_radio);
	gtk_widget_ref (fr->r_gui.from_begin_radio);
	gtk_widget_ref (fr->r_gui.forward_radio);
	gtk_widget_ref (fr->r_gui.backward_radio);
	gtk_widget_ref (fr->r_gui.regexp_radio);
	gtk_widget_ref (fr->r_gui.string_radio);
	gtk_widget_ref (fr->r_gui.ignore_case_check);
	gtk_widget_ref (fr->r_gui.replace_prompt_check);
	gtk_widget_ref (fr->r_gui.whole_word_check);

	gtk_widget_grab_focus (combo_entry2);
}

static GtkWidget *
create_replace_messagebox ()
{
	GtkWidget *replace_mesgbox;
	GtkWidget *dialog_vbox9;
	GtkWidget *replace_prompt_yes;
	GtkWidget *replace_prompt_no;
	GtkWidget *replace_prompt_cancel;
	GtkWidget *dialog_action_area9;

	replace_mesgbox =
		gnome_message_box_new (_("Do you want to replace this?"),
				       GNOME_MESSAGE_BOX_QUESTION, NULL);
	gtk_window_set_policy (GTK_WINDOW (replace_mesgbox), FALSE, FALSE,
			       FALSE);

	dialog_vbox9 = GNOME_DIALOG (replace_mesgbox)->vbox;
	gtk_widget_show (dialog_vbox9);

	gnome_dialog_append_button (GNOME_DIALOG (replace_mesgbox),
				    GNOME_STOCK_BUTTON_YES);
	replace_prompt_yes =
		g_list_last (GNOME_DIALOG (replace_mesgbox)->buttons)->data;
	gtk_widget_show (replace_prompt_yes);
	GTK_WIDGET_SET_FLAGS (replace_prompt_yes, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (replace_mesgbox),
				    GNOME_STOCK_BUTTON_NO);
	replace_prompt_no =
		g_list_last (GNOME_DIALOG (replace_mesgbox)->buttons)->data;
	gtk_widget_show (replace_prompt_no);
	GTK_WIDGET_SET_FLAGS (replace_prompt_no, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (replace_mesgbox),
				    GNOME_STOCK_BUTTON_CANCEL);
	replace_prompt_cancel =
		g_list_last (GNOME_DIALOG (replace_mesgbox)->buttons)->data;
	gtk_widget_show (replace_prompt_cancel);
	GTK_WIDGET_SET_FLAGS (replace_prompt_cancel, GTK_CAN_DEFAULT);

	dialog_action_area9 = GNOME_DIALOG (replace_mesgbox)->action_area;
	return replace_mesgbox;
}

static void
on_replace_text_ok_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	gchar *f_string, *r_string;
	gchar buff[256];
	FindAndReplace *fr = user_data;
	gint ret, count;

	find_replace_hide (fr);
/*	update_gtk ();*/
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
		sprintf (buff, "The string \"%s\" was not found in the current file", f_string);
		break;
	case TEXT_EDITOR_FIND_SCOPE_SELECTION:
		sprintf (buff, _("The string \"%s\" was not found in the selected text"), f_string);
		break;
	default:
		sprintf (buff, _("The string \"%s\" was not found from the current location"), f_string);
		break;
	}
	gtk_widget_hide (fr->r_gui.GUI);
	count = 0;
	while (1)
	{
		/* Only the first search */
		if (count == 0)
		{
			ret = text_editor_find (te, f_string, fr->find_text->area, fr->find_text->forward,
				fr->find_text->regexp, fr->find_text->ignore_case, fr->find_text->whole_word);
		}
		else
		{
			ret = text_editor_find (te, f_string, TEXT_EDITOR_FIND_SCOPE_CURRENT,
				fr->find_text->forward,
				fr->find_text->regexp, fr->find_text->ignore_case, fr->find_text->whole_word);
		}
		if (ret < 0)
		{
			if (count == 0)
				anjuta_error (buff);
			return;
		}
		else
		{
			glong sel_start;
			gint but;
			
			if (fr->replace_prompt)
				but = gnome_dialog_run (GNOME_DIALOG (create_replace_messagebox ()));
			else
				but = 0;
			switch (but)
			{
			case 0:
				sel_start = scintilla_send_message (SCINTILLA(te->widgets.editor), SCI_GETSELECTIONSTART, 0, 0);
				text_editor_replace_selection (te, r_string);
				if (!fr->find_text->forward)
					scintilla_send_message (SCINTILLA(te->widgets.editor), SCI_SETCURRENTPOS, sel_start, 0);
				break;
			case 1:
				break;
			default:
				return;
			}
		}
		count++;
	}
}

static void
on_replace_text_cancel_clicked (GtkButton * button, gpointer user_data)
{
	find_replace_hide ((FindAndReplace *) user_data);
}

static void
on_replace_text_help_clicked (GtkButton * button, gpointer user_data)
{

}

static gboolean
on_replace_text_delete_event (GtkWidget * widget,
			      GdkEvent * event, gpointer user_data)
{
	FindAndReplace *fr = (FindAndReplace *) user_data;
	find_replace_hide (fr);
	return TRUE;
}

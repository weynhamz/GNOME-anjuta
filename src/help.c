 /*
  * help.c
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
#	include <config.h>
#endif


#include "anjuta.h"
#include "resources.h"
#include "utilities.h"

static void create_anjuta_help_gui (AnjutaHelp* help);

AnjutaHelp* anjuta_help_new(void)
{
	AnjutaHelp* help;
	help = g_malloc (sizeof(AnjutaHelp));
	if(help)
	{
		help->history = NULL;
		help->is_showing = FALSE;
		create_anjuta_help_gui (help);
	}
	return help;
}

void anjuta_help_destroy(AnjutaHelp* help)
{
	g_return_if_fail (help != NULL);
	gtk_widget_unref(help->widgets.entry);
	gtk_widget_unref(help->widgets.combo);
	gtk_widget_unref(help->widgets.gnome_radio);
	gtk_widget_unref(help->widgets.man_radio);
	gtk_widget_unref(help->widgets.info_radio);
	gtk_widget_destroy (help->widgets.window);
	glist_strings_free(help->history);
	g_free (help);
}

void anjuta_help_show(AnjutaHelp* help)
{
	g_return_if_fail (help != NULL);
	if(help->is_showing)
	{
		gdk_window_raise(help->widgets.window->window);
	}
	else
	{
		gtk_widget_show(help->widgets.window);
	}
	help->is_showing = TRUE;
}

void anjuta_help_hide(AnjutaHelp* help)
{
	g_return_if_fail (help != NULL);
	help->is_showing = FALSE;
}

gboolean anjuta_help_search(AnjutaHelp* help, const gchar* search_word)
{
	if (anjuta_is_installed ("devhelp", TRUE))
	{
		if(search_word)
		{
			help->history =	update_string_list (help->history, search_word, COMBO_LIST_LENGTH);
			gtk_combo_set_popdown_strings (GTK_COMBO(help->widgets.combo), help->history);
		}
		anjuta_res_help_search (search_word);
		return TRUE;
	}
	return FALSE;
}

static void
on_ok_clicked                          (GtkButton       *button,
                                        AnjutaHelp*   help)
{
	gchar* word = gtk_entry_get_text (GTK_ENTRY(help->widgets.entry));
	if(strlen(word)==0) return;
	if (NULL != help)
		gnome_dialog_close(GNOME_DIALOG(help->widgets.window));
	anjuta_help_search(help, word);
}

static void
on_cancel_clicked                      (GtkButton       *button,
                                        AnjutaHelp*   help)
{
	if (NULL != help)
		gnome_dialog_close(GNOME_DIALOG(help->widgets.window));
}

static gboolean
on_close							   (GtkWidget *w,
										AnjutaHelp*  help)
{
	anjuta_help_hide(help);
	return FALSE;
}

static void
create_anjuta_help_gui (AnjutaHelp* help)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *combo1;
  GtkWidget *combo_entry1;
  GtkWidget *hbox1;
  GSList *hbox1_group = NULL;
  GtkWidget *radiobutton1;
  GtkWidget *radiobutton2;
  GtkWidget *radiobutton3;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GtkWidget *button2;

  dialog1 = gnome_dialog_new (_("Search Help"), NULL);
  /* gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window)); */
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
  gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);
  gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_object_set_data (GTK_OBJECT (dialog1), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  combo1 = gtk_combo_new ();
  gtk_widget_show (combo1);
  gtk_combo_disable_activate (GTK_COMBO(combo1));
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), combo1, FALSE, FALSE, 0);

  combo_entry1 = GTK_COMBO (combo1)->entry;
  gtk_widget_show (combo_entry1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);

  radiobutton1 = gtk_radio_button_new_with_label (hbox1_group, _("Gnome API"));
  hbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
  gtk_widget_show (radiobutton1);
  gtk_box_pack_start (GTK_BOX (hbox1), radiobutton1, FALSE, FALSE, 0);

  radiobutton2 = gtk_radio_button_new_with_label (hbox1_group, _("Man pages"));
  hbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
  gtk_widget_show (radiobutton2);
  gtk_box_pack_start (GTK_BOX (hbox1), radiobutton2, FALSE, FALSE, 0);

  radiobutton3 = gtk_radio_button_new_with_label (hbox1_group, _("Info pages"));
  hbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
  gtk_widget_show (radiobutton3);
  gtk_box_pack_start (GTK_BOX (hbox1), radiobutton3, FALSE, FALSE, 0);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button1 = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog1)->buttons)->data);
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
  button2 = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog1)->buttons)->data);
  gtk_widget_show (button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
  
  gnome_dialog_set_default (GNOME_DIALOG(dialog1), 0);
  gnome_dialog_grab_focus (GNOME_DIALOG(dialog1), 0);
  gnome_dialog_editable_enters (GNOME_DIALOG(dialog1),
	  GTK_EDITABLE(combo_entry1));
  gtk_widget_grab_focus (combo_entry1);
  
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_clicked),
                      help);
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
                      GTK_SIGNAL_FUNC (on_cancel_clicked),
                      help);
  gtk_signal_connect (GTK_OBJECT (dialog1), "close",
				  	  GTK_SIGNAL_FUNC (on_close),
					  help);

  help->widgets.window = dialog1;
  help->widgets.entry = combo_entry1;
  help->widgets.combo = combo1;
  help->widgets.gnome_radio = radiobutton1;
  help->widgets.man_radio = radiobutton2;
  help->widgets.info_radio = radiobutton3;
  
  gtk_widget_ref (dialog1);
  gtk_widget_ref (combo_entry1);
  gtk_widget_ref (combo1);
  gtk_widget_ref (radiobutton1);
  gtk_widget_ref (radiobutton2);
  gtk_widget_ref (radiobutton3);
}

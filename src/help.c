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
	g_object_unref (help->gxml);
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
on_response (GtkDialog *dialog, gint response, AnjutaHelp *help)
{
	if (response == GTK_RESPONSE_CANCEL)
	{
		gtk_dialog_response (GTK_DIALOG(help->widgets.window),
							 GTK_RESPONSE_NONE);
	}
	else
	{
		const gchar* word = gtk_entry_get_text (GTK_ENTRY(help->widgets.entry));
		if(strlen(word)==0) return;
		gtk_dialog_response (GTK_DIALOG(help->widgets.window),
							 GTK_RESPONSE_NONE);
		anjuta_help_search(help, word);
	}
}

static gboolean
on_close (GtkWidget *w, AnjutaHelp *help)
{
	anjuta_help_hide(help);
	return FALSE;
}

static void
create_anjuta_help_gui (AnjutaHelp* help)
{
  help->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "help_search_dialog", NULL);
  glade_xml_signal_autoconnect (help->gxml);
  help->widgets.window = glade_xml_get_widget (help->gxml, "help_search_dialog");
  gtk_widget_hide (help->widgets.window);
  gtk_window_set_transient_for (GTK_WINDOW (help->widgets.window),
                                GTK_WINDOW(app->widgets.window));
  help->widgets.entry = glade_xml_get_widget (help->gxml, "help_search_entry");
  help->widgets.combo = glade_xml_get_widget (help->gxml, "help_search_combo");
  help->widgets.gnome_radio = glade_xml_get_widget (help->gxml, "help_search_gnome_api");
  help->widgets.man_radio = glade_xml_get_widget (help->gxml, "help_search_man_pages");
  help->widgets.info_radio = glade_xml_get_widget (help->gxml, "help_search_info_pages");

  g_signal_connect (G_OBJECT (help->widgets.window), "response",
                    G_CALLBACK (on_response), help);
  g_signal_connect (G_OBJECT (help->widgets.window), "close",
				  	G_CALLBACK (on_close), help);
}

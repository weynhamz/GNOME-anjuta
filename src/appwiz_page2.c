/*
    appwiz_page2.c
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>

#include "resources.h"
#include "utilities.h"
#include "appwizard.h"
#include "appwizard_cbs.h"

static void
on_prj_name_entry_changed (GtkEditable * editable, gpointer user_data)
{
	gchar *text;
	gint i;
	AppWizard *aw = user_data;

	text = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));

	if (text)
		for (i = 0; i < strlen (text); i++)
			text[i] = (text[i] == '-') ? '_' : tolower (text[i]);

	gtk_entry_set_text (GTK_ENTRY (aw->widgets.target_entry),
			    text ? text : "");
	string_assign (&aw->target, text);
	g_free (text);
}

static gboolean
on_prj_name_entry_focus_out_event (GtkWidget * widget,
				   GdkEventFocus * event, gpointer user_data)
{
	gchar *temp;
	AppWizard *aw = user_data;
	temp = g_strdup_printf ("%s Version %s", _STR(aw->prj_name), _STR(aw->version));
	gtk_entry_set_text (GTK_ENTRY (aw->widgets.menu_entry_entry), temp);
	gtk_entry_set_text (GTK_ENTRY (aw->widgets.menu_comment_entry), temp);
	g_free (temp);
	return FALSE;
}

static void
on_lang_c_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_C;
}

static void
on_lang_cpp_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_CPP;
}

static void
on_lang_c_cpp_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_C_CPP;
}

void
create_app_wizard_page2 (AppWizard * aw)
{
	gtk_signal_connect (GTK_OBJECT (aw->widgets.prj_name_entry), "changed",
			    GTK_SIGNAL_FUNC (on_prj_name_entry_changed), aw);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.prj_name_entry), "focus_out_event",
			    GTK_SIGNAL_FUNC(on_prj_name_entry_focus_out_event), aw);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.prj_name_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->prj_name);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.target_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->target);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.version_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->version);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.version_entry), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->version);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.author_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->author);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.author_entry), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->author);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.language_c_radio), "toggled",
		      GTK_SIGNAL_FUNC (on_lang_c_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.language_cpp_radio), "toggled",
		      GTK_SIGNAL_FUNC (on_lang_cpp_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.language_c_cpp_radio), "toggled",
		      GTK_SIGNAL_FUNC (on_lang_c_cpp_toggled),
		      aw);

	gtk_widget_ref (aw->widgets.prj_name_entry);
	gtk_widget_ref (aw->widgets.version_entry);
	gtk_widget_ref (aw->widgets.target_entry);
	gtk_widget_ref (aw->widgets.author_entry);
	gtk_widget_ref (aw->widgets.language_c_radio);
	gtk_widget_ref (aw->widgets.language_cpp_radio);
	gtk_widget_ref (aw->widgets.language_c_cpp_radio);
}

/*
    appwiz_page4.c
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

#include <gnome.h>

#include "resources.h"
#include "appwizard.h"
#include "appwizard_cbs.h"

static void
on_gpl_checkbutton_toggled (GtkToggleButton * tb, gpointer user_data)
{
	AppWizard *aw = user_data;
	aw->use_header = gtk_toggle_button_get_active (tb);
}

static void
on_gettext_support_checkbutton_toggled (GtkToggleButton * togglebutton,
					gpointer user_data)
{
	AppWizard *aw;
	aw = user_data;
	aw->gettext_support = gtk_toggle_button_get_active (togglebutton);
}


static void
on_need_term_checkbutton_toggled (GtkToggleButton * tb, gpointer user_data)
{
	AppWizard *aw = user_data;
	aw->need_terminal = gtk_toggle_button_get_active (tb);
}

static void
on_use_glade_checkbutton_toggled (GtkToggleButton * tb, gpointer user_data)
{
	AppWizard *aw = user_data;
	aw->use_glade = gtk_toggle_button_get_active (tb);
}

void
create_app_wizard_page4 (AppWizard * aw)
{
	gtk_signal_connect (GTK_OBJECT (aw->widgets.file_header_check), "toggled",
			    GTK_SIGNAL_FUNC (on_gpl_checkbutton_toggled), aw);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.gettext_support_check), "toggled",
			    GTK_SIGNAL_FUNC(on_gettext_support_checkbutton_toggled), aw);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.menu_entry_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->menu_entry);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.menu_entry_entry), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->menu_entry);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.menu_comment_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->menu_comment);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.menu_comment_entry), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->menu_comment);

	gtk_signal_connect (GTK_OBJECT (aw->widgets.app_group_combo), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->app_group);
	
	gtk_signal_connect (GTK_OBJECT (aw->widgets.term_check), "toggled",
			    GTK_SIGNAL_FUNC(on_need_term_checkbutton_toggled), aw);
	gtk_signal_connect (GTK_OBJECT (aw->widgets.use_glade_check), "toggled",
			    GTK_SIGNAL_FUNC(on_use_glade_checkbutton_toggled), aw);

	gtk_widget_ref (aw->widgets.gettext_support_check);
	gtk_widget_ref (aw->widgets.file_header_check);
	gtk_widget_ref (aw->widgets.use_glade_check);
	gtk_widget_ref (aw->widgets.menu_frame);
	gtk_widget_ref (aw->widgets.menu_entry_entry);
	gtk_widget_ref (aw->widgets.menu_comment_entry);
	gtk_widget_ref (aw->widgets.icon_entry);
	gtk_widget_ref (aw->widgets.app_group_combo);
	gtk_widget_ref (aw->widgets.app_group_entry);
	gtk_widget_ref (aw->widgets.term_check);
}

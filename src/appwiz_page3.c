/*
    appwiz_page3.c
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
#include "pixmaps.h"

#if 0
static void
on_target_exec_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->target_type = PROJECT_TARGET_TYPE_EXECUTABLE;
}

static void
on_target_slib_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->target_type = PROJECT_TARGET_TYPE_STATIC_LIB;
}

static void
on_target_dlib_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->target_type = PROJECT_TARGET_TYPE_DYNAMIC_LIB;
}
#endif

void
create_app_wizard_page3 (AppWizard * aw)
{
#if 0 /* Not implemented yet */
	gtk_signal_connect (GTK_OBJECT (radiobutton1), "toggled",
		      GTK_SIGNAL_FUNC (on_target_exec_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (radiobutton2), "toggled",
		      GTK_SIGNAL_FUNC (on_target_slib_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (radiobutton3), "toggled",
		      GTK_SIGNAL_FUNC (on_target_dlib_toggled),
		      aw);
#endif
	gtk_widget_ref (aw->widgets.description_text);
#if 0	
	aw->widgets.target_exec_radio = radiobutton1;
	gtk_widget_ref (radiobutton1);
	aw->widgets.target_slib_radio = radiobutton2;
	gtk_widget_ref (radiobutton2);
	aw->widgets.target_dlib_radio = radiobutton3;
	gtk_widget_ref (radiobutton3);
#endif
}

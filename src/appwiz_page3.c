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

void
create_app_wizard_page3 (AppWizard * aw)
{
	// Do not get confused, the radiobuttons are on page 2 now but
	// the callbacks are connected here...
	g_signal_connect (G_OBJECT (aw->widgets.target_exec_radio),
			"toggled", G_CALLBACK (on_target_exec_toggled),
		      aw);
	g_signal_connect (G_OBJECT (aw->widgets.target_slib_radio),
			"toggled", G_CALLBACK (on_target_slib_toggled),
		      aw);
	g_signal_connect (G_OBJECT (aw->widgets.target_dlib_radio), 
			"toggled", G_CALLBACK (on_target_dlib_toggled),
		      aw);
	gtk_widget_ref (aw->widgets.description_text);
	gtk_widget_ref (aw->widgets.target_exec_radio);
	gtk_widget_ref (aw->widgets.target_slib_radio);
	gtk_widget_ref (aw->widgets.target_dlib_radio);
}	

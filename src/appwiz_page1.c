/*
    appwiz_page0.c
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
#include "pixmaps.h"
#include "utilities.h"

static void
on_wizard_app_icon_select (GnomeIconList * gil, gint num,
			    GdkEvent * event, gpointer user_data)
{
  AppWizard *aw = user_data;
  switch (num)
  {
  case 0:
    aw->prj_type = PROJECT_TYPE_GENERIC;
    break;
  case 1:
    aw->prj_type = PROJECT_TYPE_GTK2;
    break;
  case 2:
    aw->prj_type = PROJECT_TYPE_GNOME2;
    break;
  case 3:
    aw->prj_type = PROJECT_TYPE_LIBGLADE2;
    break;
  case 4:
    aw->prj_type = PROJECT_TYPE_GTKMM2;
    break;
  case 5:
    aw->prj_type = PROJECT_TYPE_GNOMEMM2;
    break;
  case 6:
    aw->prj_type = PROJECT_TYPE_BONOBO2;
    break;
  case 7:
    aw->prj_type = PROJECT_TYPE_WXWIN;
    break;
  case 8:
    aw->prj_type = PROJECT_TYPE_XWIN;
    break;
  case 9:
    aw->prj_type = PROJECT_TYPE_XWINDOCKAPP;
    break;
 
 default: /* Invalid project type */
    aw->prj_type = PROJECT_TYPE_END_MARK;
    break;
  }
  if (aw->prj_type == PROJECT_TYPE_GTK2 ||
	  aw->prj_type == PROJECT_TYPE_GNOME2 ||
	  aw->prj_type == PROJECT_TYPE_GTKMM2 ||
	  aw->prj_type == PROJECT_TYPE_GNOMEMM2 ||
	  aw->prj_type == PROJECT_TYPE_BONOBO2)
	  gtk_widget_set_sensitive (aw->widgets.use_glade_check, TRUE);
  else
	  gtk_widget_set_sensitive (aw->widgets.use_glade_check, FALSE);
}

void
create_app_wizard_page1 (AppWizard * aw)
{
  gtk_signal_connect (GTK_OBJECT (aw->widgets.icon_list), "select_icon",
		      GTK_SIGNAL_FUNC (on_wizard_app_icon_select), aw);
  gnome_icon_list_select_icon (GNOME_ICON_LIST (aw->widgets.icon_list), 2);
}

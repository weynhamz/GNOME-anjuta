/*
    appwidzard.c
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

#include "anjuta.h"
#include "messagebox.h"
#include "resources.h"
#include "appwidzard.h"

AppWidzard *
app_widzard_new (void)
{
	AppWidzard *aw = g_malloc (sizeof (AppWidzard));
	if (aw)
	{
		gchar* username;
		
		username = getenv ("USERNAME");
		if (!username)
			username = getenv ("USER");
		
		aw->prj_type = PROJECT_TYPE_GNOME;
		aw->target_type = PROJECT_TARGET_TYPE_EXECUTABLE;
		aw->prj_name = NULL;
		aw->target = NULL;
		if (username)
			aw->author = g_strdup (username);
		else
			aw->author = NULL;
		aw->version = g_strdup ("0.1");
		aw->description = NULL;
		aw->gettext_support = TRUE;
		aw->need_terminal = FALSE;
		aw->cur_page = 0;
		aw->menu_entry = NULL;
		aw->menu_comment = NULL;
		aw->icon_file = NULL;
		aw->app_group = g_strdup ("Applications");
		aw->use_header = TRUE;
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_C;

		create_app_widzard_gui (aw);
		create_app_widzard_page1 (aw);
		create_app_widzard_page2 (aw);
		create_app_widzard_page3 (aw);
		create_app_widzard_page4 (aw);
	}
	return aw;
}

void
app_widzard_proceed (void)
{
	AppWidzard *aw;

	aw = app_widzard_new ();
	g_return_if_fail (GTK_IS_WIDGET (aw->widgets.window));
	gtk_widget_show (aw->widgets.window);
}

void
app_widzard_destroy (AppWidzard * aw)
{
	gint i;
	
	g_return_if_fail (aw != NULL);

	gtk_widget_unref (aw->widgets.window);
	gtk_widget_unref (aw->widgets.druid);
	for (i=0; i<6; i++)
	{
		gtk_widget_unref (aw->widgets.page[i]);
	}
	gtk_widget_unref (aw->widgets.prj_name_entry);
	gtk_widget_unref (aw->widgets.author_entry);
	gtk_widget_unref (aw->widgets.version_entry);
	gtk_widget_unref (aw->widgets.target_entry);
	gtk_widget_unref (aw->widgets.description_text);
	gtk_widget_unref (aw->widgets.gettext_support_check);
	gtk_widget_unref (aw->widgets.file_header_check);
	gtk_widget_unref (aw->widgets.menu_frame);
	gtk_widget_unref (aw->widgets.menu_entry_entry);
	gtk_widget_unref (aw->widgets.menu_comment_entry);
	gtk_widget_unref (aw->widgets.icon_entry);
	gtk_widget_unref (aw->widgets.app_group_combo);
	gtk_widget_unref (aw->widgets.app_group_entry);
	gtk_widget_unref (aw->widgets.term_check);

	gtk_widget_unref (aw->widgets.language_c_radio);
	gtk_widget_unref (aw->widgets.language_cpp_radio);
	gtk_widget_unref (aw->widgets.language_c_cpp_radio);
	gtk_widget_unref (aw->widgets.target_exec_radio);
	gtk_widget_unref (aw->widgets.target_slib_radio);
	gtk_widget_unref (aw->widgets.target_dlib_radio);

	if (GTK_IS_WIDGET (aw->widgets.window))
		gtk_widget_destroy (aw->widgets.window);
	string_assign (&aw->prj_name, NULL);
	string_assign (&aw->target, NULL);
	string_assign (&aw->author, NULL);
	string_assign (&aw->version, NULL);

	string_assign (&aw->menu_entry, NULL);
	string_assign (&aw->menu_comment, NULL);
	string_assign (&aw->icon_file, NULL);
	string_assign (&aw->app_group, NULL);
	g_free (aw);
}

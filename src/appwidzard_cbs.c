/*
    appwidzard_cbs.c
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
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>
#include "anjuta.h"
#include "appwidzard.h"
#include "appwidzard_cbs.h"
#include "gnome_project.h"

void
on_aw_text_entry_changed (GtkEditable * editable, gpointer user_data)
{
	gchar *text;
	gchar **strp;

	strp = user_data;
	text = gtk_editable_get_chars (editable, 0, -1);
	string_assign (strp, text);
	if (!text)
		return;
	if (strlen (text) == 0)
		string_assign (strp, NULL);
	string_free (text);
}

void
on_aw_text_entry_realize (GtkWidget * widget, gpointer user_data)
{
	if (!user_data)
		return;
	gtk_entry_set_text (GTK_ENTRY (widget), (gchar *) user_data);
}

void
on_druid1_cancel (GnomeDruid * gnomedruid, gpointer user_data)
{
	app_widzard_destroy ((AppWidzard *) user_data);
}

gboolean
on_druidpagestandard1_next (GnomeDruidPage * gnomedruidpage,
			    gpointer arg1, gpointer user_data)
{
	AppWidzard *aw;
	aw = user_data;
	if ( !(aw->prj_type == PROJECT_TYPE_GENERIC
		|| aw->prj_type == PROJECT_TYPE_GTK))
	{
		gtk_widget_set_sensitive (aw->widgets.menu_frame, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (aw->widgets.menu_frame, FALSE);
	}
	gtk_widget_grab_focus (aw->widgets.prj_name_entry);
	return FALSE;
}

gboolean
on_druidpagestandard2_next (GnomeDruidPage *
			    gnomedruidpage, gpointer arg1, gpointer user_data)
{
	gchar *temp;
	gint i;
	gint error_no;
	AppWidzard *aw;
	gchar buffer[256];
	gchar error_mesg[6][32] = {
		N_("Project name"),
		N_("Source target name"),
		N_("Project Version")
	};

	aw = user_data;
	g_return_val_if_fail (aw != NULL, FALSE);

	error_no = 0;
	/*
	 * Check for valid Project name
	 */
	if (aw->prj_name == NULL)
	{
		error_no = 1;
		goto error_down;
	}
	temp = g_strstrip (aw->prj_name);
	if (strlen (temp) == 0)
	{
		error_no = 1;
		goto error_down;
	}
	if (!isalpha (temp[0]) && temp[0] != '_')
	{
		error_no = 1;
		goto error_down;
	}
	for (i = 0; i < strlen (temp); i++)
	{
		if (!isalpha (temp[i]) && !isdigit (temp[i])
		    && temp[i] != '_')
		{
			error_no = 1;
			goto error_down;
		}
	}
	if (error_no)
		goto error_down;
	/*
	 * Check for valid Version
	 */
	if (aw->version == NULL)
	{
		error_no = 6;
		goto error_down;
	}
	temp = g_strstrip (aw->version);
	if (strlen (temp) == 0)
	{
		error_no = 6;
		goto error_down;
	}
	for (i = 0; i < strlen (temp); i++)
	{
		if (isspace (temp[i]))
		{
			error_no = 6;
			goto error_down;
		}
	}
	if (error_no)
		goto error_down;
	/*
	 * Check for valid program name
	 */
	if (aw->target == NULL)
	{
		error_no = 2;
		goto error_down;
	}
	temp = g_strstrip (aw->target);
	if (strlen (temp) == 0)
	{
		error_no = 2;
		goto error_down;
	}
	if (!isalpha (temp[0]) && temp[0] != '_')
	{
		error_no = 2;
		goto error_down;
	}
	for (i = 0; i < strlen (temp); i++)
	{
		if (!isalpha (temp[i]) && !isdigit (temp[i])
		    && temp[i] != '_')
		{
			error_no = 2;
			goto error_down;
		}
	}

error_down:
	if (error_no)
	{
		sprintf (buffer,
			 _
			 ("Invalid %s field:\nYou must supply a valid %s field to proceed further."),
			 _(error_mesg[error_no - 1]),
			 _(error_mesg[error_no - 1]));
		anjuta_error (buffer);
		return TRUE;
	}
	return FALSE;
}

gboolean
on_druidpagestandard3_next (GnomeDruidPage *
			    gnomedruidpage, gpointer arg1, gpointer user_data)
{
	AppWidzard *aw;
	aw = user_data;


	string_free (aw->description);
	aw->description = gtk_editable_get_chars (GTK_EDITABLE (aw->widgets.description_text), 0, -1);
	return FALSE;
}

gboolean
on_druidpagestandard4_next (GnomeDruidPage *
			    gnomedruidpage, gpointer arg1, gpointer user_data)
{
	AppWidzard *aw;
	gchar *text, *gt_support, *icon;
	aw = user_data;
	icon =
		gnome_icon_entry_get_filename (GNOME_ICON_ENTRY
					       (aw->widgets.icon_entry));
	if (icon == NULL)
	{
		if (aw->icon_file)
		{
			g_free (aw->icon_file);
			aw->icon_file = NULL;
		}
	}
	else
	{
		if (aw->icon_file)
			g_free (aw->icon_file);
		aw->icon_file = g_strdup (icon);
	}

	if (aw->gettext_support)
		gt_support = g_strdup (_("Yes"));
	else
		gt_support = g_strdup (_("No"));
	text = g_strconcat (
			_("Confirm the following information:\n\n"),
			_("Project Name:    "), aw->prj_name, "\n",
			_("Project Type:    "), project_type_map[aw->prj_type], "\n",
			_("Target Type:     "), project_target_type_map[aw->target_type], "\n",
			_("Source Target:   "), aw->target, "\n",
			_("Version:         "), aw->version,"\n",
			_("Author:          "), aw->author, "\n", 
			_("Prog. Language:  "), programming_language_map[aw->language], "\n", 
			_("Gettext support: "), gt_support,"\n",
			NULL);
	gnome_druid_page_finish_set_text (GNOME_DRUID_PAGE_FINISH
					  (aw->widgets.page[5]), text);
	g_free (gt_support);
	return FALSE;
}

void
on_druidpagefinish1_finish (GnomeDruidPage *
			    gnomedruidpage, gpointer arg1, gpointer user_data)
{
	AppWidzard *aw = user_data;
	gtk_widget_hide (aw->widgets.window);
	create_new_project (aw);
	app_widzard_destroy (aw);
}

gboolean
on_druidpagefinish1_back (GnomeDruidPage *
			  gnomedruidpage, gpointer arg1, gpointer user_data)
{
	return FALSE;
}

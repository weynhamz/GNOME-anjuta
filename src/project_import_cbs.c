/*	project_import_cbs.c
 *  Copyright (C) 2002 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "project_import_cbs.h"
#include "project_dbase.h"
#include "utilities.h"
#include "anjuta.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>

gboolean
on_page_start_next (GnomeDruidPage * page_start, gpointer arg1, gpointer data)
{
	return FALSE;
}

gboolean
on_page2_next (GnomeDruidPage * page2, gpointer arg1, gpointer data)
{
	char *directory;
	ProjectImportWizard *piw = (ProjectImportWizard *) data;
	g_assert (data != NULL);

	directory =
		gnome_file_entry_get_full_path (GNOME_FILE_ENTRY
						(piw->widgets.file_entry),
						FALSE);
	if (!directory)
		return TRUE;
	if (!strlen (directory))
	{
		g_free (directory);
		return TRUE;
	}
	project_import_start (directory, piw);
	g_free (directory);
	return TRUE;
}

gboolean
on_page3_next (GnomeDruidPage * page3, gpointer arg1, gpointer data)
{
	ProjectImportWizard *piw = (ProjectImportWizard *) data;
	g_assert (data != NULL);
	
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.prj_name_entry), piw->prj_name);
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.version_entry), piw->prj_version);
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.target_entry), piw->prj_source_target);
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.author_entry), piw->prj_author);

	if (piw->prj_type == PROJECT_TYPE_END_MARK)
		return TRUE;
	else
	{
		if (strcmp
		    (piw->prj_programming_language,
		     programming_language_map
		     [PROJECT_PROGRAMMING_LANGUAGE_C]) == 0)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (piw->widgets.
						       language_c_radio),
						      TRUE);
		}
		else if (strcmp
			 (piw->prj_programming_language,
			  programming_language_map
			  [PROJECT_PROGRAMMING_LANGUAGE_CPP]) == 0)
		{
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON
				 (piw->widgets.language_cpp_radio), TRUE);
		}
		else
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (piw->widgets.
						       language_c_cpp_radio),
						      TRUE);
		}
		return FALSE;
	}
}

gboolean
on_page4_next (GnomeDruidPage * page4, gpointer arg1, gpointer data)
{
	gchar *name, *author, *version, *target;
	
	ProjectImportWizard *piw = (ProjectImportWizard *) data;
	g_assert (data != NULL);

	name = gtk_entry_get_text (GTK_ENTRY (piw->widgets.prj_name_entry));
	author = gtk_entry_get_text (GTK_ENTRY (piw->widgets.author_entry));
	version = gtk_entry_get_text (GTK_ENTRY (piw->widgets.version_entry));
	target = gtk_entry_get_text (GTK_ENTRY (piw->widgets.target_entry));

	// Bad hack to fill the gnome-menu-entries:
	on_prj_name_entry_focus_out_event(piw->widgets.prj_name_entry, NULL, piw);

	if (!strlen (name) || !strlen (author)
	    || !strlen (version) || !strlen (target))
	{
		gnome_ok_dialog (_("Please fill in all needed values!"));
		return TRUE;
	}
	
	/* It is necessary to free the link, because they are all allocated
	string */
	string_free(piw->prj_name);
	string_free(piw->prj_author);
	string_free(piw->prj_version);
	string_free(piw->prj_source_target);
	
	/* And re-alloc them, to stay on the track */
	piw->prj_name =	g_strdup(name);
	piw->prj_author = g_strdup(author);
	piw->prj_version = g_strdup(version);
	piw->prj_source_target = g_strdup(target);

	return FALSE;
}

gboolean
on_page5_next (GnomeDruidPage * page, gpointer arg1, gpointer data)
{
	ProjectImportWizard *piw = (ProjectImportWizard *) data;
	g_assert (data != NULL);

	string_free (piw->prj_description);
	piw->prj_description =
		gtk_editable_get_chars (GTK_EDITABLE
					(piw->widgets.description_text), 0,
					-1);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (piw->widgets.gettext_support_check),
				      piw->prj_has_gettext);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (piw->widgets.term_check),
					      piw->prj_menu_need_terminal);

/*
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.menu_entry_entry), piw->prj_menu_entry);
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.menu_comment_entry), piw->prj_menu_comment);
	gtk_entry_set_text(GTK_ENTRY(piw->widgets.app_group_entry), piw->prj_menu_group);
*/
	// That icon will never exist.
	
	return FALSE;
}

gboolean
on_page6_next (GnomeDruidPage * page, gpointer arg1, gpointer data)
{
	gchar *text, *gt_support, *icon;
	ProjectImportWizard *piw = (ProjectImportWizard *) data;
	g_assert (data != NULL);

	piw->prj_has_gettext =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (piw->widgets.
					       gettext_support_check));
	piw->prj_gpl =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (piw->widgets.
					       file_header_check));
	
	piw->prj_menu_need_terminal =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (piw->widgets.
					       gettext_support_check));
	
	string_free (piw->prj_menu_icon);
	piw->prj_menu_icon = NULL;
	icon = gnome_icon_entry_get_filename (GNOME_ICON_ENTRY
					      (piw->widgets.icon_entry));
	if (icon)
		piw->prj_menu_icon = g_strdup (icon);

	if (piw->prj_has_gettext)
		gt_support = g_strdup (_("Yes"));
	else
		gt_support = g_strdup (_("No"));
	text = g_strconcat (_("Confirm the following information:\n\n"),
			    _("Project Name:    "), piw->prj_name, "\n",
			    _("Project Type:    "),
			    project_type_map[piw->prj_type], "\n",
			    _("Target Type:     "),
			    project_target_type_map
			    [PROJECT_TARGET_TYPE_EXECUTABLE], "\n",
			    _("Source Target:   "), piw->prj_source_target,
			    "\n", _("Version:         "), piw->prj_version,
			    "\n", _("Author:          "), piw->prj_author,
			    "\n", _("Language:        "),
			    piw->prj_programming_language, "\n",
			    _("Gettext Support: "), gt_support, "\n", NULL);
	
	gnome_druid_page_finish_set_text (GNOME_DRUID_PAGE_FINISH
					  (piw->widgets.page[6]), text);
	g_free (gt_support);
	return FALSE;
}

gboolean
on_page_finish_finish (GnomeDruidPage * page_finish,
		       gpointer arg1, gpointer data)
{
	gchar *filename =
		g_strdup_printf ("%s/%s.prj", piw->directory, piw->prj_name);
	g_assert (data != NULL);

	// Delete old, maybe wrong project file (e.g. _PACKAGE.prj)
	remove (piw->filename);

	project_import_save_values (data);
	string_assign (&app->project_dbase->proj_filename, NULL);
	app->project_dbase->proj_filename = g_strdup (filename);
	project_dbase_save_project (app->project_dbase);
	project_dbase_close_project (app->project_dbase);
	project_dbase_load_project_file (app->project_dbase, filename);
	project_dbase_load_project_finish (app->project_dbase, TRUE);
	g_free (filename);
	gtk_widget_hide (piw->widgets.window);
	destroy_project_import_gui ();
	return FALSE;
}

gboolean
on_page2_back (GnomeDruidPage * page2, gpointer arg1, gpointer data)
{
	return FALSE;
}

gboolean
on_page3_back (GnomeDruidPage * page3, gpointer arg1, gpointer data)
{
	gnome_ok_dialog (_("Sorry, this is a point of no return"));
	return TRUE;
}

gboolean
on_page4_back (GnomeDruidPage * page4, gpointer arg1, gpointer data)
{
	return FALSE;
}

gboolean
on_page5_back (GnomeDruidPage * page, gpointer arg1, gpointer data)
{
	return FALSE;
}

gboolean
on_page6_back (GnomeDruidPage * page, gpointer arg1, gpointer data)
{
	return FALSE;
}


gboolean
on_page_finish_back (GnomeDruidPage * page_finish,
		     gpointer arg1, gpointer data)
{
	return FALSE;
}

void
on_druid_cancel (GnomeDruid * druid, gpointer user_data)
{
	ProjectImportWizard *piw = (ProjectImportWizard *) user_data;
	g_assert (user_data != NULL);

	gtk_widget_hide (piw->widgets.window);
	destroy_project_import_gui ();
	return;
}

void
on_wizard_import_icon_select (GnomeIconList * gil,
			      gint num, GdkEvent * event, gpointer user_data)
{
	ProjectImportWizard *piw = user_data;
	g_assert (user_data != NULL);

	switch (num)
	{
	case 0:
		piw->prj_type = PROJECT_TYPE_GENERIC;
		break;
	case 1:
		piw->prj_type = PROJECT_TYPE_GTK;
		break;
	case 2:
		piw->prj_type = PROJECT_TYPE_GNOME;
		break;
	case 3:
		piw->prj_type = PROJECT_TYPE_GTKMM;
		break;
	case 4:
		piw->prj_type = PROJECT_TYPE_GNOMEMM;
		break;
	case 5:
		piw->prj_type = PROJECT_TYPE_BONOBO;
		break;
	case 6:
		piw->prj_type = PROJECT_TYPE_LIBGLADE;
		break;
	default:		/* Invalid project type */
		piw->prj_type = PROJECT_TYPE_END_MARK;
		break;
	}
}

void
on_prj_name_entry_changed (GtkEditable * editable, gpointer user_data)
{
	gchar *text;
	gint i;

	ProjectImportWizard *piw = user_data;
	g_assert (user_data != NULL);
	text = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
	for (i = 0; i < strlen (text); i++)
		text[i] = tolower (text[i]);
	if (text)
		gtk_entry_set_text (GTK_ENTRY (piw->widgets.target_entry),
				    text);
	else
		gtk_entry_set_text (GTK_ENTRY (piw->widgets.target_entry),
				    "");
	string_free (text);
}

gboolean
on_prj_name_entry_focus_out_event (GtkWidget * widget,
				   GdkEventFocus * event, gpointer user_data)
{
	gchar *temp;
	ProjectImportWizard *piw = user_data;
	g_assert (user_data != NULL);

/* 
	23 Aug 2001 - Yannick Koehler 
		Disabling Project Name Capitalization.
*/
#if 0
	if (piw->prj_name)
	{
		piw->prj_name[0] = toupper (piw->prj_name[0]);
		gtk_entry_set_text (GTK_ENTRY (widget), piw->prj_name);
	}
#endif
	temp = g_strdup_printf ("%s Version %s", piw->prj_name,
				piw->prj_version);
	gtk_entry_set_text (GTK_ENTRY (piw->widgets.menu_entry_entry), temp);
	gtk_entry_set_text (GTK_ENTRY (piw->widgets.menu_comment_entry),
			    temp);
	g_free (temp);
	return FALSE;
}

void
on_lang_c_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	ProjectImportWizard *piw = user_data;
	g_assert (user_data != NULL);

	g_return_if_fail (piw != NULL);
	if (gtk_toggle_button_get_active (togglebutton))
		piw->prj_programming_language =
			programming_language_map
			[PROJECT_PROGRAMMING_LANGUAGE_C];
}

void
on_lang_cpp_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	ProjectImportWizard *piw = user_data;
	g_assert (user_data != NULL);

	g_return_if_fail (piw != NULL);
	if (gtk_toggle_button_get_active (togglebutton))
		piw->prj_programming_language =
			programming_language_map
			[PROJECT_PROGRAMMING_LANGUAGE_CPP];
}

void
on_lang_c_cpp_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	ProjectImportWizard *piw = user_data;
	g_assert (user_data != NULL);

	g_return_if_fail (piw != NULL);
	if (gtk_toggle_button_get_active (togglebutton))
		piw->prj_programming_language =
			programming_language_map
			[PROJECT_PROGRAMMING_LANGUAGE_C_CPP];
}

void
on_piw_text_entry_realize (GtkWidget * widget, gpointer user_data)
{
	if (!user_data)
		return;
	gtk_entry_set_text (GTK_ENTRY (widget), (gchar *) user_data);
}

void
on_piw_text_entry_changed (GtkEditable * editable, gpointer user_data)
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

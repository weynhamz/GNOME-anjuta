/*  
 *  project_import.c
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "project_import.h"
#include "preferences.h"
#include "fileselection.h"
#include "launcher.h"
#include "anjuta.h"
#include "message-manager.h"
#include "project_import.h"
#include "utilities.h"
#include "project_dbase.h"

static gboolean project_import_load_project_values (ProjectImportWizard *piw,
													gchar * filename);

static void on_import_output_arrived (AnjutaLauncher *launcher,
									  AnjutaLauncherOutputType output_type,
									  const gchar * line, gpointer data);
static void on_import_terminated (AnjutaLauncher *launcher,
								  gint child_pid, gint status,
								  gulong time_taken, gpointer data);

/* The maximum time allowed for importing a project (in seconds).
This might seem a bit high - but then, shell scripts are slow.
We can safely reduce this to a minute or so once we finish
re-writing the import script in C - Biswa */

#define AN_IMPORT_TIMEOUT 600

gboolean progressbar_timeout (gpointer data);

#define IMPORT_SCRIPT PACKAGE_BIN_DIR "/anjuta_import.sh"

gboolean
project_import_start (const gchar *topleveldir, ProjectImportWizard * piw)
{
	gchar *tmp;
	gchar *command;
	gboolean ret;
	struct stat s;

	piw->directory = g_strdup (topleveldir);

/*
	tmp = g_strconcat (app->dirs->data, "/", "anjuta_import.sh", NULL);
	if (file_is_regular (tmp) == FALSE)
	{
		anjuta_error (_("I can not find: %s.\n"
				"Unable to run import project script. :-(\n"
				"Make sure Anjuta is installed correctly."),
			      tmp);
		g_free (tmp);
		return FALSE;
	}
	g_free (tmp);
*/
	if (0 != stat(IMPORT_SCRIPT, &s))
	{
		anjuta_system_error_parented(app->widgets.window, errno
		  , _("Unable to find import script %s"), IMPORT_SCRIPT);
		return FALSE;
	}
	if (!S_ISREG(s.st_mode))
	{
		anjuta_error_parented(app->widgets.window, _("%s: Not a regular file"), IMPORT_SCRIPT);
		return FALSE;
	}
	command = g_strdup_printf (IMPORT_SCRIPT " \"%s\"", topleveldir);
	g_message("Command: %s", command);
	
	g_signal_connect (G_OBJECT (app->launcher), "child-exited",
					  G_CALLBACK (on_import_terminated), piw);

	ret = anjuta_launcher_execute (app->launcher, command,
								   on_import_output_arrived, piw);
	g_free(command);
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	if (ret)
	{
		tmp = g_strdup_printf (_("Importing Project from %s ...\n"),
				       topleveldir);
		an_message_manager_append (app->messages, tmp,
					       MESSAGE_BUILD);
		g_free (tmp);
	}
	else
	{
		an_message_manager_append (app->messages,
					       _
					       ("Could not launch script!\n"),
					       MESSAGE_BUILD);
		an_message_manager_show (app->messages, MESSAGE_BUILD);
		return ret;
	}
	an_message_manager_show (app->messages, MESSAGE_BUILD);

	gtk_widget_show (piw->widgets.progressbar);
	// gtk_progress_set_activity_mode (GTK_PROGRESS
	//				(piw->widgets.progressbar), TRUE);
	gtk_label_set_text (GTK_LABEL (piw->widgets.label),
			    _("Importing Project...please wait"));
	piw->progress_timer_id = gtk_timeout_add (AN_IMPORT_TIMEOUT,
											  progressbar_timeout,
								 GTK_PROGRESS_BAR (piw->widgets.progressbar));
	return ret;
}

static void
on_import_output_arrived (AnjutaLauncher *launcher,
						  AnjutaLauncherOutputType output_type,
						  const gchar * line, gpointer data)
{
	gchar filename[512];
	gchar *pos;
	ProjectImportWizard *piw = (ProjectImportWizard*)data;
	
	if (!line)
		return;
	an_message_manager_append (app->messages, line, MESSAGE_BUILD);
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDOUT)
	{
		if (NULL != (pos = strstr(line, "Created project file ")))
		{
			if (sscanf (pos, "Created project file %s successfully.", filename) == 1)
				piw->filename = g_strdup (filename);
		}
	}
}

static void
on_import_terminated (AnjutaLauncher *launcher,
					  gint child_pid, gint status, gulong time_taken,
					  gpointer data)
{
	gchar *full_path;
	gchar *buff;
	ProjectImportWizard *piw = (ProjectImportWizard*)data;
	
	if (piw->progress_timer_id)
		gtk_timeout_remove (piw->progress_timer_id);
	piw->progress_timer_id = 0;
	
	g_signal_handlers_disconnect_by_func (launcher,
										  G_CALLBACK (on_import_terminated),
										  data);

	anjuta_update_app_status (TRUE, NULL);

	if (status)
	{
		an_message_manager_append (app->messages,
					       _("Project import completed...unsuccessful\n"),
					       MESSAGE_BUILD);
		anjuta_status (_("Project import completed...unsuccessful"));
	}
	else
	{
		an_message_manager_append (app->messages,
					       _("Project import completed...successful\n"),
					       MESSAGE_BUILD);
		anjuta_status (_("Project import completed...successful"));
	}
	buff = g_strdup_printf (_("Total time taken: %lu secs\n"), time_taken);
	an_message_manager_append (app->messages, buff, MESSAGE_BUILD);
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();

	/* If the import operation was canceled. Just destroy it and return */
	if (piw->canceled == TRUE)
	{
		project_import_destroy (piw);
		return;
	}
	if (piw->filename == NULL)
	{
		anjuta_error (_("Could not import Project: no project file found!"));
		project_import_destroy (piw);
		return;
	}
	full_path = g_strdup_printf ("%s/%s", piw->directory, piw->filename);

	gtk_label_set_text (GTK_LABEL (piw->widgets.label),
						_("Opening Project...please wait"));
	if (!project_import_load_project_values (piw, full_path))
	{
		anjuta_error (_("Could not open generated Project file"));
		project_import_destroy (piw);
		return;
	}

	gnome_druid_set_page (GNOME_DRUID (piw->widgets.druid),
			      GNOME_DRUID_PAGE (piw->widgets.page[2]));
	// Disable back button
	gtk_widget_set_sensitive(GNOME_DRUID(piw->widgets.druid)->back, FALSE);
}

/* This is from a gtk example but it looks nice ;-) */
gboolean
progressbar_timeout (gpointer data)
{
	gfloat new_val;

	new_val = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (data)) + 0.05;
	if (new_val > 1.0)
		new_val = 0.0;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data), new_val);
	return TRUE;
}

static gboolean
project_import_load_project_values (ProjectImportWizard *piw, gchar * filename)
{
	ProjectType *type;
	ProjectDBase *p = app->project_dbase;

	if (!project_dbase_load_project_file (p, filename))
	{
		return FALSE;
	}
	piw->prj_name = project_dbase_get_name (p);
	if (!piw->prj_name)
		piw->prj_name = g_strdup ("");

	type = project_dbase_get_project_type (p);
	if (type)
	{
		piw->prj_type = type->id;
		project_type_free (type);
	}
	else
		piw->prj_type = PROJECT_TYPE_GENERIC;

	piw->prj_version = project_dbase_get_version (p);
	if (!piw->prj_version)
		piw->prj_version = g_strdup ("");

	piw->prj_source_target = prop_get (p->props, "project.source.target");
	if (!piw->prj_source_target)
		piw->prj_source_target = g_strdup ("");

	piw->prj_author = prop_get (p->props, "project.author");
	if (!piw->prj_author) ;
	piw->prj_author = g_strdup ("");

	piw->prj_has_gettext = prop_get_int (p->props, "project.has.gettext", 1);

	piw->prj_programming_language =
		prop_get (p->props, "project.programming.language");
	if (!piw->prj_programming_language)
		piw->prj_programming_language =
			g_strdup (programming_language_map
				  [PROJECT_PROGRAMMING_LANGUAGE_C]);

	piw->prj_menu_entry = prop_get (p->props, "project.menu.entry");
	if (!piw->prj_menu_entry)
		piw->prj_menu_entry = g_strdup ("");
	piw->prj_menu_group = prop_get (p->props, "project.menu.group");
	if (!piw->prj_menu_group)
		piw->prj_menu_group = g_strdup ("");
	piw->prj_menu_comment = prop_get (p->props, "project.menu.comment");
	if (!piw->prj_menu_comment)
		piw->prj_menu_comment = g_strdup ("");
	piw->prj_menu_icon = prop_get (p->props, "project.menu.icon");
	if (!piw->prj_menu_icon)
		piw->prj_menu_icon = g_strdup ("");;
	piw->prj_menu_need_terminal =
		prop_get_int (p->props, "project.menu.need.terminal", 0);

	gnome_icon_list_select_icon (GNOME_ICON_LIST (piw->widgets.iconlist),
				     piw->prj_type);
	return TRUE;
}

void
project_import_save_values (ProjectImportWizard * piw)
{
	ProjectDBase *p = app->project_dbase;
	ProjectConfig *config = p->project_config;

	ProjectType *type = project_type_load (piw->prj_type);
	prop_set_with_key (p->props, "project.name", piw->prj_name);
	prop_set_with_key (p->props, "project.type", type->name);
	project_type_free (type);
	prop_set_with_key (p->props, "project.version", piw->prj_version);
	prop_set_with_key (p->props, "project.author", piw->prj_author);
	prop_set_with_key (p->props, "project.source.target",
			   piw->prj_source_target);
	if (piw->prj_has_gettext)
		prop_set_with_key (p->props, "project.has.gettext", "1");
	else
		prop_set_with_key (p->props, "project.has.gettext", "0");

	prop_set_with_key (p->props, "project.programming.language",
			   piw->prj_programming_language);
	prop_set_with_key (p->props, "project.menu.entry",
			   piw->prj_menu_entry);
	prop_set_with_key (p->props, "project.menu.group",
			   piw->prj_menu_group);
	prop_set_with_key (p->props, "project.menu.comment",
			   piw->prj_menu_comment);
	prop_set_with_key (p->props, "project.menu.icon", piw->prj_menu_icon);
	
	if (piw->prj_menu_need_terminal)
		prop_set_with_key (p->props, "project.menu.need_terminal", "1");
	else
		prop_set_with_key (p->props, "project.menu.need_terminal", "0");
	
	project_config_set_description (config, piw->prj_description);
	
	// Do not allow anjuta to change Makefiles
	prop_set_with_key (p->props, "project.config.blocked", "1");
}

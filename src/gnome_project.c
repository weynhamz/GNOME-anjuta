/*
    gnome_project.c
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
#include <sys/wait.h>
#include <errno.h>
#include <gnome.h>

#include "appwizard.h"
#include "anjuta.h"
#include "preferences.h"
#include "launcher.h"
#include "messages.h"
#include "gnome_project.h"
#include "fileselection.h"

static void new_prj_mesg_arrived (gchar * mesg);
static void new_prj_terminated (int status, time_t t);

gboolean
create_new_project (AppWizard * aw)
{
	gchar *all_prj_dir, *top_dir, *prj_file;
	GList* list;
	gchar* files;
	FILE* fp;
	gint i;

	all_prj_dir  = preferences_get (app->preferences, PROJECTS_DIRECTORY);
	top_dir = g_strdup_printf ("%s/%s-%s", all_prj_dir, aw->prj_name, aw->version);

	if (file_is_directory (top_dir))
	{
		g_free (all_prj_dir);
		g_free (top_dir);
		
		anjuta_error (
			_("The Project already exist.\n"
			    "Project creation aborted."));
		return FALSE;
	}

	force_create_dir (all_prj_dir);
	force_create_dir (top_dir);
	chdir (top_dir);
	g_free (all_prj_dir);

	prj_file = g_strdup_printf ("%s/%s.prj", top_dir, aw->prj_name);
	g_free (top_dir);
	
	fileselection_set_filename (app->project_dbase->fileselection_open, prj_file);
	messages_clear (app->messages, MESSAGE_BUILD);
	messages_append (app->messages, _("Generating  project ...\n"), MESSAGE_BUILD);
	messages_show (app->messages, MESSAGE_BUILD);

	fp = fopen (prj_file, "w");
	if (!fp)
	{
		anjuta_system_error (errno, _("Cannot create file: %s"), prj_file);
		g_free (prj_file);
		
		return FALSE;
	}

	g_free (prj_file);

	fprintf(fp, "# Anjuta Version %s\n", VERSION);
	fprintf(fp, "Compatibility Level: %d\n\n", COMPATIBILITY_LEVEL);

	fprintf (fp, PROJECT_DESCRIPTION_START);
	if (aw->description)
		fprintf (fp, "%s", aw->description);
	fprintf (fp, PROJECT_DESCRIPTION_END);
	fprintf(fp, "%s", CONFIG_PROGS_START);
	fprintf(fp, "%s", CONFIG_PROGS_END);
	fprintf(fp, "%s", CONFIG_LIBS_START);
	fprintf(fp, "%s", CONFIG_LIBS_END);
	fprintf(fp, "%s", CONFIG_HEADERS_START);
	fprintf(fp, "%s", CONFIG_HEADERS_END);
	fprintf(fp, "%s", CONFIG_CHARACTERISTICS_START);
	fprintf(fp, "%s", CONFIG_CHARACTERISTICS_END);
	fprintf(fp, "%s", CONFIG_LIB_FUNCS_START);
	fprintf(fp, "%s", CONFIG_LIB_FUNCS_END);
	fprintf(fp, "%s", CONFIG_ADDITIONAL_START);
	fprintf(fp, "%s", CONFIG_ADDITIONAL_END);
	fprintf(fp, "%s", CONFIG_FILES_START);
	fprintf(fp, "%s", CONFIG_FILES_END);
	fprintf(fp, "%s", MAKEFILE_AM_START);
	fprintf(fp, "%s\n", MAKEFILE_AM_END);
	fprintf(fp, "project.config.disable.overwriting=0 0 0 0 0 0 0 0 0\n\n");
	fprintf(fp, "props.file.type=project\n\n");
	fprintf(fp, "anjuta.version=%s\n", VERSION);
	fprintf(fp, "anjuta.compatibility.level=%d\n\n", COMPATIBILITY_LEVEL);
	fprintf(fp, "project.name=%s\n", aw->prj_name);
	fprintf(fp, "project.type=%s\n", project_type_map[aw->prj_type]);
	fprintf(fp, "project.target.type=%s\n", project_target_type_map[aw->target_type]);
	fprintf(fp, "project.version=%s\n", aw->version);
	fprintf(fp, "project.author=%s\n", aw->author);
	fprintf(fp, "project.source.target=%s\n", aw->target);
	fprintf(fp, "project.programming.language=%s\n\n", programming_language_map[aw->language]);
	fprintf(fp, "project.has.gettext=%d\n", aw->gettext_support);
	fprintf(fp, "project.use.header=%d\n\n", aw->use_header);
	fprintf(fp, "project.menu.entry=%s\n", aw->menu_entry);
	fprintf(fp, "project.menu.group=%s\n", aw->app_group);
	fprintf(fp, "project.menu.comment=%s\n", aw->menu_comment);
	fprintf(fp, "project.menu.icon=%s_icon.%s\n", aw->target, get_file_extension(aw->icon_file));
	fprintf(fp, "project.menu.need.terminal=%d\n\n", aw->need_terminal);
	if (aw->prj_type == PROJECT_TYPE_GTK)
	{
		fprintf(fp, "compiler.options.supports=GTK\n\n");
	}
	else if (aw->prj_type == PROJECT_TYPE_GNOME)
	{
		fprintf(fp, "compiler.options.supports=GNOME\n\n");
	}
	else if (aw->prj_type == PROJECT_TYPE_BONOBO)
	{
		fprintf(fp, "compiler.options.supports=BONOBO\n\n");
	}
	else if (aw->prj_type == PROJECT_TYPE_GTKMM)
	{
		fprintf(fp, "compiler.options.supports=GTKMM\n\n");
	}
	else if (aw->prj_type == PROJECT_TYPE_GNOMEMM)
	{
		fprintf(fp, "compiler.options.supports=GNOMEMM\n\n");
	}
	fclose(fp);
	messages_append (app->messages, _("Loading  project ...\n"), MESSAGE_BUILD);
	
	if(project_dbase_load_project(app->project_dbase, FALSE)==FALSE)
		return FALSE;
	messages_append (app->messages, _("Saving  project ...\n"), MESSAGE_BUILD);
	app->project_dbase->is_saved = FALSE;
	if (project_dbase_save_project(app->project_dbase)==FALSE)
		return FALSE;

	/*  Source codes are generated */
	messages_append (app->messages, _("Generating  source codes ...\n"), MESSAGE_BUILD);
	if (project_dbase_generate_source_code (app->project_dbase)==FALSE)
		return FALSE;
	
	/* Creating icon pixmap file for gnome projects */
	messages_append (app->messages, _("Copying icon file ...\n"), MESSAGE_BUILD);
	if ( !(aw->prj_type == PROJECT_TYPE_GENERIC
		||aw->prj_type == PROJECT_TYPE_GTK) && aw->icon_file)
	{
		gchar* dir;
		gchar* dest;
		
		dir = project_dbase_get_module_dir (app->project_dbase, MODULE_PIXMAP);
		dest = g_strdup_printf ("%s/%s_icon.%s", dir, aw->target, get_file_extension (aw->icon_file));
		force_create_dir (dir);
		if (copy_file (aw->icon_file, dest, FALSE) == FALSE)
		{
			messages_append (app->messages, _("Could not create icon file ...\n"), MESSAGE_BUILD);
		}
		g_free (dir);
		g_free (dest);
	}

	/* Scanning for created files in every module */
	messages_append (app->messages, _("Locating files ...\n"), MESSAGE_BUILD);
	for(i=0; i<MODULE_END_MARK; i++)
	{
		gchar *key;	

		if (i == MODULE_PO)
			continue;
		list = project_dbase_scan_files_in_module (app->project_dbase, i, FALSE);
		files = string_from_glist(list);
		if (files)
		{
			key = g_strconcat ("module.", module_map[i], ".files", NULL);
			prop_set_with_key (app->project_dbase->props, key, files);
			g_free (key);
		}
	}
	messages_append (app->messages, _("Again saving  project ...\n"), MESSAGE_BUILD);
	if (project_dbase_save_project(app->project_dbase)==FALSE)
		return FALSE;
	messages_append (app->messages, _("Updating  project ...\n"), MESSAGE_BUILD);
	project_dbase_update_tree (app->project_dbase);
	messages_append (app->messages, _("Running  autogen.sh ...\n"), MESSAGE_BUILD);
	chdir (app->project_dbase->top_proj_dir);
	if (launcher_execute ("./autogen.sh",
			new_prj_mesg_arrived,
			new_prj_mesg_arrived, new_prj_terminated) == FALSE)
	{
		anjuta_error ("Could not run ./autogen.sh");
		return FALSE;
	}
	anjuta_update_app_status (TRUE, _("App Wizard"));
	return TRUE;
}

static void
new_prj_mesg_arrived (gchar * mesg)
{
	messages_append (app->messages, mesg, MESSAGE_BUILD);
}

static void
new_prj_terminated (int status, time_t t)
{
	if (WEXITSTATUS (status))
	{
		messages_append (app->messages,
			_("Autogeneration completed...............Unsuccessful\n"),
			MESSAGE_BUILD);
		anjuta_error (
			_("The Project was not successfully autogenerated.\n"
			"I think you have to do it manually."));
	}
	else
	{
		messages_append (app->messages,
				 _("Autogeneration completed...............Successful\n"
				  "Now Build the project to have a LOOK at it\n"),
				 MESSAGE_BUILD);
		anjuta_status (_("Project was successfully autogenerated."));
	}
	project_dbase_show (app->project_dbase);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	anjuta_update_app_status (TRUE, NULL);
}

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
#include "message-manager.h"
#include "gnome_project.h"
#include "fileselection.h"

static void new_prj_mesg_arrived (AnjutaLauncher *launcher,
								  AnjutaLauncherOutputType output_type,
								  const gchar * mesg, gpointer data);
static void new_prj_terminated (AnjutaLauncher *launcher, gint child_pid,
								gint status, time_t t, gpointer data);

gboolean
create_new_project (AppWizard * aw)
{
	gchar *all_prj_dir, *top_dir, *prj_file;
	GList* list;
	gchar* files;
	FILE* fp;
	gint i;
	ProjectType* type;

	all_prj_dir  =
		anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								PROJECTS_DIRECTORY);
	top_dir = g_strdup_printf ("%s/%s", all_prj_dir, aw->prj_name);

	if (file_is_directory (top_dir) || file_is_regular(top_dir))
	{
		g_free (all_prj_dir);
		g_free (top_dir);
		
		anjuta_error (
			_("A project or file with the same name.already exists.\n"
			    "Project creation aborted."));
		return FALSE;
	}

	force_create_dir (all_prj_dir);
	force_create_dir (top_dir);
	chdir (top_dir);
	g_free (all_prj_dir);

	prj_file = g_strdup_printf ("%s/%s.prj", top_dir, aw->prj_name);
	g_free (top_dir);
	
	fileselection_set_filename (app->project_dbase->fileselection_open,
								prj_file);
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	an_message_manager_append (app->messages, _("Generating Project ...\n"),
								   MESSAGE_BUILD);
	an_message_manager_show (app->messages, MESSAGE_BUILD);

	fp = fopen (prj_file, "w");
	if (!fp)
	{
		anjuta_system_error (errno, _("Cannot create file: %s"), prj_file);
		g_free (prj_file);
		return FALSE;
	}

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
	fprintf(fp, "project.name=%s\n", _STR(aw->prj_name));
	fprintf(fp, "project.type=%s\n", project_type_map[aw->prj_type]);
	fprintf(fp, "project.target.type=%s\n",
			project_target_type_map[aw->target_type]);
	fprintf(fp, "project.version=%s\n", _STR(aw->version));
	fprintf(fp, "project.author=%s\n", _STR(aw->author));
	fprintf(fp, "project.source.target=%s\n", _STR(aw->target));
	fprintf(fp, "project.programming.language=%s\n\n",
			programming_language_map[aw->language]);
	fprintf(fp, "project.has.gettext=%d\n", aw->gettext_support);
	fprintf(fp, "project.use.header=%d\n\n", aw->use_header);
	fprintf(fp, "project.menu.entry=%s\n", _STR(aw->menu_entry));
	fprintf(fp, "project.menu.group=%s\n", _STR(aw->app_group));
	fprintf(fp, "project.menu.comment=%s\n", _STR(aw->menu_comment));

	if (aw->icon_file) {
		gchar *ext = get_file_extension (aw->icon_file);
		fprintf (fp, "project.menu.icon=%s_icon", aw->target);

		if (ext)
			fprintf (fp, ".%s", ext);

		fprintf (fp, "\n");
	} else
		fprintf (fp, "project.menu.icon=\n");

	fprintf(fp, "project.menu.need.terminal=%d\n\n", aw->need_terminal);

	type = project_type_load (aw->prj_type);
	if (type->id != PROJECT_TYPE_GENERIC)
	{
		fprintf (fp, "compiler.options.supports=%s\n\n",
				 _STR(type->save_string));
		//free_project_type(type);
		//type = NULL ;
	}
	fclose(fp);
	an_message_manager_append (app->messages, _("Loading Project ...\n"),
								   MESSAGE_BUILD);
	
	if (project_dbase_load_project (app->project_dbase, prj_file,
									FALSE) == FALSE)
	{
		g_free (prj_file);
		return FALSE;
	}
	g_free (prj_file);

	an_message_manager_append (app->messages, _("Saving Project ...\n"),
								   MESSAGE_BUILD);
	app->project_dbase->is_saved = FALSE;
	if (project_dbase_save_project (app->project_dbase) == FALSE)
		return FALSE;

	/*  Source codes are generated */
	an_message_manager_append (app->messages,
								   _("Generating source codes ...\n"),
								   MESSAGE_BUILD);
	if (project_dbase_generate_source_code (app->project_dbase,
											aw->use_glade)==FALSE)
		return FALSE;
	
	/* Creating icon pixmap file for gnome projects */
	an_message_manager_append (app->messages, _("Copying icon file ...\n"),
								   MESSAGE_BUILD);
	if (type->gnome_support && aw->icon_file)
	{
		gchar* dir;
		gchar* dest;
		
		dir = project_dbase_get_module_dir (app->project_dbase, MODULE_PIXMAP);
		dest = g_strdup_printf ("%s/%s_icon.%s", dir, _STR(aw->target),
								_STR(get_file_extension (aw->icon_file)));
		force_create_dir (dir);
		if (copy_file (aw->icon_file, dest, FALSE) == FALSE)
		{
			an_message_manager_append (app->messages,
										  _("Could not create icon file ...\n"),
										   MESSAGE_BUILD);
		}
		g_free (dir);
		g_free (dest);
	}

	/* Scanning for created files in every module */
	if (type)
	{
		project_type_free (type);
		type = NULL;
	}

	an_message_manager_append (app->messages, _("Locating files ...\n"),
								   MESSAGE_BUILD);
	for(i=0; i<MODULE_END_MARK; i++)
	{
		gchar *key;	

		if (i == MODULE_PO)
			continue;
		list = project_dbase_scan_files_in_module (app->project_dbase,
												   i, FALSE);
		files = string_from_glist(list);
		if (files)
		{
			key = g_strconcat ("module.", module_map[i], ".files", NULL);
			prop_set_with_key (app->project_dbase->props, key, files);
			g_free (key);
		}
	}
	an_message_manager_append (app->messages, _("Saving Project ...\n"),
								   MESSAGE_BUILD);
	if (project_dbase_save_project(app->project_dbase)==FALSE)
		return FALSE;
	an_message_manager_append (app->messages, _("Updating Project ...\n"),
								   MESSAGE_BUILD);
	project_dbase_update_tree (app->project_dbase);
	an_message_manager_append (app->messages, _("Running autogen.sh ...\n"),
								   MESSAGE_BUILD);
	chdir (app->project_dbase->top_proj_dir);
	
	g_signal_connect (G_OBJECT (app->launcher), "child-exited",
					  G_CALLBACK (new_prj_terminated), NULL);
	if (anjuta_launcher_execute (app->launcher, "./autogen.sh",
								 new_prj_mesg_arrived, NULL) == FALSE)
	{
		anjuta_error ("Could not run ./autogen.sh");
		return FALSE;
	}
	anjuta_update_app_status (TRUE, _("App Wizard"));
	return TRUE;
}

static void
new_prj_mesg_arrived (AnjutaLauncher *launcher,
					  AnjutaLauncherOutputType output_type,
					  const gchar * mesg, gpointer data)
{
	an_message_manager_append (app->messages, mesg, MESSAGE_BUILD);
}

static void
new_prj_terminated (AnjutaLauncher *launcher, gint child_pid,
					gint status, time_t t, gpointer data)
{
	g_signal_handlers_disconnect_by_func (launcher,
										  G_CALLBACK (new_prj_terminated),
										  data);
	if (status)
	{
		an_message_manager_append (app->messages,
			_("Auto generation completed...............Unsuccessful\n"),
			MESSAGE_BUILD);
		anjuta_error (
			_("The Project was not successfully auto generated.\n"
			"Please run autogen.sh manually."));
	}
	else
	{
		an_message_manager_append (app->messages,
				 _("Auto generation completed...............Successful\n"
				  "Now Build the Project to have a LOOK at it\n"),
				 MESSAGE_BUILD);
		anjuta_status (_("Project was successfully auto generated."));
	}

	project_dbase_update_tags_image(app->project_dbase, TRUE);
	project_dbase_show (app->project_dbase);

	if (anjuta_preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	anjuta_update_app_status (TRUE, NULL);
}

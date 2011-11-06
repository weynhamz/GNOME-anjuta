/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    build.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>

#include "build.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/anjuta-utils.h>

#include "program.h"
#include "build-options.h"

/* Types
 *---------------------------------------------------------------------------*/

typedef struct
{
	gchar *args;
	GFile *file;
	BuildFunc func;
	IAnjutaBuilderCallback callback;
	gpointer user_data;
} BuildConfigureAndBuild;

/* Constants
 *---------------------------------------------------------------------------*/

#define PREF_INSTALL_ROOT "build-install-root"
#define PREF_INSTALL_ROOT_COMMAND "build-install-root-command"

#define DEFAULT_COMMAND_COMPILE "make"
#define DEFAULT_COMMAND_BUILD "make"
#define DEFAULT_COMMAND_IS_BUILT "make -q"
#define DEFAULT_COMMAND_BUILD_TARBALL "make dist"
#define DEFAULT_COMMAND_INSTALL "make install"
#define DEFAULT_COMMAND_CONFIGURE "configure"
#define DEFAULT_COMMAND_GENERATE "autogen.sh"
#define DEFAULT_COMMAND_CLEAN "make clean"
#define DEFAULT_COMMAND_DISTCLEAN "make distclean"
#define DEFAULT_COMMAND_AUTORECONF "autoreconf -i --force"

#define CHOOSE_COMMAND(plugin,command) \
	((plugin->commands[(IANJUTA_BUILDABLE_COMMAND_##command)]) ? \
			(plugin->commands[(IANJUTA_BUILDABLE_COMMAND_##command)]) \
			: \
			(DEFAULT_COMMAND_##command))

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Normalize a file to use project directory if it is a child */
GFile *
normalize_project_file (GFile *file, GFile *root)
{
	gchar *path;
	gchar *root_path;
	gchar *file_path;
	gchar *last_dir;
	guint i;
	GFile *new_file;

	path = g_file_get_path (root);
	root_path = anjuta_util_get_real_path (path);
	g_free (path);

	path = g_file_get_path (file);
	file_path = anjuta_util_get_real_path (path);
	g_free (path);

	if ((file_path != NULL) && (root_path != NULL))
	{
		for (i = 0; (file_path[i] == root_path[i]) && (file_path[i] != '\0') && (root_path[i] != '\0'); i++);
		if ((root_path[i] == '\0') && (file_path[i] == '\0'))
		{
			new_file = g_object_ref (root);
		}
		else if ((root_path[i] == '\0') && (file_path[i] == G_DIR_SEPARATOR))
		{
			new_file = g_file_resolve_relative_path (root, &file_path[i + 1]);
		}
		else
		{
			new_file = g_object_ref (file);
		}
	}
	else
	{
		new_file = g_object_ref (file);
	}

	g_free (root_path);
	g_free (file_path);

	return new_file;
}

gboolean
directory_has_makefile_am (BasicAutotoolsPlugin *bb_plugin,  GFile *dir)
{
	GFile *file;
	gboolean exists;

	/* We need configure.ac or configure.in too */
	if (bb_plugin->project_root_dir == NULL) return FALSE;

	exists = TRUE;
	file = g_file_get_child (bb_plugin->project_root_dir,  "configure.ac");
	if (!g_file_query_exists (file, NULL))
	{
		g_object_unref (file);
		file =  g_file_get_child (bb_plugin->project_root_dir,  "configure.in");
		if (!g_file_query_exists (file, NULL))
		{
			exists = FALSE;
		}
	}
	g_object_unref (file);

	/* Check for Makefile.am or GNUmakefile.am */
	if (g_file_has_prefix (dir, bb_plugin->project_build_dir))
	{
		/* Check for Makefile.am in source directory not build directory */
		gchar *relative;
		GFile *src_dir;

		relative = g_file_get_relative_path (bb_plugin->project_build_dir, dir);
		src_dir = g_file_get_child (bb_plugin->project_root_dir, relative);
		file = g_file_get_child (src_dir,  "Makefile.am");
		g_object_unref (src_dir);
		g_free (relative);
	}
	else if (g_file_equal (dir, bb_plugin->project_build_dir))
	{
		file = g_file_get_child (bb_plugin->project_root_dir,  "Makefile.am");
	}
	else
	{
		file = g_file_get_child (dir,  "Makefile.am");
	}

	if (!g_file_query_exists (file, NULL))
	{
		g_object_unref (file);
		file =  g_file_get_child (dir, "GNUmakefile.am");
		if (!g_file_query_exists (file, NULL))
		{
			exists = FALSE;
		}
	}
	g_object_unref (file);

	return exists;
}

gboolean
directory_has_makefile (GFile *dir)
{
	GFile *file;
	gboolean exists;

	exists = TRUE;
	file = g_file_get_child (dir, "Makefile");
	if (!g_file_query_exists (file, NULL))
	{
		g_object_unref (file);
		file = g_file_get_child (dir, "makefile");
		if (!g_file_query_exists (file, NULL))
		{
			g_object_unref (file);
			file = g_file_get_child (dir, "MAKEFILE");
			if (!g_file_query_exists (file, NULL))
			{
				exists = FALSE;
			}
		}
	}
	g_object_unref (file);

	return exists;
}

static gboolean
directory_has_file (GFile *dir, const gchar *filename)
{
	GFile *file;
	gboolean exists;

	file = g_file_get_child (dir, filename);
	exists = g_file_query_exists (file, NULL);
	g_object_unref (file);

	return exists;
}

static gchar*
shell_quotef (const gchar *format,...)
{
	va_list args;
	gchar *str;
	gchar *quoted_str;

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	quoted_str = g_shell_quote (str);
	g_free (str);

	return quoted_str;
}

/* Return FALSE if Makefile is missing and we have both a Makefile.am and a project
 * open, meaning that we need to configure the project to get a Makefile */
static gboolean
is_configured (BasicAutotoolsPlugin *plugin, GFile *file)
{
	GFile *build_dir;
	gboolean has_makefile;
	gboolean has_makefile_am;

	/* Get build directory and check for makefiles */
	build_dir = build_file_from_file (plugin, file, NULL);
	has_makefile = directory_has_makefile (build_dir);
	has_makefile_am = directory_has_makefile_am (plugin, build_dir);
	g_object_unref (build_dir);

	return has_makefile || !has_makefile_am || (plugin->project_root_dir == NULL);
}

/* Return build path from a source directory */
static GFile *
build_file_from_directory (BasicAutotoolsPlugin *plugin, GFile *directory)
{
	GFile *build_file;

	if ((plugin->project_root_dir == NULL) || (plugin->project_build_dir == NULL))
	{
		/* No change if there is no project or no build directory */
		build_file = g_object_ref (directory);
	}
	else if (g_file_has_prefix (directory, plugin->project_build_dir) || g_file_equal (directory, plugin->project_build_dir))
	{
		/* No change, already in build directory */
		build_file = g_object_ref (directory);
	}
	else if (g_file_equal (directory, plugin->project_root_dir))
	{
		/* Use build directory instead of source directory */
		build_file = g_object_ref (plugin->project_build_dir);
	}
	else if (g_file_has_prefix (directory, plugin->project_root_dir))
	{
		/* Get corresponding file in build directory */
		gchar *relative;

		relative = g_file_get_relative_path (plugin->project_root_dir, directory);
		build_file = g_file_resolve_relative_path (plugin->project_build_dir, relative);
		g_free (relative);
	}
	else
	{
		/* File outside the project directory */
		build_file = g_object_ref (directory);
	}

	return build_file;
}

/* Return build path and target from a GFile */
GFile *
build_file_from_file (BasicAutotoolsPlugin *plugin, GFile *file, gchar **target)
{

	if (target != NULL) *target = NULL;

	if (file == NULL)
	{
		/* Use project root directory */
		return build_file_from_directory (plugin, plugin->project_root_dir);
	}
	else if (g_file_query_file_type (file, 0, NULL) == G_FILE_TYPE_DIRECTORY)
	{
		return build_file_from_directory (plugin, file);
	}
	else
	{
		GFile *parent = NULL;
		GFile *build_file;
		IAnjutaProjectManager* projman;

		projman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
					                            IAnjutaProjectManager,
					                            NULL);

		if (projman != NULL)
		{
			/* Use the project manager to find the group file */
			GFile *child;

			for (child = normalize_project_file (file, plugin->project_root_dir); child != NULL;)
			{
				GFile *group;
				AnjutaProjectNodeType type;

				type = ianjuta_project_manager_get_target_type (projman, child, NULL);
				if (type == ANJUTA_PROJECT_GROUP) break;
				group = ianjuta_project_manager_get_parent (projman, child, NULL);
				g_object_unref (child);
				child = group;
			}
			parent = child;
		}

		if (parent == NULL)
		{
			/* Fallback use parent directory */
			parent = g_file_get_parent (file);
		}

		if (parent != NULL)
		{
			if (target != NULL) *target = g_file_get_relative_path (parent, file);
			build_file = build_file_from_directory (plugin, parent);
			g_object_unref (parent);

			return build_file;
		}
		else
		{
			return NULL;
		}
	}
}

GFile *
build_object_from_file (BasicAutotoolsPlugin *plugin, GFile *file)
{
	GFile *object = NULL;
	IAnjutaProjectManager* projman;

	/* Check that the GFile is a regular file */
	if ((file == NULL) || (g_file_query_file_type (file, 0, NULL) == G_FILE_TYPE_DIRECTORY))
	{
		return NULL;
	}

	projman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
				                            IAnjutaProjectManager,
				                            NULL);
	if ((projman != NULL) && ianjuta_project_manager_is_open (projman, NULL))
	{
		/* Use the project manager to find the object file */
		GFile *norm_file;

		norm_file = normalize_project_file (file, plugin->project_root_dir);
		object = ianjuta_project_manager_get_parent (projman, norm_file, NULL);
		if (object != NULL)
		{
			if (ianjuta_project_manager_get_target_type (projman, object, NULL) != ANJUTA_PROJECT_OBJECT)
			{
				g_object_unref (object);
				object = NULL;
			}
		}
		g_object_unref (norm_file);
	}
	else
	{
		/* Use language plugin trying to find an object file */
		IAnjutaLanguage* langman =	anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
			                                                      IAnjutaLanguage,
			                                                      NULL);

		if (langman != NULL)
		{
			GFileInfo* file_info;

			file_info = g_file_query_info (file,
				                                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				                                          G_FILE_QUERY_INFO_NONE,
		    		                                      NULL,
		        		                                  NULL);
			if (file_info)
			{
				gint id = ianjuta_language_get_from_mime_type (langman,
			    	                                           g_file_info_get_content_type (file_info),
			        	                                       NULL);
				if (id > 0)
				{
					const gchar *obj_ext = ianjuta_language_get_make_target (langman, id, NULL);
					gchar *basename;
					gchar *ext;
					gchar *targetname;
					GFile *parent;

					basename = g_file_get_basename (file);
					ext = strrchr (basename, '.');
					if ((ext != NULL) && (ext != basename)) *ext = '\0';
					targetname = g_strconcat (basename, obj_ext, NULL);
					g_free (basename);
					parent = g_file_get_parent (file);
					object = g_file_get_child (parent, targetname);
					g_object_unref (parent);
					g_free (targetname);
				}
			}
			g_object_unref (file_info);
		}
	}

	return object;
}

/* Save & Build
 *---------------------------------------------------------------------------*/

static BuildContext*
build_execute_command (BasicAutotoolsPlugin* bplugin, BuildProgram *prog,
					   gboolean with_view, GError **err)
{
	BuildContext *context;
	gboolean ok;

	context = build_get_context (bplugin, prog->work_dir, with_view);

	build_set_command_in_context (context, prog);
	ok = build_execute_command_in_context (context, err);

	if (ok)
	{
		return context;
	}
	else
	{
		build_context_destroy (context);

		return NULL;
	}
}

static BuildContext*
build_save_and_execute_command (BasicAutotoolsPlugin* bplugin, BuildProgram *prog,
								gboolean with_view, GError **err)
{
	BuildContext *context;

	context = build_get_context (bplugin, prog->work_dir, with_view);

	build_set_command_in_context (context, prog);
	if (!build_save_and_execute_command_in_context (context, err))
	{
		build_context_destroy (context);
		context = NULL;
	}

	return context;
}

static void
build_execute_after_command (GObject *sender,
							   IAnjutaBuilderHandle handle,
							   GError *error,
							   gpointer user_data)
{
	BuildProgram *prog = (BuildProgram *)user_data;
	BuildContext *context = (BuildContext *)handle;

	/* Try next command even if the first one fail (make distclean return an error) */
	if ((error == NULL) || (error->code != IANJUTA_BUILDER_ABORTED))
	{
		build_set_command_in_context (context, prog);
		build_execute_command_in_context (context, NULL);
	}
	else
	{
		build_program_free (prog);
	}
}

static BuildContext*
build_save_distclean_and_execute_command (BasicAutotoolsPlugin* bplugin, BuildProgram *prog,
								gboolean with_view, GError **err)
{
	BuildContext *context;
	gchar *root_path;
	gboolean same;
	BuildConfiguration *config;
	GList *vars;

	context = build_get_context (bplugin, prog->work_dir, with_view);
	root_path = g_file_get_path (bplugin->project_root_dir);
	same = strcmp (prog->work_dir, root_path) != 0;
	g_free (root_path);

	config = build_configuration_list_get_selected (bplugin->configurations);
	vars = build_configuration_get_variables (config);

	if (!same && directory_has_file (bplugin->project_root_dir, "config.status"))
	{
		BuildProgram *new_prog;

		// Need to run make clean before
		if (!anjuta_util_dialog_boolean_question (GTK_WINDOW (ANJUTA_PLUGIN (bplugin)->shell), _("Before using this new configuration, the default one needs to be removed. Do you want to do that ?"), NULL))
		{
			if (err)
				*err = g_error_new (ianjuta_builder_error_quark (),
				                   IANJUTA_BUILDER_CANCELED,
				                   _("Command canceled by user"));

			return NULL;
		}
		new_prog = build_program_new_with_command (bplugin->project_root_dir,
										   "%s",
										   CHOOSE_COMMAND (bplugin, DISTCLEAN));
		build_program_set_callback (new_prog, build_execute_after_command, prog);
		prog = new_prog;
	}
	build_program_add_env_list (prog, vars);

	build_set_command_in_context (context, prog);

	build_save_and_execute_command_in_context (context, NULL);

	return context;
}


/* Build commands
 *---------------------------------------------------------------------------*/

BuildContext*
build_build_file_or_dir (BasicAutotoolsPlugin *plugin,
						 GFile *file,
						 IAnjutaBuilderCallback callback, gpointer user_data,
						 GError **err)
{
	GFile *build_dir;
	gchar *target;
	BuildProgram *prog;
	BuildContext *context;
	BuildConfiguration *config;
	GList *vars;

	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

	build_dir = build_file_from_file (plugin, file, &target);
	prog = build_program_new_with_command (build_dir,
										   "%s %s",
										   CHOOSE_COMMAND (plugin, BUILD),
										   target ? target : "");
	build_program_set_callback (prog, callback, user_data);
	build_program_add_env_list (prog, vars);

	context = build_save_and_execute_command (plugin, prog, TRUE, err);
	g_free (target);
	g_object_unref (build_dir);

	return context;
}



BuildContext*
build_is_file_built (BasicAutotoolsPlugin *plugin, GFile *file,
					 IAnjutaBuilderCallback callback, gpointer user_data,
					 GError **err)
{
	GFile *build_dir;
	gchar *target;
	BuildProgram *prog;
	BuildContext *context;
	BuildConfiguration *config;
	GList *vars;


	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

	build_dir = build_file_from_file (plugin, file, &target);
	prog = build_program_new_with_command (build_dir,
										   "%s %s",
										   CHOOSE_COMMAND (plugin, IS_BUILT),
										   target ? target : "");
	build_program_set_callback (prog, callback, user_data);
	build_program_add_env_list (prog, vars);

	context = build_save_and_execute_command (plugin, prog, FALSE, err);

	g_free (target);
	g_object_unref (build_dir);

	return context;
}



static gchar*
get_root_install_command(BasicAutotoolsPlugin *bplugin)
{
	GSettings* settings = bplugin->settings;
	if (g_settings_get_boolean (settings, PREF_INSTALL_ROOT))
	{
		gchar* command = g_settings_get_string (settings, PREF_INSTALL_ROOT_COMMAND);
		return command;
	}
	else
		return g_strdup("");
}

BuildContext*
build_install_dir (BasicAutotoolsPlugin *plugin, GFile *dir,
                   IAnjutaBuilderCallback callback, gpointer user_data,
				   GError **err)
{
	BuildContext *context;
	gchar* root = get_root_install_command(plugin);
	GFile *build_dir;
	BuildProgram *prog;
	GString *command;
	BuildConfiguration *config;
	GList *vars;

	if ((root != NULL) && (*root != '\0'))
	{
		gchar *first = root;
		gchar *ptr = root;

		/* Replace %s or %q by respectively, the install command or the
		 * quoted install command. % character can be escaped by using two %. */
		command = g_string_new (NULL);
		while (*ptr)
		{
			if (*ptr++ == '%')
			{
				if (*ptr == 's')
				{
					/* Not quoted command */
					g_string_append_len (command, first, ptr - 1 - first);
					g_string_append (command, CHOOSE_COMMAND (plugin, INSTALL));
					first = ptr + 1;
				}
				else if (*ptr == 'q')
				{
					/* quoted command */
					gchar *quoted;

					quoted = g_shell_quote (CHOOSE_COMMAND (plugin, INSTALL));
					g_string_append_len (command, first, ptr - 1 - first);
					g_string_append (command, quoted);
					g_free (quoted);
					first = ptr + 1;
				}
				else if (*ptr == '%')
				{
					/* escaped % */
					g_string_append_len (command, first, ptr - 1 - first);
					first = ptr;
				}
				ptr++;
			}
		}
		g_string_append (command, first);
	}
	else
	{
		command = g_string_new (CHOOSE_COMMAND (plugin, INSTALL));
	}


	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

 	build_dir = build_file_from_file (plugin, dir, NULL);
	prog = build_program_new_with_command (build_dir,
		                                   "%s",
        		                           command->str);
	build_program_set_callback (prog, callback, user_data);
	build_program_add_env_list (prog, vars);

	context = build_save_and_execute_command (plugin, prog, TRUE, err);

	g_string_free (command, TRUE);
	g_object_unref (build_dir);
	g_free (root);

	return context;
}



BuildContext*
build_clean_dir (BasicAutotoolsPlugin *plugin, GFile *file,
				 GError **err)
{
	BuildContext *context = NULL;
	BuildProgram *prog;
	GFile *build_dir;
	BuildConfiguration *config;
	GList *vars;

	if (is_configured (plugin, file))
	{
		config = build_configuration_list_get_selected (plugin->configurations);
		vars = build_configuration_get_variables (config);

		build_dir = build_file_from_file (plugin, file, NULL);

		prog = build_program_new_with_command (build_dir,
		                                       "%s",
		                                       CHOOSE_COMMAND (plugin, CLEAN)),
		build_program_add_env_list (prog, vars);

		context = build_execute_command (plugin, prog, TRUE, err);
		g_object_unref (build_dir);
	}

	return context;
}



static void
build_remove_build_dir (GObject *sender,
						IAnjutaBuilderHandle context,
						GError *error,
						gpointer user_data)
{
	/* FIXME: Should we remove build dir on distclean ? */
}

BuildContext*
build_distclean (BasicAutotoolsPlugin *plugin)
{
	BuildContext *context;
	BuildProgram *prog;
	BuildConfiguration *config;
	GList *vars;


	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

	prog = build_program_new_with_command (plugin->project_build_dir,
										   "%s",
										   CHOOSE_COMMAND (plugin, DISTCLEAN));
	build_program_set_callback (prog, build_remove_build_dir, plugin);
	build_program_add_env_list (prog, vars);

	context = build_execute_command (plugin, prog, TRUE, NULL);

	return context;
}



BuildContext*
build_tarball (BasicAutotoolsPlugin *plugin)
{
	BuildContext *context;
	BuildProgram *prog;
	BuildConfiguration *config;
	GList *vars;

	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

	prog = build_program_new_with_command (plugin->project_build_dir,
	                                       "%s",
	                                       CHOOSE_COMMAND (plugin, BUILD_TARBALL)),
	build_program_add_env_list (prog, vars);

	context = build_save_and_execute_command (plugin, prog, TRUE, NULL);

	return context;
}

BuildContext*
build_compile_file (BasicAutotoolsPlugin *plugin, GFile *file)
{
	BuildContext *context = NULL;
	BuildProgram *prog;
	GFile *object;
	gchar *target_name;

	g_return_val_if_fail (file != NULL, FALSE);

	object = build_object_from_file (plugin, file);
	if (object != NULL)
	{
		GFile *build_dir;
		BuildConfiguration *config;
		GList *vars;

		config = build_configuration_list_get_selected (plugin->configurations);
		vars = build_configuration_get_variables (config);

		/* Find target directory */
		build_dir = build_file_from_file (plugin, object, &target_name);

		prog = build_program_new_with_command (build_dir, "%s %s",
	    	                                   CHOOSE_COMMAND(plugin, COMPILE),
	        	                               (target_name == NULL) ? "" : target_name);
		g_free (target_name);
		g_object_unref (build_dir);

		build_program_add_env_list (prog, vars);

		context = build_save_and_execute_command (plugin, prog, TRUE, NULL);
		g_object_unref (object);
	}
	else
	{
		/* FIXME: Prompt the user to create a Makefile with a wizard
		   (if there is no Makefile in the directory) or to add a target
		   rule in the above hash table, eg. editing the preferences, if
		   there is target extension defined for that file extension.
		*/
		GtkWindow *window;
		gchar *filename;

		filename = g_file_get_path (file);
		window = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
		anjuta_util_dialog_error (window, _("Cannot compile \"%s\": No compile rule defined for this file type."), filename);
		g_free (filename);
	}

	return context;
}


void
build_project_configured (GObject *sender,
							IAnjutaBuilderHandle handle,
							GError *error,
							gpointer user_data)
{
	BuildConfigureAndBuild *pack = (BuildConfigureAndBuild *)user_data;

	if (error == NULL)
	{
		BuildContext *context = (BuildContext *)handle;
		BasicAutotoolsPlugin *plugin = (BasicAutotoolsPlugin *)(context == NULL ? (void *)sender : (void *)build_context_get_plugin (context));
		GValue *value;
		gchar *uri;

		/* FIXME: check if build directory correspond, configuration could have changed */
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);

		uri = build_configuration_list_get_build_uri (plugin->configurations, build_configuration_list_get_selected (plugin->configurations));
		g_value_set_string (value, uri);
		g_free (uri);

		anjuta_shell_add_value (ANJUTA_PLUGIN (plugin)->shell, IANJUTA_BUILDER_ROOT_URI, value, NULL);

		build_update_configuration_menu (plugin);

		/* Call build function if necessary */
		if ((pack) && (pack->func != NULL)) pack->func (plugin, pack->file, pack->callback, pack->user_data, NULL);
	}

	if (pack)
	{
		g_free (pack->args);
		if (pack->file != NULL) g_object_unref (pack->file);
		g_free (pack);
	}
}

BuildContext*
build_configure_dir (BasicAutotoolsPlugin *plugin, GFile *dir, const gchar *args,
                     BuildFunc func, GFile *file,
                     IAnjutaBuilderCallback callback, gpointer user_data, GError **error)
{
	BuildContext *context;
	BuildProgram *prog;
	BuildConfiguration *config;
	GList *vars;
	BuildConfigureAndBuild *pack = g_new0 (BuildConfigureAndBuild, 1);
	gchar *quote;
	gchar *root_path;

	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

	root_path = g_file_get_path (plugin->project_root_dir);
	quote = shell_quotef ("%s%s%s",
		       	root_path,
		       	G_DIR_SEPARATOR_S,
			CHOOSE_COMMAND (plugin, CONFIGURE));

	prog = build_program_new_with_command (dir,
										   "%s %s",
										   quote,
										   args);
	g_free (quote);
	g_free (root_path);

	pack->args = NULL;
	pack->func = func;
	pack->file = (file != NULL) ? g_object_ref (file) : NULL;
	pack->callback = callback;
	pack->user_data = user_data;
	build_program_set_callback (prog, build_project_configured, pack);
	build_program_add_env_list (prog, vars);

	context = build_save_distclean_and_execute_command (plugin, prog, TRUE, NULL);

	return context;
}



static void
build_configure_after_autogen (GObject *sender,
							   IAnjutaBuilderHandle handle,
							   GError *error,
							   gpointer user_data)
{
	BuildConfigureAndBuild *pack = (BuildConfigureAndBuild *)user_data;

	if (error == NULL)
	{
		BuildContext *context = (BuildContext *)handle;
		BuildConfiguration *config;
		GList *vars;
		BasicAutotoolsPlugin *plugin = (BasicAutotoolsPlugin *)build_context_get_plugin (context);
		struct stat conf_stat, log_stat;
		gchar *root_path;
		gchar *filename;
		gboolean has_configure;

		root_path = g_file_get_path (plugin->project_root_dir);
		filename = g_build_filename (root_path, "configure", NULL);
		has_configure = stat (filename, &conf_stat) == 0;
		g_free (filename);

		if (has_configure)
		{
			gboolean older;

			config = build_configuration_list_get_selected (plugin->configurations);
			vars = build_configuration_get_variables (config);

			filename = g_build_filename (build_context_get_work_dir (context), "config.status", NULL);
			older =(stat (filename, &log_stat) != 0) || (log_stat.st_mtime < conf_stat.st_mtime);
			g_free (filename);

			if (older)
			{
				/* configure has not be run, run it */
				BuildProgram *prog;
				gchar *quote;
				GFile *work_file;

				quote = shell_quotef ("%s%s%s",
					     	root_path,
					       	G_DIR_SEPARATOR_S,
					       	CHOOSE_COMMAND (plugin, CONFIGURE));

				work_file = g_file_new_for_path (build_context_get_work_dir (context));
				prog = build_program_new_with_command (work_file,
													   "%s %s",
													   quote,
													   pack != NULL ? pack->args : NULL);
				g_object_unref (work_file);
				g_free (quote);
				build_program_set_callback (prog, build_project_configured, pack);
				build_program_add_env_list (prog, vars);

				build_set_command_in_context (context, prog);
				build_execute_command_in_context (context, NULL);
			}
			else
			{
				/* run next command if needed */
				build_project_configured (sender, handle, NULL, pack);
			}

			g_free (root_path);
			return;
		}
		else
		{
			anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell), _("Cannot configure project: Missing configure script in %s."), root_path);
			g_free (root_path);
		}
	}

	if (pack)
	{
		g_free (pack->args);
		if (pack->file != NULL) g_object_unref (pack->file);
		g_free (pack);
	}
}

BuildContext*
build_generate_dir (BasicAutotoolsPlugin *plugin, GFile *dir, const gchar *args,
                    BuildFunc func, GFile *file,
                    IAnjutaBuilderCallback callback, gpointer user_data, GError **error)
{
	BuildContext *context;
	BuildProgram *prog;
	BuildConfiguration *config;
	GList *vars;
	BuildConfigureAndBuild *pack = g_new0 (BuildConfigureAndBuild, 1);


	config = build_configuration_list_get_selected (plugin->configurations);
	vars = build_configuration_get_variables (config);

	if (directory_has_file (plugin->project_root_dir, "autogen.sh"))
	{
		gchar *quote;
		gchar *root_path = g_file_get_path (plugin->project_root_dir);

		quote = shell_quotef ("%s%s%s",
				root_path,
			       	G_DIR_SEPARATOR_S,
			       	CHOOSE_COMMAND (plugin, GENERATE));
		prog = build_program_new_with_command (dir,
											   "%s %s",
											   quote,
											   args);
		g_free (quote);
		g_free (root_path);
	}
	else
	{
		prog = build_program_new_with_command (dir,
											   "%s %s",
											   CHOOSE_COMMAND (plugin, AUTORECONF),
											   args);
	}
	pack->args = g_strdup (args);
	pack->func = func;
	pack->file = (file != NULL) ? g_object_ref (file) : NULL;
	pack->callback = callback;
	pack->user_data = user_data;
	build_program_set_callback (prog, build_configure_after_autogen, pack);
	build_program_add_env_list (prog, vars);

	context = build_save_distclean_and_execute_command (plugin, prog, TRUE, NULL);

	return context;
}

BuildContext*
build_configure_dialog (BasicAutotoolsPlugin *plugin, BuildFunc func, GFile *file, IAnjutaBuilderCallback callback, gpointer user_data, GError **error)
{
	GtkWindow *parent;
	gboolean run_autogen = FALSE;
	const gchar *project_root;
	GValue value = {0,};
	const gchar *old_config_name;
	BuildContext* context = NULL;

	run_autogen = !directory_has_file (plugin->project_root_dir, "configure");

	anjuta_shell_get_value (ANJUTA_PLUGIN (plugin)->shell, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI, &value, NULL);

	/* In case, a project is not loaded */
	if (!G_VALUE_HOLDS_STRING (&value)) return NULL;

	project_root = g_value_get_string (&value);
	parent = GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell);

	old_config_name = build_configuration_get_name (build_configuration_list_get_selected (plugin->configurations));
	if (build_dialog_configure (parent, project_root, plugin->configurations, &run_autogen))
	{
		BuildConfiguration *config;
		GFile *build_file;
		gchar *build_uri;
		const gchar *args;

		config = build_configuration_list_get_selected (plugin->configurations);
		build_uri = build_configuration_list_get_build_uri (plugin->configurations, config);
		build_file = g_file_new_for_uri (build_uri);
		g_free (build_uri);

		args = build_configuration_get_args (config);

		if (run_autogen)
		{
			context = build_generate_dir (plugin, build_file, args, func, file, callback, user_data, error);
		}
		else
		{
			context = build_configure_dir (plugin, build_file, args, func, file, callback, user_data, error);
		}
		g_object_unref (build_file);

		if (context == NULL)
		{
			/* Restore previous configuration */
			build_configuration_list_select (plugin->configurations, old_config_name);
		}
	}

	return context;
}

/* Run configure if needed and then the build command */
BuildContext*
build_configure_and_build (BasicAutotoolsPlugin *plugin, BuildFunc func, GFile *file, IAnjutaBuilderCallback callback, gpointer user_data, GError **error)
{
	if (!is_configured (plugin, file))
	{
		/* Run configure first */
		return build_configure_dialog (plugin, func, file, callback, user_data, error);
	}
	else
	{
		/* Some build functions have less arguments but
		 * it is not a problem in C */
		return func (plugin, file, callback, user_data, error);
	}
}

/* Configuration commands
 *---------------------------------------------------------------------------*/

GList*
build_list_configuration (BasicAutotoolsPlugin *plugin)
{
	BuildConfiguration *cfg;
	GList *list = NULL;

	for (cfg = build_configuration_list_get_first (plugin->configurations); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		const gchar *name = build_configuration_get_name (cfg);

		if (name != NULL) list = g_list_prepend (list, (gpointer)name);
	}

	return list;
}

const gchar*
build_get_uri_configuration (BasicAutotoolsPlugin *plugin, const gchar *uri)
{
	BuildConfiguration *cfg;
	BuildConfiguration *uri_cfg = NULL;
	gsize uri_len = 0;

	/* Check all configurations as other configuration directories are
	 * normally child of default configuration directory */
	for (cfg = build_configuration_list_get_first (plugin->configurations); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		const gchar *root = build_configuration_list_get_build_uri  (plugin->configurations, cfg);
		gsize len = root != NULL ? strlen (root) : 0;

		if ((len > uri_len) && (strncmp (uri, root, len) == 0))
		{
			uri_cfg = cfg;
			uri_len = len;
		}
	}

	return uri_len == 0 ? NULL : build_configuration_get_name (uri_cfg);
}


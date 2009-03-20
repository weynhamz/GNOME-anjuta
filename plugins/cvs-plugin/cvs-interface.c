/*
 * cvs-interface.c (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <libanjuta/anjuta-debug.h>


#include "cvs-execute.h"
#include "cvs-callbacks.h"
#include "cvs-interface.h"
#include "plugin.h"
#include "libgen.h"

static gchar* create_cvs_command_with_cvsroot(AnjutaPreferences* prefs,
								const gchar* action, 
								const gchar* command_options,
								const gchar* command_arguments,
								const gchar* cvsroot)
{
	gchar* cvs;
	gchar* global_options = NULL;
	gboolean ignorerc;
	gint compression;
	gchar* command;
	/* command global_options cvsroot action command_options command_arguments */
	gchar* CVS_FORMAT = "%s %s %s %s %s %s";
	
	g_return_val_if_fail (prefs != NULL, NULL);
	g_return_val_if_fail (action != NULL, NULL);
	g_return_val_if_fail (command_options != NULL, NULL);
	g_return_val_if_fail (command_arguments != NULL, NULL);
	
	cvs = anjuta_preferences_get(prefs, "cvs.path");
	compression = anjuta_preferences_get_int(prefs, "cvs.compression");
	ignorerc = anjuta_preferences_get_bool(prefs, "cvs.ignorerc");
	if (compression && ignorerc)
		global_options = g_strdup_printf("-f -z%d", compression);
	else if (compression)
		global_options = g_strdup_printf("-z%d", compression);
	else if (ignorerc)
		global_options = g_strdup("-f");
	else
		global_options = g_strdup("");
	if (cvsroot == NULL)
	{
		cvsroot = "";
	}
	command = g_strdup_printf(CVS_FORMAT, cvs, global_options, cvsroot, action,  
								command_options, command_arguments);
	g_free (cvs);
	g_free (global_options);
	
	return command;
}

inline static gchar* create_cvs_command(AnjutaPreferences* prefs,
								const gchar* action, 
								const gchar* command_options,
								const gchar* command_arguments)
{
	return create_cvs_command_with_cvsroot(prefs, action, command_options,
		command_arguments, NULL);
}

static void add_option(gboolean value, GString* options, const gchar* argument)
{
	if (value)
	{
			g_string_append(options, " ");
			g_string_append(options, argument);
	}
}

static gboolean is_directory(const gchar* filename)
{
	GFile* file;
	GFileType type;
	GFileInfo *file_info;

	// FIXME check if filename can only be local file here
	file = g_file_new_for_path(filename);
	file_info = g_file_query_info (file, 
			G_FILE_ATTRIBUTE_STANDARD_TYPE,
			G_FILE_QUERY_INFO_NONE,
			NULL, NULL);
	if (file_info == NULL)
	{
		g_object_unref (G_OBJECT (file));
		return FALSE;
	}

	type = g_file_info_get_attribute_uint32 (file_info, 
			G_FILE_ATTRIBUTE_STANDARD_TYPE);
	g_object_unref(G_OBJECT(file_info));
	g_object_unref(G_OBJECT(file));
	
	return type == G_FILE_TYPE_DIRECTORY ? TRUE : FALSE;
}

void anjuta_cvs_add (AnjutaPlugin *obj, const gchar* filename, 
	gboolean binary, GError **err)
{
	gchar* command;
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	GString* options = g_string_new("");
	gchar* file = g_strdup(filename);
	
	add_option(binary, options, "-kb");
	
	
	command = create_cvs_command(
		anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
		NULL), "add", options->str, basename(file));
	
	cvs_execute(plugin, command, dirname(file));
	g_free(command);
	g_free(file);
	g_string_free(options, TRUE);
}

void anjuta_cvs_commit (AnjutaPlugin *obj, const gchar* filename, const gchar* log,
	const gchar* rev, gboolean recurse, GError **err)
{
	GString* options = g_string_new("");
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	gchar* command;	
	
	if (strlen(log))
		g_string_printf(options, "-m '%s'", log);
	else
		g_string_printf(options, "-m 'no log message'");
	
	if (strlen(rev))
	{
		g_string_append_printf(options, " -r %s", rev);
	}
	add_option(!recurse, options, "-l");
	
	if (!is_directory(filename))
	{
		gchar* file = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
			NULL), "commit", options->str, basename(file));
		cvs_execute(plugin, command, dirname(file));
		g_free(file);
	}
	else
	{
		gchar* dir = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
			NULL), "commit", options->str, "");
		cvs_execute(plugin, command, dir);
		g_free(dir);
	}
	g_free(command);
	g_string_free(options, TRUE);
}

void anjuta_cvs_diff (AnjutaPlugin *obj, const gchar* filename, const gchar* rev, 
	gboolean recurse, gboolean patch_style, gboolean unified, GError **err)
{
	GString* options = g_string_new("");
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	gchar* command;	
	
	if (strlen(rev))
	{
		g_string_append_printf(options, " -r %s", rev);
	}
	add_option(!recurse, options, "-l");
	add_option(unified, options, "-u");
	
	if (!is_directory(filename))
	{
		gchar* file = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "diff", options->str, basename(file));
		cvs_execute_diff(plugin, command, dirname(file));
	}
	else
	{
		gchar* dir = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "diff", options->str, "");
		cvs_execute_diff(plugin, command, dir);
		g_free(dir);
	}
	g_free(command);
	g_string_free(options, TRUE);
}

void anjuta_cvs_log (AnjutaPlugin *obj, const gchar* filename, gboolean recurse, gboolean verbose, GError **err)
{
	GString* options = g_string_new("");
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	gchar* command;	

	add_option(!recurse, options, "-l");
	add_option(!verbose, options, "-h");
	
	if (!is_directory(filename))
	{
		gchar* file = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "log", options->str, basename(file));
		cvs_execute_log(plugin, command, dirname(file));
		g_free(file);
	}
	else
	{
		gchar* dir = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "log", options->str, "");
		cvs_execute_log(plugin, command, dir);
		g_free(dir);
	}
	g_free(command);
	g_string_free(options, TRUE);
}

void anjuta_cvs_remove (AnjutaPlugin *obj, const gchar* filename, GError **err)
{
	gchar* command;
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	GString* options = g_string_new("");
	gchar* file = g_strdup(filename);	

	command = create_cvs_command(
		anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
		NULL), "remove", options->str, basename(file));
		
	cvs_execute(plugin, command, dirname(file));
	g_free(file);
	g_free(command);
	g_string_free(options, TRUE);
}

void anjuta_cvs_status (AnjutaPlugin *obj, const gchar* filename, gboolean recurse, gboolean verbose, GError **err)
{
	gchar* command;
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	GString* options = g_string_new("");
	
	add_option(!recurse, options, "-l");
	add_option(verbose, options, "-v");
	
	if (!is_directory(filename))
	{
		gchar* file = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "status", options->str, basename(file));
		cvs_execute_status(plugin, command, dirname(file));
		g_free(file);
	}
	else
	{
		gchar* dir = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "status", options->str, "");
		cvs_execute_status(plugin, command, dir);
		g_free(dir);
	}
	g_free(command);
	g_string_free(options, TRUE);
}

void anjuta_cvs_update (AnjutaPlugin *obj, const gchar* filename, gboolean recurse, 
	gboolean prune, gboolean create, gboolean reset_sticky, const gchar* revision, GError **err)
{
	GString* options = g_string_new("");
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	gchar* command;	
	
	add_option(!recurse, options, "-l");
	add_option(prune, options, "-P");
	add_option(create, options, "-d");
	if (strlen(revision))
	{
		g_string_append_printf(options, " -r %s", revision);
	}
	else
	{
		add_option(reset_sticky, options, "-A");
	}
	
	if (!is_directory(filename))
	{
		gchar* file = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "update", options->str, basename(file));
		cvs_execute(plugin, command, dirname(file));
		g_free(file);
	}
	else
	{
		gchar* dir = g_strdup(filename);
		command = create_cvs_command(anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell,
								 NULL), "update", options->str, "");
		cvs_execute(plugin, command, dir);
	}
	g_free(command);
	g_string_free(options, TRUE);
}

void anjuta_cvs_import (AnjutaPlugin *obj, const gchar* dir, const gchar* cvsroot,
	const gchar* module, const gchar* vendor, const gchar* release,
	const gchar* log, gint server_type, const gchar* username, const 
	gchar* password, GError** error)
{
	gchar* cvs_command;
	gchar* root;
	GString* options = g_string_new("");
	CVSPlugin* plugin = ANJUTA_PLUGIN_CVS (obj);
	
	switch (server_type)
	{
		case SERVER_LOCAL:
		{
			root = g_strdup_printf("-d %s", cvsroot);
			break;
		}
		case SERVER_EXTERN:
		{
			root = g_strdup_printf("-d:ext:%s@%s", username, cvsroot);
			break;
		}
		case SERVER_PASSWORD:
		{
			root = g_strdup_printf("-d:pserver:%s:%s@%s", 
				username, password, cvsroot);
			break;
		}
		default:
		{
			DEBUG_PRINT("%s", "Invalid cvs server type!");
			g_string_free (options, TRUE);
			return;
		}
	}
	g_string_printf(options, "-m '%s'", log);
	g_string_append_printf(options, " %s %s %s", module, vendor, release);
	
	cvs_command = create_cvs_command_with_cvsroot(
		anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell, NULL),
		"import", options->str, "", root);
	cvs_execute(plugin, cvs_command, dir);
	
	g_string_free(options, TRUE);
	g_free(cvs_command);
}

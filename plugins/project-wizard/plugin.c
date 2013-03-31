/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

/*
 * Plugins functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "plugin.h"

#include "druid.h"
#include "tar.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-file.h>


/* Private functions
 *---------------------------------------------------------------------------*/

static void
npw_open_project_template (GFile *destination, GFile *tarfile, gpointer data, GError *error)
{
	NPWPlugin *plugin = (NPWPlugin *)data;

	if (error != NULL)
	{
		gchar *tarname = g_file_get_path (tarfile);

		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),_("Unable to extract project template %s: %s"), tarname, error->message);
	}
	else
	{
		/* Show the project wizard dialog, loading only the new projects */
		npw_plugin_show_wizard (plugin, destination);
	}
}

static gboolean
npw_install_project_template_with_callback (NPWPlugin *plugin, GFile *file, NPWTarCompleteFunc callback, GError **error)
{
	GFileInputStream *stream;
	gchar *name;
	gchar *ext;
	gchar *path;
	GFile *dest;
	gboolean ok;
	GError *err = NULL;

	/* Check if tarfile exist */
	stream = g_file_read (file, NULL, error);
	if (stream == NULL)
	{
		return FALSE;
	}
	g_input_stream_close (G_INPUT_STREAM (stream), NULL, NULL);

	/* Get name without extension */
	name = g_file_get_basename (file);
	ext = strchr (name, '.');
	if (ext != NULL) *ext = '\0';

	/* Create a directory for template */
	path = g_build_filename (g_get_user_data_dir (), "anjuta", "templates", name, NULL);
	g_free (name);
	dest = g_file_new_for_path (path);
	g_free (path);
	ok = g_file_make_directory_with_parents (dest, NULL, &err);
	if (err != NULL)
	{
		if (err->code == G_IO_ERROR_EXISTS)
		{
			/* Allow to overwrite directories */
			ok = TRUE;
			g_error_free (err);
		}
		else
		{
			g_object_unref (dest);

			return FALSE;
		}
	}

	ok = npw_tar_extract (dest, file, callback, plugin, error);
	g_object_unref (dest);

	return ok;
}

/* Public functions
 *---------------------------------------------------------------------------*/

/* Display the project wizard selection dialog using only templates in the
 * specified directory if non NULL */
gboolean
npw_plugin_show_wizard (NPWPlugin *plugin, GFile *project)
{
	if (plugin->install != NULL)
	{
		/* New project wizard is busy copying project file */
	}
	else if (plugin->druid == NULL)
	{
		/* Create a new project wizard druid */
		npw_druid_new (plugin, project);
	}

	if (plugin->druid != NULL)
	{
		/* New project wizard druid is waiting for user inputs */
		npw_druid_show (plugin->druid);
	}


	return TRUE;
}

/*---------------------------------------------------------------------------*/

/* Used in dispose */
static gpointer parent_class;

static void
npw_plugin_instance_init (GObject *obj)
{
	NPWPlugin *plugin = ANJUTA_PLUGIN_NPW (obj);

	plugin->druid = NULL;
	plugin->install = NULL;
	plugin->view = NULL;
}

/* dispose is used to unref object created with instance_init */

static void
npw_plugin_dispose (GObject *obj)
{
	NPWPlugin *plugin = ANJUTA_PLUGIN_NPW (obj);

	/* Warning this function could be called several times */
	if (plugin->view != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (plugin->view),
									  (gpointer*)(gpointer)&plugin->view);
		plugin->view = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
npw_plugin_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* finalize used to free object created with instance init is not used */

static gboolean
npw_plugin_activate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("%s", "Project Wizard Plugin: Activating project wizard plugin...");
	return TRUE;
}

static gboolean
npw_plugin_deactivate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("%s", "Project Wizard Plugin: Deactivating project wizard plugin...");
	return TRUE;
}

static void
npw_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = npw_plugin_activate;
	plugin_class->deactivate = npw_plugin_deactivate;
	klass->dispose = npw_plugin_dispose;
	klass->finalize = npw_plugin_finalize;
}

/* IAnjutaWizard implementation
 *---------------------------------------------------------------------------*/

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	npw_plugin_show_wizard (ANJUTA_PLUGIN_NPW (wiz), NULL);
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

/* IAnjutaFile implementation
 *---------------------------------------------------------------------------*/

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **error)
{
	NPWPlugin *plugin = ANJUTA_PLUGIN_NPW (ifile);
	GFileInfo *info;

	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);
	if (info != NULL)
	{
		if (strcmp(g_file_info_get_content_type (info), "application/x-anjuta-project-template") == 0)
		{
			npw_plugin_show_wizard (plugin, file);
		}
		else
		{
			npw_install_project_template_with_callback (plugin, file, npw_open_project_template, error);
		}
		g_object_unref (info);
	}
}

static GFile*
ifile_get_file (IAnjutaFile* ifile, GError** error)
{
	return NULL;
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

ANJUTA_PLUGIN_BEGIN (NPWPlugin, npw_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (NPWPlugin, npw_plugin);

/* Control access to anjuta message view to avoid a closed view
 *---------------------------------------------------------------------------*/

static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *line,
						 NPWPlugin *plugin)
{
	npw_plugin_print_view (plugin, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "");
}

IAnjutaMessageView*
npw_plugin_create_view (NPWPlugin* plugin)
{
	if (plugin->view == NULL)
	{
		IAnjutaMessageManager* man;

		man = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										  IAnjutaMessageManager, NULL);
		plugin->view =
			ianjuta_message_manager_add_view (man, _("New Project Assistant"),
											  ICON_FILE, NULL);
		if (plugin->view != NULL)
		{
			g_signal_connect (G_OBJECT (plugin->view), "buffer_flushed",
							  G_CALLBACK (on_message_buffer_flush), plugin);
			g_object_add_weak_pointer (G_OBJECT (plugin->view),
									   (gpointer *)(gpointer)&plugin->view);
		}
	}
	else
	{
		ianjuta_message_view_clear (plugin->view, NULL);
	}

	return plugin->view;
}

void
npw_plugin_append_view (NPWPlugin* plugin, const gchar* text)
{
	if (plugin->view)
	{
		ianjuta_message_view_buffer_append (plugin->view, text, NULL);
	}
}

void
npw_plugin_print_view (NPWPlugin* plugin, IAnjutaMessageViewType type,
					   const gchar* summary, const gchar* details)
{
	if (plugin->view)
	{
		ianjuta_message_view_append (plugin->view, type, summary, details, NULL);
	}
}

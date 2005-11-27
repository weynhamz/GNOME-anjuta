/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    install.c
    Copyright (C) 2004 Sebastien Granjoux

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

/*
 * Handle installation of all files
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "install.h"

#include "plugin.h"
#include "file.h"
#include "parser.h"
#include "autogen.h"
#include "action.h"

#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

/*---------------------------------------------------------------------------*/

#define FILE_BUFFER_SIZE	4096

#define AUTOGEN_START_MACRO_LEN	7
#define AUTOGEN_MARKER_1	"autogen5"
#define AUTOGEN_MARKER_2	"template"

/*---------------------------------------------------------------------------*/

struct _NPWInstall
{
	NPWAutogen* gen;
	NPWFileListParser* parser;
	NPWFileList* list;
	const NPWFile* file;
	NPWActionListParser* action_parser;
	NPWActionList* action_list;
	const NPWAction* action;
	AnjutaLauncher* launcher;
	NPWPlugin* plugin;
	const gchar* project_file;
};

/*---------------------------------------------------------------------------*/

static void on_install_end_install_file (NPWAutogen* gen, gpointer data);
static void on_run_terminated (AnjutaLauncher* launcher, gint pid, gint status, gulong time, NPWInstall* this);
static gboolean npw_install_install_file (NPWInstall* this);
static gboolean npw_run_action (NPWInstall* this);
static gboolean npw_open_action (NPWInstall* this);

/* Helper functions
 *---------------------------------------------------------------------------*/

static gboolean
npw_is_autogen_template_file (FILE* tpl)
{
	gint len;
	const gchar* marker[] = {"", AUTOGEN_MARKER_1, AUTOGEN_MARKER_2, NULL};
	const gchar** key;
	const gchar* ptr;
	gint c;


	for (key = marker; *key != NULL; ++key)
	{
		/* Skip first whitespace */
		do
		{
			c = fgetc (tpl);
			if (c == EOF) return FALSE;
		}
		while (isspace (c));

		/* Test start marker, then autogen5, then template */
		ptr = *key;
		len = *ptr == '\0' ? AUTOGEN_START_MACRO_LEN : strlen (ptr);
		do
		{
			if (len == 0) return FALSE;
			--len;
			if ((*ptr != '\0') && (tolower (c) != *ptr++)) return FALSE;
			c = fgetc (tpl);
			/* FIXME: Test this EOF test */
			if (c == EOF) return FALSE;
		}
		while (!isspace (c));
		if ((**key != '\0') && (len != 0)) return FALSE;
	}

	return TRUE;
}

static gboolean
npw_is_autogen_template (const gchar* filename)
{
	FILE* tpl;
	gboolean autogen;

	tpl = fopen (filename, "rt");
	if (tpl == NULL) return FALSE;

	autogen = npw_is_autogen_template_file (tpl);
	fclose (tpl);

	return autogen;
}

static gboolean
npw_copy_file (const gchar* destination, const gchar* source)
{
	gchar* buffer;
	FILE* src;
	FILE* dst;
	guint len;
	gboolean ok;

	buffer = g_new (gchar, FILE_BUFFER_SIZE);

	/* Copy file */
	src = fopen (source, "rb");
	if (src == NULL)
	{
		return FALSE;
	}

	dst = fopen (destination, "wb");
	if (dst == NULL)
	{
		return FALSE;
	}

	ok = TRUE;
	for (;!feof (src);)
	{
		len = fread (buffer, 1, FILE_BUFFER_SIZE, src);
		if ((len != FILE_BUFFER_SIZE) && !feof (src))
		{
			ok = FALSE;
			break;
		}

		if (len != fwrite (buffer, 1, len, dst))
		{
			ok = FALSE;
			break;
		}
	}

	fclose (dst);
	fclose (src);

	g_free (buffer);

	return ok;
}	

/* Installer object
 *---------------------------------------------------------------------------*/

NPWInstall* npw_install_new (NPWPlugin* plugin)
{
	NPWInstall* this;

	/* Skip if already created */
	if (plugin->install != NULL) return plugin->install;

	this = g_new0(NPWInstall, 1);
	this->gen = npw_autogen_new ();
	this->plugin = plugin;
	npw_plugin_create_view (plugin);

	plugin->install = this;

	return this;
}

void npw_install_free (NPWInstall* this)
{
	if (this->parser != NULL)
	{
		npw_file_list_parser_free (this->parser);
	}
	if (this->list != NULL)
	{
		npw_file_list_free (this->list);
	}
	if (this->action_parser != NULL)
	{
		npw_action_list_parser_free (this->action_parser);
	}
	if (this->action_list != NULL)
	{
		npw_action_list_free (this->action_list);
	}
	if (this->launcher != NULL)
	{
		g_signal_handlers_disconnect_by_func (G_OBJECT (this->launcher), G_CALLBACK (on_run_terminated), this);
		g_object_unref (this->launcher);
	}
	npw_autogen_free (this->gen);
	this->plugin->install = NULL;
	g_free (this);
}


gboolean
npw_install_set_property (NPWInstall* this, NPWValueHeap* values)
{
	npw_autogen_write_definition_file (this->gen, values);

	return TRUE;
}

gboolean
npw_install_set_wizard_file (NPWInstall* this, const gchar* filename)
{
	if (this->list != NULL)
	{
		npw_file_list_free (this->list);
	}
	this->list = npw_file_list_new ();

	if (this->parser != NULL)
	{
		npw_file_list_parser_free (this->parser);
	}
	this->parser = npw_file_list_parser_new (this->list, filename);

	npw_autogen_set_input_file (this->gen, filename, "[+","+]");

	return TRUE;
}

static void
on_install_read_action_list (const gchar* output, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_action_list_parser_parse (this->action_parser, output, strlen (output), NULL);
}

static void
on_install_end_action (gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	for (;;)
	{
		if (this->action == NULL)
		{
			this->action = npw_action_list_first (this->action_list);
		}
		else
		{
			this->action = npw_action_next (this->action);
		}
		if (this->action == NULL)
		{
			DEBUG_PRINT ("Project wizard done");
			/* The wizard could have been deactivated when loading the new
			 * project. Hence, the following check.
			 */
			if (anjuta_plugin_is_active (ANJUTA_PLUGIN (this->plugin)))
				anjuta_plugin_deactivate (ANJUTA_PLUGIN (this->plugin));
			npw_install_free (this);
			return;
		}
		switch (npw_action_get_type (this->action))
		{
		case NPW_RUN_ACTION:
			npw_run_action (this);
			return;
		case NPW_OPEN_ACTION:
			npw_open_action (this);
			break;
		default:
			break;
		}
	}
}

static void
on_install_read_all_action_list (NPWAutogen* gen, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_action_list_parser_end_parse (this->action_parser, NULL);

	on_install_end_install_file (NULL, this);
}

static void
on_install_read_file_list (const gchar* output, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_file_list_parser_parse (this->parser, output, strlen (output), NULL);
}

static void
on_install_read_all_file_list (NPWAutogen* gen, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_file_list_parser_end_parse (this->parser, NULL);

	this->file = NULL;
	this->project_file = NULL;

	this->action_list = npw_action_list_new ();
	this->action_parser = npw_action_list_parser_new (this->action_list);
	npw_autogen_set_output_callback (this->gen, on_install_read_action_list, this);
	npw_autogen_execute (this->gen, on_install_read_all_action_list, this, NULL);
}

static void
on_install_end_install_file (NPWAutogen* gen, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	/* Warning gen could be invalid */
	
	for (;;)
	{
		if (this->file == NULL)
		{
			this->file = npw_file_list_first (this->list);
		}
		else
		{
			if (npw_file_get_execute (this->file))
			{
				gint previous;
				/* Make this file executable */
				previous = umask (0666);
				chmod (npw_file_get_destination (this->file), 0777 & ~previous);
				umask (previous);
			}
			if (npw_file_get_project (this->file))
			{
				/* Check if project is NULL */
				this->project_file = npw_file_get_destination (this->file);
			}
			this->file = npw_file_next (this->file);
		}
		if (this->file == NULL)
		{
			/* IAnjutaFileLoader* loader; */
			/* All files have been installed */
			npw_plugin_print_view (this->plugin,
								   IANJUTA_MESSAGE_VIEW_TYPE_INFO,
								   _("New project has been created successfully"),
								   "");

			/*if (this->project_file != NULL)
			{
				loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->plugin)->shell, IAnjutaFileLoader, NULL);
				if (loader)
				{
					ianjuta_file_loader_load (loader, this->project_file, FALSE, NULL);
				}
			}*/

			on_install_end_action (this);
			
			return;
		}
		switch (npw_file_get_type (this->file))
		{
		case NPW_FILE:
			npw_install_install_file (this);
			return;
		default:
			g_warning("Unknown file type %d\n", npw_file_get_type (this->file));
			break;
		}
	}
}

gboolean
npw_install_launch (NPWInstall* this)
{
	npw_autogen_set_output_callback (this->gen, on_install_read_file_list, this);
	npw_autogen_execute (this->gen, on_install_read_all_file_list, this, NULL);

	return TRUE;
}

static gboolean
npw_install_install_file (NPWInstall* this)
{
	gchar* buffer;
	gchar* sep;
	guint len;
	const gchar* destination;
	const gchar* source;
	gchar* msg;
	gboolean use_autogen;
	gboolean ok = TRUE;

	destination = npw_file_get_destination (this->file);
	source = npw_file_get_source (this->file);

	/* Check if file already exist */
	if (g_file_test (destination, G_FILE_TEST_EXISTS))
	{
		msg = g_strdup_printf (_("Skipping %s: file already exists"), destination);
		npw_plugin_print_view (this->plugin, IANJUTA_MESSAGE_VIEW_TYPE_WARNING, msg, "");
		g_free (msg);
		on_install_end_install_file (this->gen, this);

		return FALSE;
	}	

	/* Check if autogen is needed */
	switch (npw_file_get_autogen (this->file))
	{
	case NPW_TRUE:
		use_autogen = TRUE;
		break;
	case NPW_FALSE:
		use_autogen = FALSE;
		break;
	case NPW_DEFAULT:
		use_autogen = npw_is_autogen_template (source);
		break;
	default:
		use_autogen = FALSE;
	}

	if (use_autogen)
	{	
		msg = g_strdup_printf (_("Creating %s (using AutoGen)"), destination);
	}
	else
	{
		msg = g_strdup_printf (_("Creating %s"), destination);
	}
	npw_plugin_print_view (this->plugin, IANJUTA_MESSAGE_VIEW_TYPE_INFO, msg, "");
	g_free (msg);

	len = strlen (destination) + 1;	
	buffer = g_new (gchar, MAX (FILE_BUFFER_SIZE, len));
	strcpy (buffer, destination);			
	sep = buffer;
	for (;;)	
	{
		/* Get directory one by one */
		sep = strstr (sep,G_DIR_SEPARATOR_S);
		if (sep == NULL) break;
		/* Create directory if necessary */
		*sep = '\0';
		if ((*buffer != '~') && (*buffer != '\0'))
		{
			if (!g_file_test (buffer, G_FILE_TEST_EXISTS))
			{
				if (mkdir (buffer, 0755) == -1)
				{
					g_warning("Unable to create directory %s\n", buffer);
					break;
				}
			}
		}
		*sep++ = G_DIR_SEPARATOR_S[0];
	}

	if (ok)
	{
		if (use_autogen)
		{
			npw_autogen_set_input_file (this->gen, source, NULL, NULL);
			npw_autogen_set_output_file (this->gen, destination);
			npw_autogen_execute (this->gen, on_install_end_install_file, this, NULL);
		}
		else
		{
			npw_copy_file (destination, source);
			on_install_end_install_file (this->gen, this);
		}
	}

	return ok;
}

static void
on_run_terminated (AnjutaLauncher* launcher, gint pid, gint status, gulong time, NPWInstall* this)
{
	on_install_end_action (this);
}

static void
on_run_output (AnjutaLauncher* launcher, AnjutaLauncherOutputType type, const gchar* output, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_plugin_append_view (this->plugin, output);
}

static gboolean
npw_run_action (NPWInstall* this)
{
	gchar *msg;
	if (this->launcher == NULL)
	{
		this->launcher = anjuta_launcher_new ();
	}
	g_signal_connect (G_OBJECT (this->launcher), "child-exited", G_CALLBACK (on_run_terminated), this);
	msg = g_strconcat (_("Executing: "), npw_action_get_command (this->action), NULL);
	npw_plugin_print_view (this->plugin, IANJUTA_MESSAGE_VIEW_TYPE_INFO, msg, "");
	return anjuta_launcher_execute (this->launcher, npw_action_get_command (this->action), on_run_output, this);
}

static gboolean
npw_open_action (NPWInstall* this)
{
	IAnjutaFileLoader* loader;

	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->plugin)->shell, IAnjutaFileLoader, NULL);
	if (loader)
	{
		ianjuta_file_loader_load (loader, npw_action_get_file (this->action), FALSE, NULL);

		return TRUE;
	}

	return FALSE;
}

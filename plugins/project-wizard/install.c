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
//	Copy files 

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include "plugin.h"
#include "file.h"
#include "parser.h"
#include "install.h"
#include "autogen.h"



#define NPW_ICON ""
#define FILE_BUFFER_SIZE	4096
#define AUTOGEN_START_MACRO_LEN	7
#define AUTOGEN_MARKER_1	"autogen5"
#define AUTOGEN_MARKER_2	"template"


struct _NPWInstall
{
	NPWAutogen* gen;
	NPWFileListParser* parser;
	NPWFileList* list;
	const NPWFile* file;
	NPWPlugin* plugin;
	IAnjutaMessageView* view;
	gchar* project_file;
};

static void on_install_end_install_file(NPWAutogen* gen, gpointer data);
static gboolean npw_install_install_file(NPWInstall* this);

// Helper functions

static gboolean
npw_is_autogen_template_file(FILE* tpl)
{
	gint len;
	const gchar* marker[] = {"", AUTOGEN_MARKER_1, AUTOGEN_MARKER_2, NULL};
	const gchar** key;
	const gchar* ptr;
	gint c;


	for (key = marker; *key != NULL; ++key)
	{
		// Skip first whitespace
		do
		{
			c = fgetc(tpl);
			if (c == EOF) return FALSE;
		}
		while (isspace(c));

		// Test start marker, then autogen5, then template
		ptr = *key;
		len = *ptr == '\0' ? AUTOGEN_START_MACRO_LEN : strlen(ptr);
		do
		{
			if (len == 0) return FALSE;
			--len;
			if ((*ptr != '\0') && (tolower(c) != *ptr++)) return FALSE;
			c = fgetc(tpl);
			// FIXME: Test this EOF test
			if (c == EOF) return FALSE;
		}
		while (!isspace(c));
		if ((**key != '\0') && (len != 0)) return FALSE;
	}

	return TRUE;
}

static gboolean
npw_is_autogen_template(const gchar* filename)
{
	FILE* tpl;
	gboolean autogen;

	tpl = fopen(filename, "rt");
	if (tpl == NULL) return FALSE;

	autogen = npw_is_autogen_template_file(tpl);
	fclose(tpl);

	return autogen;
}

static gboolean
npw_copy_file(const gchar* destination, const gchar* source)
{
	gchar* buffer;
	FILE* src;
	FILE* dst;
	guint len;
	gboolean ok;

	buffer = g_new(gchar, FILE_BUFFER_SIZE);

	// Copy file
	src = fopen(source, "rb");
	if (src == NULL)
	{
		return FALSE;
	}

	dst = fopen(destination, "wb");
	if (dst == NULL)
	{
		return FALSE;
	}

	ok = TRUE;
	for(;!feof(src);)
	{
		len = fread(buffer, 1, FILE_BUFFER_SIZE, src);
		if ((len != FILE_BUFFER_SIZE) && !feof(src))
		{
			ok = FALSE;
			break;
		}

		if (len != fwrite(buffer, 1, len, dst))
		{
			ok = FALSE;
			break;
		}
	}

	fclose(dst);
	fclose(src);

	g_free(buffer);

	return ok;
}	

NPWInstall* npw_install_new(NPWPlugin* plugin)
{
	NPWInstall* this;

	// Skip if already created
	if (plugin->install != NULL) return plugin->install;

	this = g_new0(NPWInstall, 1);
	this->gen = npw_autogen_new();
	this->plugin = plugin;
	if (this->plugin->view == NULL)
	{
		IAnjutaMessageManager* man;

		man = anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell, IAnjutaMessageManager, NULL);
		// Create new view
		this->plugin->view = ianjuta_message_manager_add_view(man, _("New Project Wizard"), NPW_ICON, NULL); 	
	}
	this->view = this->plugin->view;
	//ianjuta_message_view_append(this->view, IANJUTA_MESSAGE_VIEW_TYPE_INFO, "Hi", "", NULL);
	//ianjuta_message_view_clear(this->view, NULL);

	plugin->install = this;

	return this;
}

void npw_install_destroy(NPWInstall* this)
{
	if (this->parser != NULL)
	{
		npw_file_list_parser_destroy(this->parser);
	}
	if (this->list != NULL)
	{
		npw_file_list_destroy(this->list);
	}
	npw_autogen_destroy(this->gen);
	this->plugin->install = NULL;
	g_free(this);
}


gboolean
npw_install_set_property(NPWInstall* this, GQueue* page_list, AnjutaPlugin* plugin)
{
	int i;

	// Generate definition file
	npw_autogen_add_default_definition(this->gen, anjuta_shell_get_preferences(plugin->shell, NULL));	
	for (i = 0; g_queue_peek_nth(page_list, i) != NULL; ++i)
	{
		npw_autogen_add_definition(this->gen, g_queue_peek_nth(page_list, i));
	}
	npw_autogen_write_definition_file(this->gen);

	return TRUE;
}

gboolean
npw_install_set_wizard_file(NPWInstall* this, const gchar* filename)
{
	if (this->list != NULL)
	{
		npw_file_list_destroy(this->list);
	}
	this->list = npw_file_list_new();

	if (this->parser != NULL)
	{
		npw_file_list_parser_destroy(this->parser);
	}
	this->parser = npw_file_list_parser_new(this->list, filename);

	npw_autogen_set_input_file(this->gen, filename, "[+","+]");

	return TRUE;
}

static void
on_install_gen_file_list(const gchar* output, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_file_list_parser_parse(this->parser, output, strlen(output), NULL);
}

static void
on_install_use_file_list(NPWAutogen* gen, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	npw_file_list_parser_end_parse(this->parser, NULL);

	this->file = NULL;
	this->project_file = NULL;
	on_install_end_install_file(gen, this);
	
//	npw_file_list_install(this->list);

//	npw_install_destroy(this);
}

static void
on_install_end_install_file(NPWAutogen* gen, gpointer data)
{
	NPWInstall* this = (NPWInstall*)data;

	for(;;)
	{
		if (this->file == NULL)
		{
			this->file = npw_file_list_first(this->list);
		}
		else
		{
			if (npw_file_get_execute(this->file))
			{
				gint previous;
				// Make this file executable
				previous = umask(0666);
				chmod(npw_file_get_destination(this->file), 0777 & ~previous);
				umask(previous);
			}
			if (npw_file_get_project(this->file))
			{
				// Check if project is NULL;
				this->project_file = npw_file_get_destination(this->file);
			}
			this->file = npw_file_next(this->file);
		}
		if (this->file == NULL)
		{
			IAnjutaFileLoader* loader;
			// All files have been installed
			ianjuta_message_view_append(this->view, IANJUTA_MESSAGE_VIEW_TYPE_INFO, _("New project has been created successfully"), "", NULL);

			if (this->project_file != NULL)
			{
				loader = anjuta_shell_get_interface(ANJUTA_PLUGIN(this->plugin)->shell, IAnjutaFileLoader, NULL);
				if (loader)
				{
					ianjuta_file_loader_load(loader, this->project_file, FALSE, NULL);
				}
			}

			npw_install_destroy(this);
			return;
		}
		switch (npw_file_get_type(this->file))
		{
		case NPW_FILE:
		case NPW_SCRIPT:
			npw_install_install_file(this);
			return;
		default:
			break;
		}
	}
}

gboolean
npw_install_launch(NPWInstall* this)
{

	npw_autogen_set_output_callback(this->gen, on_install_gen_file_list, this);
	npw_autogen_execute(this->gen, on_install_use_file_list, this);

	return TRUE;
}

static gboolean
npw_install_install_file(NPWInstall* this)
{
	gchar* buffer;
	gchar* sep;
	guint len;
	const gchar* destination;
	const gchar* source;
	gchar* msg;
	gboolean use_autogen;

	destination = npw_file_get_destination(this->file);
	source = npw_file_get_source(this->file);

	// Check if autogen is needed
	switch (npw_file_get_autogen(this->file))
	{
	case NPW_TRUE:
		use_autogen = TRUE;
		break;
	case NPW_FALSE:
		use_autogen = FALSE;
		break;
	case NPW_DEFAULT:
		use_autogen = npw_is_autogen_template(source);
		break;
	}

	if (use_autogen)
	{	
		msg = g_strdup_printf(_("Copying %s to %s with AutoGen"), source, destination);
	}
	else
	{
		msg = g_strdup_printf(_("Copying %s to %s"), source, destination);
	}
	ianjuta_message_view_append(this->view, IANJUTA_MESSAGE_VIEW_TYPE_INFO, msg, "", NULL);
	g_free(msg);

	len = strlen(destination) + 1;	
	buffer = g_new(gchar, MAX(FILE_BUFFER_SIZE, len));
	strcpy(buffer, destination);			
	sep = buffer;
	for(;;)	
	{
		// Get directory one by one
		sep = strstr(sep,G_DIR_SEPARATOR_S);
		if (sep == NULL) break;
		// Create directory if necessary
		*sep = '\0';
		if ((*buffer != '~') && (*buffer != '\0'))
		{
			if (!g_file_test(buffer, G_FILE_TEST_EXISTS))
			{
				if (mkdir(buffer, 0755) == -1)
				{
					// ERROR
					return FALSE;
				}
			}
		}
		*sep++ = G_DIR_SEPARATOR_S[0];
	}

	if (use_autogen)
	{
		npw_autogen_set_input_file(this->gen, source, NULL, NULL);
		npw_autogen_set_output_file(this->gen, destination);
		npw_autogen_execute(this->gen, on_install_end_install_file, this);
	}
	else
	{
		npw_copy_file(destination, source);
		on_install_end_install_file(this->gen, this);
	}

	return TRUE;
}

/*static gboolean
npw_copy_file(const gchar* destination, const gchar* source)
{
	gchar* buffer;
	gchar* sep;
	FILE* src;
	FILE* dst;
	guint len;
	gboolean ok;

	len = strlen(destination) + 1;	
	buffer = g_new(gchar, MAX(FILE_BUFFER_SIZE, len));
	strcpy(buffer, destination);			
	sep = buffer;
	for(;;)	
	{
		// Get directory one by one
		sep = strstr(sep,G_DIR_SEPARATOR_S);
		if (sep == NULL) break;

		// Create directory if necessary
		*sep = '\0';
		if ((*buffer != '~') && (*buffer != '\0'))
		{
			if (!g_file_test(buffer, G_FILE_TEST_EXISTS))
			{
				if (mkdir(buffer, 0755) == -1)
				{
					// ERROR
					return FALSE;
				}
			}
		}
		*sep++ = G_DIR_SEPARATOR_S[0];
	}

	// Copy file
	src = fopen(source, "rb");
	if (src == NULL)
	{
		return FALSE;
	}

	dst = fopen(destination, "wb");
	if (dst == NULL)
	{
		return FALSE;
	}

	ok = TRUE;
	for(;!feof(src);)
	{
		len = fread(buffer, 1, FILE_BUFFER_SIZE, src);
		if ((len != FILE_BUFFER_SIZE) && !feof(src))
		{
			ok = FALSE;
			break;
		}

		if (len != fwrite(buffer, 1, len, dst))
		{
			ok = FALSE;
			break;
		}
	}

	fclose(dst);
	fclose(src);

	g_free(buffer);

	return ok;
}*/	
	

/*static void
cb_file_install(NPWFile* file)
{
	if (npw_file_get_type(file) == NPW_FILE)
	{
		printf("Copy2 %s to %s\n", npw_file_get_source(file), npw_file_get_destination(file));
		npw_copy_file(npw_file_get_destination(file), npw_file_get_source(file));
	}
}*/


/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    autogen.c
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "autogen.h"

#include <libanjuta/anjuta-launcher.h>

#define TMP_DEF_FILENAME "NPWDEFXXXXXX"
#define TMP_TPL_FILENAME "NPWTPLXXXXXX"

#define FILE_BUFFER_SIZE	4096

// AutoGen object

struct _NPWAutogen
{
	AnjutaLauncher* launcher;
	GList* definition;
	gchar* temptplfilename;
	const gchar* tplfilename;
	gchar* deffilename;
	gboolean cached;
	const gchar* outfilename;
	FILE* output;
	NPWAutogenOutputFunc outfunc;
	gpointer outdata;
	NPWAutogenFunc endfunc;
	gpointer enddata;
};

typedef struct _NPWDefinitionPage
{
	GString* data;
	NPWPage* page;
} NPWDefinitionPage;



void npw_autogen_destroy(NPWAutogen* this)
{
	if (this->launcher != NULL)
	{
		g_object_unref(this->launcher);
	}
	if (this->definition != NULL)
	{
		GList* node;
		NPWDefinitionPage* page;

		// Delete definition list
		for (node = this->definition; node != NULL; node = node->next)
		{
			page = (NPWDefinitionPage *)node->data;
			g_string_free(page->data, TRUE);
			g_free(page);
		}
		g_list_free(this->definition);
	}
	if (this->temptplfilename != NULL)
	{
		remove(this->temptplfilename);
		g_free(this->temptplfilename);
	}
	if (this->deffilename != NULL)
	{
		remove(this->deffilename);
		g_free(this->deffilename);
	}
	if (this->output != NULL)
	{
		fclose(this->output);
	}
	g_free(this);
}

gboolean
npw_autogen_add_default_definition(NPWAutogen* this, AnjutaPreferences* pref)
{
	NPWDefinitionPage* def;
	const gchar* value;

	// Remove previous definition if exist
	npw_autogen_remove_definition(this, NULL);

	// Add new node
	def = g_new(NPWDefinitionPage, 1);
	def->data = g_string_new("");
	def->page = NULL;
	this->definition = g_list_append(this->definition, def);
	this->cached = FALSE;

	// Add project directory
	value = anjuta_preferences_get(pref, "anjuta.project.directory");
//	value = "~/Projects";
	g_string_append_printf(def->data,"%s = \"%s\";\n", "AnjutaProjectDirectory", value);

	return TRUE;
}

static void
cb_autogen_add_definition(NPWProperty* property, gpointer data)
{
	NPWDefinitionPage* def = (NPWDefinitionPage*)data;
	const gchar* name;
	const gchar* value;

	name = npw_property_get_name(property);
	if (name != NULL)
	{
		value = npw_property_get_value(property);
		if (value == NULL) value = "";
		g_string_append_printf(def->data,"%s = \"%s\";\n", name, value);
	}
}

gboolean
npw_autogen_add_definition(NPWAutogen* this, NPWPage* page)
{
	NPWDefinitionPage* def;

	// Remove old page if exist
	npw_autogen_remove_definition(this, page);

	// Add new node
	def = g_new(NPWDefinitionPage, 1);
	def->data = g_string_new("");
	def->page = page;
	this->definition = g_list_append(this->definition, def);
	this->cached = FALSE;

	// Generate definition data for autogen
	npw_page_foreach_property(page, cb_autogen_add_definition, def);

	return TRUE;
}

gboolean
npw_autogen_remove_definition(NPWAutogen* this, NPWPage* page)
{
	GList* node;
	NPWDefinitionPage* def;

	for (node = this->definition; node != NULL; node = node->next)
	{
		def = (NPWDefinitionPage *)node->data;
		if (def->page == page)
		{
			g_string_free(def->data, TRUE);
			g_free(def);
			this->definition = g_list_delete_link(this->definition, node);
			this->cached = FALSE;

			return TRUE;
		}
	}

	return FALSE;
}

gboolean
npw_autogen_write_definition_file(NPWAutogen* this)
{
	FILE* def;
	GList* node;
	NPWDefinitionPage* page;

	// Definition file have already been written
	if (this->cached == TRUE) return TRUE;

	def = fopen(this->deffilename, "wt");
	if (def == NULL) return FALSE;
	fputs("AutoGen Definitions .;\n",def);

	for (node = this->definition; node != NULL; node = node->next)
	{
		page = (NPWDefinitionPage *)node->data;
		fwrite(page->data->str, 1, page->data->len, def);
	}

	fclose(def);
	this->cached = TRUE;

	return FALSE;
}

gboolean
npw_autogen_set_input_file(NPWAutogen* this, const gchar* filename, const gchar* start_macro, const gchar* end_macro)
{
	FILE* tpl;
	FILE* src;
	gboolean ok;
	gchar* buffer;
	guint len;

	g_return_val_if_fail((start_macro && end_macro) || (!start_macro && !end_macro), FALSE);

	if (this->temptplfilename != NULL)
	{
		remove(this->temptplfilename);
		g_free(this->temptplfilename);
		this->temptplfilename = NULL;
	}
	if ((start_macro == NULL) && (end_macro == NULL))
	{
		// input file is really an autogen file
		this->tplfilename = filename;

		return TRUE;
	}

	// Autogen definition is missing

	// Create temporary file
	this->temptplfilename = g_build_filename(g_get_tmp_dir(), TMP_TPL_FILENAME, NULL);
	mktemp(this->temptplfilename);
	this->tplfilename = this->temptplfilename;
	tpl = fopen(this->tplfilename, "wt");
	if (tpl == NULL) return FALSE;

	// Add autogen definition
	fputs(start_macro, tpl);
	fputs(" autogen5 template ", tpl);
	fputs(end_macro, tpl);
	fputc('\n', tpl);

	// Copy source file into this new file
	src = fopen(filename, "rb");
	if (src == NULL) return FALSE;

	buffer = g_new(gchar, FILE_BUFFER_SIZE);

	ok = TRUE;
	for(;!feof(src);)
	{
		len = fread(buffer, 1, FILE_BUFFER_SIZE, src);
		if ((len != FILE_BUFFER_SIZE) && !feof(src))
		{
			ok = FALSE;
			break;
		}

		if (len != fwrite(buffer, 1, len, tpl))
		{
			ok = FALSE;
			break;
		}
	}

	g_free(buffer);
	fclose(src);
	fclose(tpl);

	return ok;
}	

gboolean
npw_autogen_set_output_file(NPWAutogen* this, const gchar* filename)
{
	this->outfilename = filename;
	this->outfunc = NULL;

	return TRUE;
}

gboolean
npw_autogen_set_output_callback(NPWAutogen* this, NPWAutogenOutputFunc func, gpointer data)
{
	this->outfunc = func;
	this->outdata = data;
	this->outfilename = NULL;

	return TRUE;
}

static void
on_autogen_output(AnjutaLauncher* launcher, AnjutaLauncherOutputType type, const gchar* output, gpointer data)
{
	NPWAutogen* this = (NPWAutogen*)data;

	if (this->outfilename != NULL)
	{
		if (this->output == NULL)
		{
			this->output = fopen(this->outfilename, "wt");
		}
		if (this->output != NULL)
		{
			fputs(output, this->output);
		}
	}
	if (this->outfunc != NULL)
	{
		(this->outfunc)(output, this->outdata);
	}
}

static void
on_autogen_terminated(AnjutaLauncher* launcher, gint pid, gint status, gulong time, NPWAutogen* this)
{
	if (this->output != NULL)
	{
		fclose(this->output);
		this->output = NULL;
	}

	if (this->endfunc)
	{
		(this->endfunc)(this, this->enddata);
	}
}

gboolean
npw_autogen_execute(NPWAutogen* this, NPWAutogenFunc func, gpointer data) 
{
	gchar* args[] = {"autogen", "-T", NULL, NULL, NULL};

	g_return_val_if_fail(this, FALSE);
	g_return_val_if_fail(this->launcher, FALSE);

	// Write definition file
	npw_autogen_write_definition_file(this);

	if (func != NULL)
	{
		this->endfunc = func;
		this->enddata = data;
	}
	args[2] = (gchar *)this->tplfilename;
	args[3] = (gchar *)this->deffilename;

	printf("EXECUTE %s %s %s %s\n", args[0], args[1], args[2], args[3]);	
	if (!anjuta_launcher_execute_v(this->launcher, args, on_autogen_output, this))
	{
		return FALSE;
	}

	return TRUE;
}

NPWAutogen* npw_autogen_new(void)
{
	NPWAutogen* this;

	this = g_new0(NPWAutogen, 1);

	this->launcher = anjuta_launcher_new();
	this->deffilename = g_build_filename(g_get_tmp_dir(), TMP_DEF_FILENAME, NULL);
	mktemp(this->deffilename);
	g_signal_connect(G_OBJECT(this->launcher), "child-exited", G_CALLBACK(on_autogen_terminated), this);

	return this;
}

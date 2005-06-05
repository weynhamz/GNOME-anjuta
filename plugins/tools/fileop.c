/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    fileop.c
    Copyright (C) 2005 Sebastien Granjoux

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
 * Load and save tools.xml files. It is quite close but not compatible with
 * the previous tools.xml file from Anjuta 1.2.
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "fileop.h"

#include "tool.h"

#include <glib.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/

typedef enum {
	ATP_NO_TAG = 0,
	ATP_ANJUTA_TOOLS_TAG,
	ATP_TOOL_TAG,
	ATP_COMMAND_TAG,
	ATP_PARAM_TAG,
	ATP_WORKING_DIR_TAG,
	ATP_ENABLE_TAG,
	ATP_AUTOSAVE_TAG,
	ATP_TERMINAL_TAG,
	ATP_OUTPUT_TAG,
	ATP_ERROR_TAG,
	ATP_INPUT_TAG,
	ATP_INPUT_VALUE_TAG,
	ATP_SHORTCUT_TAG,
	ATP_ICON_TAG,
	ATP_UNKNOW_TAG
} ATPTag;

typedef enum {
	ATP_NO_ATTRIBUTE = 0,
	ATP_NAME_ATTRIBUTE,
	ATP_UNKNOW_ATTRIBUTE
} ATPAttribute;

/* Common parser functions
 *---------------------------------------------------------------------------*/

static ATPTag
parse_tag (const gchar* name)
{
	if (strcmp (name, "anjuta-tools") == 0)
	{
		return ATP_ANJUTA_TOOLS_TAG;
	}
	else if (strcmp ("tool", name) == 0)
	{
		return ATP_TOOL_TAG;
	}
	else if (strcmp ("command", name) == 0)
	{
		return ATP_COMMAND_TAG;
	}
	else if (strcmp ("parameter", name) == 0)
	{
		return ATP_PARAM_TAG;
	}
	else if (strcmp ("working_dir", name) == 0)
	{
		return ATP_WORKING_DIR_TAG;
	}
	else if (strcmp ("enabled", name) == 0)
	{
		return ATP_ENABLE_TAG;
	}
	else if (strcmp ("autosave", name) == 0)
	{
		return ATP_AUTOSAVE_TAG;
	}
	else if (strcmp ("run_in_terminal", name) == 0)
	{
		return ATP_TERMINAL_TAG;
	}
	else if (strcmp ("output", name) == 0)
	{
		return ATP_OUTPUT_TAG;
	}
	else if (strcmp ("error", name) == 0)
	{
		return ATP_ERROR_TAG;
	}
	else if (strcmp ("input_type", name) == 0)
	{
		return ATP_INPUT_TAG;
	}
	else if (strcmp ("input", name) == 0)
	{
		return ATP_INPUT_VALUE_TAG;
	}
	else if (strcmp ("shortcut", name) == 0)
	{
		return ATP_SHORTCUT_TAG;
	}	
	else if (strcmp ("icon", name) == 0)
	{
		return ATP_ICON_TAG;
	}
	else
	{
		return ATP_UNKNOW_TAG;
	}
}

static ATPAttribute
parse_attribute (const gchar* name)
{
	if ((strcmp ("name", name) == 0) || (strcmp ("_name", name) == 0))
	{
		return ATP_NAME_ATTRIBUTE;
	}
	else
	{	
		return ATP_UNKNOW_ATTRIBUTE;
	}
}

static gboolean
parse_boolean_string (const gchar* value)
{
	return g_ascii_strcasecmp ("no", value) && g_ascii_strcasecmp ("0", value) && g_ascii_strcasecmp ("false", value);
}

static gint
parse_integer_string (const gchar* value)
{
	return atoi(value);
}

static void
parser_warning (GMarkupParseContext* ctx, const gchar* format,...)
{
	va_list args;
	gchar* msg;
	gint line;

	g_markup_parse_context_get_position (ctx, &line, NULL);
	msg = g_strdup_printf ("line %d: %s", line, format);
	va_start (args, format);
	g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, msg, args);
	va_end (args);
	g_free (msg);
}

/* Load anjuta-tools
 *---------------------------------------------------------------------------*/

#define ATP_TOOL_PARSER_MAX_LEVEL	3	/* Maximum number of nested elements */

typedef struct _ATPToolParser
{
	GMarkupParseContext* ctx;
	/* Known element stack */
	ATPTag tag[ATP_TOOL_PARSER_MAX_LEVEL + 1];
	ATPTag* last;
	/* Unknown element stack */
	guint unknown;
	/* List where should be added the header */
	ATPToolList* list;
	/* Type of storage */
	ATPToolStore storage;
	/* Current header */
	ATPUserTool* tool;
} ATPToolParser;

static void
parse_tool_start (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	ATPToolParser* parser = (ATPToolParser*)data;
	ATPTag tag;
	gboolean known = FALSE;
	const gchar* tool_name;

	/* Recognize element */
	if (parser->unknown == 0)
	{
		/* Not inside an unknown element */
		tag = parse_tag (name);
		switch (*parser->last)
		{
		case ATP_NO_TAG:
			/* Top level element */
			switch (tag)
			{
			case ATP_ANJUTA_TOOLS_TAG:
				known = TRUE;
				break;
			case ATP_UNKNOW_TAG:
				parser_warning (parser->ctx, "Unknown element \"%s\"", name);
				break;
			default:
				break;
			}
			break;
		case ATP_ANJUTA_TOOLS_TAG:
			/* Necessary to avoid neested anjuta-tools element */
			switch (tag)
			{
			case ATP_TOOL_TAG:
				tool_name = NULL;
				while (*attributes != NULL)
				{
					if (parse_attribute (*attributes) == ATP_NAME_ATTRIBUTE)
					{
						tool_name = *values;
					}
					attributes++;
					values++;
				}
				if (tool_name == NULL)
				{
					parser_warning (parser->ctx, _("Missing tool name"));
					break;
				}
				else
				{
					parser->tool = atp_tool_list_append_new (parser->list, tool_name, parser->storage);
					known = TRUE;
				}
				break;
			default:
				parser_warning (parser->ctx, _("Unexpected element \"%s\""), name);
				break;
			}
			break;
		case ATP_TOOL_TAG:
			switch (tag)
			{
			case ATP_COMMAND_TAG:
			case ATP_PARAM_TAG:
			case ATP_WORKING_DIR_TAG:
			case ATP_ENABLE_TAG:
			case ATP_AUTOSAVE_TAG:
			case ATP_TERMINAL_TAG:
			case ATP_OUTPUT_TAG:
			case ATP_ERROR_TAG:
			case ATP_INPUT_TAG:
			case ATP_INPUT_VALUE_TAG:
			case ATP_SHORTCUT_TAG:
			case ATP_ICON_TAG:
				known = TRUE;
				break;
			case ATP_UNKNOW_TAG:
				/* No warning on unknown tags */
				break;
			default:
				parser_warning (parser->ctx, "Unexpected element \"%s\"", name);
				break;
			}
			break;
		default:
			parser_warning (parser->ctx, "Unexpected element \"%s\"", name);
			break;
		}
	}

	/* Push element */
	if (known)
	{
		/* Know element stack overflow */
		g_return_if_fail ((parser->last - parser->tag) <= ATP_TOOL_PARSER_MAX_LEVEL);
		parser->last++;
		*parser->last = tag;
	}
	else
	{
		parser->unknown++;
	}
}

static void
parse_tool_end (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	ATPToolParser* parser = (ATPToolParser*)data;
	
	if (parser->unknown > 0)
	{
		/* Pop unknown element */
		parser->unknown--;
	}
	else if (*parser->last == ATP_TOOL_TAG)
	{
		/* Pop known element */
		parser->last--;
		parser->tool = NULL;
	}
	else if (*parser->last != ATP_NO_TAG)
	{
		/* Pop known element */
		parser->last--;
	}
	else
	{
		/* Know element stack underflow */
		g_return_if_reached ();
	}
}

static void
parse_tool_text (GMarkupParseContext* context,
       	const gchar* text,
	gsize len,
	gpointer data,
	GError** error)
{
	ATPToolParser* parser = (ATPToolParser*)data;
	guint accel_key;
	GdkModifierType accel_mods;

	if (parser->unknown == 0)
	{
		switch (*parser->last)
		{
		case ATP_COMMAND_TAG:
			g_return_if_fail (parser->tool);

			atp_user_tool_set_command (parser->tool, text);
			break;
		case ATP_PARAM_TAG:
			g_return_if_fail (parser->tool);

			atp_user_tool_set_param (parser->tool, text);
			break;
		case ATP_WORKING_DIR_TAG:
			g_return_if_fail (parser->tool);

			atp_user_tool_set_working_dir (parser->tool, text);
			break;
		case ATP_ENABLE_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_flag (parser->tool, ATP_TOOL_ENABLE | (parse_boolean_string (text) ? ATP_SET : ATP_CLEAR));
			break;
		case ATP_AUTOSAVE_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_flag (parser->tool, ATP_TOOL_AUTOSAVE | (parse_boolean_string (text) ? ATP_SET : ATP_CLEAR));
			break;
		case ATP_TERMINAL_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_flag (parser->tool, ATP_TOOL_TERMINAL | (parse_boolean_string (text) ? ATP_SET : ATP_CLEAR));
			break;
		case ATP_OUTPUT_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_output (parser->tool, parse_integer_string (text));
			break;
		case ATP_ERROR_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_error (parser->tool, parse_integer_string (text));
			break;
		case ATP_INPUT_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_input (parser->tool, parse_integer_string (text), NULL);
			break;
		case ATP_INPUT_VALUE_TAG:
			g_return_if_fail (parser->tool);
			atp_user_tool_set_input (parser->tool, atp_user_tool_get_input (parser->tool), text);
			break;
		case ATP_SHORTCUT_TAG:
			g_return_if_fail (parser->tool);
			gtk_accelerator_parse (text, &accel_key, &accel_mods);
			atp_user_tool_set_accelerator (parser->tool, accel_key, accel_mods);
			break;
		case ATP_ICON_TAG:
			g_return_if_fail (parser->tool);

			atp_user_tool_set_icon (parser->tool, text);
			break;
		case ATP_ANJUTA_TOOLS_TAG:
		case ATP_TOOL_TAG:
		case ATP_UNKNOW_TAG:
			/* Nothing to do */
			break;
		default:
			/* Unknown tag */
			g_return_if_reached ();
			break;
		}
	}
}

static GMarkupParser tool_markup_parser = {
	parse_tool_start,
	parse_tool_end,
	parse_tool_text,
	NULL,
	NULL
};

static ATPToolParser*
atp_tool_parser_new (ATPToolList* list, ATPToolStore storage)
{
	ATPToolParser* this;

	this = g_new0 (ATPToolParser, 1);

	this->unknown = 0;
	this->tag[0] = ATP_NO_TAG;
	this->last = this->tag;

	this->list = list;
	this->storage = storage;
	this->tool = NULL;

	this->ctx = g_markup_parse_context_new (&tool_markup_parser, 0, this, NULL);
	g_assert (this->ctx != NULL);

	return this;
}

static void
atp_tool_parser_free (ATPToolParser* this)
{
	g_return_if_fail (this != NULL);

	g_markup_parse_context_free (this->ctx);
	g_free (this);
}

static gboolean
atp_tool_parser_parse (ATPToolParser* this, const gchar* text, gssize len, GError** error)
{
	return g_markup_parse_context_parse (this->ctx, text, len, error);
}

static gboolean
atp_tool_parser_end_parse (ATPToolParser* this, GError** error)
{
	return g_markup_parse_context_end_parse (this->ctx, error);
}

/* Loads toolset from a xml configuration file.
 * Tools properties are saved xml format */
static gboolean
atp_tool_list_load_from_file (ATPToolList* this, const gchar* filename, ATPToolStore storage)
{
	gchar* content;
	gsize len;
	ATPToolParser* parser;
	GError* err = NULL;

	g_return_val_if_fail (this != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (!g_file_get_contents (filename, &content, &len, &err))
	{
		/* This is not an error condition since the user might not have
		** defined any tools, or there might not be any global tools */
		g_error_free (err);

		return TRUE;
	}

	parser = atp_tool_parser_new (this, storage);

	parser->tool = NULL;
	atp_tool_parser_parse (parser, content, len, &err);
	if (err == NULL) atp_tool_parser_end_parse (parser, &err);
       
	atp_tool_parser_free (parser);
	g_free (content);

	if (err != NULL)
	{
		/* Parsing error */
		g_warning (err->message);
		g_error_free (err);

		return FALSE;
	}

	return TRUE;	
}

gboolean
atp_anjuta_tools_load(ATPPlugin* plugin)
{
	gboolean ok;
	gchar* file_name;

	/* First, load global tools */
	file_name = g_build_filename (ANJUTA_TOOLS_DIRECTORY, TOOLS_FILE, NULL);
	ok = atp_tool_list_load_from_file (atp_plugin_get_tool_list(plugin), file_name, ATP_TSTORE_GLOBAL);
	g_free (file_name);

	/* Now, user tools */
	file_name = g_build_filename (g_get_home_dir(), LOCAL_ANJUTA_TOOLS_DIRECTORY, TOOLS_FILE, NULL);
	ok = atp_tool_list_load_from_file (atp_plugin_get_tool_list(plugin), file_name, ATP_TSTORE_LOCAL);
	g_free (file_name);
	if (!ok)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),_("Error when loading external tools"));		
		return FALSE;
	}

	return TRUE;
}

/* Save tools file
 *---------------------------------------------------------------------------*/

typedef const gchar* const_gchar_ptr;

static gboolean
write_xml_string (const gchar *value, const gchar *tag, const_gchar_ptr *head, FILE *f)
{
	gchar* line;

	if (value == NULL) return FALSE;

	/* Check if we need the header */
	if (*head != NULL)
	{
		fprintf(f, "\t<tool name=\"%s\">\n", *head);
		*head = NULL;
	}

	/* Write xml line */
	line = g_markup_printf_escaped ("\t\t<%s>%s</%s>\n", tag, value, tag);
	fputs (line, f);
	g_free (line);

	return TRUE;
}

static gboolean
write_xml_boolean (gboolean value, const gchar *tag, const_gchar_ptr *head, FILE *f)
{
	return write_xml_string (value ? "1" : "0", tag, head, f);
}

static gboolean
write_xml_integer (gint value, const gchar *tag, const_gchar_ptr *head, FILE *f)
{
	gchar buffer[33];

	sprintf (buffer,"%d", value);

	return write_xml_string (buffer, tag, head, f);
}

static gboolean
write_xml_accelerator (guint key, GdkModifierType mods, const gchar *tag, const_gchar_ptr *head, FILE *f)
{
	gchar* value;
	gboolean ok;

	value = gtk_accelerator_name (key, mods);
	ok = write_xml_string (value, tag, head, f);
	g_free (value);

	return ok;
}

#define SAVE_STRING(func, key) \
{\
	const gchar* save; \
	save = atp_user_tool_get_##func (tool); \
	if (!over || (save != atp_user_tool_get_##func (over))) \
		write_xml_string (save, key, &head, f); \
} 	

#define SAVE_FLAG(flag, key) \
{\
	gboolean save; \
	save = atp_user_tool_get_flag (tool, flag); \
	if (!over || (save != atp_user_tool_get_flag (over, flag))) \
		write_xml_boolean (save, key, &head, f); \
} 

#define SAVE_INTEGER(func, key) \
{\
	gint save; \
	save = atp_user_tool_get_##func (tool); \
	if (!over || (save != atp_user_tool_get_##func (over))) \
		write_xml_integer (save, key, &head, f); \
} 	

#define SAVE_ACCELERATOR(func, key) \
{\
	guint save_key; \
	GdkModifierType save_mask; \
	guint over_key; \
	GdkModifierType over_mask; \
	atp_user_tool_get_##func (tool, &save_key, &save_mask); \
	if (over) atp_user_tool_get_##func (over, &over_key, &over_mask); \
	if (!over || (save_key != over_key) || (save_mask != over_mask)) \
		write_xml_accelerator (save_key, save_mask, key, &head, f); \
} 	

/* Writes tool information to the given file in xml format */
static gboolean
atp_user_tool_save (ATPUserTool *tool, FILE *f)
{
	const gchar* head;
	ATPUserTool *over;

	/* head contains the tool name until the tool header is written */
	head = atp_user_tool_get_name (tool);

	over = atp_user_tool_override (tool);

	SAVE_STRING (command , "command");	
	SAVE_STRING (param , "parameter");
	SAVE_STRING (working_dir, "working_dir");
	SAVE_FLAG (ATP_TOOL_ENABLE, "enabled");
	SAVE_FLAG (ATP_TOOL_AUTOSAVE, "autosave");
	SAVE_FLAG (ATP_TOOL_TERMINAL, "run_in_terminal");
	SAVE_INTEGER (output, "output");
	SAVE_INTEGER (error, "error");
	SAVE_INTEGER (input, "input_type");
	SAVE_STRING (input_string, "input");
	SAVE_ACCELERATOR (accelerator, "shortcut");
	SAVE_STRING (icon, "icon");

	if (head == NULL)
	{
		fprintf (f, "\t</tool>\n");

		return TRUE;
	}
	else
	{
		/* Force at least one line for the tool */
		fprintf (f, "\t<tool name=\"%s\"/>\n", atp_user_tool_get_name (tool));

		return FALSE;
	}
}

/* While saving, save only the user tools since it is unlikely that the
** user will have permission to write to the system-wide tools file
*/
gboolean atp_anjuta_tools_save(ATPPlugin* plugin)
{
	FILE *f;
	ATPUserTool *tool;
	gchar* file_name;
	gboolean ok;

	/* Save local tools */
	file_name = g_build_filename (g_get_home_dir(), LOCAL_ANJUTA_TOOLS_DIRECTORY, TOOLS_FILE, NULL);
	if (NULL == (f = fopen(file_name, "w")))
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),_("Unable to open %s for writing"), file_name);
		return FALSE;
	}
	fprintf (f, "<?xml version=\"1.0\"?>\n");
	fprintf (f, "<anjuta-tools>\n");
	ok = TRUE;
	for (tool = atp_tool_list_first_in_storage (atp_plugin_get_tool_list(plugin), ATP_TSTORE_LOCAL); tool != NULL; tool = atp_user_tool_next_in_same_storage(tool))
	{
		/* Tool is in local storage, save it */
		atp_user_tool_save(tool, f);
	}
	fprintf (f, "</anjuta-tools>\n");
	fclose(f);

	return TRUE;
}

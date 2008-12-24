/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    parser.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* 
 * All functions for parsing wizard template (.wiz) files
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "parser.h"

#include <glib.h>

#include <string.h>
#include <stdarg.h>

#include <libanjuta/anjuta-debug.h>

/*---------------------------------------------------------------------------*/

#define PROJECT_WIZARD_EXTENSION	".wiz"

typedef enum {
	NPW_NO_TAG = 0,
	NPW_PROJECT_WIZARD_TAG,
	NPW_NAME_TAG,
	NPW_DESCRIPTION_TAG,
	NPW_CATEGORY_TAG,
	NPW_REQUIRED_PROGRAM_TAG,
	NPW_REQUIRED_PACKAGE_TAG,
	NPW_ICON_TAG,
	NPW_PAGE_TAG,
	NPW_PROPERTY_TAG,
	NPW_ITEM_TAG,
	NPW_DIRECTORY_TAG,
	NPW_FILE_TAG,
	NPW_CONTENT_TAG,
	NPW_ACTION_TAG,
	NPW_RUN_TAG,
	NPW_OPEN_TAG,
	NPW_UNKNOW_TAG
} NPWTag;

typedef enum {
	NPW_NO_ATTRIBUTE = 0,
	NPW_NAME_ATTRIBUTE,
	NPW_LABEL_ATTRIBUTE,
	NPW_DESCRIPTION_ATTRIBUTE,
	NPW_VALUE_ATTRIBUTE,
	NPW_SUMMARY_ATTRIBUTE,
	NPW_TYPE_ATTRIBUTE,
	NPW_RESTRICTION_ATTRIBUTE,
	NPW_MANDATORY_ATTRIBUTE,
	NPW_EXIST_ATTRIBUTE,
	NPW_EDITABLE_ATTRIBUTE,
	NPW_SOURCE_ATTRIBUTE,
	NPW_DESTINATION_ATTRIBUTE,
	NPW_EXECUTABLE_ATTRIBUTE,
	NPW_PROJECT_ATTRIBUTE,
	NPW_AUTOGEN_ATTRIBUTE,
	NPW_COMMAND_ATTRIBUTE,
	NPW_FILE_ATTRIBUTE,
	NPW_UNKNOW_ATTRIBUTE
} NPWAttribute;

typedef enum {
	NPW_HEADER_PARSER,
	NPW_PAGE_PARSER,
	NPW_FILE_PARSER,
	NPW_ACTION_PARSER
} NPWParser;

typedef enum {
	NPW_STOP_PARSING,
} NPWParserError;


/* Read all project templates in a directory
 *---------------------------------------------------------------------------*/

gboolean
npw_header_list_readdir (GList** list, const gchar* path)
{
	GDir* dir;
	const gchar* name;
	gboolean ok = FALSE;

	g_return_val_if_fail (list != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	/* Read all project template files */
	dir = g_dir_open (path, 0, NULL);
	if (!dir) return FALSE;

	while ((name = g_dir_read_name (dir)) != NULL)
	{
		char* filename = g_build_filename (path, name, NULL);

		if (g_file_test (filename, G_FILE_TEST_IS_DIR))
		{
			/* Search recursively in sub directory */
			if (npw_header_list_readdir (list, filename))
			{
				ok = TRUE;
			}
		}
		else if (g_str_has_suffix (name, PROJECT_WIZARD_EXTENSION))
		{
			if (npw_header_list_read (list, filename))
			{
				/* Read at least one project file */
				ok = TRUE;
			}
		}
		g_free (filename);
	}

	g_dir_close (dir);

	return ok;
}

/* Common parser functions
 *---------------------------------------------------------------------------*/

static NPWTag
parse_tag (const char* name)
{
	if (strcmp (name, "project-wizard") == 0)
	{
		return NPW_PROJECT_WIZARD_TAG;
	}
	else if ((strcmp ("_name", name) == 0) || (strcmp ("name", name) == 0))
	{
		return NPW_NAME_TAG;
	}
	else if ((strcmp ("_description", name) == 0) || (strcmp ("description", name) == 0))
	{
		return NPW_DESCRIPTION_TAG;
	}
	else if (strcmp ("icon", name) == 0)
	{
		return NPW_ICON_TAG;
	}
	else if (strcmp ("category", name) == 0)
	{
		return NPW_CATEGORY_TAG;
	}
	else if (strcmp ("required-program", name) == 0)
	{
		return NPW_REQUIRED_PROGRAM_TAG;
	}
	else if (strcmp ("required-package", name) == 0)
	{
		return NPW_REQUIRED_PACKAGE_TAG;
	}
	else if (strcmp ("page", name) == 0)
	{
		return NPW_PAGE_TAG;
	}
	else if (strcmp ("property", name) == 0)
	{
		return NPW_PROPERTY_TAG;
	}
	else if (strcmp ("item", name) == 0)
	{
		return NPW_ITEM_TAG;
	}	
	else if (strcmp ("directory", name) == 0)
	{
		return NPW_DIRECTORY_TAG;
	}
	else if (strcmp ("content", name) == 0)
	{
		return NPW_CONTENT_TAG;
	}
	else if (strcmp ("file", name) == 0)
	{
		return NPW_FILE_TAG;
	}
	else if (strcmp ("action", name) == 0)
	{
		return NPW_ACTION_TAG;
	}
	else if (strcmp ("run", name) == 0)
	{
		return NPW_RUN_TAG;
	}
	else if (strcmp ("open", name) == 0)
	{
		return NPW_OPEN_TAG;
	}
	else
	{
		return NPW_UNKNOW_TAG;
	}
}

static NPWAttribute
parse_attribute (const char* name)
{
	if (strcmp ("name", name) == 0)
	{
		return NPW_NAME_ATTRIBUTE;
	}
	else if (strcmp ("_label", name) == 0)
	{
		return NPW_LABEL_ATTRIBUTE;
	}
	else if (strcmp ("_description", name) == 0)
	{
		return NPW_DESCRIPTION_ATTRIBUTE;
	}
	else if (strcmp ("default", name) == 0 || strcmp ("value", name) == 0)
	{
		return NPW_VALUE_ATTRIBUTE;
	}
	else if (strcmp ("type", name) == 0)
	{
		return NPW_TYPE_ATTRIBUTE;
	}
	else if (strcmp ("restriction", name) == 0)
	{
		return NPW_RESTRICTION_ATTRIBUTE;
	}
	else if (strcmp ("summary", name) == 0)
	{
		return NPW_SUMMARY_ATTRIBUTE;
	}
	else if (strcmp ("mandatory", name) == 0)
	{
		return NPW_MANDATORY_ATTRIBUTE;
	}
	else if (strcmp ("editable", name) == 0)
	{
		return NPW_EDITABLE_ATTRIBUTE;
	}
	else if (strcmp ("exist", name) == 0)
	{
		return NPW_EXIST_ATTRIBUTE;
	}
	else if (strcmp ("source", name) == 0)
	{
		return NPW_SOURCE_ATTRIBUTE;
	}
	else if (strcmp ("destination", name) == 0)
	{
		return NPW_DESTINATION_ATTRIBUTE;
	}
	else if (strcmp ("executable", name) == 0)
	{
		return NPW_EXECUTABLE_ATTRIBUTE;
	}
	else if (strcmp ("project", name) == 0)
	{
		return NPW_PROJECT_ATTRIBUTE;
	}
	else if (strcmp ("autogen", name) == 0)
	{
		return NPW_AUTOGEN_ATTRIBUTE;
	}
	else if (strcmp ("command", name) == 0)
	{
		return NPW_COMMAND_ATTRIBUTE;
	}
	else if (strcmp ("file", name) == 0)
	{
		return NPW_FILE_ATTRIBUTE;
	}
	else
	{	
		return NPW_UNKNOW_ATTRIBUTE;
	}
}

static gboolean
parse_boolean_string (const gchar* value)
{
	return g_ascii_strcasecmp ("no", value) && g_ascii_strcasecmp ("0", value) && g_ascii_strcasecmp ("false", value);
}

static GQuark
parser_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark = g_quark_from_static_string ("parser_error_quark");
	return error_quark;
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

static void
parser_critical (GMarkupParseContext* ctx, const gchar* format,...)
{
	va_list args;
	gchar* msg;
	gint line;

	g_markup_parse_context_get_position (ctx, &line, NULL);
	msg = g_strdup_printf ("line %d: %s", line, format);
	va_start (args, format);
	g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, msg, args);
	va_end (args);
	g_free (msg);
}
/* Parse project wizard block
 *---------------------------------------------------------------------------*/

#define NPW_HEADER_PARSER_MAX_LEVEL	2	/* Maximum number of nested elements */

typedef struct _NPWHeaderParser
{
	/* Type of parser (not used) */
	NPWParser type;
	GMarkupParseContext* ctx;
	/* Known element stack */
	NPWTag tag[NPW_HEADER_PARSER_MAX_LEVEL + 1];
	NPWTag* last;
	/* Unknown element stack */
	guint unknown;
	/* List where should be added the header */
	GList** list;
	/* Current header */
	NPWHeader* header;
	/* Name of file read */
	gchar* filename;
} NPWHeaderParser;

static void
parse_header_start (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	NPWHeaderParser* parser = (NPWHeaderParser*)data;
	NPWTag tag;
	gboolean known = FALSE;

	/* Recognize element */
	if (parser->unknown == 0)
	{
		/* Not inside an unknown element */
		tag = parse_tag (name);
		switch (*parser->last)
		{
		case NPW_NO_TAG:
			/* Top level element */
			switch (tag)
			{
			case NPW_PROJECT_WIZARD_TAG:
				parser->header = npw_header_new ();
				npw_header_set_filename (parser->header, parser->filename);
				known = TRUE;
				break;
			case NPW_UNKNOW_TAG:
				parser_warning (parser->ctx, "Unknown element \"%s\"", name);
				break;
			default:
				break;
			}
			break;
		case NPW_PROJECT_WIZARD_TAG:
			/* Necessary to avoid neested PROJECT_WIZARD element */
			switch (tag)
			{
			case NPW_NAME_TAG:
			case NPW_DESCRIPTION_TAG:
			case NPW_ICON_TAG:
			case NPW_CATEGORY_TAG:
			case NPW_REQUIRED_PROGRAM_TAG:
			case NPW_REQUIRED_PACKAGE_TAG:
				known = TRUE;
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
		g_return_if_fail ((parser->last - parser->tag) <= NPW_HEADER_PARSER_MAX_LEVEL);
		parser->last++;
		*parser->last = tag;
	}
	else
	{
		parser->unknown++;
	}
}

static void
parse_header_end (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	NPWHeaderParser* parser = (NPWHeaderParser*)data;
	
	if (parser->unknown > 0)
	{
		/* Pop unknown element */
		parser->unknown--;
	}
	else if (*parser->last != NPW_NO_TAG)
	{
		/* Pop known element */
		parser->last--;
		if (parser->last[1] == NPW_PROJECT_WIZARD_TAG)
		{
			/* Check if the element is valid */
			if (parser->header && !npw_header_get_name (parser->header))
			{
				parser_critical (parser->ctx, "Missing name attribute");
				npw_header_free (parser->header);
			}

			/* Stop parsing after first project wizard block
			 * Remaining file need to be passed through autogen
			 *  to be a valid xml file */

			/* error should be available to stop parsing */
			g_return_if_fail (error != NULL);
		
			/* Send an error */	
			*error = g_error_new_literal (parser_error_quark (), NPW_STOP_PARSING, "");
		}
	}
	else
	{
		/* Know element stack underflow */
		g_return_if_reached ();
	}
}

static void
parse_header_text (GMarkupParseContext* context,
       	const gchar* text,
	gsize len,
	gpointer data,
	GError** error)
{
	NPWHeaderParser* parser = (NPWHeaderParser*)data;

	if (parser->unknown == 0)
	{
		switch (*parser->last)
		{
		case NPW_NAME_TAG:
			if (npw_header_get_name (parser->header) == NULL)
			{
				npw_header_set_name (parser->header, text);
			}
			else
			{
				parser_critical (parser->ctx, "Duplicated name tag");
			}
			break;
		case NPW_DESCRIPTION_TAG:
			if (npw_header_get_description (parser->header) == NULL)
			{
				npw_header_set_description (parser->header, text);
			}
			else
			{
				parser_critical (parser->ctx, "Duplicated description tag");
			}
			break;
		case NPW_ICON_TAG:
			if (npw_header_get_iconfile (parser->header) == NULL)
			{
				char* filename;
				char* path;

				path = g_path_get_dirname (parser->filename);
				filename = g_build_filename (path, text, NULL);
				npw_header_set_iconfile (parser->header, filename);
				g_free (path);
				g_free (filename);
			}
			else
			{
				parser_critical (parser->ctx, "Duplicated icon tag");
			}
			break;
		case NPW_CATEGORY_TAG:
			if (npw_header_get_category (parser->header) == NULL)
			{
				npw_header_set_category (parser->header, text);
				*parser->list = npw_header_list_insert_header (*parser->list, parser->header);				
			}
			else
			{
				parser_critical (parser->ctx, "Duplicated category tag");
			}
			break;
		case NPW_REQUIRED_PROGRAM_TAG:
			npw_header_add_required_program (parser->header, text);
			break;
		case NPW_REQUIRED_PACKAGE_TAG:
			npw_header_add_required_package (parser->header, text);
			break;
		case NPW_PROJECT_WIZARD_TAG:
			/* Nothing to do */
			break;
		default:
			/* Unknown tag */
			g_return_if_reached ();
			break;
		}
	}
}

static GMarkupParser header_markup_parser = {
	parse_header_start,
	parse_header_end,
	parse_header_text,
	NULL,
	NULL
};

static NPWHeaderParser*
npw_header_parser_new (GList** list, const gchar* filename)
{
	NPWHeaderParser* parser;

	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (filename != NULL, NULL);

	parser = g_new0 (NPWHeaderParser, 1);

	parser->type = NPW_HEADER_PARSER;
	parser->unknown = 0;
	parser->tag[0] = NPW_NO_TAG;
	parser->last = parser->tag;
	parser->list = list;
	parser->header = NULL;
	parser->filename = g_strdup (filename);

	parser->ctx = g_markup_parse_context_new (&header_markup_parser, 0, parser, NULL);
	g_assert (parser->ctx != NULL);

	return parser;
}

static void
npw_header_parser_free (NPWHeaderParser* parser)
{
	g_return_if_fail (parser != NULL);

	g_free (parser->filename);
	g_markup_parse_context_free (parser->ctx);
	g_free (parser);
}

static gboolean
npw_header_parser_parse (NPWHeaderParser* parser, const gchar* text, gssize len, GError** error)
{
	return g_markup_parse_context_parse (parser->ctx, text, len, error);
}

/* Not used

static gboolean
npw_header_parser_end_parse (NPWHeaderParser* parser, GError** error)
{
	return g_markup_parse_context_end_parse (parser->ctx, error);
}*/

gboolean
npw_header_list_read (GList** list, const gchar* filename)
{
	gchar* content;
	gsize len;
	NPWHeaderParser* parser;
	GError* err = NULL;

	g_return_val_if_fail (list != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (!g_file_get_contents (filename, &content, &len, &err))
	{
 		g_warning (err->message);
		g_error_free (err);

		return FALSE;
	}

	parser = npw_header_parser_new (list, filename);

	npw_header_parser_parse (parser, content, len, &err);
	/* Parse only a part of the file, so need to call parser_end_parse */
       
	npw_header_parser_free (parser);
	g_free (content);

	if (err == NULL)
	{
		/* Parsing must end with an error
		 *  generated at the end of the project wizard block */
		g_warning ("Missing project wizard block in %s", filename);

		return FALSE;
	}
	if (g_error_matches (err, parser_error_quark (), NPW_STOP_PARSING) == FALSE)
	{
		/* Parsing error */
		g_warning (err->message);
		g_error_free (err);

		return FALSE;
	}
	g_error_free (err);

	return TRUE;	
}


/* Parse page block
 *---------------------------------------------------------------------------*/

#define NPW_PAGE_PARSER_MAX_LEVEL	3	/* Maximum number of nested elements */

struct _NPWPageParser
{
	/* Type of parser (not used) */
	NPWParser type;
	GMarkupParseContext* ctx;
	/* Known element stack */
	NPWTag tag[NPW_PAGE_PARSER_MAX_LEVEL + 1];
	NPWTag* last;
	/* Unknown element stack */
	guint unknown;
	/* page number to read */
	gint count;
	/* Current page object */
	NPWPage* page;
	/* Current property object */
	NPWProperty* property;
};

static gboolean
parse_page (NPWPageParser* parser, 
	const gchar** attributes,
	const gchar** values)
{
	if (parser->count != 0)
	{
		/* Skip this page */
		if (parser->count > 0) parser->count--;

		return FALSE;
	}
	else
	{
		/* Read this page */
		while (*attributes != NULL)
		{
			switch (parse_attribute (*attributes))
			{
			case NPW_NAME_ATTRIBUTE:
				npw_page_set_name (parser->page, *values);
				break;
			case NPW_LABEL_ATTRIBUTE:
				npw_page_set_label (parser->page, *values);
				break;
			case NPW_DESCRIPTION_ATTRIBUTE:
				npw_page_set_description (parser->page, *values);
				break;
			default:
				parser_warning (parser->ctx, "Unknown page attribute \"%s\"", *attributes);
				break;
			}
			attributes++;
			values++;
		}
		parser->count--;

		return TRUE;
	}
}

static gboolean
parse_property (NPWPageParser* parser,
	const gchar** attributes,
	const gchar** values)
{
	parser->property = npw_property_new ();
	npw_page_add_property (parser->page, parser->property);

	while (*attributes != NULL)
	{
		switch (parse_attribute (*attributes))
		{
		case NPW_TYPE_ATTRIBUTE:
			npw_property_set_string_type (parser->property, *values);
			break;
		case NPW_RESTRICTION_ATTRIBUTE:
			npw_property_set_string_restriction (parser->property, *values);
			break;
		case NPW_NAME_ATTRIBUTE:
			npw_property_set_name (parser->property, *values, parser->page);
			break;
		case NPW_LABEL_ATTRIBUTE:
			npw_property_set_label (parser->property, *values);
			break;
		case NPW_DESCRIPTION_ATTRIBUTE:
			npw_property_set_description (parser->property, *values);
			break;
		case NPW_VALUE_ATTRIBUTE:
			npw_property_set_default (parser->property, *values);
			break;
		case NPW_SUMMARY_ATTRIBUTE:
			npw_property_set_summary_option (parser->property, parse_boolean_string (*values));
			break;		
		case NPW_MANDATORY_ATTRIBUTE:
			npw_property_set_mandatory_option (parser->property, parse_boolean_string (*values));
			break;
		case NPW_EDITABLE_ATTRIBUTE:
			npw_property_set_editable_option (parser->property, parse_boolean_string (*values));
			break;
		case NPW_EXIST_ATTRIBUTE:
			npw_property_set_exist_option (parser->property, parse_boolean_string (*values));
			break;
		default:
			parser_warning (parser->ctx, "Unknown property attribute \"%s\"", *attributes);
			break;
		}
		attributes++;
		values++;
	}	

	return TRUE;
}

static gboolean
parse_item (NPWPageParser* parser,
	const gchar** attributes,
	const gchar** values)
{
	const gchar* label = NULL;
	const gchar* name = NULL;

	while (*attributes != NULL)
	{
		switch (parse_attribute (*attributes))
		{
		case NPW_NAME_ATTRIBUTE:
			name = *values;
			break;
		case NPW_LABEL_ATTRIBUTE:
			label = *values;
			break;
		default:
			parser_warning (parser->ctx, "Unknown item attribute \"%s\"", *attributes);
			break;
		}
		attributes++;
		values++;
	}

	if (name == NULL)
	{
		parser_warning (parser->ctx, "Missing name attribute");
	}
	else
	{
		npw_property_add_list_item (parser->property, name, label == NULL ? name : label);
	}

	return TRUE;
}

static void
parse_page_start (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	NPWTag tag;
	gboolean known = FALSE;

	/* Recognize element */
	if (parser->unknown == 0)
	{
		/* Not inside an unknown element */
		tag = parse_tag (name);
		switch (*parser->last)
		{
		case NPW_NO_TAG:
			/* Top level element */
			switch (tag)
			{
			case NPW_PAGE_TAG:
				known = parse_page (parser, attributes, values);
				break;
			case NPW_UNKNOW_TAG:
				parser_warning (parser->ctx, "Unknown element \"%s\"", name);
				break;
			default:
				break;
			}
			break;
		case NPW_PAGE_TAG:
			/* Necessary to avoid neested page element */
			switch (tag)
			{
			case NPW_PROPERTY_TAG:
				known = parse_property (parser, attributes, values);
				break;
			default:
				parser_warning (parser->ctx, "Unexpected element \"%s\"", name);
				break;
			}
			break;
		case NPW_PROPERTY_TAG:
			/* Necessary to avoid neested page & property element */
			switch (tag)
			{
			case NPW_ITEM_TAG:
				known = parse_item (parser, attributes, values);
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
		g_return_if_fail ((parser->last - parser->tag) <= NPW_PAGE_PARSER_MAX_LEVEL);
		parser->last++;
		*parser->last = tag;
	}
	else
	{
		parser->unknown++;
	}
}

static void
parse_page_end (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	
	if (parser->unknown > 0)
	{
		/* Pop unknown element */
		parser->unknown--;
	}
	else if (*parser->last != NPW_NO_TAG)
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

static GMarkupParser page_markup_parser = {
	parse_page_start,
	parse_page_end,
	NULL,
	NULL,
	NULL
};

NPWPageParser*
npw_page_parser_new (NPWPage* page, const gchar* filename, gint count)
{
	NPWPageParser* parser;

	g_return_val_if_fail (page != NULL, NULL);
	g_return_val_if_fail (count >= 0, NULL);

	parser = g_new (NPWPageParser, 1);

	parser->type = NPW_PAGE_PARSER;

	parser->unknown = 0;
	parser->tag[0] = NPW_NO_TAG;
	parser->last =parser->tag;

	parser->count = count;
	parser->page = page;
	parser->property = NULL;

	parser->ctx = g_markup_parse_context_new (&page_markup_parser, 0, parser, NULL);
	g_assert (parser->ctx != NULL);

	return parser;
}

void
npw_page_parser_free (NPWPageParser* parser)
{
	g_return_if_fail (parser != NULL);

	g_markup_parse_context_free (parser->ctx);
	g_free (parser);
}

gboolean
npw_page_parser_parse (NPWPageParser* parser, const gchar* text, gssize len, GError** error)
{
	return g_markup_parse_context_parse (parser->ctx, text, len, error);
}

gboolean
npw_page_parser_end_parse (NPWPageParser* parser, GError** error)
{
	return g_markup_parse_context_end_parse (parser->ctx, error);
}

gboolean
npw_page_read (NPWPage* page, const gchar* filename, gint count)
{
	gchar* content;
	gsize len;
	NPWPageParser* parser;
	GError* err = NULL;

	g_return_val_if_fail (page != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (count < 0, FALSE);

	if (!g_file_get_contents (filename, &content, &len, &err))
	{
		g_warning (err->message);
		g_error_free (err);

		return FALSE;
	}

	parser = npw_page_parser_new (page, filename, count);

	npw_page_parser_parse (parser, content, len, &err);
	if (err == NULL) npw_page_parser_end_parse (parser, &err);

	npw_page_parser_free (parser);
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


/* Parse content block
 *---------------------------------------------------------------------------*/

typedef struct _NPWFileTag
{
	NPWTag tag;
	gchar* destination;
	gchar* source;
} NPWFileTag;

struct _NPWFileListParser
{
	/* Type of parser (not used) */
	NPWParser type;
	GMarkupParseContext* ctx;
	/* Known element stack */
	GQueue* tag;
	/* Unknown element stack */
	guint unknown;
	/* Current file list */
	GList* list;
};

static void
npw_file_tag_free (NPWFileTag *tag)
{
	g_free (tag->destination);
	g_free (tag->source);
	g_slice_free (NPWFileTag, tag);
}

/* concatenate two directories names, return value must be freed if
 * not equal to path1 or path2 */

static gchar*
concat_directory (const gchar* path1, const gchar* path2)
{
	const gchar* ptr;

	/* Check for not supported . and .. directory name in path2 */
	for (ptr = path2; ptr != '\0';)
	{
		ptr = strchr (ptr, '.');
		if (ptr == NULL) break;

		/* Exception "." only is allowed */
		if ((ptr == path2) && (ptr[1] == '\0')) break;

		if ((ptr == path2) || (ptr[- 1] == G_DIR_SEPARATOR))
		{
			if (ptr[1] == '.') ptr++;
			if ((ptr[1] == G_DIR_SEPARATOR) || (ptr[1] == '\0')) return NULL;
		}
		ptr = ptr + 1;
	}	

	if ((*path1 == '\0') || (strcmp (path1, ".") == 0) || g_path_is_absolute (path2))
	{
		return (char *)path2;
	}
	else if ((*path2 == '\0') || (strcmp (path2, ".") == 0))
	{
		return (char *)path1;
	}
	else
	{
		GString* path;

		path = g_string_new (path1);
		if (path->str[path->len -1] != G_DIR_SEPARATOR)
		{
			g_string_append_c (path, G_DIR_SEPARATOR);
		}
		g_string_append (path, path2);

		return g_string_free (path, FALSE);
	}
}

static void
parse_directory (NPWFileListParser* parser, NPWFileTag* child, const gchar** attributes, const gchar** values)
{
	const gchar* source;
	const gchar* destination;
	char* path;

	/* Set default values */
	source = NULL;
	destination = NULL;

	/* Read all attributes */
	while (*attributes != NULL)
	{
		switch (parse_attribute (*attributes))
		{
		case NPW_SOURCE_ATTRIBUTE:
			source = *values;
			break;
		case NPW_DESTINATION_ATTRIBUTE:
			destination = *values;
			break;
		default:
			parser_warning (parser->ctx, "Unknow directory attribute \"%s\"", *attributes);
			break;
		}
		attributes++;
		values++;
	}

	/* Need source or destination */
	if ((source == NULL) && (destination != NULL))
	{
		source = destination;
	}
	else if ((source != NULL) && (destination == NULL))
	{
		destination = source;
	}
	else if ((source == NULL) && (destination == NULL))
	{
		parser_warning (parser->ctx, "Missing source or destination attribute");
		child->tag = NPW_NO_TAG;

		return;
	}
		
	path = concat_directory (child->source, source);
	if (path == NULL)
	{
		parser_warning (parser->ctx, "Invalid directory source value \"%s\"", source);
		child->tag = NPW_NO_TAG;

		return;
	}
	if (path == source)
	{
		g_free (child->source);
		child->source = g_strdup (path);
	}
	else if (path != child->source) 
	{
		g_free (child->source);
		child->source = path;
	}
		

	path = concat_directory (child->destination, destination);
	if (path == NULL)
	{
		parser_warning (parser->ctx, "Invalid directory destination value \"%s\"", source);
		child->tag = NPW_NO_TAG;

		return;
	}
	if (path == destination)
	{
		g_free (child->destination);
		child->destination = g_strdup (path);
	}
	else if (path != child->destination) 
	{
		g_free (child->destination);
		child->destination = path;
	}
}	

static void
parse_file (NPWFileListParser* parser, NPWFileTag* child, const gchar** attributes, const gchar** values)
{
	const gchar* source;
	const gchar* destination;
	gchar* full_source;
	gchar* full_destination;
	gboolean execute;
	gboolean project;
	gboolean autogen;
	gboolean autogen_set;
	NPWFile* file;

	/* Set default values */
	source = NULL;
	destination = NULL;
	execute = FALSE;
	project = FALSE;
	autogen = FALSE;
	autogen_set = FALSE;

	while (*attributes != NULL)
	{
		switch (parse_attribute (*attributes))
		{
		case NPW_SOURCE_ATTRIBUTE:
			source = *values;
			break;
		case NPW_DESTINATION_ATTRIBUTE:
			destination = *values;
			break;
		case NPW_PROJECT_ATTRIBUTE:
			project = parse_boolean_string (*values);
			break;
		case NPW_EXECUTABLE_ATTRIBUTE:
			execute = parse_boolean_string (*values);
			break;
		case NPW_AUTOGEN_ATTRIBUTE:
			autogen = parse_boolean_string (*values);
			autogen_set = TRUE;
			break;
		default:
			parser_warning (parser->ctx, "Unknow file attribute \"%s\"", *attributes);
			break;
		}
		attributes++;
		values++;
	}

	if ((source == NULL) && (destination != NULL))
	{
		source = destination;
	}
	else if ((source != NULL) && (destination == NULL))
	{
		destination = source;
	}
	else if ((source == NULL) && (destination == NULL))
	{
		parser_warning (parser->ctx, "Missing source or destination attribute");
		child->tag = NPW_NO_TAG;

		return;
	}

	full_source = concat_directory (child->source, source);
	if ((full_source == NULL) || (full_source == child->source))
	{
		parser_warning (parser->ctx, "Invalid file source value \"%s\"", source);
		child->tag = NPW_NO_TAG;

		return;
	}
	full_destination = concat_directory (child->destination, destination);
	if ((full_destination == NULL) || (full_destination == child->destination))
	{
		parser_warning (parser->ctx, "Invalid directory destination value \"%s\"", source);
		child->tag = NPW_NO_TAG;

		return;
	}

	file = npw_file_new_file (full_destination, full_source);
	parser->list = g_list_prepend (parser->list, file);
	npw_file_set_execute (file, execute);
	npw_file_set_project (file, project);
	if (autogen_set)
		npw_file_set_autogen (file, autogen ? NPW_TRUE : NPW_FALSE);

	if (source != full_source)
		g_free (full_source);
	if (destination != full_destination)
		g_free (full_destination);
}

static void
parse_file_start (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	NPWFileListParser* parser = (NPWFileListParser*)data;
	NPWTag tag;
	NPWFileTag* parent;
	NPWFileTag child;

	child.tag = NPW_NO_TAG;
	child.source = NULL;
	child.destination = NULL;

	/* Recognize element */
	if (parser->unknown  == 0)
	{
		/* Not inside an unknown element */
		tag = parse_tag (name);

		parent = g_queue_peek_head (parser->tag);
		child.source = g_strdup (parent->source);
		child.destination = g_strdup (parent->destination);
		switch (parent->tag)
		{
		case NPW_NO_TAG:
			/* Top level element */
			switch (tag)
			{
			case NPW_CONTENT_TAG:
				child.tag = tag;
				break;
			case NPW_UNKNOW_TAG:
				parser_warning (parser->ctx, "Unknown element \"%s\"", name);
				break;
			default:
				break;
			}
			break;
		case NPW_CONTENT_TAG:
			switch (tag)
			{
			case NPW_DIRECTORY_TAG:
				child.tag = tag;
				parse_directory (parser, &child, attributes, values);
				break;
			default:
				parser_warning (parser->ctx, "Unexpected element \"%s\"", name);
				break;
			}
			break;
		case NPW_DIRECTORY_TAG:
			switch (tag)
			{
			case NPW_DIRECTORY_TAG:
				child.tag = tag;
				parse_directory (parser, &child, attributes, values);
				break;
			case NPW_FILE_TAG:
				child.tag = tag;
				parse_file (parser, &child, attributes, values);
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
	if (child.tag != NPW_NO_TAG)
	{
		NPWFileTag* new_child;

		new_child = g_slice_new (NPWFileTag);
		memcpy (new_child, &child, sizeof (child));
		g_queue_push_head (parser->tag, new_child);
	}
	else
	{
		g_free (child.source);
		g_free (child.destination);
		parser->unknown++;
	}
}

static void
parse_file_end (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	NPWFileListParser* parser = (NPWFileListParser*)data;

	DEBUG_PRINT("parser_file_end");
	if (parser->unknown > 0)
	{
		/* Pop unknown element */
		parser->unknown--;
	}
	else if (((NPWFileTag *)g_queue_peek_head (parser->tag))->tag != NPW_NO_TAG)
	{
		/* Pop known element */
		npw_file_tag_free (g_queue_pop_head (parser->tag));
	}
	else
	{
		/* Know stack underflow */
		g_return_if_reached ();
	}
}

static GMarkupParser file_markup_parser = {
	parse_file_start,
	parse_file_end,
	NULL,
	NULL,
	NULL
};

NPWFileListParser*
npw_file_list_parser_new (const gchar* filename)
{
	NPWFileListParser* parser;
	NPWFileTag* root;

	g_return_val_if_fail (filename != NULL, NULL);

	parser = g_new (NPWFileListParser, 1);

	parser->type = NPW_FILE_PARSER;

	parser->unknown = 0;
	parser->tag = g_queue_new ();
	root = g_slice_new0 (NPWFileTag);
	root->tag = NPW_NO_TAG;
	root->destination = g_strdup (".");
	/* Use .wiz file path as base source directory */
	root->source = g_path_get_dirname (filename);
	g_queue_push_head (parser->tag, root);
	
	parser->list = NULL;

	parser->ctx = g_markup_parse_context_new (&file_markup_parser, 0, parser, NULL);
	g_assert (parser->ctx != NULL);

	return parser;
}

void
npw_file_list_parser_free (NPWFileListParser* parser)
{
	g_return_if_fail (parser != NULL);
	
	g_markup_parse_context_free (parser->ctx);
	DEBUG_PRINT("parser free");
	g_queue_foreach (parser->tag, (GFunc)npw_file_tag_free, NULL);
	DEBUG_PRINT("parser free ok");
	g_queue_free (parser->tag);
	g_free (parser);
}

gboolean
npw_file_list_parser_parse (NPWFileListParser* parser, const gchar* text, gssize len, GError** error)
{
	return g_markup_parse_context_parse (parser->ctx, text, len, error);
}

GList *
npw_file_list_parser_end_parse (NPWFileListParser* parser, GError** error)
{
	GList *list = NULL;
	
	if (g_markup_parse_context_end_parse (parser->ctx, error))
	{
		/* Reverse file list */
		parser->list = g_list_reverse (parser->list);
		
		list = parser->list;
	}
	
	return list;
}

/* Parse action block
 *---------------------------------------------------------------------------*/

#define NPW_ACTION_PARSER_MAX_LEVEL	2	/* Maximum number of nested elements */

struct _NPWActionListParser
{
	/* Type of parser (not used) */
	NPWParser type;
	GMarkupParseContext* ctx;
	/* Known element stack */
	NPWTag tag[NPW_ACTION_PARSER_MAX_LEVEL + 1];
	NPWTag* last;
	/* Unknown element stack */
	guint unknown;
	/* Current action list object */
	GList* list;
};

static gboolean
parse_run (NPWActionListParser* parser, const gchar** attributes, const gchar** values)
{
	const gchar* command = NULL;

	while (*attributes != NULL)
	{
		switch (parse_attribute (*attributes))
		{
		case NPW_COMMAND_ATTRIBUTE:
			command = *values;
			break;
		default:
			parser_warning (parser->ctx, "Unknown run attribute \"%s\"", *attributes);
			break;
		}
		attributes++;
		values++;
	}

	if (command == NULL)
	{
		parser_warning (parser->ctx, "Missing command attribute");
	}
	else
	{
		NPWAction* action;

		action = npw_action_new_command (command);
		parser->list = g_list_prepend (parser->list, action);
	}

	return TRUE;
}

static gboolean
parse_open (NPWActionListParser* parser, const gchar** attributes, const gchar** values)
{
	const gchar* file = NULL;

	while (*attributes != NULL)
	{
		switch (parse_attribute (*attributes))
		{
		case NPW_FILE_ATTRIBUTE:
			file = *values;
			break;
		default:
			parser_warning (parser->ctx, "Unknown open attribute \"%s\"", *attributes);
			break;
		}
		attributes++;
		values++;
	}

	if (file == NULL)
	{
		parser_warning (parser->ctx, "Missing file attribute");
	}
	else
	{
		NPWAction* action;

		action = npw_action_new_file (file);
		parser->list = g_list_prepend (parser->list, action);
	}

	return TRUE;
}

static void
parse_action_start (GMarkupParseContext* context, const gchar* name, const gchar** attributes,
	const gchar** values, gpointer data, GError** error)
{
	NPWActionListParser* parser = (NPWActionListParser*)data;
	NPWTag tag;
	gboolean known = FALSE;

	/* Recognize element */
	if (parser->unknown == 0)
	{
		/* Not inside an unknown element */
		tag = parse_tag (name);
		switch (*parser->last)
		{
		case NPW_NO_TAG:
			/* Top level element */
			switch (tag)
			{
			case NPW_ACTION_TAG:
				known = TRUE;
				break;
			case NPW_UNKNOW_TAG:
				parser_warning (parser->ctx, "Unknown element \"%s\"", name);
				break;
			default:
				break;
			}
			break;
		case NPW_ACTION_TAG:
			/* Necessary to avoid neested page element */
			switch (tag)
			{
			case NPW_RUN_TAG:
				known = parse_run (parser, attributes, values);
				break;
			case NPW_OPEN_TAG:
				known = parse_open (parser, attributes, values);
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
		g_return_if_fail ((parser->last - parser->tag) <= NPW_ACTION_PARSER_MAX_LEVEL);
		parser->last++;
		*parser->last = tag;
	}
	else
	{
		parser->unknown++;
	}
}

static void
parse_action_end (GMarkupParseContext* context, const gchar* name, gpointer data, GError** error)
{
	NPWActionListParser* parser = (NPWActionListParser*)data;
	
	if (parser->unknown > 0)
	{
		/* Pop unknown element */
		parser->unknown--;
	}
	else if (*parser->last != NPW_NO_TAG)
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

static GMarkupParser action_markup_parser = {
	parse_action_start,
	parse_action_end,
	NULL,
	NULL,
	NULL
};

NPWActionListParser*
npw_action_list_parser_new (void)
{
	NPWActionListParser* parser;
	
	parser = g_new (NPWActionListParser, 1);

	parser->type = NPW_ACTION_PARSER;

	parser->unknown = 0;
	parser->tag[0] = NPW_NO_TAG;
	parser->last = parser->tag;

	parser->list = NULL;

	parser->ctx = g_markup_parse_context_new (&action_markup_parser, 0, parser, NULL);
	g_assert (parser->ctx != NULL);

	return parser;
}

void
npw_action_list_parser_free (NPWActionListParser* parser)
{
	g_return_if_fail (parser != NULL);

	g_markup_parse_context_free (parser->ctx);
	g_free (parser);
}

gboolean
npw_action_list_parser_parse (NPWActionListParser* parser, const gchar* text, gssize len, GError** error)
{
	GError* err = NULL;
	
	g_markup_parse_context_parse (parser->ctx, text, len, &err);
	if (err != NULL)
	{
		g_warning (err->message);
	}

	return TRUE;
}

GList*
npw_action_list_parser_end_parse (NPWActionListParser* parser, GError** error)
{
	GList *list = NULL;
	
	if (g_markup_parse_context_end_parse (parser->ctx, error))
	{
		/* Reverse file list */
		parser->list = g_list_reverse (parser->list);
		
		list = parser->list;
	}
	
	return list;
}

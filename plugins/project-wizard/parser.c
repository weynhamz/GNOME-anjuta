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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
//	Parse .wiz file

#include <config.h>

#include "parser.h"

#include <glib/gdir.h>
#include <string.h>

#define PROJECT_WIZARD_EXTENSION	".wiz"
#define STRING_CHUNK_SIZE	256

typedef enum {
	NPW_NO_TAG = 0,
	NPW_PROJECT_WIZARD_TAG,
	NPW_NAME_TAG,
	NPW_DESCRIPTION_TAG,
	NPW_CATEGORY_TAG,
	NPW_ICON_TAG,
	NPW_PAGE_TAG,
	NPW_PROPERTY_TAG,
	NPW_ITEM_TAG,
	NPW_DIRECTORY_TAG,
	NPW_FILE_TAG,
	NPW_CONTENT_TAG,
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
	NPW_MANDATORY_ATTRIBUTE,
	NPW_EDITABLE_ATTRIBUTE,
	NPW_SOURCE_ATTRIBUTE,
	NPW_DESTINATION_ATTRIBUTE,
	NPW_EXECUTABLE_ATTRIBUTE,
	NPW_PROJECT_ATTRIBUTE,
	NPW_AUTOGEN_ATTRIBUTE,
	NPW_UNKNOW_ATTRIBUTE
} NPWAttribute;

typedef enum {
	NPW_HEADER_PARSER,
	NPW_PAGE_PARSER,
	NPW_FILE_PARSER
} NPWParser;

// Read directory

gboolean
npw_header_list_readdir(NPWHeaderList* this, const gchar* path)
{
	GDir* dir;
	const gchar* name;
	gchar* filename;

	// Read all project files
	dir = g_dir_open(path, 20, NULL);
	if (!dir) return FALSE;

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (g_str_has_suffix(name, PROJECT_WIZARD_EXTENSION))
		{
			filename = g_build_filename(path, name, NULL);

		       	if (!g_file_test(filename, G_FILE_TEST_IS_DIR))
			{
				if (!npw_header_list_read(this, filename))
				{
					g_warning("Unable to parse project wizard %s, skip it", filename);
				}
			}
			g_free(filename);
		}
	}

	g_dir_close(dir);

	return TRUE;
}

// Common parser function

static NPWTag
parse_tag(const char* name)
{
	if (strcmp(name, "project-wizard") == 0)
	{
		return NPW_PROJECT_WIZARD_TAG;
	}
	else if (strcmp("name", name) == 0)
	{
		return NPW_NAME_TAG;
	}
	else if (strcmp("description", name) == 0)
	{
		return NPW_DESCRIPTION_TAG;
	}
	else if (strcmp("icon", name) == 0)
	{
		return NPW_ICON_TAG;
	}
	else if (strcmp("category", name) == 0)
	{
		return NPW_CATEGORY_TAG;
	}
	else if (strcmp("page", name) == 0)
	{
		return NPW_PAGE_TAG;
	}
	else if (strcmp("property", name) == 0)
	{
		return NPW_PROPERTY_TAG;
	}
	else if (strcmp("item", name) == 0)
	{
		return NPW_ITEM_TAG;
	}	
	else if (strcmp("directory", name) == 0)
	{
		return NPW_DIRECTORY_TAG;
	}
	else if (strcmp("content", name) == 0)
	{
		return NPW_CONTENT_TAG;
	}
	else if (strcmp("file", name) == 0)
	{
		return NPW_FILE_TAG;
	}
	else
	{
		return NPW_UNKNOW_TAG;
	}
}

static NPWAttribute
parse_attribute(const char* name)
{
	if (strcmp("name", name) == 0)
	{
		return NPW_NAME_ATTRIBUTE;
	}
	else if (strcmp("_label", name) == 0)
	{
		return NPW_LABEL_ATTRIBUTE;
	}
	else if (strcmp("_description", name) == 0)
	{
		return NPW_DESCRIPTION_ATTRIBUTE;
	}
	else if (strcmp("default", name) == 0 || strcmp("value", name) == 0)
	{
		return NPW_VALUE_ATTRIBUTE;
	}
	else if (strcmp("type", name) == 0)
	{
		return NPW_TYPE_ATTRIBUTE;
	}
	else if (strcmp("summary", name) == 0)
	{
		return NPW_SUMMARY_ATTRIBUTE;
	}
	else if (strcmp("mandatory", name) == 0)
	{
		return NPW_MANDATORY_ATTRIBUTE;
	}
	else if (strcmp("editable", name) == 0)
	{
		return NPW_EDITABLE_ATTRIBUTE;
	}
	else if (strcmp("source", name) == 0)
	{
		return NPW_SOURCE_ATTRIBUTE;
	}
	else if (strcmp("destination", name) == 0)
	{
		return NPW_DESTINATION_ATTRIBUTE;
	}
	else if (strcmp("executable", name) == 0)
	{
		return NPW_EXECUTABLE_ATTRIBUTE;
	}
	else if (strcmp("project", name) == 0)
	{
		return NPW_PROJECT_ATTRIBUTE;
	}
	else if (strcmp("autogen", name) == 0)
	{
		return NPW_AUTOGEN_ATTRIBUTE;
	}
	else
	{
		return NPW_UNKNOW_ATTRIBUTE;
	}
}

static gboolean
parse_boolean_string(const gchar* value)
{
	return g_ascii_strcasecmp("no", value) && g_ascii_strcasecmp("0", value) && g_ascii_strcasecmp("false", value);
}

// Parse Header

typedef struct _NPWHeaderParser
{
	NPWTag parent;
	NPWTag tag;
	gint tag_count;
	gint unknown;
	NPWParser type;
	GMarkupParseContext* ctx;
	NPWHeaderList* list;
	NPWHeader* header;
	gchar* path;
} NPWHeaderParser;

static void
header_parse_start_element (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	NPWHeaderParser* parser = (NPWHeaderParser*)data;
	NPWTag tag;

	if (parser->unknown != 0)
	{
		// Parsing unknown tags
		parser->unknown++;

		return;
	}
	
	tag = parse_tag(name);
	if (parser->parent == NPW_NO_TAG)
	{
		if (tag == NPW_PROJECT_WIZARD_TAG)
		{
			parser->header = npw_header_new(parser->list);
			parser->parent = tag;
		}
		else
		{
			parser->unknown++;
		}
	}
	else if (parser->parent == NPW_PROJECT_WIZARD_TAG)
	{
		parser->tag = tag;
	}
}

static void
header_parse_end_element (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	NPWHeaderParser* parser = (NPWHeaderParser*)data;

	if (parser->unknown)
	{
		parser->unknown--;
	}
	else if (parser->tag != NPW_NO_TAG)
	{
		parser->tag = NPW_NO_TAG;
	}
	else if (parser->parent != NPW_NO_TAG)
	{
		parser->parent = NPW_NO_TAG;
	}
	else
	{
		// error
	}
}

static void
header_parse_text (GMarkupParseContext* context,
       	const gchar* text,
	gsize len,
	gpointer data,
	GError** error)
{
	NPWHeaderParser* parser = (NPWHeaderParser*)data;
	char* filename;

	switch (parser->tag)
	{
	case NPW_NAME_TAG:
		npw_header_set_name(parser->header, text);
		break;
	case NPW_DESCRIPTION_TAG:
		npw_header_set_description(parser->header, text);
		break;
	case NPW_ICON_TAG:
		filename = g_build_filename(parser->path, text, NULL);
		npw_header_set_iconfile(parser->header, filename);
		g_free(filename);
		break;
	case NPW_CATEGORY_TAG:
		npw_header_set_category(parser->header, text);
		break;
	default:
		break;
	}
}

static void
header_parse_error (GMarkupParseContext* context,
	GError* error,
	gpointer data)
{
}

static GMarkupParser header_markup_parser = {
	header_parse_start_element,
	header_parse_end_element,
	header_parse_text,
	NULL,
	header_parse_error
};


gboolean
npw_header_list_read(NPWHeaderList* this, const gchar* filename)
{
	gchar* content;
	gsize len;
	NPWHeaderParser parser;

	if (!g_file_get_contents(filename, &content, &len, NULL)) return FALSE;

	parser.unknown = 0;
	parser.parent = NPW_NO_TAG;
	parser.tag = NPW_NO_TAG;
	parser.list = this;
	parser.header = NULL;
	parser.type = NPW_HEADER_PARSER;
	parser.path = g_path_get_dirname(filename);

	parser.ctx = g_markup_parse_context_new(&header_markup_parser, 0, &parser, NULL);
	if (!parser.ctx)
	{
		g_free(parser.path);
		g_free(content);
	       	return FALSE;
	}

/*	if (!g_markup_parse_context_parse(ctx, content, len, NULL))
	{
		g_error("Parsing of project file %s fail", filename);

		g_free(parser.path);
		if (parser.header != NULL)
			npw_header_destroy(parser.header);
		g_free(content);
		return FALSE;
	}*/
	g_markup_parse_context_parse(parser.ctx, content, len, NULL);

	npw_header_set_filename(parser.header, filename);
	g_markup_parse_context_free(parser.ctx);
	g_free(parser.path);
	g_free(content);

	return TRUE;	
}

// Read page

#define NPW_PAGE_PARSER_MAX_LEVEL	3

struct _NPWPageParser
{
	// Known element stack
	NPWTag tag[NPW_PAGE_PARSER_MAX_LEVEL];
	guint level;
	// Unknown element stack
	guint unknown;

	gint count;
	NPWParser type;
	GMarkupParseContext* ctx;
	NPWPage* page;
	NPWProperty* property;
};

static gboolean
npw_page_parse_page_element(NPWPageParser* this, 
	const gchar** attributes,
	const gchar** values)
{
	gboolean ok = TRUE;

	if (this->count != 0)
	{
		// Skip this page
		if (this->count > 0) this->count--;
		// Push in unknown element stack
		this->unknown++;
	}
	else
	{
		// Read this page
		while (*attributes != NULL)
		{
			switch (parse_attribute(*attributes))
			{
			case NPW_NAME_ATTRIBUTE:
				npw_page_set_name(this->page, *values);
				break;
			case NPW_LABEL_ATTRIBUTE:
				npw_page_set_label(this->page, *values);
				break;
			case NPW_DESCRIPTION_ATTRIBUTE:
				npw_page_set_description(this->page, *values);
				break;
			default:
				g_warning ("Unknown page attribute \"%s\"", *attributes);
				ok = FALSE;		
			}
			attributes++;
			values++;
		}
		// Push in known element stack
		this->tag[this->level] = NPW_PAGE_TAG;
		this->level++;
		this->count--;
	}

	return ok;
}

static gboolean
npw_page_parse_property_element(NPWPageParser* this,
	const gchar** attributes,
	const gchar** values)
{
	gboolean ok = TRUE;

	this->property = npw_property_new(this->page);
	while (this->property && *attributes != NULL)
	{
		switch (parse_attribute(*attributes))
		{
		case NPW_TYPE_ATTRIBUTE:
			npw_property_set_string_type(this->property, *values);
			break;
		case NPW_NAME_ATTRIBUTE:
			npw_property_set_name(this->property, *values);
			break;
		case NPW_LABEL_ATTRIBUTE:
			npw_property_set_label(this->property, *values);
			break;
		case NPW_DESCRIPTION_ATTRIBUTE:
			npw_property_set_description(this->property, *values);
			break;
		case NPW_VALUE_ATTRIBUTE:
			npw_property_set_default(this->property, *values);
			break;
		case NPW_SUMMARY_ATTRIBUTE:
			npw_property_set_summary_option(this->property, parse_boolean_string(*values));
			break;		
		case NPW_MANDATORY_ATTRIBUTE:
			npw_property_set_mandatory_option(this->property, parse_boolean_string(*values));
			break;
		case NPW_EDITABLE_ATTRIBUTE:
			npw_property_set_editable_option(this->property, parse_boolean_string(*values));
			break;
		default:
			g_warning ("Unknown property attribte \"%s\"", *attributes);
			ok = FALSE;
			break;
		}
		attributes++;
		values++;
	}	
	// Push in known element stack
	this->tag[this->level] = NPW_PROPERTY_TAG;
	this->level++;

	return ok;
}

static gboolean
npw_page_parse_page_child(NPWPageParser* this, 
	NPWTag tag,	
	const gchar** attributes,
	const gchar** values)
{
	gboolean ok = TRUE;

	// Inside a page element
	switch (tag)
	{
	case NPW_PROPERTY_TAG:
		ok = npw_page_parse_property_element(this, attributes, values);
		return ok;
	default:
		ok = FALSE;
	}
	
	this->unknown++;

	return ok;
}

static gboolean
npw_page_parse_item_element(NPWPageParser* this,
	const gchar** attributes,
	const gchar** values)
{
	gboolean ok = TRUE;
	const gchar* label = NULL;
	const gchar* name = NULL;

	while (*attributes != NULL)
	{
		switch (parse_attribute(*attributes))
		{
		case NPW_NAME_ATTRIBUTE:
			name = *values;
			break;
		case NPW_LABEL_ATTRIBUTE:
			label = *values;
			break;
		default:
			g_warning ("Unknown item attribute \"%s\"", *attributes);
			ok = FALSE;
			break;
		}
		attributes++;
		values++;
	}
	if (this->property != NULL)
	{
		npw_property_add_list_item(this->property, name, label == NULL ? name : label);
	}

	// Push in known element stack
	this->tag[this->level] = NPW_ITEM_TAG;
	this->level++;

	return ok;
}


static gboolean
npw_page_parse_property_child(NPWPageParser* this,
		NPWTag tag,
		const gchar** attributes,
		const gchar** values)
{
	gboolean ok = TRUE;

	// Inside a property element
	switch  (tag)
	{
	case NPW_ITEM_TAG:
		ok = npw_page_parse_item_element(this, attributes, values);
		return ok;
	default:
		ok = FALSE;
	}

	this->unknown++;

	return ok;
}

static void
page_parse_start_element (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	NPWTag tag;

	if (parser->unknown > 0)
	{
		// All element inside an unknown tag are unknown
		parser->unknown++;

		return;
	}

	tag = parse_tag(name);
	switch (parser->level)
	{
	case 0:
		// Known element stack is empty
		if (tag == NPW_PAGE_TAG)
		{
			npw_page_parse_page_element(parser, attributes, values);
			return;
		}
		break;
	case 1:
		if (parser->tag[parser->level - 1] == NPW_PAGE_TAG)
		{
			npw_page_parse_page_child(parser, tag, attributes, values);
			return;
		}
		break;
	case 2:
		if (parser->tag[parser->level - 1] == NPW_PROPERTY_TAG)
		{
			npw_page_parse_property_child(parser, tag, attributes, values);
			return;
		}
		break;
	default:
		break;
	}

	// Push in unknown element stack
	parser->unknown++;
}

static void
page_parse_end_element (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	
	if (parser->unknown > 0)
	{
		// Pop unknown element
		parser->unknown--;
	}
	else if (parser->level > 0)
	{
		// Pop known element
		parser->level--;
	}
	else
	{
		// Error: end marker without a start
		g_warning ("In error condition, possible malformed wizard file");
	}
}

static void
page_parse_error (GMarkupParseContext* context,
	GError* error,
	gpointer data)
{
}

static GMarkupParser page_markup_parser = {
	page_parse_start_element,
	page_parse_end_element,
	NULL,
	NULL,
	page_parse_error
};

NPWPageParser*
npw_page_parser_new(NPWPage* page, const gchar* filename, gint count)
{
	NPWPageParser* this;

	this = g_new(NPWPageParser, 1);

	// Clear element stack
	this->level = 0;
	// Clear unknow element stack
	this->unknown = 0;

	this->count = count;
	this->page = page;
	this->property = NULL;
	this->type = NPW_PAGE_PARSER;

	this->ctx = g_markup_parse_context_new(&page_markup_parser, 0, this, NULL);
	if (!this->ctx)
	{
		npw_page_parser_destroy(this);
	       	return NULL;
	}

	return this;
}

void
npw_page_parser_destroy(NPWPageParser* this)
{
	if (this->ctx != NULL)
	{
		g_markup_parse_context_free(this->ctx);
	}
	g_free(this);
}

gboolean
npw_page_parser_parse(NPWPageParser* this, const gchar* text, gssize len, GError** error)
{
	return g_markup_parse_context_parse(this->ctx, text, len, error);
}

gboolean
npw_page_parser_end_parse(NPWPageParser* this, GError** error)
{
	return g_markup_parse_context_end_parse(this->ctx, error);
}

gboolean
npw_page_read(NPWPage* this, const gchar* filename, gint count)
{
	gchar* content;
	gsize len;
	NPWPageParser* parser;

	if (!g_file_get_contents(filename, &content, &len, NULL)) return FALSE;

	parser = npw_page_parser_new(this, filename, count);

	if (parser == NULL)
	{
		g_free(content);
	       	return FALSE;
	}

	if (!npw_page_parser_parse(parser, content, len, NULL))
	{
		g_error("Parsing of project file %s fail", filename);

		npw_page_parser_destroy(parser);
		g_free(content);
		return FALSE;
	}
	npw_page_parser_end_parse(parser, NULL);

	npw_page_parser_destroy(parser);
	g_free(content);

	return TRUE;	
}


// Read filelist

struct _NPWFileListParser
{
	guint unknown;
	NPWTag parent;
	NPWTag tag;
	NPWParser type;
	GMarkupParseContext* ctx;
	GQueue* dst_path;
	GQueue* src_path;
	GStringChunk* pool;
	NPWFileList* list;
};

static void
file_parse_start_element (GMarkupParseContext* context,
       	const gchar* name,
	const gchar** attributes,
	const gchar** values,
	gpointer data,
	GError** error)
{
	NPWFileListParser* parser = (NPWFileListParser*)data;
	NPWTag tag;

	if (parser->unknown > 0)
	{
		// Parsing unknown tags
		parser->unknown++;

		return;
	}
	
	tag = parse_tag(name);
	if (parser->parent == NPW_NO_TAG)
	{
		if (tag == NPW_CONTENT_TAG) 
		{
			parser->parent = tag;
		}
		else
		{
			parser->unknown++;
		}
	}
	else if (parser->parent == NPW_CONTENT_TAG)
	{
		const gchar* source;
		const gchar* destination;
		gboolean execute;
		gboolean project;
		gboolean autogen;
		gboolean autogen_set;
		char* path;
		char* fullname;
		NPWFile* file;

		parser->tag = tag;
		source = NULL;
		destination = NULL;
		execute = FALSE;
		project = FALSE;
		autogen_set = FALSE;
		while (*attributes != NULL)
		{
			switch (parse_attribute(*attributes))
			{
			case NPW_SOURCE_ATTRIBUTE:
				source = *values;
				break;
			case NPW_DESTINATION_ATTRIBUTE:
				destination = *values;
				break;
			case NPW_PROJECT_ATTRIBUTE:
				project = parse_boolean_string(*values);
				break;
			case NPW_EXECUTABLE_ATTRIBUTE:
				execute = parse_boolean_string(*values);
				break;
			case NPW_AUTOGEN_ATTRIBUTE:
				autogen = parse_boolean_string(*values);
				autogen_set = TRUE;
				break;
			default:
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
			parser->unknown++;
			return;
		}
			
		switch(tag)
		{
		case NPW_FILE_TAG:
			file = npw_file_new(parser->list);
			npw_file_set_type(file, NPW_FILE);
			path = g_queue_peek_nth(parser->src_path,0);
			if (g_path_is_absolute (source) || path == NULL)
			{
				npw_file_set_source(file, source);
			}
			else
			{
				fullname = g_build_filename(path, source, NULL);
				npw_file_set_source(file, fullname);
				g_free(fullname);
			}
			path = g_queue_peek_nth(parser->dst_path, 0);
			if (g_path_is_absolute(destination) || path == NULL)
			{
				npw_file_set_destination(file, destination);
			}
			else
			{
				fullname = g_build_filename(path, destination, NULL);
				npw_file_set_destination(file, fullname);
				g_free(fullname);
			}
			npw_file_set_execute(file, execute);
			npw_file_set_project(file, project);
			if (autogen_set)
			{
				npw_file_set_autogen(file, autogen ? NPW_TRUE : NPW_FALSE);
			}
			break;
		case NPW_DIRECTORY_TAG:
			if (g_path_is_absolute(source))
			{
				path = g_string_chunk_insert(parser->pool, source);
			}
			else
			{
				path = g_queue_peek_nth(parser->src_path, 0);
				if (strcmp(source, ".") == 0)
				{
					// Nothing to do
				}
				else if (path == NULL)
				{
					path = g_string_chunk_insert(parser->pool, source);
				}
				else
				{
					fullname = g_build_filename(path, source, NULL);
					path = g_string_chunk_insert(parser->pool, fullname);
					g_free(fullname);
				}
			}
			g_queue_push_head(parser->src_path, path);
			if (g_path_is_absolute(destination))
			{
				path = g_string_chunk_insert(parser->pool, destination);
			}
			else
			{
				path = g_queue_peek_nth(parser->dst_path, 0);
				if (strcmp(destination, ".") == 0)
				{
					// Nothing to do
				}
				else if (path == NULL)
				{
					path = g_string_chunk_insert(parser->pool, destination);
				}
				else
				{
					fullname = g_build_filename(path, destination, NULL);
					path = g_string_chunk_insert(parser->pool, fullname);
					g_free(fullname);
				}
			}	
			g_queue_push_head(parser->dst_path, path);
			break;
		default:
			g_warning ("Unknow content tag \"%s\"", name);
			parser->unknown++;
			break;
		}
	}
}

static void
file_parse_end_element (GMarkupParseContext* context,
       	const gchar* name,
	gpointer data,
	GError** error)
{
	NPWFileListParser* parser = (NPWFileListParser*)data;
	NPWTag tag;

	tag = parse_tag(name);
	if (parser->unknown > 0)
	{
		parser->unknown--;
	}
	else if (tag != NPW_CONTENT_TAG && parser->parent == NPW_CONTENT_TAG)
	{
		if (tag == NPW_DIRECTORY_TAG)
		{
			g_queue_pop_head(parser->src_path);
			g_queue_pop_head(parser->dst_path);
			// String stay allocated in the pool
		}
	}
	else if (tag == NPW_CONTENT_TAG)
	{
		parser->parent = NPW_NO_TAG;
	}
	/*
	else if (parser->tag != NPW_NO_TAG)
	{
		if (parser->tag == NPW_DIRECTORY_TAG)
		{
			g_queue_pop_head(parser->src_path);
			g_queue_pop_head(parser->dst_path);
			// String stay allocated in the pool
		}
		parser->tag = NPW_NO_TAG;
	}
	else if (parser->parent != NPW_NO_TAG)
	{
		parser->parent = NPW_NO_TAG;
	}
	*/
	else
	{
		g_warning ("Parse error, Possible malformed .wiz file");
		// error
	}
}

static void
file_parse_error (GMarkupParseContext* context,
	GError* error,
	gpointer data)
{
}

static GMarkupParser file_markup_parser = {
	file_parse_start_element,
	file_parse_end_element,
	NULL,
	NULL,
	file_parse_error
};

NPWFileListParser*
npw_file_list_parser_new(NPWFileList* list, const gchar* filename)
{
	NPWFileListParser* this;
	gchar* path;

	this = g_new(NPWFileListParser, 1);
	this->unknown = 0;
	this->parent = NPW_NO_TAG;
	this->tag = NPW_NO_TAG;
	this->type = NPW_PAGE_PARSER;
	this->dst_path = g_queue_new();
	this->src_path = g_queue_new();
	this->pool = g_string_chunk_new(STRING_CHUNK_SIZE);
	this->list = list;

	// Append .wiz file path as base file
	path = g_path_get_dirname(filename);
	g_queue_push_head(this->src_path, g_string_chunk_insert(this->pool, path));
	g_free(path);

	this->ctx = g_markup_parse_context_new(&file_markup_parser, 0, this, NULL);
	if (!this->ctx)
	{
		npw_file_list_parser_destroy(this);

		return NULL;
	}

	return this;
}

void
npw_file_list_parser_destroy(NPWFileListParser* this)
{
	if (this->ctx != NULL)
	{
		g_markup_parse_context_free(this->ctx);
	}

	g_string_chunk_free(this->pool);
	g_queue_free(this->dst_path);
	g_queue_free(this->src_path);
	g_free(this);
}

gboolean
npw_file_list_parser_parse(NPWFileListParser* this, const gchar* text, gssize len, GError** error)
{
	return g_markup_parse_context_parse(this->ctx, text, len, error);
}

gboolean
npw_file_list_parser_end_parse(NPWFileListParser* this, GError** error)
{
	return g_markup_parse_context_end_parse(this->ctx, error);
}

gboolean
npw_file_list_read(NPWFileList* this, const gchar* filename)
{
	gchar* content;
	gsize len;
	NPWFileListParser* parser;

	if (!g_file_get_contents(filename, &content, &len, NULL)) return FALSE;

	parser = npw_file_list_parser_new(this, filename);

	if (parser == NULL)
	{
		g_free(content);
	       	return FALSE;
	}

	if (!npw_file_list_parser_parse(parser, content, len, NULL))
	{
		g_error("Parsing of project file %s fail", filename);

		npw_file_list_parser_destroy(parser);
		g_free(content);
		return FALSE;
	}
	npw_file_list_parser_end_parse(parser, NULL);

	npw_file_list_parser_destroy(parser);
	g_free(content);

	return TRUE;	
}

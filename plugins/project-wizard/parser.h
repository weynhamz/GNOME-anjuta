/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    parser.h
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

#ifndef __PARSER_H__
#define __PARSER_H__

#include <glib.h>

#include "header.h"
#include "property.h"
#include "file.h"
#include "action.h"

gboolean npw_header_list_readdir (GList** this, const gchar* pathname);

gboolean npw_header_list_read (GList** this, const gchar* filename);


typedef struct _NPWPageParser NPWPageParser;

NPWPageParser* npw_page_parser_new (NPWPage* page, const gchar* filename, gint count);
void npw_page_parser_free (NPWPageParser* parser);

gboolean npw_page_parser_parse (NPWPageParser* parser, const gchar* text, gssize len, GError** error);
gboolean npw_page_parser_end_parse (NPWPageParser* parser, GError** error);

gboolean npw_page_read (NPWPage* page, const gchar* filename, gint count);


typedef struct _NPWFileListParser NPWFileListParser;

NPWFileListParser* npw_file_list_parser_new (const gchar* filename);
void npw_file_list_parser_free (NPWFileListParser* parser);

gboolean npw_file_list_parser_parse (NPWFileListParser* parser, const gchar* text, gssize len, GError** error);
GList* npw_file_list_parser_end_parse (NPWFileListParser* parser, GError** error);


typedef struct _NPWActionListParser NPWActionListParser;

NPWActionListParser* npw_action_list_parser_new (void);
void npw_action_list_parser_free (NPWActionListParser* parser);

gboolean npw_action_list_parser_parse (NPWActionListParser* parser, const gchar* text, gssize len, GError** error);
GList* npw_action_list_parser_end_parse (NPWActionListParser* parser, GError** error);

#endif

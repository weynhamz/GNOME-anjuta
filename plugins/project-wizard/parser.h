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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __PARSER_H__
#define __PARSER_H__

#include <glib.h>

#include "header.h"
#include "property.h"
#include "file.h"

gboolean npw_header_list_readdir(NPWHeaderList* this, const gchar* pathname);

gboolean npw_header_list_read(NPWHeaderList* this, const gchar* filename);


typedef struct _NPWPageParser NPWPageParser;

NPWPageParser* npw_page_parser_new(NPWPage* page, const gchar* filename, gint count);
void npw_page_parser_destroy(NPWPageParser* this);

gboolean npw_page_parser_parse(NPWPageParser* this, const gchar* text, gssize len, GError** error);
gboolean npw_page_parser_end_parse(NPWPageParser* this, GError** error);

gboolean npw_page_read(NPWPage* this, const gchar* filename, gint count);


typedef struct _NPWFileListParser NPWFileListParser;

NPWFileListParser* npw_file_list_parser_new(NPWFileList* list, const gchar* filename);
void npw_file_list_parser_destroy(NPWFileListParser* this);

gboolean npw_file_list_parser_parse(NPWFileListParser* this, const gchar* text, gssize len, GError** error);
gboolean npw_file_list_parser_end_parse(NPWFileListParser* this, GError** error);

gboolean npw_file_list_read(NPWFileList* this, const gchar* filename);

#endif

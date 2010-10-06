/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-xml-parser.h
    Copyright (C) Dragos Dena 2010

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#include <glib.h>
#include "snippets-group.h"
#include "snippets-db.h"

GList*       snippets_manager_parse_snippets_xml_file  (const gchar* snippet_packet_path,
                                                        FormatType format_type);
gboolean     snippets_manager_save_snippets_xml_file   (FormatType format_type,
                                                        GList* snippets_groups,
                                                        const gchar *file_path);
gboolean     snippets_manager_parse_variables_xml_file (const gchar* global_vars_file_path,
                                                        SnippetsDB* snippets_db);
gboolean     snippets_manager_save_variables_xml_file  (const gchar* global_variables_file_path,
                                                        GList* global_vars_name_list,
                                                        GList* global_vars_value_list,
                                                        GList* global_vars_is_command_list);

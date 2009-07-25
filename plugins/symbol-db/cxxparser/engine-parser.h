/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2009 <maxcvs@email.it>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ENGINE_MANAGER_H_
#define _ENGINE_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include <libanjuta/interfaces/ianjuta-symbol-manager.h>		

// FIXME: actually a dbe instance is passed. Change here when integrating with Anjuta.
void engine_parser_init (SymbolDBEngine* manager);
	
void engine_parser_test_print_tokens (const char *str);

void engine_parser_parse_expression (const char*str);	

void engine_parser_process_expression (const char *stmt, const char * above_text,
    const char * full_file_path, unsigned long linenum);	

void engine_parser_test_optimize_scope (const char*str);

void engine_parser_get_local_variables (const char *buf);
//*/	
#ifdef __cplusplus
}	// extern "C" 
#endif
//*/

#endif // _ENGINE_PARSER_H_

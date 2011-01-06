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

#include <libanjuta/interfaces/ianjuta-symbol-manager.h>		

void engine_parser_init (IAnjutaSymbolManager * manager);

void engine_parser_deinit ();
	
/**
 * The function parse the C++ statement, try to get the type of objects to be
 * completed and returns an iterator with those symbols.
 * @param stmt A statement like "((FooKlass*) B).", "foo_klass1->get_foo_klass2 ()->",
 * "foo_klass1->member2->", etc.
 * @above_text Text of the buffer/file before the statement up to the first byte.
 * @param full_file_path The full path to the file. This is for engine scanning purposes.
 * @param linenum The line number where the statement is.
 *	 
 * @return IAnjutaIterable * with the actual completions symbols.
 */
IAnjutaIterable *
engine_parser_process_expression (const gchar *stmt, const gchar * above_text,
    const gchar * full_file_path, gulong linenum);	

#ifdef __cplusplus
}	// extern "C" 
#endif


#endif // _ENGINE_PARSER_H_

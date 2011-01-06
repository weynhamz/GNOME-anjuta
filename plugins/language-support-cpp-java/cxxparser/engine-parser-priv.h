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

#ifndef _ENGINE_PARSER_PRIV_H_
#define _ENGINE_PARSER_PRIV_H_

#include <string>
#include <vector>


#ifdef __cplusplus
extern "C" {
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
}
#endif

#include "expression-result.h"
#include "cpp-flex-tokenizer.h"

using namespace std;


class EngineParser
{
public:

	static EngineParser* getInstance ();

	/* setter for the IAnjutaSymbolManager. */
	void setSymbolManager (IAnjutaSymbolManager *manager);
	void unsetSymbolManager ();

	void getNearestClassInCurrentScopeChainByFileLine (const char* full_file_path,
	                                                   unsigned long linenum,
	                                                   string &out_type_name);

	IAnjutaIterable * getCurrentSearchableScope (string &type_name, string &type_scope);
	
	bool getTypeNameAndScopeByToken (ExpressionResult &result, 
	    							string &token,
	    							string &op,
	    							const string& full_file_path, 
    								unsigned long linenum,
	    							const string& above_text,
    								string &out_type_name, 		/* out */
	    							string &out_type_scope);	/* out */

	IAnjutaIterable * switchTypedefToStruct (IAnjutaIterable * test, 
		IAnjutaSymbolField sym_info = 
			(IAnjutaSymbolField)(IANJUTA_SYMBOL_FIELD_NAME | IANJUTA_SYMBOL_FIELD_KIND));

	IAnjutaIterable * switchMemberToContainer (IAnjutaIterable * test);
	
	/* FIXME comments. */
	IAnjutaIterable * processExpression (const string& stmt, 
    				  							const string& above_text,
    				  							const string& full_file_path, 
    				  							unsigned long linenum);
	
protected:

	EngineParser ();

	virtual ~EngineParser ();

	/**
	 * Parse an expression and return the result. 
	 * @param in Input string expression
	 * @return An ExpressionResult struct. If it fails to parse it, 
	 * check result.m_name.empty() for success
	 */
	ExpressionResult parseExpression(const string &in);
	
private:
	
	/**
	 * Return the next token and the delimiter found, the source string is taken from the
	 * _main_tokenizer member of this class.
	 *
	 * @param token Next token
	 * @param delim Delimiter found (as ".", "::", or "->")
	 * @return true If token was found false otherwise
	 */	
	bool nextMainToken (string &out_token, string &out_delimiter);

	/**
	 * Trim a string using some default chars.
	 * The code is expected to run quite performantly, as STL doesn't provide
	 * a method to trim a string.
	 */
	void trim (string& str, string trimChars = "{};\r\n\t\v ");

	/**
	 * This method reduces the various scopes/variables/functions in the buffer 
	 * passed as parameter to a file where the only things left are the local
	 * variables and the functions names. 
	 * You can use this method to retrieve the type of a local variable, if it's
	 * present in the passed buffer of course.
	 */
	string optimizeScope(const string& srcString);
	
	/*
	 * D A T A
	 */	
	static EngineParser *s_engine;	

	CppTokenizer *_main_tokenizer;
	CppTokenizer *_extra_tokenizer;
	
	IAnjutaSymbolQuery *_query_scope;
	IAnjutaSymbolQuery *_query_search;
	IAnjutaSymbolQuery *_query_search_in_scope;
	IAnjutaSymbolQuery *_query_parent_scope;
};


/* singleton */
EngineParser *EngineParser::s_engine = NULL;


#endif // _ENGINE_PARSER_PRIV_H_

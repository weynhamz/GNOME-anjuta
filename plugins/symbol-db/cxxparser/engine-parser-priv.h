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

//#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
extern "C" {
#include "../symbol-db-engine.h"
}
#include "expression-result.h"
#include "cpp-flex-tokenizer.h"

using namespace std;


class EngineParser
{
public:

	static EngineParser* getInstance ();

	void DEBUG_printTokens (const string& text);

	// setter for the IAnjutaSymbolManager.
	void setSymbolManager (SymbolDBEngine *manager);

	// getter for the IAnjutaSymbolManager.
	SymbolDBEngine * getSymbolManager ();

	// FIXME comments.
	/**
	 * Evaluate a C++ expression. for example, the following expression: '((Notebook*)book)->'
	 * will be processed into typeName=Notebook, and typeScope=<global> (assuming Notebook is not
	 * placed within any namespace)
	 * \param stmt c++ expression
	 * \param text text where this expression was found
	 * \param fn filename context
	 * \param lineno current line number
	 * \param typeName [output]
	 * \param typeScope [output]
	 * \param oper [output] return the operator used (::, ., ->)
	 * \param scopeTemplateInitList [output] return the scope tempalte intialization (e.g. "std::auto_ptr<wxString> str;" -> <wxString>
	 * \return true on success, false otherwise. The output fields are only to be checked with the return
	 * valus is 'true'
	 */
	SymbolDBEngineIterator *
		processExpression(const string& stmt, const string& above_text, 
	    	const string& full_file_path, unsigned long linenum, 
		    string &out_type_name, string &out_type_scope, 
		    string &out_oper, string &out_scope_template_init_list);
	

	void testParseExpression (const string &in);

	string GetScopeName(const string &in, std::vector<string> *additionlNS);

	
protected:

	EngineParser ();

	/**
	 * parse an expression and return the result. this functions uses
	 * the sqlite database as its symbol table
	 * \param in input string expression
	 * \return ExpressionResult, if it fails to parse it, check result.m_name.empty() for success
	 */
	ExpressionResult parseExpression(const string &in);
private:
	/**
	 * Return the next token and the delimiter found, the source string is taken from the
	 * m_tokenScanner member of this class
	 * \param token next token
	 * \param delim delimiter found
	 * \return true if token was found false otherwise
	 */	
	bool nextToken (string &out_token, string &out_delimiter);


	/**
	 * trim () a string
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

	CppTokenizer *_tokenizer;
	SymbolDBEngine *_dbe;
};


/* singleton */
EngineParser *EngineParser::s_engine = NULL;


#endif // _ENGINE_PARSER_PRIV_H_

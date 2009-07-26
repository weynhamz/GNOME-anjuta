/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Eran Ifrah (Main file for CodeLite www.codelite.org/ )
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

#include "engine-parser-priv.h"
#include "engine-parser.h"
#include "flex-lexer-klass-tab.h"
#include "expression-parser.h"
#include "scope-parser.h"
#include "variable-parser.h"

#include <string>
#include <vector>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

using namespace std;

// Singleton
EngineParser* 
EngineParser::getInstance ()
{
	if (s_engine == NULL)
	{
		s_engine = new EngineParser ();
	}

	return s_engine;
}

EngineParser::EngineParser ()
{	
	_tokenizer = new CppTokenizer ();	
	_dbe = NULL;
}

bool 
EngineParser::nextToken (string &out_token, string &out_delimiter)
{
	int type(0);
	int depth(0);
	while ( (type = _tokenizer->yylex()) != 0 ) 
	{		
		switch (type) 
		{			
		case CLCL:
		case '.':
		case lexARROW:
			if (depth == 0) 
			{
				out_delimiter = _tokenizer->YYText();
				return true;
			} else 
			{
				out_token.append (" ").append (_tokenizer->YYText());
			}
			break;
				
		case '<':
		case '[':
		case '(':
		case '{':
			depth++;
			out_token.append (" ").append (_tokenizer->YYText());
			break;
				
		case '>':
		case ']':
		case ')':
		case '}':
			depth--;
			out_token.append (" ").append (_tokenizer->YYText());
			break;
				
		default:
			out_token.append (" ").append (_tokenizer->YYText());
			break;
		}
	}
	return false;
}

void 
EngineParser::DEBUG_printTokens (const string& text)
{
	// FIXME
	_tokenizer->setText (text.c_str ());

	string op;
	string token;
	int i = 0;
	while (nextToken(token, op)) 
	{
		printf ("tok %d %s [op %s]\n", i, token.c_str (), op.c_str ());
		//ExpressionResult result = parse_expression(token);
		//result.Print ();
		i++;
		token.clear ();
	}
	printf ("tok final %s\n", token.c_str ());	


	// try to do a symbol search

}

ExpressionResult 
EngineParser::parseExpression(const string &in)
{
	return parse_expression (in.c_str ());	
}

void
EngineParser::testParseExpression (const string &str)
{
	_tokenizer->setText(str.c_str ());

	string word;
	string op;
	ExpressionResult result;
	
	while (nextToken (word, op)) {

		cout << "--------\ngot word " << word << " op " << op << endl; 
		// fill up ExpressionResult
		result = parseExpression (word);

		result.Print ();

		word.clear ();
	}
	
//	ExpressionResult res = parseExpression (str);

//	res.Print ();	
}

void 
EngineParser::setSymbolManager (SymbolDBEngine *manager)
{
	_dbe = manager;
}

SymbolDBEngine * 
EngineParser::getSymbolManager ()
{
	return _dbe;
}

void 
EngineParser::trim (string& str, string trimChars)
{
  	string::size_type pos = str.find_last_not_of (trimChars);
	
  	if (pos != string::npos) 
	{
    	str.erase(pos + 1);
    	pos = str.find_first_not_of (trimChars);
    	if(pos != string::npos) 
		{			
			  str.erase(0, pos);
		}
  	}
  	else 
	{
		str.erase(str.begin(), str.end());
	}
}

SymbolDBEngineIterator *
EngineParser::processExpression(const string& stmt, const string& above_text,
    const string& full_file_path, unsigned long linenum,
    string &out_type_name, string &out_type_scope, string &out_oper, 
	    string &out_scope_template_init_list)
{
	bool evaluation_succeed = false;
	_tokenizer->setText(stmt.c_str ());

	string word;
	string op;
	string scope_name;
	ExpressionResult result;
	
	while (nextToken (word, op)) 
	{
		trim (word);
		
		cout << "--------\ngot word ->" << word << "<- op " << op << endl; 
		out_oper = op;

		
		
		// fill up ExpressionResult
		result = parseExpression (word);

		//parsing failed?
		if (result.m_name.empty()) {
			cout << "Failed to parse " << word << " from " << stmt << endl;
			evaluation_succeed = false;
			break;
		}
		
		result.Print ();

		// no tokens before this, what we need to do now, is find the TagEntry
		// that corresponds to the result
		if (result.m_isaType) 
		{
			//--------------------------------------------
			// Handle type (usually when casting is found)
			//--------------------------------------------

			if (result.m_isPtr && op == ".") 
			{
				cout << "Did you mean to use '->' instead of '.' ?" << endl;
				evaluation_succeed = false;
				break;
			}
			
			if (!result.m_isPtr && op == "->") 
			{
				cout << "Can not use '->' operator on a non pointer object" << endl;
				evaluation_succeed = false;
				break;
			}
			
			out_type_scope = result.m_scope.empty() ? "<global>" : result.m_scope.c_str();
			out_type_name = result.m_name.c_str();
		} 
		else if (result.m_isThis) 
		{
			//-----------------------------------------
			// special handle for 'this' keyword
			//-----------------------------------------
			out_type_scope = result.m_scope.empty() ? "<global>" : result.m_scope.c_str();
			if (scope_name == "<global>") 
			{
				cout << "'this' can not be used in the global scope" << endl;
				evaluation_succeed = false;
				break;
			}
			
			if (op == "::") 
			{
				cout << "'this' can not be used with operator ::" << endl;
				evaluation_succeed = false;
				break;
			}

			if (result.m_isPtr && op == ".") 
			{
				cout << "Did you mean to use '->' instead of '.' ?" << endl;
				evaluation_succeed = false;
				break;
			}
			
			if (!result.m_isPtr && op == "->") 
			{
				cout << "Can not use '->' operator on a non pointer object" << endl;
				evaluation_succeed = false;
				break;
			}
			out_type_name = scope_name;
		}

 		else 
		{
			//-------------------------------------------
			// found an identifier (can be a local variable, a global one etc)
			//--------------------------------------------
			
			cout << "found an identifier or local variable..." << endl;

			/* TODO */
			// get the scope iterator
			SymbolDBEngineIterator *iter = symbol_db_engine_get_scope_chain_by_file_line (_dbe,
			    full_file_path.c_str (), linenum, SYMINFO_SIMPLE);

			// it's a global one if it's NULL or if it has just only one element
			if (iter == NULL || symbol_db_engine_iterator_get_n_items (iter) <= 1)
			{
				cout << "...we've a global scope" << endl;
			}
			else 
			{
				// DEBUG PRINT
				do 
				{
					SymbolDBEngineIteratorNode *node = 
						SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);
					cout << "got scope name: " << 
						symbol_db_engine_iterator_node_get_symbol_name (node) << endl;					
				} while (symbol_db_engine_iterator_move_next (iter) == TRUE);
			}			
						    

			/* TODO */
			/* optimize scope'll clear the scopes leaving the local variables */
			string optimized_scope = optimizeScope(above_text);

			cout << "here it is the optimized scope " << optimized_scope << endl;

			VariableList li;
			std::map<std::string, std::string> ignoreTokens;
			get_variables(optimized_scope, li, ignoreTokens, false);

			cout << "variables found are..." << endl;
			for (VariableList::iterator iter = li.begin(); iter != li.end(); iter++) {
				Variable var = (*iter);
				var.Print ();				
				
				if (word == var.m_name) {
					cout << "wh0a! we found the variable type to parse... it's ->" << 
						var.m_type << "<-" << endl;

					SymbolDBEngineIterator *iter = 
						symbol_db_engine_find_symbol_by_name_pattern_filtered (
						    _dbe, var.m_type.c_str (), TRUE, NULL, TRUE, -1, NULL, -1 , 
						    -1, SYMINFO_SIMPLE);

					if (iter != NULL) {
						SymbolDBEngineIteratorNode *node = 
							SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);
						cout << "SymbolDBEngine: Searched var got name: " << 
							symbol_db_engine_iterator_node_get_symbol_name (node) << endl;
						
						// print the scope members
						SymbolDBEngineIterator * children = 
							symbol_db_engine_get_scope_members_by_symbol_id (_dbe, 
								symbol_db_engine_iterator_node_get_symbol_id (node), 
								-1,
								-1,
								SYMINFO_SIMPLE);
						if (children != NULL)
						{
							cout << "scope children are: " << endl;
							do {
								SymbolDBEngineIteratorNode *child = 
									SYMBOL_DB_ENGINE_ITERATOR_NODE (children);
								cout << "SymbolDBEngine: Searched var got name: " << 
									symbol_db_engine_iterator_node_get_symbol_name (child) << endl;
							}while (symbol_db_engine_iterator_move_next (children) == TRUE);						
						} 
					}
					break;
				}
			}			
			
			/* TODO */
			/* get the derivation list of the typename */
		}
#if 0					
/*		
			wxString scopeToSearch(scopeName);
			if (parentTypeScope.IsEmpty() == false && parentTypeScope != wxT("<global>")) {
				scopeToSearch = parentTypeScope + wxT("::") + parentTypeName;
			} else if ((parentTypeScope.IsEmpty()|| parentTypeScope == wxT("<global>")) && !parentTypeName.IsEmpty()) {
				scopeToSearch = parentTypeName;
			}

			//--------------------------------------------------------------------------------------------
			//keep the scope that we searched so far. The accumumlated scope
			//are used for types, for scenarios like:
			//void Box::GetWidth()
			// {
			//	Rectangle::
			//
			//trying to process the above code, will yield searching Rectangle inside Box scope, since we are
			//inside Box's GetWidth() function.
			//the correct behavior shuold be searching for Rectangle in the global scope.
			//to correct this, we do special handling for Qualifier followed by coloon:colon operator (::)
			if (accumulatedScope.IsEmpty() == false) {
				if (accumulatedScope == wxT("<global>")) {
					accumulatedScope = scopeToSearch;
				} else {
					accumulatedScope << wxT("::");
					accumulatedScope << scopeToSearch;
				}
			} else {
				accumulatedScope << wxT("<global>");
			}

			wxString originalScopeName(scopeToSearch);
			if (op == wxT("::")) {
				//if the operator was something like 'Qualifier::', it is safe to assume
				//that the secope to be searched is the full expression
				scopeToSearch = accumulatedScope;
			}
*/
			// get the derivation list of the typename
			bool res(false);
			wxString _name(_U(result.m_name.c_str()));
			PERF_BLOCK("TypeFromName") {
				for (int i=0; i<2; i++) {
					res = TypeFromName(	_name, 
					                    visibleScope,
					                    lastFuncSig,
					                    scopeToSearch,
					                    additionalScopes,
					                    parentTypeName.IsEmpty(),
					                    typeName,   //output
					                    typeScope); //output

					if (!res && originalScopeName.IsEmpty() == false) {
						// the scopeToSearch was modified earlier with the accumulated scope
						// restore the search scope and try again
						scopeToSearch = originalScopeName;
						continue;
					}
					break;
				}
			}

			if (!res) {
				evaluationSucceeded = false;
				break;
			}

			// do typedef subsitute
			wxString tmp_name(typeName);
			while (OnTypedef(typeName, typeScope, templateInitList, scopeName, scopeTemplateInitList)) {
				if (tmp_name == typeName) {
					//same type? break
					break;
				}
				tmp_name = typeName;
			}

			//do template subsitute
			if (OnTemplates(typeName, typeScope, parent)) {
				//do typedef subsitute
				wxString tmp_name(typeName);
				while (OnTypedef(typeName, typeScope, templateInitList, scopeName, scopeTemplateInitList)) {
					if (tmp_name == typeName) {
						//same type? break
						break;
					}
					tmp_name = typeName;
				}
			}

			// try match any overloading operator to the typeName
			wxString origTypeName(typeName);

			// keep the typeScope in variable origTypeScope since it might be modified by
			// the OnArrowOperatorOverloading() method, but we might need it again in case
			// -> operator overloading is found
			wxString origTypeScope(typeScope);
			if ( op == wxT("->") && OnArrowOperatorOverloading(typeName, typeScope) ) {

				// there is an operator overloading for ->
				// do the whole typedef/template subsitute again
				wxString tmp_name(typeName);
				while (OnTypedef(typeName, typeScope, templateInitList, scopeName, scopeTemplateInitList)) {
					if (tmp_name == typeName) {
						//same type? break
						break;
					}
					tmp_name = typeName;
				}

				// When template is found, replace the typeName with the temporary type name
				// usually it will replace 'T' with the parent type, such as
				// 'Singleton'
				if (templateInitList.IsEmpty() == false) {
					m_parentVar.m_isTemplate = true;
					m_parentVar.m_templateDecl = _C(templateInitList);
					m_parentVar.m_type = _C(origTypeName);
					m_parentVar.m_typeScope = _C(origTypeScope); // we use the original type scope
				}

				// do template subsitute
				if (OnTemplates(typeName, typeScope, m_parentVar)) {
					//do typedef subsitute
					wxString tmp_name(typeName);
					while (OnTypedef(typeName, typeScope, templateInitList, scopeName, scopeTemplateInitList)) {
						if (tmp_name == typeName) {
							//same type? break
							break;
						}
						tmp_name = typeName;
					}
				}
			}
		}

		parent = m_parentVar;

		//Keep the information about this token for next iteration
		if (!parent.m_isTemplate && result.m_isTemplate) {

			parent.m_isTemplate = true;
			parent.m_templateDecl = result.m_templateInitList;
			parent.m_type = _C(typeName);
			parent.m_typeScope = _C(typeScope);

		} else if (templateInitList.IsEmpty() == false) {

			parent.m_isTemplate = true;
			parent.m_templateDecl = _C(templateInitList);
			parent.m_type = _C(typeName);
			parent.m_typeScope = _C(typeScope);
		}

		parentTypeName = typeName;
		parentTypeScope = typeScope;

#endif		
		word.clear ();
	}	

	// FIXME
	return NULL;
}


/// Return the visible scope until pchStopWord is encountered
string 
EngineParser::optimizeScope(const string& srcString)
{
	string wxcurrScope;
	std::vector<std::string> scope_stack;
	std::string currScope;

	int type;

	// Initialize the scanner with the string to search
	const char * scannerText =  srcString.c_str ();
	_tokenizer->setText (scannerText);
	bool changedLine = false;
	bool prepLine = false;
	int curline = 0;
	while (true) {
		type = _tokenizer->yylex();


		// Eof ?
		if (type == 0) {
			if (!currScope.empty())
				scope_stack.push_back(currScope);
			break;
		}

		// eat up all tokens until next line
		if ( prepLine && _tokenizer->lineno() == curline) {
			currScope += " ";
			currScope += _tokenizer->YYText();
			continue;
		}

		prepLine = false;

		// Get the current line number, it will help us detect preprocessor lines
		changedLine = (_tokenizer->lineno() > curline);
		if (changedLine) {
			currScope += "\n";
		}

		curline = _tokenizer->lineno();
		switch (type) {
		case (int)'(':
						currScope += "\n";
			scope_stack.push_back(currScope);
			currScope = "(\n";
			break;
		case (int)'{':
						currScope += "\n";
			scope_stack.push_back(currScope);
			currScope = "{\n";
			break;
		case (int)')':
						// Discard the current scope since it is completed
						if ( !scope_stack.empty() ) {
					currScope = scope_stack.back();
					scope_stack.pop_back();
					currScope += "()";
				} else
					currScope.clear();
			break;
		case (int)'}':
						// Discard the current scope since it is completed
						if ( !scope_stack.empty() ) {
					currScope = scope_stack.back();
					scope_stack.pop_back();
					currScope += "\n{}\n";
				} else {
					currScope.clear();
				}
			break;
		case (int)'#':
						if (changedLine) {
					// We are at the start of a new line
					// consume everything until new line is found or end of text
					currScope += " ";
					currScope += _tokenizer->YYText();
					prepLine = true;
					break;
				}
		default:
			currScope += " ";
			currScope += _tokenizer->YYText();
			break;
		}
	}

	_tokenizer->reset();

	if (scope_stack.empty())
		return srcString;

	currScope.clear();
	size_t i = 0;
	for (; i < scope_stack.size(); i++)
		currScope += scope_stack.at(i);

	// if the current scope is not empty, terminate it with ';' and return
	if ( currScope.empty() == false ) {
		currScope += ";";
		return currScope.c_str();
	}

	return srcString;
}


string
EngineParser::GetScopeName(const string &in, std::vector<string> *additionlNS)
{
	std::string lastFunc, lastFuncSig;
	std::vector<std::string> moreNS;
//	FunctionList fooList;

	const char *buf = in.c_str ();

//	TagsManager *mgr = GetTagsManager();
	//std::map<std::string, std::string> ignoreTokens = mgr->GetCtagsOptions().GetPreprocessorAsMap();

	std::map<std::string, std::string> foo_map;
	
	std::string scope_name = get_scope_name(buf, moreNS, foo_map);
	string scope = scope_name;
	if (scope.empty()) {
		scope = "<global>";
	}
	if (additionlNS) {
		for (size_t i=0; i<moreNS.size(); i++) {
			additionlNS->push_back(moreNS.at(i).c_str());
		}
	}
	return scope;
}



/************ C FUNCTIONS ************/

void
engine_parser_init (SymbolDBEngine* manager)
{
	EngineParser::getInstance ()->setSymbolManager (manager);
}
//*/

void
engine_parser_test_print_tokens (const char *str)
{
	EngineParser::getInstance ()->DEBUG_printTokens (str);
}

void 
engine_parser_parse_expression (const char*str)
{
	EngineParser::getInstance ()->testParseExpression (str);
}
/*
void
engine_parser_get_local_variables (const char *str)
{
	string res = EngineParser::getInstance ()->optimizeScope (str);

	VariableList li;
	std::map<std::string, std::string> ignoreTokens;
	
	get_variables (res, li, ignoreTokens, true);

	for (VariableList::iterator iter = li.begin(); iter != li.end(); iter++) {
		Variable var = *iter;
		var.Print();
	}

	//	printf("total time: %d\n", end-start);
	printf("matches found: %d\n", li.size());	
}
*/

SymbolDBEngineIterator *
engine_parser_process_expression (const char *stmt, const char * above_text, 
    const char * full_file_path, unsigned long linenum)
{
	string out_type_name;
	string out_type_scope;
	string out_oper;
	string out_scope_template_init_list;
	
	return EngineParser::getInstance ()->processExpression (stmt, above_text,  
	    full_file_path, linenum, out_type_name,
	    out_type_scope, out_oper, out_scope_template_init_list);

/*
	cout << "process expression result: " << endl << "out_type_name " << out_type_name <<  endl << 
	    "out_type_scope " << out_type_scope << endl << "out_oper " << out_oper << endl <<
		"out_scope_template_init_list " << out_scope_template_init_list << endl;
*/	 
}

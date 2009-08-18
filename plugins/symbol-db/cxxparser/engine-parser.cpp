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

// Singleton pattern.
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
	out_token.clear ();
	
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
				trim (out_token);
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
	trim (out_token);
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

		result.print ();

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
EngineParser::trim (string& str, string trimChars /* = "{};\r\n\t\v " */)
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

/**
 * Return NULL on global 
 */
SymbolDBEngineIterator *
EngineParser::getCurrentScopeChainByFileLine (const char* full_file_path, 
    										  int linenum)
{	
	SymbolDBEngineIterator *iter = 
		symbol_db_engine_get_scope_chain_by_file_line (_dbe,
	   		full_file_path, linenum, SYMINFO_SIMPLE);

	cout << "checking for completion scope..";
	/* it's a global one if it's NULL or if it has just only one element */
	if (iter == NULL || symbol_db_engine_iterator_get_n_items (iter) <= 1)
	{
		cout << "...we've a global completion scope" << endl;
		if (iter != NULL)
		{
			g_object_unref (iter);
		}

		iter = NULL;
	}
	else 
	{
		// DEBUG PRINT
		do 
		{
			SymbolDBEngineIteratorNode *node = 
				SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);
			cout << "DEBUG: got completion scope name: " << 
				symbol_db_engine_iterator_node_get_symbol_name (node) << endl;					
		} while (symbol_db_engine_iterator_move_next (iter) == TRUE);
	}
	
	return iter;
}

bool
EngineParser::getTypeNameAndScopeByToken (ExpressionResult &result, 
    									  string &token,
    									  string &op,
    									  const string& full_file_path, 
    									  unsigned long linenum,
    									  const string& above_text,
    									  string &out_type_name, 
    									  string &out_type_scope)
{
	if (result.m_isaType) 
	{
		cout << "*** Found a cast expression" << endl;
		/*
		 * Handle type (usually when casting is found)
		 */
		if (result.m_isPtr && op == ".") 
		{
			cout << "Did you mean to use '->' instead of '.' ?" << endl;
			return false;
		}
		
		if (!result.m_isPtr && op == "->") 
		{
			cout << "Can not use '->' operator on a non pointer object" << endl;
			return false;
		}
		
		out_type_scope = result.m_scope.empty() ? "" : result.m_scope.c_str();
		out_type_name = result.m_name.c_str();
		return true;
	} 
	else if (result.m_isThis) 
	{
		cout << "*** Found 'this'" << endl;
		
		/*
		 * special handle for 'this' keyword
		 */
		out_type_scope = result.m_scope.empty() ? "" : result.m_scope.c_str();

		if (out_type_scope.empty ()) 
		{
			cout << "'this' can not be used in the global scope" << endl;
			return false;
		}
		
		if (op == "::") 
		{
			cout << "'this' can not be used with operator ::" << endl;
			return false;
		}

		if (result.m_isPtr && op == ".") 
		{
			cout << "Did you mean to use '->' instead of '.' ?" << endl;
			return false;
		}
			
		if (!result.m_isPtr && op == "->") 
		{
			cout << "Can not use '->' operator on a non pointer object" << endl;
			return false;
		}
// FIXME
//		out_type_name = scope_name;
		return true;
	}
	else 
	{
		/*
		 * Found an identifier (can be a local variable, a global one etc)
		 */			
		cout << "*** Found an identifier or local variable..." << endl;


		/* this can be NULL if the scope is global */
//		SymbolDBEngineIterator *scope_chain_iter = 
//			getCurrentScopeChainByFileLine (full_file_path.c_str(), linenum);

		/* optimize scope'll clear the scopes leaving the local variables */
		string optimized_scope = optimizeScope(above_text);
		cout << "here it is the optimized buffer scope " << optimized_scope << endl;

		VariableList li;
		std::map<std::string, std::string> ignoreTokens;
		get_variables(optimized_scope, li, ignoreTokens, false);

		/* here the trick is to start from the end of the found variables
		 * up to the begin. This because the local variable declaration should be found
		 * just above to the statement line 
		 */
		cout << "variables found are..." << endl;
		for (VariableList::reverse_iterator iter = li.rbegin(); iter != li.rend(); iter++) {
			Variable var = (*iter);
			var.print ();
			
			if (token == var.m_name) 
			{
				cout << "wh0a! we found the variable type to parse... it's \"" << 
					var.m_type << "\"" << endl;
				out_type_name = var.m_type;
				out_type_scope = var.m_typeScope;

				return true;
			}
		}

		/* if we reach this point it's likely that we missed the right var type */
		cout << "## Wrong detection of the variable type" << endl;
	}
	return false;
}

/**
 * Find a searchable scope (or container) from a type_name and type_scope strings.
 * This function should usually be used to determine first token's scope.
 */
SymbolDBEngineIterator *
EngineParser::getCurrentSearchableScope (string &type_name, string &type_scope)
{
	// FIXME: case of more results now it's hardcoded to 1
	SymbolDBEngineIterator *curr_searchable_scope =
		symbol_db_engine_find_symbol_by_name_pattern_filtered (
		    			_dbe, type_name.c_str (), 
					    SYMTYPE_SCOPE_CONTAINER, TRUE, 
					    SYMSEARCH_FILESCOPE_IGNORE, NULL, 1, 
		    			-1, (SymExtraInfo)(SYMINFO_SIMPLE | SYMINFO_KIND));
	
	if (curr_searchable_scope != NULL)
	{
		SymbolDBEngineIteratorNode *node;

		node = SYMBOL_DB_ENGINE_ITERATOR_NODE (curr_searchable_scope);
	
		cout << "Current Searchable Scope " <<
    		symbol_db_engine_iterator_node_get_symbol_name (node) << 					
			" and id "<< symbol_db_engine_iterator_node_get_symbol_id (node) << 
			endl;

		/* is it a typedef? In that case find the parent struct */
		if (g_strcmp0 (symbol_db_engine_iterator_node_get_symbol_extra_string (node,
		    SYMINFO_KIND), "typedef") == 0)
		{
			cout << "it's a typedef... trying to find the associated struct...!" << endl;

			curr_searchable_scope = switchTypedefToStruct (SYMBOL_DB_ENGINE_ITERATOR (node));
			
			node = SYMBOL_DB_ENGINE_ITERATOR_NODE (curr_searchable_scope);
			cout << "(NEW) Current Searchable Scope " <<
				symbol_db_engine_iterator_node_get_symbol_name (node) << 					
				" and id "<< symbol_db_engine_iterator_node_get_symbol_id (node) << 
				endl;					
		}
	}
	else
	{
		cout << "Current Searchable Scope NULL" << endl;
	}

	return curr_searchable_scope;
}

/**
 * @param test Must be searched with SYMINFO_KIND 
 * @return or the same test or a new struct. In that second case test is unreffed
 * 
 */
SymbolDBEngineIterator *
EngineParser::switchTypedefToStruct (SymbolDBEngineIterator * test,
                         			 SymExtraInfo sym_info 
                                     /*= (SymExtraInfo)(SYMINFO_SIMPLE | SYMINFO_KIND)*/)
{
	SymbolDBEngineIteratorNode *node = SYMBOL_DB_ENGINE_ITERATOR_NODE (test);	
	SymbolDBEngineIterator *new_struct;
	gint symbol_id = symbol_db_engine_iterator_node_get_symbol_id (node);

	cout << "Switching typedef to struct " << endl;
	
	new_struct = symbol_db_engine_get_parent_scope_by_symbol_id (_dbe,
		    symbol_id, NULL, sym_info);

	if (new_struct != NULL)
	{
		/* kill the old one */
		g_object_unref (test);

		test = new_struct;
	}
	else 
	{
		cout << "Couldn't find a parent for typedef. We'll return the same object" << endl;
	}	

	return test;
}

SymbolDBEngineIterator *
EngineParser::switchMemberToContainer (SymbolDBEngineIterator * test)
{
	SymbolDBEngineIteratorNode *node = SYMBOL_DB_ENGINE_ITERATOR_NODE (test);	
	SymbolDBEngineIterator *new_container;
	const gchar* sym_type_name = 
		symbol_db_engine_iterator_node_get_symbol_extra_string (node, SYMINFO_TYPE_NAME);

	cout << "Switching container with type_name " << sym_type_name << endl;
	/* hopefully we'll find a new container for the type_name of test param */
	new_container = symbol_db_engine_find_symbol_by_name_pattern_filtered (_dbe,
	    sym_type_name, SYMTYPE_SCOPE_CONTAINER, TRUE, SYMSEARCH_FILESCOPE_IGNORE, 
	    NULL, -1, -1, (SymExtraInfo)(SYMINFO_SIMPLE | SYMINFO_KIND | 
	                                 SYMINFO_TYPE_NAME));
	    
	if (new_container != NULL)
	{
		g_object_unref (test);

		test = new_container;

		cout << ".. found new conainer with n items " << 
			symbol_db_engine_iterator_get_n_items (test) << endl;
	}
	else 
	{
		cout << "Couldn't find a container to substitute sym_type_name " << sym_type_name << endl;
	}	

	return test;
}

/* FIXME TODO: error processing. Find out a way to notify the caller of the occurred 
 * error. The "cout" method cannot be used
 */
SymbolDBEngineIterator *
EngineParser::processExpression(const string& stmt, 
    							const string& above_text,
    							const string& full_file_path, 
    							unsigned long linenum)
{
	ExpressionResult result;
	string current_token;
	string op;
	string type_name;
	string type_scope;

	/* first token */
	_tokenizer->setText (stmt.c_str ());

	/* get the fist one */
	nextToken (current_token, op);		

	cout << "--------" << endl << "First token \"" << current_token << "\" with op \"" << op 
		 << "\"" << endl; 
		
	/* parse the current sub-expression of a statement and fill up 
	 * ExpressionResult object
	 */
	result = parseExpression (current_token);

	/* fine. Get the type name and type scope given the above result for the first 
	 * and most important token.
	 */	
	bool process_res = getTypeNameAndScopeByToken (result, 
    									  current_token,
    									  op,
    									  full_file_path, 
    									  linenum,
    									  above_text,
    									  type_name, 
    									  type_scope);

	if (process_res == false)
	{
		cout << "Well, you haven't much luck, the first token failed and then "  <<
			"I cannot continue. " << endl;
		return NULL;
	}

	cout << "Going to search for curr_searchable_scope with type_name " << type_name <<
		" and type_scope " << type_scope << endl;

	/* at this time we're enough ready to issue a first query to our db. 
	 * We absolutely need to find the searchable object scope of the first result 
	 * type. By this one we can iterate the tree of scopes and reach a result.
	 */	
	SymbolDBEngineIterator *curr_searchable_scope =
		getCurrentSearchableScope (type_name, type_scope);

	if (curr_searchable_scope == NULL)
	{
		cout << "curr_searchable_scope failed to process, check the problem please" 
			<< endl;
		return NULL;
	}	
	
	/* fine. Have we more tokens left? */
	while (nextToken (current_token, op)) 
	{
		cout << "--------\nNext token \"" << current_token << "\" with op \"" << op 
			 << "\"" << endl;

		/* parse the current sub-expression of a statement and fill up 
	 	 * ExpressionResult object
	 	 */
		result = parseExpression (current_token);
		
		if (process_res == false)
		{
			cout << "Well, you haven't much luck on the NEXT token, the NEXT token failed and then "  <<
				"I cannot continue. " << endl;

			if (curr_searchable_scope != NULL)
				g_object_unref (curr_searchable_scope );
			return NULL;
		}
		
		/* check if the name of the result is valuable or not */
		SymbolDBEngineIteratorNode *node;
		gint search_scope_id;
		SymbolDBEngineIterator * iter;

		node = SYMBOL_DB_ENGINE_ITERATOR_NODE (curr_searchable_scope);

		search_scope_id =
				symbol_db_engine_iterator_node_get_symbol_id (node);
			
		iter = symbol_db_engine_find_symbol_in_scope (_dbe, result.m_name.c_str (), 
			    search_scope_id,
			    SYMTYPE_UNDEF,
			    TRUE,
			    -1, -1, (SymExtraInfo)(SYMINFO_SIMPLE | SYMINFO_KIND |
				    					SYMINFO_TYPE | SYMINFO_TYPE_NAME));
			
		if (iter == NULL)
		{
			cout << "Warning, the result.m_name " << result.m_name << 
				" does not belong to scope (id " << search_scope_id << ")" << endl;
			
			if (curr_searchable_scope != NULL)
				g_object_unref (curr_searchable_scope );
			
			return NULL;
		}
		else 
		{
			const gchar *sym_kind;
			cout << "Good element " << result.m_name << endl;			
			

			node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);

			sym_kind = symbol_db_engine_iterator_node_get_symbol_extra_string (node, 
		    										SymExtraInfo (SYMINFO_KIND));
			
			cout << ".. it has sym_kind " << sym_kind << endl;

			/* the same check as in the engine-core on sdb_engine_add_new_sym_type () */
			if (g_strcmp0 (sym_kind, "member") == 0 || 
	    		g_strcmp0 (sym_kind, "variable") == 0 || 
	    		g_strcmp0 (sym_kind, "field") == 0)
			{
				iter = switchMemberToContainer (iter);
			}

			node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);
			
			/* check for any typedef */
			if (g_strcmp0 (symbol_db_engine_iterator_node_get_symbol_extra_string (node, 
		    										SymExtraInfo (SYMINFO_KIND)),
	    											"typedef") == 0)
			{			
				iter = switchTypedefToStruct (iter);
			}
			
			/* remove the 'old' curr_searchable_scope and replace with 
			 * this new one
			 */			
			g_object_unref (curr_searchable_scope);			
			curr_searchable_scope = iter;
			continue;
		}
	}

	cout << "returning curr_searchable_scope" << endl;
	return curr_searchable_scope;
}

/**
 * @return The visible scope until pchStopWord is encountered
 */
string 
EngineParser::optimizeScope(const string& srcString)
{
	string wxcurrScope;
	std::vector<std::string> scope_stack;
	std::string currScope;

	int type;

	/* Initialize the scanner with the string to search */
	const char * scannerText =  srcString.c_str ();
	_tokenizer->setText (scannerText);
	bool changedLine = false;
	bool prepLine = false;
	int curline = 0;
	while (true) 
	{
		type = _tokenizer->yylex();

		/* Eof ? */
		if (type == 0) 
		{
			if (!currScope.empty())
				scope_stack.push_back(currScope);
			break;
		}

		/* eat up all tokens until next line */
		if ( prepLine && _tokenizer->lineno() == curline) 
		{
			currScope += " ";
			currScope += _tokenizer->YYText();
			continue;
		}

		prepLine = false;

		/* Get the current line number, it will help us detect preprocessor lines */
		changedLine = (_tokenizer->lineno() > curline);
		if (changedLine) 
		{
			currScope += "\n";
		}

		curline = _tokenizer->lineno();
		switch (type) 
		{
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
			/* Discard the current scope since it is completed */
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
				/* We are at the start of a new line
				 * consume everything until new line is found or end of text
				 */
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

	/* if the current scope is not empty, terminate it with ';' and return */
	if ( currScope.empty() == false ) {
		currScope += ";";
		return currScope.c_str();
	}

	return srcString;
}

/************ C FUNCTIONS ************/

void
engine_parser_init (SymbolDBEngine* manager)
{
	EngineParser::getInstance ()->setSymbolManager (manager);
}
//*/


void 
engine_parser_test_get_variables ()
{
	VariableList li;
	std::map<std::string, std::string> ignoreTokens;
	get_variables("char c, d, *e;", li, ignoreTokens, false);

	/* here the trick is to start from the end of the found variables
	 * up to the begin. This because the local variable declaration should be found
	 * just above to the statement line 
	 */
	cout << "variables found are..." << endl;
	for (VariableList::reverse_iterator iter = li.rbegin(); iter != li.rend(); iter++) {
		Variable var = (*iter);
		var.print ();
				
		cout << "wh0a! we found the variable type to parse... it's \"" << 
			var.m_type << "\" and type scope \"" << var.m_typeScope << "\"" <<
			endl;

	}
}

void
engine_parser_test_print_tokens (const char *str)
{
	EngineParser::getInstance ()->DEBUG_printTokens (str);
}

void 
engine_parser_parse_expression (const gchar*str)
{
	EngineParser::getInstance ()->testParseExpression (str);
}

SymbolDBEngineIterator *
engine_parser_process_expression (const gchar *stmt, const gchar * above_text, 
    const gchar * full_file_path, gulong linenum)
{
	SymbolDBEngine * dbe = EngineParser::getInstance ()->getSymbolManager ();

	SymbolDBEngineIterator *iter = 
		EngineParser::getInstance ()->processExpression (stmt, 
		    											above_text,  
	    												full_file_path, 
		    											linenum);

	return iter;
}

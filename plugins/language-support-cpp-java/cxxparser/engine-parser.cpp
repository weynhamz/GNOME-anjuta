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

#include <libanjuta/anjuta-debug.h>
#include <string>
#include <vector>
#include <libanjuta/interfaces/ianjuta-symbol-query.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

#include "engine-parser-priv.h"
#include "engine-parser.h"
#include "flex-lexer-klass-tab.h"
#include "expression-parser.h"
#include "scope-parser.h"
#include "variable-parser.h"
#include "function-parser.h"



using namespace std;

/* Singleton pattern. */
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
	_main_tokenizer = new CppTokenizer ();	
	_extra_tokenizer = new CppTokenizer ();	
}

EngineParser::~EngineParser ()
{
	delete _main_tokenizer;
	delete _extra_tokenizer;
}

bool 
EngineParser::nextMainToken (string &out_token, string &out_delimiter)
{
	out_token.clear ();
	
	int type(0);
	int depth(0);
	while ( (type = _main_tokenizer->yylex()) != 0 ) 
	{		
		switch (type) 
		{			
		case CLCL:
		case '.':
		case lexARROW:
			if (depth == 0) 
			{
				out_delimiter = _main_tokenizer->YYText();
				trim (out_token);
				return true;
			} else 
			{
				out_token.append (" ").append (_main_tokenizer->YYText());
			}
			break;
				
		case '<':
		case '[':
		case '(':
		case '{':
			depth++;
			out_token.append (" ").append (_main_tokenizer->YYText());
			break;
				
		case '>':
		case ']':
		case ')':
		case '}':
			depth--;
			out_token.append (" ").append (_main_tokenizer->YYText());
			break;
				
		default:
			out_token.append (" ").append (_main_tokenizer->YYText());
			break;
		}
	}
	trim (out_token);
	return false;
}

ExpressionResult 
EngineParser::parseExpression(const string &in)
{
	return parse_expression (in.c_str ());	
}

void
EngineParser::unsetSymbolManager ()
{
	if (_query_scope)
		g_object_unref (_query_scope);
	_query_scope = NULL;

	if (_query_search)
		g_object_unref (_query_search);
	_query_search = NULL;

	if (_query_search_in_scope)
		g_object_unref (_query_search_in_scope);
	_query_search_in_scope = NULL;

	if (_query_parent_scope)
		g_object_unref (_query_parent_scope);
	_query_parent_scope = NULL;
}

void 
EngineParser::setSymbolManager (IAnjutaSymbolManager *manager)
{
	static IAnjutaSymbolField query_parent_scope_fields[] =
	{
		IANJUTA_SYMBOL_FIELD_ID, IANJUTA_SYMBOL_FIELD_NAME,
		IANJUTA_SYMBOL_FIELD_KIND, IANJUTA_SYMBOL_FIELD_TYPE
	};
	static IAnjutaSymbolField query_search_fields[] =
	{
		IANJUTA_SYMBOL_FIELD_ID, IANJUTA_SYMBOL_FIELD_NAME,
		IANJUTA_SYMBOL_FIELD_KIND
	};
	static IAnjutaSymbolField query_scope_fields[] =
	{
		IANJUTA_SYMBOL_FIELD_ID, IANJUTA_SYMBOL_FIELD_NAME,
		IANJUTA_SYMBOL_FIELD_SIGNATURE
	};
	static IAnjutaSymbolField query_search_in_scope_fields[] =
	{
		IANJUTA_SYMBOL_FIELD_ID, IANJUTA_SYMBOL_FIELD_NAME,
		IANJUTA_SYMBOL_FIELD_KIND, IANJUTA_SYMBOL_FIELD_RETURNTYPE,
		IANJUTA_SYMBOL_FIELD_SIGNATURE, IANJUTA_SYMBOL_FIELD_TYPE_NAME
	};
	_query_search =
		ianjuta_symbol_manager_create_query (manager,
		                                     IANJUTA_SYMBOL_QUERY_SEARCH,
		                                     IANJUTA_SYMBOL_QUERY_DB_PROJECT, NULL);
	ianjuta_symbol_query_set_filters (_query_search,
	                                  (IAnjutaSymbolType)
	                                  (IANJUTA_SYMBOL_TYPE_SCOPE_CONTAINER |
	                                   IANJUTA_SYMBOL_TYPE_TYPEDEF), TRUE, NULL);
	ianjuta_symbol_query_set_fields (_query_search,
	                                 G_N_ELEMENTS (query_search_fields),
	                                 query_search_fields, NULL);
	_query_scope =
		ianjuta_symbol_manager_create_query (manager,
		                                     IANJUTA_SYMBOL_QUERY_SEARCH_SCOPE,
		                                     IANJUTA_SYMBOL_QUERY_DB_PROJECT, NULL);
	ianjuta_symbol_query_set_fields (_query_scope,
	                                 G_N_ELEMENTS (query_scope_fields),
	                                 query_scope_fields, NULL);
	_query_search_in_scope =
		ianjuta_symbol_manager_create_query (manager,
		                                     IANJUTA_SYMBOL_QUERY_SEARCH_IN_SCOPE,
		                                     IANJUTA_SYMBOL_QUERY_DB_PROJECT, NULL);
	ianjuta_symbol_query_set_fields (_query_search_in_scope,
	                                 G_N_ELEMENTS (query_search_in_scope_fields),
	                                 query_search_in_scope_fields, NULL);
	_query_parent_scope =
		ianjuta_symbol_manager_create_query (manager,
		                                     IANJUTA_SYMBOL_QUERY_SEARCH_PARENT_SCOPE,
		                                     IANJUTA_SYMBOL_QUERY_DB_PROJECT, NULL);
	ianjuta_symbol_query_set_fields (_query_parent_scope,
	                                 G_N_ELEMENTS (query_parent_scope_fields),
	                                 query_parent_scope_fields, NULL);
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
 * @return NULL on global 
 */
void
EngineParser::getNearestClassInCurrentScopeChainByFileLine (const char* full_file_path, 
                                                            unsigned long linenum,
                                                            string &out_type_name)
{
	IAnjutaIterable *iter;

	/* Find current scope of given file and line */
	iter = ianjuta_symbol_query_search_scope (_query_scope, full_file_path,
	                                          linenum, NULL);
	if (iter == NULL)
		return;

	/* FIXME: this method doesn't take into consideration 
	 * classes with same name on multiple namespaces 
	 */
	if (iter != NULL)
	{
		do 
		{
			IAnjutaIterable *parent_iter;
			IAnjutaSymbol *node = IANJUTA_SYMBOL (iter);
			DEBUG_PRINT ("sym_name = %s", ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_NAME, NULL));
			if (ianjuta_symbol_get_sym_type (node, NULL) == IANJUTA_SYMBOL_TYPE_CLASS)
			{
				out_type_name = ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_NAME, NULL);
				break;
			}
			parent_iter =
				ianjuta_symbol_query_search_parent_scope (_query_parent_scope,
				                                          node, NULL);

			if (parent_iter && 
			    ianjuta_symbol_get_int (IANJUTA_SYMBOL (iter), IANJUTA_SYMBOL_FIELD_ID, NULL) == 
			    ianjuta_symbol_get_int (IANJUTA_SYMBOL (parent_iter), IANJUTA_SYMBOL_FIELD_ID, NULL))
			{
				g_object_unref (parent_iter);
				break;
			}			    
			
			g_object_unref (iter);
			iter = parent_iter;
		} while (iter);
		if (iter)
			g_object_unref (iter);
	}
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
		DEBUG_PRINT ("*** Found a cast expression");
		/*
		 * Handle type (usually when casting is found)
		 */
		if (result.m_isPtr && op == ".") 
		{
			DEBUG_PRINT ("Did you mean to use '->' instead of '.' ?");
			return false;
		}
		
		if (!result.m_isPtr && op == "->") 
		{
			DEBUG_PRINT ("Can not use '->' operator on a non pointer object");
			return false;
		}

		out_type_scope = result.m_scope.empty() ? "" : result.m_scope.c_str();
		out_type_name = result.m_name.c_str();
		return true;
	} 
	else if (result.m_isThis) 
	{
		DEBUG_PRINT ("*** Found 'this'");
		
		/* special handle for 'this' keyword */
		if (op == "::") 
		{
			DEBUG_PRINT ("'this' can not be used with operator ::");
			return false;
		}

		if (result.m_isPtr && op == ".") 
		{
			DEBUG_PRINT ("Did you mean to use '->' instead of '.' ?");
			return false;
		}
			
		if (!result.m_isPtr && op == "->") 
		{
			DEBUG_PRINT ("Can not use '->' operator on a non pointer object");
			return false;
		}

		/* reaching this point we are quite sure that the easiest tests about "this"
		 * calling are passed. Go on finding for the first symbol of type class that
		 * is reachable through the scopes chain.
		 */	

		/* will we find a good class scope? */
		out_type_scope = result.m_scope.empty() ? "" : result.m_scope.c_str();
		out_type_name = ""; 

		getNearestClassInCurrentScopeChainByFileLine (full_file_path.c_str (), linenum, out_type_name);
		if (out_type_name.empty ()) 
		{
			DEBUG_PRINT ("'this' has not a type name");
			return false;
		}
		
		return true;
	}
	else if (op == "::")
	{
		out_type_name = token;
		out_type_scope = result.m_scope.empty() ? "" : result.m_scope.c_str();
		return true;
	}
	else 
	{
		/*
		 * Found an identifier (can be a local variable, a global one etc)
		 */			
		DEBUG_PRINT ("*** Found an identifier or local variable...");

		/* optimize scope'll clear the scopes leaving the local variables */
		string optimized_scope = optimizeScope(above_text);

		VariableList li;
		std::map<std::string, std::string> ignoreTokens;
		get_variables(optimized_scope, li, ignoreTokens, false);

		/* here the trick is to start from the end of the found variables
		 * up to the begin. This because the local variable declaration should be found
		 * just above to the statement line 
		 */
		for (VariableList::reverse_iterator iter = li.rbegin(); iter != li.rend(); iter++) 
		{
			Variable var = (*iter);
		
			if (token == var.m_name) 
			{
				DEBUG_PRINT ("We found the variable type to parse... it's \"%s"
					"\" with typescope \"%s\"", var.m_type.c_str(), var.m_typeScope.c_str());
				out_type_name = var.m_type;
				out_type_scope = var.m_typeScope;

				return true;
			}
		}

		IAnjutaIterable* curr_scope_iter = 
			ianjuta_symbol_query_search_scope (_query_scope,
			                                   full_file_path.c_str (),
			                                   linenum, NULL);

		if (curr_scope_iter != NULL)
		{
			IAnjutaSymbol *node = IANJUTA_SYMBOL (curr_scope_iter);

			/* try to get the signature from the symbol and test if the 
			 * variable searched is found there.
			 */
			const gchar * signature = ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_SIGNATURE, NULL);
			if (signature == NULL)
			{
				g_object_unref (curr_scope_iter);
				return false;
			}
			
			DEBUG_PRINT ("Signature is %s", signature);

			get_variables(signature, li, ignoreTokens, false);
			
			for (VariableList::reverse_iterator iter = li.rbegin(); iter != li.rend(); iter++) 
			{
				Variable var = (*iter);
			
				if (token == var.m_name) 
				{
				DEBUG_PRINT ("We found the variable type to parse from signature... it's \"%s"
					"\" with typescope \"%s\"", var.m_type.c_str (), var.m_typeScope.c_str ());
					out_type_name = var.m_type;
					out_type_scope = var.m_typeScope;

					g_object_unref (curr_scope_iter);
					return true;
				}
			}			

			g_object_unref (curr_scope_iter);
		}
		
		/* if we reach this point it's likely that we missed the right var type */
		DEBUG_PRINT ("## Wrong detection of the variable type");
	}
	return false;
}

/**
 * Find a searchable scope (or container) from a type_name and type_scope strings.
 * This function should usually be used to determine first token's scope.
 */
IAnjutaIterable *
EngineParser::getCurrentSearchableScope (string &type_name, string &type_scope)
{
	// FIXME: case of more results now it's hardcoded to 1
	IAnjutaIterable *curr_searchable_scope =
		ianjuta_symbol_query_search (_query_search, type_name.c_str(), NULL);
	
	if (curr_searchable_scope != NULL)
	{
		IAnjutaSymbol *node;

		node = IANJUTA_SYMBOL (curr_searchable_scope);

		const gchar *skind = ianjuta_symbol_get_string (node,
		    					IANJUTA_SYMBOL_FIELD_KIND, NULL);
		
		DEBUG_PRINT ("Current Searchable Scope name \"%s\" kind \"%s\" and id %d",
		             ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_NAME, NULL), skind,
		             ianjuta_symbol_get_int (node, IANJUTA_SYMBOL_FIELD_ID, NULL));

		/* is it a typedef? In that case find the parent struct */
		if (g_strcmp0 (ianjuta_symbol_get_string (node,
		    IANJUTA_SYMBOL_FIELD_KIND, NULL), "typedef") == 0)
		{
			DEBUG_PRINT ("It's a TYPEDEF... trying to find the associated struct...!");

			curr_searchable_scope = switchTypedefToStruct (IANJUTA_ITERABLE (node));
			
			node = IANJUTA_SYMBOL (curr_searchable_scope);
			DEBUG_PRINT ("(NEW) Current Searchable Scope %s and id %d",
						ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_NAME, NULL), 
						ianjuta_symbol_get_int (node, IANJUTA_SYMBOL_FIELD_ID, NULL));
		}
	}
	else
	{
		DEBUG_PRINT ("Current Searchable Scope NULL");
	}

	return curr_searchable_scope;
}

/**
 * @param test Must be searched with SYMINFO_KIND 
 * @return or the same test iterator or a new struct. In that second case the input 
 * iterator test is unreffed.
 * 
 */
IAnjutaIterable *
EngineParser::switchTypedefToStruct (IAnjutaIterable * test,
                         			 IAnjutaSymbolField sym_info 
                                     /*= (SymExtraInfo)(SYMINFO_SIMPLE | SYMINFO_KIND)*/)
{
	IAnjutaSymbol *node = IANJUTA_SYMBOL (test);	
	IAnjutaIterable *new_struct;

	DEBUG_PRINT ("Switching TYPEDEF (%d) ==> to STRUCT",
	             ianjuta_symbol_get_int (node, IANJUTA_SYMBOL_FIELD_ID, NULL));
	new_struct = ianjuta_symbol_query_search_parent_scope (_query_parent_scope,
	                                                       node, NULL);
	                                         
	if (new_struct != NULL)
	{
		/* kill the old one */
		g_object_unref (test);

		test = new_struct;
	}
	else 
	{
		DEBUG_PRINT ("Couldn't find a parent for typedef. We'll return the same object");
	}	

	return test;
}

IAnjutaIterable *
EngineParser::switchMemberToContainer (IAnjutaIterable * test)
{
	IAnjutaSymbol *node = IANJUTA_SYMBOL (test);	
	IAnjutaIterable *new_container;
	const gchar* sym_type_name =
		ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_TYPE_NAME, NULL);

	DEBUG_PRINT ("Switching container with type_name %s", sym_type_name);

	/* hopefully we'll find a new container for the type_name of test param */
	new_container = ianjuta_symbol_query_search (_query_search,
	                                             sym_type_name, NULL);
	if (new_container != NULL)
	{
		g_object_unref (test);

		test = new_container;

		DEBUG_PRINT (".. found new container with n items %d", 
		             ianjuta_iterable_get_length (test, NULL));
	}
	else 
	{
		DEBUG_PRINT ("Couldn't find a container to substitute sym_type_name %s",
		             sym_type_name);
	}	

	return test;
}

/* FIXME TODO: error processing. Find out a way to notify the caller of the occurred 
 * error. The "cout" method cannot be used
 */
IAnjutaIterable *
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

	DEBUG_PRINT ("Setting text %s to the tokenizer", stmt.c_str ());
	_main_tokenizer->setText (stmt.c_str ());

	/* get the first token */
	nextMainToken (current_token, op);		

	DEBUG_PRINT ("First main token \"%s\" with op \"%s\"", current_token.c_str (), op.c_str ());

	/* parse the current sub-expression of a statement and fill up 
	 * ExpressionResult object
	 */
	result = parseExpression (current_token);

	/* fine. Get the type name and type scope given the above result for the first 
	 * and most important token.
	 * The type_scope is for instance 'std' in this statement:
	 * (std::foo_util)klass->
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
		DEBUG_PRINT ("Initial statement processing failed. "
			"I cannot continue. ");
		return NULL;
	}
	
	DEBUG_PRINT ("Searching for curr_searchable_scope with type_name \"%s\""
				" and type_scope \"%s\"", type_name.c_str (), type_scope.c_str ());

	/* at this time we're enough ready to issue a first query to our db. 
	 * We absolutely need to find the searchable object scope of the first result 
	 * type. By this one we can iterate the tree of scopes and reach a result.
	 */	
	IAnjutaIterable *curr_searchable_scope =
		getCurrentSearchableScope (type_name, type_scope);

	if (curr_searchable_scope == NULL)
	{
		DEBUG_PRINT ("curr_searchable_scope failed to process, check the problem please");
		return NULL;
	}	
	
	/* fine. Have we more tokens left? */
	while (nextMainToken (current_token, op) == 1) 
	{
		DEBUG_PRINT("Next main token \"%s\" with op \"%s\"",current_token.c_str (), op.c_str ());

		/* parse the current sub-expression of a statement and fill up 
	 	 * ExpressionResult object
	 	 */
		result = parseExpression (current_token);
		
		if (process_res == false || curr_searchable_scope == NULL)
		{
			DEBUG_PRINT ("No luck with the NEXT token, the NEXT token failed and then "
				"I cannot continue. ");

			if (curr_searchable_scope != NULL)
				g_object_unref (curr_searchable_scope );
			return NULL;
		}
		
		/* check if the name of the result is valuable or not */
		IAnjutaSymbol *node;
		IAnjutaIterable * iter;

		node = IANJUTA_SYMBOL (curr_searchable_scope);
		
		iter = ianjuta_symbol_query_search_in_scope (_query_search_in_scope,
		                                             result.m_name.c_str (),
		                                             node, NULL);
		
		if (iter == NULL)
		{
			DEBUG_PRINT ("Warning, the result.m_name %s "
				"does not belong to scope (id %d)", result.m_name.c_str (), 
			             ianjuta_symbol_get_int (node, IANJUTA_SYMBOL_FIELD_ID, NULL));
			
			if (curr_searchable_scope != NULL)
				g_object_unref (curr_searchable_scope );
			
			return NULL;
		}
		else 
		{
			gchar *sym_kind;
			DEBUG_PRINT ("Good element %s", result.m_name.c_str ());
			
			node = IANJUTA_SYMBOL (iter);
			sym_kind = (gchar*)ianjuta_symbol_get_string (node, 
		    										IANJUTA_SYMBOL_FIELD_KIND, NULL);
			
			DEBUG_PRINT (".. it has sym_kind \"%s\"", sym_kind);

			/* the same check as in the engine-core on sdb_engine_add_new_sym_type () */
			if (g_strcmp0 (sym_kind, "member") == 0 || 
	    		g_strcmp0 (sym_kind, "variable") == 0 || 
	    		g_strcmp0 (sym_kind, "field") == 0)
			{
				iter = switchMemberToContainer (iter);
				node = IANJUTA_SYMBOL (iter);
				sym_kind = (gchar*)ianjuta_symbol_get_string (node, 
		    										IANJUTA_SYMBOL_FIELD_KIND, NULL);				
			}
			
			/* check for any typedef */
			if (g_strcmp0 (ianjuta_symbol_get_string (node, 
		    										IANJUTA_SYMBOL_FIELD_KIND, NULL),
	    											"typedef") == 0)
			{			
				iter = switchTypedefToStruct (iter);
				node = IANJUTA_SYMBOL (iter);
				sym_kind = (gchar*)ianjuta_symbol_get_string (node, 
		    										IANJUTA_SYMBOL_FIELD_KIND, NULL);				
			}
			
			/* is it a function or a method? */
			if (g_strcmp0 (sym_kind, "function") == 0 ||
			    g_strcmp0 (sym_kind, "method") == 0 ||
			    g_strcmp0 (sym_kind, "prototype") == 0)
			{

				string func_ret_type_name = 
					ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_RETURNTYPE, NULL);

				string func_signature = 
					ianjuta_symbol_get_string (node, IANJUTA_SYMBOL_FIELD_SIGNATURE, NULL);
				
				func_ret_type_name += " " + result.m_name + func_signature + "{}";

				FunctionList li;
				std::map<std::string, std::string> ignoreTokens;
				get_functions (func_ret_type_name, li, ignoreTokens);

				DEBUG_PRINT ("Functions found are...");
/*				
				for (FunctionList::reverse_iterator func_iter = li.rbegin(); 
				     func_iter != li.rend(); 
				     func_iter++) 
				{
					Function var = (*func_iter);
					var.print ();			
				}
*/
			
				g_object_unref (iter);

				DEBUG_PRINT ("Going to look for the following function ret type %s",
							func_ret_type_name.c_str ());

				iter = getCurrentSearchableScope (li.front().m_returnValue.m_type,
				                                  type_scope);
			}			               
			
			/* remove the 'old' curr_searchable_scope and replace with 
			 * this new one
			 */			
			g_object_unref (curr_searchable_scope);			
			curr_searchable_scope = iter;
			continue;
		}
	}

	DEBUG_PRINT ("END of expression processing. Returning curr_searchable_scope");
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
	_extra_tokenizer->setText (scannerText);
	bool changedLine = false;
	bool prepLine = false;
	int curline = 0;
	while (true) 
	{
		type = _extra_tokenizer->yylex();

		/* Eof ? */
		if (type == 0) 
		{
			if (!currScope.empty())
				scope_stack.push_back(currScope);
			break;
		}

		/* eat up all tokens until next line */
		if ( prepLine && _extra_tokenizer->lineno() == curline) 
		{
			currScope += " ";
			currScope += _extra_tokenizer->YYText();
			continue;
		}

		prepLine = false;

		/* Get the current line number, it will help us detect preprocessor lines */
		changedLine = (_extra_tokenizer->lineno() > curline);
		if (changedLine) 
		{
			currScope += "\n";
		}

		curline = _extra_tokenizer->lineno();
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
				currScope += _extra_tokenizer->YYText();
				prepLine = true;
				break;
			}
		default:
			currScope += " ";
			currScope += _extra_tokenizer->YYText();
			break;
		}
	}

	_extra_tokenizer->reset();

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
engine_parser_init (IAnjutaSymbolManager * manager)
{
	EngineParser::getInstance ()->setSymbolManager (manager);
}

void
engine_parser_deinit ()
{
	EngineParser::getInstance ()->unsetSymbolManager ();
}

IAnjutaIterable *
engine_parser_process_expression (const gchar *stmt, const gchar * above_text,
    const gchar * full_file_path, gulong linenum)
{
	try
	{
		IAnjutaIterable *iter = 
			EngineParser::getInstance ()->processExpression (stmt, 
			                                                 above_text,  
			                                                 full_file_path, 
			                                                 linenum);
		return iter;
	}
	catch (const std::exception& error)
	{
		g_critical ("cxxparser error: %s", error.what());
		return NULL;
	}
}

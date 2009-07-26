/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Eran Ifrah (Main file for CodeLite www.codelite.org/ )
 * Copyright (C) Massimo Cora' 2009 <maxcvs@email.it> (Customizations for Anjuta)
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

#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include "string"
#include "list"
#include <stdio.h>

class Variable
{
public:
	std::string     m_name;
	bool            m_isTemplate;
	std::string     m_templateDecl;
	bool            m_isPtr;
	std::string     m_type;		/* as in 'int a;' -> type=int */
	
	std::string     m_typeScope;/* as in 'std::string a;' -> typeScope = std, 
	 							 * type=string 
	 							 */
	std::string     m_pattern;
	std::string     m_starAmp;
	int             m_lineno;
	bool            m_isConst;
	std::string     m_defaultValue;	/* used mainly for function arguments with 
	 							     * default values foo (int = 0);
									 */
	std::string     m_arrayBrackets;

public:
	Variable();
	virtual ~Variable();

	/* copy ctor */
	Variable(const Variable& src);

	/* operator = */
	Variable& operator=(const Variable& src);

	/* clear the class content */
	void reset();

	/* print the variable to stdout */
	void print();
};

typedef std::list<Variable> VariableList;

#endif // _VARIABLE_H_



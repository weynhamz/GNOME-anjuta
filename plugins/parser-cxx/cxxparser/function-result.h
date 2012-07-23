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

#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "string"
#include "list"
#include "variable-result.h"
#include <stdio.h>

class Function
{
public:
	std::string     m_name;
	std::string     m_scope;					//functions' scope
	std::string     m_retrunValusConst;			// is the return value a const?
	std::string     m_signature;
	Variable        m_returnValue;
	int             m_lineno;
	bool            m_isVirtual;
	bool            m_isPureVirtual;
	bool            m_isConst;

public:
	Function();
	virtual ~Function();

	//clear the class content
	void reset();

	//print the variable to stdout
	void print();
};

typedef std::list<Function> FunctionList;
#endif // _FUNCTION_H_

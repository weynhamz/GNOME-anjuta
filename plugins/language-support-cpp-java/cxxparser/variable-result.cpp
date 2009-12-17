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

#include "variable-result.h"

Variable::Variable()
{
	reset();
}

Variable::~Variable()
{
}

Variable::Variable(const Variable &src)
{
	*this = src;
}

Variable & Variable::operator =(const Variable &src)
{
	m_type = src.m_type;
	m_templateDecl = src.m_templateDecl;
	m_name = src.m_name;
	m_isTemplate = src.m_isTemplate;
	m_isPtr = src.m_isPtr;
	m_typeScope = src.m_typeScope;
	m_pattern = src.m_pattern;
	m_starAmp = src.m_starAmp;
	m_lineno = src.m_lineno;
	m_isConst = src.m_isConst;
	m_defaultValue = src.m_defaultValue;
	m_arrayBrackets = src.m_arrayBrackets;
	return *this;
}

void Variable::reset()
{
	m_type = "";
	m_templateDecl = "";
	m_name = "";
	m_isTemplate = false;
	m_isPtr = false;
	m_typeScope = "";
	m_pattern = "";
	m_starAmp = "";
	m_lineno = 0;
	m_isConst = false;
	m_defaultValue = "";
	m_arrayBrackets = "";
}

void Variable::print()
{
	fprintf(    stdout, "{m_name=%s, m_defaultValue=%s, m_lineno=%d, m_starAmp=%s, "
	    "m_type=%s, m_isConst=%s, m_typeScope=%s, m_templateDecl=%s, m_arrayBrackets=%s, "
	    "m_isPtr=%s, m_isTemplate=%s }\n",		
				m_name.c_str(),
				m_defaultValue.c_str(),
				m_lineno,
				m_starAmp.c_str(),
				m_type.c_str(),
				m_isConst ? "true" : "false",
				m_typeScope.c_str(),
				m_templateDecl.c_str(),
				m_arrayBrackets.c_str(),
				m_isPtr ? "true" : "false",
				m_isTemplate ? "true" : "false");

	fprintf( stdout, "Pattern: %s\n", m_pattern.c_str());
	fflush(stdout);
}

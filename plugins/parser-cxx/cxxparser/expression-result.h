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

#ifndef EXPRESSION_RESULT_H
#define EXPRESSION_RESULT_H

#include <string>
#include <stdio.h>

class ExpressionResult
{
public:

	bool        m_isFunc;
	std::string m_name;
	bool        m_isThis;
	bool        m_isaType;
	bool        m_isPtr;
	std::string m_scope;
	bool        m_isTemplate;
	std::string m_templateInitList;

public:
	ExpressionResult();
	virtual ~ExpressionResult();
	void reset();
	void print();
	std::string toString() const;
};
#endif //EXPRESSION_RESULT_H

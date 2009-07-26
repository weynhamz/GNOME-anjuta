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

#include "expression-result.h"

#define BOOL_TO_STR(b) b ? "true" : "false"

ExpressionResult::ExpressionResult()
{
	reset();
}

ExpressionResult::~ExpressionResult()
{
}

void ExpressionResult::print()
{
	printf("%s\n", toString().c_str());
}

std::string ExpressionResult::toString() const
{
	char tmp[256];
	sprintf(tmp, "{m_name:%s, m_isFunc:%s, m_isTemplate:%s, m_isThis:%s, m_isaType:%s, m_isPtr:%s, m_scope:%s, m_templateInitList:%s}", 
				m_name.c_str(), 
				BOOL_TO_STR(m_isFunc), 
				BOOL_TO_STR(m_isTemplate),
				BOOL_TO_STR(m_isThis),
				BOOL_TO_STR(m_isaType),
				BOOL_TO_STR(m_isPtr),
				m_scope.c_str(),
				m_templateInitList.c_str());
	return tmp;
}

void ExpressionResult::reset()
{
	m_isFunc = false;
	m_name = "";
	m_isThis = false;
	m_isaType = false;
	m_isPtr = false;
	m_scope = "";
	m_isTemplate = false;
	m_templateInitList = "";
}

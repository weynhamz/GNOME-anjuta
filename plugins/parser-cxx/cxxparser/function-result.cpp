/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Eran Ifrah (Main file for CodeLite www.codelite.org/ )
 * Copyright (C) Massimo Cora' 2009-2010 <maxcvs@email.it> (Customizations for Anjuta)
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

#include "function-result.h"

Function::Function()
{
	reset();
}

Function::~Function()
{
}

void Function::reset()
{
	m_name = "";
	m_scope = "";
	m_returnValue.reset();
	m_name = "";
	m_signature = "";
	m_lineno = 0;
	m_retrunValusConst = "";
	m_isVirtual = false;
	m_isPureVirtual = false;
	m_isConst = false;
}

void Function::print()
{
	fprintf(	
				stdout, "{m_name=%s, m_isConst=%s, m_lineno=%d, m_scope=%s, m_signature=%s, m_isVirtual=%s, m_isPureVirtual=%s, m_retrunValusConst=%s\nm_returnValue=", 
				m_name.c_str(), 
				m_isConst ? "yes" : "no",
				m_lineno, 
				m_scope.c_str(), 
				m_signature.c_str(), 
				m_isVirtual ? "yes" : "no", 
				m_isPureVirtual ? "yes" : "no", 
				m_retrunValusConst.c_str()
			);
				
	m_returnValue.print();
	fprintf(stdout, "}\n");
	fflush(stdout);
}


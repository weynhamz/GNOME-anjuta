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

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah
// file name            : cpp_scanner.cpp
//
// -------------------------------------------------------------------------
// A
//              _____           _      _     _ _
//             /  __ \         | |    | |   (_) |
//             | /  \/ ___   __| | ___| |    _| |_ ___
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/
//              \____/\___/ \__,_|\___\_____/_|\__\___|
//
//                                                  F i l e
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include "cpp-flex-tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


CppTokenizer::CppTokenizer() : m_curr(0)
{
	m_data = NULL;
	m_pcurr = NULL;
	m_keepComments = 0;
	m_returnWhite = 0;
	m_comment = "";
}

CppTokenizer::~CppTokenizer(void)
{
	delete m_data;
}

int 
CppTokenizer::LexerInput(char *buf, int max_size)
{
	if( !m_data )
		return 0;

	memset(buf, 0, max_size);
	char *pendData = m_data + strlen(m_data);
	int n = (max_size < (pendData - m_pcurr)) ? max_size : (pendData - m_pcurr);
	if(n > 0)
	{
		memcpy(buf, m_pcurr, n);
		m_pcurr += n;
	}
	return n;
}

void 
CppTokenizer::setText(const char* data)
{
	// release previous buffer
	reset();

	m_data = new char[strlen(data)+1];
	strcpy(m_data, data);
	m_pcurr = m_data;
}

void 
CppTokenizer::reset()
{
	if(m_data)
	{
		delete [] m_data;
		m_data = NULL;
		m_pcurr = NULL;
		m_curr = 0;
	}

	// Notify lex to restart its buffer
	yy_flush_buffer(yy_current_buffer);
	m_comment = "";
	yylineno = 1;
}

const int& 
CppTokenizer::lineNo() const
{
	return yylineno;
}

void 
CppTokenizer::clearComment() 
{ 
	m_comment = ""; 
}

const char* 
CppTokenizer::getComment () const 
{
	return m_comment.c_str ();
}

void 
CppTokenizer::keepComment (const int& keep) 
{ 
	m_keepComments = keep; 
}

void 
CppTokenizer::returnWhite (const int& rw) 
{ 
	m_returnWhite = rw; 
}

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
// file name            : cpp_scanner.h              
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
#ifndef _CPPTOKENIZER_H_
#define _CPPTOKENIZER_H_

#include "flex-lexer-klass.h"

class CppTokenizer : public flex::yyFlexLexer
{
public:
	CppTokenizer();
	~CppTokenizer(void);
	
	/* Override the LexerInput function */
	int LexerInput(char *buf, int max_size);
	void setText(const char* data);
	void reset();

	 
	/*	Note about comment and line number:
	 *	If the last text consumed is a comment, the line number 
	 *	returned is the line number of the last line of the comment
	 *	incase the comment spans over number of lines
	 */
	const int& lineNo() const; 
	inline void clearComment();
	inline const char* getComment() const;
	inline void keepComment(const int& keep);
	inline void returnWhite(const int& rw);

private:
	char *m_data;
	char *m_pcurr;
	int   m_total;
	int   m_curr;
};

#endif // _CPPTOKENIZER_H_

/*
    lexer.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _LEXER_H_
#define _LEXER_H_

#define TE_LEXER_BASE		0
#define TE_LEXER_AUTOMATIC	TE_LEXER_BASE
#define TE_LEXER_NONE		TE_LEXER_BASE+1
#define TE_LEXER_CPP		TE_LEXER_BASE+2
#define TE_LEXER_GCPP		TE_LEXER_BASE+3
#define TE_LEXER_HTML		TE_LEXER_BASE+4
#define TE_LEXER_XML		TE_LEXER_BASE+5
#define TE_LEXER_JS 		TE_LEXER_BASE+6
#define TE_LEXER_WSCRIPT	TE_LEXER_BASE+7
#define TE_LEXER_MAKE		TE_LEXER_BASE+8
#define TE_LEXER_JAVA		TE_LEXER_BASE+9
#define TE_LEXER_LUA		TE_LEXER_BASE+10
#define TE_LEXER_PYTHON		TE_LEXER_BASE+11
#define TE_LEXER_PERL		TE_LEXER_BASE+12
#define TE_LEXER_SQL		TE_LEXER_BASE+13
#define TE_LEXER_PLSQL		TE_LEXER_BASE+14
#define TE_LEXER_PHP		TE_LEXER_BASE+15
#define TE_LEXER_LATEX		TE_LEXER_BASE+16
#define TE_LEXER_DIFF		TE_LEXER_BASE+17
#define TE_LEXER_PASCAL		TE_LEXER_BASE+18
#define TE_LEXER_XCODE		TE_LEXER_BASE+19
#define TE_LEXER_PROPS		TE_LEXER_BASE+20
#define TE_LEXER_CONF		TE_LEXER_BASE+21
#define TE_LEXER_BAAN		TE_LEXER_BASE+22
#define TE_LEXER_ADA		TE_LEXER_BASE+23
#define TE_LEXER_LISP		TE_LEXER_BASE+24
#define TE_LEXER_RUBY		TE_LEXER_BASE+25
#define TE_LEXER_MATLAB		TE_LEXER_BASE+26
#define TE_LEXER_CSS 		TE_LEXER_BASE+27

#endif /* _LEXER_H_ */

/*  (c) 2004 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _XMLSTRINGS
#define _XMLSTRINGS

#include <libxml/parser.h>

/* Keep this up-to-date with the dtds 
and with xmlstrings.c*/
extern const xmlChar* TEXT_TYPE;
extern const xmlChar* AUTOTOOLS_TYPE;
extern const xmlChar* LANG_TYPE;
extern const xmlChar* LIB_TYPE;
extern const xmlChar* PRJ_TYPE;

/* General */
extern const xmlChar* NAME;
extern const xmlChar* LANGUAGE;
extern const xmlChar* DESCRIPTION;

/* Text */
extern const xmlChar* CONTENT;

/* Project */
extern const xmlChar* TARGET;
extern const xmlChar* LIBS;
extern const xmlChar* LIBRARY;
extern const xmlChar* INIT_SCRIPT;
extern const xmlChar* INIT_TARBALL;

/* autotools */
extern const xmlChar* AUTOCONF;
extern const xmlChar* AUTOMAKE;
extern const xmlChar* MACRO;
extern const xmlChar* INCLUDE;
extern const xmlChar* LD_ADD;
extern const xmlChar* PKGCONFIG;
extern const xmlChar* PACKAGE;
extern const xmlChar* PKGVERSION;

/* library */
extern const xmlChar* VERSION;
#endif

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
 
#include "xmlstrings.h"

const xmlChar* TEXT_TYPE = "text-template";
const xmlChar* AUTOTOOLS_TYPE = "autotools";
const xmlChar* LANG_TYPE = "language-template";
const xmlChar* LIB_TYPE = "library-template";
const xmlChar* PRJ_TYPE = "project-template";

/* General */
const xmlChar* NAME = "name";
const xmlChar* LANGUAGE = "language";
const xmlChar* DESCRIPTION = "description";

/* Text */
const xmlChar* CONTENT = "content";

/* Project */
const xmlChar* TARGET = "target";
const xmlChar* LIBS = "libs";
const xmlChar* LIBRARY = "library";
const xmlChar* INIT_SCRIPT = "init_script";
const xmlChar* INIT_TARBALL = "init_tarball";

/* autotools */
const xmlChar* AUTOCONF = "autoconf";
const xmlChar* AUTOMAKE = "automake";
const xmlChar* MACRO = "macro";
const xmlChar* INCLUDE = "include";
const xmlChar* LD_ADD = "ld_add";
const xmlChar* LD_PATH = "ld_path";
const xmlChar* PKGCONFIG = "pkgconfig";
const xmlChar* PACKAGE = "package";
const xmlChar* PKGVERSION = "pkgversion";


/* library */
const xmlChar* VERSION = "version";

/*
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
 
#ifndef _AUTOTOOLS_H
#define _AUTOTOOLS_H

#include <libxml/parser.h>
#include <glib.h>

typedef struct _Autotools Autotools;
typedef struct _Pkgconfig Pkgconfig;

struct _Pkgconfig
{
	gchar* package;
	gchar* version;
};

struct _Autotools
{
	GList* autoconf_macros;
	GList* pkgconfig;
	
	GList* automake_includes;
	GList* automake_ld_add;
};

Autotools* parse_autotools(xmlDocPtr doc, xmlNodePtr cur_node);
void autotools_free(Autotools* autotools);

#endif /* _AUTOTOOLS_H */

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
 
#ifndef _LANG_TEMPLATE_H
#define _LANG_TEMPLATE_H

#include <glib.h>
#include <libxml/parser.h>

#include "autotools.h"

typedef struct _LangTemplate LangTemplate;

struct _LangTemplate
{
	gchar* name;
	gchar* description;
		
	Autotools* autotools;
};

LangTemplate* lang_template_parse(xmlDocPtr doc);
void lang_template_free(LangTemplate* lang);
#endif /* _LANG_TEMPLATE_H */

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
 
#ifndef _TEXT_TEMPLATE_H
#define _TEXT_TEMPLATE_H

#include <libxml/parser.h>
#include <glib.h>

typedef struct _TextTemplate TextTemplate;

struct _TextTemplate
{
	gchar* name;
	gchar* description;
	
	gchar* lang;
	
	gchar* text;
};

TextTemplate* text_template_parse(xmlDocPtr doc);
void text_template_free(TextTemplate* temp);

#endif /* _TEXT_TEMPLATE_H */

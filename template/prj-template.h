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
 
#ifndef _PRJ_TEMPLATE_H
#define _PRJ_TEMPLATE_H

#include <glib.h>
#include <libxml/parser.h>

#include "autotools.h"

typedef struct _PrjTemplate PrjTemplate;

typedef enum 
{
	TARGET_EXEC,
	TARGET_STATIC_LIB,
	TARGET_SHARED_LIB,
	TARGET_MISC,
	TARGET_END
} PrjTarget;


struct _PrjTemplate
{
	gchar* name;
	gchar* description;
	gchar* lang;
	
	PrjTarget target;
	
	Autotools* autotools;
	GList* libs;
	
	gchar* init_tarball;
	gchar* init_script;
};

PrjTemplate* prj_template_parse(xmlDocPtr doc);
void prj_template_free(PrjTemplate* prj);

#endif /* _PRJ_TEMPLATE_H */

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
 
#include <libxml/parser.h>
#include <glib.h>

#include "autotools.h"
#include "xmlstrings.h"
#include "lib-template.h"

/* Parse a libtemplate from a xmlfile. If parsing fails NULL is returned */

LibTemplate* lib_template_parse(xmlDocPtr doc)
{
	LibTemplate* lib = g_new0(LibTemplate, 1);
	xmlNodePtr cur_node;
	
	/* Check arguments */
	g_return_val_if_fail (doc != NULL, NULL);
	
	/* Check if this document descripes a language */
	cur_node = xmlDocGetRootElement(doc);
	if (cur_node == NULL)
	{
		xmlFreeDoc(doc);
		g_free(lib);
		return NULL;
	}
	if (xmlStrcmp(cur_node->name, LIB_TYPE) != 0)
	{
		xmlFreeDoc(doc);
		g_free(lib);
		return NULL;
	}
	
	/* Read template into the structure */
	cur_node = cur_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		const xmlChar* keyname = cur_node->name;
		xmlChar* key = xmlNodeListGetString(doc, 
			cur_node->xmlChildrenNode, 1);
		if (xmlStrcmp(keyname, NAME) == 0)
		{
			lib->name = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, DESCRIPTION) == 0)
		{
			lib->description = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, LANGUAGE) == 0)
		{
			lib->lang = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, VERSION) == 0)
		{
			lib->version = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, AUTOTOOLS_TYPE) == 0)
		{
			lib->autotools = parse_autotools(doc, cur_node);
		}
		else
		{
			/* Ignore unknown elements */
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return lib;
}

/* Free a library template and all its members */

void lib_template_free(LibTemplate* lib)
{
	if (lib == NULL)
		return;
	
	g_free(lib->name);
	g_free(lib->description);
	g_free(lib->lang);
	autotools_free(lib->autotools);
	
	g_free(lib);
}

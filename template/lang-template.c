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

#include "xmlstrings.h"
#include "autotools.h"
#include "lang-template.h"

/* Parse a language template into the structure and return it 
if possible. Otherwise return NULL. */ 

LangTemplate* lang_template_parse(xmlDocPtr doc)
{
	LangTemplate* lang = g_new0(LangTemplate, 1);
	xmlNodePtr cur_node;
	
	/* Check arguments */
	g_return_val_if_fail (doc != NULL, NULL);
	
	/* Check if this document descripes a language */
	cur_node = xmlDocGetRootElement(doc);
	if (cur_node == NULL)
	{
		xmlFreeDoc(doc);
		g_free(lang);
		return NULL;
	}
	if (xmlStrcmp(cur_node->name, LANG_TYPE) != 0)
	{
		xmlFreeDoc(doc);
		g_free(lang);
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
			lang->name = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, DESCRIPTION) == 0)
		{
			lang->description = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, AUTOTOOLS_TYPE) == 0)
		{
			lang->autotools = parse_autotools(doc, cur_node);
		}
		else
		{
			/* Ignore unknown elements */
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return lang;
}

/* Free a language template and all its elements */
void lang_template_free(LangTemplate* lang)
{
	if (lang == NULL)
		return;
	
	g_free(lang->name);
	g_free(lang->description);
	autotools_free(lang->autotools);
	
	g_free(lang);
}

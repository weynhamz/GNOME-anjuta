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

#include "text-template.h"
#include "xmlstrings.h"

/* Parse a TextTemplate from the xml file given by xmlDocPtr
and return it. The returned value must be g_freed. If the function
fails NULL is returned */

TextTemplate* text_template_parse(xmlDocPtr doc)
{
	xmlNodePtr cur_node;
	TextTemplate* text = g_new0(TextTemplate, 1);
	
	/* Check if arguments are valid */
	g_return_val_if_fail (doc != NULL, NULL);
	
	/* Check if the document contains data */
	cur_node = xmlDocGetRootElement(doc);
	if (cur_node == NULL)
	{
		xmlFreeDoc(doc);
		g_free(text);
		return NULL;
	}
	
	/* Check if the document descripes a text template */
	if (xmlStrcmp(cur_node->name, TEXT_TYPE) != 0)
	{
		xmlFreeDoc(doc);
		g_free(text);
		return NULL;
	}
	
	/* Read the values into the structure and warn on
	invalid elements. Members not included in the 
	document will stay NULL. */
	cur_node = cur_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		const xmlChar* keyname = cur_node->name;
		xmlChar* key = xmlNodeListGetString(doc, 
			cur_node->xmlChildrenNode, 1);
		if (xmlStrcmp(keyname, NAME) == 0)
		{
			text->name = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, DESCRIPTION) == 0)
		{
			text->description = g_strdup(key);		
		}
		else if (xmlStrcmp(keyname, LANGUAGE) == 0)
		{
			text->lang = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, CONTENT) == 0)
		{
			text->text = g_strdup(key);
		}
		else
		{
			/* No matching Element, ignore */
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return text;
}

/* Free a TextTemplate and all its elements */

void text_template_free(TextTemplate* temp)
{
	if (temp == NULL)
		return;
	
	g_free(temp->name);
	g_free(temp->description);
	g_free(temp->lang);
	g_free(temp->text);
	
	g_free(temp);
}

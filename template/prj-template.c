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
#include "prj-template.h"

const gchar* targets[TARGET_END] = {
	"exec",
	"static lib",
	"shared lib",
	"misc"
};

static PrjTarget target_from_string(const gchar* starget);

/* Parse a project template into the structure and return it 
if possible. Otherwise return NULL. */ 

PrjTemplate* prj_template_parse(xmlDocPtr doc)
{
	PrjTemplate* prj = g_new0(PrjTemplate, 1);
	xmlNodePtr cur_node;
	
	/* Check arguments */
	g_return_val_if_fail (doc != NULL, NULL);
	
	/* Check if this document descripes a language */
	cur_node = xmlDocGetRootElement(doc);
	if (cur_node == NULL)
	{
		xmlFreeDoc(doc);
		g_free(prj);
		return NULL;
	}
	if (xmlStrcmp(cur_node->name, PRJ_TYPE) != 0)
	{
		xmlFreeDoc(doc);
		g_free(prj);
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
			prj->name = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, DESCRIPTION) == 0)
		{
			prj->description = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, LANGUAGE) == 0)
		{
			prj->lang = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, TARGET) == 0)
		{
			prj->target = target_from_string(key);	
		}
		else if (xmlStrcmp(keyname, LIBS) == 0)
		{
			xmlNodePtr node = cur_node->xmlChildrenNode;
			while (node != NULL)
			{
				const xmlChar* subkeyname = node->name;
				xmlChar* subkey = xmlNodeListGetString(doc, 
					node->xmlChildrenNode, 1);
				if (xmlStrcmp(subkeyname, LIBRARY))
				{
					prj->libs = g_list_append(prj->libs, 
						g_strdup(subkey));
				}
				xmlFree(subkey);
				node = node->next;
			}				
		}
		else if (xmlStrcmp(keyname, AUTOTOOLS_TYPE) == 0)
		{
			prj->autotools = parse_autotools(doc, cur_node);
		}
		else if (xmlStrcmp(keyname, INIT_SCRIPT) == 0)
		{
			prj->init_script = g_strdup(key);	
		}
		else if (xmlStrcmp(keyname, INIT_TARBALL) == 0)
		{
			prj->init_tarball = g_strdup(key);	
		}
		else
		{
			/* Ignore unknown elements */
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return prj;
}

/* Free a project template and all its members */

void prj_template_free(PrjTemplate* prj)
{
	if (prj == NULL)
		return;
	
	g_free(prj->name);
	g_free(prj->description);
	g_free(prj->lang);
	g_free(prj->init_script);
	g_free(prj->init_tarball);
	g_list_free(prj->libs);
	autotools_free(prj->autotools);
	
	g_free(prj);
}

/* Get the correct PrjTarget from a string in the xmlfile */

PrjTarget target_from_string(const gchar* starget)
{
	int cur_target;
	for (cur_target = 0; cur_target < TARGET_END; cur_target++)
	{
		if (strcmp(starget, targets[cur_target]) == 0)
		{
			return (PrjTarget) cur_target;
		}
	}
	g_warning("Invalid target type %s!", starget);
	return TARGET_END;	
}

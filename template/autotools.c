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

static gboolean parse_autoconf(xmlDocPtr doc, xmlNodePtr node, 
						Autotools* autotools);
static gboolean parse_automake(xmlDocPtr doc, xmlNodePtr node, 
						Autotools* autotools);
						
/* Parse the autotools part of an xml template. Project, language and
lib templates use this to store information about autoconf/automake.
Return a a pointer to an autotools struct or NULL in case the autotools
part could not be read correctly. */

Autotools* 
parse_autotools(xmlDocPtr doc, xmlNodePtr node)
{
	xmlNodePtr cur_node;
	Autotools* autotools = g_new0(Autotools, 1);
	
	/* Check arguments */
	g_return_val_if_fail(doc != NULL, NULL);
	g_return_val_if_fail(cur_node != NULL, NULL);	
	
	/* Check if this is an autotools section */
	if (xmlStrcmp(node->name, AUTOTOOLS_TYPE) != 0)
	{
		g_free(autotools);
		return NULL;
	}
	
	cur_node = node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		const xmlChar* keyname = cur_node->name;
		xmlChar* key = xmlNodeListGetString(doc, 
			cur_node->xmlChildrenNode, 1);
		
		/* Read autoconf section into structure */
		if (xmlStrcmp(keyname, AUTOCONF) == 0)
		{
			parse_autoconf(doc, cur_node, autotools);		
		}
		
		/* Read automake section into struture */	
		else if (xmlStrcmp(keyname, AUTOMAKE) == 0)
		{
			parse_automake(doc, cur_node, autotools);		
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return autotools;	
}

/* Read the autoconf section into autotools. If parsing fails
FALSE is returned and autotools might be incomplete */

gboolean parse_autoconf(xmlDocPtr doc, xmlNodePtr node, 
						Autotools* autotools)
{
	xmlNodePtr cur_node;
	
	/* Check arguments */
	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (autotools != NULL, FALSE);
	
	/* Read all elements into structure */
	cur_node = node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		const xmlChar* keyname = cur_node->name;
		xmlChar* key = xmlNodeListGetString(doc, 
			cur_node->xmlChildrenNode, 1);
		if (xmlStrcmp(keyname, MACRO) == 0)
		{
			autotools->autoconf_macros = g_list_append(
				autotools->autoconf_macros, g_strdup(key));
		}
		if (xmlStrcmp(keyname, PKGCONFIG) == 0)
		{
			Pkgconfig* pkg = g_new0(Pkgconfig, 1);
			xmlNodePtr node = cur_node->xmlChildrenNode;
			while (node != NULL)
			{
				const xmlChar* sub_keyname = node->name;
				xmlChar* sub_key = xmlNodeListGetString(doc,
					node->xmlChildrenNode, 1);
				if (xmlStrcmp(sub_keyname, PACKAGE) == 0)
				{
					pkg->package = g_strdup(sub_key);
				}
				else if (xmlStrcmp(sub_keyname, PKGVERSION) == 0)
				{
					pkg->version = g_strdup(sub_key);
				}
				else
				{
					/* Ignore other elements */
				}
				node = node->next;
			}
			autotools->pkgconfig = g_list_append(autotools->pkgconfig,
				pkg);
		}
		else
		{
			/* Ignore unknown elements */
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return TRUE;
}

/* Read the automake section into autotools. If parsing fails
FALSE is returned and autotools might be incomplete */

gboolean parse_automake(xmlDocPtr doc, xmlNodePtr node, 
						Autotools* autotools)
{
	xmlNodePtr cur_node;
	
	/* Check arguments */
	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (autotools != NULL, FALSE);
	
	/* Read all elements into structure */
	cur_node = node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		const xmlChar* keyname = cur_node->name;
		xmlChar* key = xmlNodeListGetString(doc, 
			cur_node->xmlChildrenNode, 1);
		if (xmlStrcmp(keyname, INCLUDE) == 0)
		{
			autotools->automake_includes = g_list_append(
				autotools->automake_includes, g_strdup(key));
		}
		else if (xmlStrcmp(keyname, LD_ADD) == 0)
		{
			autotools->automake_ld_add = g_list_append(
				autotools->automake_ld_add, g_strdup(key));
		}
		else
		{
			/* Ignore unknown elements */
		}
		xmlFree(key);
		cur_node = cur_node->next;
	}
	return TRUE;
}

/* Free an autotools structure and all its elements */

void autotools_free(Autotools* autotools)
{
	
	if (autotools == NULL)
		return;
	g_list_free(autotools->autoconf_macros);
	g_list_free(autotools->pkgconfig);
	g_list_free(autotools->automake_includes);
	g_list_free(autotools->automake_ld_add);
	
	g_free(autotools);
}

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
 
#include "xmltemplate.h"

#include <libxml/parser.h>

static void print_autotools(Autotools* autotools);
static void print_project(PrjTemplate* temp);
static void print_lib(LibTemplate* temp);
static void print_lang(LangTemplate* temp);
static void print_text(TextTemplate* temp);

int main(int argc, char* argv[])
{
	xmlDocPtr doc;
	
	if (argc < 3)
	{
		printf("usage: template --[project|lib|lang|text] xmlfile\n");
		return -1;
	}
	doc = xmlParseFile(argv[2]);
	if (strcmp(argv[1], "--project") == 0)
	{
		PrjTemplate* temp;
		temp = prj_template_parse(doc);
		if (!temp)
			return -1;
		print_project(temp);
	}
	else if (strcmp(argv[1], "--lib") == 0)
	{
		LibTemplate* temp;
		temp = lib_template_parse(doc);
		if (!temp)
			return -1;
		print_lib(temp);
	}
	else if (strcmp(argv[1], "--lang") == 0)
	{
		LangTemplate* temp;
		temp = lang_template_parse(doc);
		if (!temp)
			return -1;
		print_lang(temp);
	}
	else if (strcmp(argv[1], "--text") == 0)
	{
		TextTemplate* temp;
		temp = text_template_parse(doc);
		if (!temp)
			return -1;
		print_text(temp);
	}
	
	return 0;
}

void print_autotools(Autotools* autotools)
{
	GList* node;
	node = autotools->autoconf_macros;
	while (node != NULL)
	{
		printf("macro = %s\n", node->data);
		node = node->next;
	}
	node = autotools->pkgconfig;
	while (node != NULL)
	{
		Pkgconfig* config = node->data;
		printf("pkgconfig = %s\n", config->package);
		printf("version = %s\n", config->version);
		node = node->next;
	}
	node = autotools->automake_includes;
	while (node != NULL)
	{
		printf("includes = %s\n", node->data);
		node = node->next;
	}
	node = autotools->automake_ld_add;
	while (node != NULL)
	{
		printf("macro = %s\n", node->data);
		node = node->next;
	}
}

void print_project(PrjTemplate* temp)
{
	GList * node;
	printf("name = %s\n", temp->name);
	printf("desc = %s\n", temp->description);
	printf("lang = %s\n", temp->lang);
	printf("target = %d\n", temp->target);
	node = temp->libs;
	while (node != NULL)
	{
		printf("lib = %s", node->data);
		node = node->next;
	}
	printf("init_script = %s\n", temp->init_script);
	printf("init_tarball = %s\n", temp->init_tarball);	
	
	print_autotools(temp->autotools);
}

void print_lib(LibTemplate* temp)
{
	printf("name = %s\n", temp->name);
	printf("desc = %s\n", temp->description);
	printf("lang = %s\n", temp->lang);
	printf("version = %s\n", temp->version);
	print_autotools(temp->autotools);	
}

void print_lang(LangTemplate* temp)
{
	printf("name = %s\n", temp->name);
	printf("desc = %s\n", temp->description);
	print_autotools(temp->autotools);	
}

void print_text(TextTemplate* temp)
{
	printf("name = %s\n", temp->name);
	printf("desc = %s\n", temp->description);
	printf("lang = %s\n", temp->lang);
	printf("text = %s\n", temp->text);	
}

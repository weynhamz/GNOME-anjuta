/*  xmltemplate.c (c) Johannes Schmid 2004
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
#include <libgnomevfs/gnome-vfs.h>
#include <gnome.h>

#include "xmltemplate.h"
#include "config.h"

#define XML_TYPE ".xml"

#define TEXT_TEMPLATE_DIR PACKAGE_DATA_DIR"/templates/texts"



struct _XmlTemplatePrivate
{
	GList* text_templates;
};

static void
xml_template_class_init (GObjectClass *klass);
static void
xml_template_instance_init(GObject* self);
static void
xml_template_finalize (GObject *self);


static void
xml_template_search_templates(XmlTemplate* self);

static void
xml_template_search_text_templates(GList** text_templates);
static gboolean
xml_template_read_template_dir(GList** templates, const gchar* template_dir);

GType
xml_template_get_type (void)
{
    static GType type = 0;
    if (type == 0) 
	{
     	static const GTypeInfo info = {
			sizeof (XmlTemplateClass),
			NULL,
			NULL,
			xml_template_class_init,
			NULL,
			NULL,
			sizeof (XmlTemplate),
			0, 
			xml_template_instance_init
     	 };
    	type = g_type_register_static (G_TYPE_OBJECT,
	                             "XmlTemplateType",
	                             &info, 0);
    }
    return type;
}



static void
xml_template_class_init (GObjectClass *klass)
{

}

static void
xml_template_instance_init (GObject *instance)
{	
	XmlTemplate *self = XML_TEMPLATE(instance);
  	self->priv = g_new0 (XmlTemplatePrivate, 1);
	xml_template_search_templates(self);	
}


static void
xml_template_finalize (GObject *self)
{

}

/**
 * xml_template_new:
 *
 * This function creates a new instace of xmltemplate. You need this instance to 
 * do anything usefull.
 * 
 * Returns: The newly created #XmlTemplate instance
 */

XmlTemplate*
xml_template_new()
{
	return XML_TEMPLATE(g_object_new (xml_template_get_type (), NULL));
}

static void
xml_template_search_templates(XmlTemplate* self)
{	
	/* Search availible templates */
	if (!gnome_vfs_initialized())
	{
		if(!gnome_vfs_init())
		{
			g_assert("Could not init gnome-vfs!");
			return;
		}
	}
	xml_template_search_text_templates(&self->priv->text_templates);

}






static void
xml_template_search_text_templates(GList** text_templates)
{
	if (!xml_template_read_template_dir(text_templates, TEXT_TEMPLATE_DIR))
	{
		g_warning("Could not read text templates");
	}
}

static gboolean
xml_template_read_template_dir(GList** templates, const gchar* template_dir)
{
	GnomeVFSResult result;
	GnomeVFSDirectoryHandle* dir;
	GnomeVFSFileInfo* xmlfile;
	
	result = gnome_vfs_directory_open(&dir, template_dir, 
		GNOME_VFS_FILE_INFO_DEFAULT);
  	if (result != GNOME_VFS_OK)
		return FALSE;
	while (gnome_vfs_directory_read_next(dir, xmlfile) == GNOME_VFS_OK)
	{
		if (strstr(xmlfile->name, XML_TYPE) != NULL)
		{
			gchar* filename = g_strconcat(PROJECT_TEMPLATE_DIR, "/",
				xmlfile->name, NULL);
			*templates = g_list_append(*templates, filename);
		}
	}
	gnome_vfs_directory_close(dir);	
}


/**
 * xml_template_get_all_texts:
 * @xtmp: An #XmlTemplate instance
 *
 * Get all availible text templates
 * 
 * Returns: A GList of #TextTemplates
 */

GList* xml_template_get_all_texts(XmlTemplate* xtmp)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	TextTemplate* text;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		text = text_template_parse(doc);
		if (text != NULL)
		{
			templates = g_list_append(templates, text);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

/**
 * xml_template_get_text:
 * @xtmp: An #XmlTemplate instance
 * @name: The name of the text template
 *
 * Get a specific language template by name
 * 
 * Returns: A #TextTemplate or NULL in case to templates matches
 */

TextTemplate* xml_template_get_text(XmlTemplate* xtmp, const gchar* name)
{
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	TextTemplate* text;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		text = text_template_parse(doc);
		if (text != NULL && strcmp(name, text->name) == 0)
		{
			return text;
		}
		text_template_free(text);
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return NULL;
}

/**
 * xml_template_query_texts_by_language:
 * @xtmp: An #XmlTemplate instance
 * @language: A string which descripes the languages (C, C++, etc.)
 *
 * Get all text templates for a given language
 * 
 * Returns: Returns a GList of #TextTemplates which match the given language.
 *	    The list might also be empty
 */

GList* xml_template_query_texts_by_language(XmlTemplate* xtmp, 
	const gchar* language)
{
		GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	TextTemplate* text;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		text = text_template_parse(doc);
		if (text != NULL && strcmp(language, text->lang) == 0)
		{
			templates = g_list_append(templates, text);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

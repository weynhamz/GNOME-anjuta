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

#define PROJECT_TEMPLATE_DIR PACKAGE_DATA_DIR"/template/project"
#define LANG_TEMPLATE_DIR PACKAGE_DATA_DIR"/templates/langs"
#define LIB_TEMPLATE_DIR PACKAGE_DATA_DIR"/templates/libs"
#define FILE_TEMPLATE_DIR PACKAGE_DATA_DIR"/templates/files"
#define TEXT_TEMPLATE_DIR PACKAGE_DATA_DIR"/templates/texts"



struct _XmlTemplatePrivate
{
	GList* prj_templates;
	GList* lang_templates;
	GList* lib_templates;
	
	GList* file_templates;
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
xml_template_search_prj_templates(GList** prj_templates);
static void
xml_template_search_lang_templates(GList** lang_templates);
static void
xml_template_search_lib_templates(GList** lib_templates);
static void
xml_template_search_file_templates(GList** file_templates);
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
	xml_template_search_prj_templates(&self->priv->prj_templates);
	xml_template_search_lang_templates(&self->priv->lang_templates);
	xml_template_search_lib_templates(&self->priv->lib_templates);
	xml_template_search_file_templates(&self->priv->file_templates);
	xml_template_search_text_templates(&self->priv->text_templates);

}




static void
xml_template_search_prj_templates(GList** prj_templates)
{
	GnomeVFSResult result;
	GnomeVFSDirectoryHandle* dir;
	GnomeVFSFileInfo* file = g_new0(GnomeVFSFileInfo, 1);
	
	result = gnome_vfs_directory_open(&dir, PROJECT_TEMPLATE_DIR, 
								GNOME_VFS_FILE_INFO_DEFAULT);
  	if (result != GNOME_VFS_OK)
	{
		g_warning("Could not read project templates");
		return;
	}
	while (gnome_vfs_directory_read_next(dir, file) == GNOME_VFS_OK)
	{
		if (file->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		{
			GnomeVFSDirectoryHandle* subdir;
			GnomeVFSFileInfo* xmlfile = g_new0(GnomeVFSFileInfo, 1);
			gchar* path = g_strconcat(PROJECT_TEMPLATE_DIR, "/", file->name, NULL);
			result = gnome_vfs_directory_open(&subdir, path, 
				GNOME_VFS_FILE_INFO_DEFAULT);
			g_free(path);
  			if (result != GNOME_VFS_OK)
			{
				g_warning("Could not read project templates");
				return;
			}
			while (gnome_vfs_directory_read_next(subdir, xmlfile) == GNOME_VFS_OK)
			{
				if (strstr(xmlfile->name, XML_TYPE) != NULL)
				{					
					gchar* filename = g_strconcat(PROJECT_TEMPLATE_DIR, "/",
						file->name, "/", xmlfile->name, NULL);
					*prj_templates = g_list_append(*prj_templates, filename);
				}
				gnome_vfs_file_info_clear(xmlfile);
			}
			gnome_vfs_directory_close(subdir);
		}			
	}
	gnome_vfs_directory_close(dir);
}

static void
xml_template_search_lang_templates(GList** lang_templates)
{
	if (!xml_template_read_template_dir(lang_templates, LANG_TEMPLATE_DIR))
	{
		g_warning("Could not read language templates");
	}
}

static void
xml_template_search_lib_templates(GList** lib_templates)
{
	if (!xml_template_read_template_dir(lib_templates, LIB_TEMPLATE_DIR))
	{
		g_warning("Could not read library templates");
	}
}

static void
xml_template_search_file_templates(GList** file_templates)
{
	if (!xml_template_read_template_dir(file_templates, FILE_TEMPLATE_DIR))
	{
		g_warning("Could not read file templates");
	}
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
 * xml_template_get_all_projects:
 * @xtmp: An #XmlTemplate instance
 *
 * Get a list of all availible #PrjTemplates
 *
 * Returns: A GList of project templates
 */

GList* xml_template_get_all_projects(XmlTemplate* xtmp)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->prj_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	PrjTemplate* prj;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		prj = prj_template_parse(doc);
		if (prj != NULL)
		{
			templates = g_list_append(templates, prj);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

/**
 * xml_template_get_project:
 * @xtmp: An #XmlTemplate instance
 * @name: The name of the project template
 *
 * Get a specific project template by name
 * 
 * Returns: A #PrjTemplate or NULL in case to templates matches
 */

PrjTemplate* xml_template_get_project(XmlTemplate* xtmp, const gchar* name)
{
	GList* cur_file = xtmp->priv->prj_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	PrjTemplate* prj;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		prj = prj_template_parse(doc);
		if (prj != NULL && strcmp(name, prj->name) == 0)
		{
			return prj;
		}
		prj_template_free(prj);
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return NULL;
}

/**
 * xml_template_query_projects_by_language:
 * @xtmp: An #XmlTemplate instance
 * @language: A string which descripes the languages (C, C++, etc.)
 *
 * Get all project templates for a given language
 * 
 * Returns: Returns a GList or #PrjTemplates which match the given language.
 *	    The list might also be empty
 */

GList* xml_template_query_projects_by_language (XmlTemplate* xtmp, 
	const gchar* language)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->prj_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	PrjTemplate* prj;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		prj = prj_template_parse(doc);
		if (prj != NULL && strcmp(prj->lang, language) == 0)
		{
			templates = g_list_append(templates, prj);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

/**
 * xml_template_get_all_libs:
 * @xtmp: An #XmlTemplate instance
 *
 * Get all availible library templates
 * 
 * Returns: A GList of PrjTemplates
 */

GList* xml_template_get_all_libs(XmlTemplate* xtmp)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	LibTemplate* lib;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		lib = lib_template_parse(doc);
		if (lib != NULL)
		{
			templates = g_list_append(templates, lib);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

/**
 * xml_template_get_lib:
 * @xtmp: An #XmlTemplate instance
 * @name: The name of the library template
 *
 * Get a specific library template by name
 * 
 * Returns: A #LibTemplate or NULL in case to templates matches
 */

LibTemplate* xml_template_get_lib(XmlTemplate* xtmp, const gchar* name)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	LibTemplate* lib;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		lib = lib_template_parse(doc);
		if (lib != NULL && strcmp(name, lib->name) == 0)
		{
			return lib;
		}
		lib_template_free(lib);
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return NULL;
}

/**
 * xml_template_query_libs_by_language:
 * @xtmp: An #XmlTemplate instance
 * @language: A string which descripes the languages (C, C++, etc.)
 *
 * Get all library templates for a given language
 * 
 * Returns: Returns a GList or LibTemplates which match the given language.
 *	    The list might also be empty
 */

GList* xml_template_query_libs_by_language(XmlTemplate* xtmp, 
	const gchar* language)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	LibTemplate* lib;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		lib = lib_template_parse(doc);
		if (lib != NULL && strcmp(language, lib->lang) == 0)
		{
			templates = g_list_append(templates, lib);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

/**
 * xml_template_get_all_languages:
 * @xtmp: An #XmlTemplate instance
 *
 * Get all availible language templates
 * 
 * Returns: A GList of #LangTemplates
 */

GList* xml_template_get_all_languages(XmlTemplate* xtmp)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	LangTemplate* lang;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		lang = lang_template_parse(doc);
		if (lang != NULL)
		{
			templates = g_list_append(templates, lang);
		}
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return templates;
}

/**
 * xml_template_get_language:
 * @xtmp: An #XmlTemplate instance
 * @name: The name of the language template
 *
 * Get a specific language template by name
 * 
 * Returns: A #LangTemplate or NULL in case no templates matches
 */

LangTemplate* xml_template_get_language(XmlTemplate* xtmp, const gchar* name)
{
	GList* templates = NULL;
	GList* cur_file = xtmp->priv->lib_templates;
	while (cur_file != NULL)
	{
		xmlDocPtr doc; 
    	LangTemplate* lang;
		doc = xmlParseFile(cur_file->data);
    	if (doc == NULL) 
		{
	    	cur_file = cur_file->next;
			continue;
		}
		lang = lang_template_parse(doc);
		if (lang != NULL && strcmp(name, lang->name) == 0)
		{
			return lang;
		}
		lang_template_free(lang);
		xmlFreeDoc(doc);
		cur_file = cur_file->next;
	}
	return NULL;
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

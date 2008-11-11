/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "plugin.h"
#define LANG_FILE PACKAGE_DATA_DIR"/languages.xml"

#define LANGUAGE_MANAGER(o) (LanguageManager*) (o)

typedef struct _Language Language;

struct _Language
{
	IAnjutaLanguageId id;
	gchar* name;
	GList* strings;
	GList* mime_types;
};
	

static gpointer parent_class;


static void
language_destroy (gpointer data)
{
	Language* lang = (Language*) data;
	
	g_free(lang->name);
	g_list_foreach(lang->strings, (GFunc) g_free, NULL);
	g_list_foreach(lang->mime_types, (GFunc) g_free, NULL);
	
	g_free(lang);
}

static void
load_languages (LanguageManager* language_manager)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr cur_node;
	
	LIBXML_TEST_VERSION
		
	doc = xmlReadFile (LANG_FILE, NULL, 0);
	if (!doc)
	{
		DEBUG_PRINT("Could not read language xml file %s!", LANG_FILE);
		return;
	}
	root = xmlDocGetRootElement (doc);
	
	if (!g_str_equal (root->name, "languages"))
	{
		DEBUG_PRINT ("%s", "Invalid languages.xml file");
	}
	
	for (cur_node = root->children; cur_node != NULL; cur_node = cur_node->next)
	{
		Language* lang = g_new0(Language, 1);
		gchar* id;
		gchar* mime_types;
		gchar* strings;
		
		if (!g_str_equal (cur_node->name, (const xmlChar*) "language"))
			continue;
		
		id = (gchar*) xmlGetProp (cur_node, (const xmlChar*) "id");
		lang->id = atoi(id);
		lang->name = (gchar*) xmlGetProp(cur_node, (const xmlChar*) "name");
		mime_types = (gchar*) xmlGetProp (cur_node, (const xmlChar*) "mime-types");
		strings = (gchar*) xmlGetProp (cur_node, (const xmlChar*) "strings");
		if (lang->id != 0 && lang->name && mime_types && strings)
		{
			GStrv mime_typesv = g_strsplit(mime_types, ",", -1);
			GStrv stringsv = g_strsplit (strings, ",", -1);
			GStrv type;
			GStrv string;
			
			for (type = mime_typesv; *type != NULL; type++)
			{
				lang->mime_types = g_list_append (lang->mime_types, g_strdup(*type));
			}
			g_strfreev(mime_typesv);

			for (string = stringsv; *string != NULL; string++)
			{
				lang->strings = g_list_append (lang->strings, g_strdup(*string));
			}
			g_strfreev(stringsv);
			
			g_hash_table_insert (language_manager->languages, GINT_TO_POINTER(lang->id), lang);
		}
		else
		{
			g_free(lang);
		}
		g_free (id);
		g_free (mime_types);
		g_free (strings);
	}	
	xmlFreeDoc(doc);
}

static gboolean
language_manager_activate (AnjutaPlugin *plugin)
{
	LanguageManager *language_manager;
	
	DEBUG_PRINT ("%s", "LanguageManager: Activating LanguageManager plugin ...");
	language_manager = (LanguageManager*) plugin;
	
	load_languages(language_manager);

	return TRUE;
}

static gboolean
language_manager_deactivate (AnjutaPlugin *plugin)
{	
	DEBUG_PRINT ("%s", "LanguageManager: Dectivating LanguageManager plugin ...");
	
	return TRUE;
}

static void
language_manager_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
language_manager_dispose (GObject *obj)
{
	/* Disposition codes */
	LanguageManager* lang = LANGUAGE_MANAGER (obj);
	
	g_hash_table_unref (lang->languages);
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
language_manager_instance_init (GObject *obj)
{
	LanguageManager *plugin = (LanguageManager*)obj;
	plugin->languages = g_hash_table_new_full (NULL, NULL,
											   NULL, language_destroy);

}

static void
language_manager_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = language_manager_activate;
	plugin_class->deactivate = language_manager_deactivate;
	klass->finalize = language_manager_finalize;
	klass->dispose = language_manager_dispose;
}

typedef struct
{
	LanguageManager* lang;
	const gchar* target;
	IAnjutaLanguageId result_id;
	gboolean found;
} LangData;

static void
language_manager_find_mime_type (gpointer key, Language* language, LangData* data)
{
	if (data->found)
		return;
	if (g_list_find_custom (language->mime_types, data->target, (GCompareFunc) strcmp))
	{
		data->result_id = language->id;
		data->found = TRUE;
	}
}

static void
language_manager_find_string (gpointer key, Language* language, LangData* data)
{
	if (data->found)
		return;
	if (g_list_find_custom (language->strings, data->target, (GCompareFunc) g_ascii_strcasecmp))
	{
		DEBUG_PRINT ("Found language %i: %s", language->id, language->name);
		data->result_id = language->id;
		data->found = TRUE;
	}
}

static IAnjutaLanguageId
ilanguage_get_from_mime_type (IAnjutaLanguage* ilang, const gchar* mime_type, GError** e)
{
	if (!mime_type)
		return 0;
	LanguageManager* lang = LANGUAGE_MANAGER(ilang);
	LangData* data = g_new0(LangData, 1);
	IAnjutaLanguageId ret_id;
	data->target = mime_type;
	g_hash_table_foreach (lang->languages, (GHFunc)language_manager_find_mime_type, data);
	if (data->found)
	{
		ret_id = data->result_id;
	}
	else
	{
		DEBUG_PRINT ("Unknown mime-type = %s", mime_type);
		ret_id = 0;
	}
	g_free(data);
	return ret_id;
}

static IAnjutaLanguageId
ilanguage_get_from_string (IAnjutaLanguage* ilang, const gchar* string, GError** e)
{
	if (!string)
		return 0;
	LanguageManager* lang = LANGUAGE_MANAGER(ilang);
	LangData* data = g_new0(LangData, 1);
	IAnjutaLanguageId ret_id;
	data->target = string;
	g_hash_table_foreach (lang->languages, (GHFunc)language_manager_find_string, data);
	if (data->found)
	{
		ret_id = data->result_id;
	}
	else
	{
		DEBUG_PRINT ("Unknown language string = %s", string);
		ret_id = 0;
	}
	g_free(data);
	return ret_id;
}

static const gchar*
ilanguage_get_name (IAnjutaLanguage* ilang, IAnjutaLanguageId id, GError** e)
{
	LanguageManager* lang = LANGUAGE_MANAGER(ilang);
	Language* language = g_hash_table_lookup (lang->languages,
											   GINT_TO_POINTER(id));
		
	if (language)
	{
		return language->name;
		DEBUG_PRINT ("Found language: %s", language->name);
	}
	else
		return NULL;
}

static GList*
ilanguage_get_strings (IAnjutaLanguage* ilang, IAnjutaLanguageId id, GError** e)
{
	LanguageManager* lang = LANGUAGE_MANAGER(ilang);
	Language* language = g_hash_table_lookup (lang->languages,
											   GINT_TO_POINTER(id));
	if (language)
		return language->strings;
	else
		return NULL;	
}

static IAnjutaLanguageId
ilanguage_get_from_editor (IAnjutaLanguage* ilang, IAnjutaEditorLanguage* editor, GError** e)
{	
	const gchar* language = 
			ianjuta_editor_language_get_language (editor, e);
		
	IAnjutaLanguageId id = 
			ilanguage_get_from_string (ilang, language, e);
	
	return id;
}

static const gchar*
ilanguage_get_name_from_editor (IAnjutaLanguage* ilang, IAnjutaEditorLanguage* editor, GError** e)
{
	return ilanguage_get_name (ilang,
							   ilanguage_get_from_editor (ilang, editor, e), e);
}

static void
ilanguage_iface_init (IAnjutaLanguageIface* iface)
{
	iface->get_from_mime_type = ilanguage_get_from_mime_type;
	iface->get_from_string = ilanguage_get_from_string;
	iface->get_name = ilanguage_get_name;
	iface->get_strings = ilanguage_get_strings;
	iface->get_from_editor = ilanguage_get_from_editor;
	iface->get_name_from_editor = ilanguage_get_name_from_editor;
};	

ANJUTA_PLUGIN_BEGIN (LanguageManager, language_manager);
ANJUTA_PLUGIN_ADD_INTERFACE (ilanguage, IANJUTA_TYPE_LANGUAGE);
ANJUTA_PLUGIN_END

ANJUTA_SIMPLE_PLUGIN (LanguageManager, language_manager);

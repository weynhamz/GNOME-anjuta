/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-xml-parser.c
    Copyright (C) Dragos Dena 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#include "snippets-xml-parser.h"
#include "snippet.h"
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <string.h>

#define NATIVE_XML_HEADER           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"

#define NATIVE_XML_ROOT             "anjuta-snippets-packet"
#define NATIVE_XML_GROUP_TAG        "anjuta-snippets-group"
#define NATIVE_XML_GROUP_NAME_TAG   "name"
#define NATIVE_XML_SNIPPETS_TAG     "anjuta-snippets"
#define NATIVE_XML_SNIPPET_TAG      "anjuta-snippet"
#define NATIVE_XML_LANGUAGES_TAG    "languages"
#define NATIVE_XML_VARIABLES_TAG    "variables"
#define NATIVE_XML_VARIABLE_TAG     "variable"
#define NATIVE_XML_CONTENT_TAG      "snippet-content"
#define NATIVE_XML_KEYWORDS_TAG     "keywords"
#define NATIVE_XML_NAME_PROP        "name"
#define NATIVE_XML_GLOBAL_PROP      "is_global"
#define NATIVE_XML_DEFAULT_PROP     "default"
#define NATIVE_XML_TRIGGER_PROP     "trigger"
#define NATIVE_XML_TRUE             "true"
#define NATIVE_XML_FALSE            "false"

#define GLOBAL_VARS_XML_ROOT         "anjuta-global-variables"
#define GLOBAL_VARS_XML_VAR_TAG      "global-variable"
#define GLOBAL_VARS_XML_NAME_PROP    "name"
#define GLOBAL_VARS_XML_COMMAND_PROP "is_command"
#define GLOBAL_VARS_XML_TRUE         "true"
#define GLOBAL_VARS_XML_FALSE        "false"

#define CDATA_START                  "<![CDATA["
#define CDATA_END                    "]]>"
#define CDATA_MID                    "]]><![CDATA["
#define IS_CDATA_END(text, i)        (text[i - 1] == ']' && text[i] == ']' && text[i + 1] == '>')           

#define QUOTE_STR                    "&quot;"


static void
write_simple_start_tag (GOutputStream *os,
                        const gchar *name)
{
	gchar *tag = g_strconcat ("<", name, ">\n", NULL);
	g_output_stream_write (os, tag, strlen (tag), NULL, NULL);
	g_free (tag);
}

static void
write_simple_end_tag (GOutputStream *os,
                      const gchar *name)
{
	gchar *tag = g_strconcat ("</", name, ">\n", NULL);
	g_output_stream_write (os, tag, strlen (tag), NULL, NULL);
	g_free (tag);
}

static gchar*
escape_text_cdata (const gchar *content)
{
	GString *formated_content = g_string_new (CDATA_START);
	gint i = 0, content_len = 0;

	content_len = strlen (content);
	for (i = 0; i < content_len; i ++)
	{
		/* If we found the "]]>" string in the content we should escape it. */
		if (i > 0 && IS_CDATA_END (content, i))
			g_string_append (formated_content, CDATA_MID);

		g_string_append_c (formated_content, content[i]);
	}
	g_string_append (formated_content, CDATA_END);

	return g_string_free (formated_content, FALSE);

}

static gchar*
escape_quotes (const gchar *text)
{
	GString *escaped_text = g_string_new ("");
	gint i = 0, len = 0;

	len = strlen (text);
	for (i = 0; i < len; i += 1)
	{
		if (text[i] == '\"')
		{
			escaped_text = g_string_append (escaped_text, QUOTE_STR);
			continue;
		}
		
		escaped_text = g_string_append_c (escaped_text, text[i]);
	}

	return g_string_free (escaped_text, FALSE);
}

static void
write_start_end_tag_with_content (GOutputStream *os,
                                  const gchar *tag_name,
                                  const gchar *content)
{
	gchar *tag_with_content = NULL, *escaped_content = NULL;

	escaped_content = escape_text_cdata (content);
	tag_with_content = g_strconcat ("<", tag_name, ">", escaped_content, "</", tag_name, ">\n", NULL);
	
	g_output_stream_write (os, tag_with_content, strlen (tag_with_content), NULL, NULL);
	g_free (tag_with_content);
	g_free (escaped_content);
}

static void
write_start_end_tag_with_content_as_list (GOutputStream *os,
                                          const gchar *tag_name,
                                          GList *content_list)
{
	GList *iter = NULL;
	GString *content = g_string_new ("");
	gchar *cur_word = NULL;

	for (iter = g_list_first (content_list); iter != NULL; iter = g_list_next (iter))
	{
		cur_word = (gchar *)iter->data;
		g_string_append (content, cur_word);
		g_string_append (content, " ");
	}
	
	write_start_end_tag_with_content (os, tag_name, content->str);
	g_string_free (content, TRUE);

}

static void
write_anjuta_snippet_tag (GOutputStream *os,
                          const gchar *trigger,
                          const gchar *name)
{
	gchar *tag = NULL, *escaped_name = NULL;

	escaped_name = escape_quotes (name);
	tag = g_strconcat ("<anjuta-snippet trigger=\"", trigger, "\" name=\"", escaped_name, "\">\n", NULL);
	g_output_stream_write (os, tag, strlen (tag), NULL, NULL);
	g_free (tag);
	g_free (escaped_name);
}

static void
write_variable_tag (GOutputStream *os,
                    const gchar *name,
                    const gchar *default_val,
                    gboolean is_global)
{
	const gchar *global_val = (is_global) ? NATIVE_XML_TRUE : NATIVE_XML_FALSE;
	gchar *tag = NULL, *escaped_name = NULL, *escaped_default_val = NULL;

	/* Escape the quotes */
	escaped_name = escape_quotes (name);
	escaped_default_val = escape_quotes (default_val);

	/* Write the tag */
	tag = g_strconcat ("<variable name=\"", escaped_name, 
	                   "\" default=\"", escaped_default_val, 
	                   "\" is_global=\"", global_val, "\" />\n", NULL);
	g_output_stream_write (os, tag, strlen (tag), NULL, NULL);

	/* Free the data */
	g_free (tag);
	g_free (escaped_name);
	g_free (escaped_default_val);
}

static gboolean
write_snippet (GOutputStream *os,
               AnjutaSnippet *snippet)
{
	GList *iter = NULL, *iter2 = NULL, *iter3 = NULL, *keywords = NULL,
	      *vars_names = NULL, *vars_defaults = NULL, *vars_globals = NULL;
	const gchar *content = NULL;

	/* Assertions */
	g_return_val_if_fail (G_IS_OUTPUT_STREAM (os), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);

	write_anjuta_snippet_tag (os, 
	                          snippet_get_trigger_key (snippet),
	                          snippet_get_name (snippet));

	/* Write the languages */
	write_start_end_tag_with_content_as_list (os, 
	                                          NATIVE_XML_LANGUAGES_TAG,
	                                          (GList *)snippet_get_languages (snippet));

	/* Write the variables */
	write_simple_start_tag (os, NATIVE_XML_VARIABLES_TAG);
	/* Write each variable */
	vars_names    = snippet_get_variable_names_list (snippet);
	vars_defaults = snippet_get_variable_defaults_list (snippet);
	vars_globals  = snippet_get_variable_globals_list (snippet);
	iter  = g_list_first (vars_names);
	iter2 = g_list_first (vars_defaults);
	iter3 = g_list_first (vars_globals);
	while (iter != NULL && iter2 != NULL && iter3 != NULL)
	{
		write_variable_tag (os, 
		                    (gchar *)iter->data, 
		                    (gchar *)iter2->data, 
		                    GPOINTER_TO_INT (iter3->data));

		iter  = g_list_next (iter);
		iter2 = g_list_next (iter2);
		iter3 = g_list_next (iter3);
	}
	g_list_free (vars_names);
	g_list_free (vars_defaults);
	g_list_free (vars_globals);
	write_simple_end_tag (os, NATIVE_XML_VARIABLES_TAG);

	/* Write the content */
	content = snippet_get_content (snippet);
	write_start_end_tag_with_content (os, NATIVE_XML_CONTENT_TAG, content);

	/* Write the keywords */
	keywords = snippet_get_keywords_list (snippet);
	write_start_end_tag_with_content_as_list (os, NATIVE_XML_KEYWORDS_TAG, keywords);
	g_list_free (keywords);
	write_simple_end_tag (os, NATIVE_XML_SNIPPET_TAG);

	return TRUE;
}

static gboolean
write_snippets_group (GOutputStream *os,
                      AnjutaSnippetsGroup *snippets_group)
{
	GList *iter = NULL, *snippets = NULL;

	/* Assertions */
	g_return_val_if_fail (G_IS_OUTPUT_STREAM (os), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);

	write_simple_start_tag (os, NATIVE_XML_GROUP_TAG);

	/* Write the name tag */
	write_start_end_tag_with_content (os, 
	                                  NATIVE_XML_GROUP_NAME_TAG,
	                                  snippets_group_get_name (snippets_group));

	write_simple_start_tag (os, NATIVE_XML_SNIPPETS_TAG);

	/* Write the snippets */
	snippets = snippets_group_get_snippets_list (snippets_group);
	for (iter = g_list_first (snippets); iter != NULL; iter = g_list_next (iter))
	{
		if (!ANJUTA_IS_SNIPPET (iter->data))
			continue;

		write_snippet (os, ANJUTA_SNIPPET (iter->data));
	}
	write_simple_end_tag (os, NATIVE_XML_SNIPPETS_TAG);

	write_simple_end_tag (os, NATIVE_XML_GROUP_TAG);
	
	return TRUE;
}

static gboolean 
snippets_manager_save_native_xml_file (GList* snippets_groups,
                                       const gchar *file_path)
{
	GFile *file = NULL;
	GOutputStream *os = NULL;
	GList *iter = NULL;

	/* Assertions */
	g_return_val_if_fail (file_path != NULL, FALSE);

	/* Open the file */
	file = g_file_new_for_path (file_path);
	os = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL));
	if (!G_IS_OUTPUT_STREAM (os))
	{
		g_object_unref (file);
		return FALSE;
	}

	if (g_output_stream_write (os, NATIVE_XML_HEADER, strlen (NATIVE_XML_HEADER), NULL, NULL) < 0)
	{
		g_output_stream_close (os, NULL, NULL);
		g_object_unref (os);
		g_object_unref (file);
		return FALSE;
	}

	write_simple_start_tag (os, NATIVE_XML_ROOT);

	for (iter = g_list_first (snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		if (!ANJUTA_IS_SNIPPETS_GROUP (iter->data))
			continue;

		write_snippets_group (os, ANJUTA_SNIPPETS_GROUP (iter->data));
	}

	write_simple_end_tag (os, NATIVE_XML_ROOT);

	/* Close the file */
	g_output_stream_close (os, NULL, NULL);
	g_object_unref (os);
	g_object_unref (file);

	return TRUE;
}

static gboolean 
snippets_manager_save_gedit_xml_file (GList* snippets_group,
                                      const gchar *file_path)
{
	/* TODO */
	return FALSE;
}

static AnjutaSnippet*
parse_snippet_node (xmlNodePtr snippet_node)
{
	AnjutaSnippet* snippet = NULL;
	gchar *trigger_key = NULL, *snippet_name = NULL, *snippet_content = NULL, *cur_var_name = NULL, 
	      *cur_var_default = NULL, *cur_var_global = NULL, *keywords_temp = NULL,
	      **keywords_temp_array = NULL, *keyword_temp = NULL, *languages_temp = NULL,
	      **languages_temp_array = NULL, *language_temp = NULL;
	GList *variable_names = NULL, *variable_default_values = NULL,
	      *variable_globals = NULL, *keywords = NULL, *iter = NULL, 
	      *snippet_languages = NULL;
	xmlNodePtr cur_field_node = NULL, cur_variable_node = NULL;
	gboolean cur_var_is_global = FALSE;
	gint i = 0;

	/* Assert that the snippet_node is indeed a anjuta-snippet tag */
	g_return_val_if_fail (!g_strcmp0 ((gchar *)snippet_node->name, NATIVE_XML_SNIPPET_TAG), NULL);
	
	/* Get the snippet name, trigger-key and language properties */
	trigger_key = (gchar *)xmlGetProp (snippet_node, (const xmlChar *)NATIVE_XML_TRIGGER_PROP);
	snippet_name = (gchar *)xmlGetProp (snippet_node, (const xmlChar *)NATIVE_XML_NAME_PROP);

	/* Make sure we got all the above properties */
	if (trigger_key == NULL || snippet_name == NULL)
	{
		g_free (trigger_key);
		g_free (snippet_name);
		return NULL;
	}

	/* Get the snippet fields (variables, content and keywords) in the following loop */
	cur_field_node = snippet_node->xmlChildrenNode;
	while (cur_field_node != NULL)
	{
		/* We will look in all the variables and save them */
		if (!g_strcmp0 ((gchar*)cur_field_node->name, NATIVE_XML_VARIABLES_TAG))
		{
			cur_variable_node = cur_field_node->xmlChildrenNode;
			while (cur_variable_node != NULL)
			{
				/* Check if it's a variable tag */
				if (g_strcmp0 ((gchar *)cur_variable_node->name, NATIVE_XML_VARIABLE_TAG))
				{
					cur_variable_node = cur_variable_node->next;
					continue;
				}

				/* Get the variable properties */
				cur_var_name = (gchar*)xmlGetProp (cur_variable_node,\
				                                   (const xmlChar*)NATIVE_XML_NAME_PROP);
				cur_var_default = (gchar*)xmlGetProp (cur_variable_node,\
				                                      (const xmlChar*)NATIVE_XML_DEFAULT_PROP);
				cur_var_global = (gchar*)xmlGetProp (cur_variable_node,\
				                                     (const xmlChar*)NATIVE_XML_GLOBAL_PROP);
				if (!g_strcmp0 (cur_var_global, NATIVE_XML_TRUE))
					cur_var_is_global = TRUE;
				else
					cur_var_is_global = FALSE;
				g_free (cur_var_global);

				/* Add the properties to the lists */
				variable_names = g_list_append (variable_names, cur_var_name);
				variable_default_values = g_list_append (variable_default_values, cur_var_default);
				variable_globals = g_list_append (variable_globals, GINT_TO_POINTER (cur_var_is_global));
				
				cur_variable_node = cur_variable_node->next;
			}
		}

		/* Get the actual snippet content */
		if (!g_strcmp0 ((gchar*)cur_field_node->name, NATIVE_XML_CONTENT_TAG))
		{
			snippet_content = (gchar *)xmlNodeGetContent (cur_field_node);
		}

		/* Parse the keywords of the snippet */
		if (!g_strcmp0 ((gchar*)cur_field_node->name, NATIVE_XML_KEYWORDS_TAG))
		{
			keywords_temp = (gchar *)xmlNodeGetContent (cur_field_node);
			keywords_temp_array = g_strsplit (keywords_temp, " ", -1);
			
			i = 0;
			while (keywords_temp_array[i])
			{
				if (g_strcmp0 (keywords_temp_array[i], ""))
				{
					keyword_temp = g_strdup (keywords_temp_array[i]);
					keywords = g_list_append (keywords, keyword_temp);
				}
				i ++;
			}
			
			g_free (keywords_temp);
			g_strfreev (keywords_temp_array);
		}

		/* Parse the languages of the snippet */
		if (!g_strcmp0 ((gchar *)cur_field_node->name, NATIVE_XML_LANGUAGES_TAG))
		{
			languages_temp = (gchar *)xmlNodeGetContent (cur_field_node);
			languages_temp_array = g_strsplit (languages_temp, " ", -1);

			i = 0;
			while (languages_temp_array[i])
			{
				if (g_strcmp0 (languages_temp_array[i], ""))
				{
					language_temp = g_strdup (languages_temp_array[i]);
					snippet_languages = g_list_append (snippet_languages, language_temp); 
				}
				i ++;
			}
			
			g_free (languages_temp);
			g_strfreev (languages_temp_array);
		}
		
		cur_field_node = cur_field_node->next;
	}
	
	/* Make a new AnjutaSnippet object */
	snippet = snippet_new (trigger_key, 
	                       snippet_languages, 
	                       snippet_name, 
	                       snippet_content, 
	                       variable_names, 
	                       variable_default_values, 
	                       variable_globals, 
	                       keywords);

	/* Free the memory (the data is copied on the snippet constructor) */
	g_free (trigger_key);
	g_free (snippet_name);
	g_free (snippet_content);
	for (iter = g_list_first (variable_names); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	for (iter = g_list_first (variable_default_values); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (variable_names);
	g_list_free (variable_default_values);
	g_list_free (variable_globals);
	for (iter = g_list_first (snippet_languages); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (snippet_languages);
	for (iter = g_list_first (keywords); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (keywords);

	return snippet;
}

static AnjutaSnippetsGroup*
parse_snippets_group_node (xmlNodePtr snippets_group_node)
{
	AnjutaSnippetsGroup *snippets_group = NULL;
	AnjutaSnippet *cur_snippet = NULL;
	xmlNodePtr cur_snippet_node = NULL, cur_node = NULL;
	gchar *group_name = NULL;

	/* Get the group name */
	cur_node = snippets_group_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		if (!g_strcmp0 ((gchar*)cur_node->name, NATIVE_XML_GROUP_NAME_TAG))
		{
			group_name = g_strdup ((gchar *)xmlNodeGetContent (cur_node));
			break;
		}
		
		cur_node = cur_node->next;
	}
	if (group_name == NULL)
		return NULL;

	/* Initialize the snippets group */
	snippets_group = snippets_group_new (group_name);

	/* Get the snippets */
	cur_node = snippets_group_node->xmlChildrenNode;
	while (cur_node)
	{
		if (!g_strcmp0 ((gchar *)cur_node->name, NATIVE_XML_SNIPPETS_TAG))
		{
			cur_snippet_node = cur_node->xmlChildrenNode;

			/* Look trough all the snippets */
			while (cur_snippet_node)
			{
				/* Make sure it's a snippet node */
				if (g_strcmp0 ((gchar *)cur_snippet_node->name, NATIVE_XML_SNIPPET_TAG))
				{
					cur_snippet_node = cur_snippet_node->next;
					continue;
				}

				/* Get a new AnjutaSnippet object from the current node and add it to the group*/
				cur_snippet = parse_snippet_node (cur_snippet_node);
				if (ANJUTA_IS_SNIPPET (cur_snippet))
					snippets_group_add_snippet (snippets_group, cur_snippet);

				cur_snippet_node = cur_snippet_node->next;
			}

			break;
		}

		cur_node = cur_node->next;
	}

	return snippets_group;
}

static GList* 
snippets_manager_parse_native_xml_file (const gchar *snippet_packet_path)
{
	AnjutaSnippetsGroup* snippets_group = NULL;
	GList* snippets_groups = NULL;
	xmlDocPtr snippet_packet_doc = NULL;
	xmlNodePtr cur_node = NULL;
	
	/* Parse the XML file and load it into a xmlDoc */
	snippet_packet_doc = xmlParseFile (snippet_packet_path);
	if (snippet_packet_doc == NULL)
		return NULL;
	
	/* Get the root and assert it */
	cur_node = xmlDocGetRootElement (snippet_packet_doc);
	if (cur_node == NULL || g_strcmp0 ((gchar *)cur_node->name, NATIVE_XML_ROOT))
	{
		xmlFreeDoc (snippet_packet_doc);
		return NULL;
	}

	/* Get the snippets groups */
	cur_node = cur_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		/* Get the current snippets group */
		if (!g_strcmp0 ((gchar*)cur_node->name, NATIVE_XML_GROUP_TAG))
		{
			snippets_group = parse_snippets_group_node (cur_node);

			if (ANJUTA_IS_SNIPPETS_GROUP (snippets_group))
				snippets_groups = g_list_prepend (snippets_groups, snippets_group);
		}
		
		cur_node = cur_node->next;
	}
	
	xmlFreeDoc (snippet_packet_doc);
	
	return snippets_groups;
}

static GList*
snippets_manager_parse_gedit_xml_file (const gchar* snippet_packet_path)
{
	/* TODO */
	return NULL;
}


/**
 * snippets_manager_parse_xml_file:
 * @snippet_packet_path: The path of the XML file describing the Snippet Group
 * @format_Type: The type of the XML file (see snippets-db.h for the supported types)
 *
 * Parses the given XML file.
 *
 * Returns: A list of #AnjutaSnippetGroup objects.
 **/
GList*	
snippets_manager_parse_snippets_xml_file (const gchar* snippet_packet_path,
                                          FormatType format_type)
{
	switch (format_type)
	{
		case NATIVE_FORMAT:
			return snippets_manager_parse_native_xml_file (snippet_packet_path);
		
		case GEDIT_FORMAT:
			return snippets_manager_parse_gedit_xml_file (snippet_packet_path);
		
		default:
			return NULL;
	}
}

/**
 * snippets_manager_parse_xml_file:
 * @format_type: The type of the XML file (see snippets-db.h for the supported types)
 * @snippets_group: A list #AnjutaSnippetsGroup objects.
 *
 * Saves the given snippets to a snippet-packet XML file.
 *
 * Returns: TRUE on success.
 **/
gboolean
snippets_manager_save_snippets_xml_file (FormatType format_type,
                                         GList* snippets_groups,
                                         const gchar *file_path)
{
	switch (format_type)
	{
		case NATIVE_FORMAT:
			return snippets_manager_save_native_xml_file (snippets_groups, file_path);
		
		case GEDIT_FORMAT:
			return snippets_manager_save_gedit_xml_file (snippets_groups, file_path);
		
		default:
			return FALSE;
	}
}

/**
 * snippets_manager_parse_variables_xml_file:
 * @global_vars_path: A path with a XML file describing snippets global variables.
 * @snippets_db: A #SnippetsDB object where the global variables should be loaded.
 *
 * Loads the global variables from the given XML file in the given #SnippetsDB object.
 *
 * Returns: TRUE on success.
 */
gboolean 
snippets_manager_parse_variables_xml_file (const gchar* global_vars_path,
                                           SnippetsDB* snippets_db)
{
	xmlDocPtr global_vars_doc = NULL;
	xmlNodePtr cur_var_node = NULL;
	gchar *cur_var_name = NULL, *cur_var_is_command = NULL, *cur_var_content = NULL;
	gboolean cur_var_is_command_bool = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (global_vars_path != NULL, FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);

	/* Parse the XML file */
	global_vars_doc = xmlParseFile (global_vars_path);
	g_return_val_if_fail (global_vars_doc != NULL, FALSE);

	/* Get the root and assert it */
	cur_var_node = xmlDocGetRootElement (global_vars_doc);
	if (cur_var_node == NULL ||\
	    g_strcmp0 ((gchar *)cur_var_node->name, GLOBAL_VARS_XML_ROOT))
	{
		xmlFreeDoc (global_vars_doc);
		return FALSE;
	}

	/* Get the name and description fields */
	cur_var_node = cur_var_node->xmlChildrenNode;
	while (cur_var_node != NULL)
	{
		/* Get the current Snippet Global Variable */
		if (!g_strcmp0 ((gchar*)cur_var_node->name, GLOBAL_VARS_XML_VAR_TAG))
		{
			/* Get the name, is_command properties and the content */
			cur_var_name = (gchar*)xmlGetProp (cur_var_node,\
		                                       (const xmlChar*)GLOBAL_VARS_XML_NAME_PROP);
			cur_var_is_command = (gchar*)xmlGetProp (cur_var_node,\
		                                       (const xmlChar*)GLOBAL_VARS_XML_COMMAND_PROP);
		    cur_var_content = g_strdup ((gchar *)xmlNodeGetContent (cur_var_node));
			if (!g_strcmp0 (cur_var_is_command, GLOBAL_VARS_XML_TRUE))
				cur_var_is_command_bool = TRUE;
			else
				cur_var_is_command_bool = FALSE;

			/* Add the Global Variable to the Snippet Database */
			snippets_db_add_global_variable (snippets_db,
			                                 cur_var_name,
			                                 cur_var_content,
			                                 cur_var_is_command_bool,
			                                 TRUE);
			
		    g_free (cur_var_content);
		    g_free (cur_var_name);
		    g_free (cur_var_is_command);
		}
		
		cur_var_node = cur_var_node->next;
	}

	return TRUE;
}

static void
write_global_var_tags (GOutputStream *os,
                       const gchar *name,
                       const gchar *value,
                       gboolean is_command)
{
	gchar *command_string = NULL, *escaped_content = NULL, *line = NULL,
	      *escaped_name = NULL;

	/* Assertions */
	g_return_if_fail (G_IS_OUTPUT_STREAM (os));

	/* Escape the text */
	command_string = (is_command) ? GLOBAL_VARS_XML_TRUE : GLOBAL_VARS_XML_FALSE;
	escaped_content = escape_text_cdata (value);
	escaped_name = escape_quotes (name);

	/* Write the tag */
	line = g_strconcat ("<global-variable name=\"", escaped_name, 
	                    "\" is_command=\"", command_string, "\">", 
	                    escaped_content, 
	                    "</global-variable>\n",
	                    NULL);
	g_output_stream_write (os, line, strlen (line), NULL, NULL);
	
	/* Free the memory */
	g_free (line);
	g_free (escaped_content);
	g_free (escaped_name);
}

/**
 * snippets_manager_save_variables_xml_file:
 * @global_vars_path: A path with a XML file describing snippets global variables.
 * @global_vars_name_list: A #GList with the name of the variables.
 * @global_vars_value_list: A #GList with the values of the variables.
 * @global_vars_is_command_list: A #Glist with #gboolean values showing if the value
 *                               of the given variable is a command. 
 *
 * Saves the given snippets global variables in a XML file at the given path.
 *
 * Returns: TRUE on success.
 */
gboolean 
snippets_manager_save_variables_xml_file (const gchar* global_variables_path,
                                          GList* global_vars_name_list,
                                          GList* global_vars_value_list,
                                          GList* global_vars_is_command_list)
{
	GList *iter = NULL, *iter2 = NULL, *iter3 = NULL;
	GFile *file = NULL;
	GOutputStream *os = NULL;

	/* Assertions */
	g_return_val_if_fail (global_variables_path != NULL, FALSE);

	/* Open the file */
	file = g_file_new_for_path (global_variables_path);
	os = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL));
	if (!G_IS_OUTPUT_STREAM (os))
	{
		g_object_unref (file);
		return FALSE;
	}

	if (g_output_stream_write (os, NATIVE_XML_HEADER, strlen (NATIVE_XML_HEADER), NULL, NULL) < 0)
	{
		g_output_stream_close (os, NULL, NULL);
		g_object_unref (os);
		g_object_unref (file);
		return FALSE;
	}

	write_simple_start_tag (os, GLOBAL_VARS_XML_ROOT);

	/* Write each of the global variable */
	iter  = g_list_first (global_vars_name_list);
	iter2 = g_list_first (global_vars_value_list);
	iter3 = g_list_first (global_vars_is_command_list);
	while (iter != NULL && iter2 != NULL && iter3 != NULL)
	{
		write_global_var_tags (os, 
		                       (gchar *)iter->data, 
		                       (gchar *)iter2->data, 
		                       GPOINTER_TO_INT (iter3->data));

		iter  = g_list_next (iter);
		iter2 = g_list_next (iter2);
		iter3 = g_list_next (iter3);
	}
	
	write_simple_end_tag (os, GLOBAL_VARS_XML_ROOT);
	
	/* Close the file */
	g_output_stream_close (os, NULL, NULL);
	g_object_unref (os);
	g_object_unref (file);	

	return TRUE;
}

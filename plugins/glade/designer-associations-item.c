/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "designer-associations.h"
#include "designer-associations-item.h"

static gchar *
claim_xml_string (xmlChar *str)
{
	gchar *new_str = NULL;


	if (xmlStrcmp (str, BAD_CAST ("")) != 0)
		new_str = g_strdup ((gchar *) str);
	xmlFree (str);
	return new_str;
}

/* DesignerAssociationsItem */

G_DEFINE_TYPE (DesignerAssociationsItem, designer_associations_item, G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;

static void
designer_associations_item_init (DesignerAssociationsItem *self)
{

}

static void
designer_associations_item_finalize (GObject *object)
{
	GList *node;
	DesignerAssociationsItem *self = DESIGNER_ASSOCIATIONS_ITEM (object);

	if (self->designer)
		g_object_unref (self->designer);
	g_free (self->widget_name);
	if (self->editor)
		g_object_unref (self->editor);

	node = self->options;
	while (node)
	{
		DesignerAssociationsOption *option = node->data;
		designer_associations_option_free (option);

		node = node->next;
	}
	g_list_free (self->options);
	self->options = NULL;

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
designer_associations_item_class_init (DesignerAssociationsItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = designer_associations_item_finalize;

}

DesignerAssociationsItem *
designer_associations_item_new (void)
{
	return g_object_new (DESIGNER_TYPE_ASSOCIATIONS_ITEM, NULL);
}

DesignerAssociationsItem *
designer_associations_item_from_data (GFile *editor, const gchar *widget_name,
                                      GFile *designer, GList *options,
                                      GFile *project_root)
{
	DesignerAssociationsItem *retval;

	retval = g_object_new (DESIGNER_TYPE_ASSOCIATIONS_ITEM, NULL);
	retval->editor = editor;
	g_object_ref (editor);
	retval->widget_name = g_strdup (widget_name);
	retval->designer = designer;
	g_object_ref (designer);
	retval->options = options;
	return retval;
}

GList *
designer_associations_item_get_option_node (DesignerAssociationsItem *self,
                                            const gchar *name)
{
	DesignerAssociationsOption *option;
	GList *node;

	g_return_val_if_fail (DESIGNER_IS_ASSOCIATIONS_ITEM (self), NULL);
	node = self->options;
	while (node)
	{
		option = node->data;
		if (g_str_equal (name, option->name))
			return node;

		node = node->next;
	}

	return NULL;
}

void
designer_associations_item_set_option (DesignerAssociationsItem *self,
                                       const gchar *name, const gchar *value)
{
	DesignerAssociationsOption *option;
	GList *node;

	node = designer_associations_item_get_option_node (self, name);
	if (node)
	{
		option = node->data;
		g_free (option->value);
		option->value = value ? g_strdup (value) : NULL;
	}
	else
	{
		option = designer_associations_option_new ();
		option->name = g_strdup (name);
		option->value = g_strdup (value);
		self->options = g_list_prepend (self->options, option);
	}
}

gchar *
designer_associations_item_get_option (DesignerAssociationsItem *self,
                                       const gchar *name)
{
	GList *node;

	g_return_val_if_fail (DESIGNER_IS_ASSOCIATIONS_ITEM (self), NULL);

	node = designer_associations_item_get_option_node (self, name);
	if (node)
		return g_strdup (((DesignerAssociationsOption *)node->data)->value);
	else
		return NULL;
}

void
designer_associations_item_set_widget_name (DesignerAssociationsItem *self,
                                            const gchar *value)
{
	g_return_if_fail (DESIGNER_IS_ASSOCIATIONS_ITEM (self));

	g_free (self->widget_name);
	self->widget_name = g_strdup (value);
}

gint
designer_associations_item_get_option_as_int (DesignerAssociationsItem *self,
                                              const gchar *name, const gchar **descriptions)
{
	gchar *str;
	gint i;

	str = designer_associations_item_get_option (self, name);
	if (!str)
		return 0;

	if (descriptions)
	{
		gint i = 0;

		while (descriptions[i])
		{
			if (g_str_equal (descriptions[i], str))
			{
				g_free (str);
				return i;
			}

			i++;
		}
	}

	i = g_ascii_strtoll (str, NULL, 10);
	g_free (str);
	return i;
}

xmlNodePtr
search_child (xmlNodePtr node, const gchar *name)
{
	xmlNodePtr child_node;

	for (child_node = node->children; child_node;
	     child_node = child_node->next)
	{
		if (!xmlStrcmp (child_node->name, BAD_CAST (name)))
			return child_node;
	}
	return NULL;
}

DesignerAssociationsItem *
designer_associations_item_from_xml (DesignerAssociationsItem *self,
                                     xmlDocPtr xml_doc, xmlNodePtr node,
                                     GFile *project_root,
                                     GError **error)
{
	GError *nested_error = NULL;
	DesignerAssociationsOption *option;
	xmlNodePtr child_node;

	g_return_val_if_fail (error == NULL || *error == NULL, self);
	g_return_val_if_fail (xml_doc, self);
	g_return_val_if_fail (node, self);

	/* self->designer */
	child_node = search_child (node, DA_XML_TAG_DESIGNER);
	if (!child_node)
	{
		g_set_error (error,
		             DESIGNER_ASSOCIATIONS_ERROR,
			         DESIGNER_ASSOCIATIONS_ERROR_LOADING,
			         _("association item has no designer"));
		return self;
	}
	self->designer = associations_file_from_xml (xml_doc, child_node,
	                                                 project_root, &nested_error);
	if (nested_error)
	{
		g_propagate_error (error, nested_error);
		return self;
	}

	/* self->widget_name */
	child_node = search_child (node, DA_XML_TAG_WIDGET);
	if (child_node)
	{
		self->widget_name = claim_xml_string (xmlGetProp (child_node, BAD_CAST (DA_XML_PROP_OPTION_NAME)));
	}

	/* self->editor */
	child_node = search_child (node, DA_XML_TAG_EDITOR);
	if (!child_node)
	{
		g_set_error (error,
		             DESIGNER_ASSOCIATIONS_ERROR,
			         DESIGNER_ASSOCIATIONS_ERROR_LOADING,
			         _("association item has no editor"));
		return self;
	}
	self->editor = associations_file_from_xml (xml_doc, child_node,
	                                           project_root, &nested_error);
	if (nested_error)
	{
		g_propagate_error (error, nested_error);
		return self;
	}

	/* options */
	child_node = node->children;
	while (child_node)
	{
		if (!xmlStrcmp (child_node->name, BAD_CAST (DA_XML_TAG_OPTION)))
		{
			option = designer_associations_option_new ();
			designer_associations_option_from_xml (option, xml_doc, child_node,
			                                       &nested_error);
			if (nested_error)
			{
				designer_associations_option_free (option);
				g_propagate_error (error, nested_error);
				return self;
			}
			self->options = g_list_append (self->options, option);
		}
		child_node = child_node->next;
	}

	return self;
}

void
designer_associations_item_to_xml (DesignerAssociationsItem *self,
                                   xmlDocPtr xml_doc, xmlNodePtr node,
                                   GFile *project_root)
{
	xmlNodePtr child_node;

	child_node = xmlNewDocNode (xml_doc, NULL, BAD_CAST (DA_XML_TAG_DESIGNER), NULL);
	xmlAddChild (node, child_node);
	associations_file_to_xml (self->designer, xml_doc, child_node, project_root);

	child_node = xmlNewDocNode (xml_doc, NULL, BAD_CAST (DA_XML_TAG_WIDGET), NULL);
	xmlAddChild (node, child_node);
	xmlSetProp (child_node, BAD_CAST (DA_XML_PROP_WIDGET), BAD_CAST (self->widget_name));

	child_node = xmlNewDocNode (xml_doc, NULL, BAD_CAST (DA_XML_TAG_EDITOR), NULL);
	xmlAddChild (node, child_node);
	associations_file_to_xml (self->editor, xml_doc, child_node, project_root);

	designer_associations_options_to_xml (self->options, xml_doc, node);
}

void
designer_associations_options_to_xml (GList *options,
                                      xmlDocPtr xml_doc, xmlNodePtr node)
{
	GList *item;
	DesignerAssociationsOption *option;
	xmlNodePtr option_node;

	item = options;
	while (item)
	{
		option = item->data;

		if (option->name && option->value)
		{
			option_node = xmlNewDocNode (xml_doc, NULL, BAD_CAST (DA_XML_TAG_OPTION), NULL);
			xmlAddChild (node, option_node);
			xmlSetProp (option_node, BAD_CAST (DA_XML_PROP_OPTION_NAME), BAD_CAST (option->name));
			xmlSetProp (option_node, BAD_CAST (DA_XML_PROP_OPTION_VALUE), BAD_CAST (option->value));
		}

		item = item->next;
	}

	return;
}

DesignerAssociationsOption *
designer_associations_option_from_xml (DesignerAssociationsOption *self,
									   xmlDocPtr xml_doc, xmlNodePtr node,
									   GError **error)
{
	xmlChar *name = NULL;
	xmlChar *value = NULL;

	g_return_val_if_fail (error == NULL || *error == NULL, self);
	g_return_val_if_fail (xml_doc, self);
	g_return_val_if_fail (node, self);

	name = xmlGetProp (node, BAD_CAST (DA_XML_PROP_OPTION_NAME));
	value = xmlGetProp (node, BAD_CAST (DA_XML_PROP_OPTION_VALUE));

	if (!name || !value)
	{
		g_set_error (error,
		             DESIGNER_ASSOCIATIONS_ERROR,
		             DESIGNER_ASSOCIATIONS_ERROR_LOADING,
		             _("bad association item option in the node %s"),
		             node->name);
		xmlFree (name);
		xmlFree (value);
		return self;
	}

	g_free (self->name);
	g_free (self->value);
	self->name = claim_xml_string (name);
	self->value = claim_xml_string (value);

	return self;
}

gchar *
designer_associations_options_to_string (GList *options, gchar *value_delimiter,
                                         gchar *option_delimiter)
{
	GList *item;
	DesignerAssociationsOption *option;
	gchar **str_array;
	guint count;
	gchar *retval;
	guint i;

	count = g_list_length (options);
	if (count == 0)
		return NULL;

	str_array = g_new0 (gchar *, count + 1);
	item = options;
	i = 0;
	while (item)
	{
		option = item->data;

		if (option->name && option->value)
		{
			str_array[i] = g_strconcat (option->name, value_delimiter, option->value, NULL);
			i++;
		}
		item = item->next;
	}
	str_array[i] = NULL;

	retval = g_strjoinv (option_delimiter, str_array);

	for (i=0; i<count; i++)
	{
		g_free (str_array[i]);
	}
	g_free (str_array);

	return retval;
}

DesignerAssociationsOption *designer_associations_option_new (void)
{
	return (DesignerAssociationsOption *) g_new0 (DesignerAssociationsOption, 1);
}

void
designer_associations_option_free (DesignerAssociationsOption *self)
{
	g_free (self->name);
	g_free (self->value);
}

/* AssociationsFilename */

GFile *
associations_file_from_xml (xmlDocPtr xml_doc, xmlNodePtr node,
                            GFile *project_root, GError **error)
{
	xmlChar *value;
	xmlNodePtr filename_node;
	gboolean is_relative = FALSE;
	GFile *retval;

	filename_node = search_child (node, DA_XML_TAG_FILENAME);
	if (!filename_node)
	{
		g_set_error (error,
		             DESIGNER_ASSOCIATIONS_ERROR,
		             DESIGNER_ASSOCIATIONS_ERROR_LOADING,
		             _("no filename found in the node %s"), node->name);
		return NULL;
	}

	value = xmlGetProp (filename_node, BAD_CAST (DA_XML_PROP_FILENAME_IS_RELATIVE));
	if (value)
	{
		gint i;

		if (!xmlStrcmp (value, BAD_CAST (DA_XML_PROP_VALUE_TRUE)))
			is_relative = TRUE;
		else if (!xmlStrcmp (value, BAD_CAST (DA_XML_PROP_VALUE_FALSE)))
			is_relative = FALSE;
		else
		{
			i = g_ascii_strtoll ((gchar *)value, NULL, 10);
			if (errno != 0)
			{
				g_set_error (error,
				             DESIGNER_ASSOCIATIONS_ERROR,
				             DESIGNER_ASSOCIATIONS_ERROR_LOADING,
				             _("invalid %s property value"),
				             DA_XML_PROP_FILENAME_IS_RELATIVE);
				xmlFree (value);
				return NULL;
			}

			is_relative = i ? TRUE : FALSE;
		}

		xmlFree (value);
	}

	value = xmlGetProp (filename_node, BAD_CAST (DA_XML_PROP_FILENAME_PATH));
	if (!value)
	{
		g_set_error (error,
		             DESIGNER_ASSOCIATIONS_ERROR,
		             DESIGNER_ASSOCIATIONS_ERROR_LOADING,
		             _("association item filename has no path"));

		return NULL;
	}
	if (is_relative)
		retval = g_file_resolve_relative_path (project_root, (gchar*)value);
	else
		retval = g_file_new_for_uri ((gchar*)value);
	xmlFree (value);

	return retval;
}

void
associations_file_to_xml (GFile *self,
                          xmlDocPtr xml_doc, xmlNodePtr node,
                          GFile *project_root)
{
	xmlNodePtr filename_node;
	gchar *path;

	path = g_file_get_relative_path (project_root, self);

	filename_node = xmlNewDocNode (xml_doc, NULL, BAD_CAST (DA_XML_TAG_FILENAME), NULL);
	xmlAddChild (node, filename_node);

	xmlSetProp (filename_node, BAD_CAST (DA_XML_PROP_FILENAME_IS_RELATIVE),
	            path ? BAD_CAST (DA_XML_PROP_VALUE_TRUE) : BAD_CAST (DA_XML_PROP_VALUE_FALSE));
	if (!path)
		path = g_file_get_uri (self);

	xmlSetProp (filename_node, BAD_CAST (DA_XML_PROP_FILENAME_PATH),
	            path ? BAD_CAST (path) :  BAD_CAST (""));
}

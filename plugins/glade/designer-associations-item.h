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

#ifndef _DESIGNER_ASSOCIATIONS_ITEM_H_
#define _DESIGNER_ASSOCIATIONS_ITEM_H_

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <libxml/tree.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define DESIGNER_TYPE_ASSOCIATIONS_ITEM   	(designer_associations_item_get_type ())
#define DESIGNER_ASSOCIATIONS_ITEM(obj)   	(G_TYPE_CHECK_INSTANCE_CAST ((obj), DESIGNER_TYPE_ASSOCIATIONS_ITEM, DesignerAssociationsItem))
#define DESIGNER_ASSOCIATIONS_ITEM_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), DESIGNER_TYPE_ASSOCIATIONS_ITEM, DesignerAssociationsItemClass))
#define DESIGNER_IS_ASSOCIATIONS_ITEM(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), DESIGNER_TYPE_ASSOCIATIONS_ITEM))
#define DESIGNER_IS_ASSOCIATIONS_ITEM_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), DESIGNER_TYPE_ASSOCIATIONS_ITEM))
#define DESIGNER_ASSOCIATIONS_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DESIGNER_TYPE_ASSOCIATIONS_ITEM, DesignerAssociationsItemClass))

typedef struct _DesignerAssociationsItemClass DesignerAssociationsItemClass;
typedef struct _DesignerAssociationsItem DesignerAssociationsItem;

typedef struct _DesignerAssociationsOption DesignerAssociationsOption;
typedef struct _AssociationsFilename AssociationsFilename;

struct _DesignerAssociationsItemClass
{
	GObjectClass parent_class;
};

struct _AssociationsFilename
{
	gboolean is_relative;
	gchar *relative_path;
	GFile *file;
};

struct _DesignerAssociationsItem
{
	GObject parent_instance;
	guint id;
	GFile *designer;
	gchar *widget_name;
	GFile *editor;
	GList *options;
};

struct _DesignerAssociationsOption
{
	gchar *name;
	gchar *value;
};

#define DA_XML_TAG_ROOT "associations"
#define DA_XML_TAG_ITEM "item"
#define DA_XML_TAG_DESIGNER "designer"
#define DA_XML_TAG_WIDGET "widget"
#define DA_XML_PROP_WIDGET "name"
#define DA_XML_TAG_EDITOR "editor"
#define DA_XML_TAG_OPTION "option"
#define DA_XML_PROP_OPTION_NAME "name"
#define DA_XML_PROP_OPTION_VALUE "value"
#define DA_XML_TAG_FILENAME "filename"
#define DA_XML_PROP_FILENAME_IS_RELATIVE "is_relative"
#define DA_XML_PROP_FILENAME_PATH "path"
#define DA_XML_PROP_VALUE_TRUE "true"
#define DA_XML_PROP_VALUE_FALSE "false"

/* AnjutaDesignerAssociationItem */

GType designer_associations_item_get_type (void) G_GNUC_CONST;

DesignerAssociationsItem *designer_associations_item_new (void);

DesignerAssociationsItem *
designer_associations_item_from_data (GFile *editor, const gchar *widget_name,
                                      GFile *designer, GList *options,
                                      GFile *project_root);
DesignerAssociationsItem *
designer_associations_item_from_xml (DesignerAssociationsItem *self,
                                     xmlDocPtr xml_doc, xmlNodePtr node,
                                     GFile *project_root,
                                     GError **error);
void
designer_associations_item_to_xml (DesignerAssociationsItem *self,
                                   xmlDocPtr xml_doc, xmlNodePtr node,
                                   GFile *project_root);
gchar *
designer_associations_item_get_option (DesignerAssociationsItem *self,
                                       const gchar *name);
gint
designer_associations_item_get_option_as_int (DesignerAssociationsItem *self,
                                              const gchar *name,
                                              const gchar **descriptions);
GList *
designer_associations_item_get_option_node (DesignerAssociationsItem *self,
                                            const gchar *name);
void
designer_associations_item_set_option (DesignerAssociationsItem *self,
                                       const gchar *name, const gchar *value);
void
designer_associations_item_set_widget_name (DesignerAssociationsItem *self,
                                            const gchar *value);
void
designer_associations_item_free (DesignerAssociationsItem *self);

/* DesignerAssociationsOption */

DesignerAssociationsOption *designer_associations_option_new (void);

void designer_associations_option_free (DesignerAssociationsOption *self);

DesignerAssociationsOption *
designer_associations_option_from_xml (DesignerAssociationsOption *self,
                                       xmlDocPtr xml_doc, xmlNodePtr node,
                                       GError **error);
void
designer_associations_options_to_xml (GList *options,
                                      xmlDocPtr xml_doc, xmlNodePtr node);
gchar *
designer_associations_options_to_string (GList *options, gchar *value_delimiter,
                                         gchar *option_delimiter);

/* AssociationsFilename */

GFile *
associations_file_from_xml (xmlDocPtr xml_doc, xmlNodePtr node,
                            GFile *project_root,
                            GError **error);
void
associations_file_to_xml (GFile *self,
                          xmlDocPtr xml_doc, xmlNodePtr node,
                          GFile *project_root);


xmlNodePtr
search_child (xmlNodePtr node, const gchar *name);

G_END_DECLS

#endif /* _DESIGNER_ASSOCIATIONS_ITEM_H_ */

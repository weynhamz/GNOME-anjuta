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
 
#ifndef _XMLTEMPLATE_H
#define _XMLTEMPLATE_H

#include <libxml/parser.h>
#include <gnome.h>

#include "text-template.h"
#include "lib-template.h"
#include "lang-template.h"
#include "prj-template.h"

#define XML_TEMPLATE_TYPE (xml_template_get_type ())
#define XML_TEMPLATE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TEMPLATE_TYPE, XmlTemplate))
#define XML_TEMPLATE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XML_TEMPLATE_TYPE, XmlTemplateClass))
#define XML_IS_TEMPLATE(obj) (G_TYPE_CHECK_TYPE ((obj), XML_TEMPLATE_TYPE))
#define XML_IS_TEMPLATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TEMPLATE_TYPE))
#define XML_TEMPLATE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XML_TEMPLATE_TYPE, XmlTemplateClass))
	
typedef struct _XmlTemplate XmlTemplate;
typedef struct _XmlTemplateClass XmlTemplateClass;
typedef struct _XmlTemplatePrivate XmlTemplatePrivate;

struct _XmlTemplate
{
	GObject parent;
	
	XmlTemplatePrivate* priv;
};

struct _XmlTemplateClass
{
	GObjectClass parent;
};

GType xml_template_get_type();
XmlTemplate* xml_template_new();

GList* xml_template_get_all_projects(XmlTemplate* xtmp);
PrjTemplate* xml_template_get_project(XmlTemplate* xtmp, const gchar* name);
GList* xml_template_query_projects_by_language(XmlTemplate* xtmp, 
	const gchar* language);

GList* xml_template_get_all_libs(XmlTemplate* xtmp);
LibTemplate* xml_template_get_lib(XmlTemplate* xtmp, const gchar* name);
GList* xml_template_query_libs_by_language(XmlTemplate* xtmp, 
	const gchar* language);

GList* xml_template_get_all_languages(XmlTemplate* xtmp);
LangTemplate* xml_template_get_language(XmlTemplate* xtmp, const gchar* name);

GList* xml_template_get_all_texts(XmlTemplate* xtmp);
TextTemplate* xml_template_get_text(XmlTemplate* xtmp, const gchar* name);
GList* xml_template_query_texts_by_language(XmlTemplate* xtmp, 
	const gchar* language);


#endif /* _XMLTEMPLATE_H */

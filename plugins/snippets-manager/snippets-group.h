/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-group.h
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

#ifndef __SNIPPETS_GROUP_H__
#define __SNIPPETS_GROUP_H__

#include <glib.h>
#include <glib-object.h>
#include "snippet.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_SNIPPETS_GROUP            (snippets_group_get_type ())
#define ANJUTA_SNIPPETS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPETS_GROUP, AnjutaSnippetsGroup))
#define ANJUTA_SNIPPETS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPETS_GROUP, AnjutaSnippetsGroupClass))
#define ANJUTA_IS_SNIPPETS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPETS_GROUP))
#define ANJUTA_IS_SNIPPETS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPETS_GROUP))

typedef struct _AnjutaSnippetsGroup AnjutaSnippetsGroup;
typedef struct _AnjutaSnippetsGroupPrivate AnjutaSnippetsGroupPrivate;
typedef struct _AnjutaSnippetsGroupClass AnjutaSnippetsGroupClass;

struct _AnjutaSnippetsGroup
{
	GObject parent_instance;
	
	/*< private >*/
	AnjutaSnippetsGroupPrivate* priv;
};

struct _AnjutaSnippetsGroupClass
{
	GObjectClass parent_class;

};

GType                 snippets_group_get_type          (void) G_GNUC_CONST;
AnjutaSnippetsGroup*  snippets_group_new               (const gchar* snippets_group_name);
const gchar*          snippets_group_get_name          (AnjutaSnippetsGroup* snippets_group);
void                  snippets_group_set_name          (AnjutaSnippetsGroup* snippets_group,
                                                        const gchar* new_group_name);
gboolean              snippets_group_add_snippet       (AnjutaSnippetsGroup* snippets_group,
                                                        AnjutaSnippet* snippet);
void                  snippets_group_remove_snippet    (AnjutaSnippetsGroup* snippets_group,
                                                        const gchar* trigger_key,
                                                        const gchar* language,
                                                        gboolean remove_all_languages_support);
gboolean              snippets_group_has_snippet       (AnjutaSnippetsGroup *snippets_group,
                                                        AnjutaSnippet *snippet);
GList*                snippets_group_get_snippets_list (AnjutaSnippetsGroup* snippets_group);

G_END_DECLS

#endif /* __SNIPPETS_GROUP_H__ */


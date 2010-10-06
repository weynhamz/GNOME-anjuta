/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet.h
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

#ifndef __ANJUTA_SNIPPET_H__
#define __ANJUTA_SNIPPET_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _AnjutaSnippet AnjutaSnippet;
typedef struct _AnjutaSnippetPrivate AnjutaSnippetPrivate;
typedef struct _AnjutaSnippetClass AnjutaSnippetClass;

#define ANJUTA_TYPE_SNIPPET            (snippet_get_type ())
#define ANJUTA_SNIPPET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPET, AnjutaSnippet))
#define ANJUTA_SNIPPET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPET, AnjutaSnippetClass))
#define ANJUTA_IS_SNIPPET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPET))
#define ANJUTA_IS_SNIPPET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPET))

struct _AnjutaSnippet
{
	GObject parent_instance;

	/* A pointer to an AnjutaSnippetsGroup object. */
	GObject *parent_snippets_group;
	
	/*< private >*/
	AnjutaSnippetPrivate *priv;
};

struct _AnjutaSnippetClass
{
	GObjectClass parent_class;

};

GType           snippet_get_type                        (void) G_GNUC_CONST;
AnjutaSnippet*  snippet_new                             (const gchar *trigger_key,
                                                         GList *snippet_language,
                                                         const gchar *snippet_name,
                                                         const gchar *snippet_content,
                                                         GList *variable_names,
                                                         GList *variable_default_values,
                                                         GList *variable_globals,
                                                         GList *keywords);
AnjutaSnippet*  snippet_copy                            (AnjutaSnippet *snippet);
const gchar*    snippet_get_trigger_key                 (AnjutaSnippet *snippet);
void            snippet_set_trigger_key                 (AnjutaSnippet *snippet,
                                                         const gchar *new_trigger_key);
const GList*    snippet_get_languages                   (AnjutaSnippet *snippet);
gchar*          snippet_get_languages_string            (AnjutaSnippet *snippet);
const gchar*    snippet_get_any_language                (AnjutaSnippet *snippet);
gboolean        snippet_has_language                    (AnjutaSnippet *snippet,
                                                         const gchar *language);
void            snippet_add_language                    (AnjutaSnippet *snippet,
                                                         const gchar *language);
void            snippet_remove_language                 (AnjutaSnippet *snippet,
                                                         const gchar *language);
const gchar*    snippet_get_name                        (AnjutaSnippet *snippet);
void            snippet_set_name                        (AnjutaSnippet *snippet,
                                                         const gchar *new_name);
GList*          snippet_get_keywords_list               (AnjutaSnippet *snippet);
void            snippet_set_keywords_list               (AnjutaSnippet *snippet,
                                                         const GList *keywords_list);
GList*          snippet_get_variable_names_list         (AnjutaSnippet *snippet);
GList*          snippet_get_variable_defaults_list      (AnjutaSnippet *snippet);
GList*          snippet_get_variable_globals_list       (AnjutaSnippet *snippet);
gboolean        snippet_has_variable                    (AnjutaSnippet *snippet,
                                                         const gchar *variable_name);
void            snippet_add_variable                    (AnjutaSnippet *snippet,
                                                         const gchar *variable_name,
                                                         const gchar *default_value,
                                                         gboolean is_global);
void            snippet_remove_variable                 (AnjutaSnippet *snippet,
                                                         const gchar *variable_name);
void            snippet_set_variable_name               (AnjutaSnippet *snippet,
                                                         const gchar *variable_name,
                                                         const gchar *new_variable_name);
const gchar*    snippet_get_variable_default_value      (AnjutaSnippet *snippet,
                                                         const gchar *variable_name);
void            snippet_set_variable_default_value      (AnjutaSnippet *snippet,
                                                         const gchar *variable_name,
                                                         const gchar *default_value);
gboolean        snippet_get_variable_global             (AnjutaSnippet *snippet,
                                                         const gchar *variable_name);
void            snippet_set_variable_global             (AnjutaSnippet *snippet,
                                                         const gchar *variable_name,
                                                         gboolean global);
const gchar*    snippet_get_content                     (AnjutaSnippet *snippet);
void            snippet_set_content                     (AnjutaSnippet *snippet,
                                                         const gchar *new_content);
gchar*          snippet_get_default_content             (AnjutaSnippet *snippet,
                                                         GObject *snippets_db_obj,
                                                         const gchar *indent);
GList*          snippet_get_variable_relative_positions (AnjutaSnippet *snippet);
GList*          snippet_get_variable_cur_values_len     (AnjutaSnippet *snippet);
gint            snippet_get_cur_value_end_position      (AnjutaSnippet *snippet);
gboolean        snippet_is_equal                        (AnjutaSnippet *snippet,
                                                         AnjutaSnippet *snippet2);

G_END_DECLS

#endif /* __ANJUTA_SNIPPET_H__ */

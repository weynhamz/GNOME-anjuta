/* AnjutaPluginDescription - Plugin meta data
 * anjuta-plugin-description.h Copyright (C) 2002 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef ANJUTA_PLUGIN_PARSER_H
#define ANJUTA_PLUGIN_PARSER_H

#include <glib.h>

typedef struct _AnjutaPluginDescription AnjutaPluginDescription;

typedef void (*AnjutaPluginDescriptionSectionFunc) (AnjutaPluginDescription *df,
													const gchar *name,
													gpointer user_data);

/* If @key is %NULL, @value is a comment line. */
/* @value is raw, unescaped data. */
typedef void (*AnjutaPluginDescriptionLineFunc) (AnjutaPluginDescription *df,
												 const gchar *key,
												 const gchar *locale,
												 const gchar *value,
												 gpointer   data);

typedef enum
{
  ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_SYNTAX,
  ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_ESCAPES,
  ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_CHARS
} AnjutaPluginDescriptionParseError;

#define ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR \
			anjuta_plugin_description_parse_error_quark()

GQuark anjuta_plugin_description_parse_error_quark (void);


AnjutaPluginDescription* anjuta_plugin_description_new (const gchar *filename,
														GError **error);

AnjutaPluginDescription* anjuta_plugin_description_new_from_string (gchar *data,
																	GError **error);

gchar* anjuta_plugin_description_to_string (AnjutaPluginDescription *pd);

void anjuta_plugin_description_free (AnjutaPluginDescription *pd);

void anjuta_plugin_description_foreach_section (AnjutaPluginDescription *pd,
												AnjutaPluginDescriptionSectionFunc func,
												gpointer user_data);

void anjuta_plugin_description_foreach_key (AnjutaPluginDescription *dp,
											const gchar *section,
											gboolean include_localized,
											AnjutaPluginDescriptionLineFunc func,
											gpointer user_data);

/* Gets the raw text of the key, unescaped */
gboolean anjuta_plugin_description_get_raw (AnjutaPluginDescription *dp,
										    const gchar *section,
											const gchar *keyname,
											const gchar *locale,
											gchar **val);

gboolean anjuta_plugin_description_get_integer (AnjutaPluginDescription *dp,
												const gchar *section,
												const gchar *keyname,
												gint *val);

gboolean anjuta_plugin_description_get_string (AnjutaPluginDescription   *dp,
											   const gchar *section,
											   const gchar *keyname,
											   gchar **val);

gboolean anjuta_plugin_description_get_locale_string (AnjutaPluginDescription *dp,
													  const gchar *section,
													  const gchar *keyname,
													  gchar **val);
#endif /* ANJUTA_PLUGIN_PARSER_H */

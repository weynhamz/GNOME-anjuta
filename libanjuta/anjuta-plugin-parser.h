/* AnjutaPluginParser - a parser of plugin files
 * anjuta-plugin-parser.h Copyright (C) 2002 Red Hat, Inc.
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

typedef struct _AnjutaPluginFile AnjutaPluginFile;

typedef void (* AnjutaPluginFileSectionFunc) (AnjutaPluginFile *df,
					    const char       *name,
					    gpointer          data);

/* If @key is %NULL, @value is a comment line. */
/* @value is raw, unescaped data. */
typedef void (* AnjutaPluginFileLineFunc) (AnjutaPluginFile *df,
					 const char       *key,
					 const char       *locale,
					 const char       *value,
					 gpointer          data);

typedef enum 
{
  ANJUTA_PLUGIN_FILE_PARSE_ERROR_INVALID_SYNTAX,
  ANJUTA_PLUGIN_FILE_PARSE_ERROR_INVALID_ESCAPES,
  ANJUTA_PLUGIN_FILE_PARSE_ERROR_INVALID_CHARS
} AnjutaPluginFileParseError;

#define ANJUTA_PLUGIN_FILE_PARSE_ERROR anjuta_plugin_file_parse_error_quark()
GQuark anjuta_plugin_file_parse_error_quark (void);

AnjutaPluginFile *anjuta_plugin_file_new_from_string (char                    *data,
						  GError                 **error);
char *          anjuta_plugin_file_to_string       (AnjutaPluginFile          *df);
void            anjuta_plugin_file_free            (AnjutaPluginFile          *df);


void                   anjuta_plugin_file_foreach_section (AnjutaPluginFile            *df,
							 AnjutaPluginFileSectionFunc  func,
							 gpointer                   user_data);
void                   anjuta_plugin_file_foreach_key     (AnjutaPluginFile            *df,
							 const char                *section,
							 gboolean                   include_localized,
							 AnjutaPluginFileLineFunc     func,
							 gpointer                   user_data);


/* Gets the raw text of the key, unescaped */
gboolean anjuta_plugin_file_get_raw            (AnjutaPluginFile   *df,
					      const char       *section,
					      const char       *keyname,
					      const char       *locale,
					      char            **val);
gboolean anjuta_plugin_file_get_integer        (AnjutaPluginFile   *df,
					      const char       *section,
					      const char       *keyname,
					      int              *val);
gboolean anjuta_plugin_file_get_string         (AnjutaPluginFile   *df,
					      const char       *section,
					      const char       *keyname,
					      char            **val);
gboolean anjuta_plugin_file_get_locale_string  (AnjutaPluginFile   *df,
					      const char       *section,
					      const char       *keyname,
					      char            **val);

#endif /* ANJUTA_PLUGIN_PARSER_H */

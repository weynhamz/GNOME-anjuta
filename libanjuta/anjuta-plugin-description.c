/*
 * AnjutaPluginDescription - Plugin meta data
 * anjuta-plugin-description.c Copyright (C) 2002 Red Hat, Inc.
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:anjuta-plugin-description
 * @title: AnjutaPluginDescription
 * @short_description: Plugin description from .plugin file
 * @see_also: #AnjutaPlugin, #AnjutaPluginHandle
 * @stability: Unstable
 * @include: libanjuta/anjuta-plugin-description.h
 * 
 */

#include <string.h>
#include <locale.h>
#include <stdlib.h>

#include <libanjuta/anjuta-plugin-description.h>

typedef struct _AnjutaPluginDescriptionSection AnjutaPluginDescriptionSection;
typedef struct _AnjutaPluginDescriptionLine AnjutaPluginDescriptionLine;
typedef struct _AnjutaPluginDescriptionParser AnjutaPluginDescriptionParser;

struct _AnjutaPluginDescriptionSection {
  GQuark section_name; /* 0 means just a comment block (before any section) */
  gint n_lines;
  AnjutaPluginDescriptionLine *lines;
};

struct _AnjutaPluginDescriptionLine {
  GQuark key; /* 0 means comment or blank line in value */
  char *locale;
  gchar *value;
};

struct _AnjutaPluginDescription {
  gint n_sections;
  AnjutaPluginDescriptionSection *sections;
  char *current_locale[2];
};

struct _AnjutaPluginDescriptionParser {
  AnjutaPluginDescription *df;
  gint current_section;
  gint n_allocated_lines;
  gint n_allocated_sections;
  gint line_nr;
  char *line;
};

#define VALID_KEY_CHAR 1
#define VALID_LOCALE_CHAR 2
guchar valid[256] = { 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x3 , 0x2 , 0x0 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x2 , 
   0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
};

static void report_error (AnjutaPluginDescriptionParser *parser,
						  char *message,
						  AnjutaPluginDescriptionParseError    error_code,
						  GError                  **error);

static AnjutaPluginDescriptionSection *lookup_section (AnjutaPluginDescription *df,
					      const char *section);

static AnjutaPluginDescriptionLine *lookup_line (AnjutaPluginDescription *df,
												 AnjutaPluginDescriptionSection *section,
												 const char *keyname,
												 const char *locale);

GQuark
anjuta_plugin_description_parse_error_quark (void)
{
  static GQuark quark;
  if (!quark)
    quark = g_quark_from_static_string ("g_desktop_parse_error");

  return quark;
}

static void
parser_free (AnjutaPluginDescriptionParser *parser)
{
  anjuta_plugin_description_free (parser->df);
}

static void
anjuta_plugin_description_line_free (AnjutaPluginDescriptionLine *line)
{
  g_free (line->locale);
  g_free (line->value);
}

static void
anjuta_plugin_description_section_free (AnjutaPluginDescriptionSection *section)
{
  int i;

  for (i = 0; i < section->n_lines; i++)
    anjuta_plugin_description_line_free (&section->lines[i]);
  
  g_free (section->lines);
}

/**
 * anjuta_plugin_description_free:
 * @df: an #AnjutaPluginDescription object
 *
 * Frees the #AnjutaPluginDescription instance.
 */
void
anjuta_plugin_description_free (AnjutaPluginDescription *df)
{
  int i;

  for (i = 0; i < df->n_sections; i++)
    anjuta_plugin_description_section_free (&df->sections[i]);
  g_free (df->sections);
  g_free (df->current_locale[0]);
  g_free (df->current_locale[1]);

  g_free (df);
}

static void
grow_lines (AnjutaPluginDescriptionParser *parser)
{
  int new_n_lines;
  AnjutaPluginDescriptionSection *section;

  if (parser->n_allocated_lines == 0)
    new_n_lines = 1;
  else
    new_n_lines = parser->n_allocated_lines*2;

  section = &parser->df->sections[parser->current_section];

  section->lines = g_realloc (section->lines,
			      sizeof (AnjutaPluginDescriptionLine) * new_n_lines);
  parser->n_allocated_lines = new_n_lines;
}

static void
grow_sections (AnjutaPluginDescriptionParser *parser)
{
  int new_n_sections;

  if (parser->n_allocated_sections == 0)
    new_n_sections = 1;
  else
    new_n_sections = parser->n_allocated_sections*2;

  parser->df->sections = g_realloc (parser->df->sections,
				    sizeof (AnjutaPluginDescriptionSection) * new_n_sections);
  parser->n_allocated_sections = new_n_sections;
}

static gchar *
unescape_string (gchar *str, gint len)
{
  gchar *res;
  gchar *p, *q;
  gchar *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, len + 1);
  p = str;
  q = res;
  end = str + len;

  while (p < end)
    {
      if (*p == 0)
	{
	  /* Found an embedded null */
	  g_free (res);
	  return NULL;
	}
      if (*p == '\\')
	{
	  p++;
	  if (p >= end)
	    {
	      /* Escape at end of string */
	      g_free (res);
	      return NULL;
	    }
	  
	  switch (*p)
	    {
	    case 's':
              *q++ = ' ';
              break;
           case 't':
              *q++ = '\t';
              break;
           case 'n':
              *q++ = '\n';
              break;
           case 'r':
              *q++ = '\r';
              break;
           case '\\':
              *q++ = '\\';
              break;
           default:
	     /* Invalid escape code */
	     g_free (res);
	     return NULL;
	    }
	  p++;
	}
      else
	*q++ = *p++;
    }
  *q = 0;

  return res;
}

static gchar *
escape_string (const gchar *str, gboolean escape_first_space)
{
  gchar *res;
  char *q;
  const gchar *p;
  const gchar *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, strlen (str)*2 + 1);
  
  p = str;
  q = res;
  end = str + strlen (str);

  while (*p)
    {
      if (*p == ' ')
	{
	  if (escape_first_space && p == str)
	    {
	      *q++ = '\\';
	      *q++ = 's';
	    }
	  else
	    *q++ = ' ';
	}
      else if (*p == '\\')
	{
	  *q++ = '\\';
	  *q++ = '\\';
	}
      else if (*p == '\t')
	{
	  *q++ = '\\';
	  *q++ = 't';
	}
      else if (*p == '\n')
	{
	  *q++ = '\\';
	  *q++ = 'n';
	}
      else if (*p == '\r')
	{
	  *q++ = '\\';
	  *q++ = 'r';
	}
      else
	*q++ = *p;
      p++;
    }
  *q = 0;

  return res;
}


static void 
open_section (AnjutaPluginDescriptionParser *parser,
	      const char           *name)
{
  int n;
  
  if (parser->n_allocated_sections == parser->df->n_sections)
    grow_sections (parser);

  if (parser->current_section == 0 &&
      parser->df->sections[0].section_name == 0 &&
      parser->df->sections[0].n_lines == 0)
    {
      if (!name)
	g_warning ("non-initial NULL section\n");
      
      /* The initial section was empty. Piggyback on it. */
      parser->df->sections[0].section_name = g_quark_from_string (name);

      return;
    }
  
  n = parser->df->n_sections++;

  if (name)
    parser->df->sections[n].section_name = g_quark_from_string (name);
  else
    parser->df->sections[n].section_name = 0;
  parser->df->sections[n].n_lines = 0;
  parser->df->sections[n].lines = NULL;

  parser->current_section = n;
  parser->n_allocated_lines = 0;
  grow_lines (parser);
}

static AnjutaPluginDescriptionLine *
new_line (AnjutaPluginDescriptionParser *parser)
{
  AnjutaPluginDescriptionSection *section;
  AnjutaPluginDescriptionLine *line;

  section = &parser->df->sections[parser->current_section];
  
  if (parser->n_allocated_lines == section->n_lines)
    grow_lines (parser);

  line = &section->lines[section->n_lines++];

  memset (line, 0, sizeof (AnjutaPluginDescriptionLine));
  
  return line;
}

static gboolean
is_blank_line (AnjutaPluginDescriptionParser *parser)
{
  gchar *p;

  p = parser->line;

  while (*p && *p != '\n')
    {
      if (!g_ascii_isspace (*p))
	return FALSE;

      p++;
    }
  return TRUE;
}

static void
parse_comment_or_blank (AnjutaPluginDescriptionParser *parser)
{
  AnjutaPluginDescriptionLine *line;
  gchar *line_end;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  line = new_line (parser);
  
  line->value = g_strndup (parser->line, line_end - parser->line);

  parser->line = (line_end) ? line_end + 1 : NULL;
  parser->line_nr++;
}

static gboolean
parse_section_start (AnjutaPluginDescriptionParser *parser, GError **error)
{
  gchar *line_end;
  gchar *section_name;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  if (line_end - parser->line <= 2 ||
      line_end[-1] != ']')
    {
      report_error (parser, "Invalid syntax for section header", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  section_name = unescape_string (parser->line + 1, line_end - parser->line - 2);

  if (section_name == NULL)
    {
      report_error (parser, "Invalid escaping in section name", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_ESCAPES, error);
      parser_free (parser);
      return FALSE;
    }

  open_section (parser, section_name);
  
  parser->line = (line_end) ? line_end + 1 : NULL;
  parser->line_nr++;

  g_free (section_name);
  
  return TRUE;
}

static gboolean
parse_key_value (AnjutaPluginDescriptionParser *parser, GError **error)
{
  AnjutaPluginDescriptionLine *line;
  gchar *line_end;
  gchar *key_start;
  gchar *key_end;
  gchar *key;
  gchar *locale_start = NULL;
  gchar *locale_end = NULL;
  gchar *value_start;
  gchar *value;
  gchar *p;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  p = parser->line;
  key_start = p;
  while (p < line_end &&
	 (valid[(guchar)*p] & VALID_KEY_CHAR)) 
    p++;
  key_end = p;

  if (key_start == key_end)
    {
      report_error (parser, "Empty key name", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  if (p < line_end && *p == '[')
    {
      p++;
      locale_start = p;
      while (p < line_end && *p != ']')
	p++;
      locale_end = p;

      if (p == line_end)
	{
	  report_error (parser, "Unterminated locale specification in key", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_SYNTAX, error);
	  parser_free (parser);
	  return FALSE;
	}
      
      p++;
    }
  
  /* Skip space before '=' */
  while (p < line_end && *p == ' ')
    p++;

  if (p < line_end && *p != '=')
    {
      report_error (parser, "Invalid characters in key name", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_CHARS, error);
      parser_free (parser);
      return FALSE;
    }

  if (p == line_end)
    {
      report_error (parser, "No '=' in key/value pair", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  /* Skip the '=' */
  p++;

  /* Skip space after '=' */
  while (p < line_end && *p == ' ')
    p++;

  value_start = p;

  value = unescape_string (value_start, line_end - value_start);
  if (value == NULL)
    {
      report_error (parser, "Invalid escaping in value", ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR_INVALID_ESCAPES, error);
      parser_free (parser);
      return FALSE;
    }

  line = new_line (parser);
  key = g_strndup (key_start, key_end - key_start);
  line->key = g_quark_from_string (key);
  g_free (key);
  if (locale_start)
    line->locale = g_strndup (locale_start, locale_end - locale_start);
  line->value = value;
  
  parser->line = (*line_end) ? line_end + 1 : NULL;
  parser->line_nr++;
  
  return TRUE;
}


static void
report_error (AnjutaPluginDescriptionParser *parser,
	      char                   *message,
	      AnjutaPluginDescriptionParseError  error_code,
	      GError                **error)
{
  AnjutaPluginDescriptionSection *section;
  const gchar *section_name = NULL;

  section = &parser->df->sections[parser->current_section];

  if (section->section_name)
    section_name = g_quark_to_string (section->section_name);
  
  if (error)
    {
      if (section_name)
	*error = g_error_new (ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR,
			      error_code,
			      "Error in section %s at line %d: %s", section_name, parser->line_nr, message);
      else
	*error = g_error_new (ANJUTA_PLUGIN_DESCRIPTION_PARSE_ERROR,
			      error_code,
			      "Error at line %d: %s", parser->line_nr, message);
    }
}

/**
 * anjuta_plugin_description_new_from_string:
 * @data: The data to parse. The format of the data is .ini style.
 *
 * Parses the given plugin description data (usally read from the plugin
 * description file and creates an instance of #AnjutaPluginDescription.
 * The format of the content string is similar to .ini format.
 *
 * Return value: a new #AnjutaPluginDescription object
 */
AnjutaPluginDescription *
anjuta_plugin_description_new_from_string (char *data, GError **error)
{
  AnjutaPluginDescriptionParser parser;

  parser.df = g_new0 (AnjutaPluginDescription, 1);
  parser.current_section = -1;

  parser.n_allocated_lines = 0;
  parser.n_allocated_sections = 0;
  parser.line_nr = 1;

  parser.line = data;

  /* Put any initial comments in a NULL segment */
  open_section (&parser, NULL);
  
  while (parser.line && *parser.line)
    {
      if (*parser.line == '[') {
	if (!parse_section_start (&parser, error))
	  return NULL;
      } else if (is_blank_line (&parser) ||
		 *parser.line == '#')
	parse_comment_or_blank (&parser);
      else
	{
	  if (!parse_key_value (&parser, error))
	    return NULL;
	}
    }

  return parser.df;
}

/**
 * anjuta_plugin_description_to_string:
 * @df: an #AnjutaPluginDescription object.
 *
 * Converts the description detains into string format, usually for
 * saving it in a file.
 *
 * Return value: The string representation of the description. The
 * returned values must be freed after use.
 */
char *
anjuta_plugin_description_to_string (AnjutaPluginDescription *df)
{
  AnjutaPluginDescriptionSection *section;
  AnjutaPluginDescriptionLine *line;
  GString *str;
  char *s;
  int i, j;
  
  str = g_string_sized_new (800);

  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      if (section->section_name)
	{
	  g_string_append_c (str, '[');
	  s = escape_string (g_quark_to_string (section->section_name), FALSE);
	  g_string_append (str, s);
	  g_free (s);
	  g_string_append (str, "]\n");
	}
      
      for (j = 0; j < section->n_lines; j++)
	{
	  line = &section->lines[j];
	  
	  if (line->key == 0)
	    {
	      g_string_append (str, line->value);
	      g_string_append_c (str, '\n');
	    }
	  else
	    {
	      g_string_append (str, g_quark_to_string (line->key));
	      if (line->locale)
		{
		  g_string_append_c (str, '[');
		  g_string_append (str, line->locale);
		  g_string_append_c (str, ']');
		}
	      g_string_append_c (str, '=');
	      s = escape_string (line->value, TRUE);
	      g_string_append (str, s);
	      g_free (s);
	      g_string_append_c (str, '\n');
	    }
	}
    }
  
  return g_string_free (str, FALSE);
}

static AnjutaPluginDescriptionSection *
lookup_section (AnjutaPluginDescription  *df,
		const char        *section_name)
{
  AnjutaPluginDescriptionSection *section;
  GQuark section_quark;
  int i;

  section_quark = g_quark_try_string (section_name);
  if (section_quark == 0)
    return NULL;
  
  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      if (section->section_name == section_quark)
	return section;
    }
  return NULL;
}

static AnjutaPluginDescriptionLine *
lookup_line (AnjutaPluginDescription        *df,
	     AnjutaPluginDescriptionSection *section,
	     const char              *keyname,
	     const char              *locale)
{
  AnjutaPluginDescriptionLine *line;
  GQuark key_quark;
  int i;

  key_quark = g_quark_try_string (keyname);
  if (key_quark == 0)
    return NULL;
  
  for (i = 0; i < section->n_lines; i++)
    {
      line = &section->lines[i];
      
      if (line->key == key_quark &&
	  ((locale == NULL && line->locale == NULL) ||
	   (locale != NULL && line->locale != NULL && strcmp (locale, line->locale) == 0)))
	return line;
    }
  
  return NULL;
}

/**
 * anjuta_plugin_description_get_raw:
 * @df: an #AnjutaPluginDescription object.
 * @section_name: Name of the section.
 * @keyname: Name of the key.
 * @locale: The locale for which the value is to be retrieved.
 * @val: Pointer to the variable to store the string value.
 *
 * Retrieves the value of a key (in the given section) for the given locale.
 * The value returned in @val must be freed after use.
 *
 * Return value: TRUE if sucessful, otherwise FALSE.
 */
gboolean
anjuta_plugin_description_get_raw (AnjutaPluginDescription *df,
			    const char    *section_name,
			    const char    *keyname,
			    const char    *locale,
			    char         **val)
{
  AnjutaPluginDescriptionSection *section;
  AnjutaPluginDescriptionLine *line;

  *val = NULL;

  section = lookup_section (df, section_name);
  if (!section)
    return FALSE;

  line = lookup_line (df,
		      section,
		      keyname,
		      locale);

  if (!line)
    return FALSE;
  
  *val = g_strdup (line->value);
  
  return TRUE;
}

/**
 * anjuta_plugin_description_foreach_section:
 * @df: an #AnjutaPluginDescription object.
 * @func: Callback function.
 * @user_data: User data to pass to @func.
 *
 * Calls @func for each of the sections in the description.
 */
void
anjuta_plugin_description_foreach_section (AnjutaPluginDescription *df,
				  AnjutaPluginDescriptionSectionFunc func,
				  gpointer user_data)
{
  AnjutaPluginDescriptionSection *section;
  int i;

  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      (*func) (df, g_quark_to_string (section->section_name),  user_data);
    }
  return;
}

/**
 * anjuta_plugin_description_foreach_key:
 * @df: an #AnjutaPluginDescription object.
 * @section_name: Name of the section.
 * @include_localized: Whether each localized key should be called separately.
 * @func: The callback function.
 * @user_data: User data to pass to @func.
 *
 * Calls @func for each of the keys in the given section. @include_localized,
 * if set to TRUE will make it call @func for the localized keys also, otherwise
 * only one call is made for the key in current locale.
 */
void
anjuta_plugin_description_foreach_key (AnjutaPluginDescription *df,
			      const char                  *section_name,
			      gboolean                     include_localized,
			      AnjutaPluginDescriptionLineFunc     func,
			      gpointer                     user_data)
{
  AnjutaPluginDescriptionSection *section;
  AnjutaPluginDescriptionLine *line;
  int i;

  section = lookup_section (df, section_name);
  if (!section)
    return;
  
  for (i = 0; i < section->n_lines; i++)
    {
      line = &section->lines[i];

      (*func) (df, g_quark_to_string (line->key), line->locale, line->value, user_data);
    }
  
  return;
}


static void
calculate_locale (AnjutaPluginDescription   *df)
{
  char *p, *lang;

  lang = g_strdup (setlocale (LC_MESSAGES, NULL));
  
  if (lang)
    {
      p = strchr (lang, '.');
      if (p)
	*p = '\0';
      p = strchr (lang, '@');
      if (p)
	*p = '\0';
    }
  else
    lang = g_strdup ("C");
  
  p = strchr (lang, '_');
  if (p)
    {
      df->current_locale[0] = g_strdup (lang);
      *p = '\0';
      df->current_locale[1] = lang;
    }
  else
    {
      df->current_locale[0] = lang;
      df->current_locale[1] = NULL;
    }
}

/**
 * anjuta_plugin_description_get_locale_string:
 * @df: an #AnjutaPluginDescription object.
 * @section: Section name.
 * @keyname: Key name.
 * @val: Pointer to value to store retured value.
 * 
 * Returns the value of key in the given section in current locale.
 *
 * Return value: TRUE if sucessful, otherwise FALSE.
 */
gboolean
anjuta_plugin_description_get_locale_string  (AnjutaPluginDescription  *df,
				     const char      *section,
				     const char      *keyname,
				     char           **val)
{
  gboolean res;

  if (df->current_locale[0] == NULL)
    calculate_locale (df);

  if  (df->current_locale[0] != NULL)
    {
      res = anjuta_plugin_description_get_raw (df,section, keyname,
				      df->current_locale[0], val);
      if (res)
	return TRUE;
    }
  
  if  (df->current_locale[1] != NULL)
    {
      res = anjuta_plugin_description_get_raw (df,section, keyname,
				      df->current_locale[1], val);
      if (res)
	return TRUE;
    }
  
  return anjuta_plugin_description_get_raw (df, section, keyname, NULL, val);
}

/**
 * anjuta_plugin_description_get_string:
 * @df: an #AnjutaPluginDescription object.
 * @section: Section name.
 * @keyname: Key name.
 * @val: Pointer to value to store retured value.
 * 
 * Returns the value of key in the given section.
 *
 * Return value: TRUE if sucessful, otherwise FALSE.
 */
gboolean
anjuta_plugin_description_get_string (AnjutaPluginDescription   *df,
			     const char       *section,
			     const char       *keyname,
			     char            **val)
{
  return anjuta_plugin_description_get_raw (df, section, keyname, NULL, val);
}

/**
 * anjuta_plugin_description_get_integer:
 * @df: an #AnjutaPluginDescription object.
 * @section: Section name.
 * @keyname: Key name.
 * @val: Pointer to value to store retured value.
 * 
 * Returns the value of key as integer in the given section.
 *
 * Return value: TRUE if sucessful, otherwise FALSE.
 */
gboolean
anjuta_plugin_description_get_integer (AnjutaPluginDescription   *df,
			      const char       *section,
			      const char       *keyname,
			      int              *val)
{
  gboolean res;
  char *str;
  
  *val = 0;

  res = anjuta_plugin_description_get_raw (df, section, keyname, NULL, &str);
  if (!res)
    return FALSE;

  
  *val = atoi (str);
  g_free (str);
  
  return TRUE;
  
}

/**
 * anjuta_plugin_description_get_boolean:
 * @df: an #AnjutaPluginDescription object.
 * @section: Section name.
 * @keyname: Key name.
 * @val: Pointer to value to store retured value.
 * 
 * Returns the value of key as boolean in the given section.
 *
 * Return value: TRUE if sucessful, otherwise FALSE.
 */
gboolean
anjuta_plugin_description_get_boolean (AnjutaPluginDescription   *df,
			      const char       *section,
			      const char       *keyname,
			      gboolean         *val)
{
  gboolean res;
  char *str;
  
  *val = 0;

  res = anjuta_plugin_description_get_raw (df, section, keyname, NULL, &str);
  if (!res)
    return FALSE;

  if ((g_ascii_strcasecmp (str, "yes") == 0) ||
      (g_ascii_strcasecmp (str, "true") == 0))
  {
    *val = TRUE;
  }
  else if ((g_ascii_strcasecmp (str, "no") == 0) ||
	          (g_ascii_strcasecmp (str, "false") == 0))	  

  {
    *val = FALSE;
  }
  else
  {
     res = FALSE;
  }

  g_free (str);

  return res;
}

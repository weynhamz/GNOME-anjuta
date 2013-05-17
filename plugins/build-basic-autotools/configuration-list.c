/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    configuration-list.c
    Copyright (C) 2008 Sébastien Granjoux

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "configuration-list.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-builder.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <string.h>

/* Constants
 *---------------------------------------------------------------------------*/

/* Type defintions
 *---------------------------------------------------------------------------*/

struct  _BuildConfiguration
{
	gchar *name;
	gchar *build_uri;
	gchar *args;
	GList *env;
	gboolean translate;
	BuildConfiguration *next;
	BuildConfiguration *prev;
};

struct _BuildConfigurationList
{
	BuildConfiguration *cfg;
	gchar *project_root_uri;
	BuildConfiguration *selected;
};

typedef struct _DefaultBuildConfiguration DefaultBuildConfiguration;

struct  _DefaultBuildConfiguration
{
	gchar *name;
	gchar *build_uri;
	gchar *args;
	gchar **env;
};

/* The name value is kept untranslated for saving in the session file or
 * used as an id but a translated value is needed for displaying it.
 * Some predefined names are defined using contants named
 * IANJUTA_BUILDER_CONFIGURATION_*. They cannot be used in the name
 * field as we need a translated value, so they are used in build
 * directory instead */
const DefaultBuildConfiguration default_config[] = {
	{N_("Default"), NULL, "--enable-maintainer-mode", NULL},
	{N_("Debug"), IANJUTA_BUILDER_CONFIGURATION_DEBUG, "--enable-maintainer-mode 'CFLAGS=-g -O0' 'CXXFLAGS=-g -O0' 'JFLAGS=-g -O0' 'FFLAGS=-g -O0'", NULL},
	{N_("Profiling"), IANJUTA_BUILDER_CONFIGURATION_PROFILING, "--enable-maintainer-mode 'CFLAGS=-g -pg' 'CXXFLAGS=-g -pg' 'JFLAGS=-g -pg' 'FFLAGS=-g -pg'", NULL},
	{N_("Optimized"), IANJUTA_BUILDER_CONFIGURATION_OPTIMIZED, "--enable-maintainer-mode 'CFLAGS=-O2' 'CXXFLAGS=-O2' 'JFLAGS=-O2' 'FFLAGS=-O2'", NULL},
	{NULL, NULL, NULL, NULL}
};

/* Helper functions
 *---------------------------------------------------------------------------*/

static gchar*
build_escape_string (const char *unescaped)
{
 	static const gchar hex[16] = "0123456789ABCDEF";
	GString *esc;

 	g_return_val_if_fail (unescaped != NULL, NULL);

	esc = g_string_sized_new (strlen (unescaped) + 16);

	for (; *unescaped != '\0'; unescaped++)
	{
		guchar c = *unescaped;

		if (g_ascii_isalnum (c) || (c == '_') || (c == '-') || (c == '.'))
		{
			g_string_append_c (esc, c);
		}
		else
		{
			g_string_append_c (esc, '%');
			g_string_append_c (esc, hex[c >> 4]);
	  		g_string_append_c (esc, hex[c & 0xf]);
		}
	}

	return g_string_free (esc, FALSE);
}

static gchar *
build_unescape_string (const gchar *escaped)
{
	gchar *unesc;
	gchar *end;

 	if (escaped == NULL)
		return NULL;

	unesc = g_new (gchar, strlen (escaped) + 1);
	end = unesc;

	for (; *escaped != '\0'; escaped++)
	{

		if (*escaped == '%')
		{
			*end++ = (g_ascii_xdigit_value (escaped[1]) << 4) | g_ascii_xdigit_value (escaped[2]);
			escaped += 2;
		}
		else
		{
			*end++ = *escaped;
		}
	}
	*end = '\0';

	return unesc;
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
build_configuration_list_free_list (BuildConfigurationList *list)
{
	BuildConfiguration *cfg;

	for (cfg = list->cfg; cfg != NULL;)
	{
		BuildConfiguration *next = cfg->next;

		if (cfg->args) g_free (cfg->args);
		g_list_foreach (cfg->env, (GFunc)g_free, NULL);
		g_list_free (cfg->env);
		if (cfg->build_uri) g_free (cfg->build_uri);
		if (cfg->name) g_free (cfg->name);
		g_free (cfg);
		cfg = next;
	}
	list->cfg = NULL;
}

static BuildConfiguration *
build_configuration_list_untranslated_get (BuildConfigurationList *list, const gchar *name)
{
	BuildConfiguration *cfg;

	for (cfg = build_configuration_list_get_first (list); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		if (strcmp (cfg->name, name) == 0) return cfg;
	}

	return NULL;
}

/* Public functions
 *---------------------------------------------------------------------------*/

BuildConfiguration *
build_configuration_list_get_first (BuildConfigurationList *list)
{
	return list->cfg;
}

BuildConfiguration *
build_configuration_next (BuildConfiguration *cfg)
{
	return cfg->next;
}

BuildConfiguration *
build_configuration_list_get (BuildConfigurationList *list, const gchar *name)
{
	BuildConfiguration *cfg;

	for (cfg = build_configuration_list_get_first (list); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		if (strcmp (cfg->name, name) == 0) return cfg;
	}

	return NULL;
}

BuildConfiguration *
build_configuration_list_get_selected (BuildConfigurationList *list)
{
	return list->selected == NULL ? list->cfg : list->selected;
}

gint
build_configuration_list_get_position (BuildConfigurationList *list, BuildConfiguration *cfg)
{
	BuildConfiguration *node;
	gint position = 0;

	for (node = build_configuration_list_get_first (list); node != NULL; node = node->next)
	{
		if (node == cfg) return position;
		position++;
	}

	return -1;
}

BuildConfiguration *
build_configuration_list_select (BuildConfigurationList *list, const gchar *name)
{
	BuildConfiguration *cfg = NULL;

	if (name != NULL) cfg = build_configuration_list_get (list, name);
	list->selected = cfg;

	return list->selected;
}

BuildConfiguration *
build_configuration_list_create (BuildConfigurationList *list, const gchar *name)
{
	BuildConfiguration *cfg = NULL;
	BuildConfiguration *prev;

	if (name == NULL) return NULL;

	cfg = build_configuration_list_get (list, name);
	if (cfg == NULL)
	{
		/* Add configuration */
		cfg = g_new0 (BuildConfiguration, 1);
		cfg->name = g_strdup (name);
		prev = build_configuration_list_get_first (list);
		if (prev != NULL)
		{
			/* Append configuration if list is not empty */
			for (;prev->next != NULL; prev = prev->next) ;
			prev->next = cfg;
			cfg->prev = prev;
		}
	}
	list->selected = cfg;

	return list->selected;
}

void
build_configuration_list_from_string_list (BuildConfigurationList *list, GList *str_list)
{
	GList *node;
	BuildConfiguration *prev = NULL;
	const DefaultBuildConfiguration *dcfg;

	build_configuration_list_free_list (list);

	/* Read all configurations from list */
	for (node = str_list; node != NULL; node = g_list_next(node))
	{
		BuildConfiguration *cfg = g_new0 (BuildConfiguration, 1);
		gchar *str = (gchar *)node->data;
		gchar *end;

		cfg->translate = *str == '1';
		str += 2;
		end = strchr (str, ':');
		if (end != NULL)
		{
			gchar *name;

			*end = '\0';
			name = build_unescape_string (str);
			cfg->name = name;
			str = end + 1;

			cfg->build_uri = *str == '\0' ? NULL : g_strdup (str);

			cfg->args = NULL;
			cfg->env = NULL;

			cfg->next = NULL;
			cfg->prev = prev;
			if (prev == NULL)
			{
				list->cfg = cfg;
			}
			else
			{
				prev->next = cfg;
			}
			prev = cfg;
		}
		else
		{
			g_free (cfg);
		}
	}

	/* Add default entry if missing */
	for (dcfg = default_config; dcfg->name != NULL; dcfg++)
	{
		BuildConfiguration *cfg;

		cfg = build_configuration_list_untranslated_get (list, dcfg->name);
		if (cfg == NULL)
		{
			/* Add configuration */
			cfg = g_new (BuildConfiguration, 1);
			cfg->translate = 1;
			cfg->name = g_strdup (dcfg->name);
			cfg->build_uri = g_strdup (dcfg->build_uri);
			cfg->args = NULL;
			cfg->env = NULL;
			cfg->next = NULL;
			cfg->prev = prev;
			if (prev == NULL)
			{
				list->cfg = cfg;
			}
			else
			{
				prev->next = cfg;
			}
			prev = cfg;
		}
		if ((cfg->args == NULL) && (dcfg->args))
		{
			cfg->args = g_strdup (dcfg->args);
		}
		if ((cfg->env == NULL) && (dcfg->env))
		{
			gchar **item;

			for (item = dcfg->env; *item != NULL; item++)
			{
				cfg->env = g_list_prepend (cfg->env, g_strdup (*item));
			}
			cfg->env = g_list_reverse (cfg->env);
		}
	}
}

GList *
build_configuration_list_to_string_list (BuildConfigurationList *list)
{
	GList *str_list = NULL;
	BuildConfiguration *cfg;

	for (cfg = build_configuration_list_get_first (list); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		gchar *esc_name = build_escape_string (cfg->name);
		str_list = g_list_prepend (str_list, g_strdup_printf("%c:%s:%s", cfg->translate ? '1' : '0', esc_name, cfg->build_uri == NULL ? "" : cfg->build_uri));
		g_free (esc_name);
	}
	str_list = g_list_reverse (str_list);

	return str_list;
}

/* Get and Set functions
 *---------------------------------------------------------------------------*/

void
build_configuration_list_set_project_uri (BuildConfigurationList *list, const gchar *uri)
{
	g_free (list->project_root_uri);
	list->project_root_uri = g_strdup (uri);
}

const gchar *
build_configuration_get_translated_name (BuildConfiguration *cfg)
{
	return cfg->translate ? _(cfg->name) : cfg->name;
}

const gchar *
build_configuration_get_name (BuildConfiguration *cfg)
{
	return cfg->name;
}

gboolean
build_configuration_list_set_build_uri (BuildConfigurationList *list, BuildConfiguration *cfg, const gchar *build_uri)
{
	GFile *root;
	GFile *build;
	gchar *rel_uri;
	gboolean ok;

	g_free (cfg->build_uri);
	root = g_file_new_for_uri (list->project_root_uri);
	build = g_file_new_for_uri (build_uri);

	rel_uri = g_file_get_relative_path (root, build);
	/* rel_uri could be NULL if root == build */
	cfg->build_uri = rel_uri;
	ok = (rel_uri != NULL) || g_file_equal (root, build);
	g_object_unref (root);
	g_object_unref (build);

	return ok;
}

GFile*
build_configuration_list_get_build_file (BuildConfigurationList *list, BuildConfiguration *cfg)
{
	GFile *build;

	if (!list->project_root_uri)
		return NULL;

	build = g_file_new_for_uri (list->project_root_uri);
	if (cfg->build_uri != NULL)
	{
		GFile *root;
		root = build;
		build = g_file_resolve_relative_path (root, cfg->build_uri);
		g_object_unref (root);
	}

	return build;
}

const gchar *
build_configuration_get_relative_build_uri (BuildConfiguration *cfg)
{
	return cfg->build_uri;
}

void
build_configuration_set_args (BuildConfiguration *cfg, const gchar *args)
{
	if (cfg->args) g_free (cfg->args);
	cfg->args = args != NULL ? g_strdup (args) : NULL;
}

const gchar *
build_configuration_get_args (BuildConfiguration *cfg)
{
	return cfg->args;
}

void
build_configuration_clear_variables (BuildConfiguration *cfg)
{
	g_list_foreach (cfg->env, (GFunc)g_free, NULL);
	g_list_free (cfg->env);
	cfg->env = NULL;
}

void
build_configuration_set_variable (BuildConfiguration *cfg, const gchar *var)
{
	GList *item;
	guint len = 0;
	const gchar *equal;

	equal = strchr (var, '=');
	if (equal == NULL)
	{
		len = equal - var;
	}

	/* Check if variable already exist */
	for (item = cfg->env; item != NULL; item = g_list_next (item))
	{
		if (((len == 0) && (strcmp ((gchar *)item->data, var) == 0)) ||
		    ((len > 0) && (strncmp ((gchar *)item->data, var, len) == 0) && (((gchar *)item->data)[len] == '=')))
		{
			g_free (item->data);
			cfg->env = g_list_delete_link (cfg->env, item);
		}
	}

	cfg->env = g_list_append (cfg->env, g_strdup (var));
}

GList *
build_configuration_get_variables (BuildConfiguration *cfg)
{
	return cfg->env;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

void
build_configuration_list_free (BuildConfigurationList *list)
{
	g_free (list->project_root_uri);

	build_configuration_list_free_list (list);

	g_free (list);
}

BuildConfigurationList*
build_configuration_list_new (void)
{
	BuildConfigurationList *list;

	list = g_new0 (BuildConfigurationList, 1);

	return list;
}

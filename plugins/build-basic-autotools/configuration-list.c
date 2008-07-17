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
	gchar **args;
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
};

const DefaultBuildConfiguration default_config[] = {
	{N_("Default"), NULL, NULL},
	{N_("Debug"), "Debug", "'CFLAGS=-g -O0' 'CXXFLAGS=-g -O0' 'JFLAGS=-g -O0' 'FFLAGS=-g -O0'"},
	{N_("Profiling"), "Profiling", "'CFLAGS=-g -pg' 'CXXFLAGS=-g -pg' 'JFLAGS=-g -pg' 'FFLAGS=-g -pg'"},
	{N_("Optimized"), "Optimized", "'CFLAGS=-O2' 'CXXFLAGS=-O2' 'JFLAGS=-O2' 'FFLAGS=-O2'"},
	{NULL, NULL, NULL}
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
			*end++ = (g_ascii_xdigit_value (escaped[1]) << 4) | g_ascii_xdigit_value (escaped[0]);
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

#if 0

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
on_select_configuration (GtkComboBox *widget, gpointer user_data)
{
	BuildConfigureDialog *dlg = (BuildConfigureDialog *)user_data;
	gchar *name;
	
	name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dlg->combo));
	
	if (*name == '\0')
	{
		/* Configuration name is mandatory disable Ok button */
		gtk_widget_set_sensitive (dlg->ok, FALSE);
	}
	else
	{
		GList *node;
		
		gtk_widget_set_sensitive (dlg->ok, TRUE);
		
		for (node = dlg->config_list; node != NULL; node = g_list_next (node))
		{
			BuildConfiguration *cfg = (BuildConfiguration *)node->data;
		
			if (strcmp (name, cfg->name) == 0)
			{
				/* Find existing configuration */
				if (cfg->args == NULL)
				{
					gtk_entry_set_text (GTK_ENTRY (dlg->args), "");
				}
				else
				{
					GString* args_str;
					gchar **arg;
				
					args_str = g_string_new (NULL);
					for (arg = cfg->args; *arg != NULL; arg++)
					{
						gchar *quoted_arg = g_shell_quote (*arg);
						
						g_string_append (args_str, quoted_arg);
						g_free (quoted_arg);
						g_string_append_c (args_str, ' ');
					}
					gtk_entry_set_text (GTK_ENTRY (dlg->args), args_str->str);
					g_string_free (args_str, TRUE);
				}

				if (cfg->build_uri == NULL)
				{
					/* No build directory defined, use source directory */
					gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dlg->build_dir_chooser), dlg->project_uri);
				}
				else
				{
					gchar *scheme;
					
					scheme = g_uri_parse_scheme (cfg->build_uri);
					if (scheme)
					{
						/* Absolute directory */
						g_free (scheme);
						build_gtk_file_chooser_create_and_set_current_folder_uri (GTK_FILE_CHOOSER (dlg->build_dir_chooser), cfg->build_uri);
					}
					else
					{
						GFile *dir;
						GFile *build_dir;
						gchar *build_uri;
						
						/* Relative directory */
						dir = g_file_new_for_uri (dlg->project_uri);
						build_dir = g_file_resolve_relative_path (dir,cfg->build_uri);
						g_object_unref (dir);
						build_uri = g_file_get_uri (build_dir);
						g_object_unref (build_dir);
						build_gtk_file_chooser_create_and_set_current_folder_uri (GTK_FILE_CHOOSER (dlg->build_dir_chooser), build_uri);
						g_free (build_uri);										
					}
					
				}
			}
		}
	}
	g_free (name);
}

static void 
fill_dialog (BuildConfigureDialog *dlg)
{
	GtkListStore* store = gtk_list_store_new(1, G_TYPE_STRING);
	GList *node;
	const DefaultBuildConfiguration *cfg;

	/* Add default entry if missing */
	for (cfg = default_config; cfg->name != NULL; cfg++)
	{
		for (node = g_list_first (dlg->config_list); node != NULL; node = g_list_next (node))
		{
			if (strcmp (((BuildConfiguration *)node->data)->name, cfg->name) == 0) break;
		}
		if (node == NULL)
		{
			/* Add configuration */
			BuildConfiguration *new_cfg;
			
			new_cfg = g_new (BuildConfiguration, 1);
			new_cfg->name = g_strdup (cfg->name);
			new_cfg->build_uri = g_strdup (cfg->build_uri);
			new_cfg->args = NULL;
			if (cfg->args)
			{
				g_shell_parse_argv (cfg->args, NULL, &new_cfg->args, NULL);
			}
				
			dlg->config_list = g_list_append (dlg->config_list, new_cfg);
		}
	}
	
	gtk_combo_box_set_model (GTK_COMBO_BOX(dlg->combo), GTK_TREE_MODEL(store));
	gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (dlg->combo), 0);
	
	for (node = g_list_first (dlg->config_list); node != NULL; node = g_list_next (node))
	{
		GtkTreeIter iter;
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, ((BuildConfiguration *)node->data)->name, -1);
	}
}

static void
save_configuration (BuildConfigureDialog *dlg)
{
	gchar *configuration;
	GList *node;
	BuildConfiguration *cfg;
	gchar *uri;
	
	configuration = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dlg->combo));
	
	for (node = dlg->config_list; node != NULL; node = g_list_next (node))
	{
		cfg = (BuildConfiguration *)node->data;
		
		if (strcmp (configuration, cfg->name) == 0)
		{
			/* Move this configuration at the beginning */
			dlg->config_list = g_list_remove_link (dlg->config_list, node);
			dlg->config_list = g_list_concat (node, dlg->config_list);
			g_free (configuration);
			break;
		}
	}
	if (node == NULL)
	{
		/* Create a new configuration */
		dlg->config_list = g_list_prepend (dlg->config_list, g_new0 (BuildConfiguration, 1));
		node = dlg->config_list;
		cfg = (BuildConfiguration *)node->data;
		cfg->name = configuration;
	}
	
	g_strfreev (cfg->args);
	cfg->args = NULL;
	g_free (cfg->build_uri);
	
	g_shell_parse_argv (gtk_entry_get_text (GTK_ENTRY (dlg->args)), NULL, &cfg->args, NULL);
	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dlg->build_dir_chooser));
	cfg->build_uri = uri;
}

BuildConfiguration*
build_dialog_configure (GtkWindow* parent, const gchar *project_root_uri, GList **config_list, gboolean *run_autogen)
{
	GladeXML* gxml;
	BuildConfigureDialog dlg;
	BuildConfiguration *cfg;

	gint response;
	
	/* Get all dialog widgets */
	gxml = glade_xml_new (GLADE_FILE, CONFIGURE_DIALOG, NULL);
	dlg.win = glade_xml_get_widget (gxml, CONFIGURE_DIALOG);
	dlg.combo = glade_xml_get_widget(gxml, CONFIGURATION_COMBO);
	dlg.autogen = glade_xml_get_widget(gxml, RUN_AUTOGEN_CHECK);
	dlg.build_dir_chooser = glade_xml_get_widget(gxml, BUILD_DIR_CHOOSER);
	dlg.args = glade_xml_get_widget(gxml, CONFIGURE_ARGS_ENTRY);
	dlg.ok = glade_xml_get_widget(gxml, OK_BUTTON);
	g_object_unref (gxml);
	
	dlg.config_list = *config_list;
	dlg.project_uri = project_root_uri;

	/* Set run autogen option */	
	if (*run_autogen) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg.autogen), TRUE);

	g_signal_connect (dlg.combo, "changed", G_CALLBACK (on_select_configuration), &dlg);
	
	fill_dialog(&dlg);
	gtk_combo_box_set_active (GTK_COMBO_BOX (dlg.combo), 0);	
	
	response = gtk_dialog_run (GTK_DIALOG (dlg.win));
	
	if (response == GTK_RESPONSE_OK)
	{
		save_configuration (&dlg);	
		*config_list = dlg.config_list;
		*run_autogen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg.autogen));

		cfg = build_configuration_copy ((BuildConfiguration *)dlg.config_list->data);
		build_gtk_file_chooser_keep_folder (GTK_FILE_CHOOSER (dlg.build_dir_chooser), cfg->build_uri);		
	}
	else
	{
		cfg = NULL;
	}
	gtk_widget_destroy (GTK_WIDGET(dlg.win));

	return cfg;
}

#endif


/* Private functions
 *---------------------------------------------------------------------------*/

static void
build_configuration_list_free_list (BuildConfigurationList *list)
{
	BuildConfiguration *cfg;
	
	for (cfg = list->cfg; cfg != NULL;)
	{
		BuildConfiguration *next = cfg->next;
		
		if (cfg->args) g_strfreev (cfg->args);
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
				g_shell_parse_argv (dcfg->args, NULL, &cfg->args, NULL);
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
	
	root = g_file_new_for_uri (list->project_root_uri);
	build = g_file_new_for_uri (build_uri);
	
	rel_uri = g_file_get_relative_path (root, build);
	if (rel_uri)
	{
		g_free (cfg->build_uri);
		cfg->build_uri = rel_uri;
	}
	g_object_unref (root);
	g_object_unref (build);
	
	return rel_uri != NULL;
}

gchar *
build_configuration_list_get_build_uri (BuildConfigurationList *list, BuildConfiguration *cfg)
{
	if (cfg->build_uri != NULL)
	{
		GFile *root;
		GFile *build;
		gchar *uri;
	
		root = g_file_new_for_uri (list->project_root_uri);
		build = g_file_resolve_relative_path (root, cfg->build_uri);
	
		uri = g_file_get_uri (build);
	
		g_object_unref (root);
		g_object_unref (build);
		
		return uri;
	}
	else
	{
		return g_strdup (list->project_root_uri);
	}
}

const gchar *
build_configuration_get_relative_build_uri (BuildConfiguration *cfg)
{
	return cfg->build_uri;
}

void
build_configuration_set_args (BuildConfiguration *cfg, const gchar *args)
{
	if (cfg->args) g_strfreev (cfg->args);
	if (args != NULL)
	{
		g_shell_parse_argv (args, NULL, &cfg->args, NULL);
	}
}

gchar **
build_configuration_get_args (BuildConfiguration *cfg)
{
	return cfg->args;
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

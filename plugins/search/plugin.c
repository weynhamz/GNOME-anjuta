/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <glib/gi18n.h>
#include "plugin.h"
#include "search-replace.h"
#include "search-replace_backend.h"
#include "config.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-search.ui"
#define ICON_FILE "anjuta-search-plugin-48.png"

static void
on_find1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(FALSE, FALSE);
}

static void
on_find_and_replace1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(TRUE, FALSE);
}

static void
on_find_in_files1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(FALSE, TRUE);
}

/*  *user_data : TRUE=Forward  False=Backward  */
static void
on_findnext1_activate (GtkAction * action, gpointer user_data)
{
	search_replace_next();
}

static void
on_findprevious1_activate (GtkAction * action, gpointer user_data)
{
	search_replace_previous();
}

static GtkActionEntry actions_search[] = {
  { "ActionMenuEditSearch", NULL, N_("_Search"), NULL, NULL, NULL},
  { "ActionEditSearchFind", GTK_STOCK_FIND, N_("_Find..."), "<control><alt>f",
	N_("Search for a string or regular expression in the editor"),
    G_CALLBACK (on_find1_activate)},
  { "ActionEditSearchFindNext", GTK_STOCK_FIND, N_("Find _Next"), "<control>g",
	N_("Repeat the last Find command"),
    G_CALLBACK (on_findnext1_activate)},
  { "ActionEditSearchFindPrevious", GTK_STOCK_FIND, N_("Find _Previous"),
	"<control><shift>g",
	N_("Repeat the last Find command"),
	G_CALLBACK (on_findprevious1_activate)},
  { "ActionEditSearchReplace", GTK_STOCK_FIND_AND_REPLACE, N_("Find and R_eplace..."),
	"<control>h",
	N_("Search for and replace a string or regular expression with another string"),
    G_CALLBACK (on_find_and_replace1_activate)},
  { "ActionEditAdvancedSearch", GTK_STOCK_FIND, N_("Search and Replace"),
	NULL, N_("Search and Replace"),
	G_CALLBACK (on_find1_activate)},
  { "ActionEditSearchInFiles", NULL, N_("Fin_d in Files..."), NULL,
	N_("Search for a string in multiple files or directories"),
    G_CALLBACK (on_find_in_files1_activate)},
};

gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	SearchPlugin* splugin = ANJUTA_PLUGIN_SEARCH (plugin);
	IAnjutaDocumentManager* docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell,
																IAnjutaDocumentManager, NULL);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSearch",
					_("Searching..."),
					actions_search,
					G_N_ELEMENTS (actions_search),
					GETTEXT_PACKAGE, TRUE, plugin);

	
	splugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	splugin->docman = docman;
	search_and_replace_init(docman);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	//SearchPlugin *plugin = ANJUTA_PLUGIN_SEARCH (obj);
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	//SearchPlugin *plugin = ANJUTA_PLUGIN_SEARCH (obj);
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
search_plugin_instance_init (GObject *obj)
{
	//SearchPlugin *plugin = ANJUTA_PLUGIN_SEARCH (obj);
}

static void
search_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}
ANJUTA_PLUGIN_BEGIN (SearchPlugin, search_plugin);
ANJUTA_PLUGIN_END;
ANJUTA_SIMPLE_PLUGIN (SearchPlugin, search_plugin);

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) 2000-2007 Naba Kumar <naba@gnome.org>
 *
 * This file is part of anjuta.
 * Anjuta is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * at your option) any later version.
 *
 * Anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with anjuta; if not, contact the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

#include "plugin.h"
#include "search-replace.h"
#include "search-replace_backend.h"
#include "config.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-search.ui"
#define ICON_FILE "anjuta-search-plugin-48.png"

#define ANJUTA_PIXMAP_MATCH_NEXT	"anjuta-go-match-next"
#define ANJUTA_PIXMAP_MATCH_PREV	"anjuta-go-match-prev"
#define ANJUTA_STOCK_MATCH_NEXT		"anjuta-match-next"
#define ANJUTA_STOCK_MATCH_PREV		"anjuta-match-prev"

static gpointer parent_class;

/* user_data for all actions is AnjutaPlugin *plugin */
static void
on_find1_activate (GtkAction *action, gpointer user_data)
{
	/* FIXME get sg data from somewhere not static
	SearchPlugin *plugin;
	SearchReplaceGUI *sg;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	sg = plugin->dialog_data;
	anj_sr_activate (sg, FALSE, FALSE);
*/
	anj_sr_activate (FALSE, FALSE);
}

static void
on_find_and_replace1_activate (GtkAction *action, gpointer user_data)
{
	/* FIXME get sg data from somewhere not static
	SearchPlugin *plugin;
	SearchReplaceGUI *sg;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	sg = plugin->dialog_data;
	anj_sr_activate (sg, TRUE, FALSE);
*/
	anj_sr_activate (TRUE, FALSE);
}

static void
on_find_in_files1_activate (GtkAction *action, gpointer user_data)
{
	/* FIXME get sg data from somewhere not static
	SearchPlugin *plugin;
	SearchReplaceGUI *sg;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	sg = plugin->dialog_data;
	anj_sr_activate (sg, FALSE, TRUE);
*/
	anj_sr_activate (FALSE, TRUE);
}

static void
on_findnext1_activate (GtkAction *action, gpointer user_data)
{
	/* FIXME get sg data from somewhere not static
	SearchPlugin *plugin;
	SearchReplaceGUI *sg;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	sg = plugin->dialog_data;
*/
	SearchReplaceGUI *sg;

	anj_sr_get_best_uidata (&sg, NULL);
	anj_sr_select_next (sg);
}

static void
on_findprevious1_activate (GtkAction *action, gpointer user_data)
{
	/* FIXME get sg data from somewhere not static
	SearchPlugin *plugin;
	SearchReplaceGUI *sg;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	sg = plugin->dialog_data;
*/
	SearchReplaceGUI *sg;

	anj_sr_get_best_uidata (&sg, NULL);
	anj_sr_select_previous (sg);
}
/* this is not a duplicate of on_findprevious1_activate(). That re-uses the
  pattern from the last search via the search dialog
  CHECKME is this available in UI ?
static void
on_prev_occur (GtkAction *action, gpointer user_data)
{
	SearchPlugin *plugin;
	IAnjutaDocumentManager *docman;
	IAnjutaDocument *doc;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if (IANJUTA_IS_EDITOR (doc))
	{
		IAnjutaEditor *te;
		gchar *buffer;

		te = IANJUTA_EDITOR (doc);
		buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
		if (buffer != NULL)
		{
			g_strstrip (buffer);
			if (*buffer == 0)
			{
				g_free (buffer);
				buffer = NULL;
			}
		}
		if (buffer == NULL)
			buffer = ianjuta_editor_get_current_word (te, NULL);
		if (buffer != NULL)
		{
		    search_incremental (te, buffer, SD_BACKWARD);
			g_free (buffer);
		}
	}
}
*/
/* this is not a duplicate of on_findnext1_activate(). That re-uses the
  pattern from the last search via the search dialog.
  CHECKME is this available in UI ?
static void
on_next_occur (GtkAction *action, gpointer user_data)
{
	SearchPlugin *plugin;
	IAnjutaDocumentManager *docman;
	IAnjutaDocument *doc;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if (IANJUTA_IS_EDITOR (doc))
	{
		IAnjutaEditor *te;
		gchar *buffer;

		te = IANJUTA_EDITOR (doc);
		buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
		if (buffer != NULL)
		{
			g_strstrip (buffer);
			if (*buffer == 0)
			{
				g_free (buffer);
				buffer = NULL;
			}
		}
		if (buffer == NULL)
			buffer = ianjuta_editor_get_current_word (te, NULL);
		if (buffer != NULL)
		{
		    search_incremental (te, buffer, SD_FORWARD);
			g_free (buffer);
		}
	}
}
*/

static void
on_search_again_activate (GtkAction *action, gpointer user_data)
{
	/* FIXME get sg data from somewhere not static
	SearchPlugin *plugin;
	SearchReplaceGUI *sg;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	sg = plugin->dialog_data;
*/
	SearchReplaceGUI *sg;

	sg = anj_sr_get_default_uidata ();
	anj_sr_repeat (sg);
}

static void
on_find_usage (GtkAction *action, gpointer user_data)
{
	SearchPlugin *plugin;
	IAnjutaDocumentManager *docman;
	IAnjutaDocument *doc;

	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if (IANJUTA_IS_EDITOR (doc))
	{
		IAnjutaEditor *te;
		gchar *buffer;

		te = IANJUTA_EDITOR (doc);
		buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
		if (buffer != NULL)
		{
			g_strstrip (buffer);
			if (*buffer == 0)
			{
				g_free (buffer);
				buffer = NULL;
			}
		}
		if (buffer == NULL)
			buffer = ianjuta_editor_get_current_word (te, NULL);
		if (buffer != NULL)
		{
			anj_sr_list_all_uses (buffer);
			g_free (buffer);
		}
	}
}

static GtkActionEntry actions_search[] =
{
  { "ActionMenuEditSearch", NULL, N_("_Search"), NULL, NULL, NULL},
  { "ActionEditSearchFind", GTK_STOCK_FIND, N_("_Find..."), "<control><alt>f",
	N_("Search for a string or regular expression in the editor"),
    G_CALLBACK (on_find1_activate)},
  { "ActionEditSearchFindNext", ANJUTA_STOCK_MATCH_NEXT, N_("Find _Next"), "<control>g",
	N_("Find next match using the last-used search parameters"),
    G_CALLBACK (on_findnext1_activate)},
  { "ActionEditSearchFindPrevious", ANJUTA_STOCK_MATCH_PREV, N_("Find _Previous"),
	"<control><shift>g",
	N_("Find previous match using the last-used search parameters"),
	G_CALLBACK (on_findprevious1_activate)},
  { "ActionEditSearchReplace", GTK_STOCK_FIND_AND_REPLACE, N_("Find and R_eplace..."),
	"<control>h",
	N_("Search for a string or regular expression and replace with another string"),
    G_CALLBACK (on_find_and_replace1_activate)},
  { "ActionEditSearchAgain", NULL, N_("Search/Replace _Again"),	"<shift><control><alt>f",
	N_("Repeat last-used find or replace operation"),
    G_CALLBACK (on_search_again_activate)},
/*  { "ActionEditAdvancedSearch", GTK_STOCK_FIND, N_("Advanced Search And Replace"),
	NULL, N_("New advance search and replace stuff"),
	G_CALLBACK (on_find1_activate)},
*/
  { "ActionEditSearchInFiles", NULL, N_("Fin_d in Files..."), NULL,
	N_("Search for a string in multiple files or directories"),
    G_CALLBACK (on_find_in_files1_activate)},
  { "ActionEditSearchUseInFiles", NULL, N_("List all matches"), NULL,
	N_("List occurrences of current selection or word in all project files"),
    G_CALLBACK (on_find_usage)},
/* effectively superseded by search-box
  { "ActionEditGotoOccuranceNext", ANJUTA_STOCK_MATCH_NEXT,
	N_("Ne_xt Occurrence"), NULL,
	N_("Find the next occurrence of current word"),
    G_CALLBACK (on_next_occur)},
  { "ActionEditGotoOccurancePrev", ANJUTA_STOCK_MATCH_PREV,
	N_("Pre_vious Occurrence"),  NULL,
	N_("Find the previous occurrence of current word"),
    G_CALLBACK (on_prev_occur)},
 extract from .ui file to enable the above
		<placeholder name="PlaceholderGotoMenus">
		<menu name="Goto" action="ActionMenuGoto">
			<placeholder name="PlaceholderGotoOccurence">
				<menuitem name="PreviousOccurance" action="ActionEditGotoOccurancePrev" />
				<menuitem name="NextOccurance" action="ActionEditGotoOccuranceNext" />
			</placeholder>
		</menu>
		</placeholder>
*/
};

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	static gboolean init = FALSE;
	AnjutaUI *ui;
	SearchPlugin *splugin;
	IAnjutaDocumentManager *docman;

	if (!init)
	{
		BEGIN_REGISTER_ICON (plugin);
		REGISTER_ICON_FULL (ANJUTA_PIXMAP_MATCH_NEXT, ANJUTA_STOCK_MATCH_NEXT);
		REGISTER_ICON_FULL (ANJUTA_PIXMAP_MATCH_PREV, ANJUTA_STOCK_MATCH_PREV);
		END_REGISTER_ICON;
		init = TRUE;
	}

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSearch",
					_("Searching..."),
					actions_search,
					G_N_ELEMENTS (actions_search),
					GETTEXT_PACKAGE, TRUE, plugin);

	splugin = ANJUTA_PLUGIN_SEARCH (plugin);
	splugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager,
										NULL);
	splugin->docman = docman;

	search_replace_init (plugin);

	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_default_uidata (); /* CHECKME if > 1 dialog */
	anj_sr_destroy_ui_data (sg);
	search_replace_data_destroy (NULL);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	//SearchPlugin *plugin = ANJUTA_PLUGIN_SEARCH (obj);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
search_plugin_instance_init (GObject *obj)
{
	//SearchPlugin *plugin = ANJUTA_PLUGIN_SEARCH (obj);
}

static void
search_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class;

	parent_class = g_type_class_peek_parent (klass);

	plugin_class = ANJUTA_PLUGIN_CLASS (klass);
	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

#undef ICON_FILE

ANJUTA_PLUGIN_BEGIN (SearchPlugin, search_plugin);
ANJUTA_PLUGIN_END;
ANJUTA_SIMPLE_PLUGIN (SearchPlugin, search_plugin);

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

#include <libegg/menu/egg-entry-action.h>

#include "plugin.h"
#include "search-replace.h"
#include "search-replace_backend.h"
#include "config.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-search.ui"
#define ICON_FILE "anjuta-search-plugin-48.png"

#define ANJUTA_PIXMAP_MATCH_NEXT				  "anjuta-go-match-next"
#define ANJUTA_PIXMAP_MATCH_PREV				  "anjuta-go-match-prev"
#define ANJUTA_STOCK_MATCH_NEXT				  "anjuta-match-next"
#define ANJUTA_STOCK_MATCH_PREV				  "anjuta-match-prev"

/* Find next occurence of expression in Editor
   Caching of FileBuffer might be useful here to improve performance
   Returns: TRUE = found, FALSE = not found
*/

static gboolean find_incremental(IAnjutaEditor* te, gchar* expression, 
								 SearchDirection dir)
{
	FileBuffer* fb = file_buffer_new_from_te (te);
	SearchExpression* se = g_new0(SearchExpression, 1);
	MatchInfo* info;
	gboolean ret;
		
	se->search_str = expression;
	se->regex = FALSE;
	se->greedy = FALSE;
	se->ignore_case = TRUE;
	se->whole_word = FALSE;
	se->whole_line = FALSE;
	se->word_start = FALSE;
	se->no_limit = FALSE;
	se->actions_max = 1;
	se->re = NULL;

	info = get_next_match(fb, dir, se);
	
	if (info != NULL)
	{
		IAnjutaIterable *start, *end;
		start = ianjuta_editor_get_position_from_offset (te, info->pos, NULL);
		end = ianjuta_editor_get_position_from_offset (te, info->pos + info->len, NULL);
		ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (te),
									  start, end, NULL);
		g_object_unref (start);
		g_object_unref (end);
		ret = TRUE;
	}
	else
		ret = FALSE;
	
	match_info_free(info);
	file_buffer_free(fb);
	g_free(se);
	
	return ret;
}

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

static void
on_prev_occur(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor* te;
	IAnjutaDocumentManager *docman;
	IAnjutaDocument* doc;
	SearchPlugin *plugin;
    gint return_;
	gchar *buffer = NULL;
	
	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	doc = ianjuta_document_manager_get_current_document(docman, NULL);
	te = IANJUTA_IS_EDITOR(doc) ? IANJUTA_EDITOR(doc) : NULL;
	if(!te) return;
	if ((buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL)))
	{
		g_strstrip(buffer);
		if ('\0' == *buffer)
		{
			g_free(buffer);
			buffer = NULL;
		}
	}
	if (NULL == buffer)
	{
		buffer = ianjuta_editor_get_current_word(te, NULL);
		if (!buffer)
			return;
	}
    return_= find_incremental(te, buffer, SD_BACKWARD);
	
	g_free(buffer);
}

static void 
on_next_occur(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor* te;
	IAnjutaDocumentManager *docman;
	IAnjutaDocument* doc;
	SearchPlugin *plugin;
    gint return_;
	gchar *buffer = NULL;
	
	plugin = ANJUTA_PLUGIN_SEARCH (user_data);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	doc = ianjuta_document_manager_get_current_document(docman, NULL);
	te = IANJUTA_IS_EDITOR(doc) ? IANJUTA_EDITOR(doc) : NULL;
	if(!te) return;
	if ((buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL)))
	{
		g_strstrip(buffer);
		if ('\0' == *buffer)
		{
			g_free(buffer);
			buffer = NULL;
		}
	}
	if (NULL == buffer)
	{
		buffer = ianjuta_editor_get_current_word(te, NULL);
		if (!buffer)
			return;
	}
    return_= find_incremental(te, buffer, SD_FORWARD);
	
	g_free(buffer);
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
	{ "ActionEditGotoOccuranceNext", ANJUTA_STOCK_MATCH_NEXT,
	N_("Ne_xt Occurrence"), NULL,
	N_("Find the next occurrence of current word"),
    G_CALLBACK (on_next_occur)},
  { "ActionEditGotoOccurancePrev",ANJUTA_STOCK_MATCH_PREV,
	N_("Pre_vious Occurrence"),  NULL,
	N_("Find the previous occurrence of current word"),
    G_CALLBACK (on_prev_occur)},
};

gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	static gboolean init = FALSE;
	AnjutaUI *ui;
	SearchPlugin* splugin = ANJUTA_PLUGIN_SEARCH (plugin);
	IAnjutaDocumentManager* docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell,
																IAnjutaDocumentManager, NULL);
	
	
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
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}
ANJUTA_PLUGIN_BEGIN (SearchPlugin, search_plugin);
ANJUTA_PLUGIN_END;
ANJUTA_SIMPLE_PLUGIN (SearchPlugin, search_plugin);

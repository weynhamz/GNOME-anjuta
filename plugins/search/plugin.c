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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#define ICON_FILE "anjuta-search.png"

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
		gboolean backward;
		backward = dir == SD_BACKWARD?TRUE:FALSE;
		ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (te),
									  info->pos, info->pos + info->len, backward, NULL);
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
on_enterselection (GtkAction * action, gpointer user_data)
{
	GtkAction *entry_action;
	AnjutaUI* ui;
	IAnjutaEditor *te;
	IAnjutaDocumentManager* docman;
	SearchPlugin* plugin;
	gchar *selectionText = NULL;
	GSList *proxies;
	
	plugin = (SearchPlugin *) user_data;
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL);
	docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell,
										IAnjutaDocumentManager, NULL);
	te = ianjuta_document_manager_get_current_editor(docman, NULL);
	if (!te) return;
	
	entry_action = anjuta_ui_get_action (ui, "ActionGroupSearch",
									   "ActionEditSearchEntry");
	g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
	
	selectionText = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te),
												  NULL);
	if (selectionText != NULL && selectionText[0] != '\0')
	{
		egg_entry_action_set_text (EGG_ENTRY_ACTION (entry_action), selectionText);
	}		
	/* Which proxy to focus? For now just focus the first one */
	proxies = gtk_action_get_proxies (GTK_ACTION (entry_action));
	if (proxies)
	{
		GtkWidget *child;
		child = gtk_bin_get_child (GTK_BIN (proxies->data));
		gtk_widget_grab_focus (GTK_WIDGET (child));
	}
	g_free (selectionText);
}

static void
on_prev_occur(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor* te;
	IAnjutaDocumentManager *docman;
	SearchPlugin *plugin;
    gint return_;
	gchar *buffer = NULL;
	
	plugin = (SearchPlugin *) user_data;
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	te = ianjuta_document_manager_get_current_editor (docman, NULL);
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
	SearchPlugin *plugin;
    gint return_;
	gchar *buffer = NULL;
	
	plugin = (SearchPlugin *) user_data;
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDocumentManager, NULL);
	te = ianjuta_document_manager_get_current_editor (docman, NULL);
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

/* Incremental search */

typedef struct
{
	gint pos;
	gboolean wrap;
	
} IncrementalSearch;

static void
on_incremental_entry_key_press (GtkWidget *entry, GdkEventKey *event,
								SearchPlugin *plugin)
{
	if (event->keyval == GDK_Escape)
	{
		IAnjutaEditor *te;
		
		te = ianjuta_document_manager_get_current_editor(plugin->docman, NULL);
		if (te)
			ianjuta_editor_grab_focus (te, NULL);
	}
}

static void on_toolbar_find_start_over(GtkAction * action, gpointer user_data);

/* FIXME: Wrapping does not yet work */

static void
on_toolbar_find_clicked (GtkAction *action, gpointer user_data)
{
	const gchar *string;
	gchar* expression;
	gint ret;
	IAnjutaEditor *te;
	IAnjutaDocumentManager *docman;
	SearchPlugin *plugin;
	IncrementalSearch *search_params;
	gboolean search_wrap = FALSE;
	AnjutaStatus *status; 
	AnjutaUI* ui;
	
	plugin = (SearchPlugin *) user_data;
	docman = plugin->docman;
	te = ianjuta_document_manager_get_current_editor(docman, NULL);
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL);
	
	if (!te)
		return;

	search_params = g_object_get_data (G_OBJECT (te), "incremental_search");
	if (!search_params)
	{
		search_params = g_new0 (IncrementalSearch, 1);
		g_object_set_data_full (G_OBJECT (te), "incremental_search",
								search_params, (GDestroyNotify)g_free);
	}
	if (EGG_IS_ENTRY_ACTION (action))
	{
		string = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	else
	{
		GtkAction *entry_action;
		entry_action = anjuta_ui_get_action (ui,
											 "ActionGroupSearch",
											 "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
		string = egg_entry_action_get_text (EGG_ENTRY_ACTION (entry_action));
	}
	if (search_params->pos >= 0 &&  search_params->wrap)
	{
		/* If incremental search wrap requested, so wrap it. */
		search_wrap = TRUE;
	}
	
	expression = g_strdup(string);
	if (search_wrap)
	{
		ianjuta_editor_goto_position(te, 0, NULL);
		ret = find_incremental(te, expression, SD_FORWARD);
		search_params->wrap = FALSE;
	}
	else
	{
		ret = find_incremental(te, expression, SD_FORWARD);
	}
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (user_data)->shell, NULL);
	
	if (ret == FALSE) {
		if (search_params->pos < 0)
		{
			GtkWindow *parent;
			GtkWidget *dialog;
			
			parent = GTK_WINDOW (ANJUTA_PLUGIN(user_data)->shell);
			dialog = gtk_message_dialog_new (parent,
											 GTK_DIALOG_DESTROY_WITH_PARENT,
											 GTK_MESSAGE_QUESTION,
											 GTK_BUTTONS_YES_NO,
					_("No matches. Wrap search around the document?"));
			if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
				on_toolbar_find_start_over (action, user_data);
			gtk_widget_destroy (dialog);
		}
		else
		{
			if (search_wrap == FALSE)
			{
				anjuta_status_push(status,
					_("Incremental search for '%s' failed. Press Enter or click Find to continue searching at the top."),
					string);
				search_params->wrap = 1;
				gdk_beep();
			}
			else
			{
				anjuta_status_push (status, _("Incremental search for '%s' (continued at top) failed."), 
				                    string);
			}
		}
	}
	else
		anjuta_status_clear_stack (status);
}

static void
on_toolbar_find_start_over (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *te;
	IAnjutaDocumentManager *docman;
	SearchPlugin *plugin;
	
	plugin = (SearchPlugin *) user_data;
	docman = plugin->docman;
	te = ianjuta_document_manager_get_current_editor(docman, NULL);
	
	/* search from doc start */
	ianjuta_editor_goto_position(te, 0, NULL);
	on_toolbar_find_clicked (action, user_data);
}

static gboolean
on_toolbar_find_incremental_start (GtkAction *action, gpointer user_data)
{
	IAnjutaEditor *te;
	IAnjutaDocumentManager *docman;
	SearchPlugin *plugin;
	IncrementalSearch *search_params;
	GSList *entries, *node;
	static GHashTable *entries_connected = NULL;
	
	plugin = (SearchPlugin *) user_data;
	docman = plugin->docman;
	te = ianjuta_document_manager_get_current_editor(docman, NULL);

	if (!te) return FALSE;
	
	/* Make sure we set up escape for getting out the focus to the editor */
	if (entries_connected == NULL)
	{
		entries_connected = g_hash_table_new (g_direct_hash, g_direct_equal);
	}
	entries = gtk_action_get_proxies (action);
	node = entries;
	while (node)
	{
		GtkWidget *entry;
		entry = GTK_WIDGET (node->data);
		if (!g_hash_table_lookup (entries_connected, entry))
		{
			g_signal_connect (G_OBJECT (entry), "key-press-event",
							  G_CALLBACK (on_incremental_entry_key_press),
							  plugin);
			g_hash_table_insert (entries_connected, entry, entry);
		}
		node = g_slist_next (node);
	}

	search_params = g_object_get_data (G_OBJECT (te), "incremental_search");
	if (!search_params)
	{
		search_params = g_new0 (IncrementalSearch, 1);
		g_object_set_data_full (G_OBJECT (te), "incremental_search",
								search_params, (GDestroyNotify)g_free);
	}
	/* Prepare to begin incremental search */	
	search_params->pos = ianjuta_editor_get_position(te, NULL);
	search_params->wrap = FALSE;
	return FALSE;
}

static gboolean
on_toolbar_find_incremental_end (GtkAction *action, gpointer user_data)
{
	IAnjutaEditor *te;
	IAnjutaDocumentManager *docman;
	SearchPlugin *plugin;
	IncrementalSearch *search_params;
	AnjutaStatus *status; 
	
	plugin = (SearchPlugin *) user_data;
	docman = plugin->docman;
	te = ianjuta_document_manager_get_current_editor(docman, NULL);

	if (!te) 
		return FALSE;
		
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (user_data)->shell, NULL);
	anjuta_status_clear_stack (status);
	
	search_params = g_object_get_data (G_OBJECT (te), "incremental_search");
	if (search_params)
	{
		search_params->pos = -1;
		search_params->wrap = FALSE;
	}
	return FALSE;
}

static void
on_toolbar_find_incremental (GtkAction *action, gpointer user_data)
{
	const gchar *entry_text;
	IAnjutaEditor *te;
	IAnjutaDocumentManager *docman;
	SearchPlugin *plugin;
	IncrementalSearch *search_params;
	
	plugin = (SearchPlugin *) user_data;
	docman = plugin->docman;
	te = ianjuta_document_manager_get_current_editor(docman, NULL);
	
	if (!te)
		return;
	
	search_params = g_object_get_data (G_OBJECT (te), "incremental_search");
	if (!search_params)
	{
		search_params = g_new0 (IncrementalSearch, 1);
		g_object_set_data_full (G_OBJECT (te), "incremental_search",
								search_params, (GDestroyNotify)g_free);
	}
	
	if (search_params->pos < 0)
		return;
	
	if (EGG_IS_ENTRY_ACTION (action))
	{
		entry_text = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	else
	{
		AnjutaUI *ui;
		GtkAction *entry_action;
		
		ui = ANJUTA_UI (g_object_get_data (G_OBJECT (user_data), "ui"));
		entry_action = anjuta_ui_get_action (ui, "ActionGroupSearch",
											 "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
		entry_text =
			egg_entry_action_get_text (EGG_ENTRY_ACTION (entry_action));
	}
	if (!entry_text || strlen(entry_text) < 1) return;
	
	ianjuta_editor_goto_position(te, search_params->pos, NULL);
	on_toolbar_find_clicked (NULL, user_data);
}

static GtkActionEntry actions_search[] = {
  { "ActionMenuEditSearch", NULL, N_("_Search"), NULL, NULL, NULL},
  { "ActionEditSearchFind", GTK_STOCK_FIND, N_("_Find..."), "<control>f",
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
	"<shift><control>f",
	N_("Search for and replace a string or regular expression with another string"),
    G_CALLBACK (on_find_and_replace1_activate)},
  { "ActionEditAdvancedSearch", GTK_STOCK_FIND, N_("Advanced Search And Replace"),
	NULL, N_("New advance search And replace stuff"),
	G_CALLBACK (on_find1_activate)},
  { "ActionEditSearchSelectionISearch", NULL, N_("_Enter Selection/I-Search"),
	"<control>e",
	N_("Enter the selected text as the search target"),
    G_CALLBACK (on_enterselection)},
  { "ActionEditSearchInFiles", NULL, N_("Fin_d in Files..."), NULL,
	N_("Search for a string in multiple files or directories"),
    G_CALLBACK (on_find_in_files1_activate)},
	{ "ActionEditGotoOccuranceNext", GTK_STOCK_JUMP_TO,
	N_("Ne_xt occurrence"), NULL,
	N_("Find the next occurrence of current word"),
    G_CALLBACK (on_next_occur)},
  { "ActionEditGotoOccurancePrev",GTK_STOCK_JUMP_TO,
	N_("Pre_vious occurrence"),  NULL,
	N_("Find the previous occurrence of current word"),
    G_CALLBACK (on_prev_occur)},
};

gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkActionGroup* group;
	GtkAction*  action;
	SearchPlugin* splugin = (SearchPlugin*) plugin;
	IAnjutaDocumentManager* docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell,
																IAnjutaDocumentManager, NULL);
	
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSearch",
					_("Searching..."),
					actions_search,
					G_N_ELEMENTS (actions_search),
					GETTEXT_PACKAGE, plugin);
		
	group = gtk_action_group_new ("ActionGroupSearch");
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditSearchEntry",
						   "label", _("Search"),
						   "tooltip", _("Incremental search"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 150,
						   NULL);
	g_assert (EGG_IS_ENTRY_ACTION (action));
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_find_clicked), plugin);
	g_signal_connect (action, "changed",
					  G_CALLBACK (on_toolbar_find_incremental), plugin);
	g_signal_connect (action, "focus-in",
					  G_CALLBACK (on_toolbar_find_incremental_start), plugin);
	g_signal_connect (action, "focus-out",
					  G_CALLBACK (on_toolbar_find_incremental_end), plugin);
	gtk_action_group_add_action (group, action);
	anjuta_ui_add_action_group(ui, "ActionGroupSearch", _("Search Toolbar"), group);
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);

	
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
	//SearchPlugin *plugin = (SearchPlugin*)obj;
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
search_plugin_instance_init (GObject *obj)
{
	//SearchPlugin *plugin = (SearchPlugin*)obj;
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

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * action-callbacks.c
 * Copyright (C) 2003 Naba Kumar
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * on_enter_sel
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <config.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-utils.h>

#include <libegg/menu/egg-entry-action.h>

#include "anjuta-docman.h"
#include "action-callbacks.h"
#include "text_editor.h"
#include "search-replace.h"
#include "search-replace_backend.h"
#include "plugin.h"
#include "goto_line.h"
#include "print.h"
#include "lexer.h"
#include "file_history.h"

gboolean closing_state;

void
on_open1_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	anjuta_docman_open_file (docman);
}

void
on_save1_activate (GtkAction *action, gpointer user_data)
{
	gboolean ret;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	if (te->uri == NULL)
	{
		anjuta_docman_set_current_editor (docman, te);
		anjuta_docman_save_as_file (docman);
		return;
	}
	ret = text_editor_save_file (te, TRUE);
	if (closing_state && ret == TRUE)
	{
		anjuta_docman_remove_editor (docman, te);
		closing_state = FALSE;
	}
}

void
on_save_as1_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	anjuta_docman_save_as_file (docman);
}

void
on_save_all1_activate (GtkAction * action, gpointer user_data)
{
	GList *node;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	node = anjuta_docman_get_all_editors (docman);
	while (node)
	{
		TextEditor* te;
		GList* next;
		te = node->data;
		next = node->next;
		if(te)
		{
			if (!text_editor_is_saved (te) && te->uri)
			{
				text_editor_save_file (te, TRUE);
			}
		}
		node = next;
	}
}

void
on_close_file1_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	TextEditor *te;
	GtkWidget *parent;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
#if 0
	if (te->used_by_cvs) {
		GtkWidget* dialog;
		gint value;
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
		    GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			_("The editor is being used as output buffer for an operation.\n"
			"Closing it will result in stopping the process.\n"
			"Do you still want close the editor?"));
		value = gtk_dialog_run (GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		if (value == 1) return;
	}
#endif
	if (text_editor_is_saved (te) == FALSE)
	{
		gchar *mesg;
		GtkWidget *dialog;
		gint res;
		
		closing_state = TRUE;
		mesg = g_strdup_printf (_("The file '%s' is not saved.\n"
								"Do you want to save it before closing?"),
								te->filename);
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE, mesg);
		g_free (mesg);
		gtk_dialog_add_button (GTK_DIALOG (dialog), 
							   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		anjuta_util_dialog_add_button (GTK_DIALOG (dialog), _("Do_n't save"),
									   GTK_STOCK_NO, GTK_RESPONSE_NO);
		gtk_dialog_add_button (GTK_DIALOG (dialog),
							   GTK_STOCK_SAVE, GTK_RESPONSE_YES);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
										 GTK_RESPONSE_CANCEL);
		res = gtk_dialog_run (GTK_DIALOG (dialog));
		if (res == GTK_RESPONSE_YES)
			on_save1_activate (NULL, docman);
		else if (res == GTK_RESPONSE_NO)
		{
			anjuta_docman_remove_editor (docman, te);
			closing_state = FALSE;
		}
		else
			closing_state = FALSE;
		gtk_widget_destroy (dialog);
	}
	else
		anjuta_docman_remove_editor (docman, te);
}

void
on_close_all_file1_activate (GtkAction * action, gpointer user_data)
{
	GList *node;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	/* Close all 'saved' files */
	node = anjuta_docman_get_all_editors (docman);
	while (node)
	{
		TextEditor* te;
		GList* next;
		te = node->data;
		next = node->next; /* Save it now, as we may change it. */
		if(te)
		{
			if (text_editor_is_saved (te))
			{
				anjuta_docman_remove_editor (docman, te);
			}
		}
		node = next;
	}
}

void
on_reload_file1_activate (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	gchar mesg[256];
	GtkWidget *dialog;
	GtkWidget *parent;
	EditorPlugin *plugin;
	AnjutaDocman *docman;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;

	sprintf (mesg, _("Are you sure you want to reload '%s'?\n"
					 "Any unsaved changes will be lost."),
			 te->filename);

	parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_NONE, mesg);
	gtk_dialog_add_button (GTK_DIALOG (dialog),
						   GTK_STOCK_CANCEL,	GTK_RESPONSE_NO);
	anjuta_util_dialog_add_button (GTK_DIALOG (dialog), _("_Reload"),
							  GTK_STOCK_REVERT_TO_SAVED,
							  GTK_RESPONSE_YES);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
									 GTK_RESPONSE_NO);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		text_editor_load_file (te);
		/* FIXME: anjuta_update_title (); */
	}
	gtk_widget_destroy (dialog);
}

void
anjuta_print_cb (GtkAction *action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	anjuta_print (FALSE, plugin->prefs, te);
}

void
anjuta_print_preview_cb (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	anjuta_print (TRUE, plugin->prefs, te);
}

static void
on_editor_command_activate (GtkAction * action, gint command, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, command, 0, 0);
}

void on_editor_command_upper_case_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_UPRCASE, data);
}

void on_editor_command_lower_case_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_LWRCASE, data);
}

void on_editor_command_eol_crlf_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_EOL_CRLF, data);
}

void on_editor_command_eol_lf_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_EOL_LF, data);
}

void on_editor_command_eol_cr_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_EOL_CR, data);
}

void on_editor_command_select_all_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_SELECTALL, data);
}

void on_editor_command_select_to_brace_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_SELECTTOBRACE, data);
}

void on_editor_command_select_block_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_SELECTBLOCK, data);
}

void on_editor_command_match_brace_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_MATCHBRACE, data);
}

void on_editor_command_undo_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_UNDO, data);
}

void on_editor_command_redo_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_REDO, data);
}

void on_editor_command_cut_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_CUT, data);
}

void on_editor_command_copy_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_COPY, data);
}

void on_editor_command_paste_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_PASTE, data);
}

void on_editor_command_clear_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_CLEAR, data);
}

void on_editor_command_complete_word_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_COMPLETEWORD, data);
}

void on_editor_command_indent_increase_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_INDENT_INCREASE, data);
}

void on_editor_command_indent_decrease_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_INDENT_DECREASE, data);
}

void on_editor_command_close_folds_all_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_CLOSE_FOLDALL, data);
}

void on_editor_command_open_folds_all_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_OPEN_FOLDALL, data);
}

void on_editor_command_toggle_fold_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_TOGGLE_FOLD, data);
}

void on_editor_command_bookmark_toggle_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_BOOKMARK_TOGGLE, data);
}

void on_editor_command_bookmark_first_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_BOOKMARK_FIRST, data);
}

void on_editor_command_bookmark_next_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_BOOKMARK_NEXT, data);
}

void on_editor_command_bookmark_prev_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_BOOKMARK_PREV, data);
}

void on_editor_command_bookmark_last_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_BOOKMARK_LAST, data);
}

void on_editor_command_bookmark_clear_activate (GtkAction * action, gpointer data)
{
	on_editor_command_activate (action, ANE_BOOKMARK_CLEAR, data);
}

void
on_editor_select_function (GtkAction *action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	function_select(te);
}

void on_editor_select_word (GtkAction *action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_WORDSELECT, 0, 0);
}

void on_editor_select_line (GtkAction *action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_LINESELECT, 0, 0);
}

void
on_transform_eolchars1_activate (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	glong mode;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;

	mode = (glong)g_object_get_data (G_OBJECT (user_data), "user_data");
	aneditor_command (te->editor_id, ANE_EOL_CONVERT, mode, 0);
}

void
on_autocomplete1_activate (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_COMPLETEWORD, 0, 0);
}

void
on_search1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(FALSE, FALSE);
}

void
on_find1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(FALSE, FALSE);
}

void
on_find_and_replace1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(TRUE, FALSE);
}

void
on_find_in_files1_activate (GtkAction * action, gpointer user_data)
{
	anjuta_search_replace_activate(FALSE, TRUE);
}

void on_prev_occur(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	gboolean ret;
    gint return_;
	gchar *buffer = NULL;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor(docman);
	if(!te) return;
	if (text_editor_has_selection(te))
	{
		buffer = text_editor_get_selection(te);
		g_strstrip(buffer);
		if ('\0' == *buffer)
		{
			g_free(buffer);
			buffer = NULL;
		}
	}
	if (NULL == buffer)
	{
		buffer = g_new(char, 256);
		ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buffer, 255L);
		if (!ret)
		{
			g_free(buffer);
			return;
		}
	}
    return_=text_editor_find(te,buffer,TEXT_EDITOR_FIND_SCOPE_CURRENT,0,0,1,1,0);
	
	g_free(buffer);

}

void on_next_occur(GtkAction * action, gpointer user_data)
{
	gboolean ret;
	gchar *buffer = NULL;
    gint return_;
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor(docman);
	if(!te) return;
	if (text_editor_has_selection(te))
	{
		buffer = text_editor_get_selection(te);
		g_strstrip(buffer);
		if ('\0' == *buffer)
		{
			g_free(buffer);
			buffer = NULL;
		}
	}
	if (NULL == buffer)
	{
		buffer = g_new(char, 256);
		ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buffer, 255L);
		if (!ret)
		{
			g_free(buffer);
			return;
		}
	}
    return_=text_editor_find(te,buffer,TEXT_EDITOR_FIND_SCOPE_CURRENT,1,0,1,1,0);
	
	g_free(buffer);

}

void on_prev_history (GtkAction *action, gpointer user_data)
{
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	an_file_history_back (docman);
}

void on_next_history (GtkAction *action, gpointer user_data)
{
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	an_file_history_forward (docman);
}

void on_comment_block (GtkAction * action, gpointer user_data)
{
TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_BLOCKCOMMENT, 0, 0);
}

void on_comment_box (GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_BOXCOMMENT, 0, 0);
}

void on_comment_stream (GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_STREAMCOMMENT, 0, 0);
}

void
on_goto_activate (GtkAction *action, gpointer user_data)
{
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	anjuta_ui_activate_action_by_path (plugin->ui,
		"ActionGroupNavigation/ActionEditGotoLineEntry");
}

void
on_toolbar_goto_clicked (GtkAction *action, gpointer user_data)
{
	EditorPlugin *plugin;
	AnjutaUI *ui;
	AnjutaDocman *docman;
	TextEditor *te;
	guint line;
	const gchar *line_ascii;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	ui = ANJUTA_UI (ANJUTA_PLUGIN (plugin));
	
	if (EGG_IS_ENTRY_ACTION (action))
	{
		line_ascii = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	else
	{
		GtkAction *entry_action;
		entry_action = anjuta_ui_get_action (ui, "ActionNavigation",
									   "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (action));
		line_ascii = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	if (strlen (line_ascii) == 0)
		return;
	
	if (te)
	{
		line = atoi (line_ascii);
		if (text_editor_goto_line (te, line, TRUE, TRUE) == FALSE)
		{
			GtkWidget *parent;
			parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
			anjuta_util_dialog_error (GTK_WINDOW (parent),
									  _("There is no line number %d in \"%s\"."),
									  line, te->filename);
		}
	}
}

void
on_goto_line_no1_activate (GtkAction * action, gpointer user_data)
{
	GtkWidget *gt;
	EditorPlugin *plugin;
	AnjutaDocman *docman;
	TextEditor *te;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	
	if (te)
	{
		gt = gotoline_new (te);
		gtk_widget_show (gt);
	}
}

void
on_goto_block_start1_activate (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	text_editor_goto_block_start(te);
}

void
on_goto_block_end1_activate (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	text_editor_goto_block_end(te);
}

void
on_editor_linenos1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_LINENUMBERS_MARGIN, state);
}

void
on_editor_markers1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_MARKER_MARGIN, state);
}

void
on_editor_codefold1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_FOLD_MARGIN, state);
}

void
on_editor_indentguides1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_INDENTATION_GUIDES, state);
}

void
on_editor_whitespaces1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_WHITE_SPACES, state);
}

void
on_editor_eolchars1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_EOL, state);
}

void
on_editor_linewrap1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_LINE_WRAP, state);
}

#define MAX_ZOOM_FACTOR 8
#define MIN_ZOOM_FACTOR -8

static void
on_zoom_text_activate (GtkAction * action, const gchar *zoom_text,
					   EditorPlugin *plugin)
{
#if 0
	gint zoom;
	gchar buf[20];
	
	if (!zoom_text)
		zoom = 0;
	else if (0 == strncmp(zoom_text, "++", 2))
		zoom = prop_get_int(plugin->prefs->props, TEXT_ZOOM_FACTOR, 0) + 2;
	else if (0 == strncmp(zoom_text, "--", 2))
		zoom = prop_get_int(plugin->prefs->props, TEXT_ZOOM_FACTOR, 0) - 2;
	else
		zoom = atoi(zoom_text);
	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;
	g_snprintf(buf, 20, "%d", zoom);
	prop_set_with_key (plugin->prefs->props, TEXT_ZOOM_FACTOR, buf);
	// FIXME: anjuta_docman_set_zoom_factor(zoom);
#endif
}

void
on_zoom_in_text_activate (GtkAction * action, gpointer user_data)
{
	on_zoom_text_activate (action, "++", user_data);
}

void
on_zoom_out_text_activate (GtkAction * action, gpointer user_data)
{
	on_zoom_text_activate (action, "--", user_data);
}

void
on_force_hilite_activate (GtkWidget *menuitem, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	const gchar *file_extension;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	file_extension = g_object_get_data (G_OBJECT (menuitem), "file_extension");
	
	if (te == NULL)
		return;
	text_editor_set_hilite_type (te, file_extension);
	text_editor_hilite (te, TRUE);
}

void
on_indent1_activate (GtkAction * action, gpointer user_data)
{
    //trying to restore line no where i was before autoformat invoked
    gint lineno;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
    lineno = aneditor_command (te->editor_id, ANE_GET_LINENO, 0, 0);
	if (te == NULL)
		return;
	text_editor_autoformat (te);
	// FIXME: anjuta_update_title ();
    text_editor_goto_line (te, lineno+1, TRUE, FALSE);
}

/*  *user_data : TRUE=Forward  False=Backward  */
void
on_findnext1_activate (GtkAction * action, gpointer user_data)
{
	search_replace_next();
}

void
on_findprevious1_activate (GtkAction * action, gpointer user_data)
{
	search_replace_previous();
}

void
on_enterselection (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	GtkAction *entry_action;
	TextEditor *te;
	gchar *selectionText = NULL;
	GSList *proxies;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (!te) return;
	
	entry_action = anjuta_ui_get_action (plugin->ui, "ActionGroupNavigation",
									   "ActionEditSearchEntry");
	g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
	
	selectionText = text_editor_get_selection (te);
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

void
on_format_indent_style_clicked (GtkAction * action, gpointer user_data)
{
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	gtk_signal_emit_by_name (GTK_OBJECT (plugin->prefs),
										 "activate");
}

/* Incremental search */

typedef struct
{
	gint pos;
	gboolean wrap;
	
} IncrementalSearch;

gboolean
on_toolbar_find_incremental_start (GtkAction *action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	IncrementalSearch *search_params;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);

	if (!te) return FALSE;
	search_params = g_object_get_data (G_OBJECT (te), "incremental_search");
	if (!search_params)
	{
		search_params = g_new0 (IncrementalSearch, 1);
		g_object_set_data_full (G_OBJECT (te), "incremental_search",
								search_params, (GDestroyNotify)g_free);
	}
	/* Prepare to begin incremental search */	
	search_params->pos = text_editor_get_current_position(te);
	search_params->wrap = FALSE;
	return FALSE;
}

gboolean
on_toolbar_find_incremental_end (GtkAction *action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	IncrementalSearch *search_params;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);

	if (!te) return FALSE;
	search_params = g_object_get_data (G_OBJECT (te), "incremental_search");
	if (search_params)
	{
		search_params->pos = -1;
		search_params->wrap = FALSE;
	}
	return FALSE;
}

void
on_toolbar_find_incremental (GtkAction *action, gpointer user_data)
{
	const gchar *entry_text;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	IncrementalSearch *search_params;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	
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
		entry_action = anjuta_ui_get_action (ui, "ActionGroupNavigation",
											 "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
		entry_text =
			egg_entry_action_get_text (EGG_ENTRY_ACTION (entry_action));
	}
	if (!entry_text || strlen(entry_text) < 1) return;
	
	text_editor_goto_point (te, search_params->pos);
	on_toolbar_find_clicked (NULL, user_data);
}

static void
on_toolbar_find_start_over (GtkAction * action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	
	/* search from doc start */
	text_editor_goto_point (te, 0);
	on_toolbar_find_clicked (action, user_data);
}

void
on_toolbar_find_clicked (GtkAction *action, gpointer user_data)
{
	const gchar *string;
	gint ret;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	IncrementalSearch *search_params;
	gboolean search_wrap = FALSE;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
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
		entry_action = anjuta_ui_get_action (plugin->ui,
											 "ActionGroupNavigation",
											 "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
		string = egg_entry_action_get_text (EGG_ENTRY_ACTION (entry_action));
	}
	if (search_params->pos >= 0 && search_params->wrap)
	{
		/* If incremental search wrap requested, so wrap it. */
		search_wrap = TRUE;
	}
	ret = text_editor_find (te, string,
							TEXT_EDITOR_FIND_SCOPE_CURRENT,
							TRUE, /* Forward */
							FALSE, TRUE, FALSE, search_wrap);
	if (ret < 0) {
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
#if 0 /* FIXME */
				anjuta_status(
				"Failling I-Search: '%s'. Press Enter or click Find to overwrap.",
				string);
				search_params->wrap = 1;
				if (anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
											BEEP_ON_BUILD_COMPLETE))
#endif
					gdk_beep();
			}
#if 0 /* FIXME */
			else
			{
				anjuta_status ("Failling Overwrapped I-Search: %s.", string);
			}
#endif
		}
	}
}

void
on_calltip1_activate (GtkAction * action, gpointer user_data)
{
}

/* Gets the swapped (c/h) file names */
static gchar*
get_swapped_filename (const gchar* filename)
{
	size_t len;
	gchar *newfname;
	GnomeVFSURI *vfs_uri;

	g_return_val_if_fail (filename != NULL, NULL);

	len = strlen (filename);
	newfname = g_malloc (len+5); /* to be on the safer side */
	while (len)
	{
		if (filename[len] == '.')
			break;
		len--;
	}
	len++;
	strcpy (newfname, filename);
	if (strncasecmp (&filename[len], "h", 1) == 0)
	{
		strcpy (&newfname[len], "cc");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
		
		strcpy (&newfname[len], "cpp");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
		
		strcpy (&newfname[len], "cxx");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
		
		strcpy (&newfname[len], "c");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{ 
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
	}
	else if (strncasecmp (&filename[len], "c", 1)==0 )
	{
		strcpy (&newfname[len], "h");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{ 
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
		
		strcpy (&newfname[len], "hh");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
		
		strcpy (&newfname[len], "hxx");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
		
		strcpy (&newfname[len], "hpp");
		vfs_uri = gnome_vfs_uri_new (newfname);
		if (gnome_vfs_uri_exists (vfs_uri))
		{
			gnome_vfs_uri_unref (vfs_uri);
			return newfname;
		}
		gnome_vfs_uri_unref (vfs_uri);
	}
	g_free (newfname);	
	return NULL;
}

void
on_swap_activate (GtkAction *action, gpointer user_data)
{
	gchar *newfname;
	const gchar *uri;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (!te)
		return;
	uri = te->uri;
	if (!uri)
		return;
	newfname = get_swapped_filename (uri);
	if (newfname)
	{
		anjuta_docman_goto_file_line (docman, newfname, -1);
		g_free (newfname);
	}
	return;
}

void
on_editor_add_view_activate (GtkAction *action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (!te)
		return;
	text_editor_add_view (te);
}

void
on_editor_remove_view_activate (GtkAction *action, gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (!te)
		return;
	text_editor_remove_view (te);
}

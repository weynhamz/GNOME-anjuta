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

#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-utils.h>

#include <libegg/menu/egg-entry-action.h>

#include "anjuta-docman.h"
#include "action-callbacks.h"
#include "text_editor.h"
#include "file.h"
#include "search-replace.h"
#include "search-replace_backend.h"
#include "plugin.h"
#include "goto_line.h"
#include "print.h"
#include "lexer.h"

gboolean closing_state;

void
on_new_file1_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);

	display_new_file(docman);
}

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
	anjuta_docman_save_as_file (docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	if (te->full_filename == NULL)
	{
		anjuta_docman_set_current_editor (docman, te);
		// FIXME: gtk_widget_show (app->save_as_fileselection);
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
	// anjuta_docman_save_all_files();
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
		next = node->next; // Save it now, as we may change it.
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
		// anjuta_update_title ();
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
	// glong mode;
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
on_insert_c_gpl_notice(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_c_gpl_notice(te);
}

void
on_insert_cpp_gpl_notice(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cpp_gpl_notice(te);
}

void
on_insert_py_gpl_notice(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_py_gpl_notice(te);
}

void
on_insert_username(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_username(te);
}

void
on_insert_changelog_entry(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_changelog_entry(te);
}

void
on_insert_date_time(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_date_time(te);
}

void
on_insert_header_template(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_header_template(te);
}

void
on_insert_header(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_header(te);
}

void
on_insert_switch_template(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_switch_template(te);
}

void
on_insert_for_template(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_for_template(te);
}

void
on_insert_while_template(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_while_template(te);
}

void
on_insert_ifelse_template(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_ifelse_template(te);
}

void
on_insert_cvs_author(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_author(te);
}

void
on_insert_cvs_date(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_date(te);
}

void
on_insert_cvs_header(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_header(te);
}

void
on_insert_cvs_id(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_id(te);
}

void
on_insert_cvs_log(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_log(te);
}

void
on_insert_cvs_name(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_name(te);
}

void
on_insert_cvs_revision(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_revision(te);
}

void
on_insert_cvs_source(GtkAction * action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
	insert_cvs_source(te);
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

void on_insert_custom_indent (GtkAction *action, gpointer user_data)
{
    TextEditor* te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if(!te) return;
    aneditor_command (te->editor_id, ANE_CUSTOMINDENT, 0, 0);
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
	
	gt = gotoline_new (te);
	gtk_widget_show (gt);
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
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								"margin.linenumber.visible", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_LINENUMBERMARGIN, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_markers1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	anjuta_preferences_set_int (plugin->prefs,
								"margin.marker.visible", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_SELMARGIN, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_codefold1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								"margin.fold.visible", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_FOLDMARGIN, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_indentguides1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								"view.indentation.guides", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWGUIDES, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_whitespaces1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								"view.whitespace", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWSPACE, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_eolchars1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								"view.eol", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWEOL, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_linewrap1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	GList *node, *editors;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	editors = anjuta_docman_get_all_editors (docman);
	
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								"view.line.wrap", state);
	node = editors;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_LINEWRAP, state, 0);
		node = g_list_next (node);
	}
}

#define MAX_ZOOM_FACTOR 8
#define MIN_ZOOM_FACTOR -8

static void
on_zoom_text_activate (GtkAction * action, const gchar *zoom_text,
					   EditorPlugin *plugin)
{
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

static void
on_force_hilite1_activate (GtkAction * action, gint highlight_type,
						   gpointer user_data)
{
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	te->force_hilite = (gint) user_data;
	text_editor_set_hilite_type (te);
}

void on_force_hilite1_auto_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_AUTOMATIC, user_data);
}

void on_force_hilite1_none_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_NONE, user_data);
}

void on_force_hilite1_cpp_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_CPP, user_data);
}

void on_force_hilite1_html_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_HTML, user_data);
}

void on_force_hilite1_xml_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_XML, user_data);
}

void on_force_hilite1_js_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_JS, user_data);
}

void on_force_hilite1_wscript_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_WSCRIPT, user_data);
}

void on_force_hilite1_make_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_MAKE, user_data);
}

void on_force_hilite1_java_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_JAVA, user_data);
}

void on_force_hilite1_lua_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_LUA, user_data);
}

void on_force_hilite1_python_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_PYTHON, user_data);
}

void on_force_hilite1_perl_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_PERL, user_data);
}

void on_force_hilite1_sql_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_SQL, user_data);
}

void on_force_hilite1_plsql_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_PLSQL, user_data);
}

void on_force_hilite1_php_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_PHP, user_data);
}

void on_force_hilite1_latex_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_LATEX, user_data);
}

void on_force_hilite1_diff_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_DIFF, user_data);
}

void on_force_hilite1_pascal_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_PASCAL, user_data);
}

void on_force_hilite1_xcode_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_XCODE, user_data);
}

void on_force_hilite1_props_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_PROPS, user_data);
}

void on_force_hilite1_conf_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_CONF, user_data);
}

void on_force_hilite1_ada_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_ADA, user_data);
}

void on_force_hilite1_baan_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_BAAN, user_data);
}

void on_force_hilite1_lisp_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_LISP, user_data);
}

void on_force_hilite1_ruby_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_RUBY, user_data);
}

void on_force_hilite1_matlab_activate (GtkAction * action, gpointer user_data)
{
	on_force_hilite1_activate (action, TE_LEXER_MATLAB, user_data);
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
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	// FIXME: enter_selection_as_search_target();
	
	entry_action = anjuta_ui_get_action (plugin->ui, "ActionNavigation",
								   "ActionEditSearchEntry");
	g_return_if_fail (EGG_IS_ENTRY_ACTION (entry_action));
	// line_ascii = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	// FIXME: gtk_widget_grab_focus (entry_action);
}

void
on_format_indent_style_clicked (GtkAction * action, gpointer user_data)
{
	EditorPlugin *plugin;
	plugin = (EditorPlugin *) user_data;
	gtk_signal_emit_by_name (GTK_OBJECT (plugin->prefs),
										 "activate");
}

gboolean
on_toolbar_find_incremental_start (GtkAction *action, gpointer user_data)
{
	// gchar *string;
	// const gchar *string1;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	// EggEntryAction *entry_action;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);

	if (!te) return FALSE;
#if 0 //FIXME:
	/* Updated find combo history */
	string1 =
		gtk_entry_get_text (GTK_ENTRY
				    (app->toolbar.main_toolbar.find_entry));
	if (string1 && strlen (string1) > 0)
	{
		string = g_strdup (string1);
		app->find_replace->find_text->find_history =
			update_string_list (app->find_replace->find_text->
						find_history, string, COMBO_LIST_LENGTH);
		gtk_combo_set_popdown_strings (GTK_COMBO
						   (app->toolbar.main_toolbar.find_combo),
						   app->find_replace->find_text->find_history);
		g_free (string);
	}
	/* Prepare to begin incremental search */	
	app->find_replace->find_text->incremental_pos =
		text_editor_get_current_position(te);
	app->find_replace->find_text->incremental_wrap = FALSE;
#endif 
	return FALSE;
}

gboolean
on_toolbar_find_incremental_end (GtkAction *action, gpointer user_data)
{
#if 0 /* Ambiguity during merge */
	app->find_replace->find_text->incremental_pos = -1;
	if (EGG_IS_ENTRY_ACTION (action))
	{
		string1 = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	else
	{
		EggEntryAction *entry_action;
		AnjutaUI *ui = ANJUTA_ui (g_object_get_data (G_BOJECT (user_data), "ui"));
		action = anjuta_ui_get_action (ui, "ActionGroupNavigation",
									   "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (action));
		string1 = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	if (!string1 || strlen (string1) == 0)
		return FALSE;
	string = g_strdup (string1);
	app->find_replace->find_text->find_history =
		update_string_list (app->find_replace->find_text->
				    find_history, string, COMBO_LIST_LENGTH);
	/*
	gtk_combo_set_popdown_strings (GTK_COMBO
				       (app->widgets.toolbar.main_toolbar.
					find_combo),
				       app->find_replace->find_text->
				       find_history);
	*/
	g_free (string);
#endif
	return FALSE;
}

void
on_toolbar_find_incremental (GtkAction *action, gpointer user_data)
{
#if 0
	const gchar *entry_text;
	TextEditor *te = anjuta_get_current_text_editor();
	if (!te)
		return;
	if (app->find_replace->find_text->incremental_pos < 0)
		return;
	
	text_editor_goto_point (te, app->find_replace->find_text->incremental_pos);

	if (EGG_IS_ENTRY_ACTION (action))
	{
		entry_text = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	else
	{
		EggEntryAction *entry_action;
		AnjutaUI *ui = ANJUTA_ui (g_object_get_data (G_BOJECT (user_data), "ui"));
		action = anjuta_ui_get_action (ui, "ActionGroupNavigation",
									   "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (action));
		entry_text = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	if (!entry_text || strlen(entry_text) < 1) return;
	
	/* 2 is passed to indicate the call is from incremental and at the
	   same time the search is forward */
	on_toolbar_find_clicked (NULL, GINT_TO_POINTER(2));
#endif
}

#if 0 //FIXME:
/*  *user_data : TRUE=Forward  False=Backward  */
static void
on_toolbar_find_start_over (GtkAction * action, gpointer user_data)
{
	long length;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	
	length = aneditor_command(te->editor_id, ANE_GETLENGTH, 0, 0);

	if (GPOINTER_TO_INT(user_data))
		/* search from doc start */
		text_editor_goto_point (te, 0);
	else
		/* search from doc end */
		text_editor_goto_point (te, length);

	on_toolbar_find_clicked (action, user_data);
}
#endif

/*  *user_data : TRUE=Forward  False=Backward  */
void
on_toolbar_find_clicked (GtkAction * action, gpointer user_data)
{
	const gchar *string;
	// gint ret;
	TextEditor *te;
	AnjutaDocman *docman;
	EditorPlugin *plugin;
	// gboolean search_wrap = FALSE;
	
	plugin = (EditorPlugin *) user_data;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (!te)
		return;

	if (EGG_IS_ENTRY_ACTION (action))
	{
		string = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
	else
	{
		GtkAction *entry_action;
		entry_action = anjuta_ui_get_action (plugin->ui, "ActionGroupNavigation",
									   "ActionEditSearchEntry");
		g_return_if_fail (EGG_IS_ENTRY_ACTION (action));
		string = egg_entry_action_get_text (EGG_ENTRY_ACTION (action));
	}
#if 0 //FIXME:	
	/* The 2 below is checked to make sure the wrapping is only done when
	  it is called by 'activate' (and not 'changed') signal */ 
	if (app->find_replace->find_text->incremental_pos >= 0 &&
		(app->find_replace->find_text->incremental_wrap == 2 ||
		 (app->find_replace->find_text->incremental_wrap == 1 &&
		  GPOINTER_TO_INT (user_data) < 2)))
	{
		/* If incremental search wrap requested, so wrap it. */
		search_wrap = TRUE;
		app->find_replace->find_text->incremental_wrap = 2;
	}
	if (app->find_replace->find_text->incremental_pos >= 0)
	{
		/* If incremental search */
		ret = text_editor_find (te, string,
					TEXT_EDITOR_FIND_SCOPE_CURRENT,
					GPOINTER_TO_INT(user_data), /* Forward - Backward */
					FALSE, TRUE, FALSE, search_wrap);
	}
	else
	{
		/* Normal search */
		ret = text_editor_find (te, string,
					TEXT_EDITOR_FIND_SCOPE_CURRENT,
					GPOINTER_TO_INT(user_data), /* Forward - Backward */
					app->find_replace->find_text->regexp,
					app->find_replace->find_text->ignore_case,
					app->find_replace->find_text->whole_word,
					FALSE);
	}
	if (ret < 0) {
		if (app->find_replace->find_text->incremental_pos < 0)
		{
			GtkWidget *dialog;
			// Dialog to be made HIG compliant.
			dialog = gtk_message_dialog_new (GTK_WINDOW (app),
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
				anjuta_status(
				"Failling I-Search: '%s'. Press Enter or click Find to overwrap.",
				string);
				app->find_replace->find_text->incremental_wrap = TRUE;
				if (anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
											BEEP_ON_BUILD_COMPLETE))
					gdk_beep();
			}
			else
			{
				anjuta_status ("Failling Overwrapped I-Search: %s.", string);
			}
		}
	}
#endif
}

void
on_calltip1_activate (GtkAction * action, gpointer user_data)
{
}

void
on_detach1_activate  (GtkAction * action, gpointer user_data)
{
}

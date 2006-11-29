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

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>
#include <libanjuta/interfaces/ianjuta-editor-line-mode.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-view.h>
#include <libanjuta/interfaces/ianjuta-editor-folds.h>
#include <libanjuta/interfaces/ianjuta-bookmark.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-print.h>
#include <libanjuta/interfaces/ianjuta-editor-comment.h>
#include <libanjuta/interfaces/ianjuta-editor-zoom.h>
#include <libanjuta/interfaces/ianjuta-editor-goto.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>

#include <libegg/menu/egg-entry-action.h>

#include <sys/wait.h>
#include <sys/stat.h>

#include "anjuta-docman.h"
#include "action-callbacks.h"
#include "plugin.h"
#include "file_history.h"
#include "goto_line.h"

#define AUTOFORMAT_DISABLE         "autoformat.disable"
#define AUTOFORMAT_STYLE           "autoformat.style"
#define AUTOFORMAT_LIST_STYLE      "autoformat.list.style"
#define AUTOFORMAT_OPTS            "autoformat.opts"

void
static editor_autoformat (IAnjutaEditor *te, AnjutaPreferences* prefs)
{
	gchar *cmd,*dir;
	gchar *indent_style = NULL;
	gchar *fopts = NULL;
	pid_t pid;
	int status;
	const gchar* file;

	if (anjuta_util_prog_is_installed ("indent", TRUE) == FALSE)
		return;
	if (anjuta_preferences_get_int (prefs, AUTOFORMAT_DISABLE))
	{
		GtkWindow *parent;
		parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (te)));
		anjuta_util_dialog_warning (parent, _("Auto format is currently disabled. Change the setting in Preferences."));
		return;
	}
	if (te == NULL)
		return;

	file = ianjuta_editor_get_filename(IANJUTA_EDITOR(te), NULL);
	
	if (!anjuta_preferences_get_pair (prefs, AUTOFORMAT_STYLE,
                                 GCONF_VALUE_STRING, GCONF_VALUE_STRING,
                                 &indent_style, &fopts))
		return;
	
	if (!fopts)
	{
		gchar *msg;
		msg = g_strdup_printf (_("Anjuta does not know %s!"), indent_style);
		anjuta_util_dialog_warning (NULL, msg);
		g_free(msg);
		return;
	}
	cmd = g_strconcat ("indent ", fopts, " ", file, NULL);
	g_free (fopts);
	dir = g_path_get_dirname (file);
	pid = anjuta_util_execute_shell (dir, cmd);
	g_free (dir);
	g_free (cmd);

	waitpid (pid, &status, 0);
}

static IAnjutaEditor*
get_current_editor(gpointer user_data)
{
	DocmanPlugin* plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	AnjutaDocman* docman = ANJUTA_DOCMAN(plugin->docman);
	return IANJUTA_EDITOR(anjuta_docman_get_current_editor(docman));
}

void
on_open_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	anjuta_docman_open_file (docman);
}

void
on_save_activate (GtkAction *action, gpointer user_data)
{
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	
	anjuta_docman_save_editor (docman, te, NULL);
}

void
on_save_as_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	IAnjutaEditor *te;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	
	anjuta_docman_save_editor_as (docman, te, NULL);
}

void
on_save_all_activate (GtkAction * action, gpointer user_data)
{
	GList *node;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	node = anjuta_docman_get_all_editors (docman);
	while (node)
	{
		IAnjutaEditor* te;
		GList* next;
		te = node->data;
		next = node->next;
		if(te)
		{
			if (ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(te), NULL) && 
				ianjuta_file_get_uri(IANJUTA_FILE(te), NULL))
			{
				ianjuta_file_savable_save(IANJUTA_FILE_SAVABLE(te), NULL);
			}
		}
		node = next;
	}
}

void
on_close_file_activate (GtkAction * action, gpointer user_data)
{
	AnjutaDocman *docman;
	IAnjutaEditor *te;
	GtkWidget *parent;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (te));

	if (ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE (te), NULL))
	{
		gchar *mesg;
		GtkWidget *dialog;
		gint res;
		
		mesg = g_strdup_printf (_("The file '%s' is not saved.\n"
								"Do you want to save it before closing?"),
								ianjuta_editor_get_filename(te, NULL));
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
		
		gtk_widget_destroy (dialog);
		if (res == GTK_RESPONSE_YES)
		{
			anjuta_docman_save_editor (docman, te, NULL);
			anjuta_docman_remove_editor (docman, te);
		}
		else if (res == GTK_RESPONSE_NO)
		{
			anjuta_docman_remove_editor (docman, te);
		}
	}
	else
		anjuta_docman_remove_editor (docman, te);
}

void
on_close_all_file_activate (GtkAction * action, gpointer user_data)
{
	GList *node;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	/* Close all 'saved' files */
	node = anjuta_docman_get_all_editors (docman);
	while (node)
	{
		IAnjutaEditor* te;
		GList* next;
		te = node->data;
		next = node->next; /* Save it now, as we may change it. */
		if(te)
		{
			if (!ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE (te), NULL))
			{
				anjuta_docman_remove_editor (docman, te);
			}
		}
		node = next;
	}
}

void
on_reload_file_activate (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *te;
	gchar mesg[256];
	GtkWidget *dialog;
	GtkWidget *parent;
	DocmanPlugin *plugin;
	AnjutaDocman *docman;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;

	sprintf (mesg, _("Are you sure you want to reload '%s'?\n"
					 "Any unsaved changes will be lost."),
			 ianjuta_editor_get_filename(te, NULL));

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
		const gchar* uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
		ianjuta_file_open(IANJUTA_FILE(te), uri, NULL);
		/* FIXME: anjuta_update_title (); */
	}
	gtk_widget_destroy (dialog);
}

void
anjuta_print_cb (GtkAction *action, gpointer user_data)
{
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	ianjuta_print_print (IANJUTA_PRINT(te), NULL);
}

void
anjuta_print_preview_cb (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	ianjuta_print_print_preview (IANJUTA_PRINT(te), NULL);
}

void
on_editor_command_upper_case_activate (GtkAction * action, gpointer data)
{
	IAnjutaEditor *editor;
	gint start, end;
	
	editor = get_current_editor (data);
	start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor), NULL);
	end = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor), NULL);
	ianjuta_editor_convert_to_upper (IANJUTA_EDITOR_CONVERT (editor), start, end, NULL);
}

void
on_editor_command_lower_case_activate (GtkAction * action, gpointer data)
{
	IAnjutaEditor *editor;
	gint start, end;
	
	editor = get_current_editor (data);
	start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor), NULL);
	end = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor), NULL);
	ianjuta_editor_convert_to_lower (IANJUTA_EDITOR_CONVERT (editor), start, end, NULL);
}

void
on_editor_command_eol_crlf_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_line_mode_convert (IANJUTA_EDITOR_LINE_MODE (get_current_editor(data)),
									  IANJUTA_EDITOR_LINE_MODE_CRLF, NULL);
}

void
on_editor_command_eol_lf_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_line_mode_convert (IANJUTA_EDITOR_LINE_MODE (get_current_editor(data)),
									  IANJUTA_EDITOR_LINE_MODE_LF, NULL);
}

void
on_editor_command_eol_cr_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_line_mode_convert (IANJUTA_EDITOR_LINE_MODE (get_current_editor(data)),
									  IANJUTA_EDITOR_LINE_MODE_CR, NULL);
}

void
on_editor_command_select_all_activate (GtkAction * action, gpointer data)
{
	GtkWidget *widget=NULL;
	GtkWidget *main_window=NULL;
	
	main_window = GTK_WIDGET (gtk_window_list_toplevels ()->data);
	widget = gtk_window_get_focus (GTK_WINDOW(main_window));
   
	if (widget && GTK_IS_EDITABLE (widget))
	{
		gtk_editable_select_region (GTK_EDITABLE (widget), 0, -1);
	}
	else
	{
		ianjuta_editor_selection_select_all (IANJUTA_EDITOR_SELECTION (get_current_editor(data)),
																	   NULL);
	}
}

void
on_editor_command_select_to_brace_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_selection_select_to_brace (IANJUTA_EDITOR_SELECTION (get_current_editor(data)), NULL);
}

void
on_editor_command_select_block_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_selection_select_block (IANJUTA_EDITOR_SELECTION (get_current_editor(data)), NULL);
}

void
on_editor_command_match_brace_activate (GtkAction * action, gpointer data)
{
	// FIXME:
	// on_editor_command_activate (action, ANE_MATCHBRACE, data);
}

void
on_editor_command_undo_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_undo (get_current_editor(data), NULL);
}

void
on_editor_command_redo_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_redo (get_current_editor(data), NULL);
}

void
on_editor_command_cut_activate (GtkAction * action, gpointer data)
{
	GtkWidget *widget;
	GtkWidget *main_window;
	
	main_window = GTK_WIDGET (gtk_window_list_toplevels ()->data);
	widget = gtk_window_get_focus (GTK_WINDOW(main_window));

	if (widget && GTK_IS_EDITABLE (widget))
	{
		gtk_editable_cut_clipboard (GTK_EDITABLE (widget));
	}
	else
	{
		ianjuta_editor_selection_cut (IANJUTA_EDITOR_SELECTION (get_current_editor(data)), NULL);
	}
}

void
on_editor_command_copy_activate (GtkAction * action, gpointer data)
{	
	GtkWidget *widget;
	GtkWidget *main_window;
	
	main_window = GTK_WIDGET (gtk_window_list_toplevels ()->data);
	widget = gtk_window_get_focus (GTK_WINDOW(main_window));

	if (widget && GTK_IS_EDITABLE (widget))
	{
		gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
	}
	else
	{
		ianjuta_editor_selection_copy (IANJUTA_EDITOR_SELECTION (get_current_editor(data)), NULL);
	}
}

void
on_editor_command_paste_activate (GtkAction * action, gpointer data)
{
	GtkWidget *widget;
	GtkWidget *main_window;
	
	main_window = GTK_WIDGET (gtk_window_list_toplevels ()->data);
	widget = gtk_window_get_focus (GTK_WINDOW(main_window));

	if (widget && GTK_IS_EDITABLE (widget))
	{
		gtk_editable_paste_clipboard (GTK_EDITABLE (widget));
	}
	else
	{
		ianjuta_editor_selection_paste (IANJUTA_EDITOR_SELECTION (get_current_editor(data)), NULL);
	}
}

void
on_editor_command_clear_activate (GtkAction * action, gpointer data)
{
	GtkWidget *widget;
	GtkWidget *main_window;
	gint start, end;
	
	main_window = GTK_WIDGET (gtk_window_list_toplevels ()->data);
	widget = gtk_window_get_focus (GTK_WINDOW(main_window));

	if (widget && GTK_IS_EDITABLE (widget))
	{
	
		if (!gtk_editable_get_selection_bounds (GTK_EDITABLE (widget), &start, &end))
		{
			start=gtk_editable_get_position (GTK_EDITABLE (widget));
			end=start+1;
		}

		gtk_editable_delete_text (GTK_EDITABLE (widget), start, end);

	}
	else
	{
		ianjuta_editor_selection_clear (IANJUTA_EDITOR_SELECTION (get_current_editor(data)), NULL);
	}
}

void
on_editor_command_complete_word_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_assist_autocomplete (IANJUTA_EDITOR_ASSIST (get_current_editor(data)), NULL);
}

void on_editor_command_close_folds_all_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_folds_close_all (IANJUTA_EDITOR_FOLDS (get_current_editor(data)), NULL);
}

void on_editor_command_open_folds_all_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_folds_open_all (IANJUTA_EDITOR_FOLDS (get_current_editor(data)), NULL);
}

void on_editor_command_toggle_fold_activate (GtkAction * action, gpointer data)
{
	ianjuta_editor_folds_toggle_current (IANJUTA_EDITOR_FOLDS (get_current_editor(data)), NULL);
}

void on_editor_command_bookmark_toggle_activate (GtkAction * action, gpointer data)
{
	ianjuta_bookmark_toggle (IANJUTA_BOOKMARK (get_current_editor (data)),
							 ianjuta_editor_get_lineno (get_current_editor (data), NULL),
							 FALSE, NULL);
}

void on_editor_command_bookmark_first_activate (GtkAction * action, gpointer data)
{
	ianjuta_bookmark_first (IANJUTA_BOOKMARK (get_current_editor (data)), NULL);
}

void on_editor_command_bookmark_next_activate (GtkAction * action, gpointer data)
{
	ianjuta_bookmark_next (IANJUTA_BOOKMARK (get_current_editor (data)), NULL);
}

void on_editor_command_bookmark_prev_activate (GtkAction * action, gpointer data)
{
	ianjuta_bookmark_previous (IANJUTA_BOOKMARK (get_current_editor (data)), NULL);
}

void on_editor_command_bookmark_last_activate (GtkAction * action, gpointer data)
{
	ianjuta_bookmark_last (IANJUTA_BOOKMARK (get_current_editor (data)), NULL);
}

void on_editor_command_bookmark_clear_activate (GtkAction * action, gpointer data)
{
	ianjuta_bookmark_clear_all (IANJUTA_BOOKMARK (get_current_editor (data)), NULL);
}

void
on_transform_eolchars1_activate (GtkAction * action, gpointer user_data)
{
	ianjuta_editor_line_mode_fix (IANJUTA_EDITOR_LINE_MODE (get_current_editor(user_data)),
								  NULL);
}

void
on_prev_history (GtkAction *action, gpointer user_data)
{
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	an_file_history_back (docman);
}

void
on_next_history (GtkAction *action, gpointer user_data)
{
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	an_file_history_forward (docman);
}

void
on_comment_block (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor* te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	ianjuta_editor_comment_block(IANJUTA_EDITOR_COMMENT(te), NULL);
}

void on_comment_box (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor* te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	ianjuta_editor_comment_box(IANJUTA_EDITOR_COMMENT(te), NULL);
}

void on_comment_stream (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor* te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	ianjuta_editor_comment_stream(IANJUTA_EDITOR_COMMENT(te), NULL);
}

void
on_goto_activate (GtkAction *action, gpointer user_data)
{
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	anjuta_ui_activate_action_by_path (plugin->ui,
		"ActionGroupNavigation/ActionEditGotoLineEntry");
}

void
on_toolbar_goto_clicked (GtkAction *action, gpointer user_data)
{
	DocmanPlugin *plugin;
	AnjutaUI *ui;
	AnjutaShell* shell;
	AnjutaDocman *docman;
	IAnjutaEditor *te;
	guint line;
	const gchar *line_ascii;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
	ui = anjuta_shell_get_ui(shell, NULL);
	
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
		ianjuta_editor_goto_line(te, line, NULL);
		/* FIXME
		if (text_editor_goto_line (te, line, TRUE, TRUE) == FALSE)
		{
			GtkWidget *parent;
			parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
			anjuta_util_dialog_error (GTK_WINDOW (parent),
									  _("There is no line number %d in \"%s\"."),
									  line, te->filename);
		}*/
	}
}

void
on_goto_line_no1_activate (GtkAction * action, gpointer user_data)
{
	GtkWidget *gt;
	DocmanPlugin *plugin;
	AnjutaDocman *docman;
	IAnjutaEditor *te;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
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
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	ianjuta_editor_goto_start_block(IANJUTA_EDITOR_GOTO(te), NULL);
}

void
on_goto_block_end1_activate (GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	ianjuta_editor_goto_end_block(IANJUTA_EDITOR_GOTO(te), NULL);
}

#define VIEW_LINENUMBERS_MARGIN    "margin.linenumber.visible"
#define VIEW_MARKER_MARGIN         "margin.marker.visible"
#define VIEW_FOLD_MARGIN           "margin.fold.visible"
#define VIEW_INDENTATION_GUIDES    "view.indentation.guides"
#define VIEW_WHITE_SPACES          "view.whitespace"
#define VIEW_EOL                   "view.eol"
#define VIEW_LINE_WRAP             "view.line.wrap"

void
on_editor_linenos1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_LINENUMBERS_MARGIN, state);
}

void
on_editor_markers1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_MARKER_MARGIN, state);
}

void
on_editor_codefold1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_FOLD_MARGIN, state);
}

void
on_editor_indentguides1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_INDENTATION_GUIDES, state);
}

void
on_editor_whitespaces1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_WHITE_SPACES, state);
}

void
on_editor_eolchars1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_EOL, state);
}

void
on_editor_linewrap1_activate (GtkAction * action, gpointer user_data)
{
	gboolean state;
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	anjuta_preferences_set_int (plugin->prefs,
								VIEW_LINE_WRAP, state);
}

void
on_zoom_in_text_activate (GtkAction * action, gpointer user_data)
{
	DocmanPlugin *plugin;
	AnjutaDocman *docman;
	IAnjutaEditor *te;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	
	if (te == NULL)
		return;
	ianjuta_editor_zoom_in (IANJUTA_EDITOR_ZOOM (te), NULL);
}

void
on_zoom_out_text_activate (GtkAction * action, gpointer user_data)
{
	DocmanPlugin *plugin;
	AnjutaDocman *docman;
	IAnjutaEditor *te;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	
	if (te == NULL)
		return;
	ianjuta_editor_zoom_out (IANJUTA_EDITOR_ZOOM (te), NULL);
}

void
on_force_hilite_activate (GtkWidget *menuitem, gpointer user_data)
{
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	const gchar *language_code;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	language_code = g_object_get_data (G_OBJECT (menuitem), "language_code");
	
	if (te == NULL)
		return;
	if (language_code && IANJUTA_IS_EDITOR_LANGUAGE (te))
		ianjuta_editor_language_set_language (IANJUTA_EDITOR_LANGUAGE (te),
											  language_code, NULL);
}

void
on_indent1_activate (GtkAction * action, gpointer user_data)
{
	//trying to restore line no where i was before autoformat invoked
	gint lineno;
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	lineno = ianjuta_editor_get_lineno(te, NULL);
	editor_autoformat (te, plugin->prefs);
	ianjuta_editor_goto_line (te, lineno, NULL);
}

void
on_format_indent_style_clicked (GtkAction * action, gpointer user_data)
{
	DocmanPlugin *plugin;
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	gtk_signal_emit_by_name (GTK_OBJECT (plugin->prefs),
										 "activate");
}

void
on_calltip1_activate (GtkAction * action, gpointer user_data)
{
	// FIXME
	// on_editor_command_activate (action, ANE_COMPLETECALLTIP, user_data);
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
	IAnjutaEditor *te;
	AnjutaDocman *docman;
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	te = anjuta_docman_get_current_editor (docman);
	if (!te)
		return;
	uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
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
	IAnjutaEditor *te = get_current_editor (user_data);
	if (!te)
		return;
	ianjuta_editor_view_create (IANJUTA_EDITOR_VIEW (te), NULL);
}

void
on_editor_remove_view_activate (GtkAction *action, gpointer user_data)
{
	IAnjutaEditor *te = get_current_editor (user_data);
	if (!te)
		return;
	ianjuta_editor_view_remove_current (IANJUTA_EDITOR_VIEW (te), NULL);
}

 /*
  * mainmenu_callbacks.c
  * Copyright (C) 2000  Kh. Naba Kumar Singh
  * 
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  * 
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>

#include <gnome.h>
#include "anjuta.h"
#include "about.h"
#include "text_editor.h"
#include "messagebox.h"
#include "mainmenu_callbacks.h"
#include "build_project.h"
#include "clean_project.h"
#include "preferences.h"
#include "message-manager.h"
#include "compile.h"
#include "launcher.h"
#include "appwizard.h"
#include "project_dbase.h"
#include "debugger.h"
#include "breakpoints_cbs.h"
#include "goto_line.h"
#include "resources.h"
#include "executer.h"
#include "controls.h"
#include "signals_cbs.h"
#include "watch_cbs.h"
#include "help.h"
#include "Scintilla.h"
#include "ScintillaWidget.h"

#include "tm_tagmanager.h"
#include "file_history.h"

void on_toolbar_find_clicked (GtkButton * button, gpointer user_data);

gboolean closing_state;		/* Do not temper with this variable  */

void
on_new_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_append_text_editor (NULL);
}


void
on_open1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_widget_show (app->fileselection);
}


void
on_save1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean ret;
	TextEditor *te;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	if (te->full_filename == NULL)
	{
		gtk_widget_show (app->save_as_fileselection);
		return;
	}
	ret = text_editor_save_file (te);
	if (closing_state && ret == TRUE)
	{
		anjuta_remove_current_text_editor ();
		closing_state = FALSE;
	}
}


void
on_save_as1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (anjuta_get_current_text_editor () == NULL)
		return;
	gtk_widget_show (app->save_as_fileselection);
}

void
on_save_all1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	int i;
	tags_manager_freeze (app->tags_manager);
	for (i = 0; i < g_list_length (app->text_editor_list); i++)
	{
		te = g_list_nth_data (app->text_editor_list, i);
		if (te->full_filename && !text_editor_is_saved (te))
			text_editor_save_file (te);
	}
	tags_manager_thaw (app->tags_manager);
	anjuta_status (_("All files saved ..."));
}

void
on_close_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar mesg[256];

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	if (text_editor_is_saved (te) == FALSE)
	{
		closing_state = TRUE;
		sprintf (mesg,
			 _
			 ("The file \"%s\" is not saved.\nDo you want to save it before closing?"),
			 te->filename);

		messagebox3 (GNOME_MESSAGE_BOX_QUESTION, mesg,
			     GNOME_STOCK_BUTTON_YES,
			     GNOME_STOCK_BUTTON_NO,
			     GNOME_STOCK_BUTTON_CANCEL,
			     on_save1_activate,
			     on_save_on_close_no_clicked,
			     on_save_on_close_cancel_clicked, NULL);
	}
	else
		anjuta_remove_current_text_editor ();
}

void
on_close_all_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GList *node;
	
	/* Close all 'saved' files */
	node = app->text_editor_list;
	while (node)
	{
		TextEditor* te;
		GList* next;
		te = node->data;
		next = node->next; // Save it now, as we may change it.
		if(te)
		{
			if (text_editor_is_saved (te) && te->full_filename)
			{
				anjuta_remove_text_editor(te);
			}
		}
		node = next;
	}
}

void
on_reload_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar mesg[256];

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	sprintf (mesg,
		 _
		 ("Are you sure you want to reload %s.\nYou will lose any unsaved modification."),
		 te->filename);

	messagebox2 (GNOME_MESSAGE_BOX_QUESTION, mesg,
		     GNOME_STOCK_BUTTON_YES,
		     GNOME_STOCK_BUTTON_NO, on_reload_yes_clicked, NULL, te);
}

void
on_new_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app_wizard_proceed ();
}

void
on_open_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_open_project (app->project_dbase);
}

void
on_save_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_save_project (app->project_dbase);
}


void
on_close_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_close_project (app->project_dbase);
}


void
on_rename1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}


void
on_page_setup1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_notebook_set_page (GTK_NOTEBOOK
			       (app->preferences->widgets.notebook), 4);
	preferences_show (app->preferences);
}

static void
on_print_confirm_yes_clicked (GtkButton * b, gpointer data)
{
	gchar *pr_cmd, *cmd;
	pid_t pid;
	int status;
	TextEditor *te;

	te = anjuta_get_current_text_editor ();

	if (!te)
		return;
	pr_cmd = preferences_get (app->preferences, COMMAND_PRINT);
	if (!pr_cmd)
		pr_cmd = g_strdup ("lpr");

	cmd = g_strconcat (pr_cmd, " ", te->full_filename, NULL);
	g_free (pr_cmd);
	pid = gnome_execute_shell (app->dirs->home, cmd);
	if (pid < 0)
	{
		anjuta_system_error (errno, _("Could not execute cmd: %s"), cmd);
		g_free (cmd);
		return;
	}
	waitpid (pid, &status, 0);
	if (WEXITSTATUS (status) != 0)
	{
		anjuta_error(_("There was an error while printing.\n"
			"The cmd is: %s"), cmd);
	}
	else
	{
		messagebox (GNOME_MESSAGE_BOX_INFO,
			    _("The file has been sent for printing."));
	}
	g_free (cmd);
}

void
on_print1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();

	if (!te)
		return;
	if (text_editor_is_saved (te) == FALSE)
	{
		messagebox (GNOME_MESSAGE_BOX_INFO,
			    _("You must save the file first and then print"));
		return;
	}
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to print the current file?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     on_print_confirm_yes_clicked, NULL, NULL);
}


void
on_file2_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}

void
on_nonimplemented_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}


void
on_exit1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (on_anjuta_delete (NULL, NULL, NULL) == FALSE)
		on_anjuta_destroy (NULL, NULL);
}


void
on_undo1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_UNDO, 0, 0);
}

void
on_redo1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_REDO, 0, 0);
}

void
on_cut1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_CUT, 0, 0);
}


void
on_copy1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_COPY, 0, 0);
}


void
on_paste1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_PASTE, 0, 0);
}


void
on_clear1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_CLEAR, 0, 0);
}

void
on_transform_upper1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_UPRCASE, 0, 0);
}

void
on_transform_lower1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_LWRCASE, 0, 0);
}

void
on_transform_eolchars1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_EOL_CONVERT, 0, 0);
}


void
on_insert_c_gpl_notice(GtkMenuItem * menuitem, gpointer user_data)
{
       TextEditor *te;
       char *GPLNotice =
			"/*\n"
			" *  This program is free software; you can redistribute it and/or modify\n"
			" *  it under the terms of the GNU General Public License as published by\n"
			" *  the Free Software Foundation; either version 2 of the License, or\n"
			" *  (at your option) any later version.\n"
			" *\n"
			" *  This program is distributed in the hope that it will be useful,\n"
			" *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			" *  GNU Library General Public License for more details.\n"
			" *\n"
			" *  You should have received a copy of the GNU General Public License\n"
			" *  along with this program; if not, write to the Free Software\n"
			" *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
			" */\n"
			" \n";

       te = anjuta_get_current_text_editor ();
       if (te == NULL)
               return;
       aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)GPLNotice);
}

void
on_insert_cpp_gpl_notice(GtkMenuItem * menuitem, gpointer user_data)
{
       TextEditor *te;
       char *GPLNotice =
			"// This program is free software; you can redistribute it and/or modify\n"
			"// it under the terms of the GNU General Public License as published by\n"
			"// the Free Software Foundation; either version 2 of the License, or\n"
			"// (at your option) any later version.\n"
			"//\n"
			"// This program is distributed in the hope that it will be useful,\n"
			"// but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			"// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			"// GNU Library General Public License for more details.\n"
			"//\n"
			"// You should have received a copy of the GNU General Public License\n"
			"// along with this program; if not, write to the Free Software\n"
			"// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
			"\n";

       te = anjuta_get_current_text_editor ();
       if (te == NULL)
               return;
       aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)GPLNotice);
}

void
on_insert_py_gpl_notice(GtkMenuItem * menuitem, gpointer user_data)
{
	   TextEditor *te;
	   char *GPLNotice =
		   "# This program is free software; you can redistribute it and/or modify\n"
		   "# it under the terms of the GNU General Public License as published by\n"
		   "# the Free Software Foundation; either version 2 of the License, or\n"
		   "# (at your option) any later version.\n"
		   "#\n"
		   "# This program is distributed in the hope that it will be useful,\n"
		   "# but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		   "# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		   "# GNU Library General Public License for more details.\n"
		   "#\n"
		   "# You should have received a copy of the GNU General Public License\n"
		   "# along with this program; if not, write to the Free Software\n"
		   "# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
		   "\n";

	   te = anjuta_get_current_text_editor ();
	   if (te == NULL)
			   return;
	   aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)GPLNotice);
}

void
on_insert_username(GtkMenuItem * menuitem, gpointer user_data)
{
       TextEditor *te;
	   char *Username;
	  
	   Username = getenv("USERNAME");
	   
	   te = anjuta_get_current_text_editor ();
	   if (te == NULL)
			   return;
       aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)Username);
}

void
on_insert_changelog_entry(GtkMenuItem * menuitem, gpointer user_data)
{
/* TODO: this function should insert a string of the form yyyy-mm-dd<tab>name<tab>e-mail */
/* it will require preferences to specify a name and e-mail address                      */
/* NB the name should be the user's personal name "Albert Einstein" since we             */
/* can get USERNAME from the environment (see above)                                     */
/* suggest the default should be getenv("USERNAME") plus the machine hostname            */
/*
       TextEditor *te;
	   char *CLEntry;
	   
	   .... some code to build the changelog entry string in CLEntry ....
			   create the date string (build from ctime?)
			   read the name and e-mail from prefs

	   te = anjuta_get_current_text_editor ();
	   if (te == NULL)
			   return;
       aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)CLEntry);
*/
}

void
on_insert_date_time(GtkMenuItem * menuitem, gpointer user_data)
{
       TextEditor *te;
       time_t cur_time = time(NULL);
       char DateTime[256] = {0};
       
       sprintf(DateTime,ctime(&cur_time));
       DateTime[strlen(DateTime)-1] = '\0';    // strip the \n that ctime appends
               
       te = anjuta_get_current_text_editor ();
       if (te == NULL)
               return;
       aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)DateTime);
}

void
on_insert_header_template(GtkMenuItem * menuitem, gpointer user_data)
{
       TextEditor *te;
       char *header_template =
	"#ifndef _HEADER_H\n"
	"#define _HEADER_H\n"
	"\n"
	"#ifdef __cplusplus\n"
	"extern \"C\"\n"
	"{\n"
	"#endif\n"
	"\n"
	"#ifdef __cplusplus\n"
	"}\n"
	"#endif\n"
	"\n"
	"#endif /* _HEADER_H */\n";

       te = anjuta_get_current_text_editor ();
       if (te == NULL)
               return;
       aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)header_template);
}

void
on_autocomplete1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_COMPLETEWORD, 0, 0);
}

void
on_calltip1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_select_all1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_SELECTALL, 0, 0);
}

void
on_select_matchbrace1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_SELECTTOBRACE, 0, 0);
}

void
on_select_block1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_SELECTBLOCK, 0, 0);
}

void
on_find1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	find_text_show (app->find_replace->find_text);
}


void
on_find_in_files1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	find_in_files_show (app->find_in_files);
}


void
on_find_and_replace1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	find_replace_show (app->find_replace);
}


void
on_goto_line_no1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *gt;
	gt = gotoline_new ();
	gtk_widget_show (gt);
}

void
on_goto_matchbrace1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_MATCHBRACE, 0, 0);
}

void
on_goto_block_start1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	text_editor_goto_block_start(te);
}

void
on_goto_block_end1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	text_editor_goto_block_end(te);
}

void
on_goto_prev_mesg1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_message_manager_previous (app->messages);
}

void
on_goto_next_mesg1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_message_manager_next (app->messages);
}

void
on_edit_app_gui1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_summon_glade (app->project_dbase);
}


void
on_save_build_messages_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (!anjuta_message_manager_build_is_empty(app->messages))
		gtk_widget_show (app->save_as_build_msg_sel);
	else
		anjuta_error("There are no build messages.");

	return;
}


/***********************************************************************/

void
on_messages1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = anjuta_message_manager_is_shown(app->messages);
	if(state) {
		gtk_widget_hide(GTK_WIDGET(app->messages));
	} else {
		anjuta_message_manager_show (app->messages, MESSAGE_NONE);
	}
}

void
on_project_listing1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = app->project_dbase->is_showing;
	if(state) {
		project_dbase_hide (app->project_dbase);
	} else {
		project_dbase_show (app->project_dbase);
	}
}

void
on_bookmarks1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}

void
on_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_show (debugger.breakpoints_dbase);
}

void
on_watch_window1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	expr_watch_show (debugger.watch);
}


void
on_registers1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	cpu_registers_show (debugger.cpu_registers);
}


void
on_program_stack1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	stack_trace_show (debugger.stack);
}

void
on_shared_lib1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	sharedlibs_show (debugger.sharedlibs);
}

void
on_kernal_signals1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	signals_show (debugger.signals);
}

void
on_dump_window1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}


void
on_console1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}

void
on_showhide_locals(GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_set_show_locals( app->project_dbase, GTK_CHECK_MENU_ITEM (menuitem)->active ) ;
}

/************************************************************************/

void
on_editor_linenos1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	preferences_set_int (app->preferences, "margin.linenumber.visible",
			     state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_LINENUMBERMARGIN, state,
				  0);
		node = g_list_next (node);
	}
}

void
on_editor_markers1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	preferences_set_int (app->preferences, "margin.marker.visible", state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_SELMARGIN, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_codefold1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	preferences_set_int (app->preferences, "margin.fold.visible", state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_FOLDMARGIN, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_indentguides1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	preferences_set_int (app->preferences, "view.indentation.guides",
			     state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWGUIDES, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_whitespaces1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	preferences_set_int (app->preferences, "view.whitespace", state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWSPACE, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_eolchars1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	preferences_set_int (app->preferences, "view.eol", state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWEOL, state, 0);
		node = g_list_next (node);
	}
}

/************************************************************************/
void
on_zoom_text_plus_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	gint zoom;
	gchar buff[20];
	p = app->preferences;

	zoom = prop_get_int (p->props, "text.zoom.factor", 0);
	sprintf(buff, "%d", zoom+2);
	prop_set_with_key(p->props, "text.zoom.factor", buff);
	anjuta_apply_preferences();
}
void
on_zoom_text_8_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "8");
	anjuta_apply_preferences();
}

void
on_zoom_text_6_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "6");
	anjuta_apply_preferences();
}

void
on_zoom_text_4_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "4");
	anjuta_apply_preferences();
}
void
on_zoom_text_2_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "2");
	anjuta_apply_preferences();
}
void
on_zoom_text_0_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "0");
	anjuta_apply_preferences();
}
void
on_zoom_text_s2_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "-2");
	anjuta_apply_preferences();
}
void
on_zoom_text_s4_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "-4");
	anjuta_apply_preferences();
}
void
on_zoom_text_s6_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "-6");
	anjuta_apply_preferences();
}
void
on_zoom_text_s8_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	p = app->preferences;

	prop_set_with_key (p->props, "text.zoom.factor", "-8");
	anjuta_apply_preferences();
}
void
on_zoom_text_minus_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	Preferences *p;
	gint zoom;
	gchar buff[20];
	p = app->preferences;

	zoom = prop_get_int (p->props, "text.zoom.factor", 0);
	sprintf(buff, "%d", zoom-2);
	prop_set_with_key(p->props, "text.zoom.factor", buff);
	anjuta_apply_preferences();
}

/************************************************************************/
void
on_main_toolbar1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view (ANJUTA_MAIN_TOOLBAR, state, TRUE, TRUE);
}

void
on_extended_toolbar1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view (ANJUTA_EXTENDED_TOOLBAR, state, TRUE, TRUE);
}

void
on_tags_toolbar1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view (ANJUTA_TAGS_TOOLBAR, state, TRUE, TRUE);
}

void
on_debug_toolbar1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view (ANJUTA_DEBUG_TOOLBAR, state, TRUE, TRUE);
}

void
on_browser_toolbar1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view (ANJUTA_BROWSER_TOOLBAR, state, TRUE, TRUE);
}

void
on_format_toolbar1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view (ANJUTA_FORMAT_TOOLBAR, state, TRUE, TRUE);
}

/*************************************************************************/
void
on_prj_add_src1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_add_pix1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_add_doc1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_add_gen1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_add_dir1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_remove1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_readme1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *filename;
	if (app->project_dbase->project_is_open == FALSE)
		return;
	filename =
		g_strconcat (app->project_dbase->top_proj_dir, "/README",
			     NULL);
	anjuta_append_text_editor (filename);
	g_free (filename);
}

void
on_prj_todo1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *filename;
	if (app->project_dbase->project_is_open == FALSE)
		return;
	filename =
		g_strconcat (app->project_dbase->top_proj_dir, "/TODO", NULL);
	anjuta_append_text_editor (filename);
	g_free (filename);
}

void
on_prj_changelog1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *filename;
	if (app->project_dbase->project_is_open == FALSE)
		return;
	filename =
		g_strconcat (app->project_dbase->top_proj_dir, "/ChangeLog",
			     NULL);
	anjuta_append_text_editor (filename);
	g_free (filename);
}

void
on_prj_news1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *filename;
	if (app->project_dbase->project_is_open == FALSE)
		return;
	filename =
		g_strconcat (app->project_dbase->top_proj_dir, "/NEWS", NULL);
	anjuta_append_text_editor (filename);
	g_free (filename);
}

void
on_prj_config1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

void
on_prj_info1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
}

/*************************************************************************/
void
on_force_hilite1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	te->force_hilite = (gint) user_data;
	text_editor_set_hilite_type (te);
}

void
on_indent1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	text_editor_autoformat (te);
	anjuta_update_title();
}

void
on_indent_inc1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INDENT_INCREASE, 0, 0);
}

void
on_indent_dcr1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INDENT_DECREASE, 0, 0);
}

void
on_update_tags1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gint i;

	tags_manager_freeze (app->tags_manager);
	/*tags_manager_clear (app->tags_manager);*/

	if (app->project_dbase->project_is_open)
	{
		project_dbase_update_tags_image(app->project_dbase);
	}
	else
	{
		tags_manager_clear (app->tags_manager);
		for (i = 0; i < g_list_length (app->text_editor_list); i++)
		{
			te = g_list_nth_data (app->text_editor_list, i);
			if (te->full_filename)
				tags_manager_update (app->tags_manager,
						     te->full_filename);
		}
	}
	tags_manager_thaw (app->tags_manager);
	anjuta_status (_("Tags image update successfully."));
}

void
on_open_folds1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	aneditor_command (te->editor_id, ANE_OPEN_FOLDALL, 0, 0);
}

void
on_close_folds1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	aneditor_command (te->editor_id, ANE_CLOSE_FOLDALL, 0, 0);
}

void
on_toggle_fold1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	aneditor_command (te->editor_id, ANE_TOGGLE_FOLD, 0, 0);
}

void
on_detach1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gint page_num;
	TextEditor *te;
	GtkWidget *container;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	page_num =
		gtk_notebook_get_current_page (GTK_NOTEBOOK
					       (app->widgets.notebook));

	container = te->widgets.client->parent;
	text_editor_undock (te, container);
	gtk_notebook_remove_page (GTK_NOTEBOOK (app->widgets.notebook),
				  page_num);
	
	on_anjuta_window_focus_in_event (NULL, NULL, NULL);
}

/*************************************************************************/
void
on_compile1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	compile_file (FALSE);
}

void
on_make1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	compile_file (TRUE);
}

void
on_build_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	build_project ();
}

void
on_install_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	build_install_project ();
}

void
on_autogen_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	build_autogen_project ();
}

void
on_build_dist_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	build_dist_project ();
}

void
on_build_all_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	build_all_project ();
}

void
on_configure_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	configurer_show (app->configurer);
}

void
on_clean_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	clean_project ();
}

void
on_clean_all_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	clean_all_project ();
}

void
on_stop_build_make1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	launcher_reset ();
}

void
on_go_execute1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	executer_execute (app->executer);
}

void
on_go_execute2_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	executer_show (app->executer);
}

/*******************************************************************************/
void
on_book_toggle1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_BOOKMARK_TOGGLE, 0, 0);
}

void
on_book_first1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_BOOKMARK_FIRST, 0, 0);
}

void
on_book_prev1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_BOOKMARK_PREV, 0, 0);
}

void
on_book_next1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_BOOKMARK_NEXT, 0, 0);
}

void
on_book_last1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_BOOKMARK_LAST, 0, 0);
}

void
on_book_clear1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_BOOKMARK_CLEAR, 0, 0);
}


/*******************************************************************************/
void
on_toggle_breakpoint1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_toggle_breakpoint(debugger.breakpoints_dbase);
}

void
on_set_breakpoint1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_bk_add_breakpoint_clicked (NULL, debugger.breakpoints_dbase);
}

void
on_disable_all_breakpoints1_activate (GtkMenuItem * menuitem,
				      gpointer user_data)
{

}

void
on_show_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_show (debugger.breakpoints_dbase);
}

void
on_clear_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_bk_delete_all_clicked (NULL, debugger.breakpoints_dbase);
}

/*******************************************************************************/
void
on_execution_continue1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_run ();
}

void
on_execution_step_in1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_step_in ();
}

void
on_execution_step_out1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_step_out ();
}

void
on_execution_step_over1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_step_over ();
}

void
on_execution_run_to_cursor1_activate (GtkMenuItem * menuitem,
				      gpointer user_data)
{
	guint line;
	gchar *buff;
	TextEditor* te;

	te = anjuta_get_current_text_editor();
	g_return_if_fail (te != NULL);
	g_return_if_fail (te->full_filename != NULL);
	if (debugger_is_active()==FALSE) return;
	if (debugger_is_ready()==FALSE) return;

	line = text_editor_get_current_lineno (te);

	buff = g_strdup_printf ("%s:%d", te->filename, line);
	debugger_run_to_location (buff);
	g_free (buff);
}

/*******************************************************************************/
void
on_info_targets_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info target",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_program_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info program",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_udot_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info udot",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_threads_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info threads",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_variables_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info variables",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_locals_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info locals",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_frame_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info frame",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}

void
on_info_args_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_put_cmd_in_queqe ("set print pretty on", DB_CMD_NONE, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("set verbos off", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("info args",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				   debugger_dialog_message, NULL);
	debugger_put_cmd_in_queqe ("set verbos on", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("set print pretty off", DB_CMD_NONE, NULL,
				   NULL);
	debugger_execute_cmd_in_queqe ();
}


/********************************************************************************/

void
on_debugger_start_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *prog, *temp;
	gint s_re, e_re;
	struct stat s_stat, e_stat;
	TextEditor *te;

	prog = NULL;
	if (app->project_dbase->project_is_open)
	{
		gint target_type;
		target_type = project_dbase_get_target_type (app->project_dbase);
		if (target_type >= PROJECT_TARGET_TYPE_END_MARK)
			anjuta_error (_("Target of this project is unknown"));
		else if ( target_type != PROJECT_TARGET_TYPE_EXECUTABLE)
			anjuta_warning (_("Target of this project is not executable"));
		prog = project_dbase_get_source_target (app->project_dbase);
		if (file_is_executable (prog) == FALSE)
		{
			anjuta_warning(_("Executable does not exist for this project."));
			g_free (prog);
			prog = NULL;
		}
	}
	else
	{
		te = anjuta_get_current_text_editor ();
		if (te)
		{
			if (te->full_filename)
			{
				prog = g_strdup (te->full_filename);
				temp = get_file_extension (prog);
				if (temp)
					*(--temp) = '\0';
				s_re = stat (te->full_filename, &s_stat);
				e_re = stat (prog, &e_stat);
				if ((e_re != 0) || (s_re != 0))
				{
					anjuta_warning(_("No executable for this file."));
					g_free (prog);
					prog = NULL;
				}
				else if ((!text_editor_is_saved (te)) || (e_stat.st_mtime < s_stat.st_mtime))
				{
					anjuta_warning (_("Executable is not up-to-date."));
				}
			}
			else
			{
				anjuta_warning(_("No executable for this file."));
			}
		}
	}
	debugger_start (prog);
	if (prog) g_free (prog);
}

void
on_debugger_open_exec_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_open_exec_file ();
}

void
on_debugger_attach_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (debugger_is_active ())
		attach_process_show (debugger.attach_process);
	else
		anjuta_error (_("Debugger is not running. Start it first."));
}

void
on_debugger_load_core_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_load_core_file ();
}

void
on_debugger_restart_prog_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_restart_program ();
}

void
on_debugger_stop_prog_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_stop_program ();
}

void
on_debugger_detach_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_detach_process ();
}

void
on_debugger_stop_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (debugger.prog_is_running == TRUE)
	{
		if (debugger.prog_is_attached == TRUE)
		{
			messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
				     _
				     ("Program is ATTACHED.\nDo you still want to stop Debugger?"),
				     GNOME_STOCK_BUTTON_YES,
				     GNOME_STOCK_BUTTON_NO,
				     on_debugger_confirm_stop_yes_clicked,
				     NULL, NULL);
		}
		else
		{
			messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
				     _
				     ("Program is running.\nDo you still want to stop Debugger?"),
				     GNOME_STOCK_BUTTON_YES,
				     GNOME_STOCK_BUTTON_NO,
				     on_debugger_confirm_stop_yes_clicked,
				     NULL, NULL);
		}
	}
	else
		debugger_stop ();
}

void
on_debugger_confirm_stop_yes_clicked (GtkButton * button, gpointer data)
{
	debugger_stop ();
}

void
on_debugger_interrupt_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	debugger_interrupt ();
}

void
on_debugger_signal_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_signals_send_activate (NULL, debugger.signals);
}

void
on_debugger_inspect_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *w = create_eval_dialog ();
	gtk_widget_show (w);
}

void
on_debugger_add_watch_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	expr_watch_show (debugger.watch);
	on_watch_add_activate (NULL, NULL);
}

void
on_debugger_custom_command_activate (GtkMenuItem * menuitem,
				     gpointer user_data)
{
	debugger_custom_command ();
}

/************************************************************************************************/

void
on_windows1_new_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_new_file1_activate (NULL, NULL);
}

void
on_windows1_close_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_close_file1_activate (NULL, NULL);
}

/************************************************************************************************/
void
on_set_compiler1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	compiler_options_show (app->compiler_options);
}

void
on_set_src_paths1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	src_paths_get (app->src_paths);
}

void
on_set_commands1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	command_editor_show (app->command_editor);
}

void
on_set_preferences1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	preferences_show (app->preferences);
}

void
on_set_default_preferences1_activate (GtkMenuItem * menuitem,
				      gpointer user_data)
{
	preferences_reset_defaults (app->preferences);
}

/************************************************************************************************/
void
on_utilities1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_grep_utility1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_compare_two_files1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_diff_utility1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
	on_file_view__char_octal_hex_1_activate
	(GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_c_beautifier1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_c_flow1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_c_cross_reference1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_c_trace1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_archive_maintenace1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}

void
on_contents1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	pid_t pid;

	if (anjuta_is_installed ("gnome-help-browser", TRUE) == FALSE)
		return;

	if ((pid = fork ()) == 0)
	{
		execlp ("gnome-help-browser", "gnome-help-browser",
			"ghelp:anjuta");
		g_error (_("Cannot launch gnome-help-browser"));
	}
	if (pid > 0)
		anjuta_register_child_process (pid, NULL, NULL);
}


void
on_index1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	pid_t pid;
	if (anjuta_is_installed ("gnome-help-browser", TRUE) == FALSE)
		return;

	if ((pid = fork ()) == 0)
	{
		execlp ("gnome-help-browser", "gnome-help-browser",
			"ghelp:anjuta#index");
		g_error (_("Cannot launch gnome-help-browser"));
	}
	if (pid > 0)
		anjuta_register_child_process (pid, NULL, NULL);
}

void
on_gnome_pages1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (anjuta_is_installed ("devhelp", TRUE))
	{
		anjuta_res_help_search (NULL);
	}
}

void
on_man_pages1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	pid_t pid;

	if (anjuta_is_installed ("gnome-help-browser", TRUE) == FALSE)
		return;

	if ((pid = fork ()) == 0)
	{
		execlp ("gnome-help-browser", "gnome-help-browser",
			"toc:man");
		g_error (_("Cannot launch gnome-help-browser"));
	}
	if (pid > 0)
		anjuta_register_child_process (pid, NULL, NULL);
}

void
on_info_pages1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	pid_t pid;

	if (anjuta_is_installed ("gnome-help-browser", TRUE) == FALSE)
		return;

	if ((pid = fork ()) == 0)
	{
		execlp ("gnome-help-browser", "gnome-help-browser",
			"toc:info");
		g_error (_("Cannot launch gnome-help-browser"));
	}
	if (pid > 0)
		anjuta_register_child_process (pid, NULL, NULL);
}

void
on_context_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor* te;
	gboolean ret;
	gchar buffer[1000];
	
	te = anjuta_get_current_text_editor();
	if(!te) return;
	ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buffer, (long)sizeof(buffer));
	if (ret == FALSE) return;
	anjuta_help_search(app->help_system, buffer);
}

void
on_goto_tag_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor* te;
	gboolean ret;
	gchar buffer[1000];

	te = anjuta_get_current_text_editor();
	if(!te) return;
	ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buffer, (long)sizeof(buffer));
	if (!ret)
		return;
	else
		anjuta_goto_symbol_definition(buffer, te);
}

void
on_go_back_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	an_file_history_back();
}

void
on_go_forward_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	an_file_history_forward();
}

void
on_history_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	an_file_history_dump();
}

void
on_search_a_topic1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_help_show(app->help_system);
}

void
on_url_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (user_data)
	{
		gnome_url_show(user_data);
	}
}

void
on_about1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_widget_show (create_about_gui ());
}

void
on_save_on_close_no_clicked (GtkButton * button, gpointer data)
{
	anjuta_remove_current_text_editor ();
	closing_state = FALSE;
}

void
on_save_on_close_cancel_clicked (GtkButton * button, gpointer data)
{
	closing_state = FALSE;
}

void
on_reload_yes_clicked (GtkButton * button, gpointer te_data)
{
	text_editor_load_file ((TextEditor *) te_data);
	anjuta_update_title ();
}
void
on_findnext1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_toolbar_find_clicked ( NULL, NULL );
}

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
#include <sys/wait.h>
#include <errno.h>

#include <gnome.h>

#include <libgnomeui/gnome-window-icon.h>

#include <libanjuta/resources.h>

#include "anjuta.h"
#include "text_editor.h"
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
#include "breakpoints.h"
#include "goto_line.h"
#include "executer.h"
#include "controls.h"
#include "help.h"
#include "project_import.h"
#include "cvs_gui.h"
#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "about.h"
#include "an_file_view.h"
#include "tm_tagmanager.h"
#include "file_history.h"
#include "memory.h"
#include "fileselection.h"
#include "anjuta-tools.h"
#include "search-replace.h"
#include "search_incremental.h"
#include "anjuta_info.h"
#include "watch_gui.h"
#include "signals_cbs.h"
#include "watch_cbs.h"
#include "start-with.h"
#include "file.h"

void on_toolbar_find_clicked (EggAction *action, gpointer user_data);

gboolean closing_state;		/* Do not tamper with this variable  */

static gchar *insert_header_c( TextEditor *te);

void
on_new_project1_activate (EggAction * action, gpointer user_data)
{
	if (app->project_dbase->project_is_open)
	{
		project_dbase_close_project (app->project_dbase);
	}
	if( app->project_dbase->project_is_open )
		return ;
	app_wizard_proceed ();
}

void
on_import_project_activate (EggAction * action, gpointer user_data)
{
	/* Note: ProjectImportWizard object, which is created by the
	following call, will be automatically destroyed when the Import is
	done or canceled. We do not need to take care of it. */
	project_import_new ();
}

void
on_open_project1_activate (EggAction * action, gpointer user_data)
{
	anjuta_open_project();
}

void
on_save_project1_activate (EggAction * action, gpointer user_data)
{
	project_dbase_save_project (app->project_dbase);
}


void
on_close_project1_activate (EggAction * action, gpointer user_data)
{
	project_dbase_close_project (app->project_dbase);
}

void
on_exit1_activate (EggAction * action, gpointer user_data)
{
	if (on_anjuta_delete (NULL, NULL, NULL) == FALSE)
		on_anjuta_destroy (NULL, NULL);
}

void
on_goto_prev_mesg1_activate (EggAction * action, gpointer user_data)
{
	an_message_manager_previous (app->messages);
}

void
on_goto_next_mesg1_activate (EggAction * action, gpointer user_data)
{
	an_message_manager_next (app->messages);
}

void
on_edit_app_gui1_activate (EggAction * action, gpointer user_data)
{
	project_dbase_edit_gui (app->project_dbase);
}


void
on_save_build_messages_activate (EggAction * action, gpointer user_data)
{
	if (!an_message_manager_build_is_empty(app->messages))
		gtk_widget_show (app->save_as_build_msg_sel);
	else
		anjuta_error("There are no build messages.");

	return;
}


/***********************************************************************/

void
on_messages1_activate (EggAction * action, gpointer user_data)
{
#if 0
	gboolean state;
	state = an_message_manager_is_shown(app->messages);
	if(state) {
		gtk_widget_hide(GTK_WIDGET(app->messages));
	} else {
		an_message_manager_show (app->messages, MESSAGE_NONE);
	}
#endif
	an_message_manager_show (app->messages, MESSAGE_NONE);
}

void
on_project_listing1_activate (EggAction * action, gpointer user_data)
{
#if 0
	gboolean state;
	state = app->project_dbase->is_showing;
	if(state) {
		project_dbase_hide (app->project_dbase);
	} else {
		project_dbase_show (app->project_dbase);
	}
#endif
}

void
on_bookmarks1_activate (EggAction * action, gpointer user_data)
{
	anjuta_not_implemented (__FILE__, __LINE__);
}

void
on_breakpoints1_activate (EggAction * action, gpointer user_data)
{
	breakpoints_dbase_show (debugger.breakpoints_dbase);
}


void
on_registers1_activate (EggAction * action, gpointer user_data)
{
	cpu_registers_show (debugger.cpu_registers);
}

void
on_shared_lib1_activate (EggAction * action, gpointer user_data)
{
	sharedlibs_show (debugger.sharedlibs);
}

void
on_kernal_signals1_activate (EggAction * action, gpointer user_data)
{
	signals_show (debugger.signals);
}

/************************************************************************/

void
on_anjuta_toolbar_activate (EggAction * action, gpointer user_data)
{
	gboolean state;
	state = (int) EGG_TOGGLE_ACTION (action)->active;
	anjuta_toolbar_set_view ((gchar *) user_data, state, TRUE, TRUE);
}

void
on_update_tagmanager_activate (EggAction * action, gpointer user_data)
{
	if (app->project_dbase->project_is_open)
	{
		if (user_data)
			project_dbase_sync_tags_image(app->project_dbase);
		else
			project_dbase_update_tags_image(app->project_dbase, FALSE);
	}
}

/*************************************************************************/
void
on_detach1_activate (EggAction * action, gpointer user_data)
{
#if 0
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
#endif
}

void
on_ordertab1_activate (EggAction * action, gpointer user_data)
{
	if (GTK_CHECK_MENU_ITEM(action)->active)
		anjuta_order_tabs();
}

/*************************************************************************/
void
on_compile1_activate (EggAction * action, gpointer user_data)
{
	compile_file (FALSE);
}

void
on_make1_activate (EggAction * action, gpointer user_data)
{
	compile_file (TRUE);
}

void
on_build_project1_activate (EggAction * action, gpointer user_data)
{
	build_project ();
}

void
on_install_project1_activate (EggAction * action, gpointer user_data)
{
	build_install_project ();
}

void
on_autogen_project1_activate (EggAction * action, gpointer user_data)
{
	build_autogen_project ();
}

void
on_build_dist_project1_activate (EggAction * action, gpointer user_data)
{
	build_dist_project ();
}

void
on_build_all_project1_activate (EggAction * action, gpointer user_data)
{
	build_all_project ();
}

void
on_configure_project1_activate (EggAction * action, gpointer user_data)
{
	configurer_show (app->configurer);
}

void
on_clean_project1_activate (EggAction * action, gpointer user_data)
{
	clean_project (NULL);
}

void
on_clean_all_project1_activate (EggAction * action, gpointer user_data)
{
	clean_all_project ();
}

void
on_stop_build_make1_activate (EggAction * action, gpointer user_data)
{
	anjuta_launcher_reset (app->launcher);
}

void
on_go_execute1_activate (EggAction * action, gpointer user_data)
{
	executer_execute (app->executer);
}

void
on_go_execute2_activate (EggAction * action, gpointer user_data)
{
	executer_show (app->executer);
}

void
on_toggle_breakpoint1_activate (EggAction * action, gpointer user_data)
{
	breakpoints_dbase_toggle_breakpoint(debugger.breakpoints_dbase, 0);
}

void
on_set_breakpoint1_activate (EggAction * action, gpointer user_data)
{
	breakpoints_dbase_add (debugger.breakpoints_dbase);
}

void
on_disable_all_breakpoints1_activate (EggAction * action,
				      gpointer user_data)
{
	breakpoints_dbase_disable_all (debugger.breakpoints_dbase);
}

void
on_show_breakpoints1_activate (EggAction * action, gpointer user_data)
{
	breakpoints_dbase_show (debugger.breakpoints_dbase);
}

void
on_clear_breakpoints1_activate (EggAction * action, gpointer user_data)
{
	breakpoints_dbase_remove_all (debugger.breakpoints_dbase);
}

/*******************************************************************************/
void
on_execution_continue1_activate (EggAction * action, gpointer user_data)
{
	debugger_run ();
}

void
on_execution_step_in1_activate (EggAction * action, gpointer user_data)
{
	debugger_step_in ();
}

void
on_execution_step_out1_activate (EggAction * action, gpointer user_data)
{
	debugger_step_out ();
}

void
on_execution_step_over1_activate (EggAction * action, gpointer user_data)
{
	debugger_step_over ();
}

void
on_execution_run_to_cursor1_activate (EggAction * action,
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
on_info_targets_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_target (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_program_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_program (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_udot_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_udot (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_threads_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_threads (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_variables_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_variables (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_locals_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_locals (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_frame_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_frame (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_args_activate (EggAction * action, gpointer user_data)
{
	debugger_query_info_args (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_memory_activate (EggAction * action, gpointer user_data)
{
	GtkWidget *win_memory;

	win_memory = memory_info_new (NULL);
	gtk_widget_show(win_memory);
}

/********************************************************************************/

void
on_debugger_start_activate (EggAction * action, gpointer user_data)
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
			anjuta_error (_("The target executable of this Project is unknown"));
		else if ( target_type != PROJECT_TARGET_TYPE_EXECUTABLE)
			anjuta_warning (_("The target executable of this Project is not executable"));
		prog = project_dbase_get_source_target (app->project_dbase);
		if (file_is_executable (prog) == FALSE)
		{
			anjuta_warning(_("The target executable does not exist for this Project"));
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
					anjuta_warning (_("The executable is not up-to-date."));
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
on_debugger_open_exec_activate (EggAction * action, gpointer user_data)
{
	debugger_open_exec_file ();
}

void
on_debugger_attach_activate (EggAction * action, gpointer user_data)
{
	if (debugger_is_active ())
		attach_process_show (debugger.attach_process);
	else
		anjuta_error (_("Debugger is not running. Start it first."));
}

void
on_debugger_load_core_activate (EggAction * action, gpointer user_data)
{
	debugger_load_core_file ();
}

void
on_debugger_restart_prog_activate (EggAction * action, gpointer user_data)
{
	debugger_restart_program ();
}

void
on_debugger_stop_prog_activate (EggAction * action, gpointer user_data)
{
	debugger_stop_program ();
}

void
on_debugger_detach_activate (EggAction * action, gpointer user_data)
{
	debugger_detach_process ();
}

void
on_debugger_stop_activate (EggAction * action, gpointer user_data)
{
	debugger_stop ();
}

void
on_debugger_confirm_stop_yes_clicked (GtkButton * button, gpointer data)
{
	debugger_stop ();
}

void
on_debugger_interrupt_activate (EggAction * action, gpointer user_data)
{
	debugger_interrupt ();
}

void
on_debugger_signal_activate (EggAction * action, gpointer user_data)
{
	on_signals_send_activate (NULL, debugger.signals);
}

void
on_debugger_inspect_activate (EggAction * action, gpointer user_data)
{
	GtkWidget *w = create_eval_dialog (GTK_WINDOW(app), debugger.watch);
	gtk_widget_show (w);
}

void
on_debugger_add_watch_activate (EggAction * action, gpointer user_data)
{
	on_watch_add_activate (NULL, debugger.watch);
}

void
on_debugger_custom_command_activate (EggAction * action,
				     gpointer user_data)
{
	debugger_custom_command ();
}

/************************************************************************************************/

void
on_windows1_new_activate (EggAction * action, gpointer user_data)
{
	on_new_file1_activate (NULL, NULL);
}

void
on_windows1_close_activate (EggAction * action, gpointer user_data)
{
	on_close_file1_activate (NULL, NULL);
}

/*************************************************************************************************/
void
on_cvs_update_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_UPDATE, NULL, FALSE);
}

void
on_cvs_commit_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_COMMIT, NULL, FALSE);
}

void
on_cvs_status_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_STATUS, NULL, FALSE);
}

void
on_cvs_log_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_LOG, NULL, FALSE);
}

void
on_cvs_add_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_ADD, NULL, FALSE);
}

void
on_cvs_remove_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_REMOVE, NULL, FALSE);
}

void
on_cvs_diff_file_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_diff_gui (app->cvs, NULL, FALSE);
}

void
on_cvs_update_project_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_gui(app->cvs, CVS_ACTION_UPDATE, prj, TRUE);
}

void
on_cvs_commit_project_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	
	create_cvs_gui(app->cvs, CVS_ACTION_COMMIT, prj, TRUE);
}

void
on_cvs_import_project_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_import_gui (app->cvs);
}

void
on_cvs_project_status_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_gui(app->cvs, CVS_ACTION_STATUS, prj, TRUE);
}

void
on_cvs_project_log_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_gui(app->cvs, CVS_ACTION_LOG, prj, TRUE);
}

void
on_cvs_project_diff_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_diff_gui (app->cvs, prj, TRUE);
}

void
on_cvs_login_activate                  (EggAction     *action,
                                        gpointer         user_data)
{
	create_cvs_login_gui (app->cvs);
}

/************************************************************************************************/
void
on_set_compiler1_activate (EggAction * action, gpointer user_data)
{
	compiler_options_show (app->compiler_options);
}

void
on_set_src_paths1_activate (EggAction * action, gpointer user_data)
{
	src_paths_show (app->src_paths);
}

void
on_set_commands1_activate (EggAction * action, gpointer user_data)
{
	command_editor_show (app->command_editor);
}

void
on_set_preferences1_activate (EggAction * action, gpointer user_data)
{
	gtk_widget_show (GTK_WIDGET (app->preferences));
}

void
on_set_style_editor_activate (EggAction * action, gpointer user_data)
{
	style_editor_show (app->style_editor);
}

void
on_file_view_filters_activate (EggAction * action, gpointer user_data)
{
	fv_customize(TRUE);
}

void
on_edit_user_properties1_activate           (EggAction     *action,
                                        gpointer         user_data)
{
	gchar* user_propfile = g_strconcat (app->dirs->home, "/.anjuta" PREF_SUFFIX "/user.properties", NULL);
	anjuta_goto_file_line_mark (user_propfile, 1, FALSE);
	g_free (user_propfile);
}

void
on_set_default_preferences1_activate (EggAction * action,
				      gpointer user_data)
{
	anjuta_preferences_reset_defaults (ANJUTA_PREFERENCES (app->preferences));
}

void
on_start_with_dialog_activate (EggAction * action, gpointer user_data)
{
	start_with_dialog_show (GTK_WINDOW (app),
							app->preferences, TRUE);
}

void
on_setup_wizard_activate (EggAction * action, gpointer user_data)
{
	// TODO.
}

void
on_help_activate (EggAction *action, gpointer data)
{
	if (gnome_help_display ((const gchar*)data, NULL, NULL) == FALSE)
	{
		anjuta_error (_("Unable to display help. Please make sure Anjuta documentation package is install. It can be downloaded from http://anjuta.org"));	
	}
}

void
on_gnome_pages1_activate (EggAction     *action, gpointer user_data)
{
	if (anjuta_is_installed ("devhelp", TRUE))
	{
		anjuta_res_help_search (NULL);
	}
}

void
on_context_help_activate (EggAction * action, gpointer user_data)
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
on_goto_tag_activate (EggAction * action, gpointer user_data)
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
		anjuta_goto_tag(buffer, te, (gboolean) user_data);
}

void
on_lookup_symbol_activate (EggAction * action, gpointer user_data)
{
	TextEditor* te;
	gchar *buf = NULL;

	te = anjuta_get_current_text_editor();
	if(!te) return;
	buf = text_editor_get_current_word(te);
	if (buf)
	{
		anjuta_search_sources_for_symbol(buf);
		g_free(buf);
	}
}

void
on_go_back_activate (EggAction * action, gpointer user_data)
{
	an_file_history_back();
}

void
on_go_forward_activate (EggAction * action, gpointer user_data)
{
	an_file_history_forward();
}

void
on_history_activate (EggAction * action, gpointer user_data)
{
	an_file_history_dump();
}

void
on_search_a_topic1_activate (EggAction * action, gpointer user_data)
{
	anjuta_help_show (app->help_system);
}

void
on_url_activate (EggAction * action, gpointer user_data)
{
	if (user_data)
	{
		anjuta_res_url_show(user_data);
	}
}

/*
static int
about_box_event_callback (GtkWidget *widget,
                          GdkEvent *event,
                          void *data)
{
        GtkWidget **widget_pointer;

        widget_pointer = (GtkWidget **) data;

        gtk_widget_destroy (GTK_WIDGET (*widget_pointer));
        *widget_pointer = NULL;

        return TRUE;
}
*/

void
on_about1_activate (EggAction * action, gpointer user_data)
{
	GtkWidget *about_dlg = about_box_new ();
	gtk_widget_show (about_dlg);
}

void on_customize_shortcuts_activate(EggAction *action, gpointer user_data)
{
	gtk_widget_show (GTK_WIDGET (app->ui));
}

void on_tool_editor_activate(EggAction *action, gpointer user_data)
{
	anjuta_tools_edit();
}

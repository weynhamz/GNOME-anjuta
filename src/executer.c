/*
    executer.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include "anjuta.h"
#include "text_editor.h"
#include "messagebox.h"
#include "utilities.h"
#include "executer.h"
#include "resources.h"

/* #define DEBUG */

static GtkWidget* create_executer_dialog(Executer*);

static const gchar szRunInTerminalItem[] = {"RunInTerminal"};
//static const gchar szPgmArgsSection[] = {"PgmArgs"};
static
void on_executer_run_button_clicked (GtkButton* button, gpointer user_data);


static void
on_executer_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_executer_checkbutton_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

Executer *
executer_new (PropsID props)
{
	Executer *e = malloc (sizeof (Executer));
	if (e)
	{
		e->props = props;
		e->terminal = TRUE;
		e->m_PgmArgs	= NULL ;
	}
	return e;
}
void
executer_destroy (Executer * e)
{
	if (e)
	{
		int i ;
		if( e->m_PgmArgs )
		{
			for (i = 0; i < g_list_length (e->m_PgmArgs); i++)
				g_free (g_list_nth (e->m_PgmArgs, i)->data);

			if (e->m_PgmArgs)
				g_list_free (e->m_PgmArgs);
		}
		g_free (e);
	}
}

void
executer_show (Executer * e)
{
	gtk_widget_show (create_executer_dialog (e));		
	if (e->m_PgmArgs)
		gtk_combo_set_popdown_strings (GTK_COMBO (e->m_gui.combo1),
					       	e->m_PgmArgs );
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (e->m_gui.check_terminal), e->terminal);
}

static
void on_executer_run_button_clicked (GtkButton* button, gpointer user_data)
{
	Executer	*e =(Executer*) user_data ;
		
	g_return_if_fail( NULL != user_data );
	e->m_PgmArgs = update_string_list ( e->m_PgmArgs,
						   gtk_entry_get_text (GTK_ENTRY (e->m_gui.combo_entry1)),
						   	COMBO_LIST_LENGTH);
	executer_execute( e );
}

static GtkWidget *
create_executer_dialog (Executer * e)
{
	//GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *frame1;
	//GtkWidget *combo1;
	//GtkWidget *combo_entry1;
	//GtkWidget *checkbutton1;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button3;
	gchar* options;

	e->m_gui.dialog = gnome_dialog_new (_("Execute with arguments"), NULL);
	gtk_window_set_transient_for (GTK_WINDOW(e->m_gui.dialog), GTK_WINDOW(app->widgets.window));
	gtk_window_set_position (GTK_WINDOW (e->m_gui.dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_policy (GTK_WINDOW (e->m_gui.dialog), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (e->m_gui.dialog), "exec", "Anjuta");
	gtk_widget_set_usize (e->m_gui.dialog, 400, -2);
	gnome_dialog_set_close (GNOME_DIALOG (e->m_gui.dialog), TRUE);

	dialog_vbox1 = GNOME_DIALOG (e->m_gui.dialog)->vbox;
	gtk_widget_show (dialog_vbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

	frame1 = gtk_frame_new (_("Command arguments"));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	e->m_gui.combo1 = gtk_combo_new ();
	gtk_widget_show (e->m_gui.combo1);
	gtk_container_add (GTK_CONTAINER (frame1), e->m_gui.combo1);

	gtk_container_set_border_width (GTK_CONTAINER (e->m_gui.combo1), 5);

	e->m_gui.combo_entry1 = GTK_COMBO (e->m_gui.combo1)->entry;
	gtk_widget_show (e->m_gui.combo_entry1);

	e->m_gui.check_terminal = gtk_check_button_new_with_label (_("Run in Terminal"));
	gtk_widget_show (e->m_gui.check_terminal);
	gtk_box_pack_start (GTK_BOX (vbox1), e->m_gui.check_terminal, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (e->m_gui.check_terminal), 5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (e->m_gui.check_terminal), TRUE);

	dialog_action_area1 = GNOME_DIALOG (e->m_gui.dialog)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (e->m_gui.dialog),
				    GNOME_STOCK_BUTTON_OK);
	button1 = g_list_last (GNOME_DIALOG (e->m_gui.dialog)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (e->m_gui.dialog),
				    GNOME_STOCK_BUTTON_CLOSE);
	button3 = g_list_last (GNOME_DIALOG (e->m_gui.dialog)->buttons)->data;
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	options = prop_get (e->props, EXECUTER_PROGRAM_ARGS_KEY);
	if (options)
	{
		gtk_entry_set_text (GTK_ENTRY (e->m_gui.combo_entry1), options);
		g_free (options);
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (e->m_gui.check_terminal), e->terminal);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (e->m_gui.dialog));

	gtk_signal_connect (GTK_OBJECT (e->m_gui.combo_entry1), "changed",
			    GTK_SIGNAL_FUNC (on_executer_entry_changed), e);
	gtk_signal_connect (GTK_OBJECT (e->m_gui.check_terminal), "toggled",
			    GTK_SIGNAL_FUNC (on_executer_checkbutton_toggled),
			    e);
	/* FIXME: fix this mess of braces, maybe overriding gnome_dialog_append_dialog */
	gtk_label_set_text(GTK_LABEL((g_list_last(gtk_container_children(GTK_CONTAINER((g_list_last(gtk_container_children(GTK_CONTAINER((g_list_first(gtk_container_children(GTK_CONTAINER(button1))))->data))))->data))))->data), _("Execute"));
	gtk_widget_show(button1);
	
	gtk_signal_connect (GTK_OBJECT(button1), "clicked",
				GTK_SIGNAL_FUNC(on_executer_run_button_clicked),
				e);

	gtk_widget_grab_focus (e->m_gui.combo_entry1);
	gtk_widget_grab_default (button1);
	return e->m_gui.dialog;
}

static void
on_executer_entry_changed (GtkEditable * editable, gpointer user_data)
{
	Executer *e = user_data;
	gchar *options;
	options = gtk_entry_get_text (GTK_ENTRY (editable));
	if (options)
		prop_set_with_key (e->props, EXECUTER_PROGRAM_ARGS_KEY,  options);
	else
		prop_set_with_key (e->props, EXECUTER_PROGRAM_ARGS_KEY, "");
}

static void
on_executer_checkbutton_toggled (GtkToggleButton * togglebutton,
				 gpointer user_data)
{
	Executer *e = user_data;
	e->terminal = gtk_toggle_button_get_active (togglebutton);
}

void
executer_execute (Executer * e)
{
	gchar *dir, *cmd, *command;

	/* Doing some checks before actualing starting */
	if (app->project_dbase->project_is_open) /* Project mode */
	{
		gint target_type;
		gchar* prog;
		
		prog = NULL;
		target_type = project_dbase_get_target_type (app->project_dbase);
		if (target_type >= PROJECT_TARGET_TYPE_END_MARK)
		{
			anjuta_error (_("The target executable of this Project is unknown"));
			return;
		}
		else if ( target_type != PROJECT_TARGET_TYPE_EXECUTABLE)
		{
			anjuta_warning (_("The target executable of this Project is not executable"));
		}
		prog = project_dbase_get_source_target (app->project_dbase);
		if (file_is_executable (prog) == FALSE)
		{
			anjuta_warning(_("The target executable does not exist for this Project"));
			g_free (prog);
			return;
		}
		g_free (prog);
	}
	else  /* File mode checks */
	{
		TextEditor *te;

		te = anjuta_get_current_text_editor ();
		if (!te)
		{
			anjuta_warning(_("No file or Project opened."));
			return;
		}
		if (te->full_filename == NULL)
		{
			anjuta_warning(_("No executable for this file."));
			return;
		}
		if (anjuta_get_file_property (FILE_PROP_IS_INTERPRETED, te->full_filename, 0) == 0)
		{
			gchar *prog, *temp;
			gint s_re, e_re;
			struct stat s_stat, e_stat;

			prog = NULL;
			prog = g_strdup (te->full_filename);
			temp = get_file_extension (prog);
			if (temp)
				*(--temp) = '\0';
			s_re = stat (te->full_filename, &s_stat);
			e_re = stat (prog, &e_stat);
			g_free (prog);
			if ((e_re != 0) || (s_re != 0))
			{
				anjuta_warning(_("No executable for this file."));
				return;
			}
			else if ((!text_editor_is_saved (te)) || (e_stat.st_mtime < s_stat.st_mtime))
			{
				anjuta_warning (_("Executable is not up-to-date."));
				/* But continue execution */
			}
		} /* else continue execution */
	}

	if (app->project_dbase->project_is_open)  /* Execute project */
	{
		cmd = command_editor_get_command (app->command_editor,
			COMMAND_EXECUTE_PROJECT);
		if (cmd == NULL)
		{
			anjuta_warning (_("Unable to execute Project. Check Settings->Commands."));
			return;
		}
		dir = project_dbase_get_module_dir (app->project_dbase, MODULE_SOURCE);
	}
	else /* Execute file */
	{
		TextEditor* te;
		
		te = anjuta_get_current_text_editor ();
		g_return_if_fail (te != NULL);

		if (te->full_filename == NULL || text_editor_is_saved(te) == FALSE)
		{
			anjuta_warning (_("This file has not been saved. Save it first."));
			return;
		}
		anjuta_set_file_properties (te->full_filename);
		cmd =	command_editor_get_command_file (app->command_editor,
							 COMMAND_EXECUTE_FILE, te->filename);
		if (cmd == NULL)
		{
			anjuta_warning (_("Unable to execute file. Check Settings->Commands."));
			return;
		}
		dir = g_dirname (te->full_filename);
	}
	command = g_strconcat ("anjuta_launcher ", cmd, NULL);
	g_free (cmd);

#ifdef DEBUG
	g_message("Raw Command is: %s", command);
#endif
	
	/* Get command and execute */
	if(e->terminal)
	{
		gchar* escaped_cmd;
		escaped_cmd = anjuta_util_escape_quotes(command);
		prop_set_with_key (e->props, "anjuta.current.command", escaped_cmd);

#ifdef DEBUG
		g_message("Escaped Command is: %s", escaped_cmd);
#endif
		
		cmd = command_editor_get_command (app->command_editor, COMMAND_TERMINAL);
		g_free(escaped_cmd);
	}
	else
	{
		prop_set_with_key (e->props, "anjuta.current.command", command);
		cmd = g_strdup (command);
	}

#ifdef DEBUG
	g_message("Final Command is: %s", cmd);
#endif

	anjuta_set_execution_dir (dir);
	if (dir) chdir (dir);
	gnome_execute_shell (dir, cmd);
	g_free (dir);
	g_free (command);
	g_free (cmd);
}


void
executer_save_session( Executer *e, ProjectDBase *p )
{
	g_return_if_fail( NULL != e );
	g_return_if_fail( NULL != p );
	g_return_if_fail( p->project_is_open );
	/* Save 'run in terminal option' */
	session_save_bool( p, SECSTR(SECTION_EXECUTER), szRunInTerminalItem, e->terminal );
	session_save_strings( p, SECSTR(SECTION_EXECUTERARGS), e->m_PgmArgs );
}

void
executer_load_session( Executer *e, ProjectDBase *p )
{
	g_return_if_fail( NULL != e );
	g_return_if_fail( NULL != p );
	g_return_if_fail( p->project_is_open );

	e->m_PgmArgs	= session_load_strings( p, SECSTR(SECTION_EXECUTERARGS), e->m_PgmArgs );
	e->terminal		= session_get_bool( p, SECSTR(SECTION_EXECUTER), szRunInTerminalItem, TRUE);
}



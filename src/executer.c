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

static GtkWidget* create_executer_dialog(Executer*);

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
	}
	return e;
}

void
executer_destroy (Executer * e)
{
	if (e) g_free (e);
}

void
executer_show (Executer * e)
{
	gtk_widget_show (create_executer_dialog (e));
}

static GtkWidget *
create_executer_dialog (Executer * e)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *frame1;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *checkbutton1;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button3;
	gchar* options;

	dialog1 = gnome_dialog_new (_("Execute"), NULL);
	gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_CENTER);
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "exec", "Anjuta");
	gtk_widget_set_usize (dialog1, 400, -2);
	gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);

	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

	frame1 = gtk_frame_new (_("Command arguments (if any)"));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	combo1 = gtk_combo_new ();
	gtk_widget_show (combo1);
	gtk_container_add (GTK_CONTAINER (frame1), combo1);
	gtk_container_set_border_width (GTK_CONTAINER (combo1), 5);

	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_show (combo_entry1);

	checkbutton1 = gtk_check_button_new_with_label (_("Run in Terminal"));
	gtk_widget_show (checkbutton1);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_EDGE);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_OK);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CANCEL);
	button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	options = prop_get (e->props, EXECUTER_PROGRAM_ARGS_KEY);
	if (options)
	{
		gtk_entry_set_text (GTK_ENTRY (combo_entry1), options);
		g_free (options);
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), e->terminal);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (dialog1));

	gtk_signal_connect (GTK_OBJECT (combo_entry1), "changed",
			    GTK_SIGNAL_FUNC (on_executer_entry_changed), e);
	gtk_signal_connect (GTK_OBJECT (checkbutton1), "toggled",
			    GTK_SIGNAL_FUNC (on_executer_checkbutton_toggled),
			    e);

	gtk_widget_grab_focus (button1);
	gtk_widget_grab_default (button1);
	return dialog1;
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
			anjuta_error (_("Target of this project is unknown"));
			return;
		}
		else if ( target_type != PROJECT_TARGET_TYPE_EXECUTABLE)
		{
			anjuta_warning (_("Target of this project is not executable"));
		}
		prog = project_dbase_get_source_target (app->project_dbase);
		if (file_is_executable (prog) == FALSE)
		{
			anjuta_warning(_("Executable does not exsits for this project."));
			string_free (prog);
			return;
		}
		string_free (prog);
	}
	else  /* File mode checks */
	{
		TextEditor *te;

		te = anjuta_get_current_text_editor ();
		if (!te)
		{
			anjuta_warning(_("No file or project opened."));
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
			string_free (prog);
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
			anjuta_warning (_("Unable to execute project. Check Settings->Commands."));
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
			anjuta_warning (_("This file is not saved. Save it first."));
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
	prop_set_with_key (e->props, "anjuta.current.command", command);
	string_free (cmd);
	
	/* Get command and execute */
	if(e->terminal)
		cmd = command_editor_get_command (app->command_editor, COMMAND_TERMINAL);
	else
		cmd = g_strdup (command);
	anjuta_set_execution_dir (dir);
	if (dir) chdir (dir);
	gnome_execute_shell (dir, cmd);
	string_free (dir);
	string_free (command);
	string_free (cmd);
}

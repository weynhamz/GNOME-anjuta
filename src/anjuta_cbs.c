/*
    anjuta_cbs.c
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
#include <signal.h>
#include <string.h>

#include <gnome.h>

#include "text_editor.h"
#include "anjuta.h"
#include "controls.h"
#include "fileselection.h"
#include "utilities.h"
#include "resources.h"
#include "messagebox.h"
#include "launcher.h"
#include "debugger.h"

/* Blurb ?? */
extern gboolean closing_state;

gint
on_anjuta_session_die(GnomeClient * client, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

/* Saves the current anjuta session */
gint on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
		       GnomeSaveStyle s_style, gint shutdown,
		       GnomeInteractStyle i_style, gint fast, gpointer data)
{
	gchar *argv[] = { "rm",	"-rf", NULL};
	gchar *prefix, **res_argv;
	gint res_argc, counter;
	GList* node;

	prefix = gnome_client_get_config_prefix (client);
	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);

	res_argc = g_list_length (app->text_editor_list) + 1;
	if (app->project_dbase->project_is_open)
		res_argc++;
	res_argv = g_malloc (res_argc * sizeof (char*));
	res_argv[0] = data;
	counter = 1;
	if (app->project_dbase->project_is_open)
	{
		res_argv[counter] = app->project_dbase->proj_filename;
		counter++;
	}
	node = app->text_editor_list;
	while (node)
	{
		TextEditor* te;
		
		te = node->data;
		if (te->full_filename)
		{
			res_argv[counter] = te->full_filename;
			counter++;
		}
		node = g_list_next (node);
	}
	gnome_client_set_restart_command(client, res_argc, res_argv);
	g_free (res_argv);
	anjuta_save_settings ();	
	return TRUE;
}

gint on_anjuta_delete (GtkWidget * w, GdkEvent * event, gpointer data)
{
	TextEditor *te;
	GList *list;
	gboolean file_not_saved;
	gint max_recent_files, max_recent_prjs;
	
	if (!app) return TRUE;
	file_not_saved = FALSE;
	max_recent_files = preferences_get_int (app->preferences, MAXIMUM_RECENT_FILES);
	max_recent_prjs = preferences_get_int (app->preferences, MAXIMUM_RECENT_PROJECTS);
	list = app->text_editor_list;
	while (list)
	{
		te = list->data;
		if (te->full_filename)
			app->recent_files = update_string_list (app->recent_files,
					te->full_filename, max_recent_files);
		if (!text_editor_is_saved (te))
			file_not_saved = TRUE;
		list = g_list_next (list);
	}
	if (app->project_dbase->project_is_open)
	{
		app->recent_projects =
			update_string_list (app->recent_projects,
					    app->project_dbase->proj_filename,
					    max_recent_prjs);
	}
	anjuta_save_settings ();

	/* No need to check for saved project, as it is done in 
	close project call later. */
	if (file_not_saved)
	{
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
			     _("One or more files are not saved.\n"
				"Do you still want to exit?"),
			     GNOME_STOCK_BUTTON_YES,
			     GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_anjuta_exit_yes_clicked), NULL,
			     NULL);
		return TRUE;
	}
	else
	{
		on_anjuta_exit_yes_clicked (NULL, NULL);
	}
	return TRUE;
}

void
on_anjuta_exit_yes_clicked (GtkButton * b, gpointer data)
{
	anjuta_clean_exit ();
}

void
on_anjuta_destroy (GtkWidget * w, gpointer data)
{
	/* Nothing to be done here */
}

void
on_anjuta_notebook_switch_page (GtkNotebook * notebook,
				GtkNotebookPage * page,
				gint page_num, gpointer user_data)
{
	anjuta_set_current_text_editor (anjuta_get_notebook_text_editor
					(page_num));
	anjuta_grab_text_focus ();
}

void
on_anjuta_dnd_drop (gchar* filename, gpointer data)
{
	anjuta_goto_file_line (filename, 0);
}

void
on_open_filesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *full_filename;
	gchar *entry_filename = NULL;
	int i;
	GList * list;
	int elements;
	
	list = fileselection_get_filelist(app->fileselection);
	elements = g_list_length(list);
	/* If filename is only only written in entry but not selected (Bug #506441) */
	if (elements == 0)
	{
		entry_filename = fileselection_get_filename(app->fileselection);
		if (entry_filename)
		{
			list = g_list_append(list, entry_filename);
			elements++;
		}
	}
	for(i=0;i<elements;i++)
	{
		/*  full_filename = fileselection_get_filename (app->fileselection); */
		/*	full_filename = (gchar *)g_list_nth_data(list,i); */
		full_filename = g_strdup(g_list_nth_data(list,i));
		/*printf("Filename retrived = %s\n",full_filename);*/
		if (!full_filename)
			return;
		if (strlen (extract_filename (full_filename)) == 0)
		{
			g_free (full_filename);
			return;
		}
		if (file_is_regular (full_filename) == FALSE)
		{
			anjuta_error (_("Not a regular file: %s."), full_filename);
			g_free (full_filename);
			return;
		}
		if (file_is_readable (full_filename) == FALSE)
		{
			anjuta_error (_("No read permission for: %s."), full_filename);
			g_free (full_filename);
			return;
		}
		/*printf("I have reached this point\n");*/
		anjuta_goto_file_line (full_filename, -1);
		/*printf("I have reached this point goto line\n");*/
		//gtk_widget_hide (app->fileselection);
		g_free (full_filename);
	}

	if (entry_filename)
	{
		g_list_remove(list, entry_filename);
		g_free(entry_filename);
	}
	/* g_free(list); */
}

void
on_save_as_filesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *filename, *buff;

	filename = fileselection_get_filename (app->save_as_fileselection);
	if (file_is_regular (filename))
	{
		buff =
			g_strdup_printf (_
					 ("The file \"%s\" already exists.\nDo you want to overwrite it?."),
					 filename);
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION, buff,
			     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_save_as_overwrite_yes_clicked), NULL,
			     user_data);
		g_free (buff);
	}
	else
		on_save_as_overwrite_yes_clicked (NULL, user_data);
	g_free (filename);

	if (preferences_get_int (app->preferences, EDITOR_TABS_ORDERING))
		anjuta_order_tabs ();
}

void
on_save_as_overwrite_yes_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	gchar *full_filename, *saved_filename, *saved_full_filename;
	gint page_num;
	GtkWidget *child;
	gint status;

	full_filename = fileselection_get_filename (app->save_as_fileselection);
	if (!full_filename)
		return;

	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	g_return_if_fail (strlen (extract_filename (full_filename)) != 0);

	saved_filename = te->filename;
	saved_full_filename = te->full_filename;
	
	te->full_filename = g_strdup (full_filename);
	te->filename = g_strdup (extract_filename (full_filename));
	status = text_editor_save_file (te);
	gtk_widget_hide (app->save_as_fileselection);
	if (status == FALSE)
	{
		g_free (te->filename);
		te->filename = saved_filename;
		g_free (te->full_filename);
		te->full_filename = saved_full_filename;
		if (closing_state) closing_state = FALSE;
		g_free (full_filename);
		return;
	} else {
		if (saved_filename)
			g_free (saved_filename);
		if (saved_full_filename)
		{
			g_free (saved_full_filename);
		}
		if (closing_state)
		{
			anjuta_remove_current_text_editor ();
			closing_state = FALSE;
		}
		else
		{
			text_editor_set_hilite_type(te);
			if (te->mode == TEXT_EDITOR_PAGED)
			{
				GtkLabel* label;
				
				page_num =
					gtk_notebook_get_current_page (GTK_NOTEBOOK
									   (app->widgets.notebook));
				g_return_if_fail (page_num >= 0);
				child =
					gtk_notebook_get_nth_page (GTK_NOTEBOOK
								   (app->widgets.notebook), page_num);
				/* This crashes */
				/* gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (app->widgets.notebook),
								 child,
								 anjuta_get_notebook_text_editor
								 (page_num)->filename);
				*/
				label = GTK_LABEL(te->widgets.tab_label);
				gtk_label_set_text(GTK_LABEL(label), anjuta_get_notebook_text_editor
								 (page_num)->filename);
	
				gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(app->widgets.notebook),
								child,
								anjuta_get_notebook_text_editor
								(page_num)->filename);
			}
		}
		anjuta_update_title ();
		g_free (full_filename);
	}
}

gboolean
on_anjuta_window_focus_in_event (GtkWidget * widget,
				 GdkEventFocus * event, gpointer user_data)
{
	gint page_num;
	TextEditor *te;

	if (g_list_length (GTK_NOTEBOOK (app->widgets.notebook)->children) >
	    0)
	{
		page_num =
			gtk_notebook_get_current_page (GTK_NOTEBOOK
						       (app->widgets.
							notebook));
		if (page_num >= 0)
			te = anjuta_get_notebook_text_editor (page_num);
		else
			te = NULL;
	}
	else if (app->text_editor_list == NULL)
	{
		te = NULL;
	}
	else
	{
		/* If there is TextEditor in the list, but no page 
		 * * in the notebook, we don't change the current
		 * * TextEditor
		 */
		return FALSE;
	}
	if (te == anjuta_get_current_text_editor ())
		return FALSE;
	anjuta_set_current_text_editor (te);
	return FALSE;
}

enum {
	m__ = 0,
	mS_ = GDK_SHIFT_MASK,
	m_C = GDK_CONTROL_MASK,
	mSC = GDK_SHIFT_MASK | GDK_CONTROL_MASK
};

enum {
	ID_NEXTBUFFER = 1, /* Note: the value mustn't be 0 ! */
	ID_PREVBUFFER
};

typedef struct {
	int modifiers;
	unsigned int gdk_key;
	int id;
} ShortcutMapping;

static ShortcutMapping global_keymap[] = {
	{ m_C, GDK_Tab,		 ID_NEXTBUFFER },
	{ mSC, GDK_ISO_Left_Tab, ID_PREVBUFFER },
	{ 0,   0,		 0 }
};

/*!
state flag for Ctrl-TAB
*/
static gboolean g_tabbing = FALSE;

gint
on_anjuta_window_key_press_event (GtkWidget   *widget,
				  GdkEventKey *event,
				  gpointer     user_data)
{
	int modifiers;
	int i;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_APP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	modifiers = event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK);
  
	for (i = 0; global_keymap[i].id; i++)
		if (event->keyval == global_keymap[i].gdk_key &&
		    (event->state & global_keymap[i].modifiers) == global_keymap[i].modifiers)
			break;

	if (!global_keymap[i].id)
		return FALSE;

	switch (global_keymap[i].id) {
	case ID_NEXTBUFFER:
	case ID_PREVBUFFER: {
		GtkNotebook *notebook = GTK_NOTEBOOK (app->widgets.notebook);
		int pages_nb;
		int cur_page;

		if (!notebook->children)
			return FALSE;

    if (!g_tabbing)
    {
      g_tabbing = TRUE;
    }
    
		pages_nb = g_list_length (notebook->children);
		cur_page = gtk_notebook_get_current_page (notebook);

		if (global_keymap[i].id == ID_NEXTBUFFER)
			cur_page = (cur_page < pages_nb - 1) ? cur_page + 1 : 0;
		else
			cur_page = cur_page ? cur_page - 1 : pages_nb -1;

		gtk_notebook_set_page (notebook, cur_page);

		break;
	}
	default:
		return FALSE;
	}

	/* Note: No reason for a shortcut to do more than one thing a time */
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");

	return TRUE;
}

gint
on_anjuta_window_key_release_event (GtkWidget   *widget,
				  GdkEventKey *event,
				  gpointer     user_data)
{
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_APP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

  if (g_tabbing && ((event->keyval == GDK_Control_L) || (event->keyval == GDK_Control_R)))
  {
		GtkNotebook *notebook = GTK_NOTEBOOK (app->widgets.notebook);
    GtkWidget *widget;
    int cur_page;
    g_tabbing = FALSE;
    /*
    move the current notebook page to first position
    that maintains Ctrl-TABing on a list of most recently edited files
    */
    cur_page = gtk_notebook_get_current_page (notebook);
    widget = gtk_notebook_get_nth_page (notebook, cur_page);
    gtk_notebook_reorder_child (notebook, widget, 0);
  }
  
  return FALSE;
}

void
on_save_as_filesel_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->save_as_fileselection);
	closing_state = FALSE;
}

void
on_build_msg_save_ok_clicked(GtkButton * button, gpointer user_data)
{
	gchar *filename, *buff;

	filename = fileselection_get_filename (app->save_as_build_msg_sel);
	if (file_is_regular (filename))
	{
		buff =
			g_strdup_printf (_
					 ("The file \"%s\" already exists.\nDo you want to overwrite it?."),
					 filename);
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION, buff,
			     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_build_msg_save_overwrite), NULL,
			     user_data);
		g_free (buff);
	}
	else
		on_build_msg_save_overwrite(NULL, user_data);
	g_free (filename);
}

void
on_build_msg_save_cancel_clicked(GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->save_as_build_msg_sel);
	closing_state = FALSE;
}


void 
on_build_msg_save_overwrite(GtkButton * button, gpointer user_data)
{
	gchar *filename;
	FILE *msgfile; 
	
	filename = fileselection_get_filename (app->save_as_build_msg_sel);
	msgfile = fopen(filename, "w");
	if (! msgfile)
	{
		anjuta_error("Could not open file for writing");
		return;
	}
	
	anjuta_message_manager_save_build(app->messages, msgfile);
			
	fclose(msgfile);

	return;
}


void
on_prj_list_undock_button_clicked (GtkButton * button, gpointer user_data)
{
	project_dbase_undock (app->project_dbase);
}


void
on_prj_list_hide_button_clicked (GtkButton * button, gpointer user_data)
{
	project_dbase_hide (app->project_dbase);
}


void
on_mesg_win_hide_button_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(app->messages));
}


void
on_mesg_win_undock_button_clicked (GtkButton * button, gpointer user_data)
{
	anjuta_message_manager_undock (app->messages);
}

void
on_anjuta_progress_cancel (gpointer data)
{
	app->in_progress = FALSE;
	app->progress_cancel_cb (data);
	return;
}

gdouble
on_anjuta_progress_cb (gpointer data)
{
	return (app->progress_value);
}

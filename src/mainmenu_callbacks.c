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
#include <libgnomeui/gnome-window-icon.h>

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
#include "resources.h"
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

void on_toolbar_find_clicked (GtkButton * button, gpointer user_data);

gboolean closing_state;		/* Do not tamper with this variable  */

static char *insert_date_time(void);

static gchar *insert_header_c( TextEditor *te);

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

	if (user_data != NULL) {
		te = (TextEditor*)user_data;
	} else {
		te = anjuta_get_current_text_editor ();
	};
	if (te == NULL)
		return;
	if (te->full_filename == NULL)
	{
		anjuta_set_current_text_editor (te);
		gtk_widget_show (app->save_as_fileselection);
		return;
	}
	ret = text_editor_save_file (te);
	if (closing_state && ret == TRUE)
	{
		anjuta_remove_text_editor (te);
		closing_state = FALSE;
	}
}


void
on_save_as1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	fileselection_set_filename (app->save_as_fileselection, te->full_filename);
	gtk_widget_show (app->save_as_fileselection);
}

void
on_save_all1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_save_all_files();
}

void
on_close_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;

	if (user_data != NULL) {
		te = (TextEditor*)user_data;
	} else {
		te = anjuta_get_current_text_editor ();
	};
	if (te == NULL)
		return;
	
	if (te->used_by_cvs) {
		GtkWidget* dialog;
		gint value;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
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
		dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE, mesg);
		g_free (mesg);
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), 
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								GTK_STOCK_NO, GTK_RESPONSE_NO,
								GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
										 GTK_RESPONSE_CANCEL);
		res = gtk_dialog_run (GTK_DIALOG (dialog));
		if (res == GTK_RESPONSE_YES)
			on_save1_activate (NULL, te);
		else if (res == GTK_RESPONSE_NO)
		{
			anjuta_remove_current_text_editor ();
			closing_state = FALSE;
		}
		else
			closing_state = FALSE;
		gtk_widget_destroy (dialog);
	}
	else
		anjuta_remove_text_editor (te);
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
			if (text_editor_is_saved (te))
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
	GtkWidget *dialog;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	sprintf (mesg, _("Are you sure you want to reload '%s'?\n"
					 "Any unsaved changes will be lost."),
			 te->filename);

	dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO, mesg);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
									 GTK_RESPONSE_NO);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		text_editor_load_file (te);
		anjuta_update_title ();
	}
	gtk_widget_destroy (dialog);
}

void
on_new_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
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
on_import_project_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	create_project_import_gui ();
}

void
on_open_project1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	anjuta_open_project();
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
#warning "G3: Show print setup preferences page here"

	//gtk_notebook_set_page (GTK_NOTEBOOK
	//		       (app->preferences->notebook), 4);
	gtk_widget_show (GTK_WIDGET (app->preferences));
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
on_editor_command_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, (gint) user_data, 0, 0);
}

void
on_transform_eolchars1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	glong mode = (glong)user_data;
	
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_EOL_CONVERT, mode, 0);
}

static gchar *
insert_c_gpl_notice(void)
{
	gchar *GPLNotice =
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

	return  GPLNotice;
}

void
on_insert_c_gpl_notice(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *GPLNotice;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	GPLNotice = insert_c_gpl_notice();
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)GPLNotice);
}

static gchar *
insert_cpp_gpl_notice(void)
{
	gchar *GPLNotice =
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

	return GPLNotice;
}

void
on_insert_cpp_gpl_notice(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *GPLNotice;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	GPLNotice = insert_cpp_gpl_notice();
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)GPLNotice);
}

static gchar *
insert_py_gpl_notice(void)
{
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
	return GPLNotice;
}

void
on_insert_py_gpl_notice(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *GPLNotice;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	GPLNotice = insert_py_gpl_notice();
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)GPLNotice);
}

static gchar *
insert_username(void)
{
	gchar *Username;
	
	Username = getenv("USERNAME");
	if (!Username)
		Username =
			anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
									IDENT_NAME);
	if (!Username)
		Username = getenv("USER");
	return Username;
}

void
on_insert_username(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *Username;
		
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	Username = insert_username();
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)Username);
}

static gchar *insert_name(void)
{
	gchar *Username;

	Username =
		anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								IDENT_NAME);
	  if (!Username)
			Username = getenv("USERNAME");
		if (!Username)
			Username = getenv("USER");
	return Username;
}

static gchar *insert_email(void)
{
	gchar *email;
	gchar *Username;

	email =
		anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								IDENT_EMAIL);
	if (!email)
	{
		email = getenv("HOSTNAME");
		Username = getenv("USERNAME");
		if (!Username)
			Username = getenv("USER");
		email = g_strconcat(Username, "@", email, NULL);
	}
	return email;
}


static gchar *
insert_copyright(void)
{
	gchar *Username;
	gchar *copyright;
	gchar datetime[20];
	struct tm *lt;
	time_t cur_time = time(NULL);

	lt = localtime(&cur_time);
	strftime (datetime, 20, N_("%Y"), lt);
	Username = insert_name();
	copyright = g_strconcat("Copyright  ", datetime, "  ", Username, NULL);

	return copyright;
}

static gchar *
insert_changelog_entry(void)
{
	gchar *Username;
	gchar *email;
	gchar *CLEntry;
	gchar datetime[20];
	struct tm *lt;
	time_t cur_time = time(NULL);

	CLEntry = g_new(gchar, 200);
	lt = localtime(&cur_time);
	strftime (datetime, 20, N_("%Y-%m-%d"), lt);

	Username =  insert_name();
	email = insert_email();
	sprintf(CLEntry,"%s  %s <%s>\n", datetime, Username, email);
	g_free(email);
  	
	return  CLEntry;
}

void
on_insert_changelog_entry(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *changelog;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
  	return;
	changelog = insert_changelog_entry();
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)changelog);
	g_free(changelog);
}

static char *
insert_date_time(void)
{
	time_t cur_time = time(NULL);
	gchar *DateTime;

	DateTime = g_new(gchar, 100);
	sprintf(DateTime,ctime(&cur_time));
	return DateTime;
}                                                            ;

void
on_insert_date_time(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *DateTime;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	DateTime = insert_date_time();
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)DateTime);
	g_free(DateTime);
}

static gchar *
insert_header_template(TextEditor *te)
{
	gchar *header_template =
	"_H\n"
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
	"#endif /* _";
	gchar *buffer;
	gchar *name = NULL;
	gchar mesg[256];
	gint i;

	i = strlen(te->filename);
	if ( g_strcasecmp((te->filename) + i - 2, ".h") == 0)
		name = g_strndup(te->filename, i - 2);
	else
	{
		sprintf(mesg, _("The file \"%s\" is not a header file."),
				te->filename);
		anjuta_warning (mesg);
		return NULL;
	}
	g_strup(name);  /* do not use with GTK2 */
	buffer = g_strconcat("#ifndef _", name, "_H\n#define _", name,
						header_template, name, "_H */\n", NULL);

	g_free(name);
	return buffer;
}


void
on_insert_header_template(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *header;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	header =  insert_header_template(te);
	if (header == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)header);
	g_free(header);
}

static gchar *
insert_header_c( TextEditor *te)
{
 	gchar *buffer;
	gchar *tmp;
	gchar *star;
	gchar *copyright;
	gchar *email;

	star =  g_strnfill(75, '*');
	tmp = g_strdup(te->filename);
	buffer = g_strconcat("/", star, "\n *            ", tmp, "\n *\n", NULL);
	g_free(tmp);
	tmp = insert_date_time();
	buffer = g_strconcat( buffer, " *  ", tmp, NULL);
	g_free(tmp);
	copyright = insert_copyright();
	buffer = g_strconcat(buffer, " *  ", copyright, "\n", NULL);
	g_free(copyright);
	email = insert_email();
	buffer = g_strconcat(buffer, " *  ", email, "\n", NULL);
	g_free(email);
	buffer = g_strconcat(buffer, " ", star, "*/\n", NULL);
	g_free(star);

	return buffer;
}

void
on_insert_header(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *header;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	header = insert_header_c(te);
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)header);
	g_free(header);
}

void
on_insert_switch_template(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *switch_template =
	"switch ( )\n"
	"{\n"
	"\tcase  :\n"
	"\t\t;\n"
	"\t\tbreak;\n"
	"\tcase  :\n"
	"\t\t;\n"
	"\t\tbreak;\n"
	"\tdefaults:\n"
	"\t\t;\n"
	"\t\tbreak;\n"
	"}\n";

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)switch_template);
}

void
on_insert_for_template(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *for_template =
	"for ( ; ; )\n"
	"{\n"
	"\t;\n"
	"}\n";

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)for_template);
}

void
on_insert_while_template(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *while_template =
	"while ( )\n"
	"{\n"
	"\t;\n"
	"}\n";

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)while_template);
}

void
on_insert_ifelse_template(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *ifelse_template =
	"if ( )\n"
	"{\n"
	"\t;\n"
	"}\n"
	"else\n"
	"{\n"
	"\t;\n"
	"}\n";

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)ifelse_template);
}

void
on_insert_cvs_author(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Author";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_date(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Date";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_header(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Header";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_id(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Id";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_log(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Log";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_name(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Name";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_revision(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Revision";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
}

void
on_insert_cvs_source(GtkMenuItem * menuitem, gpointer user_data)
{
	TextEditor *te;
	gchar *cvs_string_value = "Source";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)cvs_string);
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
	anjuta_not_implemented (__FILE__, __LINE__);
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



void on_prev_occur(GtkMenuItem * menuitem, gpointer user_data)
{
    TextEditor* te;
	gboolean ret;
	gchar *buffer = NULL;
    gint return_;
	te = anjuta_get_current_text_editor();
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
    return_=text_editor_find(te,buffer,TEXT_EDITOR_FIND_SCOPE_CURRENT,0,0,1,1);
	
	g_free(buffer);

}

void on_next_occur(GtkMenuItem * menuitem, gpointer user_data)
{
    TextEditor* te;
	gboolean ret;
	gchar *buffer = NULL;
    gint return_;
	te = anjuta_get_current_text_editor();
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
    return_=text_editor_find(te,buffer,TEXT_EDITOR_FIND_SCOPE_CURRENT,1,0,1,1);
	
	g_free(buffer);

}

void on_comment_block (GtkMenuItem * menuitem, gpointer user_data)
{
    TextEditor* te;
	te = anjuta_get_current_text_editor();
	if(!te) return;
    aneditor_command (te->editor_id, ANE_BLOCKCOMMENT, 0, 0);
}

void on_comment_box (GtkMenuItem * menuitem, gpointer user_data)
{
    TextEditor* te;
	te = anjuta_get_current_text_editor();
	if(!te) return;
    aneditor_command (te->editor_id, ANE_BOXCOMMENT, 0, 0);
}

void on_comment_stream (GtkMenuItem * menuitem, gpointer user_data)
{
    TextEditor* te;
	te = anjuta_get_current_text_editor();
	if(!te) return;
    aneditor_command (te->editor_id, ANE_STREAMCOMMENT, 0, 0);
}

void
on_goto_line_no1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *gt;
	gt = gotoline_new ();
	gtk_widget_show (gt);
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
	an_message_manager_previous (app->messages);
}

void
on_goto_next_mesg1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	an_message_manager_next (app->messages);
}

void
on_edit_app_gui1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_edit_gui (app->project_dbase);
}


void
on_save_build_messages_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (!an_message_manager_build_is_empty(app->messages))
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
	state = an_message_manager_is_shown(app->messages);
	if(state) {
		gtk_widget_hide(GTK_WIDGET(app->messages));
	} else {
		an_message_manager_show (app->messages, MESSAGE_NONE);
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
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"margin.linenumber.visible", state);
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
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"margin.marker.visible", state);
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
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"margin.fold.visible", state);
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
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"view.indentation.guides", state);
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
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"view.whitespace", state);
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
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"view.eol", state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_VIEWEOL, state, 0);
		node = g_list_next (node);
	}
}

void
on_editor_linewrap1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	GList *node;
	TextEditor *te;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
								"view.line.wrap", state);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) (node->data);
		aneditor_command (te->editor_id, ANE_LINEWRAP, state, 0);
		node = g_list_next (node);
	}
}

#define MAX_ZOOM_FACTOR 8
#define MIN_ZOOM_FACTOR -8

void
on_zoom_text_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	AnjutaPreferences *p = ANJUTA_PREFERENCES (app->preferences);
	gint zoom;
	gchar buf[20];
	const gchar *zoom_text = (const gchar *) user_data;

	if (!zoom_text)
		zoom = 0;
	else if (0 == strncmp(zoom_text, "++", 2))
		zoom = prop_get_int(p->props, TEXT_ZOOM_FACTOR, 0) + 2;
	else if (0 == strncmp(zoom_text, "--", 2))
		zoom = prop_get_int(p->props, TEXT_ZOOM_FACTOR, 0) - 2;
	else
		zoom = atoi(zoom_text);
	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;
	g_snprintf(buf, 20, "%d", zoom);
	prop_set_with_key (p->props, TEXT_ZOOM_FACTOR, buf);
	anjuta_set_zoom_factor(zoom);
}

void
on_anjuta_toolbar_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gboolean state;
	state = GTK_CHECK_MENU_ITEM (menuitem)->active;
	anjuta_toolbar_set_view ((gchar *) user_data, state, TRUE, TRUE);
}

void
on_update_tagmanager_activate (GtkMenuItem * menuitem, gpointer user_data)
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
    //trying to restore line no where i was before autoformat invoked
    gint lineno;
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
    lineno=aneditor_command (te->editor_id, ANE_GET_LINENO, 0, 0);
	if (te == NULL)
		return;
	text_editor_autoformat (te);
	anjuta_update_title();
    text_editor_goto_line(te,lineno+1,TRUE);
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

void
on_ordertab1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (GTK_CHECK_MENU_ITEM(menuitem)->active)
		anjuta_order_tabs();
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

void
on_toggle_breakpoint1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_toggle_breakpoint(debugger.breakpoints_dbase);
}

void
on_set_breakpoint1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_add (debugger.breakpoints_dbase);
}

void
on_disable_all_breakpoints1_activate (GtkMenuItem * menuitem,
				      gpointer user_data)
{
	breakpoints_dbase_disable_all (debugger.breakpoints_dbase);
}

void
on_show_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_show (debugger.breakpoints_dbase);
}

void
on_clear_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	breakpoints_dbase_remove_all (debugger.breakpoints_dbase);
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

void
on_info_memory_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *win_memory;

	win_memory = memory_info_new (NULL);
	gtk_widget_show(win_memory);
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
	GtkWidget *w = create_eval_dialog (GTK_WINDOW(app->widgets.window));
	gtk_widget_show (w);
}

void
on_debugger_add_watch_activate (GtkMenuItem * menuitem, gpointer user_data)
{
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

/*************************************************************************************************/
void
on_cvs_update_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_UPDATE, NULL, FALSE);
}

void
on_cvs_commit_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_COMMIT, NULL, FALSE);
}

void
on_cvs_status_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_STATUS, NULL, FALSE);
}

void
on_cvs_log_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_LOG, NULL, FALSE);
}

void
on_cvs_add_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_ADD, NULL, FALSE);
}

void
on_cvs_remove_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_gui(app->cvs, CVS_ACTION_REMOVE, NULL, FALSE);
}

void
on_cvs_diff_file_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_diff_gui (app->cvs, NULL, FALSE);
}

void
on_cvs_update_project_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_gui(app->cvs, CVS_ACTION_UPDATE, prj, TRUE);
}

void
on_cvs_commit_project_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	
	/* Do not bypass dialog for commit as we need to get the log */
	create_cvs_gui(app->cvs, CVS_ACTION_COMMIT, prj, FALSE);
}

void
on_cvs_import_project_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_import_gui (app->cvs);
}

void
on_cvs_project_status_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_gui(app->cvs, CVS_ACTION_STATUS, prj, TRUE);
}

void
on_cvs_project_log_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_gui(app->cvs, CVS_ACTION_LOG, prj, TRUE);
}

void
on_cvs_project_diff_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar* prj;
	prj = app->project_dbase->top_proj_dir;
	create_cvs_diff_gui (app->cvs, prj, TRUE);
}

void
on_cvs_login_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	create_cvs_login_gui (app->cvs);
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
	src_paths_show (app->src_paths);
}

void
on_set_commands1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	command_editor_show (app->command_editor);
}

void
on_set_preferences1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_widget_show (GTK_WIDGET (app->preferences));
}

void
on_set_style_editor_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	style_editor_show (app->style_editor);
}

void
on_file_view_filters_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	fv_customize(TRUE);
}

void
on_edit_user_properties1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar* user_propfile = g_strconcat (app->dirs->home, "/.anjuta" PREF_SUFFIX "/user.properties", NULL);
	anjuta_goto_file_line_mark (user_propfile, 1, FALSE);
	g_free (user_propfile);
}

void
on_set_default_preferences1_activate (GtkMenuItem * menuitem,
				      gpointer user_data)
{
	anjuta_preferences_reset_defaults (ANJUTA_PREFERENCES (app->preferences));
}

void
on_start_with_dialog_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	start_with_dialog_show (GTK_WINDOW (app->widgets.window),
							app->preferences, TRUE);
}

void
on_setup_wizard_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	// TODO.
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
		anjuta_goto_tag(buffer, te, (gboolean) user_data);
}

void
on_lookup_symbol_activate (GtkMenuItem * menuitem, gpointer user_data)
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
	anjuta_help_show (app->help_system);
}

void
on_url_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (user_data)
	{
		anjuta_res_url_show(user_data);
	}
}

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

void
on_about1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *about_dlg = about_box_new ();
	gtk_widget_show (about_dlg);
}

void
on_findnext1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	on_toolbar_find_clicked ( NULL, NULL );
}

void
on_enterselection (GtkMenuItem * menuitem, gpointer user_data)
{
    enter_selection_as_search_target();
	gtk_widget_grab_focus (app->widgets.toolbar.main_toolbar.find_entry);
}

void on_customize_shortcuts_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *dialog;
	gchar *message = _("Hover the mouse pointer over any menu item and press"
					"\n the shortcut key to associate with it.");
	anjuta_information (message);
}

void on_tool_editor_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	anjuta_tools_edit();
}

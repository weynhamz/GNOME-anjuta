/*  cvs.c (C) 2002 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "text_editor.h"
#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "cvs.h"
#include "cvs_gui.h"
#include "launcher.h"
#include "anjuta.h"
#include "lexer.h"

#define DEBUG

/* struct is private, use functions to access members */
struct _CVS
{
	gboolean force_update;
	gboolean unified_diff;
	gboolean context_diff;
	gboolean use_date_diff;
	
	guint compression;
	
	gboolean editor_destroyed;
};

/* Callbacks for launcher */
static void on_cvs_stdout (gchar * line);
static void on_cvs_stderr (gchar * line);
static void on_cvs_buffer_in (gchar * line);
static void on_cvs_terminate (int status, time_t time);

/* Utility functions */
static void launch_cvs_command (gchar * command, gchar * dir);
static gchar *get_full_cvsroot (CVS * cvs, ServerType type, 
		gchar* server, gchar* dir, gchar* user);
static gchar *add_compression (CVS * cvs);

/* Server types translation table */
static gchar *server_type_identifiers[CVS_END] =
	{ ":local", ":pserver", ":ext", ":server" };

/* Text editor for diff */
static TextEditor *diff_editor = NULL;
		
/* 
	Initialisize cvs module. Read values from properties. 
*/

CVS *
cvs_new (PropsID p)
{
	CVS *cvs = g_new0 (CVS, 1);
	
	cvs->force_update = prop_get_int (p, "cvs.update.force", 0);
	cvs->unified_diff = prop_get_int (p, "cvs.diff.unified", 1);
	cvs->context_diff = prop_get_int (p, "cvs.diff.context", 0);
	cvs->use_date_diff = prop_get_int (p, "cvs.diff.usedate", 0);
	cvs->compression = prop_get_int (p, "cvs.compression", 3);

	cvs->editor_destroyed = FALSE;
	
	return cvs;
}

void cvs_set_editor_destroyed (CVS* cvs)
{
	g_return_if_fail (cvs != NULL);
	cvs->editor_destroyed = TRUE;
}


/*
	Following functions allow to set adjust values in the
	cvs structure.
*/

void
cvs_set_force_update (CVS * cvs, gboolean force_update)
{
	g_return_if_fail (cvs != NULL);
	cvs->force_update = force_update;
}

void
cvs_set_unified_diff (CVS * cvs, gboolean unified_diff)
{
	g_return_if_fail (cvs != NULL);
	cvs->unified_diff = unified_diff;
}

void
cvs_set_context_diff (CVS * cvs, gboolean context_diff)
{
	g_return_if_fail (cvs != NULL);
	cvs->context_diff = context_diff;
}

void
cvs_set_compression (CVS * cvs, guint compression)
{
	g_return_if_fail (cvs != NULL);
	cvs->compression = compression;
}

void
cvs_set_diff_use_date(CVS* cvs, gboolean state)
{
	g_return_if_fail (cvs != NULL);
	cvs->use_date_diff = state;
}

/*
	Following functions allow to access the cvs module.
*/

gboolean
cvs_get_force_update(CVS * cvs)
{
	g_return_val_if_fail (cvs != NULL, 0);
	return cvs->force_update;
}

gboolean 
cvs_get_unified_diff(CVS* cvs)
{
	g_return_val_if_fail (cvs != NULL, 0);
	return cvs->unified_diff;
}

gboolean
cvs_get_context_diff(CVS* cvs)
{
	g_return_val_if_fail (cvs != NULL, 0);
	return cvs->context_diff;
}

guint
cvs_get_compression (CVS * cvs)
{
	g_return_val_if_fail (cvs != NULL, 0);
	return cvs->compression;
}

gboolean
cvs_get_diff_use_date(CVS* cvs)
{
	g_return_if_fail (cvs != NULL);
	return cvs->use_date_diff;
}

/* 
	Free the memory of the cvs module 
*/

void
cvs_destroy (CVS * cvs)
{
	g_return_if_fail (cvs != NULL);
	g_free (cvs);
}


/* CVS Commands: */

/*
	Updates the working copy of a file in local_dir. 
	If brach == NULL it is ignored 
*/

void
cvs_update (CVS * cvs, gchar * filename, gchar * branch, gboolean is_dir)
{
	gchar *command = NULL;
	gchar *compression;
	gchar *file;
	gchar *dir;

	g_return_if_fail (cvs != NULL);

	if (is_dir) {
		file = NULL;
		dir = g_strdup (filename);
	} else {
		file = extract_filename (filename);
		dir = extract_directory (filename);
	}

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " update ", NULL);
	if (cvs->force_update)
		command = g_strconcat (command, " -P -d -A ", NULL);
	
	if (branch != NULL && strlen (branch) > 0)
		command = g_strconcat (command, "-j ", branch, " ", NULL);
	if (file) 
		command = g_strconcat (command, file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS Updating "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	
	launch_cvs_command (command, dir);

	g_free (command);
	g_free (compression);
	g_free (dir);
}

/* 
	Commits the changes in the working copy to the repository.
	message is the log message. 
*/

void
cvs_commit (CVS * cvs, gchar * filename, gchar * revision,
		 gchar * message, gboolean is_dir)
{
	gchar *command;
	gchar *compression;
	gchar *file;
	gchar *dir;
	gchar *escaped_message;

	g_return_if_fail (cvs != NULL);

	if (is_dir) {
		file = NULL;
		dir = g_strdup (filename);
	} else {
		file = extract_filename (filename);
		dir = extract_directory (filename);
	}

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " commit ", NULL);
	if (revision != NULL && strlen (revision) > 0)
		command = g_strconcat (command, "-r ", revision, " ", NULL);

	escaped_message = anjuta_util_escape_quotes(message);
	command = g_strconcat (command, "-m \"", escaped_message, "\" ",NULL);
	g_free(escaped_message);
	
	if (file) 
		command = g_strconcat (command, file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS Committing "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	launch_cvs_command (command, dir);

	g_free (command);
	g_free (compression);
	g_free (dir);
}

/* 
	Import a project to a repository. Fails if no project is currently
	open. The base directory of the project is set the checked out 
	project directory.
*/

void cvs_import_project (CVS * cvs, ServerType type, gchar* server,
			gchar* dir, gchar* user, gchar* module, gchar* release,
			gchar* vendor, gchar* message)
{
	gchar* command;
	gchar* compression;
	gchar* cvsroot;
	gchar* prj_dir;
	gchar* escaped_message;
	
	g_return_if_fail (cvs != NULL);
	g_return_if_fail (app->project_dbase->project_is_open == TRUE);
	
	cvsroot = get_full_cvsroot (cvs, type, server, dir, user);
	compression = add_compression (cvs);
	
	escaped_message = anjuta_util_escape_quotes(message);
	command = g_strconcat ("cvs ", "-d ", cvsroot, " ", compression, 
							" import ", NULL);
	command = g_strconcat (command, "-m \"", escaped_message, "\" ",NULL);
	g_free(escaped_message);
	
	if (!strlen (module))
		return;
	if (!strlen (vendor))
	{
		g_free (vendor);
		vendor = g_strdup ("none");
	}
	if (!strlen (release))
	{
		g_free (release);
		release = g_strdup ("start");
	}
	command = g_strconcat (command, module, " ", vendor, " ", release, NULL);
	
	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS Importing...\n"), MESSAGE_CVS);
	
	prj_dir = project_dbase_get_dir(app->project_dbase);
	launch_cvs_command (command, prj_dir);
	
	g_free (command);
	g_free (cvsroot);
	g_free (compression);
}


/*
	Adds a file to the repository. If message == NULL it is ignored.
*/

void
cvs_add_file (CVS * cvs, gchar * filename, gchar * message)
{
	gchar *file;
	gchar *dir;
	gchar *compression;
	gchar *command;
	gchar *escaped_message;

	g_return_if_fail (cvs != NULL);

	file = extract_filename (filename);
	dir = extract_directory (filename);

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " add ", NULL);
	if (message != NULL && strlen (message) > 0) {
		escaped_message = anjuta_util_escape_quotes(message);
		command = g_strconcat (command, "-m \"", escaped_message, "\" ", NULL);
		g_free(escaped_message);
	}
	command = g_strconcat (command, file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS Adding "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	launch_cvs_command (command, dir);

	g_free (compression);
	g_free (dir);
	g_free (command);
}

/*
	Removes a file from CVS.
*/

void
cvs_remove_file (CVS * cvs, gchar * filename)
{
	gchar *file;
	gchar *dir;
	gchar *compression;
	gchar *command;

	g_return_if_fail (cvs != NULL);

	file = extract_filename (filename);
	dir = extract_directory (filename);

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " remove ", file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS Removing "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	launch_cvs_command (command, dir);

	g_free (compression);
	g_free (dir);
	g_free (command);
}

/* 
	Print the status of a file to the cvs tab of the message window.
	Verbose output (-v) is used
*/

void
cvs_status (CVS * cvs, gchar * filename, gboolean is_dir)
{
	gchar *command;
	gchar *compression;
	gchar *dir;
	gchar *file;

	g_return_if_fail (cvs != NULL);

	if (is_dir) {
		file = NULL;
		dir = g_strdup (filename);
	} else {
		file = extract_filename (filename);
		dir = extract_directory (filename);
	}

	compression = add_compression (cvs);
	command =
		g_strconcat ("cvs ", compression, " status -v ", NULL);
	if (file) 
		command = g_strconcat (command, file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("Getting CVS status "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	anjuta_message_manager_show (app->messages, MESSAGE_CVS);
	
	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL);
	diff_editor->force_hilite = TE_LEXER_NONE;
	diff_editor->used_by_cvs = TRUE;
	text_editor_set_hilite_type (diff_editor);

	chdir (dir);
	launcher_execute (command, on_cvs_buffer_in, on_cvs_stderr,
			  on_cvs_terminate);

	g_free (compression);
	g_free (command);
	g_free (dir);
}

void
cvs_log (CVS * cvs, gchar * filename, gboolean is_dir)
{
	gchar *command;
	gchar *compression;
	gchar *dir;
	gchar *file;

	g_return_if_fail (cvs != NULL);

	if (is_dir) {
		file = NULL;
		dir = g_strdup (filename);
	} else {
		file = extract_filename (filename);
		dir = extract_directory (filename);
	}

	compression = add_compression (cvs);
	command =
		g_strconcat ("cvs ", compression, " log ", NULL);
	if (file) 
		command = g_strconcat (command, file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("Getting CVS log "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	anjuta_message_manager_show (app->messages, MESSAGE_CVS);

	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL);
	diff_editor->force_hilite = TE_LEXER_NONE;
	diff_editor->used_by_cvs = TRUE;
	text_editor_set_hilite_type (diff_editor);

	chdir (dir);
	launcher_execute (command, on_cvs_buffer_in, on_cvs_stderr,
			  on_cvs_terminate);

	g_free (compression);
	g_free (command);
	g_free (dir);
}

/*
	Shows the differnces between working copy and cvs in a new text
	buffer. If unified = TRUE, the unified diff format is used.
	If revision == NULL or revision is empty it is ignored.
	If date == 0 it is ignored.
*/

#define DATE_LENGTH 17

void
cvs_diff (CVS * cvs, gchar * filename, gchar * revision,
	       time_t date, gboolean is_dir)
{
	gchar *file;
	gchar *dir;
	gchar *command;
	gchar *compression;

	g_return_if_fail (cvs != NULL);

	if (is_dir) {
		file = NULL;
		dir = g_strdup (filename);
	} else {
		file = extract_filename (filename);
		dir = extract_directory (filename);
	}

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " diff", NULL);
	if (cvs->unified_diff)
		command = g_strconcat (command, " -u ", NULL);
	else if (cvs->context_diff)
		command = g_strconcat (command, " -c ", NULL);
	if (revision != NULL && strlen (revision))
		command = g_strconcat (command, " -r ", revision, NULL);
	if (date > 0 && cvs->use_date_diff)
	{
		struct tm *tm_time = localtime (&date);
		gchar *time_str = g_new0 (char, DATE_LENGTH + 1);
		strftime (time_str, DATE_LENGTH, "%Y/%m/%d %k:%M", tm_time);
		command =
			g_strconcat (command, " -D \"", time_str, "\" ",
				     NULL);
		g_free (time_str);
	}
	if (file)
		command = g_strconcat (command, " ", file, NULL);
	
	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS diffing "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	anjuta_message_manager_show (app->messages, MESSAGE_CVS);

	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL);
	diff_editor->used_by_cvs = TRUE;
	diff_editor->force_hilite = TE_LEXER_DIFF;
	text_editor_set_hilite_type (diff_editor);

	chdir (dir);
	launcher_execute (command, on_cvs_buffer_in, on_cvs_stderr,
			  on_cvs_terminate);

	g_free (command);
	g_free (dir);
	g_free (compression);
}

/* 
	Log in on Password servers and other non-local servers 
*/

void
cvs_login (CVS * cvs, ServerType type, gchar* server, gchar* dir, 
		gchar* user)
{
	gchar *cvsroot;
	gchar *command;

	g_return_if_fail (cvs != NULL);

	cvsroot = get_full_cvsroot (cvs, type, server, dir, user);
	command = g_strconcat ("cvs -d ", cvsroot, " login", NULL);

	/* Insert login code here */
	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS login "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, cvsroot, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	launch_cvs_command (command, NULL);

	g_free (command);
	g_free (cvsroot);
}

/*
	Saves the cvs settings to properties
*/

gboolean
cvs_save_yourself (CVS * cvs, FILE * stream)
{
	if (!cvs)
		return FALSE;

	fprintf (stream, "cvs.update.force=%d\n", cvs->force_update);
	fprintf (stream, "cvs.diff.unified=%d\n", cvs->unified_diff);
	fprintf (stream, "cvs.diff.context=%d\n", cvs->context_diff);
	fprintf (stream, "cvs.diff.usedate=%d\n", cvs->use_date_diff);
	fprintf (stream, "cvs.compression=%d\n", cvs->compression);

	return TRUE;
}

/* PRIVATE: */

/*
	Puts messages that arrive from cvs to the message window.
*/

static void
on_cvs_stdout (gchar * line)
{
	anjuta_message_manager_append (app->messages, line, MESSAGE_CVS);
}

/*
	Puts error messages that arrive from cvs to the message window.
*/

static void
on_cvs_stderr (gchar * line)
{
	anjuta_message_manager_append (app->messages, line, MESSAGE_CVS);
}

/*
	Puts the diff produced by cvs in a new text buffer.
*/
static void
on_cvs_buffer_in (gchar * line)
{
	guint length;
	g_return_if_fail (line != NULL);
	g_return_if_fail (diff_editor != NULL);
	
	if (app->cvs->editor_destroyed)
	{
		on_cvs_stdout(line);
		return;
	}
	
	length = strlen (line);
	if (length)
	{
		scintilla_send_message (SCINTILLA
					(diff_editor->widgets.editor),
					SCI_ADDTEXT, length, (long) line);
	}
}

/*
	This is called when a cvs command finished. It prints a message
	whether the command was sucessful.
*/

static void
on_cvs_terminate (int status, time_t time)
{
	gchar *buff;
	if (status)
	{
		anjuta_message_manager_append (app->messages,
					       _
					       ("Project import completed...unsuccessful\n"),
					       MESSAGE_BUILD);
		anjuta_status (_("Project import completed...unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
					       _
					       ("CVS completed...successful\n"),
					       MESSAGE_CVS);
		anjuta_status (_("CVS completed...successful"));
	}
	buff = g_strdup_printf (_("Total time taken: %d secs\n"),
				(gint) time);
	anjuta_message_manager_append (app->messages, buff, MESSAGE_CVS);
	
	if (diff_editor != NULL)
	{
		if (app->cvs->editor_destroyed == FALSE)
		{
			diff_editor->used_by_cvs = FALSE;
		}
		app->cvs->editor_destroyed = FALSE;
		diff_editor = NULL;
	}
}

/* 
	Create the full CVSROOT string with servertype, username and dir.
	Returned string must be freed. NULL is returned of failsure.
*/

static gchar *
get_full_cvsroot (CVS * cvs, ServerType type, gchar* server, 
		gchar* dir, gchar* user)
{
	gchar *cvsroot;
	g_return_val_if_fail (cvs != NULL, NULL);
	cvsroot = g_strdup (server_type_identifiers[type]);
	switch (type)
	{
	case CVS_LOCAL:
	{
		cvsroot = g_strconcat (cvsroot, ":", dir, NULL);
		break;
	}
	case CVS_PASSWORD:
	case CVS_EXT:
	case CVS_SERVER:
	{
		cvsroot =
			g_strconcat (cvsroot, ":", user, "@",
				     server, ":", dir, NULL);
		break;
	}
	default:
		g_warning ("Unsupported Server type!");
	}
	// Be sure that cvsroot is passed corretly
	cvsroot = g_strconcat ("'", cvsroot, "'", NULL);

	return cvsroot;
}

/*
	Create a string like -z3 to pass the compression to cvs.
	Returned string must be freed. NULL is return on failsure.
*/

static gchar *
add_compression (CVS * cvs)
{
	gchar *string = NULL;

	g_return_val_if_fail (cvs != NULL, NULL);

	if (cvs->compression > 0)
		string = g_strdup_printf ("-z%d", cvs->compression);
	else
		string = g_strdup ("");

	return string;
}

/*
	Starts anjuta launcher in the given directory with the given
	command. Checks if launcher is busy before.
*/

static void
launch_cvs_command (gchar * command, gchar * dir)
{
	g_return_if_fail (command != NULL);
	/* g_return_if_fail (dir != NULL); */

	if (launcher_is_busy ())
	{
		anjuta_error (_
			      ("There are jobs running, please wait until they are finished"));
	}

	if (dir) chdir (dir);

	anjuta_message_manager_show (app->messages, MESSAGE_CVS);

	launcher_execute (command, on_cvs_stdout, on_cvs_stderr,
			  on_cvs_terminate);
	return;
}

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
	gchar *server;
	gchar *server_dir;
	ServerType server_type;
	guint compression;

	gchar *username;
	gchar *passwd;
};

/* Callbacks for launcher */
static void on_cvs_stdout (gchar * line);
static void on_cvs_stderr (gchar * line);
static void on_cvs_buffer_in (gchar * line);
static void on_cvs_terminate (int status, time_t time);

/* Utility functions */
static void launch_cvs_command (gchar * command, gchar * dir);
static gchar *get_full_cvsroot (CVS * cvs);
static gchar *add_compression (CVS * cvs);

/* Server types translation table */
static gchar *server_type_identifiers[CVS_END] =
	{ ":local", ":pserver", ":ext", ":server" };

/* Text editor for diff */
static TextEditor *diff_editor;

/* 
	Initialisize cvs module. Read values from properties. 
*/

CVS *
cvs_new (PropsID p)
{
	CVS *cvs = g_new0 (CVS, 1);
	cvs->server_dir = prop_get (p, "cvs.server.dir");

	cvs->server = prop_get (p, "cvs.server");
	cvs->server_type = prop_get_int (p, "cvs.server.type", CVS_LOCAL);

	cvs->username = prop_get (p, "cvs.server.username");
	cvs->passwd = prop_get (p, "cvs.server.passwd");

	cvs->compression = prop_get_int (p, "cvs.compression", 0);

	if (cvs->server == NULL)
		cvs->server = g_strdup ("");
	if (cvs->server_dir == NULL)
		cvs->server_dir = g_strdup ("");
	if (cvs->username == NULL)
		cvs->username = g_strdup ("");
	if (cvs->passwd == NULL)
		cvs->passwd = g_strdup ("");

	return cvs;
}


/*
	Following functions allow to set adjust values in the
	cvs structure.
*/

void
cvs_set_server (CVS * cvs, gchar * server)
{
	g_return_if_fail (cvs != NULL);
	g_return_if_fail (server != NULL);
	g_free (cvs->server);
	cvs->server = g_strdup (server);
}

void
cvs_set_server_type (CVS * cvs, ServerType type)
{
	g_return_if_fail (cvs != NULL);
	cvs->server_type = type;
}

void
cvs_set_directory (CVS * cvs, gchar * directory)
{
	g_return_if_fail (cvs != NULL);
	g_return_if_fail (directory != NULL);
	g_free (cvs->server_dir);
	cvs->server_dir = g_strdup (directory);
}

void
cvs_set_username (CVS * cvs, gchar * username)
{
	g_return_if_fail (username != NULL);
	g_free (cvs->username);
	cvs->username = g_strdup (username);
}

/*
	Here the password can be set by the user but it is not required.
	If the password is needed an none is set, then the user will be asked.
	If the password is stored in the preferences it is stored as clean text
	so be careful if it should be "top-secret".
*/

void
cvs_set_passwd (CVS * cvs, gchar * passwd)
{
	g_return_if_fail (passwd != NULL);
	g_free (cvs->passwd);
	cvs->passwd = g_strdup (passwd);
}

void
cvs_set_compression (CVS * cvs, guint compression)
{
	cvs->compression = compression;
}


/*
	Following functions allow to access the cvs module.
	If gchar* are returned they need to be g_freed
*/

gchar *
cvs_get_server (CVS * cvs)
{
	return g_strdup (cvs->server);
}

ServerType
cvs_get_server_type (CVS * cvs)
{
	return cvs->server_type;
}

gchar *
cvs_get_directory (CVS * cvs)
{
	return g_strdup (cvs->server_dir);
}

gchar *
cvs_get_username (CVS * cvs)
{
	return g_strdup (cvs->username);
}

gchar *
cvs_get_passwd (CVS * cvs)
{
	return g_strdup (cvs->passwd);
}

guint
cvs_get_compression (CVS * cvs)
{
	return cvs->compression;
}


/* 
	Free the memory of the cvs module 
*/

void
cvs_destroy (CVS * cvs)
{
	g_return_if_fail (cvs != NULL);
	g_free (cvs->server);
	g_free (cvs->server_dir);

	g_free (cvs->username);
	g_free (cvs->passwd);

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
	Commits the changes in the working copy to the repositry.
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

	command = g_strconcat (command, "-m \"", message, "\" ",NULL);
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
	Adds a file to the repositry. If message == NULL it is ignored.
*/

void
cvs_add_file (CVS * cvs, gchar * filename, gchar * message)
{
	gchar *file;
	gchar *dir;
	gchar *compression;
	gchar *command;

	g_return_if_fail (cvs != NULL);

	file = extract_filename (filename);
	dir = extract_directory (filename);

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " add ", NULL);
	if (message != NULL && strlen (message) > 0)
		command =
			g_strconcat (command, "-m \"", message, "\" ", NULL);
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
	
	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL);
	diff_editor->force_hilite = TE_LEXER_NONE;
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
	
	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL);
	diff_editor->force_hilite = TE_LEXER_NONE;
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
	       time_t date, gboolean unified, gboolean is_dir)
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
	command = g_strconcat ("cvs ", compression, "diff", NULL);
	if (unified)
		command = g_strconcat (command, " -u ", NULL);
	if (revision != NULL && strlen (revision))
		command = g_strconcat (command, " -r ", revision, NULL);
	if (date > 0)
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
		command = g_strconcat (command, file, NULL);

	anjuta_message_manager_clear (app->messages, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, _("CVS diffing "), MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, filename, MESSAGE_CVS);
	anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	anjuta_message_manager_show (app->messages, MESSAGE_CVS);

	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL);
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
cvs_login (CVS * cvs)
{
	gchar *cvsroot;
	gchar *command;

	g_return_if_fail (cvs != NULL);

	if (cvs->server_type != CVS_PASSWORD && cvs->server_type != CVS_EXT)
		return;

	cvsroot = get_full_cvsroot (cvs);
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

	fprintf (stream, "cvs.server=%s\n", cvs->server);
	fprintf (stream, "cvs.server.dir=%s\n", cvs->server_dir);
	fprintf (stream, "cvs.server.type=%d\n", cvs->server_type);
	fprintf (stream, "cvs.server.username=%s\n", cvs->username);
	fprintf (stream, "cvs.server.passwd=%s\n", cvs->passwd);

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
	if (WEXITSTATUS (status))
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
}

/* 
	Create the full CVSROOT string with servertype, username and dir.
	Returned string must be freed. NULL is returned of failsure.
*/

static gchar *
get_full_cvsroot (CVS * cvs)
{
	char *cvsroot;
	g_return_val_if_fail (cvs != NULL, NULL);
	cvsroot = g_strdup (server_type_identifiers[cvs->server_type]);
	switch (cvs->server_type)
	{
	case CVS_LOCAL:
	{
		cvsroot = g_strconcat (cvsroot, cvs->server_dir, NULL);
		break;
	}
	case CVS_PASSWORD:
	case CVS_EXT:
	case CVS_SERVER:
	{
		cvsroot =
			g_strconcat (cvsroot, ":", cvs->username, "@",
				     cvs->server, ":", cvs->server_dir, NULL);
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

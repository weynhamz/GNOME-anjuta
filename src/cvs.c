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
#include "an_file_view.h"

/* struct is private, use functions to access members */
struct _CVS
{
	gboolean update_directories;
	gboolean reset_sticky_tags;
	gboolean unified_diff;
	gboolean context_diff;
	gboolean use_date_diff;
	
	guint compression;
	
	gboolean editor_destroyed;
};

/* Callbacks for launcher */
static void on_cvs_buffer_output_arrived (AnjutaLauncher *launcher,
										  AnjutaLauncherOutputType output_type,
										  const gchar * line, gpointer data);
static void on_cvs_output_arrived (AnjutaLauncher *launcher,
								   AnjutaLauncherOutputType output_type,
								   const gchar * line, gpointer data);
static void on_cvs_buffer_terminated (AnjutaLauncher *launcher, gint child_pid,
									  gint status, gulong time_taken,
									  gpointer data);
static void on_cvs_terminated (AnjutaLauncher *launcher, gint child_pid,
							   gint status, gulong time_taken, gpointer data);
static void on_cvs_terminated_real (gint status, gulong time);

/* Utility functions */
static void launch_cvs_command (gchar * command, gchar * dir);
static gchar *get_full_cvsroot (CVS *cvs, ServerType type, 
                                const gchar *server, const gchar *dir,
                                const gchar *user);
static gchar *add_compression (CVS * cvs);

/* Server types translation table */
static gchar *server_type_identifiers[CVS_END] =
	{ ":local", ":pserver", ":ext", ":server" };

/* Text editor for diff */
static TextEditor *diff_editor = NULL;

/* Update FileView ? */ 

static gboolean update_fileview = FALSE;
	
/* 
	Initialisize cvs module. Read values from properties. 
*/

CVS *
cvs_new (PropsID p)
{
	CVS *cvs = g_new0 (CVS, 1);
	
	cvs_apply_preferences(cvs, p);
	
	cvs->editor_destroyed = FALSE;
	
	return cvs;
}

void
cvs_apply_preferences(CVS *cvs, PropsID p)
{
	g_return_if_fail (cvs != NULL);
	cvs->update_directories = prop_get_int (p, "cvs.update.directories", 1);
	cvs->reset_sticky_tags = prop_get_int (p, "cvs.update.reset", 0);
	cvs->unified_diff = prop_get_int (p, "cvs.diff.unified", 1);
	cvs->context_diff = prop_get_int (p, "cvs.diff.context", 0);
	cvs->use_date_diff = prop_get_int (p, "cvs.diff.usedate", 0);
	cvs->compression = prop_get_int (p, "cvs.compression", 3);
}

void cvs_set_editor_destroyed (CVS* cvs)
{
	g_return_if_fail (cvs != NULL);
	cvs->editor_destroyed = TRUE;
	anjuta_launcher_reset(app->launcher);
}


/*
	Following functions allow to set adjust values in the
	cvs structure.
*/

void
cvs_set_update_directories (CVS * cvs, gboolean update_directories)
{
	g_return_if_fail (cvs != NULL);
	cvs->update_directories = update_directories;
}

void
cvs_set_update_reset_sticky_tags (CVS * cvs, gboolean reset_sticky_tags)
{
	g_return_if_fail (cvs != NULL);
	cvs->reset_sticky_tags = reset_sticky_tags;
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
cvs_get_update_directories(CVS * cvs)
{
	g_return_val_if_fail (cvs != NULL, 0);
	return cvs->update_directories;
}

gboolean
cvs_get_update_reset_sticky_tags(CVS * cvs)
{
	g_return_val_if_fail (cvs != NULL, 0);
	return cvs->reset_sticky_tags;
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
	g_return_val_if_fail (cvs != NULL, FALSE);
	return cvs->use_date_diff;
}

/* 
	Free the memory of the cvs module 
*/
void
cvs_destroy (CVS *cvs)
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
cvs_update (CVS *cvs, const gchar *filename,
            const gchar *branch, gboolean is_dir)
{
	gchar *command = NULL;
	gchar *compression;
	const gchar *file;
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
	if (cvs->update_directories)
		command = g_strconcat (command, " -dP ", NULL);
	if (cvs->reset_sticky_tags)
		command = g_strconcat (command, " -A ", NULL);
	
	if (branch != NULL && strlen (branch) > 0)
		command = g_strconcat (command, "-j ", branch, " ", NULL);
	if (file) 
		command = g_strconcat (command, file, NULL);

	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS Updating "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	
	update_fileview = TRUE;
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
cvs_commit (CVS *cvs, const gchar *filename, const gchar *revision,
            const gchar *message, gboolean is_dir)
{
	gchar *command;
	gchar *compression;
	const gchar *file;
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

	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS Committing "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	update_fileview = TRUE;
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

void cvs_import_project (CVS *cvs, ServerType type, const gchar *server,
			const gchar *dir, const gchar *user, const gchar *module,
            const gchar *rel, const gchar *ven, const gchar *message)
{
	gchar* command;
	gchar* compression;
	gchar* cvsroot;
	gchar* prj_dir;
	gchar* escaped_message;
	gchar* vendor;
	gchar* release;
	
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
	
	vendor = g_strdup (ven);
	release = g_strdup (rel);
	
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
	
	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS Importing...\n"), MESSAGE_CVS);
	
	prj_dir = project_dbase_get_dir(app->project_dbase);
	launch_cvs_command (command, prj_dir);
	
	g_free (command);
	g_free (cvsroot);
	g_free (compression);
	g_free (vendor);
	g_free (release);
}

/*
	Adds a file to the repository. If message == NULL it is ignored.
*/

void
cvs_add_file (CVS *cvs, const gchar *filename, const gchar *message)
{
	const gchar *file;
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

	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS Adding "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	launch_cvs_command (command, dir);

	g_free (compression);
	g_free (dir);
	g_free (command);
}

/*
	Removes a file from CVS.
*/

void
cvs_remove_file (CVS *cvs, const gchar *filename)
{
	const gchar *file;
	gchar *dir;
	gchar *compression;
	gchar *command;

	g_return_if_fail (cvs != NULL);

	file = extract_filename (filename);
	dir = extract_directory (filename);

	compression = add_compression (cvs);
	command = g_strconcat ("cvs ", compression, " remove -f ", file, NULL);

	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS Removing "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

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
cvs_status (CVS *cvs, const gchar *filename, gboolean is_dir)
{
	gchar *command;
	gchar *compression;
	gchar *dir;
	const gchar *file;

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

	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("Getting CVS status "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	an_message_manager_show (app->messages, MESSAGE_CVS);
	
	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL, NULL);
	diff_editor->force_hilite = TE_LEXER_DIFF;
	diff_editor->used_by_cvs = TRUE;
	text_editor_set_hilite_type (diff_editor);

	chdir (dir);
	if (anjuta_launcher_is_busy (app->launcher))
	{
		anjuta_error (_
			      ("There are jobs running, please wait until they are finished"));
	}
	else
	{
		g_signal_connect (G_OBJECT (app->launcher), "child-exited",
						  G_CALLBACK (on_cvs_buffer_terminated), NULL);
		anjuta_launcher_execute (app->launcher, command,
								 on_cvs_buffer_output_arrived, NULL);
	}

	g_free (compression);
	g_free (command);
	g_free (dir);
}

void
cvs_log (CVS *cvs, const gchar *filename, gboolean is_dir)
{
	gchar *command;
	gchar *compression;
	gchar *dir;
	const gchar *file;

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

	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("Getting CVS log "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	an_message_manager_show (app->messages, MESSAGE_CVS);

	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL, NULL);
	diff_editor->force_hilite = TE_LEXER_DIFF;
	diff_editor->used_by_cvs = TRUE;
	text_editor_set_hilite_type (diff_editor);

	chdir (dir);
	if (anjuta_launcher_is_busy (app->launcher))
	{
		anjuta_error (_
			      ("There are jobs running, please wait until they are finished"));
	}
	else
	{
		g_signal_connect (G_OBJECT (app->launcher), "child-exited",
						  G_CALLBACK (on_cvs_buffer_terminated), NULL);
		anjuta_launcher_execute (app->launcher, command,
								 on_cvs_buffer_output_arrived, NULL);
	}
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
cvs_diff (CVS *cvs, const gchar *filename, const gchar *revision,
          time_t date, gboolean is_dir)
{
	const gchar *file;
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
	
	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS diffing "), MESSAGE_CVS);
	an_message_manager_append (app->messages, filename, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);
	an_message_manager_show (app->messages, MESSAGE_CVS);

	// Create Text Editor for diff
	diff_editor = anjuta_append_text_editor(NULL, NULL);
	diff_editor->used_by_cvs = TRUE;
	diff_editor->force_hilite = TE_LEXER_DIFF;
	text_editor_set_hilite_type (diff_editor);

	chdir (dir);
	if (anjuta_launcher_is_busy (app->launcher))
	{
		anjuta_error (_
			      ("There are jobs running, please wait until they are finished"));
	}
	else
	{
		g_signal_connect (G_OBJECT (app->launcher), "child-exited",
						  G_CALLBACK (on_cvs_buffer_terminated), NULL);
		anjuta_launcher_execute (app->launcher, command,
								 on_cvs_buffer_output_arrived, NULL);
	}
	g_free (command);
	g_free (dir);
	g_free (compression);
}

/* 
	Log in on Password servers and other non-local servers 
*/

void
cvs_login (CVS *cvs, ServerType type, const gchar *server,
           const gchar *dir, const gchar *user)
{
	gchar *cvsroot;
	gchar *command;

	g_return_if_fail (cvs != NULL);

	cvsroot = get_full_cvsroot (cvs, type, server, dir, user);
	command = g_strconcat ("cvs -d ", cvsroot, " login", NULL);

	/* Insert login code here */
	an_message_manager_clear (app->messages, MESSAGE_CVS);
	an_message_manager_append (app->messages, _("CVS login "), MESSAGE_CVS);
	an_message_manager_append (app->messages, cvsroot, MESSAGE_CVS);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_CVS);

	launch_cvs_command (command, NULL);

	g_free (command);
	g_free (cvsroot);
}

gboolean is_cvs_active_for_dir(const gchar *dir)
{
	struct stat s;
	char entries[PATH_MAX];
	g_return_val_if_fail(dir, FALSE);
	g_snprintf(entries, PATH_MAX, "%s/CVS/Entries", dir);
	if (0 == stat(entries, &s))
	{
		if ((0 < s.st_size) && S_ISREG(s.st_mode))
			return TRUE;
	}
	return FALSE;
}

/*
	Saves the cvs settings to properties
*/

gboolean
cvs_save_yourself (CVS * cvs, FILE * stream)
{
	if (!cvs)
		return FALSE;

	fprintf (stream, "cvs.update.directories=%d\n", cvs->update_directories);
	fprintf (stream, "cvs.update.reset=%d\n", cvs->reset_sticky_tags);
	fprintf (stream, "cvs.diff.unified=%d\n", cvs->unified_diff);
	fprintf (stream, "cvs.diff.context=%d\n", cvs->context_diff);
	fprintf (stream, "cvs.diff.usedate=%d\n", cvs->use_date_diff);
	fprintf (stream, "cvs.compression=%d\n", cvs->compression);

	return TRUE;
}

/* PRIVATE: */
/* Puts messages that arrive from cvs to the message window. */
static void
on_cvs_output_arrived (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * line, gpointer data)
{
	an_message_manager_append (app->messages, line, MESSAGE_CVS);
}

/*	Puts the diff produced by cvs in a new text buffer. */
static void
on_cvs_buffer_output_arrived (AnjutaLauncher *launcher,
							  AnjutaLauncherOutputType output_type,
							  const gchar * line, gpointer data)
{
	guint length;
	switch (output_type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
		g_return_if_fail (line != NULL);
		g_return_if_fail (diff_editor != NULL);
		
		if (app->cvs->editor_destroyed)
		{
			on_cvs_output_arrived (launcher, output_type, line, data);
			return;
		}
		
		length = strlen (line);
		if (length)
		{
			scintilla_send_message (SCINTILLA
						(diff_editor->widgets.editor),
						SCI_APPENDTEXT, length, (long) line);
		}
		break;
	default:
		an_message_manager_append (app->messages, line, MESSAGE_CVS);
	}	
}

/*
	This is called when a cvs command finished. It prints a message
	whether the command was sucessful.
*/

static void
on_cvs_terminated_real (gint status, gulong time_taken)
{
	gchar *buff;
	if (status)
	{
		an_message_manager_append (app->messages,
					       _("Project import completed...unsuccessful\n"),
					       MESSAGE_BUILD);
		anjuta_status (_("Project import completed...unsuccessful"));
	}
	else
	{
		an_message_manager_append (app->messages,
					       _("CVS completed...successful\n"),
					       MESSAGE_CVS);
		anjuta_status (_("CVS completed...successful"));
	}
	buff = g_strdup_printf (_("Total time taken: %lu secs\n"), time_taken);
	an_message_manager_append (app->messages, buff, MESSAGE_CVS);
	
	if (update_fileview && app->project_dbase->project_is_open)
	{
		an_message_manager_append (app->messages,
						_("Updating versions in file tree..."), MESSAGE_CVS);
		fv_populate(TRUE);
		an_message_manager_append (app->messages, _("done\n"), MESSAGE_CVS);
	}
	update_fileview = FALSE;
		
	
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

static void
on_cvs_terminated (AnjutaLauncher *launcher, gint child_pid,
				   gint status, gulong time_taken, gpointer data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (app->launcher),
										  G_CALLBACK (on_cvs_terminated),
										  data);
	on_cvs_terminated_real (status, time_taken);
}

static void
on_cvs_buffer_terminated (AnjutaLauncher *launcher, gint child_pid,
				   gint status, gulong time_taken, gpointer data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (app->launcher),
										  G_CALLBACK (on_cvs_buffer_terminated),
										  data);
	on_cvs_terminated_real (status, time_taken);
}

/* 
	Create the full CVSROOT string with servertype, username and dir.
	Returned string must be freed. NULL is returned of failsure.
*/

static gchar *
get_full_cvsroot (CVS * cvs, ServerType type, const gchar *server, 
                  const gchar *dir, const gchar *user)
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

	if (anjuta_launcher_is_busy (app->launcher))
	{
		anjuta_error (_
			      ("There are jobs running, please wait until they are finished"));
	}

	if (dir) chdir (dir);

	an_message_manager_show (app->messages, MESSAGE_CVS);

	g_signal_connect (G_OBJECT (app->launcher), "child-exited",
					  G_CALLBACK (on_cvs_terminated), NULL);
	anjuta_launcher_execute (app->launcher, command,
							 on_cvs_output_arrived, NULL);
	return;
}

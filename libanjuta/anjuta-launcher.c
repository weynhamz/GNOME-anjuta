/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-launcher.c
 * Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * SECTION:anjuta-launcher
 * @short_description: External process launcher with async input/output
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-launcher.h
 * 
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#if defined(__FreeBSD__)
#  include <libutil.h>
#elif defined(__OpenBSD__) || defined(__NetBSD__)
#  include <util.h>
#elif !defined(__sun)
#  include <pty.h>
#endif

#include "anjuta-utils-priv.h"

#include <assert.h>
#include <termios.h>

#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include <glib.h>

#include "anjuta-utils.h"
#include "anjuta-marshal.h"
#include "resources.h"
#include "anjuta-launcher.h"
#include "anjuta-debug.h"

#define ANJUTA_PIXMAP_PASSWORD "password.png"
#define FILE_BUFFER_SIZE 1024
#define FILE_INPUT_BUFFER_SIZE		1048576
#ifndef __MAX_BAUD
#  if defined(B460800)
#    define __MAX_BAUD B460800
#  elif defined(B307200)
#    define __MAX_BAUD B307200
#  elif defined(B256000)
#    define __MAX_BAUD B256000
#  else
#    define __MAX_BAUD B230400
#  endif
#endif

/*
static gboolean
anjuta_launcher_pty_check_child_exit_code (AnjutaLauncher *launcher,
										   const gchar* line);
*/
struct _AnjutaLauncherPriv
{
	/*
	*  Busy flag is TRUE if the Launcher
	*  is currently executing a child.
	*/
	gboolean busy;
	
	/* These flags are used to synchronize the IO operations. */
	gboolean stdout_is_done;
	gboolean stderr_is_done;
	
	/* GIO channels */
	GIOChannel *stdout_channel;
	GIOChannel *stderr_channel;
	/*GIOChannel *stdin_channel;*/
	GIOChannel *pty_channel;

	/* GIO watch handles */
	guint stdout_watch;
	guint stderr_watch;
	guint pty_watch;
	
	/* Output line buffers */
	gchar *stdout_buffer;
	gchar *stderr_buffer;
	
	/* Output of the pty is constantly stored here.*/
	gchar *pty_output_buffer;

	/* Terminal echo */
	gboolean terminal_echo_on;
	
	/* The child */
	pid_t child_pid;
	guint source;
	gint child_status;
	gboolean child_has_terminated;
	
	/* Synchronization in progress */
	guint completion_check_timeout;

	/* Terminate child on child exit */
	gboolean terminate_on_exit;

	/* Start time of execution */
	time_t start_time;
	
	/* Should the outputs be buffered */
	gboolean buffered_output;
	
	/* Should we check for password prompts in stdout and pty */
	gboolean check_for_passwd_prompt;
	
	/* Output callback */
	AnjutaLauncherOutputCallback output_callback;
	
	/* Callback data */
	gpointer callback_data;
	
	/* Encondig */
	gboolean custom_encoding;
	gchar* encoding;
	
	/* Env */
	GHashTable* env;
};

enum
{
	/* OUTPUT_ARRIVED_SIGNAL, */
	CHILD_EXITED_SIGNAL,
	BUSY_SIGNAL,
	LAST_SIGNAL
};

static void anjuta_launcher_class_init (AnjutaLauncherClass * klass);
static void anjuta_launcher_init (AnjutaLauncher * obj);
static gboolean anjuta_launcher_call_execution_done (gpointer data);
static gboolean anjuta_launcher_check_for_execution_done (gpointer data);
static void anjuta_launcher_execution_done_cleanup (AnjutaLauncher *launcher,
													gboolean emit_signal);

static gboolean is_password_prompt(const gchar* line);

static guint launcher_signals[LAST_SIGNAL] = { 0 };
static AnjutaLauncherClass *parent_class;

static void
anjuta_launcher_initialize (AnjutaLauncher *obj)
{
	/* Busy flag */
	obj->priv->busy = FALSE;
	
	/* These flags are used to synchronize the IO operations. */
	obj->priv->stdout_is_done = FALSE;
	obj->priv->stderr_is_done = FALSE;
	
	/* GIO channels */
	obj->priv->stdout_channel = NULL;
	obj->priv->stderr_channel = NULL;
	obj->priv->pty_channel = NULL;
	
	/* Output line buffers */
	obj->priv->stdout_buffer = NULL;
	obj->priv->stderr_buffer = NULL;
	
	/* Pty buffer */
	obj->priv->pty_output_buffer = NULL;
	
	obj->priv->terminal_echo_on = TRUE;
	
	/* The child */
	obj->priv->child_pid = 0;
	obj->priv->child_status = -1;
	obj->priv->child_has_terminated = TRUE;
	obj->priv->source = 0;
	
	/* Synchronization in progress */
	obj->priv->completion_check_timeout = 0;
	
	/* Terminate child on child exit */
	obj->priv->terminate_on_exit = FALSE;

	/* Start time of execution */
	obj->priv->start_time = 0;
	
	obj->priv->buffered_output = TRUE;
	obj->priv->check_for_passwd_prompt = TRUE;
	
	/* Output callback */
	obj->priv->output_callback = NULL;
	obj->priv->callback_data = NULL;
	
	/* Encoding */
	obj->priv->custom_encoding = FALSE;
	obj->priv->encoding = NULL;
	
	/* Env */
	obj->priv->env = g_hash_table_new_full (g_str_hash, g_str_equal,
											g_free, g_free);
}

GType
anjuta_launcher_get_type ()
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (AnjutaLauncherClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) anjuta_launcher_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (AnjutaLauncher),
			0,              /* n_preallocs */
			(GInstanceInitFunc) anjuta_launcher_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "AnjutaLauncher", &obj_info, 0);
	}
	return obj_type;
}

static void
anjuta_launcher_dispose (GObject *obj)
{
	AnjutaLauncher *launcher = ANJUTA_LAUNCHER (obj);
	if (anjuta_launcher_is_busy (launcher))
	{
		g_source_remove (launcher->priv->source);
		launcher->priv->source = 0;
		
		anjuta_launcher_execution_done_cleanup (launcher, FALSE);
		
		launcher->priv->busy = FALSE;
		
	}
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
anjuta_launcher_finalize (GObject *obj)
{
	AnjutaLauncher *launcher = ANJUTA_LAUNCHER (obj);	
	if (launcher->priv->custom_encoding && launcher->priv->encoding)
		g_free (launcher->priv->encoding);
	
	g_hash_table_destroy (launcher->priv->env);
	
	g_free (launcher->priv);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
anjuta_launcher_class_init (AnjutaLauncherClass * klass)
{
	GObjectClass *object_class;
	g_return_if_fail (klass != NULL);
	object_class = (GObjectClass *) klass;
	
	/* DEBUG_PRINT ("%s", "Initializing launcher class"); */
	
	parent_class = g_type_class_peek_parent (klass);

	/**
	 * AnjutaLauncher::child-exited
 	 * @launcher: a #AnjutaLancher object.
	 * @child_pid: process ID of the child
	 * @status: status as returned by waitpid function
	 * @time: time in seconds taken by the child
	 * 
	 * Emitted when the child has exited and all i/o channels have
	 * been closed. If the terminate on exit flag is set, the i/o
	 * channels are automatically closed when the child exit.
	 * You need to use WEXITSTATUS and friend to get the child exit
	 * code from the status returned.
	 **/
	launcher_signals[CHILD_EXITED_SIGNAL] =
		g_signal_new ("child-exited",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaLauncherClass,
									 child_exited),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__INT_INT_ULONG,
					G_TYPE_NONE, 3, G_TYPE_INT,
					G_TYPE_INT, G_TYPE_ULONG);
	
	/**
	 * AnjutaLauncher::busy
 	 * @launcher: a #AnjutaLancher object.
	 * @busy: TRUE is a child is currently running
	 * 
	 * Emitted when a child starts after a call to one execute function
	 * (busy is TRUE) or when a child exits and all i/o channels are
	 * closed (busy is FALSE).
	 **/
	launcher_signals[BUSY_SIGNAL] =
		g_signal_new ("busy",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaLauncherClass,
									 busy),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__BOOLEAN,
					G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	
	object_class->dispose = anjuta_launcher_dispose;
	object_class->finalize = anjuta_launcher_finalize;
}

static void
anjuta_launcher_init (AnjutaLauncher * obj)
{
	g_return_if_fail (obj != NULL);
	obj->priv = g_new0 (AnjutaLauncherPriv, 1);
	anjuta_launcher_initialize (obj);
}

/**
 * anjuta_launcher_is_busy:
 * @launcher: a #AnjutaLancher object.
 *
 * Tells if the laucher is currently executing any command.
 *
 * Return value: TRUE if launcher is busy, otherwisee FALSE.
 */
gboolean
anjuta_launcher_is_busy (AnjutaLauncher *launcher)
{
	return launcher->priv->busy;
}

static void
anjuta_launcher_set_busy (AnjutaLauncher *launcher, gboolean flag)
{
	gboolean old_busy = launcher->priv->busy;
	launcher->priv->busy = flag;
	if (old_busy != flag)
		g_signal_emit_by_name (G_OBJECT (launcher), "busy", flag);
}

/**
 * anjuta_launcher_send_stdin:
 * @launcher: a #AnjutaLancher object.
 * @input_str: The string to send to STDIN of the process.
 * 
 * Sends a string to Standard input of the process currently being executed.
 */
void
anjuta_launcher_send_stdin (AnjutaLauncher *launcher, const gchar * input_str)
{
	g_return_if_fail (launcher);
	g_return_if_fail (input_str);

	anjuta_launcher_send_ptyin (launcher, input_str);
}

/**
 * anjuta_launcher_send_stdin:
 * @launcher: a #AnjutaLancher object.
 * 
 * Sends a EOF to Standard input of the process currently being executed.
 */

void 
anjuta_launcher_send_stdin_eof (AnjutaLauncher *launcher)
{
	GError* err = NULL;
	g_io_channel_shutdown (launcher->priv->pty_channel, TRUE,
						   &err);
	g_io_channel_unref (launcher->priv->pty_channel);
	launcher->priv->pty_channel = NULL;
	
	if (err)
	{
		g_warning ("g_io_channel_shutdown () failed: %s", err->message);
	}
}

/**
 * anjuta_launcher_send_ptyin:
 * @launcher: a #AnjutaLancher object.
 * @input_str: The string to send to PTY of the process.
 * 
 * Sends a string to TTY input of the process currently being executed.
 * Mostly useful for entering passwords and other inputs which are directly
 * read from TTY input of the process.
 */
void
anjuta_launcher_send_ptyin (AnjutaLauncher *launcher, const gchar * input_str)
{
	gsize bytes_written;
	GError *err = NULL;
	
	g_return_if_fail (launcher);
	g_return_if_fail (input_str);
	
	if (strlen (input_str) == 0) 
		return;

	do
	{	
		g_io_channel_write_chars (launcher->priv->pty_channel,
							  input_str, strlen (input_str),
							  &bytes_written, &err);
		g_io_channel_flush (launcher->priv->pty_channel, NULL);
		if (err)
		{
			g_warning ("Error encountered while writing to PTY!. %s",
					   err->message);
			g_error_free (err);

			return;
		}
		input_str += bytes_written;
	}
	while (*input_str);
}

/**
 * anjuta_launcher_reset:
 * @launcher: a #AnjutaLancher object.
 * 
 * Resets the launcher and kills (SIGTERM) current process, if it is still
 * executing.
 */
void
anjuta_launcher_reset (AnjutaLauncher *launcher)
{
	if (anjuta_launcher_is_busy (launcher))
		kill (launcher->priv->child_pid, SIGTERM);
}

/**
 * anjuta_launcher_signal:
 * @launcher: a #AnjutaLancher object.
 * @sig: kernel signal ID (e.g. SIGTERM).
 *
 * Sends a kernel signal to the process that is being executed.
 */
void
anjuta_launcher_signal (AnjutaLauncher *launcher, int sig)
{
	kill (launcher->priv->child_pid, sig);
}

/**
 * anjuta_launcher_get_child_pid:
 * @launcher: a #AnjutaLancher object.
 * 
 * Gets the Process ID of the child being executed.
 *
 * Return value: Process ID of the child.
 */
pid_t
anjuta_launcher_get_child_pid (AnjutaLauncher *launcher)
{
  if (anjuta_launcher_is_busy (launcher))
    return launcher->priv->child_pid;
  else
    return -1;
}

static void
anjuta_launcher_synchronize (AnjutaLauncher *launcher)
{
	if (launcher->priv->child_has_terminated &&
		launcher->priv->stdout_is_done &&
		launcher->priv->stderr_is_done)
	{
		if (launcher->priv->completion_check_timeout != 0)
			g_source_remove (launcher->priv->completion_check_timeout);
		launcher->priv->completion_check_timeout = 
			/* Use a low priority timer to make sure all pending I/O are flushed out */
		    g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 50, anjuta_launcher_check_for_execution_done,
							 launcher, NULL);
	}
	
	/* This case is not very good, but it blocks the whole IDE
	because we never new if the child has finished */
	else if (launcher->priv->stdout_is_done &&
			 launcher->priv->stderr_is_done)
	{
		/* DEBUG_PRINT ("%s", "Child has't exited yet waiting for 200ms"); */
		if (launcher->priv->completion_check_timeout != 0)
			g_source_remove (launcher->priv->completion_check_timeout);
		launcher->priv->completion_check_timeout = 
			/* Use a low priority timer to make sure all pending I/O are flushed out */
		    g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 200, anjuta_launcher_check_for_execution_done,
							launcher, NULL);
	}
	/* Add this case for gdb. It creates child inheriting gdb
	 * pipes which are not closed if gdb crashes */
	else if (launcher->priv->child_has_terminated &&
			launcher->priv->terminate_on_exit)
	{
		if (launcher->priv->completion_check_timeout != 0)
			g_source_remove (launcher->priv->completion_check_timeout);
		launcher->priv->completion_check_timeout = 
			/* Use a low priority timer to make sure all pending I/O are flushed out */
			g_idle_add ( anjuta_launcher_call_execution_done,
						 launcher);
	}
}

/* Password dialog */
static GtkWidget*
create_password_dialog (const gchar* prompt)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *box;
	GtkWidget *icon;
	GtkWidget *label;
	GtkWidget *entry;
	
	g_return_val_if_fail (prompt, NULL);
	
	dialog = gtk_dialog_new_with_buttons (prompt,
	                        NULL, //FIXME: Pass the parent window here
							// for transient purpose.
	                        GTK_DIALOG_DESTROY_WITH_PARENT,
	                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                        GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_wmclass (GTK_WINDOW (dialog), "launcher-password-prompt",
							"anjuta");
	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), hbox);
	
	icon = anjuta_res_get_image (ANJUTA_PIXMAP_PASSWORD);
	gtk_widget_show (icon);
	gtk_box_pack_start (GTK_BOX(hbox), icon, TRUE, TRUE, 0);
	
	if (strlen (prompt) < 20) {
		box = gtk_hbox_new (FALSE, 5);
	} else {
		box = gtk_vbox_new (FALSE, 5);
	}
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, TRUE, 0);
	
	label = gtk_label_new (_(prompt));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	
	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
	
	g_object_ref (entry);
	g_object_set_data_full (G_OBJECT (dialog), "password_entry",
							  g_object_ref (entry),
							  g_object_unref);
	gtk_widget_grab_focus (entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	
	return dialog;
}

/* pty buffer check for password authentication */
static void
anjuta_launcher_check_password_real (AnjutaLauncher *launcher,
									 const gchar* last_line)
{	
	if (anjuta_launcher_is_busy (launcher) == FALSE) 
		return;
	
	if (last_line) {

		/* DEBUG_PRINT ("(In password) Last line = %s", last_line); */
		if (is_password_prompt(last_line)) {
			/* Password prompt detected */
			GtkWidget* dialog;
			gint button;
			const gchar* passwd;
			gchar* line;
			
			dialog = create_password_dialog (last_line);
			button = gtk_dialog_run (GTK_DIALOG(dialog));
			switch (button) {
				case GTK_RESPONSE_OK:
					passwd = gtk_entry_get_text (
						GTK_ENTRY (g_object_get_data (G_OBJECT (dialog),
									"password_entry")));
					line = g_strconcat (passwd, "\n", NULL);
					anjuta_launcher_send_ptyin (launcher, line);
					g_free (line);
					break;
				case GTK_RESPONSE_CANCEL:
					anjuta_launcher_reset (launcher);
					break;
				default:
					break;
			}
			gtk_widget_destroy (dialog);
		}
	}
}

static void
anjuta_launcher_check_password (AnjutaLauncher *launcher, const gchar *chars)
{
	glong start, end;
	gchar *last_line;

	if (!chars || strlen(chars) <= 0)
		return;
	
	/* DEBUG_PRINT ("Chars buffer = %s", chars); */
	start = end = strlen (chars);
	while (start > 0 && chars[start-1] != '\n') start--;

	if (end > start)
	{
		last_line = g_strndup (&chars[start], end - start + 1);

		/* DEBUG_PRINT ("Last line = %s", last_line); */
		/* Checks for password, again */
		anjuta_launcher_check_password_real (launcher, last_line);
		g_free (last_line);
	}
}

static gboolean
is_password_prompt (const gchar* line)
{
	const gchar* password = "assword";
	const gchar* passphrase = "assphrase";
	
	if (strlen (line) < strlen (password)
		|| strlen (line) < strlen (passphrase))
			return FALSE;
	
	if (g_strstr_len(line, 80, password) != NULL
		|| g_strstr_len(line, 80, passphrase) != NULL)
	{
		int i;
		for (i = strlen(line) - 1; i != 0; --i)
		{
			if (line[i] == ':')
					return TRUE;
			if (g_ascii_isspace(line[i]))
				continue;
			else
				return FALSE;
		}
	}
	return FALSE;
}

static void
anjuta_launcher_buffered_output (AnjutaLauncher *launcher,
								 AnjutaLauncherOutputType output_type,
								 const gchar *chars)
{
	gchar *all_lines;
	gchar *incomplete_line;
	gchar **buffer;

	g_return_if_fail (chars != NULL);
	g_return_if_fail (strlen (chars) > 0);

	if (launcher->priv->output_callback == NULL)
		return;
	if (launcher->priv->buffered_output == FALSE)
	{
		(launcher->priv->output_callback)(launcher, output_type, chars,
										  launcher->priv->callback_data);
		return;
	}
	switch (output_type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
		buffer = &launcher->priv->stdout_buffer;
		break;
	case ANJUTA_LAUNCHER_OUTPUT_STDERR:
		buffer = &launcher->priv->stderr_buffer;
		break;
	default:
		g_warning ("Should not reach here");
		return;
	}
	if (*buffer) 
		all_lines = g_strconcat (*buffer, chars, NULL);
	else
		all_lines = g_strdup (chars);
	
	/* Buffer the last incomplete line */
	incomplete_line = all_lines + strlen (all_lines);
	while (incomplete_line > all_lines &&
		   *incomplete_line != '\n')
	{
		incomplete_line = g_utf8_prev_char (incomplete_line);
	}
	if (*incomplete_line == '\n')
		incomplete_line++;
	
	/* Update line buffer */
	g_free(*buffer);
	*buffer = NULL;
	if (strlen(incomplete_line))
	{
		*buffer = g_strdup (incomplete_line);
		/* DEBUG_PRINT ("Line buffer for %d: %s", output_type, incomplete_line); */
	}
	/* Check for password prompt */
	if (launcher->priv->check_for_passwd_prompt)
		anjuta_launcher_check_password (launcher, incomplete_line);
	
	/* Deliver complete lines */
	*incomplete_line = '\0';
	if (strlen (all_lines) > 0)
		(launcher->priv->output_callback)(launcher, output_type, all_lines,
										  launcher->priv->callback_data);
	g_free (all_lines);
}

static gboolean
anjuta_launcher_scan_output (GIOChannel *channel, GIOCondition condition,
							 AnjutaLauncher *launcher)
{
	gsize n;
	gchar buffer[FILE_BUFFER_SIZE];
	gboolean ret = TRUE;

	if (condition & G_IO_IN)
	{
		GError *err = NULL;
		do
		{
			g_io_channel_read_chars (channel, buffer, FILE_BUFFER_SIZE-1, &n, &err);
			if (n > 0) /* There is output */
			{
				gchar *utf8_chars = NULL;
				buffer[n] = '\0';
				if (!launcher->priv->custom_encoding)
					utf8_chars = anjuta_util_convert_to_utf8 (buffer);
				else
					utf8_chars = g_strdup(buffer);
				anjuta_launcher_buffered_output (launcher,
												 ANJUTA_LAUNCHER_OUTPUT_STDOUT,
												 utf8_chars);
				g_free (utf8_chars);
			}
			/* Ignore illegal characters */
			if (err && err->domain == G_CONVERT_ERROR)
			{
				g_error_free (err);
				err = NULL;
			}
			/* The pipe is closed on the other side */
			/* if not related to non blocking read or interrupted syscall */
			else if (err && errno != EAGAIN && errno != EINTR)
			{
				launcher->priv->stdout_is_done = TRUE;
				anjuta_launcher_synchronize (launcher);
				ret = FALSE;
			}
		/* Read next chars if buffer was too small
		 * (the maximum length of one character is 6 bytes) */
		} while (!err && (n > FILE_BUFFER_SIZE - 7));
		if (err)
			g_error_free (err);
	}
	if ((condition & G_IO_ERR) || (condition & G_IO_HUP))
	{
		DEBUG_PRINT ("%s", "launcher.c: STDOUT pipe closed");
		launcher->priv->stdout_is_done = TRUE;
		anjuta_launcher_synchronize (launcher);
		ret = FALSE;
	}
	return ret;
}

static gboolean
anjuta_launcher_scan_error (GIOChannel *channel, GIOCondition condition,
							AnjutaLauncher *launcher)
{
	gsize n;
	gchar buffer[FILE_BUFFER_SIZE];
	gboolean ret = TRUE;
	
	if (condition & G_IO_IN)
	{
		GError *err = NULL;
		do
		{
			g_io_channel_read_chars (channel, buffer, FILE_BUFFER_SIZE-1, &n, &err);
			if (n > 0) /* There is stderr output */
			{
				gchar *utf8_chars;
				buffer[n] = '\0';
				utf8_chars = anjuta_util_convert_to_utf8 (buffer);
				anjuta_launcher_buffered_output (launcher,
												 ANJUTA_LAUNCHER_OUTPUT_STDERR,
												 utf8_chars);
				g_free (utf8_chars);
			}
			/* Ignore illegal characters */
			if (err && err->domain == G_CONVERT_ERROR)
			{
				g_error_free (err);
				err = NULL;
			}
			/* The pipe is closed on the other side */
			/* if not related to non blocking read or interrupted syscall */
			else if (err && errno != EAGAIN && errno != EINTR)
			{
				
				launcher->priv->stderr_is_done = TRUE;
				anjuta_launcher_synchronize (launcher);
				ret = FALSE;
			}
		/* Read next chars if buffer was too small
		 * (the maximum length of one character is 6 bytes) */
		} while (!err && (n > FILE_BUFFER_SIZE - 7));
		if (err)
			g_error_free (err);
	}
	if ((condition & G_IO_ERR) || (condition & G_IO_HUP))
	{
		DEBUG_PRINT ("%s", "launcher.c: STDERR pipe closed");
		launcher->priv->stderr_is_done = TRUE;
		anjuta_launcher_synchronize (launcher);
		ret = FALSE;
	}
	return ret;
}

static gboolean
anjuta_launcher_scan_pty (GIOChannel *channel, GIOCondition condition,
						  AnjutaLauncher *launcher)
{
	gsize n;
	gchar buffer[FILE_BUFFER_SIZE];
	gboolean ret = TRUE;
	
	if (condition & G_IO_IN)
	{
		GError *err = NULL;
		do
		{
			g_io_channel_read_chars (channel, buffer, FILE_BUFFER_SIZE-1, &n, &err);
			if (n > 0) /* There is stderr output */
			{
				gchar *utf8_chars;
				gchar *old_str = launcher->priv->pty_output_buffer;
				buffer[n] = '\0';
				utf8_chars = anjuta_util_convert_to_utf8 (buffer);
				if (old_str)
				{
					gchar *str = g_strconcat (old_str, utf8_chars, NULL);
					launcher->priv->pty_output_buffer = str;
					g_free (old_str);
				}
				else
					launcher->priv->pty_output_buffer = g_strdup (utf8_chars);
				g_free (utf8_chars);
			}
			/* Ignore illegal characters */
			if (err && err->domain == G_CONVERT_ERROR)
			{
				g_error_free (err);
				err = NULL;
			}
			/* The pipe is closed on the other side */
			/* if not related to non blocking read or interrupted syscall */
			else if (err && errno != EAGAIN && errno != EINTR)
			{
				ret = FALSE;
			}
		/* Read next chars if buffer was too small
		 * (the maximum length of one character is 6 bytes) */
		} while (!err && (n > FILE_BUFFER_SIZE - 7));
		if (err)
			g_error_free (err);
		if (launcher->priv->check_for_passwd_prompt
			&& launcher->priv->pty_output_buffer
			&& strlen (launcher->priv->pty_output_buffer) > 0)
		{
			anjuta_launcher_check_password (launcher,
											launcher->priv->pty_output_buffer);
		}
	}
	/* In pty case, we handle the cases in different invocations */
	/* Do not hook up for G_IO_HUP */
	if (condition & G_IO_ERR)
	{
		DEBUG_PRINT ("%s", "launcher.c: PTY pipe error!");
		ret = FALSE;
	}
	return ret;
}

static void
anjuta_launcher_execution_done_cleanup (AnjutaLauncher *launcher,
										gboolean emit_signal)
{
	gint child_status, child_pid;
	time_t start_time;

	/* Remove pending timeout */
	if (launcher->priv->completion_check_timeout != 0)
		g_source_remove (launcher->priv->completion_check_timeout);
	
	if (launcher->priv->stdout_channel)
	{	
		g_io_channel_shutdown (launcher->priv->stdout_channel, emit_signal, NULL);
		g_io_channel_unref (launcher->priv->stdout_channel);
		g_source_remove (launcher->priv->stdout_watch);
	}

	if (launcher->priv->stderr_channel)
	{
		g_io_channel_shutdown (launcher->priv->stderr_channel, emit_signal, NULL);
		g_io_channel_unref (launcher->priv->stderr_channel);
		g_source_remove (launcher->priv->stderr_watch);
	}

	if (launcher->priv->pty_channel)
	{
		g_io_channel_shutdown (launcher->priv->pty_channel, emit_signal, NULL);
		g_io_channel_unref (launcher->priv->pty_channel);
	
		g_source_remove (launcher->priv->pty_watch);
	}

	if (launcher->priv->pty_output_buffer)
		g_free (launcher->priv->pty_output_buffer);
	if (launcher->priv->stdout_buffer)
		g_free (launcher->priv->stdout_buffer);
	if (launcher->priv->stderr_buffer)
		g_free (launcher->priv->stdout_buffer);
	
	/* Save them before we re-initialize */
	child_status = launcher->priv->child_status;
	child_pid = launcher->priv->child_pid;
	start_time = launcher->priv->start_time;
	
	if (emit_signal)
		anjuta_launcher_set_busy (launcher, FALSE);
	
	anjuta_launcher_initialize (launcher);
	
	
	/* Call this here, after set_busy (FALSE) so we are able to 
	   launch a new child from the terminate function.
	   (by clubfan 2002-04-07)
	*/
	/* DEBUG_PRINT ("Exit status: %d", child_status); */
	if (emit_signal)
		g_signal_emit_by_name (launcher, "child-exited", child_pid,
							   child_status,
							   time (NULL) - start_time);
}

/* Using this function is necessary because
 * anjuta_launcher_execution_done_cleanup needs to be called in the same
 * thread than the gtk main loop */
static gboolean
anjuta_launcher_call_execution_done (gpointer data)
{
	AnjutaLauncher *launcher = data;

	launcher->priv->completion_check_timeout = 0;
	anjuta_launcher_execution_done_cleanup (launcher, TRUE);
	return FALSE;
}

/* monitors closure of stdout stderr and pty through a gtk_timeout_add setup */
static gboolean
anjuta_launcher_check_for_execution_done (gpointer data)
{
	AnjutaLauncher *launcher = data;
	/*
	DEBUG_PRINT ("launcher_execution_done STDOUT:%d, STDERR:%d",
				 launcher->priv->stdout_is_done ? 1 : 0,
				 launcher->priv->stderr_is_done ? 1 : 0);
	*/
	if (launcher->priv->stdout_is_done == FALSE ||
	    launcher->priv->stderr_is_done == FALSE)
		return TRUE;
	if (launcher->priv->child_has_terminated == FALSE)
	{
		/* DEBUG_PRINT ("%s", "launcher: We missed the exit of the child"); */
	}
	launcher->priv->completion_check_timeout = 0;
	anjuta_launcher_execution_done_cleanup (launcher, TRUE);
	return FALSE;
}

static void
anjuta_launcher_child_terminated (GPid pid, gint status, gpointer data)
{
	AnjutaLauncher *launcher = data;
	
	g_return_if_fail(ANJUTA_IS_LAUNCHER(launcher));
	
	/* Save child exit code */
	launcher->priv->child_status = status;
	launcher->priv->child_has_terminated = TRUE;
	anjuta_launcher_synchronize (launcher);
}


static gboolean
anjuta_launcher_set_encoding_real (AnjutaLauncher *launcher, const gchar *charset)
{
	GIOStatus s;
	gboolean r = TRUE;

	g_return_val_if_fail (launcher != NULL, FALSE);
	// charset can be NULL
		
	s = g_io_channel_set_encoding (launcher->priv->stderr_channel, charset, NULL);
	if (s != G_IO_STATUS_NORMAL) r = FALSE;
	s = g_io_channel_set_encoding (launcher->priv->stdout_channel, charset, NULL);
	if (s != G_IO_STATUS_NORMAL) r = FALSE;
	s = g_io_channel_set_encoding (launcher->priv->pty_channel, charset, NULL);
	if (s != G_IO_STATUS_NORMAL) r = FALSE;

	if (! r)
	{
		g_warning ("launcher.c: Failed to set channel encoding!");
	}
	return r;
}


/**
 * anjuta_launcher_set_encoding:
 * @launcher: a #AnjutaLancher object.
 * @charset: Character set to use for Input/Output with the process.
 * 
 * Sets the character set to use for Input/Output with the process.
 *
 */
void
anjuta_launcher_set_encoding (AnjutaLauncher *launcher, const gchar *charset)
{
	if (launcher->priv->custom_encoding)
		g_free (launcher->priv->encoding);
	
	launcher->priv->custom_encoding = TRUE;
	if (charset)
	  launcher->priv->encoding = g_strdup(charset);
	else
	  launcher->priv->encoding = NULL;		
}

static pid_t
anjuta_launcher_fork (AnjutaLauncher *launcher, gchar *const args[], gchar *const envp[])
{
	char *working_dir;
	int pty_master_fd, md;
	int stdout_pipe[2], stderr_pipe[2];
	pid_t child_pid;
	struct termios termios_flags;
	gchar * const *env;
	
	working_dir = g_get_current_dir ();
	
	/* The pipes */
	pipe (stderr_pipe);
	pipe (stdout_pipe);

	/* Fork the command */
	child_pid = forkpty (&pty_master_fd, NULL, NULL, NULL);
	if (child_pid == 0)
	{
		close (2);
		dup (stderr_pipe[1]);
		close (1);
		dup (stdout_pipe[1]);
		
		/* Close unnecessary pipes */
		close (stderr_pipe[0]);
		close (stdout_pipe[0]);
		
		/*
		if ((ioctl(pty_slave_fd, TIOCSCTTY, NULL))
			perror ("Could not set new controlling tty");
		*/
		/* Set no delays for the write pipes (non_buffered) so
		that we get all the outputs immidiately */
		if ((md = fcntl (stdout_pipe[1], F_GETFL)) != -1)
			fcntl (stdout_pipe[1], F_SETFL, O_SYNC | md);
		if ((md = fcntl (stderr_pipe[1], F_GETFL)) != -1)
			fcntl (stderr_pipe[1], F_SETFL, O_SYNC | md);
		
		/* Set up environment */
		if (envp != NULL)
		{
			GString *variable = g_string_new (NULL);
			for (env = envp; *env != NULL; env++)
			{
				gchar *value = strchr (*env, '=');

				if (value == NULL)
				{
					g_setenv (*env, NULL, TRUE);
				}
				else
				{
					g_string_truncate (variable, 0);
					g_string_append_len (variable, *env, value - *env);
					g_setenv (variable->str, value + 1, TRUE);
				}
			}
			g_string_free (variable, TRUE);
		}
	
		execvp (args[0], args);
		g_warning (_("Cannot execute command: \"%s\""), args[0]);
		perror(_("execvp failed"));
		_exit(-1);
	}
	g_free (working_dir);
	
	/* Close parent's side pipes */
	close (stderr_pipe[1]);
	close (stdout_pipe[1]);
	
	if (child_pid < 0)
	{
		g_warning ("launcher.c: Fork failed!");
		/* Close parent's side pipes */
		close (stderr_pipe[0]);
		close (stdout_pipe[0]);
		return child_pid;
	}
	
	/*
	*  Set pipes none blocking, so we can read big buffers
	*  in the callback without having to use FIONREAD
	*  to make sure the callback doesn't block.
	*/
	if ((md = fcntl (stdout_pipe[0], F_GETFL)) != -1)
		fcntl (stdout_pipe[0], F_SETFL, O_NONBLOCK | md);
	if ((md = fcntl (stderr_pipe[0], F_GETFL)) != -1)
		fcntl (stderr_pipe[0], F_SETFL, O_NONBLOCK | md);
	if ((md = fcntl (pty_master_fd, F_GETFL)) != -1)
		fcntl (pty_master_fd, F_SETFL, O_NONBLOCK | md);
	
	launcher->priv->child_pid = child_pid;
	launcher->priv->stderr_channel = g_io_channel_unix_new (stderr_pipe[0]);
	launcher->priv->stdout_channel = g_io_channel_unix_new (stdout_pipe[0]);
	launcher->priv->pty_channel = g_io_channel_unix_new (pty_master_fd);

	g_io_channel_set_buffer_size (launcher->priv->pty_channel, FILE_INPUT_BUFFER_SIZE);

	if (!launcher->priv->custom_encoding)
	  g_get_charset ((const gchar**)&launcher->priv->encoding);
	anjuta_launcher_set_encoding_real (launcher, launcher->priv->encoding);
	
	tcgetattr(pty_master_fd, &termios_flags);
	termios_flags.c_iflag &= ~(IGNPAR | INPCK | INLCR | IGNCR | ICRNL | IXON |
					IXOFF | ISTRIP);
	termios_flags.c_iflag |= IGNBRK | BRKINT | IMAXBEL | IXANY;
	termios_flags.c_oflag &= ~OPOST;
//	termios_flags.c_oflag |= 0;
	termios_flags.c_cflag &= ~(CSTOPB | PARENB | HUPCL);
	termios_flags.c_cflag |= CS8 | CLOCAL;

	if (!launcher->priv->terminal_echo_on)
	{
		termios_flags.c_lflag &= ~(ECHOKE | ECHOE | ECHO | ECHONL | 
#ifdef ECHOPRT
						ECHOPRT |
#endif
						ECHOCTL | ISIG | ICANON | IEXTEN | NOFLSH | TOSTOP);
	}
//	termios_flags.c_lflag |= 0;
	termios_flags.c_cc[VMIN] = 0;
	cfsetospeed(&termios_flags, __MAX_BAUD);
	tcsetattr(pty_master_fd, TCSANOW, &termios_flags);

	launcher->priv->stderr_watch = 
		g_io_add_watch (launcher->priv->stderr_channel,
						G_IO_IN | G_IO_ERR | G_IO_HUP,
						(GIOFunc)anjuta_launcher_scan_error, launcher);
	launcher->priv->stdout_watch = 
		g_io_add_watch (launcher->priv->stdout_channel,
						G_IO_IN | G_IO_ERR | G_IO_HUP,
						(GIOFunc)anjuta_launcher_scan_output, launcher);
	launcher->priv->pty_watch = 
		g_io_add_watch (launcher->priv->pty_channel,
						G_IO_IN | G_IO_ERR, /* Do not hook up for G_IO_HUP */
						(GIOFunc)anjuta_launcher_scan_pty, launcher);
	
	/* DEBUG_PRINT ("Terminal child forked: %d", launcher->priv->child_pid); */
	launcher->priv->source = g_child_watch_add (launcher->priv->child_pid,
			anjuta_launcher_child_terminated, launcher);
	return child_pid;
}

/**
 * anjuta_launcher_execute_v:
 * @launcher: a #AnjutaLancher object.
 * @argv: Command args.
 * @envp: Additional environment variable.
 * @callback: The callback for delivering output from the process.
 * @callback_data: Callback data for the above callback.
 * 
 * The first of the @args is the command itself. The rest are sent to the
 * as it's arguments. This function works similar to anjuta_launcher_execute().
 * 
 * Return value: TRUE if successfully launched, otherwise FALSE.
 */
gboolean
anjuta_launcher_execute_v (AnjutaLauncher *launcher, gchar *const argv[],
						   gchar *const envp[],
						   AnjutaLauncherOutputCallback callback,
						   gpointer callback_data)
{
	if (anjuta_launcher_is_busy (launcher))
		return FALSE;
	
	anjuta_launcher_set_busy (launcher, TRUE);
	
	launcher->priv->start_time = time (NULL);
	launcher->priv->child_status = 0;
	launcher->priv->stdout_is_done = FALSE;
	launcher->priv->stderr_is_done = FALSE;
	launcher->priv->child_has_terminated = FALSE;
	launcher->priv->output_callback = callback;
	launcher->priv->callback_data = callback_data;
	
	/* On a fork error perform a cleanup and return */
	if (anjuta_launcher_fork (launcher, argv, envp) < 0)
	{
		anjuta_launcher_initialize (launcher);
		return FALSE;
	}
	return TRUE;
}

/**
 * anjuta_launcher_execute:
 * @launcher: a #AnjutaLancher object.
 * @command_str: The command to execute.
 * @callback: The callback for delivering output from the process.
 * @callback_data: Callback data for the above callback.
 * 
 * Executes a command in the launcher. Both outputs (STDOUT and STDERR) are
 * delivered to the above callback. The data are delivered as they arrive
 * from the process and could be of any lenght. If the process asks for
 * passwords, the user will be automatically prompted with a dialog to enter
 * it. Please note that not all formats of the password are recognized. Those
 * with the standard 'assword:' substring in the prompt should work well.
 * 
 * Return value: TRUE if successfully launched, otherwise FALSE.
 */
gboolean
anjuta_launcher_execute (AnjutaLauncher *launcher, const gchar *command_str,
						 AnjutaLauncherOutputCallback callback,
						 gpointer callback_data)
{
	GList *args_list, *args_list_ptr;
	gchar **args, **args_ptr;
	gboolean ret;
	
	/* Prepare command args */
	args_list = anjuta_util_parse_args_from_string (command_str);
	args = g_new (char*, g_list_length (args_list) + 1);
	args_list_ptr = args_list;
	args_ptr = args;
	while (args_list_ptr)
	{
		*args_ptr = (char*) args_list_ptr->data;
		args_list_ptr = g_list_next (args_list_ptr);
		args_ptr++;
	}
	*args_ptr = NULL;

	ret = anjuta_launcher_execute_v (launcher, args, NULL,
		callback, callback_data);
	g_free (args);
	anjuta_util_glist_strings_free (args_list);
	return ret;
}

/**
 * anjuta_launcher_set_buffered_output:
 * @launcher: a #AnjutaLancher object.
 * @buffered: buffer output.
 * 
 * Sets if output should buffered or not. By default, it is buffered.
 *
 * Return value: Previous flag value
 */
gboolean
anjuta_launcher_set_buffered_output (AnjutaLauncher *launcher, gboolean buffered)
{
	gboolean past_value = launcher->priv->buffered_output;
	launcher->priv->buffered_output = buffered;
	return past_value;
}

/**
 * anjuta_launcher_set_check_passwd_prompt:
 * @launcher: a #AnjutaLancher object.
 * @check_passwd: check for password.
 * 
 * Set if output is checked for a password prompti. A special dialog box
 * is use to enter it in this case. By default, this behavior is enabled.
 *
 * Return value: Previous flag value
 */
gboolean
anjuta_launcher_set_check_passwd_prompt (AnjutaLauncher *launcher, gboolean check_passwd)
{
	gboolean past_value = launcher->priv->check_for_passwd_prompt;
	launcher->priv->check_for_passwd_prompt = check_passwd;
	return past_value;
}

/**
 * anjuta_launcher_set_terminal_echo:
 * @launcher: a #AnjutaLancher object.
 * @echo_on: Echo ON flag.
 * 
 * Sets if input (those given in STDIN) should enabled or disabled. By default,
 * it is disabled.
 *
 * Return value: Previous flag value
 */
gboolean
anjuta_launcher_set_terminal_echo (AnjutaLauncher *launcher,
								   gboolean echo_on)
{
	gboolean past_value = launcher->priv->terminal_echo_on;
	launcher->priv->terminal_echo_on = echo_on;
	return past_value;
}

/**
 * anjuta_launcher_set_terminate_on_exit:
 * @launcher: a #AnjutaLancher object.
 * @terminate_on_exit: terminate on exit flag
 * 
 * When this flag is set, al i/o channels are closed and the child-exit
 * signal is emitted as soon as the child exit. By default, or when this
 * flag is clear, the launcher object wait until the i/o channels are
 * closed.
 *
 * Return value: Previous flag value
 */
gboolean
anjuta_launcher_set_terminate_on_exit (AnjutaLauncher *launcher,
						gboolean terminate_on_exit)
{
	gboolean past_value = launcher->priv->terminate_on_exit;
	launcher->priv->terminate_on_exit = terminate_on_exit;
	return past_value;
}

/**
 * anjuta_launcher_new:
 * 
 * Sets if input (those given in STDIN) should enabled or disabled. By default,
 * it is disabled.
 *
 * Return value: a new instance of #AnjutaLancher class.
 */
AnjutaLauncher*
anjuta_launcher_new ()
{
	return g_object_new (ANJUTA_TYPE_LAUNCHER, NULL);
}

/* GTK - The GIMP Toolkit
 * Copyright (C) 2003 Naba Kumar
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pty.h>
#include <assert.h>
#include <gnome.h>
#include <termios.h>

#include "pixmaps.h"
#include "launcher.h"
#include "resources.h"
#include "utilities.h"
#include "anjuta-marshalers.h"
#include "anjuta.h"

#define FILE_BUFFER_SIZE 1024
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

	/* The child */
	pid_t child_pid;
	gint child_status;
	gboolean child_has_terminated;
	
	/* Synchronization in progress */
	gboolean in_synchronization;
	
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
static gboolean anjuta_launcher_check_for_execution_done (gpointer data);

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
	
	/* The child */
	obj->priv->child_pid = 0;
	obj->priv->child_status = -1;
	obj->priv->child_has_terminated = TRUE;
	
	/* Synchronization in progress */
	obj->priv->in_synchronization = FALSE;
	
	/* Start time of execution */
	obj->priv->start_time = 0;
	
	obj->priv->buffered_output = TRUE;
	obj->priv->check_for_passwd_prompt = TRUE;
	
	/* Output callback */
	obj->priv->output_callback = NULL;
	obj->priv->callback_data = NULL;
}

guint
anjuta_launcher_get_type ()
{
	static guint obj_type = 0;
	
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
			sizeof (AnjutaLauncherClass),
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
	/* AnjutaLauncher *launcher = ANJUTA_LAUNCHER (obj); */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_launcher_finalize (GObject *obj)
{
	AnjutaLauncher *launcher = ANJUTA_LAUNCHER (obj);	
	/* FIXME: Test this Clean up procedure when launcher is still busy */
	if (anjuta_launcher_is_busy (launcher))
	{
		anjuta_unregister_child_process (launcher->priv->child_pid);
		if (!launcher->priv->stdout_is_done)
		{
			g_source_remove (launcher->priv->stdout_watch);
			g_io_channel_shutdown (launcher->priv->stdout_channel, TRUE, NULL);
			g_io_channel_unref (launcher->priv->stdout_channel);
		}
		if (!launcher->priv->stderr_is_done)
		{
			g_source_remove (launcher->priv->stderr_watch);
			g_io_channel_shutdown (launcher->priv->stderr_channel, TRUE, NULL);
			g_io_channel_unref (launcher->priv->stderr_channel);
		}
		
		/* Shutdown pty */
		g_source_remove (launcher->priv->pty_watch);
		g_io_channel_shutdown (launcher->priv->pty_channel, TRUE, NULL);
		g_io_channel_unref (launcher->priv->pty_channel);
		
		/* Shutdowin stdin */
		//g_io_channel_shutdown (launcher->priv->stdin_channel, TRUE, NULL);
		//g_io_channel_unref (launcher->priv->stdin_channel);
	}
	g_free (launcher->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_launcher_class_init (AnjutaLauncherClass * klass)
{
	GObjectClass *object_class;
	g_return_if_fail (klass != NULL);
	object_class = (GObjectClass *) klass;
	
	g_message ("Initializing launcher class");
	
	parent_class = g_type_class_peek_parent (klass);
	/*
	launcher_signals[OUTPUT_ARRIVED_SIGNAL] =
		g_signal_new ("output-arrived",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaLauncherClass,
									 output_arrived_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__INT_STRING,
					G_TYPE_NONE, 2, GTK_TYPE_INT, GTK_TYPE_STRING);
	*/
	launcher_signals[CHILD_EXITED_SIGNAL] =
		g_signal_new ("child-exited",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaLauncherClass,
									 child_exited_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__INT_INT_ULONG,
					G_TYPE_NONE, 3, GTK_TYPE_INT,
					GTK_TYPE_INT, GTK_TYPE_ULONG);
	
	launcher_signals[BUSY_SIGNAL] =
		g_signal_new ("busy",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaLauncherClass,
									 busy_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__BOOLEAN,
					G_TYPE_NONE, 1, GTK_TYPE_BOOL);
	
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

void
anjuta_launcher_send_stdin (AnjutaLauncher *launcher, const gchar * input_str)
{
	g_return_if_fail (launcher);
	g_return_if_fail (input_str);

	anjuta_launcher_send_ptyin (launcher, input_str);
}

void
anjuta_launcher_send_ptyin (AnjutaLauncher *launcher, const gchar * input_str)
{
	int bytes_written;
	GError *err = NULL;
	
	if (!input_str || strlen (input_str) == 0) return;
	
	g_io_channel_write_chars (launcher->priv->pty_channel,
							  input_str, strlen (input_str),
							  &bytes_written, &err);
	g_io_channel_flush (launcher->priv->pty_channel, NULL);
	if (err)
	{
		g_warning ("Error encountered while writing to PTY!. %s",
				   err->message);
		g_error_free (err);
	}
}

void
anjuta_launcher_reset (AnjutaLauncher *launcher)
{
	if (anjuta_launcher_is_busy (launcher))
		kill (launcher->priv->child_pid, SIGTERM);
}

void
anjuta_launcher_signal (AnjutaLauncher *launcher, int sig)
{
	kill (launcher->priv->child_pid, sig);
}

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
		launcher->priv->in_synchronization = TRUE;
		gtk_timeout_add (50, anjuta_launcher_check_for_execution_done,
						 launcher);
	}
	
	/* This case is not very good, but it blocks the whole IDE
	because we never new if the child has finished */
	else if (launcher->priv->stdout_is_done &&
			 launcher->priv->stderr_is_done)
		gtk_timeout_add(200, anjuta_launcher_check_for_execution_done,
						 launcher);
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
	gtk_box_pack_start_defaults (GTK_BOX(hbox), icon);
	
	if (strlen (prompt) < 20) {
		box = gtk_hbox_new (FALSE, 5);
	} else {
		box = gtk_vbox_new (FALSE, 5);
	}
	gtk_widget_show (box);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), box);
	
	label = gtk_label_new (_(prompt));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	
	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
	
	gtk_widget_ref (entry);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "password_entry",
							  gtk_widget_ref (entry),
							  (GDestroyNotify) gtk_widget_unref);
	gtk_widget_grab_focus (entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	
	return dialog;
}

/* pty buffer check for password authentication */
static void
anjuta_launcher_check_password_real (AnjutaLauncher *launcher,
									 const gchar* last_line)
{
	gchar *prompt = "assword: ";
	const gchar *prompt_index;
	
	if (anjuta_launcher_is_busy (launcher) == FALSE) return;
	
	if (last_line) {
#ifdef DEBUG
		// g_message ("(In password) Last line = %s", last_line);
#endif
		if (strlen (last_line) < strlen (prompt))
			return;
		prompt_index = &last_line[strlen (last_line) - strlen (prompt)];
		if (strcasecmp (prompt_index, prompt) == 0) {
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
					anjuta_launcher_send_ptyin (launcher, "<canceled>\n");
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
#ifdef DEBUG
	// g_message("Chars buffer = %s", chars);
#endif
	start = end = strlen (chars);
	while (start > 0 && chars[start-1] != '\n') start--;

	if (end > start)
	{
		last_line = g_strndup (&chars[start], end - start + 1);

#ifdef DEBUG
		// g_message ("Last line = %s", last_line);
#endif
		/* Checks for password, again */
		anjuta_launcher_check_password_real (launcher, last_line);
		g_free (last_line);
	}
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
	incomplete_line = all_lines + strlen (all_lines) - 1;
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
#ifdef DEBUG
		// g_message ("Line buffer for %d: %s", output_type, incomplete_line);
#endif
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
	int n;
	gchar buffer[FILE_BUFFER_SIZE];
	gboolean ret = TRUE;
	
	if (condition & G_IO_IN)
	{
		GError *err = NULL;
		g_io_channel_read_chars (channel, buffer, FILE_BUFFER_SIZE-1, &n, &err);
		if (n > 0 && !err) /* There is output */
		{
			gchar *utf8_chars;
			buffer[n] = '\0';
			utf8_chars = anjuta_util_convert_to_utf8 (buffer);
			anjuta_launcher_buffered_output (launcher,
											 ANJUTA_LAUNCHER_OUTPUT_STDOUT,
											 utf8_chars);
			g_free (utf8_chars);
		}
		/* The pipe is closed on the other side */
		/* if not related to non blocking read or interrupted syscall */
		else if (err && errno != EAGAIN && errno != EINTR)
		{
#ifdef DEBUG
			g_message(_("launcher.c: Error while reading child stdout\n"));
#endif
			launcher->priv->stdout_is_done = TRUE;
			anjuta_launcher_synchronize (launcher);
			ret = FALSE;
		}
		if (err)
			g_error_free (err);
	}
	if ((condition & G_IO_ERR) || (condition & G_IO_HUP))
	{
#ifdef DEBUG
		g_message(_("launcher.c: STDOUT pipe closed"));
#endif
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
	int n;
	gchar buffer[FILE_BUFFER_SIZE];
	gboolean ret = TRUE;
	
	if (condition & G_IO_IN)
	{
		GError *err = NULL;
		g_io_channel_read_chars (channel, buffer, FILE_BUFFER_SIZE-1, &n, &err);
		if (n > 0 && !err) /* There is stderr output */
		{
			gchar *utf8_chars;
			buffer[n] = '\0';
			utf8_chars = anjuta_util_convert_to_utf8 (buffer);
			anjuta_launcher_buffered_output (launcher,
											 ANJUTA_LAUNCHER_OUTPUT_STDERR,
											 utf8_chars);
			g_free (utf8_chars);
		}
		/* The pipe is closed on the other side */
		/* if not related to non blocking read or interrupted syscall */
		else if (err && errno != EAGAIN && errno != EINTR)
		{
#ifdef DEBUG
			g_message(_("launcher.c: Error while reading child stderr"));
#endif
			launcher->priv->stderr_is_done = TRUE;
			anjuta_launcher_synchronize (launcher);
			ret = FALSE;
		}
		if (err)
			g_error_free (err);
	}
	if ((condition & G_IO_ERR) || (condition & G_IO_HUP))
	{
#ifdef DEBUG
		g_message (_("launcher.c: STDERR pipe closed"));
#endif
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
	int n;
	gchar buffer[FILE_BUFFER_SIZE];
	gboolean ret = TRUE;
	
	if (condition & G_IO_IN)
	{
		GError *err = NULL;
		g_io_channel_read_chars (channel, buffer, FILE_BUFFER_SIZE-1, &n, &err);
		if (n > 0 && !err) /* There is stderr output */
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
		/* The pipe is closed on the other side */
		/* if not related to non blocking read or interrupted syscall */
		else if (err && errno != EAGAIN && errno != EINTR)
		{
			g_warning (_("launcher.c: Error while reading child pty\n"));
			ret = FALSE;
		}
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
#ifdef DEBUG
		g_message(_("launcher.c: PTY pipe error!"));
#endif
		ret = FALSE;
	}
	return ret;
}

static void
anjuta_launcher_execution_done_cleanup (AnjutaLauncher *launcher)
{
	gint child_status, child_pid;
	time_t start_time;
	
	/* PTY is not synchronized. So it is removed explicitly. */
	g_source_remove (launcher->priv->pty_watch);
	
	g_io_channel_shutdown (launcher->priv->stdout_channel, TRUE, NULL);
	g_io_channel_shutdown (launcher->priv->stderr_channel, TRUE, NULL);
	//g_io_channel_shutdown (launcher->priv->stdin_channel, TRUE, NULL);
	g_io_channel_shutdown (launcher->priv->pty_channel, TRUE, NULL);
	
	g_io_channel_unref (launcher->priv->stdout_channel);
	g_io_channel_unref (launcher->priv->stderr_channel);
	//g_io_channel_unref (launcher->priv->stdin_channel);
	g_io_channel_unref (launcher->priv->pty_channel);
	
	if (launcher->priv->pty_output_buffer)
		g_free (launcher->priv->pty_output_buffer);

	/* Save them before we re-initialize */
	child_status = launcher->priv->child_status;
	child_pid = launcher->priv->child_pid;
	start_time = launcher->priv->start_time;
	
	anjuta_launcher_set_busy (launcher, FALSE);
	anjuta_launcher_initialize (launcher);
	
	/* Call this here, after set_busy (FALSE) so we are able to 
	   launch a new child from the terminate function.
	   (by clubfan 2002-04-07)
	*/
#ifdef DEBUG
	g_message ("Exit status: %d", child_status);
#endif
	g_signal_emit_by_name (launcher, "child-exited", child_pid, child_status,
						   time (NULL) - start_time);
}

/* monitors closure of stdout stderr and pty through a gtk_timeout_add setup */
static gboolean
anjuta_launcher_check_for_execution_done (gpointer data)
{
	AnjutaLauncher *launcher = data;
	
#ifdef DEBUG	
	g_message ("launcher_execution_done STDOUT:%d, STDERR:%d",
			   launcher->priv->stdout_is_done ? 1 : 0,
			   launcher->priv->stderr_is_done ? 1 : 0);
#endif

	if (launcher->priv->stdout_is_done == FALSE ||
	    launcher->priv->stderr_is_done == FALSE)
		return TRUE;
	if (launcher->priv->child_has_terminated == FALSE)
	{
#ifdef DEBUG
		g_warning ("launcher: We missed the exit of the child");
#endif
		anjuta_kernel_signals_connect();
	}
	
	anjuta_launcher_execution_done_cleanup (launcher);
	return FALSE;
}

static void
anjuta_launcher_child_terminated (int status, gpointer data)
{
	AnjutaLauncher *launcher = data;
	
#ifdef DEBUG	
	g_message ("Terminal child terminated called .. status = %d", status);
#endif
	
	/* Save child exit code */
	launcher->priv->child_status = status;
	launcher->priv->child_has_terminated = TRUE;
	anjuta_launcher_synchronize (launcher);
}

gboolean
anjuta_launcher_set_encoding (AnjutaLauncher *launcher, const gchar *charset)
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
		g_warning (_("launcher.c: Failed to set channel encoding!"));
	}
	return r;
}

static pid_t
anjuta_launcher_fork (AnjutaLauncher *launcher, gchar *const args[])
{
	char *working_dir;
	int pty_master_fd, md;
	int stdout_pipe[2], stderr_pipe[2]/*, stdin_pipe[2]*/;
	pid_t child_pid;
	struct termios termios_flags;
	const gchar *charset;
	
	working_dir = g_get_current_dir ();
	
	/* The pipes */
	pipe (stderr_pipe);
	pipe (stdout_pipe);
	// pipe (stdin_pipe);

	/* Fork the command */
	child_pid = forkpty (&pty_master_fd, NULL, NULL, NULL);
	if (child_pid == 0)
	{
		close (2);
		dup (stderr_pipe[1]);
		close (1);
		dup (stdout_pipe[1]);
		/*close (0);
		dup (stdin_pipe[0]);
		*/
		/* Close unnecessary pipes */
		close (stderr_pipe[0]);
		close (stdout_pipe[0]);
		// close (stdin_pipe[1]);
		
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
		
		execvp (args[0], args);
		g_error (_("Cannot execute command shell"));
	}
	g_free (working_dir);
	
	/* Close parent's side pipes */
	close (stderr_pipe[1]);
	close (stdout_pipe[1]);
	// close (stdin_pipe[0]);
	
	if (child_pid < 0)
	{
		g_warning ("launcher.c: Fork failed!");
		/* Close parent's side pipes */
		close (stderr_pipe[0]);
		close (stdout_pipe[0]);
		// close (stdin_pipe[1]);
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
	// launcher->priv->stdin_channel = g_io_channel_unix_new (stdin_pipe[1]);
	launcher->priv->pty_channel = g_io_channel_unix_new (pty_master_fd);

	g_get_charset (&charset);
	anjuta_launcher_set_encoding (launcher, charset);

	tcgetattr(pty_master_fd, &termios_flags);
	termios_flags.c_iflag &= ~(IGNPAR | INPCK | INLCR | IGNCR | ICRNL | IXON |
					IXOFF | ISTRIP);
	termios_flags.c_iflag |= IGNBRK | BRKINT | IMAXBEL | IXANY;
	termios_flags.c_oflag &= ~OPOST;
//	termios_flags.c_oflag |= 0;
	termios_flags.c_cflag &= ~(CSTOPB | CREAD | PARENB | HUPCL);
	termios_flags.c_cflag |= CS8 | CLOCAL;
	
	/* Do not enable terminal echo. Password prompts fail to work */ 
	/* termios_flags.c_lflag &= ~(ECHOKE | ECHOE | ECHO | ECHONL | ECHOPRT |
					ECHOCTL | ISIG | ICANON | IEXTEN | NOFLSH | TOSTOP); */
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
#ifdef DEBUG
	g_message ("Terminal child forked: %d", launcher->priv->child_pid);
#endif
	anjuta_register_child_process (launcher->priv->child_pid,
								   anjuta_launcher_child_terminated,
								   launcher);
	return child_pid;
}

gboolean
anjuta_launcher_execute_v (AnjutaLauncher *launcher, gchar *const args[],
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
	launcher->priv->in_synchronization = FALSE;
	launcher->priv->output_callback = callback;
	launcher->priv->callback_data = callback_data;
	
	/* On a fork error perform a cleanup and return */
	if (anjuta_launcher_fork (launcher, args) < 0)
	{
		anjuta_launcher_initialize (launcher);
		return FALSE;
	}
	return TRUE;
}

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

	ret = anjuta_launcher_execute_v (launcher, args, 
		callback, callback_data);
	g_free (args);
	glist_strings_free (args_list);
	return ret;
}

GObject *
anjuta_launcher_new ()
{
	return g_object_new (ANJUTA_TYPE_LAUNCHER, NULL);
}

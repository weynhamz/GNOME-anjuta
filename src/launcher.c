/*
    launcher.c
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
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <gnome.h>

#include "anjuta.h"
#include "resources.h"
#include "launcher.h"
#include "controls.h"
#include "global.h"

static void to_launcher_child_terminated (int status, gpointer data);
static gint launcher_poll_inputs_on_idle (gpointer data);
static gboolean launcher_execution_done (gpointer data);
static void launcher_set_busy (gboolean flag);

/************************************************************
*    Due to some programming restriction only one instance of
*    Launcher is declared. Moreover, I have no intension of using
*    more than one instance of this structure.
*
*    That's why I have defined it here and not in launcher.h
*************************************************************/
struct				/* Launcher */
{
/*
 *  Busy flag is TRUE if the Launcher
 *  is currently executing a child.
 */
  gboolean busy;

/* These flags are used to synchronize the IO operations. */
  gboolean stdout_is_done;
  gboolean stderr_is_done;
  gboolean child_has_terminated;

/* Child's stdin, stdout and stderr pipes */
  gint stderr_pipe[2];
  gint stdout_pipe[2];
  gint stdin_pipe[2];

/* Gdk  Input Tags */
  gint idle_id;
  gint poll_id;

/* The child */
  pid_t child_pid;
  gint child_status;

/* Start time of execution */
  time_t start_time;

  /* Callbacks */
  void (*stdout_arrived) (gchar *);
  void (*stderr_arrived) (gchar *);
  void (*child_terminated) (gint status, time_t time);

}
launcher;

/* By any means DO NOT call launcher_init() more than once */
void
launcher_init ()
{
  launcher_set_busy (FALSE);
}

gboolean launcher_is_busy ()
{
  return launcher.busy;
}

static void
launcher_set_busy (gboolean flag)
{
  launcher.busy = flag;
  main_toolbar_update ();
  extended_toolbar_update ();
}

static void
launcher_scan_output ()
{
  int n;
  gchar buffer[FILE_BUFFER_SIZE + 1];

  n = read (launcher.stdout_pipe[0], buffer, FILE_BUFFER_SIZE);
  if (n > 0)			/*    There is output  */
  {
    *(buffer + n) = '\0';
    if (launcher.stdout_arrived)
      (*(launcher.stdout_arrived)) (buffer);
  }
  /* The pipe is closed on the other side */
  if (n == 0)
  {
    launcher.stdout_is_done = TRUE;
  }
}

static void
launcher_scan_error ()
{
  int n;
  gchar buffer[FILE_BUFFER_SIZE + 5];

  /* we have to read all the error outputs, otherwise we will miss some of them */
  do
  {
    n = read (launcher.stderr_pipe[0], buffer, FILE_BUFFER_SIZE);
    if (n > 0)			/*    There is stderr output  */
    {
      *(buffer + n) = '\0';
      if (launcher.stderr_arrived)
	(*(launcher.stderr_arrived)) (buffer);
    }
    if (n == 0)			/* The pipe is closed on the other side */
    {
      launcher.stderr_is_done = TRUE;
    }
  }
  while (n == FILE_BUFFER_SIZE);
}

void
launcher_send_stdin (gchar * input_str)
{
  write (launcher.stdin_pipe[1], input_str,
	 strlen (input_str) * sizeof (char));
  write (launcher.stdin_pipe[1], "\n", sizeof (char));
}

void
launcher_reset (void)
{
  if (launcher_is_busy ())
    kill (launcher.child_pid, SIGTERM);
}

void
launcher_signal (int sig)
{
  if (launcher_is_busy ())
    kill (launcher.child_pid + 1, sig);
}

gboolean
launcher_execute (gchar * command_str,
		  void (*so_line_arrived) (gchar *),
		  void (*se_line_arrived) (gchar *),
		  void (*cmd_terminated) (gint status, time_t time))
{
  int md;
  gchar *shell;

  if (launcher_is_busy ())
    return FALSE;

  launcher_set_busy (TRUE);

/* The callback functions */
  launcher.stdout_arrived = so_line_arrived;
  launcher.stderr_arrived = se_line_arrived;
  launcher.child_terminated = cmd_terminated;

/* The pipes */
  pipe (launcher.stderr_pipe);
  pipe (launcher.stdout_pipe);
  pipe (launcher.stdin_pipe);

  launcher.start_time = time (NULL);
  launcher.child_has_terminated = FALSE;
  launcher.stdout_is_done = FALSE;
  launcher.stderr_is_done = FALSE;

  shell = gnome_util_user_shell();
  if ((launcher.child_pid = fork ()) == 0)
  {
    close (2);
    dup (launcher.stderr_pipe[1]);
    close (1);
    dup (launcher.stdout_pipe[1]);
    close (0);
    dup (launcher.stdin_pipe[0]);

    /* Close unnecessary pipes */
    close (launcher.stderr_pipe[0]);
    close (launcher.stdout_pipe[0]);
    close (launcher.stdin_pipe[1]);

    execlp (shell, shell, "-c", command_str, NULL);
    g_error (_("Cannot execute command shell"));
  }
  anjuta_register_child_process (launcher.child_pid,
				 to_launcher_child_terminated, NULL);

  close (launcher.stderr_pipe[1]);
  close (launcher.stdout_pipe[1]);
  close (launcher.stdin_pipe[0]);

/*
 *  Set pipes none blocking, so we can read big buffers
 *  in the callback without having to use FIONREAD
 *  to make sure the callback doesn't block.
 */
  if ((md = fcntl (launcher.stdout_pipe[0], F_GETFL)) != -1)
    fcntl (launcher.stdout_pipe[0], F_SETFL, O_NONBLOCK | md);
  if ((md = fcntl (launcher.stderr_pipe[0], F_GETFL)) != -1)
    fcntl (launcher.stderr_pipe[0], F_SETFL, O_NONBLOCK | md);

  launcher.poll_id = gtk_timeout_add (50, launcher_poll_inputs_on_idle, NULL);
  return TRUE;
}

static gint launcher_poll_inputs_on_idle (gpointer data)
{
  gboolean ret;
  ret = FALSE;
  if (launcher.stderr_is_done == FALSE)
  {
    launcher_scan_error ();
    ret = TRUE;
  }
  if (launcher.stdout_is_done == FALSE)
  {
    launcher_scan_output ();
    ret = TRUE;
  }
  if (ret == FALSE)
  {
    close (launcher.stdout_pipe[0]);
    close (launcher.stderr_pipe[0]);
  }
  return ret;
}

pid_t launcher_get_child_pid ()
{
  if (launcher_is_busy ())
    return launcher.child_pid;
  else
    return -1;
}

static void
to_launcher_child_terminated (int status, gpointer data)
{
  close (launcher.stdin_pipe[1]);
  launcher.child_status = status;
  launcher.child_has_terminated = TRUE;
  launcher.idle_id = gtk_idle_add (launcher_execution_done, NULL);
}

static gboolean launcher_execution_done (gpointer data)
{
  if (launcher.stdout_is_done == FALSE || launcher.stderr_is_done == FALSE)
    return TRUE;

  if (launcher.child_terminated)
    (*(launcher.child_terminated)) (launcher.child_status, time (NULL) - launcher.start_time);
  launcher_set_busy (FALSE);
  return FALSE;
}

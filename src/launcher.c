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

#include <ctype.h>
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
#include <zvt/zvtterm.h>

#include "anjuta.h"
#include "resources.h"
#include "launcher.h"
#include "controls.h"
#include "global.h"
#include "pixmaps.h"

#define DEBUG

static void to_terminal_child_terminated (GtkWidget* term, gpointer data);
static void to_launcher_child_terminated (int status, gpointer data);
static gint launcher_poll_inputs_on_idle (gpointer data);
static gboolean launcher_execution_done (gpointer data);
static void launcher_set_busy (gboolean flag);

static void launcher_send_ptyin (gchar * input_str);
static void launcher_pty_check_password(gchar* buffer);
static GtkWidget* create_password_dialog (gchar* prompt);

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

/* Controlling terminal */
  GtkWidget* terminal;
/* The position where the last buffer was delivered */
  glong char_pos;

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

} launcher;

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

static void
launcher_scan_pty()
{
   gchar* chars;
   chars = zvt_term_get_buffer(ZVT_TERM(launcher.terminal), NULL, VT_SELTYPE_LINE,
      -10000, 0, 10000, 0);
   if (chars && strlen(chars) > launcher.char_pos) {
	  glong start, end;
	  gchar *last_line;
	  start = launcher.char_pos;
	  end = strlen(chars)-1;
	  while (start > 0 && chars[start-1] != '\n') start--;
	  while (end > start && chars[end] == '\n') end--;
	  if (end > start) {
		  last_line = g_strndup(&chars[start], end-start+1);
		  launcher_pty_check_password(last_line);
		  g_free(last_line);
	  }
	  launcher.char_pos = strlen(chars);
	  g_free(chars);
   }
};

void
launcher_send_stdin (gchar * input_str)
{
  write (launcher.stdin_pipe[1], input_str,
	 strlen (input_str) * sizeof (char));
  write (launcher.stdin_pipe[1], "\n", sizeof (char));
}

void
launcher_send_ptyin (gchar * input_str)
{
  if (!input_str || strlen(input_str) == 0) return;
  zvt_term_writechild(ZVT_TERM(launcher.terminal), input_str, strlen(input_str));
}

void
launcher_reset (void)
{
  if (launcher_is_busy ())
    /* kill (launcher.child_pid, SIGTERM); */
	zvt_term_killchild(ZVT_TERM(launcher.terminal), SIGTERM);
}

void
launcher_signal (int sig)
{
  if (launcher_is_busy ())
    /* kill (launcher.child_pid + 1, sig); */
	zvt_term_killchild(ZVT_TERM(launcher.terminal), sig);
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
  
  launcher.char_pos = 1;

  shell = gnome_util_user_shell();
  
  launcher.terminal = zvt_term_new ();
  gtk_signal_connect (GTK_OBJECT (launcher.terminal), "child_died", 
		GTK_SIGNAL_FUNC (to_terminal_child_terminated), NULL);

  
  launcher.child_pid = zvt_term_forkpty (ZVT_TERM (launcher.terminal), 
		ZVT_TERM_DO_UTMP_LOG | ZVT_TERM_DO_WTMP_LOG);
  if (launcher.child_pid == 0)
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

	/* Set no delays for the write pipes (non_buffered) so
	  that we get all the outputs immidiately */
	if ((md = fcntl (launcher.stdout_pipe[1], F_GETFL)) != -1)
		fcntl (launcher.stdout_pipe[1], F_SETFL, O_SYNC | md);
	if ((md = fcntl (launcher.stderr_pipe[1], F_GETFL)) != -1)
		fcntl (launcher.stderr_pipe[1], F_SETFL, O_SYNC | md);
  
    execlp (shell, shell, "-c", command_str, NULL);
    g_error (_("Cannot execute command shell"));
  }
  /* FIXME: SIGCHLD is not being called here. It seems
   * ptyfork is spawning the child as a separate process group.
   * Becuase of this, the child's exit status could not be
   * determined.
  */
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
  
  if (launcher.child_has_terminated == FALSE && ret == TRUE)
	  launcher_scan_pty();
  
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
to_terminal_child_terminated (GtkWidget* term, gpointer data)
{
#ifdef DEBUG	
	printf("Terminal child terminated called\n");
#endif
	close (launcher.stdin_pipe[1]);
	
	/* FIXME: How do we know the child's exit
	 * status here??
	 */
	launcher.child_status = 0;
	launcher.child_has_terminated = TRUE;
	launcher.idle_id = gtk_idle_add (launcher_execution_done, NULL);
}

static void
to_launcher_child_terminated (int status, gpointer data)
{
  /* This function is no longer being called */
  printf("Launcher child terminated called\n");
}

static gboolean
launcher_execution_done (gpointer data)
{
  if (launcher.stdout_is_done == FALSE || launcher.stderr_is_done == FALSE)
    return TRUE;

  if (launcher.child_terminated)
    (*(launcher.child_terminated)) (launcher.child_status, time (NULL) - launcher.start_time);
  
  zvt_term_closepty(ZVT_TERM(launcher.terminal));
  zvt_term_reset(ZVT_TERM(launcher.terminal), 1);
  gtk_widget_destroy(launcher.terminal);
  
  launcher_set_busy (FALSE);
  return FALSE;
}

/* Password dialog */
static GtkWidget*
create_password_dialog (gchar* prompt)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *box;
	GtkWidget *icon;
	gchar* icon_file;
	GtkWidget *label;
	GtkWidget *entry;
	
	g_return_val_if_fail(prompt, NULL);
	
	dialog =
		gnome_dialog_new (prompt, _("Ok"), _("Cancel"), NULL);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "launcher-password-prompt",
				"anjuta");
	
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_widget_show(hbox);
	gtk_box_pack_start_defaults (GTK_BOX
				     (GNOME_DIALOG (dialog)->vbox),
				     hbox);
	
	icon_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_PASSWORD);
	if (icon_file) {
		icon = gnome_pixmap_new_from_file(icon_file);
		gtk_widget_show(icon);
		gtk_box_pack_start_defaults (GTK_BOX(hbox), icon);
		g_free(icon_file);
	} else {
		g_warning (ANJUTA_PIXMAP_PASSWORD" counld not be found.");
	}
	
	if (strlen(prompt) < 20) {
		box = gtk_hbox_new(FALSE, 5);
	} else {
		box = gtk_vbox_new(FALSE, 5);
	}
	gtk_widget_show(box);
	gtk_box_pack_start_defaults (GTK_BOX(hbox), box);
	
	label = gtk_label_new (_(prompt));
	gtk_widget_show(label);
	/* gtk_misc_set_alignment(GTK_MISC(label), -1, 0); */
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	
	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	
	gtk_widget_ref(entry);
	gtk_object_set_data_full(GTK_OBJECT(dialog), "password_entry",
				entry, (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_grab_focus(entry);
	
	return dialog;
}

/* pty buffer check for password authentication */
static void
launcher_pty_check_password(gchar* last_line)
{
	gchar *prompt = "assword: ";
	gchar *prompt_index;
	
	if (launcher_is_busy() == FALSE) return;
	
	if (last_line) {
#ifdef DEBUG
		printf("%s\n", last_line);
#endif
		if (strlen(last_line) < strlen(prompt))
			return;
		prompt_index = &last_line[strlen(last_line) - strlen(prompt)];
		if (strcasecmp(prompt_index, prompt) == 0) {
			/* Password prompt detected */
			GtkWidget* dialog;
			gint button;
			gchar* passwd;
			gchar* line;
			
			dialog = create_password_dialog(last_line);
			button = gnome_dialog_run(GNOME_DIALOG(dialog));
			switch(button) {
				case 0:
					passwd = gtk_entry_get_text(
						GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(dialog),
									"password_entry")));
					line = g_strconcat(passwd, "\n", NULL);
					launcher_send_ptyin(line);
					g_free(line);
					break;
				case 1:
					launcher_send_ptyin("<canceled>\n");
					launcher_reset();
					break;
				default:
			}
			gtk_widget_destroy(dialog);
		}
	}
}

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
#include <libzvt/libzvt.h>

#include "anjuta.h"
#include "resources.h"
#include "launcher.h"
#include "controls.h"
#include "global.h"
#include "pixmaps.h"

/* #define DEBUG */

#ifdef __FreeBSD__
#ifndef O_SYNC
#define O_SYNC 0
#endif /* O_SYNC */
#endif /* __FreeBSD__ */ 

static void to_terminal_child_terminated (GtkWidget* term, gpointer data);
static gint launcher_poll_inputs_on_idle (gpointer data);
static gboolean launcher_execution_done (gpointer data);
static void launcher_set_busy (gboolean flag);
static void launcher_send_ptyin (gchar * input_str);
static void launcher_pty_check_password(gchar* buffer);
static gboolean launcher_pty_check_child_exit_code (gchar* line);
static GtkWidget* create_password_dialog (gchar* prompt);
static void execution_done_cleanup ();

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
  gboolean pty_is_done;
  gboolean child_has_terminated;
  
/* Track if launcher_execution_done has been started */
  gboolean waiting_for_done;

/* Child's stdin, stdout and stderr pipes */
  gint stderr_pipe[2];
  gint stdout_pipe[2];
  gint stdin_pipe[2];

/* Gdk  Input Tags */
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
  gchar buffer[FILE_BUFFER_SIZE];

  n = read (launcher.stdout_pipe[0], buffer, FILE_BUFFER_SIZE-1);
  if (n > 0)			/*    There is output  */
  {
    buffer[n] = '\0';
    if (launcher.stdout_arrived)
      (*(launcher.stdout_arrived)) (buffer);
  }
  /* The pipe is closed on the other side */
  else if (n == 0)
  {
    launcher.stdout_is_done = TRUE;
  }
  /* Error - abort if not related to non blocking read or interrupted syscall */
  else if (errno != EAGAIN && errno != EINTR) {
    g_warning(_("launcher.c: error while reading child stdout - %s\n"), strerror(errno));
    launcher.stdout_is_done = TRUE;      
  }
}

static void
launcher_scan_error ()
{
  int n;
  gchar buffer[FILE_BUFFER_SIZE];

  n = read (launcher.stderr_pipe[0], buffer, FILE_BUFFER_SIZE-1);
  if (n > 0)			/*    There is stderr output  */
  {
    buffer[n] = '\0';
    if (launcher.stderr_arrived)
      (*(launcher.stderr_arrived)) (buffer);
  }
  else if (n == 0)			/* The pipe is closed on the other side */
  {
    #ifdef DEBUG
      g_warning("launcher_scan_error - EOF");
    #endif
    launcher.stderr_is_done = TRUE;
  }
  /* Error - abort if not related to non blocking read or interrupted syscall */
 else if (errno != EAGAIN && errno != EINTR) {
    g_warning(_("launcher.c: error while reading child stderr - %s\n"), strerror(errno));
    launcher.stderr_is_done = TRUE;      
  }
}

static void
launcher_scan_pty()
{
	if (launcher.terminal)
	{
		gint len;
		gchar* chars = NULL;
		
		vt_clear_selection ((ZVT_TERM (launcher.terminal))->vx);
		chars = zvt_term_get_buffer(ZVT_TERM(launcher.terminal),
		                            &len, VT_SELTYPE_LINE, -10000,
                                    0, 10000, 0);
		vt_clear_selection (ZVT_TERM (launcher.terminal)->vx);
		
		zvt_term_reset(ZVT_TERM(launcher.terminal), TRUE);
		launcher.char_pos = 1;
		if (chars && strlen(chars) > launcher.char_pos)
		{
			glong start, end;
			gchar *last_line;
#ifdef DEBUG
			g_print("Chars buffer = %s, len = %d", chars, len);
#endif
			end = strlen(chars)-1;
			while (end > 0 && chars[end] == '\n') end--;
			start = end;
			while (start > 0 && chars[start-1] != '\n') start--;

			if (end > start)
			{
				last_line = g_strndup(&chars[start], end-start+1);

#ifdef DEBUG
				g_print("Last line = %s", last_line);
#endif
				if (!launcher.child_has_terminated) /* TTimo: not sure what this check is really worth */
					launcher_pty_check_password(last_line);
				launcher.pty_is_done = launcher_pty_check_child_exit_code(last_line);
				g_free(last_line);
			}
			launcher.char_pos = strlen(chars);
		}
		if (chars) g_free(chars);
	}
};

/* call regularly scheduled by a gtk_timeout_add - stops upon first FALSE return value */
static gint launcher_poll_inputs_on_idle (gpointer data)
{
  if (launcher.stderr_is_done == FALSE) {
    launcher_scan_error ();
  }
  if (launcher.stdout_is_done == FALSE) {
    launcher_scan_output ();
  }
  if (launcher.pty_is_done == FALSE) {
	launcher_scan_pty();	
	if (launcher.child_has_terminated) {
		launcher.pty_is_done = TRUE;
	}
  }
  /* keep running me as long as there is at least one not done yet */
  return (!launcher.stderr_is_done || !launcher.stdout_is_done || !launcher.pty_is_done);
}

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

pid_t launcher_get_child_pid ()
{
  if (launcher_is_busy ())
    return launcher.child_pid;
  else
    return -1;
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
  launcher.child_status = 0;
  launcher.stdout_is_done = FALSE;
  launcher.stderr_is_done = FALSE;
  launcher.pty_is_done = FALSE;
  
  launcher.char_pos = 1;

  shell = gnome_util_user_shell();
  if (NULL == shell || '\0' == shell[0])
    shell = "sh";
  
  launcher.terminal = zvt_term_new ();
  zvt_term_set_size(ZVT_TERM (launcher.terminal), 100, 100);
  gtk_signal_connect (GTK_OBJECT (launcher.terminal), "child_died", 
  		GTK_SIGNAL_FUNC (to_terminal_child_terminated), NULL);

#ifdef LAUNCHER_DEBUG
  if (launcher.terminal) {
	  GtkWindow* win;
	  win = gtk_window_new(0);
	  gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(app->widgets.window));
	  gtk_container_add(GTK_CONTAINER(win), launcher.terminal);
	  gtk_widget_show_all(win);
  }
#endif
  
  launcher.child_pid = zvt_term_forkpty (ZVT_TERM (launcher.terminal), 
		ZVT_TERM_DO_UTMP_LOG | ZVT_TERM_DO_WTMP_LOG);
  if (launcher.child_pid == 0)
  {
    char *total_cmd;
	
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
  
	/* This is a quick hack to get the child's exit code */
	/* Don't complain!! */
	total_cmd = g_strconcat(command_str,
		"; echo -n \\(Child exit code: $?\\) > /dev/tty",
		NULL);
	execlp (shell, shell, "-c", total_cmd, NULL);
	g_error (_("Cannot execute command shell"));
  }
  
#ifdef DEBUG
  printf("zvt_term_forkpty %d\n", launcher.child_pid);
#endif  
  
  close (launcher.stderr_pipe[1]);
  close (launcher.stdout_pipe[1]);
  close (launcher.stdin_pipe[0]);

  /* on a fork error perform a cleanup and return */
  if (launcher.child_pid == -1)
  {
	execution_done_cleanup ();
	return;
  }
	  

/*
 *  Set pipes none blocking, so we can read big buffers
 *  in the callback without having to use FIONREAD
 *  to make sure the callback doesn't block.
 */
  if ((md = fcntl (launcher.stdout_pipe[0], F_GETFL)) != -1)
    fcntl (launcher.stdout_pipe[0], F_SETFL, O_NONBLOCK | md);
  if ((md = fcntl (launcher.stderr_pipe[0], F_GETFL)) != -1)
    fcntl (launcher.stderr_pipe[0], F_SETFL, O_NONBLOCK | md);

  launcher.poll_id = gtk_timeout_add (150, launcher_poll_inputs_on_idle, NULL);
  return TRUE;
}

static void
to_terminal_child_terminated (GtkWidget* term, gpointer data)
{
#ifdef DEBUG	
  printf("Terminal child terminated called\n");
#endif

  launcher.child_has_terminated = TRUE;
  if (!launcher.waiting_for_done) {
    gtk_timeout_add (50, launcher_execution_done, NULL);
    launcher.waiting_for_done = TRUE;
  }
#ifdef DEBUG    
  else {
    printf("WARNING: to_terminal_child_terminated - waiting_for_done == TRUE\n");
  }
#endif
}

/* monitors closure of stdout stderr and pty through a gtk_timeout_add setup */
static gboolean
launcher_execution_done (gpointer data)
{
#ifdef DEBUG	
  printf("launcher_execution_done %d %d %d\n", launcher.stdout_is_done ? 1 : 0, launcher.stderr_is_done ? 1 : 0, launcher.pty_is_done ? 1 : 0);
#endif

  if (launcher.stdout_is_done == FALSE ||
	  launcher.stderr_is_done == FALSE ||
	  launcher.pty_is_done == FALSE)
    return TRUE; 
    
  execution_done_cleanup ();
  
  /* Call this here, after set_busy(FALSE)so we are able to 
	 launch a new child from the terminate function.
	 (by clubfan 2002-04-07) */
#ifdef DEBUG
  g_print("Exit status: %d\n", launcher.child_status);
#endif
  if (launcher.child_terminated)
    (*(launcher.child_terminated)) (launcher.child_status, time (NULL) - launcher.start_time);
  
  return FALSE;
}

static void
execution_done_cleanup ()
{
  close (launcher.stdin_pipe[1]);
  close (launcher.stdout_pipe[0]);
  close (launcher.stderr_pipe[0]);
	
  zvt_term_closepty(ZVT_TERM(launcher.terminal));
  zvt_term_reset(ZVT_TERM(launcher.terminal), 1);
  gtk_widget_destroy(launcher.terminal);
  launcher.terminal = NULL;
  launcher_set_busy (FALSE);
  
  launcher.waiting_for_done = FALSE; /* After this, we can only return FALSE; */	
}

static gboolean
launcher_pty_check_child_exit_code (gchar* line)
{
	gboolean ret;
	gboolean exit_code;
	gchar* prompt = "(Child exit code: ";

#ifdef DEBUG
	g_print ("Child exit code called: %s\n", line);
#endif
	
	if (!line) return FALSE;
	if (strlen(line) <= (strlen(prompt)+1)) return FALSE;
	
	ret = FALSE;
	if (strncmp(line, prompt, strlen(prompt)) == 0) {
		gchar *ascii_str = g_strdup(&line[strlen(prompt)]);
		gchar *ptr = strstr(ascii_str, ")");
		if (ptr) {
			*ptr = '\0';
			exit_code = atoi(ascii_str);
			launcher.child_status = exit_code;
#ifdef DEBUG
			g_print ("Exit code: %d\n", exit_code);
#endif
                        /* TTimo: I've seen situations where to_terminal_child_died is not called
                                    so make sure we still monitor process exit here */
                        if (!launcher.waiting_for_done) {
                          gtk_timeout_add (50, launcher_execution_done, NULL);
                          launcher.waiting_for_done = TRUE;
                        }
                        #ifdef DEBUG    
                        else {
                          printf("launcher_pty_check_child_exit_code - waiting_for_done == TRUE\n");
                        }
                        #endif                  
			ret = TRUE;
		}
		g_free(ascii_str);
	}
	return ret;
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
			const gchar* passwd;
			gchar* line;
			
			dialog = create_password_dialog (last_line);
			button = gtk_dialog_run(GTK_DIALOG(dialog));
			switch(button) {
				case GTK_RESPONSE_OK:
					passwd = gtk_entry_get_text(
						GTK_ENTRY(g_object_get_data(G_OBJECT(dialog),
									"password_entry")));
					line = g_strconcat(passwd, "\n", NULL);
					launcher_send_ptyin(line);
					g_free(line);
					break;
				case GTK_RESPONSE_CANCEL:
					launcher_send_ptyin("<canceled>\n");
					launcher_reset();
					break;
				default:
					break;
			}
			gtk_widget_destroy(dialog);
		}
	}
}

/* Password dialog */
static GtkWidget*
create_password_dialog (gchar* prompt)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *box;
	GtkWidget *icon;
	GtkWidget *label;
	GtkWidget *entry;
	
	g_return_val_if_fail(prompt, NULL);
	
	dialog = gtk_dialog_new_with_buttons (prompt,
	                        GTK_WINDOW (app->widgets.window),
	                        GTK_DIALOG_DESTROY_WITH_PARENT,
	                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                        GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_wmclass (GTK_WINDOW (dialog), "launcher-password-prompt",
				"anjuta");
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_widget_show(hbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), hbox);
	
	icon = anjuta_res_get_image (ANJUTA_PIXMAP_PASSWORD);
	gtk_widget_show(icon);
	gtk_box_pack_start_defaults (GTK_BOX(hbox), icon);
	
	if (strlen(prompt) < 20) {
		box = gtk_hbox_new(FALSE, 5);
	} else {
		box = gtk_vbox_new(FALSE, 5);
	}
	gtk_widget_show(box);
	gtk_box_pack_start_defaults (GTK_BOX(hbox), box);
	
	label = gtk_label_new (_(prompt));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	
	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	
	gtk_widget_ref(entry);
	gtk_object_set_data_full(GTK_OBJECT(dialog), "password_entry",
				gtk_widget_ref(entry), (GDestroyNotify)gtk_widget_unref);
	gtk_widget_grab_focus(entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	
	return dialog;
}

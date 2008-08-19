/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-launcher.h
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
#ifndef __ANJUTA_LAUNCHER_H__
#define __ANJUTA_LAUNCHER_H__

#include <sys/types.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _AnjutaLauncher      AnjutaLauncher;
typedef struct _AnjutaLauncherClass AnjutaLauncherClass;
typedef struct _AnjutaLauncherPriv  AnjutaLauncherPriv;

#define ANJUTA_TYPE_LAUNCHER            (anjuta_launcher_get_type ())

#define ANJUTA_LAUNCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_LAUNCHER, AnjutaLauncher))
#define ANJUTA_LAUNCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_LAUNCHER, AnjutaLauncherClass))

#define ANJUTA_IS_LAUNCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_LAUNCHER))
#define ANJUTA_IS_LAUNCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_LAUNCHER))

typedef enum {
	ANJUTA_LAUNCHER_OUTPUT_STDOUT,
	ANJUTA_LAUNCHER_OUTPUT_STDERR,
	ANJUTA_LAUNCHER_OUTPUT_PTY
} AnjutaLauncherOutputType;

/**
* AnjutaLauncherOutputCallback:
* @launcher: a #AnjutaLauncher object
* @output_type: Type of the output
* @chars: Characters being outputed
* @user_data: User data passed back to the user
* 
* This callback is called when new characters arrive from the launcher
* execution.
*/
typedef void (*AnjutaLauncherOutputCallback) (AnjutaLauncher *launcher,
										  AnjutaLauncherOutputType output_type,
											  const gchar *chars,
											  gpointer user_data);

struct _AnjutaLauncher
{
    GObject parent;
	AnjutaLauncherPriv *priv;
};

struct _AnjutaLauncherClass
{
    GObjectClass parent_class;
	
	/* Signals */
	void (*child_exited) (AnjutaLauncher *launcher,
						  int child_pid, int exit_status,
						  gulong time_taken_in_seconds);
	void (*busy) (AnjutaLauncher *launcher, gboolean busy_flag);
};

GType anjuta_launcher_get_type (void);
AnjutaLauncher* anjuta_launcher_new (void);
gboolean anjuta_launcher_is_busy (AnjutaLauncher *launcher);
gboolean anjuta_launcher_execute (AnjutaLauncher *launcher,
								  const gchar *command_str,
								  AnjutaLauncherOutputCallback callback,
								  gpointer callback_data);
gboolean anjuta_launcher_execute_v (AnjutaLauncher *launcher,
									gchar *const argv[],
									gchar *const envp[],
									AnjutaLauncherOutputCallback callback,
									gpointer callback_data);
void anjuta_launcher_set_encoding (AnjutaLauncher *launcher,
									   const gchar *charset);

void anjuta_launcher_send_stdin (AnjutaLauncher *launcher,
								 const gchar *input_str);
void anjuta_launcher_send_stdin_eof (AnjutaLauncher *launcher);

void anjuta_launcher_send_ptyin (AnjutaLauncher *launcher,
								 const gchar *input_str);
pid_t anjuta_launcher_get_child_pid (AnjutaLauncher *launcher);
void anjuta_launcher_reset (AnjutaLauncher *launcher);
void anjuta_launcher_signal (AnjutaLauncher *launcher, int sig);
gboolean anjuta_launcher_set_buffered_output (AnjutaLauncher *launcher,
										  gboolean buffered);
gboolean anjuta_launcher_set_check_passwd_prompt (AnjutaLauncher *launcher,
											  gboolean check_passwd);
/* Returns old value */
gboolean anjuta_launcher_set_terminal_echo (AnjutaLauncher *launcher,
											gboolean echo_on);
gboolean anjuta_launcher_set_terminate_on_exit (AnjutaLauncher *launcher,
		gboolean terminate_on_exit);

G_END_DECLS

#endif				/* __ANJUTA_LAUNCHER_H__ */

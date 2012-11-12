/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>

#include <signal.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-terminal.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-terminal-plugin.xml"
#define PREFS_BUILDER PACKAGE_DATA_DIR"/glade/anjuta-terminal-plugin.ui"
#define ICON_FILE "anjuta-terminal-plugin-48.png"

/* Some desktop/gnome-terminal gconf keys. */
#define GCONF_MONOSPACE_FONT      "/desktop/gnome/interface/monospace_font_name"
#define GCONF_DEFAULT_PROFILE     "/apps/gnome-terminal/global/default_profile"
#define GCONF_PROFILE_LIST        "/apps/gnome-terminal/global/profile_list"

#define GCONF_PROFILE_PREFIX      "/apps/gnome-terminal/profiles"
#define GCONF_BACKGROUND_COLOR    "background_color"
#define GCONF_BACKSPACE_BINDING   "backspace_binding"
#define GCONF_CURSOR_BLINK        "cursor_blink"
#define GCONF_DELETE_BINDING      "delete_binding"
#define GCONF_EXIT_ACTION         "exit_action"
#define GCONF_VTE_TERMINAL_FONT   "font"
#define GCONF_FOREGROUND_COLOR    "foreground_color"
#define GCONF_SCROLLBACK_LINES    "scrollback_lines"
#define GCONF_SCROLL_ON_KEYSTROKE "scroll_on_keystroke"
#define GCONF_SCROLL_ON_OUTPUT    "scroll_on_output"
#define GCONF_SILENT_BELL         "silent_bell"
#define GCONF_USE_SYSTEM_FONT     "use_system_font"
#define GCONF_WORD_CHARS          "word_chars"
#define GCONF_PTY_FLAGS           "pty_flags"

#define PREF_SCHEMA                           "org.gnome.anjuta.terminal"
#define PREFS_TERMINAL_PROFILE_USE_DEFAULT    "terminal-default-profile"
#define PREFS_TERMINAL_PROFILE                "terminal-profile"

#include <vte/vte.h>
#include <pwd.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-plugin.h>

extern GType terminal_plugin_get_type (GTypeModule *module);
#define ANJUTA_PLUGIN_TERMINAL_TYPE         (terminal_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_TERMINAL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_PLUGIN_TERMINAL_TYPE, TerminalPlugin))
#define ANJUTA_PLUGIN_TERMINAL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_PLUGIN_TERMINAL_CLASS, TerminalPluginClass))
#define ANJUTA_IS_PLUGIN_TERMINAL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_PLUGIN_TERMINAL_TYPE))
#define ANJUTA_IS_PLUGIN_TERMINAL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_PLUGIN_TERMINAL_TYPE))
#define ANJUTA_PLUGIN_TERMINAL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_PLUGIN_TERMINAL_TYPE, TerminalPluginClass))

typedef struct _TerminalPlugin TerminalPlugin;
typedef struct _TerminalPluginClass TerminalPluginClass;

struct _TerminalPlugin{
	AnjutaPlugin parent;

	gint uiid;
	GtkActionGroup *action_group;

	GPid child_pid;
	GtkWidget *shell, *term;
	GtkWidget *shell_box, *term_box;
	GtkWidget *frame;
	GtkWidget *pref_profile_combo;
	GtkWidget *pref_default_button;
	gboolean   widget_added_to_shell;
	GSettings *settings;
	guint root_watch_id;
	gboolean pty_flags;
#if OLD_VTE == 1
	gboolean first_time_realization;
#endif
};

struct _TerminalPluginClass{
	AnjutaPluginClass parent_class;
};

static gpointer parent_class;

#if 0
static const gchar*
get_profile_key (const gchar *profile, const gchar *key)
{
	/* A resonably safe buffer */
	static gchar buffer[1024];

	g_return_val_if_fail (profile != NULL && key != NULL, NULL);

	snprintf (buffer, 1024, "%s/%s/%s", GCONF_PROFILE_PREFIX, profile, key);
	return buffer;
}
#endif

static void
terminal_set_preferences (VteTerminal *term, GSettings* settings, TerminalPlugin *term_plugin)
{
#if 0
	char *text;
	int value;
	gboolean setting;
	GdkColor color[2];
	GdkColor* foreground;
	GdkColor* background;
	gchar *profile;

	g_return_if_fail (settings != NULL);

	/* Update the currently available list of terminal profiles */
	setting = g_settings_get_boolean (settings,
	                                  PREFS_TERMINAL_PROFILE_USE_DEFAULT);
	if (setting)
	{
		// TODO: Get from GSettings instead of GConf
		/* Use the currently selected profile in gnome-terminal */
		text = gconf_client_get_string (client, GCONF_DEFAULT_PROFILE, NULL);
	}
	else
	{
		/* Otherwise use the user selected profile */
		text = g_settings_get_string (settings, PREFS_TERMINAL_PROFILE);
	}
	if (!text || (*text == '\0'))
			text = g_strdup ("Default");
	profile = text;

	vte_terminal_set_mouse_autohide (term, TRUE);

	/* Set terminal font either using the desktop wide font or g-t one. */
	setting = g_settings_get_boolean (GCONF_USE_SYSTEM_FONT);
	if (setting) {
		text = gconf_client_get_string (client, GCONF_MONOSPACE_FONT, NULL);
		if (!text)
			text = GET_PROFILE_STRING (GCONF_VTE_TERMINAL_FONT);
	} else {
		text = GET_PROFILE_STRING (GCONF_VTE_TERMINAL_FONT);
	}
	if (text)
		vte_terminal_set_font_from_string (term, text);
	g_free (text);

	setting = GET_PROFILE_BOOL (GCONF_CURSOR_BLINK);
	vte_terminal_set_cursor_blink_mode ((term),
	                                    setting ? VTE_CURSOR_BLINK_ON : VTE_CURSOR_BLINK_OFF);
	setting = GET_PROFILE_BOOL (GCONF_SILENT_BELL);
	vte_terminal_set_audible_bell (term, !setting);
	value = GET_PROFILE_INT (GCONF_SCROLLBACK_LINES);
	vte_terminal_set_scrollback_lines (term, (value == 0) ? 500 : value);
	setting = GET_PROFILE_BOOL (GCONF_SCROLL_ON_KEYSTROKE);
	vte_terminal_set_scroll_on_keystroke (term, setting);
	setting = GET_PROFILE_BOOL (GCONF_SCROLL_ON_OUTPUT);
	vte_terminal_set_scroll_on_output (term, TRUE);
	text = GET_PROFILE_STRING (GCONF_WORD_CHARS);
	if (text)
		vte_terminal_set_word_chars (term, text);
	g_free (text);

	text = GET_PROFILE_STRING (GCONF_BACKSPACE_BINDING);
	if (text)
	{
		if (!strcmp (text, "ascii-del"))
			vte_terminal_set_backspace_binding (term,
								VTE_ERASE_ASCII_DELETE);
		else if (!strcmp (text, "escape-sequence"))
			vte_terminal_set_backspace_binding (term,
								VTE_ERASE_DELETE_SEQUENCE);
		else if (!strcmp (text, "control-h"))
			vte_terminal_set_backspace_binding (term,
								VTE_ERASE_ASCII_BACKSPACE);
		else
			vte_terminal_set_backspace_binding (term,
								VTE_ERASE_AUTO);
		g_free (text);
	}
	text = GET_PROFILE_STRING (GCONF_DELETE_BINDING);
	if (text)
	{
		if (!strcmp (text, "ascii-del"))
			vte_terminal_set_delete_binding (term,
							 VTE_ERASE_ASCII_DELETE);
		else if (!strcmp (text, "escape-sequence"))
			vte_terminal_set_delete_binding (term,
							 VTE_ERASE_DELETE_SEQUENCE);
		else if (!strcmp (text, "control-h"))
			vte_terminal_set_delete_binding (term,
							 VTE_ERASE_ASCII_BACKSPACE);
		else
			vte_terminal_set_delete_binding (term,
							 VTE_ERASE_AUTO);
		g_free (text);
	}
	/* Set fore- and background colors. */
	text = GET_PROFILE_STRING (GCONF_BACKGROUND_COLOR);
	if (text)
	{
		gdk_color_parse (text, &color[0]);
		g_free (text);
	}
	background = text ? &color[0] : NULL;
	text = GET_PROFILE_STRING (GCONF_FOREGROUND_COLOR);
	if (text)
	{
		gdk_color_parse (text, &color[1]);
		g_free (text);
	}
	foreground = text ? &color[1] : NULL;
	/* vte_terminal_set_colors works even if the terminal widget is not realized
	 * which is not the case with vte_terminal_set_color_foreground and
	 * vte_terminal_set_color_background */
	vte_terminal_set_colors (term, foreground, background, NULL, 0);

	/* vte_terminal is not working depending on update_records setting at least
	 * on FreeBSD */
	term_plugin->pty_flags = GET_PROFILE_INT_DEFAULT (GCONF_PTY_FLAGS,VTE_PTY_DEFAULT);

	g_free (profile);
	g_object_unref (client);
#endif
}

static void
preferences_changed (GSettings* settings, TerminalPlugin *term)
{
	terminal_set_preferences (VTE_TERMINAL (term->shell), settings, term);
	terminal_set_preferences (VTE_TERMINAL (term->term), settings, term);
}

static void
on_notify_prefs_profile(GSettings* settings,
                        const gchar* key,
                        gpointer user_data)
{
	TerminalPlugin *tp = ANJUTA_PLUGIN_TERMINAL (user_data);
	preferences_changed (settings, tp);
}

static void
on_notify_prefs_default (GSettings* settings,
                         const gchar* key,
                         gpointer user_data)
{
	TerminalPlugin *tp = ANJUTA_PLUGIN_TERMINAL (user_data);
	preferences_changed (settings, tp);
}

static void
prefs_init (TerminalPlugin *tp)
{
	g_signal_connect (tp->settings, "changed::" PREFS_TERMINAL_PROFILE,
	                  G_CALLBACK (on_notify_prefs_profile), tp);
	g_signal_connect (tp->settings, "changed::" PREFS_TERMINAL_PROFILE_USE_DEFAULT,
	                  G_CALLBACK (on_notify_prefs_default), tp);
}

static void
use_default_profile_cb (GtkToggleButton *button,
						TerminalPlugin *term)
{
	if (gtk_toggle_button_get_active (button))
		gtk_widget_set_sensitive (term->pref_profile_combo, FALSE);
	else
		gtk_widget_set_sensitive (term->pref_profile_combo, TRUE);
}

static void
terminal_child_exited_cb (GPid pid, gint status, gpointer user_data)
{
	TerminalPlugin *term_plugin = ANJUTA_PLUGIN_TERMINAL (user_data);
	if (term_plugin->child_pid == pid)
	{
		gboolean focus;

		focus = gtk_widget_is_focus (term_plugin->term);

		gtk_container_remove (GTK_CONTAINER (term_plugin->frame), term_plugin->term_box);
		gtk_container_add (GTK_CONTAINER (term_plugin->frame), term_plugin->shell_box);
		gtk_widget_show_all (term_plugin->shell_box);
		if (focus)
			gtk_widget_grab_focus (term_plugin->shell);

		term_plugin->child_pid = 0;
	}

	g_signal_emit_by_name(term_plugin, "child-exited", pid, status);
	g_spawn_close_pid (pid);
}

static pid_t
terminal_execute (TerminalPlugin *term_plugin, const gchar *directory,
				  const gchar *command, gchar **environment)
{
	char **args, **args_ptr;
	GList *args_list, *args_list_ptr;
	gchar *dir;
	VteTerminal *term;
	GPid pid;

	g_return_val_if_fail (command != NULL, 0);

	/* Prepare command args */
	args_list = anjuta_util_parse_args_from_string (command);
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

	if (directory == NULL)
		dir = g_path_get_dirname (args[0]);
	else
		dir = g_strdup (directory);

	term = VTE_TERMINAL (term_plugin->term);

/*
	vte_terminal_reset (term, TRUE, TRUE);
*/

	if (vte_terminal_fork_command_full (term, term_plugin->pty_flags,
	                                    dir, args, environment,
	                                    G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL,
	                                    &pid, NULL))
	{
		gboolean focus;

		term_plugin->child_pid = pid;
		g_child_watch_add (pid, terminal_child_exited_cb, term_plugin);

		/* Display terminal widget */
		focus = gtk_widget_is_focus (term_plugin->shell);
		gtk_container_remove (GTK_CONTAINER (term_plugin->frame), term_plugin->shell_box);
		gtk_container_add (GTK_CONTAINER (term_plugin->frame), term_plugin->term_box);
		gtk_widget_show_all (term_plugin->term_box);
		if (focus)
			gtk_widget_grab_focus (term_plugin->term);

		if (term_plugin->widget_added_to_shell)
			anjuta_shell_present_widget (ANJUTA_PLUGIN (term_plugin)->shell,
									 term_plugin->frame, NULL);
	}

	g_free (dir);
	g_free (args);
	g_list_foreach (args_list, (GFunc)g_free, NULL);
	g_list_free (args_list);

	return pid;
}

static void
init_shell (TerminalPlugin *term_plugin, const char *path)
{
	gchar *shell[2] = {0, 0};
	static gboolean first_time = TRUE;
	VteTerminal *term = VTE_TERMINAL (term_plugin->shell);

	shell[0] = vte_get_user_shell ();
	if (shell[0] == NULL) shell[0] = g_strdup("/bin/sh");

	if (!first_time)
		vte_terminal_reset (term, FALSE, TRUE);
	else
		first_time = FALSE;

	vte_terminal_fork_command_full (term, term_plugin->pty_flags,
	                                path, shell, NULL,
	                                0, NULL, NULL,
	                                NULL, NULL);
	g_free (shell[0]);
}

static gboolean
terminal_focus_cb (GtkWidget *widget, GdkEvent  *event,
				   TerminalPlugin *term)
{
	gtk_widget_grab_focus (widget);
	return FALSE;
}

static gboolean
terminal_keypress_cb (GtkWidget *widget, GdkEventKey  *event,
						   TerminalPlugin *terminal_plugin)
{
	if (event->type != GDK_KEY_PRESS)
		return FALSE;

	/* ctrl-d */
	if ((event->keyval == GDK_KEY_d || event->keyval == GDK_KEY_D) &&
		(event->state & GDK_CONTROL_MASK))
	{
		if (terminal_plugin->child_pid)
		{
			kill (terminal_plugin->child_pid, SIGINT);
			terminal_plugin->child_pid = 0;
		}
		return TRUE;
	}
	return FALSE;
}

static gboolean
terminal_click_cb (GtkWidget *widget, GdkEventButton *event,
				   TerminalPlugin *term)
{
	if (event->button == 3)
	{
		AnjutaUI *ui;
		GtkMenu *popup;
		GtkAction *action;

		ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(term)->shell, NULL);
		popup =  GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupTerminal"));
		action = gtk_action_group_get_action (term->action_group, "ActionCopyFromTerminal");
		gtk_action_set_sensitive (action,vte_terminal_get_has_selection(VTE_TERMINAL(widget)));

		gtk_menu_popup (popup, NULL, NULL, NULL, NULL,
						event->button, event->time);
	}

	return FALSE;
}

#if OLD_VTE == 1
/* VTE has a terrible bug where it could crash when container is changed.
 * The problem has been traced in vte where its style-set handler does not
 * adequately check if the widget is realized, resulting in a crash when
 * style-set occurs in an unrealized vte widget.
 *
 * This work around blocks all style-set signal emissions when the vte
 * widget is unrealized. -Naba
 */
static void
terminal_realize_cb (GtkWidget *term, TerminalPlugin *plugin)
{
	gint count;

	if (plugin->first_time_realization)
	{
		/* First time realize does not have the signals blocked */
		plugin->first_time_realization = FALSE;
		return;
	}
	count = g_signal_handlers_unblock_matched (term,
											   G_SIGNAL_MATCH_DATA,
											   0,
											   g_quark_from_string ("style-set"),
											   NULL,
											   NULL,
											   NULL);
	DEBUG_PRINT ("Unlocked %d terminal signal", count);
}

static void
terminal_unrealize_cb (GtkWidget *term, TerminalPlugin *plugin)
{
	gint count;
	count = g_signal_handlers_block_matched (term,
											 G_SIGNAL_MATCH_DATA,
											 0,
											 g_quark_from_string ("style-set"),
											 NULL,
											 NULL,
											 NULL);
	DEBUG_PRINT ("Blocked %d terminal signal", count);
}
#endif

static void
on_terminal_copy_cb (GtkAction * action, TerminalPlugin *term_plugin)
{
	VteTerminal *term;

	if (term_plugin->child_pid)
		term = VTE_TERMINAL (term_plugin->term);
	else
		term = VTE_TERMINAL (term_plugin->shell);

	if (vte_terminal_get_has_selection(term))
		vte_terminal_copy_clipboard(term);
}

static void
on_terminal_paste_cb (GtkAction * action, TerminalPlugin *term_plugin)
{
	VteTerminal *term;

	if (term_plugin->child_pid)
		term = VTE_TERMINAL (term_plugin->term);
	else
		term = VTE_TERMINAL (term_plugin->shell);

	vte_terminal_paste_clipboard(term);
}

static void
on_terminal_command_cb (GtkAction * action, TerminalPlugin *term_plugin)
{
	VteTerminal *term;
	gchar c;

	if (term_plugin->child_pid)
		term = VTE_TERMINAL (term_plugin->term);
	else
		term = VTE_TERMINAL (term_plugin->shell);

	/* this only works for control + letter */
	c = gtk_action_get_name (action) [strlen (gtk_action_get_name (action)) - 1] - 64;

	vte_terminal_feed_child (term, &c, 1);
}

static GtkActionEntry actions_terminal[] = {
	{
		"ActionCopyFromTerminal", 	              /* Action name */
		GTK_STOCK_COPY,                           /* Stock icon, if any */
		N_("_Copy"),		                      /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		G_CALLBACK (on_terminal_copy_cb)          /* action callback */
	},
	{
		"ActionPasteInTerminal",
		GTK_STOCK_PASTE,
		N_("_Paste"),
		NULL,
		NULL,
		G_CALLBACK (on_terminal_paste_cb)
	},
	/* Add other control + letter commands here ending in -CTRL and then letter e.g. -CTRLT */
	{
		"ActionCommandToTerminal-CTRLC",
		NULL,
		N_("Ctrl-C"),
		NULL,
		NULL,
		G_CALLBACK (on_terminal_command_cb)
	},
	{
		"ActionCommandToTerminal-CTRLX",
		NULL,
		N_("Ctrl-X"),
		NULL,
		NULL,
		G_CALLBACK (on_terminal_command_cb)
	},
	{
		"ActionCommandToTerminal-CTRLZ",
		NULL,
		N_("Ctrl-Z"),
		NULL,
		NULL,
		G_CALLBACK (on_terminal_command_cb)
	},
};

static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name,
					   const GValue *value, gpointer user_data)
{
	TerminalPlugin *term_plugin;
	const gchar *root_uri;

	term_plugin = (TerminalPlugin *)plugin;

	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		GFile *file;
		char *path;

		file = g_file_new_for_uri (root_uri);
		path = g_file_get_path (file);

		init_shell (term_plugin, path);

		g_object_unref (file);
		g_free (path);
	}
}

static GtkWidget *
create_terminal (TerminalPlugin *term_plugin)
{
	GtkWidget *term;

	/* Create new terminal. */
	term = vte_terminal_new ();
	gtk_widget_set_size_request (GTK_WIDGET (term), 10, 10);
	vte_terminal_set_size (VTE_TERMINAL (term), 50, 1);

	g_signal_connect (G_OBJECT (term), "focus-in-event",
					  G_CALLBACK (terminal_focus_cb), term_plugin);

	g_signal_connect (G_OBJECT (term), "button-press-event",
					  G_CALLBACK (terminal_click_cb), term_plugin);

#if OLD_VTE == 1
	g_signal_connect (G_OBJECT (term), "realize",
					  G_CALLBACK (terminal_realize_cb), term_plugin);
	g_signal_connect (G_OBJECT (term), "unrealize",
					  G_CALLBACK (terminal_unrealize_cb), term_plugin);
#endif

	return term;
}

static GtkWidget *
create_box (GtkWidget *term)
{
	GtkWidget *sb, *hbox;

	sb = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT (vte_terminal_get_adjustment (VTE_TERMINAL (term))));
	gtk_widget_set_can_focus (sb, FALSE);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (hbox), term, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), sb, FALSE, TRUE, 0);

	g_object_ref_sink (hbox);

	return hbox;
}

static void
terminal_create (TerminalPlugin *term_plugin)
{
	GtkWidget *frame;

	g_return_if_fail(term_plugin != NULL);

	term_plugin->child_pid = 0;

	/* Create the terminals. */
	term_plugin->shell = create_terminal (term_plugin);
	term_plugin->shell_box = create_box (term_plugin->shell);

	term_plugin->term = create_terminal (term_plugin);
	term_plugin->term_box = create_box (term_plugin->term);

	/* key-press handler for ctrl-d "kill" */
	g_signal_connect (G_OBJECT (term_plugin->term), "key-press-event",
					  G_CALLBACK (terminal_keypress_cb), term_plugin);

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (frame), term_plugin->shell_box);
	gtk_widget_show_all (frame);

	term_plugin->frame = frame;

	init_shell (term_plugin, NULL);
}

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON (ICON_FILE, "terminal-plugin-icon");
	END_REGISTER_ICON;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	TerminalPlugin *term_plugin;
	static gboolean initialized = FALSE;
	AnjutaUI *ui;

	DEBUG_PRINT ("%s", "TerminalPlugin: Activating Terminal plugin ...");

	term_plugin = ANJUTA_PLUGIN_TERMINAL (plugin);
	term_plugin->widget_added_to_shell = FALSE;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	term_plugin->action_group = anjuta_ui_add_action_group_entries (ui,
										"ActionGroupTerminal",
										_("terminal operations"),
										actions_terminal,
										G_N_ELEMENTS (actions_terminal),
										GETTEXT_PACKAGE, TRUE, term_plugin);
	term_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

	terminal_create (term_plugin);

	if (!initialized)
	{
		register_stock_icons (plugin);
	}

	/* Setup prefs callbacks */
	prefs_init (term_plugin);

	/* Added widget in shell */
	anjuta_shell_add_widget (plugin->shell, term_plugin->frame,
							 "AnjutaTerminal", _("Terminal"),
							 "terminal-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);
	/* terminal_focus_cb (term_plugin->term, NULL, term_plugin); */
	term_plugin->widget_added_to_shell = TRUE;
	initialized = TRUE;

	/* Set all terminal preferences, at that time the terminal widget is
	 * not realized, a few vte functions are not working. Another
	 * possibility could be to call this when the widget is realized */
	preferences_changed (term_plugin->settings, term_plugin);

	/* set up project directory watch */
	term_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
														  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
														  on_project_root_added,
														  0, NULL);

	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	TerminalPlugin *term_plugin;
	AnjutaUI *ui;

	term_plugin = ANJUTA_PLUGIN_TERMINAL (plugin);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, term_plugin->uiid);
	if (term_plugin->action_group)
	{
		anjuta_ui_remove_action_group (ui, term_plugin->action_group);
		term_plugin->action_group = NULL;
	}

	/* terminal plugin widgets are destroyed as soon as it is removed */
	anjuta_shell_remove_widget (plugin->shell, term_plugin->frame, NULL);

	g_object_unref (term_plugin->shell_box);
	g_object_unref (term_plugin->term_box);

	/* remove watch */
	anjuta_plugin_remove_watch (plugin, term_plugin->root_watch_id, FALSE);

	term_plugin->child_pid = 0;

#if OLD_VTE == 1
	g_signal_handlers_disconnect_by_func (G_OBJECT (term_plugin->term),
			 G_CALLBACK (terminal_unrealize_cb), term_plugin);

	term_plugin->first_time_realization = TRUE;
#endif

	return TRUE;
}

static void
terminal_plugin_dispose (GObject *obj)
{
	TerminalPlugin *term_plugin = ANJUTA_PLUGIN_TERMINAL (obj);

	g_object_unref (term_plugin->settings);

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
terminal_plugin_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
terminal_plugin_instance_init (GObject *obj)
{
	TerminalPlugin *term_plugin = ANJUTA_PLUGIN_TERMINAL (obj);

	term_plugin->settings = g_settings_new (PREF_SCHEMA);
	term_plugin->child_pid = 0;
	term_plugin->pref_profile_combo = NULL;
	term_plugin->uiid = 0;
	term_plugin->action_group = NULL;
	term_plugin->pty_flags = VTE_PTY_DEFAULT;
#if OLD_VTE == 1
	plugin->first_time_realization = TRUE;
#endif
}

static void
terminal_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = terminal_plugin_dispose;
	klass->finalize = terminal_plugin_finalize;
}

static pid_t
iterminal_execute_command (IAnjutaTerminal *terminal,
						   const gchar *directory,
						   const gchar *command,
						   gchar **environment, GError **error)
{
	TerminalPlugin *plugin;
	pid_t pid;

	plugin = ANJUTA_PLUGIN_TERMINAL (terminal);

	pid = terminal_execute (plugin, directory, command, environment);
	if (pid <= 0)
	{
		g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, _("Unable to execute command"));
	}

	return pid;
}

static void
iterminal_iface_init(IAnjutaTerminalIface *iface)
{
	iface->execute_command = iterminal_execute_command;
}

#if 0
static void
on_add_string_in_store (gpointer data, gpointer user_data)
{
	GtkListStore* model = (GtkListStore *)user_data;
	GtkTreeIter iter;

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, (const gchar *)data, -1);
}

static void
on_concat_string (gpointer data, gpointer user_data)
{
	GString* str = (GString *)user_data;

	if (str->len != 0)
		g_string_append_c (str, ',');
	g_string_append (str, data);
}

static void
on_pref_profile_changed (GtkComboBox* combo, TerminalPlugin* term_plugin)
{
	GtkTreeModel* model = gtk_combo_box_get_model (combo);
	GtkTreeIter iter;
	gchar* text;

	gtk_combo_box_get_active_iter (combo,
	                               &iter);
	gtk_tree_model_get (model, &iter, 0, &text, -1);
	g_settings_set_string (term_plugin->settings,
	                       PREFS_TERMINAL_PROFILE,
	                       text);
	g_free (text);
}
#endif

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GError* error = NULL;

	/* Create the terminal preferences page */
	TerminalPlugin* term_plugin = ANJUTA_PLUGIN_TERMINAL (ipref);
	GtkBuilder *bxml = gtk_builder_new ();

	if (!gtk_builder_add_from_file (bxml, PREFS_BUILDER, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}


	anjuta_preferences_add_from_builder (anjuta_preferences_default (), bxml,
	                                     term_plugin->settings,
	                                     "Terminal", _("Terminal"), ICON_FILE);

	term_plugin->pref_profile_combo = GTK_WIDGET (gtk_builder_get_object (bxml, "profile_list_combo"));
	term_plugin->pref_default_button = GTK_WIDGET (gtk_builder_get_object (bxml, "preferences_toggle:bool:1:0:terminal-default-profile"));

#if 0
	GSList *profiles;

	/* FIXME: Update to GSettings */
	/* Update the currently available list of terminal profiles */
	client = gconf_client_get_default ();
	profiles = gconf_client_get_list (client, GCONF_PROFILE_LIST,
									  GCONF_VALUE_STRING, NULL);
	if (profiles)
	{
		GtkListStore *store;
		GString *default_value = g_string_new (NULL);

		store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (term_plugin->pref_profile_combo)));

		gtk_list_store_clear (store);
		g_slist_foreach (profiles, on_add_string_in_store, store);
		g_slist_foreach (profiles, on_concat_string, default_value);
		g_slist_foreach (profiles, (GFunc)g_free, NULL);
		g_slist_free (profiles);

		g_signal_connect (term_plugin->pref_profile_combo, "changed",
		                  G_CALLBACK (on_pref_profile_changed), term_plugin);

		use_default_profile_cb (GTK_TOGGLE_BUTTON (term_plugin->pref_default_button), term_plugin);
		g_signal_connect (G_OBJECT(term_plugin->pref_default_button), "toggled",
						  G_CALLBACK (use_default_profile_cb), term_plugin);

		g_string_free (default_value, TRUE);
	}
	else
#endif
	{
		/* No profile, perhaps GNOME Terminal is not installed,
		 * Remove selection */
		gtk_widget_set_sensitive (term_plugin->pref_profile_combo, FALSE);
		gtk_widget_set_sensitive (term_plugin->pref_default_button, FALSE);
	}

	g_object_unref (bxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	TerminalPlugin* term_plugin = ANJUTA_PLUGIN_TERMINAL (ipref);
	g_signal_handlers_disconnect_by_func(G_OBJECT(term_plugin->pref_default_button),
		G_CALLBACK (use_default_profile_cb), term_plugin);
	anjuta_preferences_remove_page(prefs, _("Terminal"));
	term_plugin->pref_profile_combo = NULL;
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (TerminalPlugin, terminal_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (iterminal, IANJUTA_TYPE_TERMINAL);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (TerminalPlugin, terminal_plugin);

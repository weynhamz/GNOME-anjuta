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

#include <signal.h>

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-terminal-plugin.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-terminal-plugin.glade"
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

#define PREFS_TERMINAL_PROFILE_USE_DEFAULT    "terminal.default.profile"
#define PREFS_TERMINAL_PROFILE                "terminal.profile"

#include <gconf/gconf-client.h>
#include <vte/vte.h>
#include <vte/reaper.h>
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
	
	AnjutaPreferences *prefs;
	pid_t child_pid;
	GtkWidget *term;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *scrollbar;
	GtkWidget *pref_profile_combo;
	GtkWidget *pref_default_button;
	gboolean   widget_added_to_shell;
	GList *gconf_notify_ids;
#if OLD_VTE == 1
	gboolean first_time_realization;
#endif
};

struct _TerminalPluginClass{
	AnjutaPluginClass parent_class;
};

static gpointer parent_class;

static const gchar*
get_profile_key (const gchar *profile, const gchar *key)
{
	/* A resonably safe buffer */
	static gchar buffer[1024];
	
	g_return_val_if_fail (profile != NULL && key != NULL, NULL);
	
	snprintf (buffer, 1024, "%s/%s/%s", GCONF_PROFILE_PREFIX, profile, key);
	return buffer;
}

#define GET_PROFILE_BOOL(key) \
			gconf_client_get_bool (client, \
								   get_profile_key (profile, key), \
								   NULL);
#define GET_PROFILE_INT(key) \
			gconf_client_get_int (client, \
								  get_profile_key (profile, key), \
								  NULL);
#define GET_PROFILE_STRING(key) \
			gconf_client_get_string (client, \
									 get_profile_key (profile, key), \
									 NULL);

static void
preferences_changed (AnjutaPreferences *prefs, TerminalPlugin *term)
{
	GConfClient *client;
	char *text;
	int value;
	gboolean setting;
	GdkColor color[2];
	GdkColor* foreground;
	GdkColor* background;
	GtkWidget *vte;
	gchar *profile;
	AnjutaPreferences *pref;
	
	pref = term->prefs;
	vte = term->term;
	client = gconf_client_get_default ();
	
	g_return_if_fail (client != NULL);
	
	/* Update the currently available list of terminal profiles */
	setting = anjuta_preferences_get_int (pref,
										  PREFS_TERMINAL_PROFILE_USE_DEFAULT);
	if (setting)
	{
		/* Use the currently selected profile in gnome-terminal */
		text = gconf_client_get_string (client, GCONF_DEFAULT_PROFILE, NULL);
	}
	else
	{
		/* Otherwise use the user selected profile */
		text = anjuta_preferences_get (pref, PREFS_TERMINAL_PROFILE);
	}
	if (!text || (*text == '\0')) 
			text = g_strdup ("Default");
	profile = text;
	
	vte_terminal_set_mouse_autohide (VTE_TERMINAL (vte), TRUE);

	/* Set terminal font either using the desktop wide font or g-t one. */
	setting = GET_PROFILE_BOOL (GCONF_USE_SYSTEM_FONT);
	if (setting) {
		text = gconf_client_get_string (client, GCONF_MONOSPACE_FONT, NULL);
		if (!text)
			text = GET_PROFILE_STRING (GCONF_VTE_TERMINAL_FONT);
	} else {
		text = GET_PROFILE_STRING (GCONF_VTE_TERMINAL_FONT);
	}
	vte_terminal_set_font_from_string (VTE_TERMINAL (vte), text);
	g_free (text);
	
	setting = GET_PROFILE_BOOL (GCONF_CURSOR_BLINK);
	vte_terminal_set_cursor_blinks (VTE_TERMINAL (vte), setting);
	setting = GET_PROFILE_BOOL (GCONF_SILENT_BELL);
	vte_terminal_set_audible_bell (VTE_TERMINAL (vte), !setting);
	value = GET_PROFILE_INT (GCONF_SCROLLBACK_LINES);
	vte_terminal_set_scrollback_lines (VTE_TERMINAL (vte), value);
	setting = GET_PROFILE_BOOL (GCONF_SCROLL_ON_KEYSTROKE);
	vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (vte), setting);
	setting = GET_PROFILE_BOOL (GCONF_SCROLL_ON_OUTPUT);
	vte_terminal_set_scroll_on_output (VTE_TERMINAL (vte), TRUE);
	text = GET_PROFILE_STRING (GCONF_WORD_CHARS);
	if (text)
		vte_terminal_set_word_chars (VTE_TERMINAL (vte), text);
	g_free (text);
	
	text = GET_PROFILE_STRING (GCONF_BACKSPACE_BINDING);
	if (text)
	{
		if (!strcmp (text, "ascii-del"))
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_ASCII_DELETE);
		else if (!strcmp (text, "escape-sequence"))
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_DELETE_SEQUENCE);
		else if (!strcmp (text, "control-h"))
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_ASCII_BACKSPACE);
		else
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_AUTO);
		g_free (text);
	}
	text = GET_PROFILE_STRING (GCONF_DELETE_BINDING);
	if (text)
	{
		if (!strcmp (text, "ascii-del"))
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_ASCII_DELETE);
		else if (!strcmp (text, "escape-sequence"))
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_DELETE_SEQUENCE);
		else if (!strcmp (text, "control-h"))
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_ASCII_BACKSPACE);
		else
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
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
	vte_terminal_set_colors (VTE_TERMINAL (vte), foreground, background, NULL, 0);
	g_free (profile);
	g_object_unref (client);
}

static void
on_gconf_notify_prefs (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	TerminalPlugin *tp = ANJUTA_PLUGIN_TERMINAL (user_data);
	preferences_changed (tp->prefs, tp);
}

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (tp->prefs, \
											   key, func, tp, NULL); \
	tp->gconf_notify_ids = g_list_prepend (tp->gconf_notify_ids, \
										   GUINT_TO_POINTER (notify_id));
static void
prefs_init (TerminalPlugin *tp)
{
	guint notify_id;
	REGISTER_NOTIFY (PREFS_TERMINAL_PROFILE, on_gconf_notify_prefs);
	REGISTER_NOTIFY (PREFS_TERMINAL_PROFILE_USE_DEFAULT, on_gconf_notify_prefs);
}

static void
prefs_finalize (TerminalPlugin *tp)
{
	GList *node;
	node = tp->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (tp->prefs,
										  GPOINTER_TO_UINT (node->data));
		node = g_list_next (node);
	}
	g_list_free (tp->gconf_notify_ids);
	tp->gconf_notify_ids = NULL;
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
terminal_child_exited_cb (VteReaper *vtereaper, gint pid, gint status, TerminalPlugin *term_plugin)
{
	g_signal_emit_by_name(term_plugin, "child-exited", pid, status);	
}

static pid_t
terminal_execute (TerminalPlugin *term_plugin, const gchar *directory,
				  const gchar *command, gchar **environment)
{
	char **args, **args_ptr;
	GList *args_list, *args_list_ptr;
	gchar *dir;
	VteTerminal *term;
	
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
	
	vte_terminal_reset (term, TRUE, TRUE);

	term_plugin->child_pid = vte_terminal_fork_command (term, args[0], args,
														environment, dir, 0, 0, 0);
	vte_reaper_add_child (term_plugin->child_pid);
	
	g_free (dir);
	g_free (args);
	g_list_foreach (args_list, (GFunc)g_free, NULL);
	g_list_free (args_list);
	
	if (term_plugin->widget_added_to_shell)
		anjuta_shell_present_widget (ANJUTA_PLUGIN (term_plugin)->shell,
									 term_plugin->frame, NULL);
	
	return term_plugin->child_pid;
}

static void
terminal_init_cb (GtkWidget *widget, TerminalPlugin *term_plugin)
{
	struct passwd *pw;
	const char *shell;
	const char *dir;
	
	pw = getpwuid (getuid ());
	if (pw) {
		shell = pw->pw_shell;
		dir = pw->pw_dir;
	} else {
		shell = "/bin/sh";
		dir = "/";
	}
	terminal_execute (term_plugin, dir, shell, NULL);
}

static gboolean
terminal_focus_cb (GtkWidget *widget, GdkEvent  *event,
				   TerminalPlugin *term) 
{
	if (term->child_pid == 0)
	{
		terminal_init_cb (widget, term);
	}
	gtk_widget_grab_focus (widget);
	return FALSE;
}

static gboolean
terminal_keypress_cb (GtkWidget *widget, GdkEventKey  *event,
					  TerminalPlugin *term) 
{
	/* Fixme: GDK_KEY_PRESS doesn't seem to be called for our keys */
	if (event->type != GDK_KEY_RELEASE)
		return FALSE;
	
	/* ctrl-d */
	if (event->keyval == GDK_d ||
		event->keyval == GDK_D)
	{
		/* Ctrl pressed */
		if (event->state & GDK_CONTROL_MASK)
		{
			kill (term->child_pid, SIGINT);
			term->child_pid = 0;
			terminal_init_cb (GTK_WIDGET (term->term), term);
			return TRUE;
		}
	}
	/* Shift-Insert */
	if ((event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert) &&
		 event->state & GDK_SHIFT_MASK)
	{
		vte_terminal_paste_clipboard(VTE_TERMINAL(term->term));
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
		gtk_action_set_sensitive (action,vte_terminal_get_has_selection(VTE_TERMINAL(term->term)));
		
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
terminal_destroy_cb (GtkWidget *widget, TerminalPlugin *term)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
							G_CALLBACK (terminal_init_cb), term);
}

static void
on_terminal_copy_cb (GtkAction * action, TerminalPlugin *term)
{
	if (vte_terminal_get_has_selection(VTE_TERMINAL(term->term)))
		vte_terminal_copy_clipboard(VTE_TERMINAL(term->term));
}

static void
on_terminal_paste_cb (GtkAction * action, TerminalPlugin *term)
{
	vte_terminal_paste_clipboard(VTE_TERMINAL(term->term));
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
	}
};

static void
terminal_create (TerminalPlugin *term_plugin)
{
	GtkWidget *sb, *frame, *hbox;
	
	g_return_if_fail(term_plugin != NULL);

	term_plugin->child_pid = 0;
	
	/* Create new terminal. */
	term_plugin->term = vte_terminal_new ();
	gtk_widget_set_size_request (GTK_WIDGET (term_plugin->term), 10, 10);
	vte_terminal_set_size (VTE_TERMINAL (term_plugin->term), 50, 1);
	
	g_signal_connect (G_OBJECT (term_plugin->term), "focus_in_event",
					  G_CALLBACK (terminal_focus_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "child-exited",
					  G_CALLBACK (terminal_init_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "destroy",
					  G_CALLBACK (terminal_destroy_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "key-press-event",
					  G_CALLBACK (terminal_keypress_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "button-press-event",
					  G_CALLBACK (terminal_click_cb), term_plugin);
#if OLD_VTE == 1
	g_signal_connect (G_OBJECT (term_plugin->term), "realize",
					  G_CALLBACK (terminal_realize_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "unrealize",
					  G_CALLBACK (terminal_unrealize_cb), term_plugin);
#endif
	g_signal_connect (vte_reaper_get(), "child-exited",
					  G_CALLBACK (terminal_child_exited_cb), term_plugin);
	
	sb = gtk_vscrollbar_new (GTK_ADJUSTMENT (VTE_TERMINAL (term_plugin->term)->adjustment));
	GTK_WIDGET_UNSET_FLAGS (sb, GTK_CAN_FOCUS);
	
	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start (GTK_BOX (hbox), term_plugin->term, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), sb, FALSE, TRUE, 0);
	gtk_widget_show_all (frame);
	
	term_plugin->scrollbar = sb;
	term_plugin->frame = frame;
	term_plugin->hbox = hbox;

	terminal_init_cb (GTK_WIDGET (term_plugin->term), term_plugin);
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
	
	DEBUG_PRINT ("TerminalPlugin: Activating Terminal plugin ...");
	
	term_plugin = ANJUTA_PLUGIN_TERMINAL (plugin);
	term_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
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
	preferences_changed (term_plugin->prefs, term_plugin);
	
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
	
	prefs_finalize (term_plugin);

#if OLD_VTE == 1
	g_signal_handlers_disconnect_by_func (G_OBJECT (term_plugin->term),
			 G_CALLBACK (terminal_unrealize_cb), term_plugin);
#endif

	/* terminal plugin widgets are destroyed as soon as it is removed */
	anjuta_shell_remove_widget (plugin->shell, term_plugin->frame, NULL);
	
	/*
	g_signal_handlers_disconnect_by_func (G_OBJECT (term_plugin->pref_default_button),
										  G_CALLBACK (use_default_profile_cb),
										  term_plugin);
	*/
	term_plugin->frame = NULL;
	term_plugin->term = NULL;
	term_plugin->scrollbar = NULL;
	term_plugin->hbox = NULL;
	term_plugin->child_pid = 0;
#if OLD_VTE == 1
	term_plugin->first_time_realization = TRUE;
#endif

	// terminal_finalize (term_plugin);
	return TRUE;
}

static void
terminal_plugin_dispose (GObject *obj)
{
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
	TerminalPlugin *plugin = ANJUTA_PLUGIN_TERMINAL (obj);
	plugin->gconf_notify_ids = NULL;
	plugin->child_pid = 0;
	plugin->pref_profile_combo = NULL;
	plugin->uiid = 0;
	plugin->action_group = NULL;
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
						   gchar **environment, GError **err)
{
	TerminalPlugin *plugin;
	const gchar *dir;
	
	plugin = ANJUTA_PLUGIN_TERMINAL (terminal);
	
	if (directory == NULL || strlen (directory) <= 0)
		dir = NULL;
	else
		dir = directory;
	
	return terminal_execute (plugin, directory, command, environment);
}

static void
iterminal_iface_init(IAnjutaTerminalIface *iface)
{
	iface->execute_command = iterminal_execute_command;
}

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
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GSList *profiles;
	GConfClient *client;	
	
	/* Create the terminal preferences page */
	TerminalPlugin* term_plugin = ANJUTA_PLUGIN_TERMINAL (ipref);
	GladeXML *gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog_terminal", NULL);
	anjuta_preferences_add_page (term_plugin->prefs, gxml,
									"Terminal", _("Terminal"), ICON_FILE);
	
	term_plugin->pref_profile_combo = glade_xml_get_widget (gxml, "profile_list_combo");
	term_plugin->pref_default_button = glade_xml_get_widget (gxml, "preferences_toggle:bool:1:0:terminal.default.profile");

	/* Update the currently available list of terminal profiles */
	client = gconf_client_get_default ();
	profiles = gconf_client_get_list (client, GCONF_PROFILE_LIST,
									  GCONF_VALUE_STRING, NULL);
	if (profiles)
	{
		GtkListStore *store;
		GString *default_value;
			
		default_value = g_string_new (NULL);
		store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (term_plugin->pref_profile_combo)));
		
		gtk_list_store_clear (store);
		g_slist_foreach (profiles, on_add_string_in_store, store);
		g_slist_foreach (profiles, on_concat_string, default_value);
		g_slist_foreach (profiles, (GFunc)g_free, NULL);
		g_slist_free (profiles);
		
		anjuta_preferences_register_property_raw (term_plugin->prefs,
												  term_plugin->pref_profile_combo,
												  PREFS_TERMINAL_PROFILE,
												  default_value->str,
												  1,
												  ANJUTA_PROPERTY_OBJECT_TYPE_COMBO,
												  ANJUTA_PROPERTY_DATA_TYPE_TEXT);
		g_string_free (default_value, TRUE);
		
		use_default_profile_cb (GTK_TOGGLE_BUTTON (term_plugin->pref_default_button), term_plugin);
		g_signal_connect (G_OBJECT(term_plugin->pref_default_button), "toggled",
						  G_CALLBACK (use_default_profile_cb), term_plugin);
	}
	else
	{
		/* No profile, perhaps GNOME Terminal is not installed,
		 * Remove selection */
		gtk_widget_set_sensitive (term_plugin->pref_profile_combo, FALSE);
		gtk_widget_set_sensitive (term_plugin->pref_default_button, FALSE);
	}
	
	g_object_unref (gxml);
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

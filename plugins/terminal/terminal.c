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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-children.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-terminal.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/plugins.h>

#include <signal.h>

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-terminal.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-terminal-plugin.glade"
#define ICON_FILE "preferences-terminal.png"

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
#include <pwd.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-plugin.h>

extern char **environ;

typedef struct _TerminalPlugin TerminalPlugin;
typedef struct _TerminalPluginClass TerminalPluginClass;

struct _TerminalPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	pid_t child_pid;
	GtkWidget *term;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *scrollbar;
	GtkWidget *pref_profile_combo;
	GtkWidget *pref_default_button;
	GList *gconf_notify_ids;
	gboolean child_initizlized;
	gboolean first_time_realization;
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
	GdkColor color;
	GtkWidget *vte;
	gchar *profile;
	AnjutaPreferences *pref;
	GSList *profiles;
	
	pref = term->prefs;
	vte = term->term;
	client = gconf_client_get_default ();
	
	g_return_if_fail (client != NULL);
	
	/* Update the currently available list of terminal profiles */
	profiles = gconf_client_get_list (client, GCONF_PROFILE_LIST,
											  GCONF_VALUE_STRING, NULL);
	if (profiles)
	{
		GList *list = NULL;
		GSList *node = profiles;
		while (node)
		{
			if (node->data)
				list = g_list_append (list, node->data);
			node = g_slist_next (node);
		}
		gtk_combo_set_popdown_strings (GTK_COMBO (term->pref_profile_combo), list);
		g_list_free (list);
	}
	setting = anjuta_preferences_get_int (pref, PREFS_TERMINAL_PROFILE_USE_DEFAULT);
	if (setting)
	{
		/* Use the currently selected profile in gnome-terminal */
		text = gconf_client_get_string (client, GCONF_DEFAULT_PROFILE, NULL);
		if (!text)
			text = "Default";
	}
	else
	{
		/* Otherwise use the user selected profile */
		text = anjuta_preferences_get (pref, PREFS_TERMINAL_PROFILE);
		if (!text)
			text = "Default";
	}
	profile = g_strdup (text);
	
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
	if (text && GTK_WIDGET (vte)->window)
		vte_terminal_set_font_from_string (VTE_TERMINAL (vte), text);

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
	}
	/* Set fore- and background colors. */
	text = GET_PROFILE_STRING (GCONF_BACKGROUND_COLOR);
	if (text)
	{
		gdk_color_parse (text, &color);
		vte_terminal_set_color_background (VTE_TERMINAL (vte), &color);
	}
	text = GET_PROFILE_STRING (GCONF_FOREGROUND_COLOR);
	if (text)
	{
		gdk_color_parse (text, &color);
		vte_terminal_set_color_foreground (VTE_TERMINAL (vte), &color);
		vte_terminal_set_color_bold (VTE_TERMINAL (vte), &color);
	}
	g_free (profile);
	g_object_unref (client);
}

static void
on_gconf_notify_prefs (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	TerminalPlugin *tp = (TerminalPlugin*)user_data;
	preferences_changed (tp->prefs, tp);
}

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (tp->prefs, \
											   key, func, tp, NULL); \
	tp->gconf_notify_ids = g_list_prepend (tp->gconf_notify_ids, \
										   (gpointer)(notify_id));
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
		anjuta_preferences_notify_remove (tp->prefs, (guint)node->data);
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

static char **
get_child_environment (void)
{
	/* code from gnome-terminal, sort of. */
	char **p;
	int i;
	char **retval;
#define EXTRA_ENV_VARS 6

	/* count env vars that are set */
	for (p = environ; *p; p++);

	i = p - environ;
	retval = g_new (char *, i + 1 + EXTRA_ENV_VARS);

	for (i = 0, p = environ; *p; p++) {
		/* Strip all these out, we'll replace some of them */
		if ((strncmp (*p, "COLUMNS=", 8) == 0) ||
		    (strncmp (*p, "LINES=", 6) == 0)   ||
		    (strncmp (*p, "TERM=", 5) == 0)    ||
		    (strncmp (*p, "GNOME_DESKTOP_ICON=", 19) == 0)) {
			/* nothing: do not copy */
		} else {
			retval[i] = g_strdup (*p);
			++i;
		}
	}

	retval[i] = g_strdup ("TERM=xterm"); /* FIXME configurable later? */
	++i;

	retval[i] = NULL;

	return retval;
}

static pid_t
terminal_execute (TerminalPlugin *term_plugin, const gchar *directory,
				  const gchar *command)
{
	char **env, **args, **args_ptr;
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

	env = get_child_environment ();
	
#if OLD_VTE == 1
	if (dir)
		chdir (dir);
	term_plugin->child_pid = vte_terminal_fork_command (term, args[0],
														args, env);
#else
	term_plugin->child_pid = vte_terminal_fork_command (term, args[0], args,
														env, dir, 0, 0, 0);
#endif
	g_free (dir);
	g_strfreev (env);
	g_free (args);
	g_list_foreach (args_list, (GFunc)g_free, NULL);
	g_list_free (args_list);
	
	/* The fork command above overwirtes our SIGCHLD signal handler.
	 * Restore it */
	anjuta_children_recover ();
	preferences_changed (term_plugin->prefs, term_plugin);
	
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
	terminal_execute (term_plugin, dir, shell);
}

static gboolean
terminal_focus_cb (GtkWidget *widget, GdkEvent  *event,
				   TerminalPlugin *term) 
{
	if (term->child_pid > 0)
		term->child_initizlized = TRUE;
	
	if (term->child_initizlized == FALSE)
	{
		terminal_init_cb (widget, term);
		term->child_initizlized = TRUE;
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

static void
terminal_destroy_cb (GtkWidget *widget, TerminalPlugin *term)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
							G_CALLBACK (terminal_init_cb), term);
}

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
	g_signal_connect (G_OBJECT (term_plugin->term), "event",
					  G_CALLBACK (terminal_keypress_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "realize",
					  G_CALLBACK (terminal_realize_cb), term_plugin);
	g_signal_connect (G_OBJECT (term_plugin->term), "unrealize",
					  G_CALLBACK (terminal_unrealize_cb), term_plugin);

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
}

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (PACKAGE_PIXMAPS_DIR"/"ICON_FILE, "terminal-plugin-icon");
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	TerminalPlugin *term_plugin;
	static gboolean initialized = FALSE;
	
	DEBUG_PRINT ("TerminalPlugin: Activating Terminal plugin ...");
	
	term_plugin = (TerminalPlugin*) plugin;
	term_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	term_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
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
	
	initialized = TRUE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	TerminalPlugin *term_plugin;
	term_plugin = (TerminalPlugin*) plugin;
	
	prefs_finalize (term_plugin);
	
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
	term_plugin->child_initizlized = FALSE;
	term_plugin->first_time_realization = TRUE;

	// terminal_finalize (term_plugin);
	return TRUE;
}

static void
terminal_plugin_dispose (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
terminal_plugin_finalize (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
terminal_plugin_instance_init (GObject *obj)
{
	TerminalPlugin *plugin = (TerminalPlugin*) obj;
	plugin->gconf_notify_ids = NULL;
	plugin->child_initizlized = FALSE;
	plugin->first_time_realization = TRUE;
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
						   const gchar *command, GError **err)
{
	TerminalPlugin *plugin;
	const gchar *dir;
	
	plugin = (TerminalPlugin*)G_OBJECT (terminal);
	
	if (directory == NULL || strlen (directory) <= 0)
		dir = NULL;
	else
		dir = directory;
	
	return terminal_execute (plugin, directory, command);
}

static void
iterminal_iface_init(IAnjutaTerminalIface *iface)
{
	iface->execute_command = iterminal_execute_command;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	/* Create the terminal preferences page */
	TerminalPlugin* term_plugin = (TerminalPlugin*) ipref;
	GladeXML *gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog_terminal", NULL);
	anjuta_preferences_add_page (term_plugin->prefs, gxml,
									"Terminal", _("Terminal"), ICON_FILE);
	term_plugin->pref_profile_combo = glade_xml_get_widget (gxml, "profile_list_combo");
	term_plugin->pref_default_button =
		glade_xml_get_widget (gxml,
						"preferences_toggle:bool:1:0:terminal.default.profile");
	g_signal_connect (G_OBJECT(term_plugin->pref_default_button), "toggled",
						  G_CALLBACK (use_default_profile_cb), term_plugin);
	g_object_unref (gxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	TerminalPlugin* term_plugin = (TerminalPlugin*) ipref;
	g_signal_handlers_disconnect_by_func(G_OBJECT(term_plugin->pref_default_button),
		G_CALLBACK (use_default_profile_cb), term_plugin);
	anjuta_preferences_dialog_remove_page(ANJUTA_PREFERENCES_DIALOG(prefs),
		_("Terminal"));
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

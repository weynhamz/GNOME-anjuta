/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * shell.c
 * Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/plugins.h>

#include "shell.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-shell.ui"

static void
shutdown (AnjutaTestShell *shell)
{
	anjuta_plugins_unload_all (ANJUTA_SHELL (shell));
	gtk_main_quit ();
}

static gint
on_delete_event (GtkWidget *window, GdkEvent *event, gpointer data)
{
	shutdown (ANJUTA_TEST_SHELL (window));
}

static void
on_exit_activate (GtkAction *action, AnjutaTestShell *shell)
{
	shutdown (shell);
}

static void
on_preferences_activate (GtkAction *action, AnjutaTestShell *shell)
{
	gtk_widget_show (GTK_WIDGET (shell->preferences));
}

static void
on_shortcuts_activate (GtkAction *action, AnjutaTestShell *shell)
{
	GtkWidget *win, *accel_editor;
	
	accel_editor = anjuta_ui_get_accel_editor (ANJUTA_UI (shell->ui));
	win = gtk_dialog_new_with_buttons (_("Anjuta Plugins"), GTK_WINDOW (shell),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CLOSE, GTK_STOCK_CANCEL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG(win)->vbox), accel_editor);
	gtk_window_set_default_size (GTK_WINDOW (win), 500, 400);
	gtk_dialog_run (GTK_DIALOG (win));
	gtk_widget_destroy (win);
}

static GtkActionEntry action_entries[] = {
	{ "ActionMenuFile", NULL, N_("_File")},
	{ "ActionMenuEdit", NULL, N_("_Edit")},
	{ "ActionMenuView", NULL, N_("_View")},
	{ "ActionMenuSettings", NULL, N_("_Settings")},
	{ "ActionMenuHelp", NULL, N_("_Help")},
	{
		"ActionExit",
		GTK_STOCK_QUIT,
		N_("_Quit"),
		"<control>q",
		N_("Quit Anjuta Test Shell"),
	    G_CALLBACK (on_exit_activate)
	},
	{
		"ActionSettingsPreferences",
		GTK_STOCK_PROPERTIES, 
		N_("_Preferences ..."),
		NULL,
		N_("Preferences"),
		G_CALLBACK (on_preferences_activate)
	},
	{
		"ActionSettingsShortcuts",
		NULL,
		N_("C_ustomize shortcuts"),
		NULL,
		N_("Customize shortcuts associated with menu items"),
		G_CALLBACK (on_shortcuts_activate)
	}
};

static gpointer parent_class = NULL;

static void
on_add_merge_widget (GtkUIManager *merge, GtkWidget *widget,
					 GtkWidget *ui_container)
{
	DEBUG_PRINT ("Adding UI item ...");
	gtk_box_pack_start (GTK_BOX ((ANJUTA_TEST_SHELL (ui_container))->box),
						 widget, FALSE, FALSE, 0);
}

GtkWidget *
anjuta_test_shell_new (void)
{
	AnjutaTestShell *shell;

	shell = ANJUTA_TEST_SHELL (g_object_new (ANJUTA_TYPE_TEST_SHELL, 
									  "title", "Anjuta Shell",
									  NULL));
	return GTK_WIDGET (shell);
}

static void
anjuta_test_shell_instance_init (AnjutaTestShell *shell)
{
	GtkWidget *plugins;
	
	shell->values = g_hash_table_new (g_str_hash, g_str_equal);
	shell->widgets = g_hash_table_new (g_str_hash, g_str_equal);
	
	shell->box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (shell->box);
	gtk_container_add (GTK_CONTAINER (shell), shell->box);
	
	/* Status bar */
	shell->status = ANJUTA_STATUS (anjuta_status_new ());
	gtk_widget_show (GTK_WIDGET (shell->status));
	gtk_box_pack_end (GTK_BOX (shell->box), GTK_WIDGET (shell->status),
					  FALSE, FALSE, 0);
	
	plugins = anjuta_plugins_get_installed_dialog (ANJUTA_SHELL (shell));
	gtk_box_pack_end_defaults (GTK_BOX (shell->box), plugins);
	
	/* Preferencesnces */
	shell->preferences = ANJUTA_PREFERENCES (anjuta_preferences_new ());
	
	/* UI engine */
	shell->ui = anjuta_ui_new ();
	gtk_window_add_accel_group (GTK_WINDOW (shell),
								anjuta_ui_get_accel_group (shell->ui));
	g_signal_connect (G_OBJECT (shell->ui),
					  "add_widget", G_CALLBACK (on_add_merge_widget),
					  shell);
	
	gtk_window_add_accel_group (GTK_WINDOW (shell),
								anjuta_ui_get_accel_group (shell->ui));
	/* Register actions */
	anjuta_ui_add_action_group_entries (shell->ui, "ActionGroupTestShell",
										_("Test shell action group"),
										action_entries,
										G_N_ELEMENTS (action_entries),
										shell);
	/* Merge UI */
	shell->merge_id = anjuta_ui_merge (shell->ui, UI_FILE);

	gtk_window_set_default_size (GTK_WINDOW (shell), 300, 400);
}

static void
anjuta_test_shell_add_value (AnjutaShell *shell,
							 const char *name,
							 const GValue *value,
							 GError **error)
{
	GValue *copy;
	AnjutaTestShell *window = ANJUTA_TEST_SHELL (shell);

	anjuta_shell_remove_value (shell, name, error);
	
	copy = g_new0 (GValue, 1);
	g_value_init (copy, value->g_type);
	g_value_copy (value, copy);

	g_hash_table_insert (window->values, g_strdup (name), copy);
	g_signal_emit_by_name (shell, "value_added", name, copy);
}

static void
anjuta_test_shell_get_value (AnjutaShell *shell,
							 const char *name,
							 GValue *value,
							 GError **error)
{
	GValue *val;
	AnjutaTestShell *s = ANJUTA_TEST_SHELL (shell);
	
	val = g_hash_table_lookup (s->values, name);
	
	if (val) {
		if (!value->g_type) {
			g_value_init (value, val->g_type);
		}
		g_value_copy (val, value);
	} else {
		if (error) {
			*error = g_error_new (ANJUTA_SHELL_ERROR,
								  ANJUTA_SHELL_ERROR_DOESNT_EXIST,
								  _("Value doesn't exist"));
		}
	}
}

static void 
anjuta_test_shell_add_widget (AnjutaShell *shell, 
							   GtkWidget *w, 
							   const char *name,
							   const char *title,
							   const char *stock_id,
							   AnjutaShellPlacement placement,
							   GError **error)
{
	GtkWidget *client_win;
	AnjutaTestShell *window = ANJUTA_TEST_SHELL (shell);

	g_return_if_fail (w != NULL);

	// anjuta_shell_add (shell, name, G_TYPE_FROM_INSTANCE (w), w, NULL);

	g_hash_table_insert (window->widgets, g_strdup (name), w);
	client_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_add (GTK_CONTAINER (client_win), w);
	gtk_widget_show_all (client_win);
}

static gboolean
remove_from_widgets_hash (gpointer key, gpointer value, gpointer data)
{
	if (value == data) {
		// AnjutaShell *shell;
		
		// shell = g_object_get_data (G_OBJECT (value), "__temp_shell__");
		// anjuta_shell_remove_value (shell, key, NULL);
		g_free (key);
		return TRUE;
	}
	return FALSE;
}

static void 
anjuta_test_shell_remove_widget (AnjutaShell *shell, 
								  GtkWidget *w, 
								  GError **error)
{
	gint old_size;
	GtkWidget *client_win;
	AnjutaTestShell *window = ANJUTA_TEST_SHELL (shell);
	g_return_if_fail (w != NULL);

	// old_size = g_hash_table_size (window->widgets);
	// g_object_set_data (G_OBJECT (w), "__temp_shell__", shell);
	g_hash_table_foreach_steal (window->widgets, remove_from_widgets_hash, w);
	// if (old_size != g_hash_table_size (window->widgets))
	// {
		client_win = gtk_widget_get_toplevel (w);
		gtk_container_remove (GTK_CONTAINER (client_win), w);
		gtk_widget_destroy (client_win);
	//}
}

static void 
anjuta_test_shell_present_widget (AnjutaShell *shell, 
								  GtkWidget *w, 
								  GError **error)
{
	GtkWidget *client_win;
	AnjutaTestShell *window = ANJUTA_TEST_SHELL (shell);
	
	g_return_if_fail (w != NULL);

	client_win = gtk_widget_get_toplevel (w);
	gtk_window_present (GTK_WINDOW (client_win));
}

static void
anjuta_test_shell_remove_value (AnjutaShell *shell, 
								const char *name, 
								GError **error)
{
	GValue *value;
	GtkWidget *w;
	char *key;
	AnjutaTestShell *window = ANJUTA_TEST_SHELL (shell);
	
	if (g_hash_table_lookup_extended (window->widgets, name, 
					  (gpointer*)&key, (gpointer*)&w)) {
		GtkWidget *client_win;
		client_win = gtk_widget_get_toplevel (w);
		gtk_container_remove (GTK_CONTAINER (client_win), w);
		gtk_widget_destroy (client_win);
		g_free (key);
	}
	
	if (g_hash_table_lookup_extended (window->values, name, 
					  (gpointer*)&key, (gpointer*)&value)) {
		g_hash_table_remove (window->values, name);
		g_signal_emit_by_name (window, "value_removed", name);
		g_value_unset (value);
		g_free (value);
	}
}

static GObject*
anjuta_test_shell_get_object  (AnjutaShell *shell, const char *iface_name,
					    GError **error)
{
	return anjuta_plugins_get_plugin (shell, iface_name);
}

static AnjutaStatus*
anjuta_test_shell_get_status (AnjutaShell *shell, GError **error)
{
	return ANJUTA_TEST_SHELL (shell)->status;
}

static AnjutaUI *
anjuta_test_shell_get_ui  (AnjutaShell *shell, GError **error)
{
	return ANJUTA_TEST_SHELL (shell)->ui;
}

static AnjutaPreferences *
anjuta_test_shell_get_preferences  (AnjutaShell *shell, GError **error)
{
	return ANJUTA_TEST_SHELL (shell)->preferences;
}

static void
anjuta_test_shell_dispose (GObject *widget)
{
	/* FIXME */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (widget));
}

static void
anjuta_test_shell_finalize (GObject *widget)
{
	AnjutaTestShell *shell = ANJUTA_TEST_SHELL (widget);
	
	g_hash_table_destroy (shell->values);
	g_hash_table_destroy (shell->widgets);
	/* FIXME */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (widget));
}

static void
anjuta_test_shell_class_init (AnjutaTestShellClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	object_class->dispose = anjuta_test_shell_dispose;
	object_class->finalize = anjuta_test_shell_finalize;
}

static void
anjuta_shell_iface_init (AnjutaShellIface *iface)
{
	iface->add_widget = anjuta_test_shell_add_widget;
	iface->remove_widget = anjuta_test_shell_remove_widget;
	iface->present_widget = anjuta_test_shell_present_widget;
	iface->add_value = anjuta_test_shell_add_value;
	iface->get_value = anjuta_test_shell_get_value;
	iface->remove_value = anjuta_test_shell_remove_value;
	iface->get_object = anjuta_test_shell_get_object;
	iface->get_status = anjuta_test_shell_get_status;
	iface->get_ui = anjuta_test_shell_get_ui;
	iface->get_preferences = anjuta_test_shell_get_preferences;
}

ANJUTA_TYPE_BEGIN(AnjutaTestShell, anjuta_test_shell, GTK_TYPE_WINDOW);
ANJUTA_TYPE_ADD_INTERFACE(anjuta_shell, ANJUTA_TYPE_SHELL);
ANJUTA_TYPE_END;

int
main (int argc, char *argv[])
{
	GtkWidget *shell;
	GnomeProgram *program;
	gchar *data_dir;
	GList *plugins_dirs = NULL;
	GList* command_args;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain (PACKAGE);
#endif
	
	data_dir = g_strdup (PACKAGE_DATA_DIR);
	data_dir[strlen (data_dir) - strlen (PACKAGE) - 1] = '\0';
	
	/* Initialize gnome program */
	program = gnome_program_init ("Anjuta Test Shell", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_POPT_TABLE, NULL,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
		            _("Anjuta test shell"),
			    GNOME_PARAM_APP_DATADIR, data_dir,
			    NULL);
	g_free (data_dir);
	
	/* Initialize plugins */
	plugins_dirs = g_list_prepend (plugins_dirs, PACKAGE_PLUGIN_DIR);
	anjuta_plugins_init (plugins_dirs);

	shell = anjuta_test_shell_new ();
	g_signal_connect (G_OBJECT (shell), "delete-event",
					  G_CALLBACK (on_delete_event), NULL);
	gtk_widget_show_all (GTK_WIDGET (shell));
	gtk_main();
	anjuta_plugins_finalize ();
	return 0;
}

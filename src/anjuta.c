/*
 * anjuta.c Copyright (C) 2000 Naba Kumar
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

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libegg/menu/egg-toggle-action.h>
#include <gdl/gdl-dock.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/pixmaps.h>

#include "text_editor.h"
#include "fileselection.h"
#include "utilities.h"
#include "launcher.h"
#include "debugger.h"
#include "controls.h"
#include "project_dbase.h"
#include "anjuta_info.h"
#include "tm_tagmanager.h"
#include "file_history.h"
#include "anjuta-plugins.h"
#include "anjuta-tools.h"
#include "start-with.h"
#include "anjuta-encodings.h"
#include "anjuta.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "properties.h"
#include "commands.h"
/*-------------------------------------------------------------------*/
extern gboolean closing_state;
static GdkCursor *app_cursor;
/*-------------------------------------------------------------------*/
void anjuta_child_terminated (int t);
static void anjuta_show_text_editor (TextEditor * te);
static void anjuta_apply_preferences (AnjutaPreferences *pr, AnjutaApp *app);

/*-------------------------------------------------------------------*/

static void
update_ui_from_launcher (AnjutaLauncher *launcher, gboolean busy_flag)
{
	main_toolbar_update ();
	extended_toolbar_update ();
}

static gboolean anjuta_app_save_layout_to_file (AnjutaApp *app);

gpointer parent_class;

typedef struct _AnjutaWindowState AnjutaWindowState;
struct _AnjutaWindowState {
    gint     width;
    gint     height;
    gboolean maximized;
};

/* ------------ Window state functions */

static void
load_state (GtkWindow *window)
{
#if 0
	GConfClient       *gconf_client;
	AnjutaWindowState *state;
	
	state = g_object_get_data (G_OBJECT (window), "window_state");
	if (!state) {
		state = g_new0 (AnjutaWindowState, 1);
		g_object_set_data (G_OBJECT (window), "window_state", state);
	}

	/* restore window state */
	gconf_client = gconf_client_get_default ();
	state->width = gconf_client_get_int (gconf_client,
					     ANJUTA_WINDOW_STATE_WIDTH_KEY,
					     NULL);
	state->height = gconf_client_get_int (gconf_client,
					      ANJUTA_WINDOW_STATE_HEIGHT_KEY,
					      NULL);
	state->maximized = gconf_client_get_bool (gconf_client,
						  ANJUTA_WINDOW_STATE_MAXIMIZED_KEY,
						  NULL);
	gtk_window_set_default_size (GTK_WINDOW (window), state->width, state->height);
	if (state->maximized)
		gtk_window_maximize (GTK_WINDOW (window));

	g_object_unref (gconf_client);
#endif
}

static void
save_state (GtkWindow *window)
{
#if 0
	GConfClient       *gconf_client;
	AnjutaWindowState *state;
	
	state = g_object_get_data (G_OBJECT (window), "window_state");
	if (!state)
		return;
	
	/* save the window state */
	gconf_client = gconf_client_get_default ();
	gconf_client_set_int (gconf_client,
			      ANJUTA_WINDOW_STATE_HEIGHT_KEY,
			      state->height,
			      NULL);
	gconf_client_set_int (gconf_client,
			      ANJUTA_WINDOW_STATE_WIDTH_KEY,
			      state->width,
			      NULL);
	gconf_client_set_bool (gconf_client,
			       ANJUTA_WINDOW_STATE_MAXIMIZED_KEY,
			       state->maximized,
			       NULL);
	g_object_unref (gconf_client);
#endif
}

static gboolean
anjuta_window_state_cb (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   user_data)
{
	AnjutaWindowState *state;

	g_return_val_if_fail (widget != NULL, FALSE);

	state = g_object_get_data (G_OBJECT (widget), "window_state");
	if (!state) {
		state = g_new0 (AnjutaWindowState, 1);
		g_object_set_data (G_OBJECT (widget), "window_state", state);
	}

	switch (event->type) {
	    case GDK_WINDOW_STATE:
		    state->maximized = event->window_state.new_window_state &
			    GDK_WINDOW_STATE_MAXIMIZED;
		    break;
	    case GDK_CONFIGURE:
		    if (!state->maximized) {
			    state->width = event->configure.width;
			    state->height = event->configure.height;
		    }
		    break;
	    default:
		    break;
	}
	return FALSE;
}

static void 
layout_dirty_notify (GObject    *object,
		     GParamSpec *pspec,
		     gpointer    user_data)
{
	if (!strcmp (pspec->name, "dirty")) {
		gboolean dirty;
		g_object_get (object, "dirty", &dirty, NULL);
		if (dirty) {
			/* user_data is the AnjutaApp */
			g_idle_add (
				(GSourceFunc) anjuta_app_save_layout_to_file,
				user_data);
			
		}
	}
}

GtkWidget *
anjuta_app_new (void)
{
	AnjutaApp *app;

	app = ANJUTA_APP (g_object_new (ANJUTA_TYPE_APP, 
					      "win_name", "Anjuta",
					      "title", "Anjuta",
					      NULL));

	return GTK_WIDGET (app);
}

static void
anjuta_app_instance_init (AnjutaApp *appl)
{
	/* Very bad hack */
	app = appl;
	
	g_message ("Initializing Anjuta...");
	app->layout_manager = NULL;
	app->values = g_hash_table_new (g_str_hash, g_str_equal);
	app->widgets = g_hash_table_new (g_str_hash, g_str_equal);

	/* configure dock */
	app->dock = gdl_dock_new ();
	gnome_app_set_contents (GNOME_APP (app), app->dock);
	gtk_widget_show (app->dock);

	/* create placeholders for default widget positions (since an
	   initial host is provided they are automatically bound) */
	gdl_dock_placeholder_new ("ph_top", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_TOP, FALSE);
	gdl_dock_placeholder_new ("ph_bottom", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_BOTTOM, FALSE);
	gdl_dock_placeholder_new ("ph_left", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_LEFT, FALSE);
	gdl_dock_placeholder_new ("ph_right", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_RIGHT, FALSE);

	/* window state tracking */
	g_signal_connect (app, "window-state-event",
			  G_CALLBACK (anjuta_window_state_cb), NULL);
	g_signal_connect (app, "configure-event",
			  G_CALLBACK (anjuta_window_state_cb), NULL);
	load_state (GTK_WINDOW (app));

	gtk_widget_realize (GTK_WIDGET(app));
	
	gtk_widget_queue_draw (GTK_WIDGET(app));
	gtk_widget_queue_resize (GTK_WIDGET(app));

	{
		/* Must declare static, because it will be used forever */
		static FileSelData fsd1 = { N_("Open File"), NULL,
			on_open_filesel_ok_clicked,
			NULL, NULL
		};

		/* Must declare static, because it will be used forever */
		static FileSelData fsd2 = { N_("Save File As"), NULL,
			on_save_as_filesel_ok_clicked,
			on_save_as_filesel_cancel_clicked, NULL
		};
		/* Must declare static, because it will be used forever */
		static FileSelData fsd3 = { N_("Save File As"), NULL,
			on_build_msg_save_ok_clicked,
			on_build_msg_save_cancel_clicked, NULL
		};
		app->size = sizeof(AnjutaApp);
		app->addIns_list	= NULL ;
		app->shutdown_in_progress = FALSE;
		app->registered_windows = NULL;
		app->registered_child_processes = NULL;
		app->registered_child_processes_cb = NULL;
		app->registered_child_processes_cb_data = NULL;
		app->text_editor_list = NULL;
		app->current_text_editor = NULL;
		app->cur_job = NULL;
		app->recent_files = NULL;
		app->recent_projects = NULL;
		
		app->win_pos_x = 10;
		app->win_pos_y = 10;
		app->win_width = gdk_screen_width () - 10;
		app->win_height = gdk_screen_height () - 25;
		app->win_width = (app->win_width < 790)? app->win_width : 790;
		app->win_height = (app->win_height < 575)? app->win_width : 575;
		
		app->in_progress = FALSE;
		app->has_devhelp = anjuta_is_installed ("devhelp", FALSE);
		app->auto_gtk_update = TRUE;
		app->busy_count = 0;
		app->execution_dir = NULL;
		app->first_time_expose = TRUE;
		app->last_open_project	= NULL ;

		app->icon_set = gdl_icons_new(24, 16.0);
		create_anjuta_gui (app);

		app->dirs = anjuta_dirs_new ();
		app->fileselection = create_fileselection_gui (&fsd1);
		
		/* Set to the current dir */
		/* Spends too much time */
		/* getcwd(wd, PATH_MAX);
		fileselection_set_dir (app->fileselection, wd); */
		
		app->b_reload_last_project	= TRUE ;
		
		/* Preferencesnces */
		app->preferences = ANJUTA_PREFERENCES (anjuta_preferences_new ());
		anjuta_preferences_initialize (app->preferences);
		gtk_window_set_transient_for (GTK_WINDOW (app->preferences),
									  GTK_WINDOW (app));
		gtk_window_add_accel_group (GTK_WINDOW (app->preferences),
									app->accel_group);
		g_signal_connect (G_OBJECT (app->preferences), "changed",
						  G_CALLBACK (anjuta_apply_preferences), app);
		
		/* Editor encodings */
		anjuta_encodings_init (app->preferences);
		
		app->save_as_fileselection = create_fileselection_gui (&fsd2);
		gtk_window_set_modal ((GtkWindow *) app->save_as_fileselection, TRUE);
		app->save_as_build_msg_sel = create_fileselection_gui (&fsd3);
		app->compiler_options =	compiler_options_new (app->preferences->props);
		app->src_paths = src_paths_new ();
		app->messages = AN_MESSAGE_MANAGER (an_message_manager_new ());
		create_default_types (app->messages);
		app->project_dbase = project_dbase_new (app->preferences->props);
		app->configurer = configurer_new (app->project_dbase->props);
		app->executer = executer_new (app->project_dbase->props);
		app->command_editor =
			command_editor_new (app->preferences->props_global,
								app->preferences->props_local,
								app->project_dbase->props);
		app->tm_workspace = tm_get_workspace();
		if (TRUE != tm_workspace_load_global_tags(PACKAGE_DATA_DIR "/system.tags"))
			g_warning("Unable to load global tag file");
		app->help_system = anjuta_help_new();
		app->cvs = cvs_new(app->preferences->props);
		app->style_editor =	style_editor_new (app->preferences->props_global,
											  app->preferences->props_local,
											  app->preferences->props_session,
											  app->preferences->props);
		anjuta_update_title ();
		
		while (gtk_events_pending ())
			gtk_main_iteration ();
		
		app->launcher = ANJUTA_LAUNCHER (anjuta_launcher_new ());
		g_signal_connect (G_OBJECT (app->launcher), "busy",
						  G_CALLBACK (update_ui_from_launcher), NULL);
		debugger_init ();
		anjuta_plugins_load();
		anjuta_tools_load();
		anjuta_load_yourself (ANJUTA_PREFERENCES (app->preferences)->props);
		/*
		gtk_widget_set_uposition (app, app->win_pos_x,
								  app->win_pos_y);
		gtk_window_set_default_size (GTK_WINDOW (app),
								     app->win_width, app->win_height);
		*/
		main_toolbar_update ();
		extended_toolbar_update ();
		debug_toolbar_update ();
		format_toolbar_update ();
		browser_toolbar_update ();
		anjuta_apply_preferences(ANJUTA_PREFERENCES (app->preferences), app);
	}
	/* Load layout */
	anjuta_app_load_layout (app, NULL);
}

static void
anjuta_app_add_value (AnjutaShell *shell,
			 const char *name,
			 const GValue *value,
			 GError **error)
{
	GValue *copy;
	AnjutaApp *window = ANJUTA_APP (shell);

	anjuta_shell_remove_value (shell, name, error);
	
	copy = g_new0 (GValue, 1);
	g_value_init (copy, value->g_type);
	g_value_copy (value, copy);

	g_hash_table_insert (window->values, g_strdup (name), copy);
	g_signal_emit_by_name (shell, "value_added", name, copy);
}

static void
anjuta_app_get_value (AnjutaShell *shell,
			 const char *name,
			 GValue *value,
			 GError **error)
{
	GValue *val;
	AnjutaApp *app = ANJUTA_APP (shell);
	
	val = g_hash_table_lookup (app->values, name);
	
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
anjuta_app_add_widget (AnjutaShell *shell, 
			  GtkWidget *w, 
			  const char *name,
			  const char *title, 
			  GError **error)
{
	AnjutaApp *window = ANJUTA_APP (shell);
	GtkWidget *item;

	g_return_if_fail (w != NULL);

	anjuta_shell_add (shell, 
			  name, G_TYPE_FROM_INSTANCE (w), w, 
			  NULL);

	g_hash_table_insert (window->widgets, g_strdup (name), w);

	item = gdl_dock_item_new (name, title, GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item), w);
	g_object_set_data (G_OBJECT (w), "dockitem", item);

	gdl_dock_add_item (GDL_DOCK (window->dock), 
			   GDL_DOCK_ITEM (item), GDL_DOCK_TOP);
	
	gtk_widget_show_all (item);	
}

static void
anjuta_app_remove_value (AnjutaShell *shell, 
			    const char *name, 
			    GError **error)
{
	AnjutaApp *window = ANJUTA_APP (shell);
	GValue *value;
	GtkWidget *w;
	char *key;
	
	if (g_hash_table_lookup_extended (window->widgets, name, 
					  (gpointer*)&key, (gpointer*)&w)) {
		GtkWidget *item;
		item = g_object_get_data (G_OBJECT (w), "dockitem");
		gdl_dock_item_hide_item (GDL_DOCK_ITEM (item));
		gdl_dock_object_unbind (GDL_DOCK_OBJECT (item));
		g_free (key);
	}

#if 0
	if (g_hash_table_lookup_extended (window->preference_pages, name, 
					  (gpointer*)&key, (gpointer*)&w)) {
		g_hash_table_remove (window->preference_pages, name);
		anjuta_preferences_dialog_remove_page (key);
		g_free (key);
	}
#endif
	
	if (g_hash_table_lookup_extended (app->values, name, 
					  (gpointer*)&key, (gpointer*)&value)) {
		g_hash_table_remove (app->values, name);
		g_signal_emit_by_name (app, "value_removed", name);
		g_value_unset (value);
		g_free (value);
	}
}

static void
ensure_layout_manager (AnjutaApp *window)
{
	gchar *filename;

	if (!window->layout_manager) {
		/* layout manager */
		window->layout_manager = gdl_dock_layout_new (GDL_DOCK (window->dock));
		
		/* load xml layout definitions */
		filename = gnome_util_prepend_user_home (".anjuta/layout.xml");
		g_message ("Layout = %s", filename);
		if (!gdl_dock_layout_load_from_file (window->layout_manager, filename)) {
			gchar *datadir;
			datadir = anjuta_res_get_data_dir();
			g_free (filename);
			filename = g_build_filename (datadir, "/layout.xml", NULL);
			g_message ("Layout = %s", filename);
			g_free (datadir);
			if (!gdl_dock_layout_load_from_file (window->layout_manager, filename))
				g_warning ("Loading layout from failed!!");
		}
		g_free (filename);
		
		g_signal_connect (window->layout_manager, "notify::dirty",
				  (GCallback) layout_dirty_notify, window);
	}
}

static gboolean
anjuta_app_save_layout_to_file (AnjutaApp *window)
{
	char *dir;
	char *filename;

	g_message ("Saving Layout ... ");
	dir = gnome_util_prepend_user_home (".anjuta");
	if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		mkdir (dir, 0755);
		if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) {
			anjuta_util_dialog_error (GTK_WINDOW (app),
									  "Could not create .anjuta directory.");
			return FALSE;
		}
	}
	g_free (dir);

	ensure_layout_manager (window);
	
	filename = gnome_util_prepend_user_home (".anjuta/layout.xml");
	if (!gdl_dock_layout_save_to_file (window->layout_manager, filename))
		anjuta_util_dialog_error (GTK_WINDOW (window),
								  "Could not save layout.");
	g_free (filename);
	return FALSE;
}

void
anjuta_app_save_layout (AnjutaApp *window, const gchar *name)
{
	g_return_if_fail (window != NULL);

	ensure_layout_manager (window);
	g_message ("Saving layout ... ");
	gdl_dock_layout_save_layout (window->layout_manager, name);
	anjuta_app_save_layout_to_file (window);
}

void
anjuta_app_load_layout (AnjutaApp *window, const gchar *name)
{
	g_return_if_fail (window != NULL);

	ensure_layout_manager (window);
	if (!gdl_dock_layout_load_layout (window->layout_manager, name))
		g_warning ("Loading layout failed!!");
}

static void
anjuta_app_dispose (GObject *widget)
{
	AnjutaApp *window;
	GtkAllocation *alloc;
	AnjutaWindowState *state;
	// AnjutaSession *session;

	g_assert (ANJUTA_IS_APP (widget));
	window = ANJUTA_APP (widget);

	alloc = g_object_get_data (G_OBJECT (window), "windowpos");
	g_free (alloc);

	if (window->layout_manager) {
		g_object_unref (window->layout_manager);
		window->layout_manager = NULL;
	};

	state = g_object_get_data (G_OBJECT (widget), "window_state");
	if (state) {
		save_state (GTK_WINDOW (widget));
		
		/* free window state */
		g_object_set_data (G_OBJECT (widget), "window_state", NULL);
		g_free (state);
	}
#if 0
	session = g_object_get_data (G_OBJECT (widget), "session");
	if (session) {
		// anjuta_session_save (session);
		g_object_set_data (G_OBJECT (widget), "session", NULL);
		g_object_unref (session);
	}
#endif
}

static void
anjuta_app_show (AnjutaApp *app)
{
	PropsID pr;
	EggAction *action;

	GNOME_CALL_PARENT(GTK_WIDGET_CLASS, show, (app));
	
	pr = ANJUTA_PREFERENCES (app->preferences)->props;

	start_with_dialog_show (GTK_WINDOW (app),
							app->preferences, FALSE);

	/* Editor stuffs */
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorLinenumbers");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"margin.linenumber.visible",
												0));
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorMarkers");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"margin.marker.visible",
												0));
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorFolds");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"margin.fold.visible",
												0));
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorGuides");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"view.indentation.guides",
												0));
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorSpaces");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"view.whitespace",
												0));
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorEOL");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"view.eol",
												0));
	action = anjuta_ui_get_action (app->ui, "ActionGroupView",
								   "ActionViewEditorWrapping");
	egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
								  prop_get_int (pr,
												"view.line.wrap",
												1));
	update_gtk ();
		
	if (app->dirs->first_time)
	{
		gchar *file;
		app->dirs->first_time = FALSE;
		file = g_strconcat (app->dirs->data, "/welcome.txt", NULL);
		anjuta_info_show_file (file, 500, 435);
		g_free (file);
	}
}

static void
anjuta_app_class_init (AnjutaAppClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	widget_class = (GtkWidget*) class;
	object_class->dispose = anjuta_app_dispose;
	widget_class->show = anjuta_app_show;
}

static void
anjuta_shell_iface_init (AnjutaShellIface *iface)
{
	iface->add_widget = anjuta_app_add_widget;
	// iface->add_preferences = anjuta_app_add_preferences;
	iface->add_value = anjuta_app_add_value;
	iface->get_value = anjuta_app_get_value;
	iface->remove_value = anjuta_app_remove_value;
}

ANJUTA_TYPE_BEGIN(AnjutaApp, anjuta_app, GNOME_TYPE_APP);
ANJUTA_INTERFACE(anjuta_shell, ANJUTA_TYPE_SHELL);
ANJUTA_TYPE_END;

/****************************************/
void
anjuta_session_restore (GnomeClient* client)
{
	anjuta_load_cmdline_files();
}

const GList *anjuta_get_tag_list(TextEditor *te, guint tag_types)
{
	static GList *tag_names = NULL;

	if (!te)
		te = anjuta_get_current_text_editor();
	if (te && (te->tm_file) && (te->tm_file->tags_array) &&
		(te->tm_file->tags_array->len > 0))
	{
		TMTag *tag;
		guint i;

		if (tag_names)
		{
			GList *tmp;
			for (tmp = tag_names; tmp; tmp = g_list_next(tmp))
				g_free(tmp->data);
			g_list_free(tag_names);
			tag_names = NULL;
		}

		for (i=0; i < te->tm_file->tags_array->len; ++i)
		{
			tag = TM_TAG(te->tm_file->tags_array->pdata[i]);
			if (tag == NULL) 
				return NULL;
			if (tag->type & tag_types)
			{
				if ((NULL != tag->atts.entry.scope) && isalpha(tag->atts.entry.scope[0]))
					tag_names = g_list_prepend(tag_names, g_strdup_printf("%s::%s [%ld]"
					  , tag->atts.entry.scope, tag->name, tag->atts.entry.line));
				else
					tag_names = g_list_prepend(tag_names, g_strdup_printf("%s [%ld]"
					  , tag->name, tag->atts.entry.line));
			}
		}
		tag_names = g_list_sort(tag_names, (GCompareFunc) strcmp);
		return tag_names;
	}
	else
		return NULL;
}

GList *anjuta_get_file_list(void)
{
	const TMWorkspace *ws = tm_get_workspace();
	TMWorkObject *wo;
	static GList *files = NULL;
	int i, j;

	g_return_val_if_fail(ws && ws->work_objects, NULL);

	if (files)
	{
		g_list_free(files);
		files = NULL;
	}

	for (i=0; i < ws->work_objects->len; ++i)
	{
		wo = TM_WORK_OBJECT(ws->work_objects->pdata[i]);
		if (IS_TM_SOURCE_FILE(wo))
			files = g_list_append(files, wo->file_name);
		else if (IS_TM_PROJECT(wo) && (TM_PROJECT(wo)->file_list))
		{
			for (j = 0; j < TM_PROJECT(wo)->file_list->len; ++j)
				files = g_list_append(files
				  , TM_WORK_OBJECT(TM_PROJECT(wo)->file_list->pdata[j])->file_name);
		}
	}
	return files;
}

gboolean anjuta_goto_local_tag(TextEditor *te, const char *qual_name)
{
	guint line;
	gchar *spos;
	gchar *epos;

	g_return_val_if_fail((te && qual_name), FALSE);
	spos = strchr(qual_name, '[');
	if (spos)
	{
		epos = strchr(spos+1, ']');
		if (epos)
		{
			*epos = '\0';
			line = atol(spos + 1);
			*epos = ']';
			if (0 < line)
			{
				text_editor_goto_line(te, line, TRUE, TRUE);
				return TRUE;
			}
		}
	}
	return FALSE;
}

#define IS_DECLARATION(T) ((tm_tag_prototype_t == (T)) || (tm_tag_externvar_t == (T)) \
  || (tm_tag_typedef_t == (T)))

void anjuta_goto_tag(const char *symbol, TextEditor *te, gboolean prefer_definition)
{
	GPtrArray *tags;
	TMTag *tag = NULL, *local_tag = NULL, *global_tag = NULL;
	TMTag *local_proto = NULL, *global_proto = NULL;
	guint i;
	int cmp;

	g_return_if_fail(symbol);

	if (!te)
		te = anjuta_get_current_text_editor();

	/* Get the matching definition and declaration in the local file */
	if (te && (te->tm_file) && (te->tm_file->tags_array) &&
		(te->tm_file->tags_array->len > 0))
	{
		for (i=0; i < te->tm_file->tags_array->len; ++i)
		{
			tag = TM_TAG(te->tm_file->tags_array->pdata[i]);
			cmp =  strcmp(symbol, tag->name);
			if (0 == cmp)
			{
				if (IS_DECLARATION(tag->type))
					local_proto = tag;
				else
					local_tag = tag;
			}
			else if (cmp < 0)
				break;
		}
	}
	if (!(((prefer_definition) && (local_tag)) || ((!prefer_definition) && (local_proto))))
	{
		/* Get the matching definition and declaration in the workspace */
		tags =  TM_WORK_OBJECT(tm_get_workspace())->tags_array;
		if (tags && (tags->len > 0))
		{
			for (i=0; i < tags->len; ++i)
			{
				tag = TM_TAG(tags->pdata[i]);
				if (tag->atts.entry.file)
				{
					cmp = strcmp(symbol, tag->name);
					if (cmp == 0)
					{
						if (IS_DECLARATION(tag->type))
							global_proto = tag;
						else
							global_tag = tag;
					}
					else if (cmp < 0)
						break;
				}
			}
		}
	}
	if (prefer_definition)
	{
		if (local_tag)
			tag = local_tag;
		else if (global_tag)
			tag = global_tag;
		else if (local_proto)
			tag = local_proto;
		else
			tag = global_proto;
	}
	else
	{
		if (local_proto)
			tag = local_proto;
		else if (global_proto)
			tag = global_proto;
		else if (local_tag)
			tag = local_tag;
		else
			tag = global_tag;
	}

	if (te)
	{
		an_file_history_push(te->full_filename
		  , aneditor_command(te->editor_id, ANE_GET_LINENO, (long) NULL, (long) NULL));
	}
	if (tag)
	{
		anjuta_goto_file_line_mark(tag->atts.entry.file->work_object.file_name
		  , tag->atts.entry.line, TRUE);
	}
}

void
anjuta_app_save_settings (AnjutaApp *app)
{
	gchar *buffer;
	FILE *stream;

	g_return_if_fail (ANJUTA_IS_APP (app));
	
	buffer = g_strconcat (app->dirs->settings, "/session.properties", NULL);
	stream = fopen (buffer, "w");
	g_free (buffer);
	if (stream)
	{
		anjuta_save_yourself (stream);
		fclose (stream);
	}
}

gboolean
anjuta_app_save_yourself (AnjutaApp *app, FILE * stream)
{
#ifdef	USE_STD_PREFERENCES
	GList *node;
#endif
	PropsID pr;
	gchar* key;

	pr = ANJUTA_PREFERENCES (app->preferences)->props;

	gdk_window_get_root_origin (GTK_WIDGET (app)->window,
							    &app->win_pos_x, &app->win_pos_y);
	gdk_window_get_size (GTK_WIDGET (app)->window, &app->win_width,
					     &app->win_height);
	fprintf (stream,
		 _("# * DO NOT EDIT OR RENAME THIS FILE ** Generated by Anjuta **\n"));
	fprintf (stream, "anjuta.version=%s\n", VERSION);
	fprintf (stream, "anjuta.win.pos.x=%d\n", app->win_pos_x);
	fprintf (stream, "anjuta.win.pos.y=%d\n", app->win_pos_y);
	fprintf (stream, "anjuta.win.width=%d\n", app->win_width);
	fprintf (stream, "anjuta.win.height=%d\n", app->win_height);
	// fprintf (stream, "anjuta.vpaned.size=%d\n",
	//	 GTK_PANED (app->widgets.vpaned)->child1_size);
	//fprintf (stream, "anjuta.hpaned.size=%d\n",
	//	 GTK_PANED (app->widgets.hpaned)->child1_size);

	key = g_strconcat (ANJUTA_MAIN_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_EXTENDED_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_FORMAT_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_DEBUG_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_BROWSER_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	fprintf (stream, "view.eol=%d\n", prop_get_int (pr, "view.eol", 0));
	fprintf (stream, "view.indentation.guides=%d\n",
		 prop_get_int (pr, "view.indentation.guides", 0));
	fprintf (stream, "view.whitespace=%d\n",
		 prop_get_int (pr, "view.whitespace", 0));
	fprintf (stream, "view.indentation.whitespace=%d\n",
		 prop_get_int (pr, "view.indentation.whitespace", 0));
	fprintf (stream, "view.line.wrap=%d\n",
		 prop_get_int (pr, "view.line.wrap", 1));
	fprintf (stream, "margin.linenumber.visible=%d\n",
		 prop_get_int (pr, "margin.linenumber.visible", 0));
	fprintf (stream, "margin.marker.visible=%d\n",
		 prop_get_int (pr, "margin.marker.visible", 0));
	fprintf (stream, "margin.fold.visible=%d\n",
		 prop_get_int (pr, "margin.fold.visible", 0));
#ifdef	USE_STD_PREFERENCES
	fprintf (stream, "anjuta.recent.files=");
	node = app->recent_files;
	while (node)
	{
		fprintf (stream, "\\\n%s ", (gchar *) node->data);
		node = g_list_next (node);
	}
	
	fprintf (stream, "\n\n");

	fprintf (stream, "anjuta.recent.projects=");
	node = app->recent_projects;
	while (node)
	{
		fprintf (stream, "\\\n%s ", (gchar *) node->data);
		node = g_list_next (node);
	}
#else
	anjuta_session_save_strings( SECSTR(SECTION_RECENTFILES), app->recent_files );
	anjuta_session_save_strings( SECSTR(SECTION_RECENTPROJECTS), app->recent_projects );
#endif

	fprintf (stream, "\n\n");
	fprintf (stream, "%s=", ANJUTA_LAST_OPEN_PROJECT );
	{
		gchar *last_open_project_path = "" ;
		if (app->project_dbase->project_is_open &&
			(NULL != app->project_dbase->proj_filename))
			last_open_project_path = app->project_dbase->proj_filename ;
			
		fprintf (stream, "%s\n\n", last_open_project_path );
	}
	
	start_with_dialog_save_yourself(app->preferences, stream);
	
	an_message_manager_save_yourself (app->messages, stream);
	project_dbase_save_yourself (app->project_dbase, stream);

	compiler_options_save_yourself (app->compiler_options, stream);
	if (app->project_dbase->project_is_open == FALSE)
		compiler_options_save (app->compiler_options, stream);

	command_editor_save (app->command_editor, stream);

	if (app->project_dbase->project_is_open == FALSE)
		src_paths_save (app->src_paths, stream);

	debugger_save_yourself (stream);
	cvs_save_yourself(app->cvs, stream);
	style_editor_save_yourself (app->style_editor, stream);
	if (app->project_dbase->project_is_open)
	{
		anjuta_preferences_save_filtered (ANJUTA_PREFERENCES (app->preferences),
										  stream,
										  ANJUTA_PREFERENCES_FILTER_PROJECT);
	}
	else
	{
		anjuta_preferences_save (ANJUTA_PREFERENCES (app->preferences), stream);
	}
	return TRUE;
}

gboolean
anjuta_app_load_yourself (AnjutaApp *app, PropsID pr)
{
	// gint length;

	app->win_pos_x = prop_get_int (pr, "anjuta.win.pos.x", app->win_pos_x);
	app->win_pos_y = prop_get_int (pr, "anjuta.win.pos.y", app->win_pos_y);
	app->win_width = prop_get_int (pr, "anjuta.win.width", app->win_width);
	app->win_height = prop_get_int (pr, "anjuta.win.height", app->win_height);
	// length = prop_get_int (pr, "anjuta.vpaned.size", app->win_height / 2 + 7);
	// gtk_paned_set_position (GTK_PANED (app->widgets.vpaned), length);
	// length = prop_get_int (pr, "anjuta.hpaned.size", app->win_width / 4);
	// gtk_paned_set_position (GTK_PANED (app->widgets.hpaned), length);

	glist_strings_free (app->recent_files);
#ifdef	USE_STD_PREFERENCES
	app->recent_files = glist_from_data (pr, "anjuta.recent.files");
#else
	app->recent_files = anjuta_session_load_strings( SECSTR(SECTION_RECENTFILES), NULL );
#endif
	glist_strings_free (app->recent_projects);
#ifdef	USE_STD_PREFERENCES
	app->recent_projects = glist_from_data (pr, "anjuta.recent.projects");
#else
	app->recent_projects = anjuta_session_load_strings (SECSTR(SECTION_RECENTPROJECTS), NULL );
#endif
	app->last_open_project = prop_get (pr, ANJUTA_LAST_OPEN_PROJECT);

	an_message_manager_load_yourself (app->messages, pr);
	project_dbase_load_yourself (app->project_dbase, pr);
	compiler_options_load_yourself (app->compiler_options, pr);
	compiler_options_load (app->compiler_options, pr);
	src_paths_load (app->src_paths, pr);
	debugger_load_yourself (pr);
	return TRUE;
}

void
anjuta_app_save_all_files(AnjutaApp *app)
{
	TextEditor *te;
	GList *tmp;
	for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
	{
		te = (TextEditor *) tmp->data;
		/* Save the file if necessary but do not update highlighting. */ 
		if (te->full_filename && !text_editor_is_saved (te))
			text_editor_save_file (te, FALSE);
	}
	/* Update the highlighting after all the files are saved. */
	for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
		text_editor_set_hilite_type((TextEditor *) tmp->data);

	anjuta_status (_("All files saved ..."));
}

void
anjuta_app_update_title (AnjutaApp *app)
{
	gchar *buff1, *buff2;
	GtkWidget *window;
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te)
	{
		if (te->mode == TEXT_EDITOR_PAGED)
		{
			window = GTK_WIDGET (app);
			main_toolbar_update ();
			extended_toolbar_update ();
		}
		else
		{
			window = te->widgets.window;
		}

		if (text_editor_is_saved (te))
		{
			if (te->full_filename)
			{
				buff1 =
					g_strdup_printf (_("Anjuta: %s (Saved)"),
							 te->full_filename);
			} else
			{
				buff1 = g_strdup("");
			}
			buff2 =
				g_strdup_printf (_("Anjuta: %s"),
						 te->filename);
		}
		else
		{
			if (te->full_filename)
			{
				buff1 =
					g_strdup_printf (_("Anjuta: %s (Unsaved)"),
							 te->full_filename);
			}
			else
			{
				buff1 = g_strdup("");
			}
			buff2 =
				g_strdup_printf (_("Anjuta: %s"),
						 te->filename);
		}
		if (te->full_filename)
		{
			gtk_window_set_title (GTK_WINDOW (window), buff1);
		}
		else
		{
			gtk_window_set_title (GTK_WINDOW (window), buff2);
		}
		g_free (buff1);
		g_free (buff2);
	}
	else
	{
		gtk_window_set_title (GTK_WINDOW (app),
				      _("Anjuta: No file"));
	}
}

void 
show_hide_tooltips (gboolean show)
{
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(app->toolbar.main_toolbar.toolbar), show);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(app->toolbar.extended_toolbar.toolbar), show);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(app->toolbar.debug_toolbar.toolbar), show);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(app->toolbar.browser_toolbar.toolbar), show);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(app->toolbar.format_toolbar.toolbar), show);
}

void
anjuta_app_apply_styles (AnjutaApp *app)
{
	GList *node;
	node = app->text_editor_list;
	while (node)
	{
		TextEditor *te = (TextEditor*) node->data;
		text_editor_set_hilite_type (te);
		node = g_list_next (node);
	}
}

void
anjuta_app_apply_preferences (AnjutaApp *app, AnjutaPreferences *pr)
{
	TextEditor *te;
	gint i;
	gint no_tag;
	gint show_tooltips;

	app->b_reload_last_project =
		anjuta_preferences_get_int (pr, RELOAD_LAST_PROJECT);
	
	no_tag = anjuta_preferences_get_int (pr, EDITOR_TABS_HIDE);

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (app->notebook), !no_tag);

	if (!no_tag)
	{
		gchar *tag_pos;

		tag_pos = anjuta_preferences_get (pr, EDITOR_TABS_POS);
		if (!tag_pos) {
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK
						  (app->notebook),
						  GTK_POS_TOP);
		} else {
			if (strcasecmp (tag_pos, "bottom") == 0)
				gtk_notebook_set_tab_pos (GTK_NOTEBOOK
							  (app->notebook),
							  GTK_POS_BOTTOM);
			else if (strcasecmp (tag_pos, "left") == 0)
				gtk_notebook_set_tab_pos (GTK_NOTEBOOK
							  (app->notebook),
							  GTK_POS_LEFT);
			else if (strcasecmp (tag_pos, "right") == 0)
				gtk_notebook_set_tab_pos (GTK_NOTEBOOK
							  (app->notebook),
							  GTK_POS_RIGHT);
			else
				gtk_notebook_set_tab_pos (GTK_NOTEBOOK
							  (app->notebook),
							  GTK_POS_TOP);
			g_free (tag_pos);
		}

		if (anjuta_preferences_get_int (pr, EDITOR_TABS_ORDERING))
			anjuta_order_tabs ();
	}

	an_message_manager_update(app->messages);

	for (i = 0; i < g_list_length (app->text_editor_list); i++)
	{
		te = (TextEditor*) (g_list_nth (app->text_editor_list, i)->data);
		anjuta_refresh_breakpoints (te);
	}
	show_tooltips = anjuta_preferences_get_int (pr, SHOW_TOOLTIPS);
	show_hide_tooltips(show_tooltips);
}

void
anjuta_refresh_breakpoints (TextEditor* te)
{
	breakpoints_dbase_clear_all_in_editor (debugger.breakpoints_dbase, te);
	breakpoints_dbase_set_all_in_editor (debugger.breakpoints_dbase, te);
}

void
anjuta_app_exit(AnjutaApp *app)
{
	g_return_if_fail(app != NULL );
	if(		( NULL != app->project_dbase )
		&&	(app->project_dbase->project_is_open ) )
	{
		project_dbase_close_project(app->project_dbase) ;
	}
	anjuta_plugins_unload();
	app->addIns_list	= NULL ;
}

void
anjuta_app_clean_exit (AnjutaApp *app)
{
	anjuta_kernel_signals_disconnect ();
	if (app)
	{
		gchar cmd[512];
		/* This will remove all tmp files */
		snprintf (cmd, 511, "rm -f %s/anjuta_*.%ld",
				  app->dirs->tmp, (long) getpid ());
		system (cmd);
	}

/* Is it necessary to free up all the memos on exit? */
/* Assuming that it is not, I am disabling the following */
/* Basically, because it is faster */
#if 0				/* From here */

	if (app->project_dbase->project_is_open)
		project_dbase_close_project (app->project_dbase);
	debugger_stop ();

	gtk_widget_hide (app->notebook);
	for (i = 0;
	     i <
	     g_list_length (GTK_NOTEBOOK (app->notebook)->
			    children); i++)
	{
		te = anjuta_get_notebook_text_editor (i);
		gtk_container_remove (GTK_CONTAINER
				      (gtk_notebook_get_nth_page
				       (GTK_NOTEBOOK (app->widgets.
						      notebook), i)),
				      te->widgets.client);
		gtk_container_add (GTK_CONTAINER
				   (te->widgets.client_area),
				   te->widgets.client);}
	app->shutdown_in_progress = TRUE;
	for (i = 0; i < g_list_length (app->text_editor_list); i++)
	{
		te =
			(TextEditor
		      *) (g_list_nth_data (app->text_editor_list, i));
		text_editor_destroy (te);
	}
	if (app->text_editor_list)
		g_list_free (app->text_editor_list);
	for (i = 0; i < g_list_length (app->recent_files); i++)
	{
		g_free (g_list_nth_data (app->recent_files, i));
	}
	if (app->recent_files)
		g_list_free (app->recent_files);
	for (i = 0; i < g_list_length (app->recent_projects); i++)
	{
		g_free (g_list_nth_data (app->recent_projects, i));
	}
	if (app->recent_projects)
		g_list_free (app->recent_projects);
	if (app->fileselection)
		gtk_widget_destroy (app->fileselection);
	if (app->save_as_fileselection)
		gtk_widget_destroy (app->save_as_fileselection);
	if (app->preferences)
		preferences_destroy (app->preferences);
	if (app->compiler_options)
		compiler_options_destroy (app->compiler_options);
	if (app->src_paths)
		src_paths_destroy (app->src_paths);
	if (app->messages)
		gtk_widget_destroy(GTK_WIDGET(app->messages));
	if (app->project_dbase)
		project_dbase_destroy (app->project_dbase);
	if (app->command_editor)
		command_editor_destroy (app->command_editor);
	if (app->dirs)
		anjuta_dirs_destroy (app->dirs);
	if (app->executer)
		executer_destroy (app->executer);
	if (app->configurer)
		configurer_destroy (app->configurer);
	if (app->style_editor)
		style_editor_destroy (app->style_editor);
	if (app->execution_dir)
		g_free (app->execution_dir);
	debugger_shutdown ();
	
	string_assign (&app->cur_job, NULL);
	app->text_editor_list = NULL;
	app->fileselection = NULL;
	app->save_as_fileselection = NULL;
	app->preferences = NULL;
	main_menu_unref ();

	app = NULL;
#endif /* To here */
	
	gtk_main_quit ();
}

void
anjuta_app_update_status (AnjutaApp *app, gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gnome_app_flash (GNOME_APP (app), message);
	g_free(message);
}

gboolean
anjuta_boolean_question (gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	gint ret;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	dialog = gtk_message_dialog_new (GTK_WINDOW (app), 
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO, message);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (message);
	
	return (ret == GTK_RESPONSE_YES);
}

void
anjuta_set_file_properties (gchar* file)
{
	gchar *full_filename, *dir, *ext;
	const gchar *filename;
	PropsID props;

	g_return_if_fail (file != NULL);
	full_filename = g_strdup (file);
	props = ANJUTA_PREFERENCES (app->preferences)->props;
	dir = g_dirname (full_filename);
	filename = extract_filename (full_filename);
	ext = get_file_extension ((gchar*) filename);
	prop_set_with_key (props, CURRENT_FULL_FILENAME, "");
	prop_set_with_key (props, CURRENT_FILE_DIRECTORY, "");
	prop_set_with_key (props, CURRENT_FILENAME_WITH_EXT, "");
	prop_set_with_key (props, CURRENT_FILE_EXTENSION, "");

	if (full_filename) prop_set_with_key (props, CURRENT_FULL_FILENAME_WITH_EXT, full_filename);
	if (dir) prop_set_with_key (props, CURRENT_FILE_DIRECTORY, dir);
	if (filename) prop_set_with_key (props, CURRENT_FILENAME_WITH_EXT, filename);
	if (ext)
	{
		prop_set_with_key (props, CURRENT_FILE_EXTENSION, ext);
		*(--ext) = '\0';
		/* Ext has been removed */
	}
	if (filename) prop_set_with_key (props, CURRENT_FILENAME, filename);
	if (full_filename) prop_set_with_key (props, CURRENT_FULL_FILENAME, full_filename);
	if (dir) g_free (dir);
	if (full_filename) g_free (full_filename);
}

gchar *
anjuta_get_full_filename (gchar * fn)
{
	gchar *cur_dir, *dummy;
	TextEditor *te;
	TMWorkObject *source_file;
	GList *te_list;
	gchar *real_path;
	const gchar *fname;
	GList *list, *node;

	g_return_val_if_fail(fn, NULL);
	real_path = tm_get_real_path(fn);
	
	/* If it is full and absolute path, there is no need to 
	go further, even if the file is not found*/
	if (fn[0] == '/')
		return real_path;

	/* First, check if we can get the file straightaway */
	if (file_is_regular(real_path))
		return real_path;
	g_free(real_path);

	/* Get the name part of the file */
	/* if (NULL == (fname = strrchr(fn, '/')))
		fname = fn;
	else ++fname; */
   fname = extract_filename(fn);
	
	/* Next, check if the file is present in the execution directory */
	if( app->execution_dir)
	{
		dummy = g_strconcat (app->execution_dir, "/", fn, NULL);
		real_path = tm_get_real_path(dummy);
		g_free(dummy);
		if (file_is_regular (real_path) == TRUE)
			return real_path;
		g_free (real_path);
	}
	/* Next, see if we can find this from the TagManager workspace */
	source_file = tm_workspace_find_object(TM_WORK_OBJECT(app->tm_workspace), fname, TRUE);
	if (NULL != source_file)
		return g_strdup(source_file->file_name);
	cur_dir = g_get_current_dir ();
	
	/* The following matches should never be used under normal circumstances */
	if (app->project_dbase->project_is_open)
	{
		gchar *src_dir;
		gchar *real_path;

		/* See if it on of the source files */
		src_dir = project_dbase_get_module_dir (app->project_dbase, MODULE_SOURCE);
		dummy = g_strconcat (src_dir, "/", fn, NULL);
		g_free (src_dir);
		real_path = tm_get_real_path(dummy);
		g_free(dummy);
		if (file_is_regular (real_path) == TRUE)
		{
			g_free (cur_dir);
			return real_path;
		}
		g_free (real_path);
		
		/* See if the file is relative to the top project dir */
		src_dir = app->project_dbase->top_proj_dir;
		if (src_dir) {
			dummy = g_strconcat (src_dir, "/", fn, NULL);
			real_path = tm_get_real_path(dummy);
			g_free(dummy);
			if (file_is_regular (real_path) == TRUE)
			{
				g_free (cur_dir);
				return real_path;
			}
			g_free (real_path);
		}
	}
	else
	{
		dummy = g_strconcat (cur_dir, "/", fn, NULL);
		real_path = tm_get_real_path(dummy);
		g_free(dummy);

		if (file_is_regular (real_path) == TRUE)
		{
			g_free (cur_dir);
			return real_path;
		}
		g_free (real_path);
	}

	list = src_paths_get_paths (app->src_paths);
	node = list;
	while (node)
	{
		gchar *text = node->data;
		if (app->project_dbase->project_is_open)
			dummy =
				g_strconcat (app->project_dbase->top_proj_dir,
					     "/", text, "/", fn, NULL);
		else
			dummy =
				g_strconcat (cur_dir, "/", text, "/", fn,
					     NULL);
		real_path = tm_get_real_path(dummy);
		g_free(dummy);

		if (file_is_regular (real_path) == TRUE)
		{
			g_free (cur_dir);
			glist_strings_free (list);
			return real_path;
		}
		g_free (real_path);
		node = g_list_next (node);
	}
	glist_strings_free (list);
	dummy = g_strconcat (cur_dir, "/", fn, NULL);
	real_path = tm_get_real_path(dummy);
	g_free(dummy);
	g_free (cur_dir);
	
	/* If nothing is found, just return the original file name */
	if (file_is_regular (real_path) == FALSE) {
		g_free (real_path);
		real_path = g_strdup (fn);
	}
	return real_path;
}

gboolean
anjuta_app_progress_init (AnjutaApp *app, gchar * description, gdouble full_value,
		      GnomeAppProgressCancelFunc cancel_cb, gpointer data)
{
	if (app->in_progress)
		return FALSE;
	app->in_progress = TRUE;
	app->progress_value = 0.0;
	app->progress_full_value = full_value;
	app->progress_cancel_cb = cancel_cb;
	app->progress_key =
		gnome_app_progress_timeout (GNOME_APP (app),
					    description,
					    100,
					    on_anjuta_progress_cb,
					    on_anjuta_progress_cancel, data);
	return TRUE;
}

void
anjuta_app_progress_set (gdouble value)
{
	if (app->in_progress)

		app->progress_value = value / app->progress_full_value;
}

void
anjuta_app_progress_done (AnjutaApp *app, gchar * end_mesg)
{
	if (app->in_progress)
		gnome_app_progress_done (app->progress_key);
	anjuta_status (end_mesg);
	app->in_progress = FALSE;
}

void
anjuta_app_set_busy (AnjutaApp *app)
{
	//GnomeAnimator *led;
	GList *node;
	
	//led = GNOME_ANIMATOR (app->toolbar.main_toolbar.led);
	app->busy_count++;
	if (app->busy_count > 1)
		return;
	if (app_cursor)
		gdk_cursor_destroy (app_cursor);
	//gnome_animator_stop (led);
	//gnome_animator_goto_frame (led, 1);
	app_cursor = gdk_cursor_new (GDK_WATCH);
	if (GTK_WIDGET (app)->window)
		gdk_window_set_cursor (GTK_WIDGET (app)->window, app_cursor);
	node = app->text_editor_list;
	while (node)
	{
		if (((TextEditor*)node->data)->widgets.editor->window)
			scintilla_send_message (SCINTILLA(((TextEditor*)node->data)->widgets.editor),
				SCI_SETCURSOR, SC_CURSORWAIT, 0);
		node = g_list_next (node);
	}
	gdk_flush ();
}

void
anjuta_app_set_active (AnjutaApp *app)
{
	GList* node;
	
	app->busy_count--;
	if (app->busy_count > 0)
		return;
	app->busy_count = 0;
	if (app_cursor)
		gdk_cursor_destroy (app_cursor);
	app_cursor = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
	gdk_window_set_cursor (GTK_WIDGET (app)->window, app_cursor);
	node = app->text_editor_list;
	while (node)
	{
		scintilla_send_message (SCINTILLA(((TextEditor*)node->data)->widgets.editor),
			SCI_SETCURSOR, SC_CURSORNORMAL, 0);
		node = g_list_next (node);
	}
	update_led_animator ();
}

void
anjuta_load_cmdline_files ()
{
	GList *node;
	
	/* Open up the command argument files */
	node = command_args;
	while (node)
	{
		switch (get_file_ext_type (node->data))
		{
		case FILE_TYPE_IMAGE:
			break;
		case FILE_TYPE_PROJECT:
			if (!app->project_dbase->project_is_open)
			{
				fileselection_set_filename (app->project_dbase->fileselection_open,
											node->data);
				project_dbase_load_project (app->project_dbase,
											node->data, TRUE);
			}
			break;
		default:
			anjuta_goto_file_line (node->data, -1);
			break;
		}
		node = g_list_next (node);
	}
	if (command_args)
	{
		glist_strings_free (command_args);
		command_args = NULL;
	}
}

gboolean anjuta_is_installed (gchar * prog, gboolean show)
{
	gchar* prog_path = g_find_program_in_path (prog);
	if (prog_path)
	{
		g_free (prog_path);
		return TRUE;
	}
	if (show)
	{
		anjuta_error (_
					 ("The \"%s\" utility is not installed.\n"
					  "Install it and then restart Anjuta."), prog);
	}
	return FALSE;
}

void
anjuta_app_job_status (AnjutaApp *app, gboolean set_job, gchar* job_name)
{
	gchar *prj, *edit, *job, *mode;
	guint line, col, caret_pos;
	gchar *str;
	gint zoom;
	TextEditor *te;
	GtkWidget *sci;

	te = anjuta_get_current_text_editor ();
	if (te)
	{
		sci = te->widgets.editor;
	}
	else
	{
		sci = NULL;
	}
	prj = project_dbase_get_proj_name (app->project_dbase);
	if (!prj)	prj = g_strdup (_("None"));
	zoom = anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
									(app->preferences), TEXT_ZOOM_FACTOR, 0);
	if (sci)
	{
		gint editor_mode;
		
		editor_mode =  scintilla_send_message (SCINTILLA (te->widgets.editor),
			SCI_GETEOLMODE, 0, 0);
		switch (editor_mode) {
			case SC_EOL_CRLF:
				mode = g_strdup(_("DOS (CRLF)"));
				break;
			case SC_EOL_LF:
				mode = g_strdup(_("Unix (LF)"));
				break;
			case SC_EOL_CR:
				mode = g_strdup(_("Mac (CR)"));
				break;
			default:
				mode = g_strdup(_("Unknown"));
				break;
		}

		caret_pos =
			scintilla_send_message (SCINTILLA (sci),
						SCI_GETCURRENTPOS, 0, 0);
		line =
			scintilla_send_message (SCINTILLA (sci),
						SCI_LINEFROMPOSITION,
						caret_pos, 0);
		col =
			scintilla_send_message (SCINTILLA (sci),
						SCI_GETCOLUMN, caret_pos, 0);
		if (scintilla_send_message
		    (SCINTILLA (sci), SCI_GETOVERTYPE, 0, 0))
		{
			edit = g_strdup (_("OVR"));
		}
		else
		{
			edit = g_strdup (_("INS"));
		}
	}
	else
	{
		line = col = 0;
		edit = g_strdup (_("INS"));
		mode = g_strdup(_("Unix (LF)"));
	}
	if (set_job)
		string_assign (&app->cur_job, job_name);

	if (app->cur_job)
		job = g_strdup (app->cur_job);
	else
		job = g_strdup (_("None"));
	str =
		g_strdup_printf(
		_("  Project: %s       Zoom: %d       Line: %04d       Col: %03d       %s       Job: %s       Mode: %s"),
		 prj, zoom, line+1, col, edit, job, mode);
	gnome_appbar_set_default (GNOME_APPBAR (app->appbar), str);
	g_free (str);
	g_free (prj);
	g_free (edit);
	g_free (job);
	g_free (mode);
}

void
anjuta_app_toolbar_set_view (AnjutaApp *app, gchar* toolbar_name, gboolean view,
							 gboolean resize, gboolean set_in_props)
{
	gchar* key;
	BonoboDock* dock;
	BonoboDockItem* item;

	if (set_in_props)
	{
		key = g_strconcat (toolbar_name, ".visible", NULL);
		anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
									key, view);
		g_free (key);
	}

	item = gnome_app_get_dock_item_by_name (GNOME_APP (app), toolbar_name);

	g_return_if_fail(toolbar_name != NULL);
	g_return_if_fail (item != NULL);
	g_return_if_fail (BONOBO_IS_DOCK_ITEM (item));

	if (view)
	{
		gtk_widget_show(GTK_WIDGET (item));
		gtk_widget_show(GTK_BIN(item)->child);
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET (item));
		gtk_widget_hide(GTK_BIN(item)->child);
	}
	dock = gnome_app_get_dock(GNOME_APP(app));
	g_return_if_fail (dock != NULL);
	if (resize)
		gtk_widget_queue_resize (GTK_WIDGET (dock));
}

gint
anjuta_get_file_property (gchar* key, gchar* filename, gint default_value)
{
	gint value;
	gchar* value_str;

	g_return_val_if_fail (filename != NULL, default_value);
	g_return_val_if_fail (strlen (filename) != 0,  default_value);

	value_str = prop_get_new_expand (ANJUTA_PREFERENCES(app->preferences)->props,
									 key, filename);
	if (!value_str)
		return default_value;
	value = atoi (value_str);
	g_free(value_str);
	return value;
}

void
anjuta_load_this_project( const gchar * szProjectPath )
{
	/* Give the use a chance to save an existing project */
	if (app->project_dbase->project_is_open)
	{
		project_dbase_close_project (app->project_dbase);
	}
	if( app->project_dbase->project_is_open )
		return ;
	fileselection_set_filename (app->project_dbase->fileselection_open,
								(gchar*)szProjectPath);
	project_dbase_load_project (app->project_dbase, 
								(const gchar*)szProjectPath, TRUE);
}

void
anjuta_open_project( )
{
	/* Give the use a chance to save an existing project */
	if (app->project_dbase->project_is_open)
	{
		project_dbase_close_project (app->project_dbase);
	}
	if( app->project_dbase->project_is_open )
		return ;
	project_dbase_open_project (app->project_dbase);
}

void 
anjuta_load_last_project()
{
	//printf ("anjuta_load_last_project");
	
	if ((NULL != app->last_open_project) 
		&&	strlen (app->last_open_project))
	{
#ifdef	DEBUG_LOAD_PROJECTS	/* debug code, may return useful */
		char	szppp[1024];
		sprintf (szppp, "%d, >>%s<<", strlen (app->last_open_project),
				 app->last_open_project );
		MessageBox (szppp);
#endif
		anjuta_load_this_project( app->last_open_project );
	}
}

void anjuta_search_sources_for_symbol(const gchar *s)
{
	gchar command[BUFSIZ];

	if ((NULL == s) || (isspace(*s) || ('\0' == *s)))
		return;

	// exclude my own tag and backup files
	snprintf(command, BUFSIZ, "grep -FInHr --exclude=\"CVS\" --exclude=\"tm.tags\" --exclude=\"tags.cache\" --exclude=\".*\" --exclude=\"*~\" --exclude=\"*.o\" '%s' %s", s,
			 project_dbase_get_dir(app->project_dbase));
// FIXME:	
/*	g_signal_connect (app->launcher, "child-exited",
					  G_CALLBACK (find_in_files_terminated), NULL);
	
	if (anjuta_launcher_execute (app->launcher, command,
								 find_in_files_mesg_arrived, NULL) == FALSE)
		return;
*/	
	anjuta_update_app_status (TRUE, _("Looking up symbol"));
	an_message_manager_clear (app->messages, MESSAGE_FIND);
	an_message_manager_append (app->messages, _("Finding in Files ....\n"),
							   MESSAGE_FIND);
	an_message_manager_show (app->messages, MESSAGE_FIND);
}

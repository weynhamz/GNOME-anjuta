/*
 * anjuta2.c Copyright (C) 2000  Kh. Naba Kumar Singh
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
#include <syslog.h>

#include <sys/wait.h>
#include <gnome.h>

#include "text_editor.h"
#include "anjuta.h"
#include "fileselection.h"
#include "utilities.h"
#include "resources.h"
#include "messagebox.h"
#include "launcher.h"
#include "debugger.h"
#include "controls.h"
#include "project_dbase.h"
#include "anjuta_info.h"

#include "file_history.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "properties.h"
#include "commands.h"
/*-------------------------------------------------------------------*/
#define	LOCALS_PLUGIN_DIR	"/.anjuta/plugins"

extern gboolean closing_state;
static GdkCursor *app_cursor;
/*-------------------------------------------------------------------*/
void anjuta_child_terminated (int t);
static void on_message_clicked(GtkObject* obj, char* message);					  
static void anjuta_show_text_editor (TextEditor * te);
static void plugins_foreach_delete( gpointer data, gpointer user_data );
static void free_plug_ins( GList * pList );
static PlugInErr plug_in_init( AnjutaAddInPtr self, const gchar *szModName );
static GList*scan_AddIns_in_directory( AnjutaApp* pApp, const gchar *szDirName, GList *pList );
static int select_all_files (const struct dirent *e);

/*-------------------------------------------------------------------*/


static void
anjuta_fatal_signal_handler (int t)
{
	g_warning(_("Anjuta: Caught signal %d (%s) !"), t, g_strsignal (t));
	exit(1);
}

static void
anjuta_exit_signal_handler (int t)
{
	g_warning(_("Anjuta: Caught signal %d (%s) !"), t, g_strsignal (t));
	anjuta_clean_exit ();
	exit(1);
}

void
anjuta_connect_kernel_signals ()
{
	signal(SIGSEGV, anjuta_fatal_signal_handler);
	signal(SIGILL, anjuta_fatal_signal_handler);
	signal(SIGABRT, anjuta_fatal_signal_handler);
	signal(SIGSEGV, anjuta_fatal_signal_handler);

	signal(SIGINT, anjuta_exit_signal_handler);
	signal(SIGHUP, anjuta_exit_signal_handler);
	signal(SIGQUIT, anjuta_exit_signal_handler);

	signal (SIGCHLD, anjuta_child_terminated);
}

void
anjuta_new ()
{
	char wd[PATH_MAX];
	app = (AnjutaApp *) g_malloc0(sizeof (AnjutaApp));
	if (app)
	{
		/* Must declare static, because it will be used forever */
		static FileSelData fsd1 = { N_("Open File"), NULL,
			on_open_filesel_ok_clicked,
			on_open_filesel_cancel_clicked, NULL
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
		app->bUseComponentUI	= FALSE ;
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
		app->win_width = gdk_screen_width () - 10;
		app->win_height = gdk_screen_height () - 25;
		app->vpaned_height = app->win_height / 2 + 7;
		app->hpaned_width = app->win_width / 4;
		app->in_progress = FALSE;
		app->auto_gtk_update = TRUE;
		app->busy_count = 0;
		app->execution_dir = NULL;
		app->first_time_expose = TRUE;
		app->last_open_project	= NULL ;

		create_anjuta_gui (app);
		
		app->dirs = anjuta_dirs_new ();
		app->fileselection = create_fileselection_gui (&fsd1);
		
		/* Set to the current dir */
		getcwd(wd, PATH_MAX);
		fileselection_set_dir (app->fileselection, wd);
		
		app->b_reload_last_project	= TRUE ;
		app->preferences = preferences_new ();
		app->save_as_fileselection = create_fileselection_gui (&fsd2);
		app->save_as_build_msg_sel = create_fileselection_gui (&fsd3);
		app->find_replace = find_replace_new ();
		app->find_in_files = find_in_files_new ();
		app->compiler_options = compiler_options_new (app->preferences->props);
		app->src_paths = src_paths_new ();
		app->messages = ANJUTA_MESSAGE_MANAGER(anjuta_message_manager_new ());
		create_default_types(app->messages);
		gtk_signal_connect(GTK_OBJECT(app->messages), "message_clicked", GTK_SIGNAL_FUNC(on_message_clicked), NULL);
		app->project_dbase = project_dbase_new (app->preferences->props);
		app->configurer = configurer_new (app->project_dbase->props);
		app->executer = executer_new (app->project_dbase->props);
		app->command_editor = command_editor_new (app->preferences->props_global,
					app->preferences->props_local, app->project_dbase->props);
		app->tm_workspace = tm_get_workspace();
		if (TRUE != tm_workspace_load_global_tags(PACKAGE_DATA_DIR "/system.tags"))
			g_warning("Unable to load global tag file");
		app->help_system = anjuta_help_new();

		app->widgets.the_client = app->widgets.vpaned;
		app->widgets.hpaned_client = app->widgets.hpaned;
		gtk_container_add (GTK_CONTAINER (app->widgets.hpaned),
				   app->widgets.notebook);
		gtk_notebook_popup_enable (GTK_NOTEBOOK
					   (app->widgets.notebook));
		gtk_box_pack_start (GTK_BOX (app->widgets.mesg_win_container),
				    GTK_WIDGET(app->messages), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX
				    (app->widgets.
				     project_dbase_win_container),
				    app->project_dbase->widgets.client, TRUE,
				    TRUE, 0);
		project_dbase_hide (app->project_dbase);
		gtk_widget_hide(GTK_WIDGET(app->messages));
		gtk_paned_set_position (GTK_PANED (app->widgets.vpaned),
					app->vpaned_height);
		gtk_paned_set_position (GTK_PANED (app->widgets.hpaned),
					app->hpaned_width);
		anjuta_update_title ();
		launcher_init ();
		debugger_init ();
		app->addIns_list = scan_AddIns_in_directory( app, PACKAGE_PLUGIN_DIR, NULL );
		{		
			gchar *plugin_dir;
			char const * const home_dir = getenv ("HOME");
			/* Load the user plugins */
			if (home_dir != NULL)
			{
				plugin_dir = g_strconcat (home_dir, LOCALS_PLUGIN_DIR, NULL);
				app->addIns_list = scan_AddIns_in_directory( app, plugin_dir, app->addIns_list );
				g_free (plugin_dir);
			}
		}
		anjuta_load_yourself (app->preferences->props);
		gtk_widget_set_uposition (app->widgets.window, app->win_pos_x,
					  app->win_pos_y);
		gtk_window_set_default_size (GTK_WINDOW (app->widgets.window),
					     app->win_width, app->win_height);
		main_toolbar_update ();
		extended_toolbar_update ();
		debug_toolbar_update ();
		format_toolbar_update ();
		browser_toolbar_update ();
		anjuta_apply_preferences();
	}
	else
	{
		app = NULL;
		g_error (_("Cannot create application... exiting\n"));
	}
}

void
anjuta_session_restore (GnomeClient* client)
{
	anjuta_load_cmdline_files();
}

TextEditor *
anjuta_append_text_editor (gchar * filename)
{
	GtkWidget *tab_widget, *eventbox;
	TextEditor *te, *cur_page;
	gchar *buff;

	cur_page = anjuta_get_current_text_editor ();
	te = text_editor_new (filename, cur_page, app->preferences);
	if (te == NULL) return NULL;
	gtk_signal_disconnect_by_func (GTK_OBJECT (app->widgets.notebook),
				       GTK_SIGNAL_FUNC
				       (on_anjuta_notebook_switch_page),
				       NULL);
	anjuta_set_current_text_editor (te);
	anjuta_clear_windows_menu ();
	app->text_editor_list = g_list_append (app->text_editor_list, te);
	breakpoints_dbase_set_all_in_editor (debugger.breakpoints_dbase, te);
	switch (te->mode)
	{
	case TEXT_EDITOR_PAGED:
		tab_widget = text_editor_tab_widget_new(te);
	
		eventbox = gtk_event_box_new ();
		gtk_widget_show (eventbox);
		gtk_notebook_prepend_page (GTK_NOTEBOOK
					   (app->widgets.notebook), eventbox,
					   tab_widget);
		gtk_notebook_set_menu_label_text(GTK_NOTEBOOK
						(app->widgets.notebook), eventbox,
						te->filename);

		gtk_container_add (GTK_CONTAINER (eventbox),
				   te->widgets.client);
	
		/* For your kind info, this same data is also set in */
		/* the function on_text_editor_dock_activated() */
		gtk_object_set_data (GTK_OBJECT (eventbox), "TextEditor", te);
		
		if (te->full_filename)
			buff =
				g_strdup_printf (_("Anjuta: %s"),
						 te->full_filename);
		else
			buff =
				g_strdup_printf (_("Anjuta: %s"),
						 te->filename);
		gtk_window_set_title (GTK_WINDOW (app->widgets.window), buff);
		g_free (buff);
		gtk_notebook_set_page (GTK_NOTEBOOK (app->widgets.notebook),
		       0);
		break;

	case TEXT_EDITOR_WINDOWED:
		gtk_widget_show (te->widgets.window);
		break;
	}
	gtk_signal_connect (GTK_OBJECT (app->widgets.notebook), "switch_page",
			    GTK_SIGNAL_FUNC (on_anjuta_notebook_switch_page),
			    NULL);
	anjuta_set_current_text_editor(te);
	anjuta_grab_text_focus ();
	return te;
}

void
anjuta_remove_current_text_editor ()
{
	TextEditor* te = anjuta_get_current_text_editor ();
	anjuta_remove_text_editor (te);
}

void
anjuta_remove_text_editor (TextEditor* te)
{
	GtkWidget *submenu;
	GtkWidget *wid;

	gint nb_cur_page_num;

	if (te == NULL)
		return;

	gtk_signal_disconnect_by_func (GTK_OBJECT (app->widgets.notebook),
				       GTK_SIGNAL_FUNC
				       (on_anjuta_notebook_switch_page),
				       NULL);
	anjuta_clear_windows_menu ();
	if (te->full_filename != NULL)
	{
		gint max_recent_files;

		max_recent_files =
			preferences_get_int (app->preferences,
					     MAXIMUM_RECENT_FILES);
		app->recent_files =
			update_string_list (app->recent_files,
					    te->full_filename,
					    max_recent_files);
		submenu =
			create_submenu (_("Recent Files "), app->recent_files,
					GTK_SIGNAL_FUNC
					(on_recent_files_menu_item_activate));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM
					   (app->widgets.menubar.file.
					    recent_files), submenu);
	}
	breakpoints_dbase_clear_all_in_editor (debugger.breakpoints_dbase, te);
	switch (te->mode)
	{
	case TEXT_EDITOR_PAGED:
		wid = te->widgets.client->parent;
		nb_cur_page_num = 
			gtk_notebook_page_num (GTK_NOTEBOOK (app->widgets.notebook), wid);
		if (GTK_IS_CONTAINER (wid))
			gtk_container_remove (GTK_CONTAINER (wid),
					      te->widgets.client);
		gtk_notebook_remove_page (GTK_NOTEBOOK
					  (app->widgets.notebook),
					  nb_cur_page_num);
		gtk_container_add (GTK_CONTAINER (te->widgets.client_area),
				   te->widgets.client);
		app->text_editor_list =
			g_list_remove (app->text_editor_list, te);
		/* This is called to set the next active document */
		if (GTK_NOTEBOOK (app->widgets.notebook)->children == NULL)
		{
			anjuta_set_current_text_editor (NULL);
		}
		else
		{
			on_anjuta_window_focus_in_event (NULL, NULL, NULL);
		}
		break;

	case TEXT_EDITOR_WINDOWED:
		app->text_editor_list =
			g_list_remove (app->text_editor_list, te);
		break;
	}
	text_editor_destroy (te);
	gtk_signal_connect (GTK_OBJECT (app->widgets.notebook), "switch_page",
			    GTK_SIGNAL_FUNC (on_anjuta_notebook_switch_page),
			    NULL);
}

TextEditor *
anjuta_get_current_text_editor ()
{
	return app->current_text_editor;
}

void
anjuta_set_current_text_editor (TextEditor * te)
{	
	TextEditor *ote = app->current_text_editor;
	
	if (ote != NULL && ote->buttons.close != NULL) {
		gtk_widget_hide(ote->buttons.close);
		gtk_widget_show(ote->widgets.close_pixmap);
	}
	app->current_text_editor = te;
	if (te != NULL && te->buttons.close != NULL) {
		gtk_widget_show(te->buttons.close);
		gtk_widget_hide(te->widgets.close_pixmap);
	}
	main_toolbar_update ();
	extended_toolbar_update ();
	format_toolbar_update ();
	browser_toolbar_update ();
	anjuta_update_title ();
	anjuta_update_app_status (FALSE, NULL);
	if (te == NULL)
	{
		chdir (app->dirs->home);
		return;
	}
	if (te->full_filename)
	{
		gchar* dir;
		dir = g_dirname (te->full_filename);
		chdir (dir);
		g_free (dir);
		return;
	}
	return;
}

TextEditor *
anjuta_get_notebook_text_editor (gint page_num)
{
	GtkWidget *page;
	TextEditor *te;

	page =
		gtk_notebook_get_nth_page (GTK_NOTEBOOK
					   (app->widgets.notebook), page_num);
	te = gtk_object_get_data (GTK_OBJECT (page), "TextEditor");
	return te;
}

GtkWidget *
anjuta_get_current_text ()
{
	TextEditor *t = anjuta_get_current_text_editor ();
	if (t)
		return t->widgets.editor;
	return NULL;
}

gchar *
anjuta_get_current_selection ()
{
	gchar *chars;
	TextEditor* te;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return NULL;

	chars = text_editor_get_selection (te);
	return chars;
}

TextEditor *
anjuta_goto_file_line (gchar * fname, glong lineno)
{
	return anjuta_goto_file_line_mark (fname, lineno, TRUE);
}

TextEditor *
anjuta_goto_file_line_mark (gchar * fname, glong lineno, gboolean mark)
{
	gchar *fn, *dummy;
	GList *node;

	TextEditor *te;

	g_return_val_if_fail (fname != NULL, NULL);
	dummy = anjuta_get_full_filename (fname);
	fn = resolved_file_name(dummy);
	g_free(dummy);
	g_return_val_if_fail (fname != NULL, NULL);
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) node->data;
		if (te->full_filename == NULL)
		{
			node = g_list_next (node);
			continue;
		}
		if (strcmp (fn, te->full_filename) == 0)
		{
			if (lineno >= 0)
				text_editor_goto_line (te, lineno, mark);
			anjuta_show_text_editor (te);
			anjuta_grab_text_focus ();
			g_free (fn);
			an_file_history_push(te->full_filename, lineno);
			return te ;
		}
		node = g_list_next (node);
	}
	te = anjuta_append_text_editor (fn);
	if (te)
	{
		an_file_history_push(te->full_filename, lineno);
		if (lineno >= 0)
			text_editor_goto_line (te, lineno, mark);
	}
	g_free (fn);
	return te ;
}

static const GString *get_qualified_tag_name(const TMTag *tag)
{
	static GString *s = NULL;

	g_return_val_if_fail((tag && tag->name), NULL);
	if (!s)
		s = g_string_sized_new(255);
	g_string_assign(s, "");
	if (tag->atts.entry.scope)
		g_string_sprintfa(s, "%s.", tag->atts.entry.scope);
	g_string_sprintfa(s, "%s", tag->name);
	return s;
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
			if (tag->type & tag_types)
				tag_names = g_list_prepend(tag_names, g_strdup(get_qualified_tag_name(tag)->str));
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
	char *name;
	char *scope;
	guint i;
	int cmp;
	TMTag *tag;

	if (!qual_name || !te || !te->tm_file || !te->tm_file->tags_array
	  || (0 == te->tm_file->tags_array->len))
		return FALSE;
	scope = g_strdup(qual_name);
	name = strrchr(scope, '.');
	if (name)
	{
		*name = '\0';
		++ name;
		
	}
	else
	{
		name = scope;
		scope = NULL;
	}

	for (i = 0; i < te->tm_file->tags_array->len; ++i)
	{
		tag = TM_TAG(te->tm_file->tags_array->pdata[i]);
		cmp = strcmp(name, tag->name);
		if (0 == cmp)
		{
			if ((!scope || !tag->atts.entry.scope) || (0 == strcmp(scope, tag->atts.entry.scope)))
			{
				text_editor_goto_line(te, tag->atts.entry.line, TRUE);
				g_free(scope?scope:name);
				return TRUE;
			}
		}
		else if (cmp < 0)
		{
			g_free(scope?scope:name);
			return FALSE;
		}
	}
	g_free(scope?scope:name);
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

static void
anjuta_show_text_editor (TextEditor * te)
{
	GtkWidget *page;
	GList *node;
	gint i;

	if (te == NULL)
		return;
	if (te->mode == TEXT_EDITOR_WINDOWED)
	{
		gdk_window_raise (te->widgets.window->window);
		anjuta_set_current_text_editor (te);
		return;
	}
	node = GTK_NOTEBOOK (app->widgets.notebook)->children;
	i = 0;
	while (node)
	{
		TextEditor *t;

		page =
			gtk_notebook_get_nth_page (GTK_NOTEBOOK
						   (app->widgets.notebook),
						   i);
		t = gtk_object_get_data (GTK_OBJECT (page), "TextEditor");
		if (t == te && GTK_IS_EVENT_BOX (page) && t)
		{
			gint page_num;
			page_num =
				gtk_notebook_page_num (GTK_NOTEBOOK
						       (app->widgets.
							notebook), page);
			gtk_notebook_set_page (GTK_NOTEBOOK
					       (app->widgets.notebook),
					       page_num);
			anjuta_set_current_text_editor (te);
			return;
		}
		i++;
		node = g_list_next (node);
	}
}

void
anjuta_show ()
{
	PropsID pr;
	gchar* key;

	pr = app->preferences->props;

	gtk_widget_show (app->widgets.window);

	/* Editor stuffs */
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
					(app->widgets.menubar.view.
					 editor_linenos), prop_get_int (pr,
									"margin.linenumber.visible",
									0));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
					(app->widgets.menubar.view.
					 editor_markers), prop_get_int (pr,
									"margin.marker.visible",
									0));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
					(app->widgets.menubar.view.
					 editor_folds), prop_get_int (pr,
								      "margin.fold.visible",
								      0));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
					(app->widgets.menubar.view.
					 editor_indentguides),
					prop_get_int (pr,
						      "view.indentation.guides",
						      0));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
					(app->widgets.menubar.view.
					 editor_whitespaces),
					prop_get_int (pr, "view.whitespace",
						      0));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
					(app->widgets.menubar.view.
					 editor_eolchars), prop_get_int (pr,
									 "view.eol",
									 0));


	/* Hide all toolbars, since corrosponding toggle menu items */
	/* (in the view submenu) are all initially off */
	/* We do not resize or set the change in props */
	anjuta_toolbar_set_view (ANJUTA_MAIN_TOOLBAR, FALSE, FALSE, FALSE);
	anjuta_toolbar_set_view (ANJUTA_EXTENDED_TOOLBAR, FALSE, FALSE, FALSE);
	anjuta_toolbar_set_view (ANJUTA_DEBUG_TOOLBAR, FALSE, FALSE, FALSE);
	anjuta_toolbar_set_view (ANJUTA_BROWSER_TOOLBAR, FALSE, FALSE, FALSE);
	anjuta_toolbar_set_view (ANJUTA_FORMAT_TOOLBAR, FALSE, FALSE, FALSE);

	/* Now we are synced with the menu items */
	/* Show/Hide the necessary toolbars */
	key = g_strconcat (ANJUTA_MAIN_TOOLBAR, ".visible", NULL);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
			(app->widgets.menubar. view.main_toolbar),
			prop_get_int (pr, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_EXTENDED_TOOLBAR, ".visible", NULL);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
			(app->widgets.menubar. view.extended_toolbar),
			prop_get_int (pr, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_DEBUG_TOOLBAR, ".visible", NULL);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
			(app->widgets.menubar. view.debug_toolbar),
			prop_get_int (pr, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_BROWSER_TOOLBAR, ".visible", NULL);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
			(app->widgets.menubar. view.browser_toolbar),
			prop_get_int (pr, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_FORMAT_TOOLBAR, ".visible", NULL);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
			(app->widgets.menubar. view.format_toolbar),
			prop_get_int (pr, key, 1));
	g_free (key);
	
	update_gtk ();
	
	/*
	anjuta_message_manager_append (app->messages,
			 _("CVS is not yet implemented. :-(\n"), MESSAGE_CVS);
	*/
	
	if (app->dirs->first_time)
	{
		gchar *file;
		app->dirs->first_time = FALSE;
		file = g_strconcat (app->dirs->data, "/welcome.txt", NULL);
		anjuta_info_show_file (file, 500, 435);
		g_free (file);
	}
}

void
anjuta_save_settings ()
{
	gchar *buffer;
	FILE *stream;

	if (!app)
		return;
	buffer =
		g_strconcat (app->dirs->settings, "/session.properties",
			     NULL);
	stream = fopen (buffer, "w");
	g_free (buffer);
	if (stream)
	{
		anjuta_save_yourself (stream);
		fclose (stream);
	}
}

gboolean anjuta_save_yourself (FILE * stream)
{
#ifdef	USE_STD_PREFERENCES
	GList *node;
#endif
	PropsID pr;
	gchar* key;

	pr = app->preferences->props;

	gdk_window_get_root_origin (app->widgets.window->window,
				    &app->win_pos_x, &app->win_pos_y);
	gdk_window_get_size (app->widgets.window->window, &app->win_width,
			     &app->win_height);
	fprintf (stream,
		 _("# * DO NOT EDIT OR RENAME THIS FILE ** Generated by Anjuta **\n"));
	fprintf (stream, "anjuta.version=%s\n", VERSION);
	fprintf (stream, "anjuta.win.pos.x=%d\n", app->win_pos_x);
	fprintf (stream, "anjuta.win.pos.y=%d\n", app->win_pos_y);
	fprintf (stream, "anjuta.win.width=%d\n", app->win_width);
	fprintf (stream, "anjuta.win.height=%d\n", app->win_height);
	fprintf (stream, "anjuta.vpaned.size=%d\n",
		 GTK_PANED (app->widgets.vpaned)->child1_size);
	fprintf (stream, "anjuta.hpaned.size=%d\n",
		 GTK_PANED (app->widgets.hpaned)->child1_size);

	key = g_strconcat (ANJUTA_MAIN_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key, prop_get_int (app->preferences->props, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_EXTENDED_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key, prop_get_int (app->preferences->props, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_FORMAT_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key, prop_get_int (app->preferences->props, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_DEBUG_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key, prop_get_int (app->preferences->props, key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_BROWSER_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key, prop_get_int (app->preferences->props, key, 1));
	g_free (key);

	fprintf (stream, "view.eol=%d\n", prop_get_int (pr, "view.eol", 0));
	fprintf (stream, "view.indentation.guides=%d\n",
		 prop_get_int (pr, "view.indentation.guides", 0));
	fprintf (stream, "view.whitespace=%d\n",
		 prop_get_int (pr, "view.whitespace", 0));
	fprintf (stream, "view.indentation.whitespace=%d\n",
		 prop_get_int (pr, "view.indentation.whitespace", 0));
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
		if( 		app->project_dbase->project_is_open 
			&&	( NULL != app->project_dbase->proj_filename ) )
			last_open_project_path = app->project_dbase->proj_filename ;
			
		fprintf (stream, "%s\n\n", last_open_project_path );
	}
	
	anjuta_message_manager_save_yourself (app->messages, stream);
	project_dbase_save_yourself (app->project_dbase, stream);

	compiler_options_save_yourself (app->compiler_options, stream);
	if (app->project_dbase->project_is_open == FALSE)
		compiler_options_save (app->compiler_options, stream);

	command_editor_save_yourself (app->command_editor, stream);
	command_editor_save (app->command_editor, stream);

	src_paths_save_yourself (app->src_paths, stream);
	if (app->project_dbase->project_is_open == FALSE)
		src_paths_save (app->src_paths, stream);

	find_replace_save_yourself (app->find_replace, stream);
	debugger_save_yourself (stream);
	preferences_save_yourself (app->preferences, stream);
	return TRUE;
}

gboolean anjuta_load_yourself (PropsID pr)
{
	gint length;

	preferences_load_yourself (app->preferences, pr);
	
	app->win_pos_x = prop_get_int (pr, "anjuta.win.pos.x", 10);
	app->win_pos_y = prop_get_int (pr, "anjuta.win.pos.y", 10);
	app->win_width =
		prop_get_int (pr, "anjuta.win.width",
			      gdk_screen_width () - 10);
	app->win_height =
		prop_get_int (pr, "anjuta.win.height",
			      gdk_screen_height () - 25);
	length =
		prop_get_int (pr, "anjuta.vpaned.size",
			      app->win_height / 2 + 7);
	gtk_paned_set_position (GTK_PANED (app->widgets.vpaned), length);
	length = prop_get_int (pr, "anjuta.hpaned.size", app->win_width / 4);
	gtk_paned_set_position (GTK_PANED (app->widgets.hpaned), length);

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
	app->recent_projects = anjuta_session_load_strings( SECSTR(SECTION_RECENTPROJECTS), NULL );
#endif
	app->last_open_project = prop_get( pr, ANJUTA_LAST_OPEN_PROJECT );

	anjuta_message_manager_load_yourself (app->messages, pr);
	project_dbase_load_yourself (app->project_dbase, pr);
	compiler_options_load_yourself (app->compiler_options, pr);
	compiler_options_load (app->compiler_options, pr);
	src_paths_load_yourself (app->src_paths, pr);
	src_paths_load (app->src_paths, pr);
	find_replace_load_yourself (app->find_replace, pr);
	debugger_load_yourself (pr);
	return TRUE;
}

void
anjuta_save_all_files()
{
	TextEditor *te;
	int i;
	for (i = 0; i < g_list_length (app->text_editor_list); i++)
	{
		te = g_list_nth_data (app->text_editor_list, i);
		if (te->full_filename && !text_editor_is_saved (te))
			text_editor_save_file (te);
	}
	anjuta_status (_("All files saved ..."));
}

void
anjuta_update_title ()
{
	gchar *buff1, *buff2;
	GtkWidget *window;
	TextEditor *te;
	te = anjuta_get_current_text_editor ();
	if (te)
	{
		if (te->mode == TEXT_EDITOR_PAGED)
		{
			window = app->widgets.window;
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
		gtk_window_set_title (GTK_WINDOW (app->widgets.window),
				      _("Anjuta: No file"));
	}
}

void
anjuta_update_page_label (TextEditor *te)
{
	GtkRcStyle *rc_style;
	GdkColor tmpcolor;

	if (te == NULL)
		return;
	
	if (te->mode == TEXT_EDITOR_WINDOWED)
		return;
	
	if (te->widgets.tab_label == NULL)
		return;
			
	if (text_editor_is_saved(te))
	{
		gdk_color_parse("black",&tmpcolor);
	}
	else
	{
		gdk_color_parse("red",&tmpcolor);
	}
	
	rc_style = gtk_rc_style_new();
			
	rc_style->fg[GTK_STATE_NORMAL] = tmpcolor;
	rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_FG;

	gtk_widget_modify_style(te->widgets.tab_label, rc_style);
	
	gtk_rc_style_unref(rc_style);
}

void
anjuta_apply_preferences (void)
{
	TextEditor *te;
	gint i;
	gint no_tag;

	Preferences *pr = app->preferences;
	
	app->bUseComponentUI = preferences_get_int (pr, USE_COMPONENTS);
	app->b_reload_last_project	= preferences_get_int (pr , RELOAD_LAST_PROJECT);
	
	no_tag = preferences_get_int (pr, EDITOR_TAG_HIDE);
	if (no_tag == TRUE)
	{
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK
					    (app->widgets.notebook), FALSE);
	}
	else
	{
		gchar *tag_pos;

		tag_pos = preferences_get (pr, EDITOR_TAG_POS);

		if (strcmp (tag_pos, "bottom") == 0)
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK
						  (app->widgets.notebook),
						  GTK_POS_BOTTOM);
		else if (strcmp (tag_pos, "left") == 0)
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK
						  (app->widgets.notebook),
						  GTK_POS_LEFT);
		else if (strcmp (tag_pos, "right") == 0)
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK
						  (app->widgets.notebook),
						  GTK_POS_RIGHT);
		else
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK
						  (app->widgets.notebook),
						  GTK_POS_TOP);
		g_free (tag_pos);
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK
					    (app->widgets.notebook), TRUE);
	}
	anjuta_message_manager_update(app->messages);
	for (i = 0; i < g_list_length (app->text_editor_list); i++)
	{
		te =	(TextEditor*) (g_list_nth (app->text_editor_list, i)->data);
		text_editor_update_preferences (te);
		anjuta_refresh_breakpoints (te);
	}
}

void
anjuta_refresh_breakpoints (TextEditor* te)
{
	breakpoints_dbase_clear_all_in_editor (debugger.breakpoints_dbase, te);
	breakpoints_dbase_set_all_in_editor (debugger.breakpoints_dbase, te);
}

void
anjuta_application_exit(void)
{
	g_return_if_fail(app != NULL );
	if(		( NULL != app->project_dbase )
		&&	(app->project_dbase->project_is_open ) )
	{
		project_dbase_close_project(app->project_dbase) ;
	}
	free_plug_ins( app->addIns_list );
	app->addIns_list	= NULL ;
}

void
anjuta_clean_exit ()
{
	gchar *tmp;
	pid_t pid;

	g_return_if_fail (app != NULL);

	/* This will remove all tmp files */
	tmp =
		g_strdup_printf ("rm -f %s/anjuta_*.%ld",
				 app->dirs->tmp, (long) getpid ());
	pid = gnome_execute_shell (app->dirs->home, tmp);
	if (-1 == pid)
	{
		perror("Cleanup failed");
		exit(1);
	}
	waitpid (pid, NULL, 0);

/* Is it necessary to free up all the memos on exit? */
/* Assuming that it is not, I am disabling the following */
/* Basically, because it is faster */

#if 0				/* From here */
	if (app->project_dbase->project_is_open)
		project_dbase_close_project (app->project_dbase);
	debugger_stop ();

	gtk_widget_hide (app->widgets.notebook);
	for (i = 0;
	     i <
	     g_list_length (GTK_NOTEBOOK (app->widgets.notebook)->
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
	if (app->find_replace)
		find_replace_destroy (app->find_replace);
	if (app->find_in_files)
		find_in_files_destroy (app->find_in_files);
	if (app->dirs)
		anjuta_dirs_destroy (app->dirs);
	if (app->executer)
		executer_destroy (app->executer);
	if (app->configurer)
		configurer_destroy (app->configurer);
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
anjuta_status (gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gnome_app_flash (GNOME_APP (app->widgets.window), message);
}

void
anjuta_warning (gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gnome_app_warning (GNOME_APP (app->widgets.window), message);
	g_free (message);
}

void
anjuta_error (gchar * mesg, ... )
{
	gchar* message;
	gchar* str;
	va_list args;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	str = g_strconcat (_("ERROR: "), message, NULL);
	gnome_app_error (GNOME_APP (app->widgets.window), str);
	g_free (message);
	g_free (str);
}

void
anjuta_system_error (gint errnum, gchar * mesg, ... )
{
	gchar* message;
	gchar* tot_mesg;
	va_list args;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	tot_mesg = g_strconcat (message, _("\nSystem: "), g_strerror(errnum), NULL);
	gnome_app_error (GNOME_APP (app->widgets.window), tot_mesg);
	g_free (message);
	g_free (tot_mesg);
}

void
anjuta_set_file_properties (gchar* file)
{
	gchar *full_filename, *dir, *filename, *ext;
	PropsID props;

	g_return_if_fail (file != NULL);
	full_filename = g_strdup (file);
	props = app->preferences->props;
	dir = g_dirname (full_filename);
	filename = extract_filename (full_filename);
	ext = get_file_extension (filename);
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

void
anjuta_open_file (gchar* fname)
{
	Preferences *pref;
	pid_t pid;
	gchar *cmd;
	GList *list;

	g_return_if_fail (fname != NULL);
	pref = app->preferences;
	anjuta_set_file_properties (fname);
	cmd = command_editor_get_command_file (app->command_editor, COMMAND_OPEN_FILE, fname);
	list = glist_from_string (cmd);
	g_free (cmd);
	if(list == NULL)
	{
		anjuta_goto_file_line_mark (fname, -1, FALSE);
		return;
	}
	if (anjuta_is_installed ((gchar*)list->data, TRUE))
	{
		gchar **av, *dir;
		guint length;
		gint i;
		
		length = g_list_length (list);
		av = g_malloc (sizeof (gchar*) * length);
		for (i=0; i<length; i++)
		{
			av[i] = g_list_nth_data (list, i);
		}
		dir = prop_get_new_expand (pref->props, CURRENT_FILE_DIRECTORY,
			app->dirs->home);
		chdir(dir);
		pid = gnome_execute_async (dir, length, av);
		g_free (dir);
		glist_strings_free (list);
		return;
	}
	anjuta_goto_file_line (fname, -1);
}

void
anjuta_view_file (gchar* fname)
{
	Preferences *pref;
	pid_t pid;
	gchar *cmd;
	GList *list;

	g_return_if_fail (fname != NULL);
	pref = app->preferences;
	anjuta_set_file_properties (fname);
	cmd = command_editor_get_command_file (app->command_editor, COMMAND_VIEW_FILE, fname);
	list = glist_from_string (cmd);
	g_free (cmd);
	if(list == NULL)
	{
		anjuta_goto_file_line (fname, -1);
		return;
	}
	if (anjuta_is_installed ((gchar*)list->data, TRUE))
	{
		gchar **av, *dir;
		guint length;
		gint i;
		
		length = g_list_length (list);
		av = g_malloc (sizeof (gchar*) * length);
		for (i=0; i<length; i++)
		{
			av[i] = g_list_nth_data (list, i);
		}
		dir = prop_get_new_expand (pref->props, CURRENT_FILE_DIRECTORY,
			app->dirs->home);
		pid = gnome_execute_async (dir, length, av);
		g_free (dir);
		glist_strings_free (list);
		return;
	}
	anjuta_goto_file_line (fname, -1);
}

gchar *
anjuta_get_full_filename (gchar * fn)
{
	gchar *cur_dir, *dummy;
	gchar *real_path;
	GList *list;
	gchar *text;
	gint i;
	
	if (!fn)
		return NULL;
	if (fn[0] == '/')
		return tm_get_real_path (fn);

	if( app->execution_dir)
	{
		dummy = g_strconcat (app->execution_dir, "/", fn, NULL);
		real_path = tm_get_real_path(dummy);
		g_free(dummy);
		if (file_is_regular (real_path) == TRUE)
			return real_path;
		g_free (real_path);
	}
	cur_dir = g_get_current_dir ();
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

	list = GTK_CLIST (app->src_paths->widgets.src_clist)->row_list;
	for (i = 0; i < g_list_length (list); i++)
	{
		gtk_clist_get_text (GTK_CLIST
				    (app->src_paths->widgets.src_clist), i, 0,
				    &text);
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
			return real_path;
		}
		g_free (real_path);
	}
	dummy = g_strconcat (cur_dir, "/", fn, NULL);
	real_path = tm_get_real_path(dummy);
	g_free(dummy);

	g_free (cur_dir);
	return real_path;
}

gboolean
anjuta_init_progress (gchar * description, gdouble full_value,
		      GnomeAppProgressCancelFunc cancel_cb, gpointer data)
{
	if (app->in_progress)
		return FALSE;
	app->in_progress = TRUE;
	app->progress_value = 0.0;
	app->progress_full_value = full_value;
	app->progress_cancel_cb = cancel_cb;
	app->progress_key =
		gnome_app_progress_timeout (GNOME_APP (app->widgets.window),
					    description,
					    100,
					    on_anjuta_progress_cb,
					    on_anjuta_progress_cancel, data);
	return TRUE;
}

void
anjuta_set_progress (gdouble value)
{
	if (app->in_progress)

		app->progress_value = value / app->progress_full_value;
}

void
anjuta_done_progress (gchar * end_mesg)
{
	if (app->in_progress)
		gnome_app_progress_done (app->progress_key);
	anjuta_status (end_mesg);
	app->in_progress = FALSE;
}

void
anjuta_not_implemented (char *file, guint line)
{
	gchar *mesg =
		g_strdup_printf (N_
				 ("Not yet implemented.\nInsert code at \"%s:%u\""),
				 file, line);
	messagebox (GNOME_MESSAGE_BOX_INFO, _(mesg));
	g_free (mesg);
}

void
anjuta_set_busy ()
{
	GnomeAnimator *led;
	GList *node;
	
	led = GNOME_ANIMATOR (app->widgets.toolbar.main_toolbar.led);
	app->busy_count++;
	if (app->busy_count > 1)
		return;
	if (app_cursor)
		gdk_cursor_destroy (app_cursor);
	gnome_animator_stop (led);
	gnome_animator_goto_frame (led, 1);
	app_cursor = gdk_cursor_new (GDK_WATCH);
	if (app->widgets.window->window)
		gdk_window_set_cursor (app->widgets.window->window, app_cursor);
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
anjuta_set_active ()
{
	GList* node;
	
	app->busy_count--;
	if (app->busy_count > 0)
		return;
	app->busy_count = 0;
	if (app_cursor)
		gdk_cursor_destroy (app_cursor);
	app_cursor = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
	gdk_window_set_cursor (app->widgets.window->window, app_cursor);
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
anjuta_grab_text_focus ()
{
	GtkWidget *text;
	text = GTK_WIDGET (anjuta_get_current_text ());
	g_return_if_fail (text != NULL);
	scintilla_send_message (SCINTILLA (text), SCI_GRABFOCUS, 0, 0);
}

void
anjuta_register_window (GtkWidget * win)
{
	app->registered_windows =
		g_list_append (app->registered_windows, win);
}

void
anjuta_unregister_window (GtkWidget * win)
{
	app->registered_windows =
		g_list_remove (app->registered_windows, win);
}

void
anjuta_foreach_windows (GFunc cb, gpointer data)
{
	g_list_foreach (app->registered_windows, cb, data);
}

void
anjuta_register_child_process (pid_t pid,
			       void (*ch_terminated) (int s,
						      gpointer d),
			       gpointer data)
{
	if (pid < 1)
		return;
	app->registered_child_processes =
		g_list_append (app->registered_child_processes, (int *) pid);
	app->registered_child_processes_cb =
		g_list_append (app->registered_child_processes_cb,
			       ch_terminated);
	app->registered_child_processes_cb_data =
		g_list_append (app->registered_child_processes_cb_data, data);
}

void
anjuta_unregister_child_process (pid_t pid)
{
	gint index;
	index = g_list_index (app->registered_child_processes, (int *) pid);
	app->registered_child_processes =
		g_list_remove (app->registered_child_processes, (int *) pid);
	app->registered_child_processes_cb =
		g_list_remove (app->registered_child_processes_cb,
			       g_list_nth_data (app->
						registered_child_processes_cb,
						index));
	app->registered_child_processes_cb_data =
		g_list_remove (app->registered_child_processes_cb_data,
			       g_list_nth_data (app->

						registered_child_processes_cb_data,
						index));}

void
anjuta_foreach_child_processes (GFunc cb, gpointer data)
{
	g_list_foreach (app->registered_child_processes, cb, data);
}

void
anjuta_child_terminated (int t)
{
	int status;
	gint index;
	pid_t pid;
	int (*callback) (int st, gpointer d);
	gpointer cb_data;
	pid = waitpid (0, &status, WNOHANG);
	if (pid < 1)
		return;
	index = g_list_index (app->registered_child_processes, (int *) pid);
	if (index < 0)
		return;
	callback =
		g_list_nth_data (app->registered_child_processes_cb, index);
	cb_data = g_list_nth_data (app->registered_child_processes_cb, index);
	if (callback)
		(*callback) (status, cb_data);
	anjuta_unregister_child_process (pid);
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
				fileselection_set_filename (app->project_dbase->fileselection_open, node->data);
				project_dbase_load_project (app->project_dbase, TRUE);
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

void
anjuta_clear_windows_menu ()
{
	guint count;
	count = g_list_length (app->text_editor_list);
	if (count < 1)
		return;
	gnome_app_remove_menus (GNOME_APP (app->widgets.window),
				"_Windows/<Separator>", count + 1);
}

void
anjuta_delete_all_marker (gint marker)
{
	GList *node;
	node = app->text_editor_list;
	while (node)
	{
		TextEditor* te;
		te = node->data;
		scintilla_send_message (SCINTILLA (te->widgets.editor), SCI_MARKERDELETEALL,
			marker, 0);
		node = g_list_next (node);
	}
}

gboolean anjuta_set_auto_gtk_update (gboolean auto_flag)
{
	gboolean save;
	save = app->auto_gtk_update;

	app->auto_gtk_update = auto_flag;
	return save;
}

gboolean anjuta_is_installed (gchar * prog, gboolean show)
{
	if (gnome_is_program_in_path (prog))
		return TRUE;
	if (show)
	{
		anjuta_error (_
					 ("The \"%s\" utility is not installed.\n"
					  "Install it and then restart Anjuta."), prog);
	}
	return FALSE;
}

void
anjuta_update_app_status (gboolean set_job, gchar* job_name)
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
	zoom = prop_get_int (app->preferences->props, "text.zoom.factor", 0);
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
	gnome_appbar_set_default (GNOME_APPBAR (app->widgets.appbar), str);
	g_free (str);
	g_free (prj);
	g_free (edit);
	g_free (job);
	g_free (mode);
}

void
anjuta_toolbar_set_view (gchar* toolbar_name, gboolean view, gboolean resize, gboolean set_in_props)
{
	gchar* key;
	GnomeDock* dock;
	GnomeDockItem* item;

	if (set_in_props)
	{
		key = g_strconcat (toolbar_name, ".visible", NULL);
		preferences_set_int (app->preferences, key, view);
		g_free (key);
	}

	item = gnome_app_get_dock_item_by_name (GNOME_APP (app->widgets.window), toolbar_name);

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_DOCK_ITEM (item));

	if (view)
		gtk_widget_show (GTK_WIDGET (item));
	else
		gtk_widget_hide (GTK_WIDGET (item));
	dock = gnome_app_get_dock(GNOME_APP(app->widgets.window));
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

	value_str = prop_get_new_expand (app->preferences->props, key, filename);
	if (!value_str)
		return default_value;
	value = atoi (value_str);
	return value;
}

TextEditor *
anjuta_get_te_from_path( const gchar *szFullPath )
{
	GList *node;

	TextEditor *te;

	g_return_val_if_fail ( szFullPath != NULL, NULL );
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) node->data;
		if (te->full_filename != NULL)
		{
			if ( !strcmp ( szFullPath, te->full_filename) )
			{
				return te ;
			}
		}
		node = g_list_next (node);
	}
	return NULL ;
}

/* Saves a file to keep it synchronized with external programs */
void 
anjuta_save_file_if_modified( const gchar *szFullPath )
{
	TextEditor *te;

	g_return_if_fail ( szFullPath != NULL );

	te = anjuta_get_te_from_path( szFullPath );
	if( NULL != te )
	{
		if( !text_editor_is_saved (te) )
		{
			text_editor_save_file (te);
		}
	}
	return;
}

/* If an external program changed the file, we must reload it */
void 
anjuta_reload_file( const gchar *szFullPath )
{
	TextEditor *te;

	g_return_if_fail ( szFullPath != NULL );

	te = anjuta_get_te_from_path( szFullPath );
	if( NULL != te )
	{
		glong	nNowPos = te->current_line ;
		/*text_editor_check_disk_status ( te, TRUE );asd sdf*/
		text_editor_load_file (te);
		text_editor_goto_line ( te,  nNowPos, FALSE );
	}
	return;
}



AnjutaAddInPtr plug_in_new(void)
{
	AnjutaAddInPtr	self = g_new( AnjutaAddIn, 1 );
	if( NULL != self )
	{
		self->m_Handle			= NULL ;
		self->m_szModName		= NULL ;
		self->m_bStarted		= FALSE ;
		self->m_UserData		= NULL ;		
	
		self->GetDescr			= NULL ;
		self->GetVersion		= NULL ;
		self->Init				= NULL ;
		self->CleanUp			= NULL ;
		self->Activate			= NULL ;
		self->GetMenuTitle		= NULL ;
		self->GetTooltipText	= NULL ;
	}
	return self ;
}

void 
plug_in_delete( AnjutaAddInPtr self )
{
	g_return_if_fail( NULL != self );
	
	if( self->m_Handle && self->CleanUp )
		(*self->CleanUp)( self->m_Handle, self->m_UserData, app );

	if( NULL != self->m_szModName )
		g_free( self->m_szModName );

	if( self->m_Handle )
		g_module_close( self->m_Handle );

	self->m_Handle	= NULL ;
	
	g_free( self );
}


static PlugInErr
plug_in_init( AnjutaAddInPtr self, const gchar *szModName )
{
	PlugInErr	pieError = PIE_OK ;
	
	g_return_val_if_fail( (NULL != self), PIE_BADPARMS );
	g_return_val_if_fail( (szModName != NULL ) && strlen(szModName) , PIE_BADPARMS ) ;
	
	self->m_szModName	= g_strdup( szModName ) ;
	self->m_Handle		= g_module_open ( szModName , 0 );
	if( NULL != self->m_Handle )
	{
		/* Symbol load */
		if( 		g_module_symbol( self->m_Handle, "GetDescr", (gpointer*)&self->GetDescr ) 
			&&	g_module_symbol( self->m_Handle, "GetVersion", (gpointer*)&self->GetVersion )
			&&	g_module_symbol( self->m_Handle, "Init", (gpointer*)&self->Init )
			&&	g_module_symbol( self->m_Handle, "CleanUp", (gpointer*)&self->CleanUp )
			&&	g_module_symbol( self->m_Handle, "Activate", (gpointer*)&self->Activate )
			&&	g_module_symbol( self->m_Handle, "GetMenuTitle", (gpointer*)&self->GetMenuTitle )
			&&	g_module_symbol( self->m_Handle, "GetTooltipText", (gpointer*)&self->GetTooltipText ) )
		{
			self->m_bStarted = (*self->Init)( self->m_Handle, &self->m_UserData, app ) ;
			if( !self->m_bStarted )
			{
				pieError = PIE_INITFAILED ;
			}			
		} else
		{
			self->GetDescr		= NULL ;
			self->GetVersion	= NULL ;
			self->Init			= NULL ;
			self->CleanUp		= NULL ;
			self->Activate		= NULL ;
			self->GetMenuTitle	= NULL ;
			self->m_bStarted	= FALSE ;
			pieError = PIE_SYMBOLSNOTFOUND ;
		}
	} else
		pieError = PIE_NOTLOADED ;
	return pieError ;
}

static gboolean
Load_plugIn( AnjutaAddInPtr self, const gchar *szModName )
{
	gboolean		bOK = FALSE ;
	PlugInErr	lErr = plug_in_init( self, szModName ) ;
	const gchar	*pErr = g_module_error();
	if( NULL == pErr )
		pErr = "";
	switch( lErr )
	{
	default:
		syslog( LOG_WARNING|LOG_USER, _("Unable to load plugin %s. Error: %s"), szModName, pErr );
		break;
	case PIE_NOTLOADED:
		syslog( LOG_WARNING|LOG_USER, _("Unable to load plugin %s. Error: %s"), szModName, pErr );
		break;
	case PIE_SYMBOLSNOTFOUND:
		syslog( LOG_WARNING|LOG_USER, _("Plugin protocol %s unknown Error:%s"), szModName, pErr );
		break;
	case PIE_INITFAILED:
		syslog( LOG_WARNING|LOG_USER, _("Unable to start plugin %s"), szModName );
		break;
	case PIE_BADPARMS:
		syslog( LOG_WARNING|LOG_USER, _("Bad parameters to plugin %s. Error: %s"), szModName, pErr );
		break;
	case PIE_OK:
		bOK = TRUE ;
		break;
	}
		
	if( ! bOK )
		anjuta_error ( _("Unable to load plugin %s.\nError: %s"), szModName, pErr  );
	return bOK ;
}


static void
plugins_foreach_delete( gpointer data, gpointer user_data )
{
	plug_in_delete( (AnjutaAddInPtr)data );
}

static void
free_plug_ins( GList * pList )
{
	if( pList )
	{
		g_list_foreach( pList, plugins_foreach_delete, NULL );
		g_list_free (pList);
	}
}

static int
select_all_files (const struct dirent *e)
{
	return TRUE; 
}


static GList *
scan_AddIns_in_directory (AnjutaApp* pApp, const gchar *szDirName, GList *pList)
{
	GList *files;
	GList *node;

	if (!g_module_supported ())
		return NULL ;
	
	g_return_val_if_fail (pApp != NULL, pList);
	g_return_val_if_fail (((NULL != szDirName) && strlen(szDirName)), pList);

	files = NULL;

	/* Can not use p->top_proj_dir yet. */

	/* Can not use project_dbase_get_module_dir() yet */
	files = scan_files_in_dir (szDirName, select_all_files);

	node = files;
	while (node)
	{
		const char* entry_name;
		gboolean bOK = FALSE;
			
		entry_name = (const char*) node->data;
		node = g_list_next (node);

		if ((NULL != entry_name) && strlen (entry_name) > 3
			&& (0 == strcmp (&entry_name[strlen(entry_name)-3], ".so")))
		{
			/* FIXME: e questo ? bah library_file = g_module_build_path (module_dir, module_file); */
			gchar *pPIPath = g_strdup_printf ("%s/%s", szDirName, entry_name);
			AnjutaAddInPtr pPlugIn = plug_in_new ();

			if ((NULL != pPlugIn) && (NULL != pPIPath))
			{
				if (Load_plugIn (pPlugIn, pPIPath))
				{
					pList = g_list_prepend (pList, pPlugIn);
					bOK = TRUE ;
				}
			} else
				anjuta_error (_("Out of memory scanning for plugins."));

			g_free (pPIPath);

			if (!bOK && (NULL != pPlugIn))
				plug_in_delete (pPlugIn);
		}
	}

	free_string_list (files);

	return pList;
}

static void on_message_clicked(GtkObject* obj, char* message)
{
	char* fn;
	int ln;
	if (parse_error_line (message, &fn, &ln))
	{
		anjuta_goto_file_line (fn, ln);
		g_free (fn);
	}
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
	fileselection_set_filename (app->project_dbase->fileselection_open, (gchar*)szProjectPath);
	project_dbase_load_project (app->project_dbase, TRUE);
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
	
	if(		(NULL!=app->last_open_project ) 
		&&	strlen(app->last_open_project ) )
	{
#ifdef	DEBUG_LOAD_PROJECTS	/* debug code, may return useful */
		char	szppp[1024];
		sprintf( szppp, "%d, >>%s<<", strlen(app->last_open_project) , app->last_open_project );
		MessageBox( szppp );
#endif
		anjuta_load_this_project( app->last_open_project );
	}
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <string.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime.h>

#include "anjuta.h"
#include "resources.h"
#include "mainmenu_callbacks.h"
#include "pixmaps.h"
#include "cvs_gui.h"
#include "properties.h"

#include "an_file_view.h"

enum {
	PIXBUF_COLUMN,
	FILENAME_COLUMN,
	REV_COLUMN,
	COLUMNS_NB
};

/* LibGlade auto-signal-connect callbacks. Do not declare static. */
gboolean on_file_filter_delete_event (GtkWidget *widget,
									  GdkEventCrossing *event,
									  gpointer user_data);
void on_file_filter_response (GtkWidget *dlg, gint res, gpointer user_data);
void on_file_filter_close (GtkWidget *dlg, gpointer user_data);

static AnFileView *fv = NULL;

gboolean
anjuta_fv_open_file (const char *path, gboolean use_anjuta)
{
	gboolean status = FALSE;
	const char *mime_type = gnome_vfs_get_file_mime_type(path, NULL, FALSE);
	if (use_anjuta)
	{
		anjuta_goto_file_line_mark((char *) path, -1, FALSE);
		status = TRUE;
	}
	else
	{
		GnomeVFSMimeApplication *app = gnome_vfs_mime_get_default_application(mime_type);
		if ((app) && (app->command))
		{
			if (NULL != strstr (app->command, "anjuta"))
				anjuta_goto_file_line_mark ((char *) path, -1, FALSE);
			else
			{
				char **argv = g_new(char *, 3);

				argv[0] = app->command;
				argv[1] = (char *) path;
				argv[2] = NULL;

				if (-1 == gnome_execute_async (NULL, 2, argv))
					anjuta_warning(_("Unable to open %s in %s"), path, app->command);

				g_free(argv);
			}

			gnome_vfs_mime_application_free(app);
			status = TRUE;
		}
		else
		{
			anjuta_warning(_("No default viewer specified for the mime type %s.\n"
					"Please set it in GNOME control center"), mime_type);
		}
	}
	return status;
}

typedef enum {
	OPEN,
	VIEW,
	REFRESH,
	CUSTOMIZE,
	MENU_MAX
} FVSignal;

/* File browser customization */
#define FILE_FILTER_DIALOG "dialog.file.filter"
#define OK_BUTTON "button.ok"
#define FILE_FILTER_MATCH "filter.file.match"
#define FILE_FILTER_MATCH_COMBO "filter.file.match.combo"
#define FILE_FILTER_UNMATCH "filter.file.unmatch"
#define FILE_FILTER_UNMATCH_COMBO "filter.file.unmatch.combo"
#define FILE_FILTER_IGNORE_HIDDEN "filter.file.ignore.hidden"
#define DIR_FILTER_MATCH "filter.dir.match"
#define DIR_FILTER_MATCH_COMBO "filter.dir.match.combo"
#define DIR_FILTER_UNMATCH "filter.dir.unmatch"
#define DIR_FILTER_UNMATCH_COMBO "filter.dir.unmatch.combo"
#define DIR_FILTER_IGNORE_HIDDEN "filter.dir.ignore.hidden"

typedef struct _FileFilter
{
	GladeXML *xml;
	GtkWindow *dialog;
	GtkEditable *file_match_en;
	GtkEditable *file_unmatch_en;
	GtkEditable *dir_match_en;
	GtkEditable *dir_unmatch_en;
	GtkCombo *file_match_combo;
	GtkCombo *file_unmatch_combo;
	GtkToggleButton *ignore_hidden_files_tb;
	GtkCombo *dir_match_combo;
	GtkCombo *dir_unmatch_combo;
	GtkToggleButton *ignore_hidden_dirs_tb;
	GList *file_match;
	GList *file_unmatch;
	GList *dir_match;
	GList *dir_unmatch;
	GList *file_match_strings;
	GList *file_unmatch_strings;
	GList *dir_match_strings;
	GList *dir_unmatch_strings;
	gboolean ignore_hidden_files;
	gboolean ignore_hidden_dirs;
	gboolean showing;
} FileFilter;

static FileFilter *ff = NULL;

#define APPLY_PREF(var, P) \
	s = gtk_editable_get_chars(ff->var ## _en, 0, -1); \
	if (ff->var) \
		glist_strings_free(ff->var); \
	prop_set_with_key(p, P, s); \
	ff->var = glist_from_string(s); \
	update_string_list(ff->var ## _strings, s, 10); \
	if (ff->var ## _strings) \
		gtk_combo_set_popdown_strings(ff->var ## _combo, ff->var ## _strings); \
	g_free(s)

#define APPLY_PREF_BOOL(var, P) \
	ff->var = gtk_toggle_button_get_active(ff->var ## _tb); \
	prop_set_int_with_key(p, P, ff->var)

static void fv_prefs_apply(void)
{
	if (ff)
	{
		PropsID p = app->project_dbase->props;
		char *s;
		APPLY_PREF(file_match, FILE_FILTER_MATCH);
		APPLY_PREF(file_unmatch, FILE_FILTER_UNMATCH);
		APPLY_PREF_BOOL(ignore_hidden_files, FILE_FILTER_IGNORE_HIDDEN);
		APPLY_PREF(dir_match, DIR_FILTER_MATCH);
		APPLY_PREF(dir_unmatch, DIR_FILTER_UNMATCH);
		APPLY_PREF_BOOL(ignore_hidden_dirs, DIR_FILTER_IGNORE_HIDDEN);
	}
}

#define PREF_LOAD(var, P) \
	if (NULL != (value = prop_get(p->props, P))) \
	{ \
		pos = 0; \
		gtk_editable_delete_text(ff->var ## _en, 0, -1); \
		gtk_editable_insert_text(ff->var ## _en, value, strlen(value), &pos); \
		g_free(value); \
	} \

#define PREF_LOAD_BOOL(var, P) \
	gtk_toggle_button_set_active(ff->var ## _tb, \
								 prop_get_int (p->props, P, 0)); \

static void fv_prefs_load (void)
{
	gchar *value;
	gint pos;
	ProjectDBase *p = app->project_dbase;
	
	PREF_LOAD(file_match, FILE_FILTER_MATCH)
	PREF_LOAD(file_unmatch, FILE_FILTER_UNMATCH)
	PREF_LOAD_BOOL(ignore_hidden_files, FILE_FILTER_IGNORE_HIDDEN);
	PREF_LOAD(dir_match, DIR_FILTER_MATCH)
	PREF_LOAD(dir_unmatch, DIR_FILTER_UNMATCH)
	PREF_LOAD_BOOL(ignore_hidden_dirs, DIR_FILTER_IGNORE_HIDDEN);
	
	fv_prefs_apply();
}

#define PREF_SAVE(P) \
	if (NULL != (s = prop_get(p->props, P))) \
	{ \
		session_save_string (p, SECSTR (SECTION_FILE_VIEW), \
							 P, s); \
		g_free(s); \
	}

void fv_session_save (ProjectDBase *p)
{
	gchar *s;

	g_return_if_fail(p);
	
	PREF_SAVE(FILE_FILTER_MATCH)
	PREF_SAVE(FILE_FILTER_UNMATCH)
	PREF_SAVE(FILE_FILTER_IGNORE_HIDDEN)
	PREF_SAVE(DIR_FILTER_MATCH)
	PREF_SAVE(DIR_FILTER_UNMATCH)
	PREF_SAVE(DIR_FILTER_IGNORE_HIDDEN)
}

void fv_session_load (ProjectDBase *p)
{
	gpointer config_iterator;
	
	config_iterator = session_get_iterator (p, SECSTR (SECTION_FILE_VIEW));
	if (config_iterator !=  NULL)
	{
		gchar *szKey, *szValue;
		while ((config_iterator = gnome_config_iterator_next (config_iterator,
															  &szKey,
															  &szValue)))
		{
			if (szKey && szValue)
				prop_set_with_key (p->props, szKey, szValue);
			g_free (szKey);
			g_free (szValue);
			szKey = NULL;
			szValue = NULL;
		}
	}
	if (!ff)
		fv_customize(FALSE);
	fv_prefs_load();
}

gboolean
on_file_filter_delete_event (GtkWidget *widget, GdkEventCrossing *event,
							 gpointer user_data)
{
	if (ff->showing)
	{
		gtk_widget_hide((GtkWidget *) ff->dialog);
		ff->showing = FALSE;
	}
	return TRUE;
}

void
on_file_filter_close (GtkWidget *widget, gpointer user_data)
{
	if (ff->showing)
	{
		gtk_widget_hide((GtkWidget *) ff->dialog);
		ff->showing = FALSE;
	}
}

void
on_file_filter_response (GtkWidget *dlg, gint res, gpointer user_data)
{
	if (ff->showing)
	{
		gtk_widget_hide((GtkWidget *) ff->dialog);
		ff->showing = FALSE;
	}
	fv_prefs_apply();
	fv_populate(TRUE);
}

#define SET_WIDGET(var,type,name) ff->var = (type *) glade_xml_get_widget(\
	ff->xml, name); \
	gtk_widget_ref((GtkWidget *) ff->var)

void fv_customize(gboolean really_show)
{
	if (NULL == ff)
	{
		ff = g_new0(FileFilter, 1);
		if (NULL == (ff->xml = glade_xml_new(GLADE_FILE_ANJUTA, FILE_FILTER_DIALOG, NULL)))
		{
			anjuta_error(_("Unable to build user interface for file filter"));
			return;
		}
		SET_WIDGET(dialog, GtkWindow, FILE_FILTER_DIALOG);
		SET_WIDGET(file_match_en, GtkEditable, FILE_FILTER_MATCH);
		SET_WIDGET(file_match_combo, GtkCombo, FILE_FILTER_MATCH_COMBO);
		SET_WIDGET(file_unmatch_en, GtkEditable, FILE_FILTER_UNMATCH);
		SET_WIDGET(file_unmatch_combo, GtkCombo, FILE_FILTER_UNMATCH_COMBO);
		SET_WIDGET(ignore_hidden_files_tb, GtkToggleButton, FILE_FILTER_IGNORE_HIDDEN);
		SET_WIDGET(dir_match_en, GtkEditable, DIR_FILTER_MATCH);
		SET_WIDGET(dir_match_combo, GtkCombo, DIR_FILTER_MATCH_COMBO);
		SET_WIDGET(dir_unmatch_en, GtkEditable, DIR_FILTER_UNMATCH);
		SET_WIDGET(dir_unmatch_combo, GtkCombo, DIR_FILTER_UNMATCH_COMBO);
		SET_WIDGET(ignore_hidden_dirs_tb, GtkToggleButton, DIR_FILTER_IGNORE_HIDDEN);
		gtk_window_set_transient_for (GTK_WINDOW(ff->dialog)
		  , GTK_WINDOW(app->widgets.window));
		glade_xml_signal_autoconnect(ff->xml);
	}
	if (really_show && !ff->showing)
	{
		gtk_widget_show((GtkWidget *) ff->dialog);
		ff->showing = TRUE;
	}
}

static gchar *
fv_construct_full_path (AnFileView *fv, GtkTreeIter *selected_iter)
{
	gchar *path, *dir;
	GtkTreeModel *model;
	GtkTreeIter iter, parent;
	gchar *full_path = NULL;
	GtkTreeView *view = GTK_TREE_VIEW (fv->tree);

	parent = *selected_iter;
	model = gtk_tree_view_get_model (view);
	do
	{
		const gchar *filename;
		iter = parent;
		gtk_tree_model_get (model, &iter, FILENAME_COLUMN, &filename, -1);
		path = g_build_filename (filename, full_path, NULL);
		g_free (full_path);
		full_path = path;
	}
	while (gtk_tree_model_iter_parent (model, &parent, &iter));
	dir = g_path_get_dirname (fv->top_dir);
	path = g_build_filename (dir, full_path, NULL);
	g_free (full_path);
	g_free (dir);
#ifdef DEBUG
	g_message ("Full path: %s", path);
#endif
	return path;
}

static gchar *
fv_get_selected_file_path (AnFileView *fv, GtkTreeView *view)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	g_return_val_if_fail (GTK_IS_TREE_VIEW (view), NULL);
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return NULL;
	return fv_construct_full_path (fv, &iter);
}

static void
on_file_view_cvs_event (GtkMenuItem *item, gpointer user_data)
{
	gchar *path;
	
	int action = (int) user_data;
	path = fv_get_selected_file_path (fv, GTK_TREE_VIEW (fv->tree));
	
	g_return_if_fail (path != NULL);
	
	switch (action)
	{
		case CVS_ACTION_UPDATE:
		case CVS_ACTION_COMMIT:
		case CVS_ACTION_STATUS:
		case CVS_ACTION_LOG:
		case CVS_ACTION_ADD:
		case CVS_ACTION_REMOVE:
			create_cvs_gui(app->cvs, action, path, FALSE);
			break;
		case CVS_ACTION_DIFF:
			create_cvs_diff_gui(app->cvs, path, FALSE);
			break;
		default:
			break;
	}
	g_free (path);
}

static void
fv_context_handler (GtkMenuItem *item,
		    gpointer     user_data)
{
	FVSignal signal = (FVSignal) user_data;
	gchar *path = fv_get_selected_file_path (fv, GTK_TREE_VIEW (fv->tree));

	g_return_if_fail (path != NULL);
	if (g_file_test (path, G_FILE_TEST_IS_DIR))
		return;
	
	switch (signal) {
		case OPEN:
			anjuta_fv_open_file(path, TRUE);
			break;
		case VIEW:
			anjuta_fv_open_file(path, FALSE);
			break;
		case REFRESH:
			fv_populate(TRUE);
			break;
		case CUSTOMIZE:
			fv_customize(TRUE);
		default:
			break;
	}
}

static GnomeUIInfo an_file_view_cvs_uiinfo[] = {
	{
	 /* 0 */
	 GNOME_APP_UI_ITEM, N_("Update file"),
	 N_("Update current working copy"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_UPDATE, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 1 */
	 GNOME_APP_UI_ITEM, N_("Commit file"),
	 N_("Commit changes to the repository"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_COMMIT, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 2 */
	 GNOME_APP_UI_ITEM, N_("Status of file"),
	 N_("Print the status of the current file"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_STATUS, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 3 */
	 GNOME_APP_UI_ITEM, N_("Get file log"),
	 N_("Print the CVS log for the current file"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_LOG, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 4 */
	 GNOME_APP_UI_ITEM, N_("Add file"),
	 N_("Add the current file to the repository"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_ADD, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 5 */
	 GNOME_APP_UI_ITEM, N_("Remove file"),
	 N_("Remove the current file from the repository"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_REMOVE, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 6 */
	 GNOME_APP_UI_ITEM, N_("Diff file"),
	 N_("Create a diff between the working copy and the repository"),
	 on_file_view_cvs_event, (gpointer) CVS_ACTION_DIFF, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	 GNOMEUIINFO_END /* 7 */
};

static GnomeUIInfo an_file_view_menu_uiinfo[] = {
	{/* 0 */
	 GNOME_APP_UI_ITEM, N_("Open in Anjuta"),
	 NULL,
	 fv_context_handler, (gpointer) OPEN, NULL,
	 PIX_STOCK(GTK_STOCK_OPEN),
	 0, 0, NULL}
	,
	{/* 1 */
	 GNOME_APP_UI_ITEM, N_("Open in default viewer"),
	 NULL,
	 fv_context_handler, (gpointer) VIEW, NULL,
	 PIX_STOCK (GNOME_STOCK_BOOK_OPEN),
	 0, 0, NULL}
	,
	{/* 2 */
	 GNOME_APP_UI_SUBTREE, N_("CVS"),
	 NULL,
	 an_file_view_cvs_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	 ,
	{/* 3 */
	 GNOME_APP_UI_ITEM, N_("Refresh"),
	 NULL,
	 fv_context_handler, (gpointer) REFRESH, NULL,
	 PIX_STOCK(GTK_STOCK_REFRESH),
	 0, 0, NULL}
	,
	 GNOMEUIINFO_SEPARATOR, /* 4 */
	{/* 5 */
	 GNOME_APP_UI_TOGGLEITEM, N_("Docked"),
	 N_("Dock/Undock the Project Window"),
	 on_project_dock_undock1_activate, NULL, NULL,
	 PIX_FILE(DOCK),
	 0, 0, NULL}
	 ,
	{/* 6 */
	 GNOME_APP_UI_ITEM, N_("Customize"),
	 N_("Customize the file browser"),
	 fv_context_handler, (gpointer) CUSTOMIZE, NULL,
	 PIX_FILE(DOCK),
	 0, 0, NULL}
	 ,
	GNOMEUIINFO_END /* 7 */
};

static void
fv_create_context_menu ()
{
	int i;

	fv->menu.top = gtk_menu_new();
	gtk_widget_ref(fv->menu.top);

	gnome_app_fill_menu (GTK_MENU_SHELL(fv->menu.top), an_file_view_menu_uiinfo,
			     NULL, FALSE, 0);

	for (i=0; i < sizeof(an_file_view_menu_uiinfo)/sizeof(an_file_view_menu_uiinfo[0])
	  ; ++i)
		gtk_widget_ref(an_file_view_menu_uiinfo[i].widget);
	for (i=0; i < sizeof(an_file_view_cvs_uiinfo)/sizeof(an_file_view_cvs_uiinfo[0])
	  ; ++i)
		gtk_widget_ref(an_file_view_cvs_uiinfo[i].widget);

	fv->menu.open = an_file_view_menu_uiinfo[0].widget;
	fv->menu.view = an_file_view_menu_uiinfo[1].widget;
	fv->menu.cvs.top = an_file_view_menu_uiinfo[2].widget;
	fv->menu.cvs.update = an_file_view_cvs_uiinfo[0].widget;
	fv->menu.cvs.commit = an_file_view_cvs_uiinfo[1].widget;
	fv->menu.cvs.status = an_file_view_cvs_uiinfo[2].widget;
	fv->menu.cvs.log = an_file_view_cvs_uiinfo[3].widget;
	fv->menu.cvs.add = an_file_view_cvs_uiinfo[4].widget;
	fv->menu.cvs.remove = an_file_view_cvs_uiinfo[5].widget;
	fv->menu.cvs.diff = an_file_view_cvs_uiinfo[6].widget;
	fv->menu.refresh = an_file_view_menu_uiinfo[3].widget;
	fv->menu.docked = an_file_view_menu_uiinfo[5].widget;
	fv->menu.customize = an_file_view_menu_uiinfo[6].widget;

	gtk_widget_show_all(fv->menu.top);
}

static void
on_treeview_row_activated (GtkTreeView *view,
						   GtkTreePath *arg1,
						   GtkTreeViewColumn *arg2,
                           AnFileView *fv)
{
	gchar *path;

	path = fv_get_selected_file_path (fv, view);
	if (path)
		anjuta_fv_open_file (path, TRUE);
	g_free (path);
}

static gboolean
on_tree_view_event  (GtkWidget *widget,
					 GdkEvent  *event,
					 gpointer   user_data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	const gchar *version;

	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	if (!event)
		return FALSE;

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, REV_COLUMN, &version, -1);

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;

		if (e->button == 3) {
			gboolean has_cvs_entry = (version && strlen(version) > 0);

			GTK_CHECK_MENU_ITEM (fv->menu.docked)->active = app->project_dbase->is_docked;

			gtk_widget_set_sensitive (fv->menu.cvs.top,    app->project_dbase->has_cvs);
			gtk_widget_set_sensitive (fv->menu.cvs.update, has_cvs_entry);
			gtk_widget_set_sensitive (fv->menu.cvs.commit, has_cvs_entry);
			gtk_widget_set_sensitive (fv->menu.cvs.status, has_cvs_entry);
			gtk_widget_set_sensitive (fv->menu.cvs.log,    has_cvs_entry);
			gtk_widget_set_sensitive (fv->menu.cvs.add,   !has_cvs_entry);
			gtk_widget_set_sensitive (fv->menu.cvs.remove, has_cvs_entry);
			gtk_widget_set_sensitive (fv->menu.cvs.diff,   has_cvs_entry);

			gtk_menu_popup (GTK_MENU (fv->menu.top),
					NULL, NULL, NULL, NULL,
					((GdkEventButton *) event)->button,
					((GdkEventButton *) event)->time);
		}
	} else if (event->type == GDK_KEY_PRESS) {
		GtkTreePath *path;
		GdkEventKey *e = (GdkEventKey *) event;

		switch (e->keyval) {
			case GDK_Return:
				if (!gtk_tree_model_iter_has_child (model, &iter))
				{
					gchar *path = fv_get_selected_file_path(fv, view);
					if (path && !g_file_test (path, G_FILE_TEST_IS_DIR))
						anjuta_fv_open_file (path, TRUE);
					g_free (path);
					return TRUE;
				}
			case GDK_Left:
				if (gtk_tree_model_iter_has_child (model, &iter))
				{
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_collapse_row (GTK_TREE_VIEW (fv->tree),
												path);
					gtk_tree_path_free (path);
					return TRUE;
				}
			case GDK_Right:
				if (gtk_tree_model_iter_has_child (model, &iter))
				{
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_expand_row (GTK_TREE_VIEW (fv->tree),
											  path, FALSE);
					gtk_tree_path_free (path);
					return TRUE;
				}
			default:
				return FALSE;
		}
	}

	return FALSE;
}

static void
fv_disconnect ()
{
	g_return_if_fail (fv != NULL);
	g_signal_handlers_block_by_func (fv->tree,
									 G_CALLBACK (on_tree_view_event),
									 NULL);
}

static void
fv_connect ()
{
	g_return_if_fail (fv != NULL && fv->tree);
	g_signal_handlers_unblock_by_func (fv->tree,
									   G_CALLBACK (on_tree_view_event),
									   NULL);
}

static gboolean
file_entry_apply_filter (const char *name, GList *match, GList *unmatch,
						 gboolean ignore_hidden)
{
	GList *tmp;
	gboolean matched = (match == NULL);
	g_return_val_if_fail(name, FALSE);
	if (ignore_hidden && ('.' == name[0]))
		return FALSE;
	/* TTimo - ignore .svn directories */
	if (!strcmp(name, ".svn"))
		return FALSE;
	for (tmp = match; tmp; tmp = g_list_next(tmp))
	{
		if (0 == fnmatch((char *) tmp->data, name, 0))
		{
			matched = TRUE;
			break;
		}
	}
	if (!matched)
		return FALSE;
	for (tmp = unmatch; tmp; tmp = g_list_next(tmp))
	{
		if (0 == fnmatch((char *) tmp->data, name, 0))
		{
			return FALSE;
		}
	}
	return matched;	
}

static void
fv_add_tree_entry (AnFileView *fv, const gchar *path, GtkTreeIter *root)
{
	GtkTreeStore *store;
	gchar file_name[PATH_MAX];
	gchar *entries = NULL;
	struct stat s;
	DIR *dir;
	struct dirent *dir_entry;

	g_return_if_fail (path != NULL);

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree)));

	g_snprintf(file_name, PATH_MAX, "%s/CVS/Entries", path);
	if (0 == stat(file_name, &s))
	{
		if (S_ISREG(s.st_mode))
		{
			int fd;
			entries = g_new(char, s.st_size + 2);
			if (0 > (fd = open(file_name, O_RDONLY)))
			{
				g_free(entries);
				entries = NULL;
			}
			else
			{
				off_t n =0;
				off_t total_read = 1;
				while (0 < (n = read(fd, entries + total_read, s.st_size - total_read)))
					total_read += n;
				entries[s.st_size] = '\0';
				entries[0] = '\n';
				close(fd);
			}
		}
	}
	if (NULL != (dir = opendir(path)))
	{
		while (NULL != (dir_entry = readdir(dir)))
		{
			GtkTreeIter iter;
			GdkPixbuf *pixbuf;
			const gchar *file;
			
			file = dir_entry->d_name;
			
			if ((0 == strcmp(file, "."))
			  || (0 == strcmp(file, "..")))
				continue;
			
			g_snprintf(file_name, PATH_MAX, "%s/%s", path, file);
			
			if (g_file_test (file_name, G_FILE_TEST_IS_SYMLINK))
				continue;
			
			if (g_file_test (file_name, G_FILE_TEST_IS_DIR))
			{
				GtkTreeIter sub_iter;
				if (!file_entry_apply_filter (file_name, ff->dir_match,
											  ff->dir_unmatch,
											  ff->ignore_hidden_dirs))
					continue;
			
				pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
				gtk_tree_store_append (store, &iter, root);
				gtk_tree_store_set (store, &iter,
							PIXBUF_COLUMN, pixbuf,
							FILENAME_COLUMN, file,
							REV_COLUMN, "D",
							-1);
				gtk_tree_store_append (store, &sub_iter, &iter);
				gtk_tree_store_set (store, &sub_iter,
							FILENAME_COLUMN, _("Loading ..."),
							REV_COLUMN, "",
							-1);
			} else {
				gchar *version = NULL;
				if (!file_entry_apply_filter (file_name, ff->file_match,
											  ff->file_unmatch,
											  ff->ignore_hidden_files))
					continue;
				if (entries)
				{
					char *str = g_strconcat("\n/", file, "/", NULL);
					char *name_pos = strstr(entries, str);
					if (NULL != name_pos)
					{
						int len = strlen(str);
						char *version_pos = strchr(name_pos + len, '/');
						if (NULL != version_pos)
						{
							*version_pos = '\0';
							version = g_strdup(name_pos + len);
							*version_pos = '/';
						}
					}
					g_free(str);
				}
				pixbuf = gdl_icons_get_uri_icon(app->icon_set, file_name);
				gtk_tree_store_append (store, &iter, root);
				gtk_tree_store_set (store, &iter,
							PIXBUF_COLUMN, pixbuf,
							FILENAME_COLUMN, file,
							REV_COLUMN, version ? version : "",
							-1);
				gdk_pixbuf_unref (pixbuf);
				g_free (version);
			}
		}
		closedir(dir);
	}
	if (entries)
		g_free (entries);
}

static void
on_file_view_row_expanded (GtkTreeView *view,
						   GtkTreeIter *iter,
						   GtkTreePath *path,
						   AnFileView *fv)
{
	GdkPixbuf *pix;
	gchar *full_path;
	GtkTreeIter child;
	gint past_count;
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	
	pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_OPEN_FOLDER);
	gtk_tree_store_set (store, iter, PIXBUF_COLUMN, pix, -1);
	
	full_path = fv_construct_full_path (fv, iter);
	
	past_count = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), iter);
	
	fv_add_tree_entry (fv, full_path, iter);
	g_free (full_path);
	
	// Clearing all old children of this node.
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
	{
		int i;
		for (i = 0; i < past_count; i++)
		{
			if (!gtk_tree_store_remove (store, &child))
				break;
		}
	}
}

static void
on_file_view_row_collapsed (GtkTreeView *view,
						   GtkTreeIter *iter,
						   GtkTreePath *path,
						   AnFileView *fv)
{
	GdkPixbuf *pix;
	GtkTreeIter child;
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	gtk_tree_store_set (store, iter, PIXBUF_COLUMN, pix, -1);
	
	// Remove all but one children
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
	{
		gtk_tree_store_set (store, &child,
							PIXBUF_COLUMN, NULL,
							FILENAME_COLUMN, _("Loading ..."),
							REV_COLUMN, "", -1);
		if (gtk_tree_model_iter_next  (GTK_TREE_MODEL (store), &child))
		{
			while (!gtk_tree_store_remove (store, &child));
		}
	}
}

static void
fv_create ()
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	fv = g_new0 (AnFileView, 1);

	/* Scrolled window */
	fv->win = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (fv->win),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (fv->win);

	/* Tree and his model */
	store = gtk_tree_store_new (COLUMNS_NB,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_STRING);

	fv->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (fv->tree), TRUE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (fv->win), fv->tree);
	g_signal_connect (fv->tree, "row_expanded",
					  G_CALLBACK (on_file_view_row_expanded), fv);
	g_signal_connect (fv->tree, "row_collapsed",
					  G_CALLBACK (on_file_view_row_collapsed), fv);
	g_signal_connect (fv->tree, "event-after",
					  G_CALLBACK (on_tree_view_event), fv);
	g_signal_connect (fv->tree, "row_activated",
					  G_CALLBACK (on_treeview_row_activated), fv);
	gtk_widget_show (fv->tree);

	g_object_unref (G_OBJECT (store));

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("File"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", FILENAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (fv->tree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (fv->tree), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Rev"), renderer,
							   "text", REV_COLUMN,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (fv->tree), column);

	fv_create_context_menu ();
	gtk_widget_ref (fv->tree);
	gtk_widget_ref (fv->win);
}

void
fv_clear ()
{
	GtkTreeModel *model;

	g_return_if_fail (fv != NULL && fv->tree);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

static void
mapping_function (GtkTreeView *treeview, GtkTreePath *path, gpointer data)
{
	gchar *str;
	GList *map = * ((GList **) data);
	
	str = gtk_tree_path_to_string (path);
	map = g_list_append (map, str);
	* ((GList **) data) = map;
};

GList *
fv_get_node_expansion_states (void)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (fv->tree),
									 mapping_function, &map);
	return map;
}

void
fv_set_node_expansion_states (GList *expansion_states)
{
	/* Restore expanded nodes */	
	if (expansion_states)
	{
		GtkTreePath *path;
		GtkTreeModel *model;
		GList *node;
		node = expansion_states;
		
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
		while (node)
		{
			path = gtk_tree_path_new_from_string (node->data);
			gtk_tree_view_expand_row (GTK_TREE_VIEW (fv->tree), path, FALSE);
			gtk_tree_path_free (path);
			node = g_list_next (node);
		}
	}
}

AnFileView *
fv_populate (gboolean full)
{
	static gboolean busy = FALSE;
	GList *selected_items;
	GtkTreeIter sub_iter;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeStore *store;
	GdkPixbuf *pixbuf;
	gchar *project_dir;

	if (!fv)
		fv_create ();
	if (busy)
		return fv;
	else
		busy = TRUE;

	anjuta_status (_("Refreshing file view ..."));
	while (gtk_events_pending())
		gtk_main_iteration();
	
	fv_disconnect ();
	selected_items = fv_get_node_expansion_states ();
	fv_clear ();

	if (!app || !app->project_dbase || !app->project_dbase->top_proj_dir)
		goto clean_leave;
	if (!fv->top_dir || 0 != strcmp(fv->top_dir, app->project_dbase->top_proj_dir))
	{
		/* Different project - reload project preferences */
		if (fv->top_dir)
			g_free(fv->top_dir);
		fv->top_dir = g_strdup(app->project_dbase->top_proj_dir);
		if (!ff)
			fv_customize(FALSE);
		fv_prefs_load();
	}
	
	project_dir = g_path_get_basename (fv->top_dir);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
	store = GTK_TREE_STORE (model);

	pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				PIXBUF_COLUMN, pixbuf,
				FILENAME_COLUMN, project_dir,
				REV_COLUMN, "",
				-1);
	g_free (project_dir);
	
	gtk_tree_store_append (store, &sub_iter, &iter);
	gtk_tree_store_set (store, &sub_iter,
				PIXBUF_COLUMN, NULL,
				FILENAME_COLUMN, _("Loading ..."),
				REV_COLUMN, "",
				-1);

	/* Expand first node */
	gtk_tree_model_get_iter_first (model, &iter);
	path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_expand_row (GTK_TREE_VIEW (fv->tree), path, FALSE);
	gtk_tree_path_free (path);

	fv_set_node_expansion_states (selected_items);
	
clean_leave:
	if (selected_items)
		glist_strings_free (selected_items);
	fv_connect ();
	busy = FALSE;
	return fv;
}

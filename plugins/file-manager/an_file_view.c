#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <string.h>
#include <dirent.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libanjuta/resources.h>
#include <libanjuta/pixmaps.h>
#include <libegg/gdl-icons.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>

// #include "anjuta.h"
// #include "resources.h"
// #include "mainmenu_callbacks.h"
// #include "pixmaps.h"
// #include "cvs_gui.h"
// #include "properties.h"
// #include "an_file_view.h"

#include "plugin.h"

enum {
	PIXBUF_COLUMN,
	FILENAME_COLUMN,
	REV_COLUMN,
	COLUMNS_NB
};

typedef struct _FileFilter
{
	GList *file_match;
	GList *file_unmatch;
	GList *dir_match;
	GList *dir_unmatch;
	// GList *file_match_strings;
	// GList *file_unmatch_strings;
	// GList *dir_match_strings;
	// GList *dir_unmatch_strings;
	gboolean ignore_hidden_files;
	gboolean ignore_hidden_dirs;
} FileFilter;

static GdlIcons *icon_set = NULL;
static FileFilter *ff = NULL;

#if 0
/* LibGlade auto-signal-connect callbacks. Do not declare static. */
gboolean on_file_filter_delete_event (GtkWidget *widget,
									  GdkEventCrossing *event,
									  gpointer user_data);
void on_file_filter_response (GtkWidget *dlg, gint res, gpointer user_data);
void on_file_filter_close (GtkWidget *dlg, gpointer user_data);
#endif

static gboolean anjuta_file_iface_open(FileManagerPlugin* fv, const char* path)
{
	AnjutaShell* shell = ANJUTA_PLUGIN(fv)->shell;		
	IAnjutaDocumentManager* docman = anjuta_shell_get_interface(shell, IAnjutaDocumentManager, NULL);
	g_return_val_if_fail(docman != NULL, FALSE);
	
	if (IANJUTA_IS_FILE(docman))
	{
		ianjuta_file_open(IANJUTA_FILE(docman), path, NULL);
	}
	return TRUE;
}

static gboolean
anjuta_fv_open_file (FileManagerPlugin * fv, const char *path, gboolean use_anjuta)
{
	gboolean status = FALSE;
	const char *mime_type = gnome_vfs_get_mime_type(path);
	if (use_anjuta)
	{
		status = anjuta_file_iface_open(fv, path);
	}
	else
	{
		GnomeVFSMimeApplication *app = gnome_vfs_mime_get_default_application(mime_type);
		if ((app) && (app->command))
		{
			if (NULL != strstr (app->command, "anjuta"))
				status = anjuta_file_iface_open(fv, path);
			else
			{
				char **argv = g_new(char *, 3);

				argv[0] = app->command;
				argv[1] = (char *) path;
				argv[2] = NULL;

				if (-1 == gnome_execute_async (NULL, 2, argv))
				{
					//anjuta_warning(_("Unable to open %s in %s"), path, app->command);
				}

				g_free(argv);
			}

			gnome_vfs_mime_application_free(app);
			status = TRUE;
		}
		else
		{
			/*anjuta_warning(_("No default viewer specified for the mime type %s.\n"
					"Please set it in GNOME control center"), mime_type);*/
		}
	}
	return status;
}

#if 0

typedef enum {
	OPEN,
	VIEW,
	REFRESH,
	CUSTOMIZE,
	MENU_MAX
} FVSignal;

#endif

/* File filters prefs */
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

#define GET_PREF(var, P) \
	if (ff->var) \
		anjuta_util_glist_strings_free(ff->var); \
	ff->var = NULL; \
	s = anjuta_preferences_get (fv->prefs, P); \
	if (s) { \
		ff->var = anjuta_util_glist_from_string(s); \
		g_free(s); \
	}

#define GET_PREF_BOOL(var, P) \
	ff->var = FALSE; \
	ff->var = anjuta_preferences_get_int (fv->prefs, P);
	
static FileFilter*
fv_prefs_new (FileManagerPlugin *fv)
{
	gchar *s;
	FileFilter *ff = g_new0(FileFilter, 1);
	GET_PREF(file_match, FILE_FILTER_MATCH);
	GET_PREF(file_unmatch, FILE_FILTER_UNMATCH);
	GET_PREF_BOOL(ignore_hidden_files, FILE_FILTER_IGNORE_HIDDEN);
	GET_PREF(dir_match, DIR_FILTER_MATCH);
	GET_PREF(dir_unmatch, DIR_FILTER_UNMATCH);
	GET_PREF_BOOL(ignore_hidden_dirs, DIR_FILTER_IGNORE_HIDDEN);
	return ff;
}

static void
fv_prefs_free (FileFilter *ff)
{
	g_return_if_fail (ff != NULL);
	if (ff->file_match)
		anjuta_util_glist_strings_free (ff->file_match);
	if (ff->file_unmatch)
		anjuta_util_glist_strings_free (ff->file_unmatch);
	if (ff->dir_match)
		anjuta_util_glist_strings_free (ff->dir_match);
	if (ff->dir_unmatch)
		anjuta_util_glist_strings_free (ff->dir_unmatch);
/*	if (ff->file_match_strings)
		anjuta_util_glist_strings_free (ff->file_match_strings);
	if (ff->file_unmatch_strings)
		anjuta_util_glist_strings_free (ff->file_unmatch_strings);
	if (ff->dir_match_strings)
		anjuta_util_glist_strings_free (ff->dir_match_strings);
	if (ff->dir_unmatch_strings)
		anjuta_util_glist_strings_free (ff->dir_unmatch_strings);
*/	g_free (ff);
}

static gchar *
fv_construct_full_path (FileManagerPlugin *fv, GtkTreeIter *selected_iter)
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

gchar *
fv_get_selected_file_path (FileManagerPlugin *fv)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeView *view;
	
	view = GTK_TREE_VIEW (fv->tree);
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return NULL;
	return fv_construct_full_path (fv, &iter);
}

#if 0
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

#endif

static void
on_treeview_row_activated (GtkTreeView *view,
						   GtkTreePath *arg1,
						   GtkTreeViewColumn *arg2,
                           FileManagerPlugin *fv)
{
	gchar *path;

	path = fv_get_selected_file_path (fv);
	if (path)
		anjuta_fv_open_file (fv, path, TRUE);
	g_free (path);
}

static gboolean
on_tree_view_event  (GtkWidget *widget,
					 GdkEvent  *event,
					 FileManagerPlugin *fv)
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
			GtkWidget *popup;
			popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (fv->ui),
											   "/PopupFileManager");
			g_return_val_if_fail (GTK_IS_WIDGET (popup), TRUE);
#if 0
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
#endif
			gtk_menu_popup (GTK_MENU (popup),
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
					gchar *path = fv_get_selected_file_path(fv);
					if (path && !g_file_test (path, G_FILE_TEST_IS_DIR))
						anjuta_fv_open_file (fv, path, TRUE);
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
fv_disconnect (FileManagerPlugin *fv)
{
	g_return_if_fail (fv != NULL);
	g_signal_handlers_block_by_func (fv->tree,
									 G_CALLBACK (on_tree_view_event),
									 NULL);
}

static void
fv_connect (FileManagerPlugin *fv)
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
fv_add_tree_entry (FileManagerPlugin *fv, const gchar *path, GtkTreeIter *root)
{
	GtkTreeStore *store;
	gchar file_name[PATH_MAX];
	struct stat s;
	DIR *dir;
	struct dirent *dir_entry;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	GSList *file_node;
	GSList *files = NULL;
	gchar *entries = NULL;

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
			const gchar *file;
			
			file = dir_entry->d_name;
			
			if ((ff->ignore_hidden_files && *file == '.') ||
				(0 == strcmp(file, ".")) ||
				(0 == strcmp(file, "..")))
				continue;
			
			g_snprintf(file_name, PATH_MAX, "%s/%s", path, file);
			
			if (g_file_test (file_name, G_FILE_TEST_IS_SYMLINK))
				continue;
			
			if (g_file_test (file_name, G_FILE_TEST_IS_DIR))
			{
				GtkTreeIter sub_iter;
				/*if (!file_entry_apply_filter (file_name, ff->dir_match,
											  ff->dir_unmatch,
											  ff->ignore_hidden_dirs))
					continue;
				*/
				pixbuf = gdl_icons_get_mime_icon (icon_set,
											"application/directory-normal");
				// pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
				gtk_tree_store_append (store, &iter, root);
				gtk_tree_store_set (store, &iter,
							PIXBUF_COLUMN, pixbuf,
							FILENAME_COLUMN, file,
							REV_COLUMN, "D",
							-1);
				g_object_unref (pixbuf);
				gtk_tree_store_append (store, &sub_iter, &iter);
				gtk_tree_store_set (store, &sub_iter,
							FILENAME_COLUMN, _("Loading ..."),
							REV_COLUMN, "",
							-1);
			} else {
				files = g_slist_prepend (files, g_strdup (file));
			}
		}
		closedir(dir);
		files = g_slist_reverse (files);
		file_node = files;
		while (file_node)
		{
			gchar *version = NULL;
			gchar *fname = file_node->data;
			g_snprintf(file_name, PATH_MAX, "%s/%s", path, fname);
			/* if (!file_entry_apply_filter (fname, ff->file_match,
										  ff->file_unmatch,
										  ff->ignore_hidden_files))
				continue;
			*/
			if (entries)
			{
				char *str = g_strconcat("\n/", fname, "/", NULL);
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
			pixbuf = gdl_icons_get_uri_icon(icon_set, file_name);
			gtk_tree_store_append (store, &iter, root);
			gtk_tree_store_set (store, &iter,
						PIXBUF_COLUMN, pixbuf,
						FILENAME_COLUMN, fname,
						REV_COLUMN, version ? version : "",
						-1);
			gdk_pixbuf_unref (pixbuf);
			g_free (version);
			g_free (fname);
			file_node = g_slist_next (file_node);
		}
		g_slist_free (files);
	}
	if (entries)
		g_free (entries);
}

static void
on_file_view_row_expanded (GtkTreeView *view,
						   GtkTreeIter *iter,
						   GtkTreePath *path,
						   FileManagerPlugin *fv)
{
	GdkPixbuf *pix;
	gchar *full_path;
	GtkTreeIter child;
	gint past_count;
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	
	// pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_OPEN_FOLDER);
	pix = gdl_icons_get_mime_icon (icon_set, "application/directory-normal");
	gtk_tree_store_set (store, iter, PIXBUF_COLUMN, pix, -1);
	g_object_unref (pix);
	
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
						   FileManagerPlugin *fv)
{
	GdkPixbuf *pix;
	GtkTreeIter child;
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	// pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	pix = gdl_icons_get_mime_icon (icon_set, "application/directory-normal");
	gtk_tree_store_set (store, iter, PIXBUF_COLUMN, pix, -1);
	g_object_unref (pix);
	
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
on_tree_view_selection_changed (GtkTreeSelection *sel, FileManagerPlugin *fv)
{
	gchar *filename;
	GValue *value;
	
	filename = fv_get_selected_file_path (fv);
	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, filename);
	anjuta_shell_add_value (ANJUTA_PLUGIN(fv)->shell,
							"file_manager_current_filename",
							value, NULL);
}

void
fv_init (FileManagerPlugin *fv)
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	/* Scrolled window */
	fv->scrolledwindow = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (fv->scrolledwindow),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (fv->scrolledwindow);

	/* Tree and his model */
	store = gtk_tree_store_new (COLUMNS_NB,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_STRING);

	fv->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (fv->tree), TRUE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (fv->scrolledwindow), fv->tree);
	g_signal_connect (fv->tree, "row_expanded",
					  G_CALLBACK (on_file_view_row_expanded), fv);
	g_signal_connect (fv->tree, "row_collapsed",
					  G_CALLBACK (on_file_view_row_collapsed), fv);
	g_signal_connect (fv->tree, "event-after",
					  G_CALLBACK (on_tree_view_event), fv);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_tree_view_selection_changed), fv);
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

	// fv_create_context_menu ();
	g_object_ref (G_OBJECT (fv->tree));
	g_object_ref (G_OBJECT (fv->scrolledwindow));
}

void
fv_finalize (FileManagerPlugin *fv)
{
	if (fv->top_dir)
		g_free (fv->top_dir);
	g_object_unref (G_OBJECT (fv->tree));
	g_object_unref (G_OBJECT (fv->scrolledwindow));
	fv->top_dir = NULL;
	fv->tree = NULL;
	fv->scrolledwindow = NULL;
	if (ff != NULL)
		fv_prefs_free (ff);
}

void
fv_clear (FileManagerPlugin *fv)
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
fv_get_node_expansion_states (FileManagerPlugin *fv)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (fv->tree),
									 mapping_function, &map);
	return map;
}

void
fv_set_node_expansion_states (FileManagerPlugin *fv, GList *expansion_states)
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

void
fv_set_root (FileManagerPlugin *fv, const gchar *root_dir)
{
	if (!fv->top_dir || 0 != strcmp(fv->top_dir, root_dir))
	{
		/* Different directory*/
		if (fv->top_dir)
			g_free(fv->top_dir);
		fv->top_dir = g_strdup(root_dir);
		fv_refresh(fv);
	}
}

void
fv_refresh (FileManagerPlugin *fv)
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

	if (busy)
		return;
	else
		busy = TRUE;

	// anjuta_status (_("Refreshing file view ..."));
	// while (gtk_events_pending())
	//	gtk_main_iteration();
	
	if (icon_set == NULL)
		icon_set = gdl_icons_new (24, 16.0);
	if (ff != NULL)
		fv_prefs_free (ff);
	ff = fv_prefs_new (fv);
	
	fv_disconnect (fv);
	selected_items = fv_get_node_expansion_states (fv);
	fv_clear (fv);

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

	fv_set_node_expansion_states (fv, selected_items);
	
// clean_leave:
	if (selected_items)
		anjuta_util_glist_strings_free (selected_items);
	fv_connect (fv);
	busy = FALSE;
	return;
}

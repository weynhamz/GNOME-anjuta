#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#include "gdl-icons.h"

#include "an_file_view.h"

enum {
	PIXBUF_COLUMN,
	FILENAME_COLUMN,
	REV_COLUMN,
	TMFILE_ENTRY_COLUMN,
	COLUMNS_NB
};

static GdlIcons *icon_set = NULL;

static AnFileView *fv = NULL;
gboolean
anjuta_fv_open_file (const char *path,
		     gboolean    use_anjuta)
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
	GtkButton *ok_button;
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
	gchar *value;
	gchar *key;
	gpointer config_iterator;
	
	if (!ff)
		return;
	
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
	fv_prefs_load();
}

gboolean on_file_filter_delete_event (GtkWidget *widget, GdkEventCrossing *event
  , gpointer user_data)
{
	if (ff->showing)
	{
		gtk_widget_hide((GtkWidget *) ff->dialog);
		ff->showing = FALSE;
	}
	return TRUE;
}

void on_file_filter_ok_button_clicked (GtkButton *button, gpointer user_data)
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

static void fv_customize(gboolean really_show)
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
		SET_WIDGET(ok_button, GtkButton, OK_BUTTON);
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

static void
on_file_view_cvs_event (GtkMenuItem *item,
			gpointer     user_data)
{
	int action = (int) user_data;

	if ((fv->curr_entry) && (tm_file_regular_t == fv->curr_entry->type))
	{
		switch (action)
		{
			case CVS_ACTION_UPDATE:
			case CVS_ACTION_COMMIT:
			case CVS_ACTION_STATUS:
			case CVS_ACTION_LOG:
			case CVS_ACTION_ADD:
			case CVS_ACTION_REMOVE:
				create_cvs_gui(app->cvs, action, fv->curr_entry->path, FALSE);
				break;
			case CVS_ACTION_DIFF:
				create_cvs_diff_gui(app->cvs, fv->curr_entry->path, FALSE);
				break;
			default:
				break;
		}
	}
}

static void
fv_context_handler (GtkMenuItem *item,
		    gpointer     user_data)
{
	FVSignal signal = (FVSignal) user_data;

	switch (signal) {
		case OPEN:
			if ((fv->curr_entry) && (tm_file_regular_t == fv->curr_entry->type))
				anjuta_fv_open_file(fv->curr_entry->path, TRUE);
			break;
		case VIEW:
			if ((fv->curr_entry) && (tm_file_regular_t == fv->curr_entry->type))
				anjuta_fv_open_file(fv->curr_entry->path, FALSE);
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
on_treeview_row_activated (GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	g_return_if_fail (GTK_IS_TREE_VIEW (view));
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	gtk_tree_model_get (model, &iter, TMFILE_ENTRY_COLUMN, &fv->curr_entry, -1);
	if (fv->curr_entry && tm_file_regular_t == fv->curr_entry->type)
		anjuta_fv_open_file (fv->curr_entry->path, TRUE);
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

	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	if (!event)
		return FALSE;

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, TMFILE_ENTRY_COLUMN, &fv->curr_entry, -1);

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;

		if (e->button == 3) {
			gboolean has_cvs_entry = (fv->curr_entry && fv->curr_entry->version);

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
					anjuta_fv_open_file (fv->curr_entry->path, TRUE);
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

static void
on_file_view_row_expanded (GtkTreeView *view,
			   GtkTreeIter *iter,
			   GtkTreePath *path)
{
	GdkPixbuf *pix;
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_OPEN_FOLDER);
	gtk_tree_store_set (store, iter, PIXBUF_COLUMN, pix, -1);
}

static void
on_file_view_row_collapsed (GtkTreeView *view,
			   GtkTreeIter *iter,
			   GtkTreePath *path)
{
	GdkPixbuf *pix;
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	gtk_tree_store_set (store, iter, PIXBUF_COLUMN, pix, -1);
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
				    G_TYPE_STRING,
				    G_TYPE_POINTER);

	fv->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (fv->tree), TRUE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (fv->win), fv->tree);
	g_signal_connect (fv->tree, "row_expanded",
					  G_CALLBACK (on_file_view_row_expanded), NULL);
	g_signal_connect (fv->tree, "row_collapsed",
					  G_CALLBACK (on_file_view_row_collapsed), NULL);
	g_signal_connect (fv->tree, "event",
					  G_CALLBACK (on_tree_view_event), NULL);
	g_signal_connect (fv->tree, "row_activated",
					  G_CALLBACK (on_treeview_row_activated), NULL);
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

static void fv_hide(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_widget_hide(fv->tree);
}

static void fv_show(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_widget_show(fv->tree);
}

static void
fv_add_tree_entry (TMFileEntry *entry,
				   GtkTreeIter *root)
{
	GtkTreeStore *store;
	GtkTreeIter iter, sub_iter;
	GtkTreeIter *parent = root;
	GSList *tmp;
	GdkPixbuf *pixbuf;
	
	if (!entry || !entry->path || !entry->name || !fv || !fv->tree)
		return;

	if (tm_file_dir_t != entry->type || !entry->children)
		return;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree)));

	while (gtk_events_pending())
		gtk_main_iteration();
	
	pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	gtk_tree_store_append (store, &iter, parent);
	gtk_tree_store_set (store, &iter,
			    PIXBUF_COLUMN, pixbuf,
			    FILENAME_COLUMN, entry->name,
			    REV_COLUMN, entry->version ? entry->version : "",
			    TMFILE_ENTRY_COLUMN, entry,
			    -1);
	
	for (tmp = entry->children; tmp; tmp = g_slist_next (tmp)) {
		TMFileEntry *child = (TMFileEntry *) tmp->data;
		if (tm_file_dir_t == entry->type)
			fv_add_tree_entry (child, &iter);
	}

	for (tmp = entry->children; tmp; tmp = g_slist_next (tmp)) {
		TMFileEntry *child = (TMFileEntry *) tmp->data;

		if (tm_file_regular_t == child->type) {
			/*
			pixbuf = anjuta_res_get_icon_for_file (ANJUTA_PREFERENCES
					(app->preferences)->props, child->name);
			*/
			pixbuf = gdl_icons_get_uri_icon(icon_set, child->path);
			gtk_tree_store_append (store, &sub_iter, &iter);
			gtk_tree_store_set (store, &sub_iter,
					    PIXBUF_COLUMN, pixbuf,
					    FILENAME_COLUMN, child->name,
					    REV_COLUMN, child->version ? child->version : "",
					    TMFILE_ENTRY_COLUMN, child,
					    -1);
		}
	}
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

	if (!icon_set)
		icon_set = gdl_icons_new(24, 16.0);
	if (!fv)
		fv_create ();
	if (busy)
		return fv;
	else
		busy = TRUE;

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

	if (fv->file_tree)
		tm_file_entry_free (fv->file_tree);

	fv->file_tree = tm_file_entry_new(fv->top_dir, NULL, full, ff->file_match,
									  ff->file_unmatch, ff->dir_match,
									  ff->dir_unmatch, ff->ignore_hidden_files,
									  ff->ignore_hidden_dirs);
	if (!fv->file_tree)
		goto clean_leave;
	fv_add_tree_entry (fv->file_tree, NULL);

	/* Expand first node */
	{
		GtkTreeIter iter;
		GtkTreePath *path;
		GtkTreeModel *model;
		
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
		gtk_tree_model_get_iter_first (model, &iter);
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_expand_row (GTK_TREE_VIEW (fv->tree), path, FALSE);
		gtk_tree_path_free (path);
	}
	fv_set_node_expansion_states (selected_items);
	
clean_leave:
	if (selected_items)
		glist_strings_free (selected_items);
	fv_connect ();
	busy = FALSE;
	return fv;
}

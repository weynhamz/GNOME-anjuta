/* 
 * gnomefilelist.c Copyright (C) 
 *   Chris Phelps <chicane@reninet.com>,
 *   Naba kumar <kh_naba@users.sourceforge.net>
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
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Original author: Chris Phelps <chicane@reninet.com>
 * Widget rewrote: Naba Kumar <kh_naba@users.sourceforge.net>
 * Contributions:
 *     Philip Van Hoof <freax@pandora.be>
 *     Dan Elphick <dre00r@ecs.soton.ac.uk>
 *     Stephane Demurget  <demurgets@free.fr>
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gnomefilelist.h"

#include <pwd.h>
#include <sys/types.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libegg/gdl-icons.h>

enum
{
	FILE_PIXBUF_COLUMN,
	FILE_COLUMN,
	N_FILE_COLUMNS
};

enum
{
	DIRECTORY_PIXBUF_COLUMN,
	DIRECTORY_COLUMN,
	N_DIRECTORY_COLUMNS
};

static GdlIcons *icon_set = NULL;

#define FILE_PIXMAP_FOLDER PACKAGE_PIXMAPS_DIR"/bfoldc.xpm"

/* function declarations */
static void gnome_filelist_class_init (GnomeFileListClass * klass);
static void gnome_filelist_init (GnomeFileList * file_list);
static void gnome_filelist_destroy (GtkObject * object);
static void gnome_filelist_show (GtkWidget * widget, GnomeFileList * file_list);
static void gnome_filelist_hide (GtkWidget * widget, GnomeFileList * file_list);
static void gnome_filelist_get_dirs (GnomeFileList * file_list);
static gboolean gnome_filelist_check_dir_exists (const gchar * path);

static gint dir_compare_func (gchar * txt1, gchar * txt2);
static void set_file_selection (GnomeFileList * file_list);
static gboolean set_dir_internal (GnomeFileList * file_list,
								  const gchar * path);

/*
static gint g_list_find_string_pos (GList *list, gchar *new_entry);
static gint gnome_filelist_key_press (GtkWidget * widget, GdkEventKey * event);
*/
static gint selection_entry_key_press (GtkWidget * widget, GdkEventKey * event,
									   GnomeFileList * file_list);
static void refresh_listing (GtkWidget * widget, GnomeFileList * file_list);

// static void goto_directory(GtkWidget *widget, GnomeFileList *file_list);
static void goto_parent (GtkWidget * widget, GnomeFileList * file_list);
static void goto_prev (GtkWidget * widget, GnomeFileList * file_list);
static void goto_next (GtkWidget * widget, GnomeFileList * file_list);
static void check_ok_button_cb (GtkWidget * widget, GnomeFileList * file_list);
gchar *get_parent_dir (const gchar * path);
gchar *build_full_path (const gchar * path, const gchar * selection);
static void check_goto (GtkWidget * widget, GnomeFileList * file_list);
static void delete_file (GtkWidget * widget, GnomeFileList * file_list);
static void rename_file (GtkWidget * widget, GnomeFileList * file_list);
static void create_dir (GtkWidget * widget, GnomeFileList * file_list);
/*
static gint create_dir_delete_cb (GtkWidget * widget, GdkEvent * e,
								  GnomeFileList * file_list);
static void create_dir_okbutton_cb (GtkWidget * button,
									GnomeFileList * file_list);
static void create_dir_cancelbutton_cb (GtkWidget * button,
										GnomeFileList * file_list);
*/
static void home_directory_cb (GtkButton * button, GnomeFileList * file_list);
static gboolean check_if_file_exists (const gchar * filename);
static gboolean check_can_modify (const gchar * filename);
static void show_hidden_toggled (GtkToggleButton * b, gpointer data);
static void filetype_combo_go (GtkEntry *entry, GnomeFileList * file_list);
static void history_combo_go (GtkWidget * widget, GnomeFileList * file_list);
static void history_combo_clicked (GtkWidget * widget,
								   GnomeFileList * file_list);
static void file_scrollbar_value_changed (GtkAdjustment * adjustment,
										  GnomeFileList * file_list);
static void dir_scrollbar_value_changed (GtkAdjustment * adjustment,
										 GnomeFileList * file_list);
static void
on_selection_changed_file (GtkTreeSelection * selection,
						   GnomeFileList * file_list);
static void
on_selection_changed_dir (GtkTreeSelection * selection,
						  GnomeFileList * file_list);
static void
on_row_activated_dir   (GtkTreeView     *treeview, GtkTreePath     *arg1,
						GtkTreeViewColumn *arg2, GnomeFileList   *file_list);
static void
on_row_activated_file  (GtkTreeView     *treeview, GtkTreePath     *arg1,
						GtkTreeViewColumn *arg2, GnomeFileList   *file_list);

/* end function declarations */

static GtkWindowClass *parent_class = NULL;

GtkType
gnome_filelist_get_type (void)
{
	static GtkType filelist_type = 0;

	if (!filelist_type)
	{
		static const GtkTypeInfo filelist_info = {
			"GnomeFileList",
			sizeof (GnomeFileList),
			sizeof (GnomeFileListClass),
			(GtkClassInitFunc) gnome_filelist_class_init,
			(GtkObjectInitFunc) gnome_filelist_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL,
		};

		filelist_type = gtk_type_unique (GTK_TYPE_WINDOW, &filelist_info);
	}

	return (filelist_type);
}

static void
gnome_filelist_class_init (GnomeFileListClass * klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;
	object_class->destroy = gnome_filelist_destroy;
	parent_class = gtk_type_class (GTK_TYPE_WINDOW);
}

static void
gnome_filelist_init (GnomeFileList * file_list)
{
	file_list->history_combo = NULL;
	file_list->directory_list = NULL;
	file_list->file_list = NULL;
	file_list->history = NULL;
	file_list->show_hidden = FALSE;
	file_list->path = 0;
	file_list->selected = 0;
	file_list->history_position = -1;
	if (!icon_set)
		icon_set = gdl_icons_new (24, 16.0);
}

GtkWidget *
gnome_filelist_new ()
{
	return (gnome_filelist_new_with_path (g_get_home_dir ()));
}

GtkWidget *
gnome_filelist_new_with_path (const gchar * path)
{
	GnomeFileList *file_list;
	GtkWidget *main_box;
	GtkWidget *util_box;
	GtkWidget *toolbar;
	GtkWidget *paned;
	GtkWidget *label;
	GtkWidget *combolabel;
	GtkWidget *hsep;
	GtkWidget *parent_button;
	GtkWidget *refresh_button;
	GList *combolist = NULL;
	GtkAdjustment *file_adjustment;
	GtkAdjustment *dir_adjustment;
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	file_list = gtk_type_new (GNOME_TYPE_FILELIST);
	gnome_window_icon_set_from_default (GTK_WINDOW (file_list));
	gtk_container_set_border_width (GTK_CONTAINER (file_list), 5);
	// gtk_signal_connect(GTK_OBJECT(file_list), "key_press_event",
	// GTK_SIGNAL_FUNC(gnome_filelist_key_press), 0);
	gtk_signal_connect (GTK_OBJECT (file_list), "show",
						GTK_SIGNAL_FUNC (gnome_filelist_show), file_list);
	gtk_signal_connect (GTK_OBJECT (file_list), "hide",
						GTK_SIGNAL_FUNC (gnome_filelist_hide), file_list);
	main_box = gtk_vbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (file_list), main_box);
	gtk_widget_show (main_box);

	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
								 GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, FALSE, 0);
	gtk_widget_show (toolbar);

	file_list->back_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_BACK,
								  _("Go to the previous directory"), NULL,
								  GTK_SIGNAL_FUNC (goto_prev), file_list, 0);
	file_list->forward_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_FORWARD,
								  _("Go to the next directory"), NULL,
								  GTK_SIGNAL_FUNC (goto_next), file_list, 1);
	parent_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_UP,
								  _("Go to parent directory"), NULL,
								  GTK_SIGNAL_FUNC (goto_parent), file_list, 2);
	gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), 3);
	file_list->home_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_HOME,
								  _("Go to home directory"), NULL,
								  GTK_SIGNAL_FUNC (home_directory_cb),
								  file_list, 4);
	gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), 5);
	refresh_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_REFRESH,
								  _("Refresh listing"), NULL,
								  GTK_SIGNAL_FUNC (refresh_listing), file_list,
								  6);
	gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), 7);
	file_list->delete_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
								  _("Delete file"), NULL,
								  GTK_SIGNAL_FUNC (delete_file), file_list, 8);
	file_list->rename_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CONVERT,
								  _("Rename file"), NULL,
								  GTK_SIGNAL_FUNC (rename_file), file_list, 9);
	file_list->createdir_button =
		gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
								  _("Create new folder"), NULL,
								  GTK_SIGNAL_FUNC (create_dir), file_list, 10);
	gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), 11);

	file_list->show_hidden_button =
		gtk_toggle_button_new_with_label (_("Show Hidden"));
	gtk_widget_show (file_list->show_hidden_button);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar),
							   file_list->show_hidden_button,
							   _("Show hidden files"), NULL);
	gtk_signal_connect (GTK_OBJECT (file_list->show_hidden_button), "toggled",
						GTK_SIGNAL_FUNC (show_hidden_toggled), file_list);

	util_box = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_box), util_box, FALSE, FALSE, 0);
	gtk_widget_show (util_box);

	label = gtk_label_new (_("Directory: "));
	gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	file_list->history_combo = gtk_combo_new ();
	gtk_combo_disable_activate (GTK_COMBO (file_list->history_combo));
	gtk_box_pack_start (GTK_BOX (util_box), file_list->history_combo, TRUE,
						TRUE, 0);
	gtk_entry_set_editable (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry),
							FALSE);
	gtk_signal_connect (GTK_OBJECT
						(GTK_COMBO (file_list->history_combo)->entry),
						"changed", GTK_SIGNAL_FUNC (history_combo_go),
						file_list);
	gtk_signal_connect (GTK_OBJECT
						(GTK_COMBO (file_list->history_combo)->entry),
						"activate", GTK_SIGNAL_FUNC (check_goto), file_list);
	gtk_signal_connect (GTK_OBJECT
						(GTK_COMBO (file_list->history_combo)->button),
						"clicked", GTK_SIGNAL_FUNC (history_combo_clicked),
						file_list);
	gtk_widget_show (file_list->history_combo);

	paned = gtk_hpaned_new ();
	gtk_box_pack_start (GTK_BOX (main_box), paned, TRUE, TRUE, 0);
	gtk_widget_show (paned);

	file_list->scrolled_window_dir = gtk_scrolled_window_new (0, 0);
	gtk_widget_show (file_list->scrolled_window_dir);
	gtk_paned_add1 (GTK_PANED (paned), file_list->scrolled_window_dir);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
									(file_list->scrolled_window_dir),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW
										 (file_list->scrolled_window_dir),
										 GTK_SHADOW_IN);
	gtk_widget_set_usize (file_list->scrolled_window_dir, 200, 250);

	dir_adjustment =
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
											 (file_list->scrolled_window_dir));
	gtk_signal_connect (GTK_OBJECT (dir_adjustment), "value-changed",
						GTK_SIGNAL_FUNC (dir_scrollbar_value_changed),
						file_list);

	GTK_SCROLLED_WINDOW (file_list->scrolled_window_dir);

	file_list->directory_list = gtk_tree_view_new ();
	gtk_widget_show (file_list->directory_list);
	gtk_container_add (GTK_CONTAINER (file_list->scrolled_window_dir),
					   file_list->directory_list);
	/* Add directory tree model */
	store = gtk_list_store_new (N_DIRECTORY_COLUMNS,
								GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (file_list->directory_list),
							 GTK_TREE_MODEL(store));
	
	/* Add directory tree view columns */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
													   renderer,
													   "pixbuf",
													   DIRECTORY_PIXBUF_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (file_list->directory_list),
								 column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Directories"),
													   renderer,
													   "text",
													   DIRECTORY_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (file_list->directory_list),
								 column);
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (file_list->directory_list));
	
	/* Connect signals */
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_selection_changed_dir), file_list);
	g_signal_connect (G_OBJECT (file_list->directory_list), "row_activated",
					  G_CALLBACK (on_row_activated_dir), file_list);
	g_object_unref (G_OBJECT(store));

	/* File tree view scroll window */
	file_list->scrolled_window_file = gtk_scrolled_window_new (0, 0);
	gtk_widget_show (file_list->scrolled_window_file);
	gtk_paned_add2 (GTK_PANED (paned), file_list->scrolled_window_file);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
									(file_list->scrolled_window_file),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW
										 (file_list->scrolled_window_file),
										 GTK_SHADOW_IN);
	gtk_widget_set_usize (file_list->scrolled_window_file, 200, 250);

	file_adjustment =
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
											 (file_list->scrolled_window_file));
	gtk_signal_connect (GTK_OBJECT (file_adjustment), "value-changed",
						GTK_SIGNAL_FUNC (file_scrollbar_value_changed),
						file_list);

	/* File tree view */
	file_list->file_list = gtk_tree_view_new ();
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (file_list->file_list));
	gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection),
								 GTK_SELECTION_MULTIPLE);
	gtk_widget_show (file_list->file_list);
	gtk_container_add (GTK_CONTAINER (file_list->scrolled_window_file),
					   file_list->file_list);
	
	/* Add tree model */
	store = gtk_list_store_new (N_FILE_COLUMNS,
								GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (file_list->file_list),
							 GTK_TREE_MODEL(store));
	
	/* Add column */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
													   renderer,
													   "pixbuf",
													   FILE_PIXBUF_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (file_list->file_list),
								 column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Files"),
													   renderer,
													   "text",
													   FILE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (file_list->file_list),
								 column);
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (file_list->file_list));
	
	/* Connect signals */
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_selection_changed_file), file_list);
	g_signal_connect (G_OBJECT (file_list->file_list), "row_activated",
					  G_CALLBACK (on_row_activated_file), file_list);
	g_object_unref (G_OBJECT(store));

	util_box = gtk_table_new (2, 2, FALSE);

	// gtk_container_add(GTK_CONTAINER(main_box), util_box);
	gtk_box_pack_start (GTK_BOX (main_box), util_box, FALSE, FALSE, 0);

	gtk_table_set_row_spacings (GTK_TABLE (util_box), 7);
	gtk_table_set_col_spacings (GTK_TABLE (util_box), 7);

	gtk_widget_show (util_box);

	label = gtk_label_new (_("Selection: "));

	gtk_table_attach (GTK_TABLE (util_box), label, 0, 1, 0, 1,
					  (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0,
					  0);

	gtk_widget_show (label);
	file_list->selection_label = label;

	file_list->selection_entry = gtk_entry_new ();

	gtk_table_attach (GTK_TABLE (util_box), file_list->selection_entry, 1, 2, 0,
					  1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_signal_connect (GTK_OBJECT (file_list->selection_entry),
						"key_press_event",
						GTK_SIGNAL_FUNC (selection_entry_key_press), file_list);
	gtk_signal_connect (GTK_OBJECT (file_list->selection_entry), "changed",
						GTK_SIGNAL_FUNC (check_ok_button_cb), file_list);
	gtk_signal_connect (GTK_OBJECT (file_list->selection_entry), "activate",
						GTK_SIGNAL_FUNC (check_goto), file_list);
	gtk_widget_show (file_list->selection_entry);

	if (file_list->filetypes == NULL)
		file_list->filetypes =
			gnome_filelisttype_makedefaultlist (file_list->filetypes);
	combolist = gnome_filelisttype_getcombolist (file_list->filetypes);

	combolabel = gtk_label_new (_("File type: "));
	file_list->filetype_combo = gtk_combo_new ();

	gnome_filelist_set_combolist (file_list, combolist);

	gtk_table_attach (GTK_TABLE (util_box), combolabel, 0, 1, 1, 2,
					  (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0,
					  0);

	gtk_widget_show (combolabel);

	gtk_entry_set_editable (GTK_ENTRY
							(GTK_COMBO (file_list->filetype_combo)->entry),
							FALSE);
	gtk_signal_connect (GTK_OBJECT
						(GTK_COMBO (file_list->filetype_combo)->entry),
						"changed", GTK_SIGNAL_FUNC (filetype_combo_go),
						file_list);

	gtk_table_attach (GTK_TABLE (util_box), file_list->filetype_combo, 1, 2, 1,
					  2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);

	gtk_widget_show (file_list->filetype_combo);

	hsep = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (main_box), hsep, FALSE, FALSE, 10);
	gtk_widget_show (hsep);

	util_box = gtk_hbutton_box_new ();
	gtk_box_pack_start (GTK_BOX (main_box), util_box, FALSE, FALSE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (util_box), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (util_box), GNOME_PAD);
	gtk_widget_show (util_box);

	file_list->cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_box_pack_start (GTK_BOX (util_box), file_list->cancel_button, FALSE,
						FALSE, 5);
	GTK_WIDGET_SET_FLAGS (file_list->cancel_button, GTK_CAN_DEFAULT);
	gtk_widget_show (file_list->cancel_button);

	file_list->ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_box_pack_start (GTK_BOX (util_box), file_list->ok_button, FALSE, FALSE,
						5);
	GTK_WIDGET_SET_FLAGS (file_list->ok_button, GTK_CAN_DEFAULT);
	gtk_widget_set_sensitive (file_list->ok_button, FALSE);
	gtk_widget_show (file_list->ok_button);

	file_list->folder_pixbuf =
		gdk_pixbuf_new_from_file (FILE_PIXMAP_FOLDER, NULL);

	if (!gnome_filelist_set_dir (file_list, path))
		gnome_filelist_set_dir (file_list, g_get_home_dir ());

	file_list->changedironcombo = FALSE;

	gtk_widget_grab_default (file_list->ok_button);
	gtk_widget_grab_focus (file_list->ok_button);

	return GTK_WIDGET (file_list);
}

static gint
selection_entry_key_press (GtkWidget * widget, GdkEventKey * event,
						   GnomeFileList * file_list)
{
	GCompletion *filecmp;
	GList *cmplist = NULL;

	gchar *prefix = NULL, *path = NULL, *prepend_str = NULL, *full_path =
		NULL, *full_prefix = NULL, *new_prefix = NULL, *fullprefix =
		NULL, *str = NULL;
	DIR *dir_file;
	struct dirent *dir;
	gboolean incurdir = FALSE;

	if (event->keyval == '~')
	{
		gchar *text, *newtext;

		text = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
		newtext = g_strdup_printf ("%s%s", text, g_get_home_dir ());
		g_free (text);
		gtk_entry_set_text (GTK_ENTRY (widget), newtext);
		g_free (newtext);
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
		return TRUE;
	}
	if ((event->keyval == GDK_Tab || event->keyval == GDK_KP_Tab))
	{
		filecmp = g_completion_new (NULL);

		/* Extracting the directory and prefix from user-input */
		full_path = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
		full_prefix = g_strdup (full_path);
		prefix = g_strdup (g_basename (full_prefix));
		/* g_path_get_basename() in GNOME 2.0 */
		path = g_dirname (full_path);
		/* g_path_get_dirname() in GNOME 2.0 */
		if (strcmp (path, ".") == 0)
		{
			incurdir = TRUE;
			path = g_dirname (file_list->path);
		}
		/* Populate the GCompletion list (filecmp) */
		if (strcmp (path, "/") == 0)
			prepend_str = g_strdup ("");
		else
			prepend_str = g_strdup (path);
		dir_file = opendir (path);
		if (dir_file)
		{
			while ((dir = readdir (dir_file)))
			{
				str = g_new (char, strlen (dir->d_name) + 1);

				strcpy (str, dir->d_name);
				if ((str[0] != '.'))
				{
					/* If diritem is not hidden, then add it to filecmp */
					GList *items = NULL;
					gchar *fullname;

					fullname = g_strdup_printf ("%s/%s", prepend_str, str);
					items = g_list_append (items, fullname);
					if (filecmp)
						g_completion_add_items (filecmp, items);
				}
			}
			closedir (dir_file);
			g_free (str);
		}

		if (filecmp)
		{
			/* We have a list, lets complete */
			fullprefix = g_strdup_printf ("%s/%s", prepend_str, prefix);
			cmplist = g_completion_complete (filecmp, fullprefix, &new_prefix);
			if (new_prefix)
			{
				file_list->filepattern = g_strdup_printf ("%s*", prefix);
				file_list->dirpattern = g_strdup_printf ("%s*", prefix);
				gnome_filelist_set_dir (file_list, path);
				if ((incurdir) && (file_list->dirs_matched != 1))
					new_prefix = g_strdup (g_basename (new_prefix));
				gtk_entry_set_text (GTK_ENTRY (widget), new_prefix);
				file_list->filepattern = NULL;
				file_list->dirpattern = NULL;
				if ((file_list->dirs_matched == 1) &&
					(file_list->files_matched == 0))
				{
					gnome_filelist_set_dir (file_list, new_prefix);
					gtk_entry_set_text (GTK_ENTRY (widget), new_prefix);
					gtk_entry_append_text (GTK_ENTRY (widget), "/");
				}
				gtk_widget_grab_focus (widget);
				g_free (new_prefix);
			}
			g_completion_free (filecmp);
			g_free (fullprefix);
			g_free (prepend_str);
			g_free (full_path);
			g_free (full_prefix);
			g_free (path);
			g_free (prefix);
			gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
										  "key_press_event");
			return TRUE;
		}

		g_free (fullprefix);
		g_free (prepend_str);
		g_free (full_path);
		g_free (full_prefix);
		g_free (path);
		g_free (prefix);
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
	}

	return FALSE;

}

GnomeFileListType *
gnome_filelisttype_getfiletype (GnomeFileList * file_list,
								const gchar * description)
{
	GList *filetypes;

	for (filetypes = file_list->filetypes; filetypes;
		 filetypes = g_list_next (filetypes))
	{
		GnomeFileListType *filetype = (GnomeFileListType *) filetypes->data;

		if (!g_strcasecmp (filetype->description, description))
			return filetype;
	}

	return NULL;
}

GList *
gnome_filelisttype_addtype_f (GList * filetypes, const gchar * description, ...)
{
	GList *exts = NULL;
	va_list ap;

	va_start (ap, description);

	while (1)
	{
		gchar *ext = va_arg (ap, gchar *);

		if (!ext)
			break;

		exts = g_list_append (exts, g_strdup (ext));
	}

	va_end (ap);

	return gnome_filelisttype_addtype (filetypes, description, exts);
}

GList *
gnome_filelisttype_addtype (GList * filetypes, const gchar * description,
							GList * extentions)
{
	GnomeFileListType *filetype = NULL;
	GList *new_list;
	GList *last;

	filetype = g_malloc0 (sizeof (GnomeFileListType));
	filetype->description = g_strdup (description);
	filetype->extentions = extentions;

	new_list = g_list_alloc ();
	new_list->data = filetype;

	if (filetypes)
	{
		last = g_list_last (filetypes);
		last->next = new_list;
		new_list->prev = last;

		return filetypes;
	}

	return new_list;
}

GList *
gnome_filelisttype_makedefaultlist (GList * filetypes)
{
	GList *ftypes = filetypes;

	ftypes = gnome_filelisttype_addtype (ftypes, _("All files"), NULL);

	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("C/C++ source files"), ".c",
									  ".cc", ".cxx", ".cpp", ".c++", ".cs",
									  ".hpp", ".h", ".hh", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Java files"), ".java", ".js",
									  NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Pascal files"), ".pas", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("PHP files"), ".php", ".php?",
									  ".phtml", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Perl files"), ".pl", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Python files"), ".py", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Hyper Text Markup files"),
									  ".htm", ".html", ".css", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Shell Script files"), ".sh",
									  NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Config files"), ".conf", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Visual Basic files"), ".vb",
									  ".vbs", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Properties files"),
									  ".properties", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _(".mak files"), ".mak", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _(".lua files"), ".lua", NULL);
	ftypes =
		gnome_filelisttype_addtype_f (ftypes, _("Diff files"), "diff",
									  ".cvsdiff", ".patch", NULL);

	return ftypes;
}

GList *
gnome_filelisttype_getdescriptions (GList * filetypes)
{
	GList *rlist = NULL, *ftypes;

	for (ftypes = filetypes; ftypes; ftypes = g_list_next (ftypes))
	{
		GnomeFileListType *filetype = (GnomeFileListType *) ftypes->data;

		rlist = g_list_append (rlist, filetype->description);
	}

	return rlist;
}

GList *
gnome_filelisttype_getcombolist (GList * filetypes)
{
	GList *descriptions = NULL;
	GList *combolist = NULL;

	for (descriptions = gnome_filelisttype_getdescriptions (filetypes);
		 descriptions; descriptions = g_list_next (descriptions))
		combolist = g_list_append (combolist, (gchar *) descriptions->data);

	return combolist;
}

GList *
gnome_filelisttype_getextentions (GList * filetypes, const gchar * description)
{
	GList *rlist = NULL, *ftypes;
	gboolean found = FALSE;

	for (ftypes = filetypes; ftypes; ftypes = g_list_next (ftypes))
	{
		GnomeFileListType *filetype = (GnomeFileListType *) ftypes->data;

		if (!strcmp (filetype->description, description))
		{
			found = TRUE;

			for (; filetype->extentions;
				 filetype->extentions = g_list_next (filetype->extentions))
				rlist = g_list_append (rlist, filetype->extentions->data);
		}
	}

	return found ? rlist : NULL;
}

void
gnome_filelist_set_combolist (GnomeFileList * file_list, GList * combolist)
{

	gtk_combo_set_popdown_strings (GTK_COMBO (file_list->filetype_combo),
								   combolist);
}

GList *
gnome_filelisttype_clearfiletypes (GnomeFileList * file_list)
{
	g_list_free (file_list->filetypes);
	file_list->filetypes = NULL;
	return file_list->filetypes;
}

static gboolean
gnome_filelist_file_matches (GnomeFileListType * filetype,
							 const gchar * filename, gboolean chkwholefile)
{
	GList *extentions = NULL;
	gboolean match = FALSE;

	extentions = g_list_copy (filetype->extentions);
	if (!extentions)
		match = TRUE;

	while (extentions && !match)
	{
		gint result = 0;
		gchar *pattern = NULL;

		if (chkwholefile)
			pattern = g_strdup_printf ("*%s*", (gchar *) extentions->data);
		else
			pattern = g_strdup_printf ("*%s", (gchar *) extentions->data);
		result = fnmatch (pattern, filename, 0);
		g_free (pattern);
		if (!result)
		{
			match = TRUE;
			extentions = g_list_next (extentions);
			break;
		}

		extentions = g_list_next (extentions);
	}

	extentions = g_list_first (extentions);

	return match;
}

static void
history_combo_clicked (GtkWidget * widget, GnomeFileList * file_list)
{
	file_list->changedironcombo = TRUE;
	/* check_goto(widget, file_list); */
}

static void
history_combo_go (GtkWidget * widget, GnomeFileList * file_list)
{
	if (file_list->changedironcombo)
	{
		gchar *text =
			g_strdup (gtk_entry_get_text
					  (GTK_ENTRY
					   (GTK_COMBO (file_list->history_combo)->entry)));

		file_list->changedironcombo = FALSE;
		set_dir_internal (file_list, text);

		g_free (text);

		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "changed");
	}
	else
		check_ok_button_cb (widget, file_list);
}

static void
filetype_combo_go (GtkEntry *entry, GnomeFileList * file_list)
{
	GnomeFileListType *filetype;
	gchar *s;
	const gchar *type_string;
	gchar *filename;

	filename =
		g_strdup (gtk_entry_get_text (GTK_ENTRY (file_list->selection_entry)));
	type_string = gtk_entry_get_text (entry);
	g_message ("Type selected: %s", type_string);
	if (!type_string || strlen (type_string) <= 02)
		return;
	filetype = gnome_filelisttype_getfiletype (file_list, type_string);
	
	if (filetype && (!gnome_filelist_file_matches (filetype, filename, FALSE)))
	{
		GList *extentions = NULL;
		gchar *text;
		gchar *last_dot;
		GString *s;

		if (strlen (filename) <= 0)
		{
			g_free (filename);
			filename = g_strdup ("Newfile");
		}

		extentions = g_list_copy (filetype->extentions);
		extentions = g_list_first (extentions);
		/* This routine comes from The Gimp. [app/gui/file-dialog-utils.c]
		 * Thanks :) */
		text = g_strdup (filename);
		last_dot = strrchr (text, '.');
		if (last_dot == text || !text[0])
		{
			g_free (filename);
			g_free (text);
			return;
		}
		s = g_string_new (text);
		g_free (text);
		if (last_dot)
			g_string_truncate (s, last_dot - text);
		g_string_append (s, (gchar *) extentions->data);
		g_free (filename);
		filename = g_strdup (s->str);
		g_string_free (s, TRUE);
	}

	s = build_full_path (file_list->path, "");
	gnome_filelist_set_dir (file_list, s);
	if (filename)
		gtk_entry_set_text (GTK_ENTRY (file_list->selection_entry), filename);

	g_free (filename);
	g_free (s);
}

static void
gnome_filelist_destroy (GtkObject * object)
{
	GtkWidget *file_list;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILELIST (object));

	file_list = GTK_WIDGET (object);

	if (GNOME_FILELIST (file_list)->path)
		g_free (GNOME_FILELIST (file_list)->path);

	if (GNOME_FILELIST (file_list)->selected)
		g_free (GNOME_FILELIST (file_list)->selected);

	g_list_foreach (GNOME_FILELIST (file_list)->history, (GFunc) g_free, NULL);
	g_list_free (GNOME_FILELIST (file_list)->history);

	gtk_widget_destroy (file_list);
}

static void
gnome_filelist_show (GtkWidget * widget, GnomeFileList * file_list)
{
	gchar *string;
	GtkAdjustment *file_adjustment;
	GtkAdjustment *dir_adjustment;
	gfloat file_value = file_list->file_scrollbar_value;
	gfloat dir_value = file_list->dir_scrollbar_value;

	string = build_full_path (file_list->path, "");
	gnome_filelist_set_dir (file_list, string);
	g_free (string);

	file_adjustment =
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
											 (file_list->scrolled_window_file));
	gtk_adjustment_set_value (file_adjustment, file_value);
	gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW
										 (file_list->scrolled_window_file),
										 file_adjustment);

	dir_adjustment =
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
											 (file_list->scrolled_window_dir));
	gtk_adjustment_set_value (dir_adjustment, dir_value);
	gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW
										 (file_list->scrolled_window_dir),
										 dir_adjustment);

	gtk_widget_grab_focus (file_list->selection_entry);

	return;
}

static void
gnome_filelist_hide (GtkWidget * widget, GnomeFileList * file_list)
{
	return;
}

static void
file_scrollbar_value_changed (GtkAdjustment * adjustment,
							  GnomeFileList * file_list)
{
	file_list->file_scrollbar_value = adjustment->value;
}

static void
dir_scrollbar_value_changed (GtkAdjustment * adjustment,
							 GnomeFileList * file_list)
{
	file_list->dir_scrollbar_value = adjustment->value;
}

static void
on_selection_changed_file (GtkTreeSelection * selection, GnomeFileList * file_list)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint count;
	gchar *selected;

	g_free (file_list->selected);
	file_list->selected = NULL;
	
	count = gtk_tree_selection_count_selected_rows(selection);
	if (count > 0)
	{
		GList *list, *last;
		
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		last = g_list_last (list);
		gtk_tree_model_get_iter (model, &iter, last->data);
		gtk_tree_model_get (model, &iter, DIRECTORY_COLUMN, &selected, -1);
		if (selected && strlen (selected) > 0)
		{
			if (file_list->selected)
				g_free (file_list->selected);
			file_list->selected = g_strdup (selected);
			set_file_selection (file_list);
		}
		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

static void
on_selection_changed_dir (GtkTreeSelection * selection,
						  GnomeFileList * file_list)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint valid;
	gchar *selected;

	g_free (file_list->selected);
	file_list->selected = NULL;
	
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		gtk_tree_model_get (model, &iter, FILE_COLUMN, &selected, -1);
		if (selected && strlen (selected) > 0)
		{
			if (file_list->selected)
				g_free (file_list->selected);
			file_list->selected = g_strdup (selected);
			set_file_selection (file_list);
		}
	}
}

static void
on_row_activated_file  (GtkTreeView     *treeview, GtkTreePath     *arg1,
						GtkTreeViewColumn *arg2, GnomeFileList   *file_list)
{
	set_file_selection (file_list);
	gtk_signal_emit_by_name (GTK_OBJECT (file_list->ok_button),
							 "clicked", NULL);
}

static void
on_row_activated_dir   (GtkTreeView     *treeview, GtkTreePath     *arg1,
						GtkTreeViewColumn *arg2, GnomeFileList   *file_list)
{
	gchar *path;
	if (!strcmp (file_list->selected, "."))
	{
		gnome_filelist_set_dir (file_list, file_list->path);
	}
	else if (!strcmp (file_list->selected, ".."))
	{
#ifdef DEBUG
		g_message ("Original file path: %s\n", file_list->path);
#endif
		path = get_parent_dir (file_list->path);
		g_free (file_list->path);
		file_list->path = path;
#ifdef DEBUG
		g_message ("Original file path: %s\n", file_list->path);
#endif
		gnome_filelist_set_dir (file_list, file_list->path);
	}
	else
	{
		path = build_full_path (file_list->path, file_list->selected);
		gnome_filelist_set_dir (file_list, path);
		g_free (path);
	}
}

static void
gnome_filelist_get_dirs (GnomeFileList * file_list)
{
	DIR *dir_file;
	struct dirent *dir;
	struct stat st;
	gchar filename[384];

	GtkTreeIter iter;
	GtkTreeModel *file_model, *dir_model;
	GList *dirs_list = NULL;
	GList *files_list = NULL;
	GList *temp;
	GnomeFileListType *filetype;

	file_model = gtk_tree_view_get_model (GTK_TREE_VIEW (file_list->file_list));
	dir_model = gtk_tree_view_get_model (GTK_TREE_VIEW (file_list->directory_list));
	gtk_list_store_clear (GTK_LIST_STORE (file_model));
	gtk_list_store_clear (GTK_LIST_STORE (dir_model));
	
	filetype =
		gnome_filelisttype_getfiletype (file_list,
										gtk_entry_get_text (GTK_ENTRY
															(GTK_COMBO
															 (file_list->
															  filetype_combo)->
															 entry)));

	file_list->dirs_matched = 0;
	file_list->files_matched = 0;
	dir_file = opendir (file_list->path);
	if (dir_file != NULL)
	{
		while ((dir = readdir (dir_file)) != NULL)
		{
			g_snprintf (filename, sizeof (filename), "%s%s", file_list->path,
						dir->d_name);

			stat (filename, &st);
			if (S_ISDIR (st.st_mode))
			{
				if (dir->d_name[0] != '.' || file_list->show_hidden)
				{
					if (file_list->dirpattern)
					{
						if (fnmatch (file_list->dirpattern, dir->d_name, 0) ==
							0)
						{
							dirs_list =
								g_list_prepend (dirs_list,
												g_strdup (dir->d_name));
							file_list->dirs_matched++;
						}
					}
					else
					{
						dirs_list =
							g_list_prepend (dirs_list, g_strdup (dir->d_name));
						file_list->dirs_matched++;
					}
				}
			}
			else
			{
				if (dir->d_name[0] != '.' || file_list->show_hidden)
				{
					if (gnome_filelist_file_matches
						(filetype, dir->d_name, FALSE))
						files_list =
							g_list_prepend (files_list, g_strdup (dir->d_name));
				}
			}
		}

		closedir (dir_file);

		dirs_list = g_list_sort (dirs_list, (GCompareFunc) dir_compare_func);
		files_list = g_list_sort (files_list, (GCompareFunc) dir_compare_func);

		temp = dirs_list;
		while (temp != NULL)
		{
			gtk_list_store_append (GTK_LIST_STORE (dir_model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (dir_model), &iter,
								DIRECTORY_PIXBUF_COLUMN,
								file_list->folder_pixbuf,
								DIRECTORY_COLUMN, (gchar*) temp->data, -1);
			g_free (temp->data);
			temp = g_list_next (temp);
		}
		g_list_free (dirs_list);

		temp = files_list;
		while (temp != NULL)
		{
			GdkPixbuf *pixbuf;
			gchar *uri;
			uri = g_strconcat (file_list->path, "/", (gchar*) temp->data, NULL);
			pixbuf = gdl_icons_get_uri_icon (icon_set, uri);
			g_free (uri);
			gtk_list_store_append (GTK_LIST_STORE (file_model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (file_model), &iter,
								FILE_PIXBUF_COLUMN,
								pixbuf,
								FILE_COLUMN, (gchar*) temp->data, -1);
			gdk_pixbuf_unref (pixbuf);
			g_free (temp->data);
			temp = g_list_next (temp);
		}
		g_list_free (files_list);
	}
#ifdef DEBUG
	else
		g_warning ("Directory %s not found!", file_list->path);
#endif
}

static gint
dir_compare_func (gchar * txt1, gchar * txt2)
{
	return (strcmp (txt1, txt2));
}

static void
set_file_selection (GnomeFileList * file_list)
{
	gchar *temp;

	/* 
	 * Should not call getcwd(), because meantime the dir might have been
	 * changed by the user and it causes probem when cwd != file_list->path
	 * 
	 * Using the already existing file_list->path is fine. */
	if (file_list->path)
	{
		gtk_entry_set_text (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry),
							file_list->path);
		temp = file_list->path + strlen (file_list->path);
		if (temp > file_list->path)
			temp--;
		if (strcmp (file_list->path, "/") && *temp != '/')
			gtk_entry_append_text (GTK_ENTRY
								   (GTK_COMBO (file_list->history_combo)->
									entry), "/");
	}
	if (file_list->selected)
		gtk_entry_set_text (GTK_ENTRY (file_list->selection_entry),
							file_list->selected);
	else
		gtk_entry_set_text (GTK_ENTRY (file_list->selection_entry), "");
}

static gboolean
set_dir_internal (GnomeFileList * file_list, const gchar * path)
{
	gchar *path_bak;

	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (file_list != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_FILELIST (file_list), FALSE);

	path_bak = g_get_current_dir ();

	if (chdir (path))
	{
		g_free (path_bak);

		return FALSE;
	}

	chdir (path_bak);
	g_free (path_bak);

	// temp = file_list->path;
	if (file_list->path)
		g_free (file_list->path);
	file_list->path = build_full_path (path, "");
	// strcpy(file_list->path, path);
	gnome_filelist_get_dirs (file_list);
	g_free (file_list->selected);
	file_list->selected = NULL;

	set_file_selection (file_list);
	gtk_widget_grab_focus (file_list->directory_list);
	return TRUE;
}

gchar *
gnome_filelist_get_filename (GnomeFileList * file_list)
{
	const gchar *path = NULL;
	const gchar *filename = NULL;
	gchar *full = NULL;

	g_return_val_if_fail (file_list != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILELIST (file_list), NULL);
	path =
		gtk_entry_get_text (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry));
	filename = gtk_entry_get_text (GTK_ENTRY (file_list->selection_entry));
	full = build_full_path (path, filename);
	return (full);
}

static void
on_selected_foreach (GtkTreeModel *model, GtkTreePath *path,
					  GtkTreeIter *iter, gpointer data)
{
	const gchar *file;
	GList **list_ptr = data;
	
	gtk_tree_model_get (model, iter, FILE_COLUMN, &file, -1);
	if (file && strlen (file) > 0)
	{
		*list_ptr = g_list_append (*list_ptr, g_strdup (file));
	}
}

GList *
gnome_filelist_get_filelist (GnomeFileList * file_list)
{
	GtkTreeSelection *selection;
	const gchar *path = NULL;
	GList *list = NULL;
	GList *node;

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (file_list->file_list));
	gtk_tree_selection_selected_foreach (selection, on_selected_foreach, &list);	
	path =
			gtk_entry_get_text (GTK_ENTRY
								(GTK_COMBO (file_list->history_combo)->
								 entry));

	node = list;
	while (node)
	{
		gchar *full = NULL;
		
		full = g_strconcat (path, node->data, NULL);
		g_free (node->data);
		node->data = full;
		node = g_list_next (node);
	}
	return list;
}

gchar *
gnome_filelist_get_path (GnomeFileList * file_list)
{
	g_return_val_if_fail (file_list != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILELIST (file_list), NULL);
	return (file_list->path);
}

gboolean
gnome_filelist_set_filename (GnomeFileList * file_list, const gchar * fname)
{
	gchar *dir;

	g_return_val_if_fail (fname != NULL, FALSE);
	g_return_val_if_fail (file_list != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_FILELIST (file_list), FALSE);

	dir = g_dirname (fname);
	if (dir)
	{
		if (gnome_filelist_set_dir (file_list, dir))
		{
			g_free (file_list->selected);
			file_list->selected = g_strdup (g_basename (fname));
			set_file_selection (file_list);
			g_free (dir);
			return TRUE;
		}
		else
		{
			g_free (file_list->selected);
			file_list->selected = NULL;
			g_free (dir);
			return FALSE;
		}
	}
	else
	{
		g_free (file_list->selected);
		file_list->selected = NULL;
		return FALSE;
	}
}

gboolean
gnome_filelist_set_dir (GnomeFileList * file_list, const gchar path[])
{
	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (file_list != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_FILELIST (file_list), FALSE);

	if (set_dir_internal (file_list, path))
	{
		if (!file_list->history ||
			strcmp ((gchar *) file_list->history->data, file_list->path) != 0)
		{
			gboolean history_set = FALSE;
			GList *node = file_list->history;
			while (node)
			{
				gchar *str = node->data;
				if (strcmp (str, path) == 0)
				{
					file_list->history = g_list_remove (file_list->history, str);
					file_list->history = g_list_prepend (file_list->history, str);
					history_set = TRUE;
					break;
				}
				node = g_list_next (node);
			}
			if (history_set == FALSE)
				file_list->history = 
					g_list_prepend (file_list->history,
									g_strdup (file_list->path));
			gtk_combo_set_popdown_strings (GTK_COMBO (file_list->history_combo),
										   file_list->history);
			file_list->history_position = -1;
		}
		return TRUE;
	}
	return FALSE;
}

void
gnome_filelist_set_show_hidden (GnomeFileList * file_list, gboolean show_hidden)
{
	g_return_if_fail (file_list != NULL);
	g_return_if_fail (GNOME_IS_FILELIST (file_list));
	file_list->show_hidden = show_hidden;
}

void
gnome_filelist_set_title (GnomeFileList * file_list, const gchar * title)
{
	g_return_if_fail (file_list != NULL);
	g_return_if_fail (GNOME_IS_FILELIST (file_list));
	gtk_window_set_title (GTK_WINDOW (file_list), title);
}

static gboolean
gnome_filelist_check_dir_exists (const gchar * path)
{
	DIR *dir_file;

	if (!path)
		return (FALSE);
	dir_file = opendir (path);
	if (dir_file == NULL)
		return (FALSE);
	closedir (dir_file);
	return (TRUE);
}

#if 0
static gint
gnome_filelist_key_press (GtkWidget * widget, GdkEventKey * event)
{
	GnomeFileList *file_list;
	GtkCList *clist;
	gint row;
	gint skip_rows = 0;
	gfloat left = 0.0;
	GtkCTreeNode *node;
	GdkPixmap *pixmap;
	gchar path[384] = "";
	gchar *util;
	gboolean del = FALSE;

	clist = GTK_CLIST (widget);
	
	/* What the fuck is this?? */
	/* file_list = (GnomeFileList *) widget->parent->parent->parent->parent; */
	file_list = GNOME_FILELIST (gtk_widget_get_toplevel (widget));
	
	row = file_list->selected_row > -1 ? file_list->selected_row : 0;
	skip_rows = clist->clist_window_height / (clist->row_height + 1);
	left = (float) clist->clist_window_height / (float) (clist->row_height + 1);
	left -= skip_rows;
	if (left < .5)
		skip_rows--;
	if (event->keyval == GDK_Return && file_list->selected_row >= 0)
	{
		node = gtk_ctree_node_nth (GTK_CTREE (widget), file_list->selected_row);
		gtk_ctree_node_get_pixtext (GTK_CTREE (widget), node, 0, NULL, NULL,
									&pixmap, NULL);
		if (pixmap == file_list->folder_pixmap)	 // its a folder 
		{
			if (!strcmp (file_list->selected, "."))
			{
				gnome_filelist_set_dir (file_list, file_list->path);
				file_list->history_position = -1;
			}
			else if (!strcmp (file_list->selected, ".."))
			{
				if ((util = strrchr (file_list->path, '/')))
				{
					if (util == file_list->path)
						file_list->path[1] = '\0';
					else
						*util = '\0';
					gnome_filelist_set_dir (file_list, file_list->path);
					file_list->history_position = -1;
				}
			}
			else
			{
				strcat (path, file_list->path);
				if (strcmp (file_list->path, "/"))
					strcat (path, "/");
				strcat (path, file_list->selected);
				gnome_filelist_set_dir (file_list, path);
				file_list->history_position = -1;
			}
		}
		else
		{
			gtk_signal_emit_by_name (GTK_OBJECT (file_list->ok_button),
									 "clicked", NULL);
		}
		return (TRUE);
	}
	else if (event->keyval == GDK_Up)
		row--;
	else if (event->keyval == GDK_Down)
		row++;
	else if (event->keyval == GDK_Page_Up)
		row -= skip_rows;
	else if (event->keyval == GDK_Page_Down)
		row += skip_rows;
	else if (event->keyval == GDK_Delete)
	{
		gtk_signal_emit_by_name (GTK_OBJECT (file_list->delete_button),
								 "clicked", file_list);
		gtk_widget_grab_focus (GTK_WIDGET (clist));
		del = TRUE;
	}
	else if (event->keyval == 'R' || event->keyval == 'r')
	{
		gtk_signal_emit_by_name (GTK_OBJECT (file_list->rename_button),
								 "clicked", file_list);
		gtk_widget_grab_focus (GTK_WIDGET (clist));
		del = TRUE;
	}
	else if (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R)
		gnome_filelist_set_multiple_selection (file_list,
											   !file_list->multiple_selection);

	if (row >= 0 && row < clist->rows)
	{
		gtk_clist_select_row (clist, row, 0);
		if (del)
			gtk_clist_moveto (clist, row, 0, 0.0, 0.0);
	}
	else if (row < 0 && clist->rows)
	{
		gtk_clist_select_row (clist, 0, 0);
		gtk_clist_moveto (clist, 0, 0, 0.0, 0.0);
	}
	else if (row > clist->rows)
	{
		gtk_clist_select_row (clist, clist->rows - 1, 0);
		gtk_clist_moveto (clist, clist->rows - 1, 0, 1.0, 1.0);
	}
	return (TRUE);
}
#endif

static void
refresh_listing (GtkWidget * widget, GnomeFileList * file_list)
{
	gchar *string;

	string = build_full_path (file_list->path, "");
	if (!gnome_filelist_set_dir (file_list, string))
	{
		home_directory_cb (NULL, file_list);
	}
	g_free (string);
}

static void
goto_parent (GtkWidget * widget, GnomeFileList * file_list)
{
	gchar *string, *parent;

	string = build_full_path (file_list->path, "");
	parent = get_parent_dir (string);
	if (!gnome_filelist_set_dir (file_list, parent))
	{
		home_directory_cb (NULL, file_list);
	}
	file_list->history_position = -1;
	g_free (parent);
	g_free (string);
}

static void
goto_prev (GtkWidget * widget, GnomeFileList * file_list)
{
	gpointer temp = NULL;

	g_free (file_list->selected);
	file_list->selected = NULL;
	file_list->history_position++;

	if ((temp =
		 g_list_nth_data (file_list->history, file_list->history_position + 1)))
	{
		set_dir_internal (file_list, (gchar *) temp);
		check_ok_button_cb (NULL, file_list);
	}
}

static void
goto_next (GtkWidget * widget, GnomeFileList * file_list)
{
	gpointer temp = NULL;

	g_free (file_list->selected);
	file_list->selected = NULL;
	file_list->history_position--;

	if ((temp =
		 g_list_nth_data (file_list->history, file_list->history_position + 1)))
	{
		set_dir_internal (file_list, (gchar *) temp);
		check_ok_button_cb (NULL, file_list);
	}
}

static void
check_ok_button_cb (GtkWidget * widget, GnomeFileList * file_list)
{
	const gchar *path, *selected;
	/* GList *selected_list; */
	gchar *s;

#if 0
	selected_list = GTK_CLIST (file_list->file_list)->selection;
	if (g_list_length (selected_list) > 0)
	{
		gtk_widget_set_sensitive (file_list->ok_button, TRUE);
		gtk_widget_set_sensitive (file_list->delete_button, FALSE);
		gtk_widget_set_sensitive (file_list->rename_button, FALSE);
		return;
	}
#endif
	
	path =
		gtk_entry_get_text (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry));
	selected = gtk_entry_get_text (GTK_ENTRY (file_list->selection_entry));
	s = build_full_path (path, selected);

	if (gnome_filelist_check_dir_exists (s))
	{
		gtk_widget_set_sensitive (file_list->ok_button, FALSE);
		gtk_widget_set_sensitive (file_list->delete_button, FALSE);
		gtk_widget_set_sensitive (file_list->rename_button, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (file_list->ok_button, TRUE);
		if (selected && strlen (selected) && check_if_file_exists (s) &&
			check_can_modify (s))
		{
			gtk_widget_set_sensitive (file_list->delete_button, TRUE);
			gtk_widget_set_sensitive (file_list->rename_button, TRUE);
		}
		else
		{
			gtk_widget_set_sensitive (file_list->delete_button, FALSE);
			gtk_widget_set_sensitive (file_list->rename_button, FALSE);
		}
	}

	gtk_widget_set_sensitive (file_list->back_button,
							  file_list->history_position + 2 <
							  g_list_length (file_list->history));
	gtk_widget_set_sensitive (file_list->forward_button,
							  file_list->history_position + 1 > 0);

	g_free (s);
}

gchar *
get_parent_dir (const gchar * path)
{
	gchar *str, *string, *path_bak, *return_str = NULL;

	str = build_full_path (path, "");
	string = g_new (char, strlen (str) + 4);

	strcpy (string, str);
	strcat (string, "../");
	g_free (str);

	path_bak = g_get_current_dir ();

	if (!chdir (string))
	{
		return_str = g_get_current_dir ();
		chdir (path_bak);
	}

	g_free (path_bak);
	g_free (string);

	return (return_str);
}

gchar *
build_full_path (const gchar * path, const gchar * selection)
{
	gchar *ptr = NULL;
	gchar chr;
	gint malloc_size = 0;
	gint offset = 0;
	
	g_return_val_if_fail (path != NULL, NULL);

	if (selection[0] == '/')
	{
		ptr = g_strdup (selection);
		return ptr;
	}

	if (selection[0] == '~')
	{
		/* Expand "~user" or "~" into the appropriate home directory: */

		gchar *homedir = NULL;
		struct passwd *pw;
		gint i;

		/* Look for slash or end of string: */
		for (i = 1; selection[i] != '\0' && selection[i] != '/'; i++)
			;
		if (i == 1)
		{
			/* We have "~" or "~/...": */
			pw = getpwuid (getuid ());
			if (pw != NULL)
				homedir = g_strdup (pw->pw_dir);
		}
		else
		{
			gchar *username = g_strdup (selection + 1);

			username[i - 1] = '\0';
			pw = getpwnam (username);
			if (pw != NULL)
				homedir = g_strdup (pw->pw_dir);
			g_free (username);
		}

		if (homedir != NULL)
		{
			ptr = g_new (char, strlen (homedir) + strlen (selection + i) + 1);

			strcpy (ptr, homedir);
			strcat (ptr, selection + i);
			g_free (homedir);
			return ptr;
		}

		/* Could not expand ~ -- return string as is and let caller fail... */
		return g_strdup (selection);
	}

	offset = strlen (path) - 1;
	if (offset < 0)
		chr = '\0';
	else
		chr = path[offset];
	malloc_size = strlen (path) + 1;
	malloc_size += chr == '/' && offset ? 0 : 1;
	malloc_size += selection ? strlen (selection) : 0;
	ptr = g_new (char, malloc_size);
	strcpy (ptr, path);
	if (chr != '/' && offset)
		strcat (ptr, "/");
	if (selection)
		strcat (ptr, selection);
	return (ptr);
}

static void
check_goto (GtkWidget * widget, GnomeFileList * file_list)
{
	gchar *string;
	const gchar *path, *selected;

	path =
		gtk_entry_get_text (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry));
	selected = gtk_entry_get_text (GTK_ENTRY (file_list->selection_entry));
	string = build_full_path (path, selected);

	if (!gnome_filelist_set_dir (file_list, string) &&
		GTK_WIDGET_IS_SENSITIVE (file_list->ok_button))
		gtk_signal_emit_by_name (GTK_OBJECT (file_list->ok_button), "clicked",
								 NULL);
	else
		file_list->history_position = -1;
	g_free (string);
}

static void
delete_file (GtkWidget * widget, GnomeFileList * file_list)
{
	GtkWidget *dialog;
	const gchar *path;
	const gchar *selected;
	gchar *full;
	gchar *string;
	gint result;

	path =
		gtk_entry_get_text (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry));
	selected = gtk_entry_get_text (GTK_ENTRY (file_list->selection_entry));
	full = build_full_path (path, selected);
	
	string = g_strdup_printf (_("Really delete %s?"), selected);
	dialog = gtk_message_dialog_new (GTK_WINDOW (file_list),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO,
									 string, NULL);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	if (result == GTK_RESPONSE_YES)
	{
		remove (full);
		refresh_listing (NULL, file_list);
	}
	g_free (full);
}

static void
rename_file (GtkWidget * widget, GnomeFileList * file_list)
{
	GtkWidget *dialog;
	GtkWidget *vbox1;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *entry;
	const gchar *selected, *path;
	gchar *full, *full_new_name;
			
	path =
		gtk_entry_get_text (GTK_ENTRY
							(GTK_COMBO (file_list->history_combo)->entry));
	selected = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
	full = build_full_path (path, selected);

	dialog =
		gtk_dialog_new_with_buttons (_("Rename file"),
									 GTK_WINDOW (file_list),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_STOCK_OK, GTK_RESPONSE_OK,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	vbox1 = GTK_DIALOG (dialog)->vbox;
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 5);
	
	label = gtk_label_new (_("Rename file to:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	gtk_widget_grab_focus (entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_entry_set_text (GTK_ENTRY (entry), selected);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		const gchar* new_name = gtk_entry_get_text (GTK_ENTRY (entry));
		if (new_name && strlen (new_name) > 0)
		{
			full_new_name = build_full_path (path, new_name);
			if (rename (full, full_new_name))
			{
				GtkWidget *dialog =
					gtk_message_dialog_new (GTK_WINDOW (file_list),
											GTK_DIALOG_DESTROY_WITH_PARENT,
											GTK_MESSAGE_ERROR,
											GTK_BUTTONS_CLOSE,
											_("Renaming file failed '%s': %s"),
											selected, g_strerror (errno));
				gtk_dialog_set_default_response (GTK_DIALOG (dialog),
												 GTK_RESPONSE_CLOSE);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			}
			else
				refresh_listing (NULL, file_list);
			g_free (full_new_name);
		}
	}
	gtk_widget_destroy (dialog);
	g_free (full);
}

static void
create_dir_real (const gchar *dirname, GnomeFileList * file_list)
{
	gchar *indir, *full;

	if (!dirname || strlen (dirname) <= 0)
		return;
	
	indir = gnome_filelist_get_path (file_list);
	full = g_strdup_printf ("%s%s", indir, dirname);
	if (mkdir (full, (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != 0)
	{
		GtkWidget *dialog =
			gtk_message_dialog_new (GTK_WINDOW (file_list),
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_ERROR,
									GTK_BUTTONS_CLOSE,
									_("Creating folder failed '%s': %s"),
								    dirname, g_strerror (errno));
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
										 GTK_RESPONSE_CLOSE);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	else
	{
		gchar *string;
		string = build_full_path (file_list->path, "");
		gnome_filelist_set_dir (file_list, string);
		g_free (string);
	}
}

static void
create_dir (GtkWidget * widget, GnomeFileList * file_list)
{
	GtkWidget *dialog;
	GtkWidget *vbox1;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *entry;

	/* The window and the entry are in the _GnomeFileList struct */

	dialog =
		gtk_dialog_new_with_buttons (_("Create new folder"),
									 GTK_WINDOW (file_list),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_STOCK_OK, GTK_RESPONSE_OK,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	vbox1 = GTK_DIALOG (dialog)->vbox;
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 5);
	
	label = gtk_label_new (_("Folder name:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	gtk_widget_grab_focus (entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
		create_dir_real (gtk_entry_get_text (GTK_ENTRY (entry)), file_list);
	gtk_widget_destroy (dialog);
}

static void
home_directory_cb (GtkButton * button, GnomeFileList * file_list)
{
	const gchar *home;

	home = gnome_util_user_home ();
	gnome_filelist_set_dir (GNOME_FILELIST (file_list), home);
}

static gboolean
check_if_file_exists (const gchar * filename)
{
	struct stat st;

	return (gboolean) ((0 == stat (filename, &st)) && (S_ISREG (st.st_mode)));
}

static gboolean
check_can_modify (const gchar * filename)
{
	FILE *file;

	if (!(file = fopen (filename, "a")))
		return (FALSE);
	fclose (file);
	return (TRUE);
}

static void
show_hidden_toggled (GtkToggleButton * b, gpointer data)
{
	GnomeFileList *file_list = data;

	g_return_if_fail (file_list != NULL);
	g_return_if_fail (GNOME_IS_FILELIST (file_list));
	file_list->show_hidden = gtk_toggle_button_get_active (b);
	refresh_listing (NULL, file_list);
}

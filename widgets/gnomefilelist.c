#include "gnomefilelist.h"

#include "../pixmaps/bfoldc.xpm"
#include "../pixmaps/file_file.xpm"

/* function declarations */
static void gnome_filelist_class_init(GnomeFileListClass *klass);
static void gnome_filelist_init(GnomeFileList *file_list);
static void gnome_filelist_destroy(GtkObject *object);
static void file_select_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeFileList *file_list);
static void file_unselect_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeFileList *file_list);
static void gnome_filelist_get_dirs(GnomeFileList *file_list);
static gint dir_compare_func(gchar *txt1, gchar *txt2);
static void set_file_selection(GnomeFileList *file_list);
static gboolean gnome_filelist_check_dir_exists(gchar *path);
static gboolean check_list_for_entry(GList *list, gchar *new_entry);
static gint gnome_filelist_key_press(GtkWidget *widget, GdkEventKey *event);
static void refresh_listing(GtkWidget *widget, GnomeFileList *file_list);
static void goto_directory(GtkWidget *widget, GnomeFileList *file_list);
static void goto_parent(GtkWidget *widget, GnomeFileList *file_list);
static void goto_last(GtkWidget *widget, GnomeFileList *file_list);
static void check_ok_button_cb(GtkWidget *widget, GnomeFileList *file_list);
gchar *get_parent_dir(gchar *path);
gchar *build_full_path(const gchar *path, const gchar *selection);
static void check_goto(GtkWidget *widget, GnomeFileList *file_list);
static void delete_file(GtkWidget *widget, GnomeFileList *file_list);
static void rename_file(GtkWidget *widget, GnomeFileList *file_list);
static void home_directory_cb (GtkButton * button, GnomeFileList *file_list);
static gboolean check_if_file_exists(gchar *filename);
static gboolean check_can_modify(gchar *filename);
/* end function declarations */

static GtkWindowClass *parent_class = NULL;

GtkType gnome_filelist_get_type(void)
{
   static GtkType filelist_type = 0;

   if (!filelist_type) 
   {
      static const GtkTypeInfo filelist_info = 
      {	
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

static void gnome_filelist_class_init(GnomeFileListClass *klass)
{
   GtkObjectClass *object_class;
   
   object_class = (GtkObjectClass*)klass;
   object_class->destroy = gnome_filelist_destroy;
   
   parent_class = gtk_type_class(GTK_TYPE_WINDOW);
}

static void gnome_filelist_init(GnomeFileList *file_list)
{
   file_list->history_combo = NULL;
   file_list->directory_list = NULL;
   file_list->file_list = NULL;
   file_list->history = NULL;
   file_list->path = 0;
   file_list->selected = 0;
   file_list->selected_row = -1;
   file_list->history_position = -1;
}

GtkWidget *gnome_filelist_new()
{
   GtkWidget *file_list;
   gchar *home_dir;

   home_dir = g_get_home_dir ();
   file_list = gnome_filelist_new_with_path (home_dir);
   g_free (home_dir);

   return file_list;
}

GtkWidget *gnome_filelist_new_with_path(gchar *path)
{
   GnomeFileList *file_list;
   GtkWidget *main_box;
   GtkWidget *util_box;
   GtkWidget *toolbar;
   GtkWidget *pixmapwid;
   GtkWidget *scrolled_window;
   GtkWidget *paned;
   GtkWidget *label;
   GtkWidget *hsep;
   GtkWidget *parent_button;
   GtkWidget *refresh_button;   
   
   file_list = gtk_type_new(GNOME_TYPE_FILELIST);
   gtk_container_set_border_width(GTK_CONTAINER(file_list), 5);

   main_box = gtk_vbox_new(FALSE, 5);
   gtk_container_add(GTK_CONTAINER(file_list), main_box);
   gtk_widget_show(main_box);

   toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
   gtk_box_pack_start(GTK_BOX(main_box), toolbar, TRUE, TRUE, 0);
   gtk_widget_show(toolbar);

   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_BACK, 21, 21);
   file_list->back_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Go back to a previous directory", 0, pixmapwid, GTK_SIGNAL_FUNC(goto_last), file_list);
   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_FORWARD, 21, 21);
   file_list->forward_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Go to selected directory", 0, pixmapwid, GTK_SIGNAL_FUNC(goto_directory), file_list);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_HOME, 21, 21);
   file_list->home_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Go to home directory", 0, pixmapwid, GTK_SIGNAL_FUNC(home_directory_cb), file_list);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_UNDO, 21, 21);
   parent_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Go to parent directory", 0, pixmapwid, GTK_SIGNAL_FUNC(goto_parent), file_list);
   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_REFRESH, 21, 21);
   refresh_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Refresh listing", 0, pixmapwid, GTK_SIGNAL_FUNC(refresh_listing), file_list);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_CLOSE, 21, 21);
   file_list->delete_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Delete file", 0, pixmapwid, GTK_SIGNAL_FUNC(delete_file), file_list); 
   pixmapwid = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(file_list), GNOME_STOCK_PIXMAP_CONVERT, 21, 21);
   file_list->rename_button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 0, "Rename file", 0, pixmapwid, GTK_SIGNAL_FUNC(rename_file), file_list);

   util_box = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(main_box), util_box, TRUE, TRUE, 0);
   gtk_widget_show(util_box);

   label = gtk_label_new("Directory: ");
   gtk_box_pack_start(GTK_BOX(util_box), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   file_list->history_combo = gtk_combo_new();
   gtk_combo_disable_activate(GTK_COMBO(file_list->history_combo));
   gtk_box_pack_start(GTK_BOX(util_box), file_list->history_combo, TRUE, TRUE, 0);
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry), FALSE);
   gtk_signal_connect(GTK_OBJECT(GTK_COMBO(file_list->history_combo)->entry), "changed", GTK_SIGNAL_FUNC(check_ok_button_cb), file_list);
   gtk_signal_connect(GTK_OBJECT(GTK_COMBO(file_list->history_combo)->entry), "activate", GTK_SIGNAL_FUNC(check_goto), file_list);
   gtk_widget_show(file_list->history_combo);

   paned = gtk_hpaned_new();
   gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);
   gtk_widget_show(paned);

   scrolled_window = gtk_scrolled_window_new(0, 0);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_set_usize(scrolled_window, 200, 250);
   gtk_widget_show(scrolled_window);

   file_list->directory_list = gtk_ctree_new(1, 0);
   gtk_clist_set_selection_mode(GTK_CLIST(file_list->directory_list), GTK_SELECTION_SINGLE);
   gtk_clist_set_shadow_type(GTK_CLIST(file_list->directory_list), GTK_SHADOW_ETCHED_IN);
   gtk_clist_set_column_justification(GTK_CLIST(file_list->directory_list), 0, GTK_JUSTIFY_LEFT);
   gtk_clist_set_row_height(GTK_CLIST(file_list->directory_list), 18);
   gtk_clist_column_titles_show(GTK_CLIST(file_list->directory_list));
   gtk_ctree_set_expander_style(GTK_CTREE(file_list->directory_list), GTK_CTREE_EXPANDER_NONE);
   gtk_ctree_set_line_style (GTK_CTREE(file_list->directory_list), GTK_CTREE_LINES_NONE);
   gtk_clist_column_titles_passive(GTK_CLIST(file_list->directory_list));
   gtk_container_add(GTK_CONTAINER(scrolled_window), file_list->directory_list);
   gtk_signal_connect(GTK_OBJECT(file_list->directory_list), "key_press_event", GTK_SIGNAL_FUNC(gnome_filelist_key_press), 0);
   gtk_signal_connect(GTK_OBJECT(file_list->directory_list), "select_row", GTK_SIGNAL_FUNC(file_select_event), file_list);
   gtk_signal_connect(GTK_OBJECT(file_list->directory_list), "unselect_row", GTK_SIGNAL_FUNC(file_unselect_event), file_list);
   gtk_paned_add1(GTK_PANED(paned), scrolled_window);
   gtk_clist_set_column_title(GTK_CLIST(file_list->directory_list), 0, "Directories");
   gtk_widget_show(file_list->directory_list);
   
   scrolled_window = gtk_scrolled_window_new(0, 0);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_set_usize(scrolled_window, 200, 250);
   gtk_widget_show(scrolled_window);

   file_list->file_list = gtk_ctree_new(1, 0);
   gtk_clist_set_selection_mode(GTK_CLIST(file_list->file_list), GTK_SELECTION_SINGLE);
   gtk_clist_set_shadow_type(GTK_CLIST(file_list->file_list), GTK_SHADOW_ETCHED_IN);
   gtk_clist_set_column_justification(GTK_CLIST(file_list->file_list), 0, GTK_JUSTIFY_LEFT);
   gtk_clist_set_row_height(GTK_CLIST(file_list->file_list), 18);
   gtk_clist_column_titles_show(GTK_CLIST(file_list->file_list));
   gtk_ctree_set_expander_style(GTK_CTREE(file_list->file_list), GTK_CTREE_EXPANDER_NONE);
   gtk_ctree_set_line_style (GTK_CTREE(file_list->file_list), GTK_CTREE_LINES_NONE);
   gtk_clist_column_titles_passive(GTK_CLIST(file_list->file_list));
   gtk_container_add(GTK_CONTAINER(scrolled_window), file_list->file_list);
   gtk_signal_connect(GTK_OBJECT(file_list->file_list), "key_press_event", GTK_SIGNAL_FUNC(gnome_filelist_key_press), 0);
   gtk_signal_connect(GTK_OBJECT(file_list->file_list), "select_row", GTK_SIGNAL_FUNC(file_select_event), file_list);
   gtk_signal_connect(GTK_OBJECT(file_list->file_list), "unselect_row", GTK_SIGNAL_FUNC(file_unselect_event), file_list);
   gtk_paned_add2(GTK_PANED(paned), scrolled_window);
   gtk_clist_set_column_title(GTK_CLIST(file_list->file_list), 0, "Files");
   gtk_widget_show(file_list->file_list);

   util_box = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(main_box), util_box, TRUE, TRUE, 0);
   gtk_widget_show(util_box);

   label = gtk_label_new("Selection: ");
   gtk_box_pack_start(GTK_BOX(util_box), label, FALSE, FALSE, 0);
   gtk_widget_show(label);
   
   file_list->selection_entry = gtk_entry_new();
   gtk_box_pack_start(GTK_BOX(util_box), file_list->selection_entry, TRUE, TRUE, 0);
   gtk_signal_connect(GTK_OBJECT(file_list->selection_entry), "changed", GTK_SIGNAL_FUNC(check_ok_button_cb), file_list);
   gtk_signal_connect(GTK_OBJECT(file_list->selection_entry), "activate", GTK_SIGNAL_FUNC(check_goto), file_list);
   gtk_widget_show(file_list->selection_entry);
   
   hsep = gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(main_box), hsep, TRUE, TRUE, 10);
   gtk_widget_show(hsep);

   util_box = gtk_hbutton_box_new();
   gtk_box_pack_start(GTK_BOX(main_box), util_box, FALSE, TRUE, 0);
   gtk_button_box_set_layout(GTK_BUTTON_BOX(util_box), gnome_preferences_get_button_layout());
   gtk_button_box_set_spacing(GTK_BUTTON_BOX(util_box), GNOME_PAD);
   gtk_widget_show(util_box);

   file_list->ok_button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
   gtk_box_pack_start(GTK_BOX(util_box), file_list->ok_button, FALSE, FALSE, 5);
   GTK_WIDGET_SET_FLAGS(file_list->ok_button, GTK_CAN_DEFAULT);
   gtk_widget_set_sensitive(file_list->ok_button, FALSE);
   gtk_widget_show(file_list->ok_button);

   file_list->cancel_button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
   gtk_box_pack_start(GTK_BOX(util_box), file_list->cancel_button, FALSE, FALSE, 5);
   GTK_WIDGET_SET_FLAGS(file_list->cancel_button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default(file_list->cancel_button);
   gtk_widget_show(file_list->cancel_button);   

   file_list->folder = (GnomePixmap *)gnome_pixmap_new_from_xpm_d(bfoldc_xpm);
   file_list->file = (GnomePixmap *)gnome_pixmap_new_from_xpm_d(file_file_xpm);
   
   if (!gnome_filelist_set_dir (file_list, path))
      gnome_filelist_set_dir (file_list, g_get_home_dir ());
   
   return GTK_WIDGET (file_list);
}

static void gnome_filelist_destroy(GtkObject *object)
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
   g_list_free (GNOME_FILELIST(file_list)->history);
   gtk_widget_destroy (file_list);
}

static void file_select_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeFileList *file_list)
{
   gchar *path;
   gchar *selected;
   GtkCTreeNode *node;
   GdkPixmap *pixmap;
   
   file_list->selected_row = row;   
   node = gtk_ctree_node_nth(tree, row);
   gtk_ctree_node_get_pixtext(GTK_CTREE(file_list->file_list), node, 0, &selected, NULL, &pixmap, NULL);
   
   g_free(file_list->selected);
   file_list->selected = g_new(char, strlen(selected)+1);
   strcpy(file_list->selected, selected);
   set_file_selection(file_list);
   
   if (pixmap == file_list->folder->pixmap)
   {
      if (event && event->type == GDK_2BUTTON_PRESS)
      {
         if (!strcmp(file_list->selected, "."))
            gnome_filelist_set_dir(file_list, file_list->path);
         else if (!strcmp(file_list->selected, ".."))
         {
            path = get_parent_dir(file_list->path); 
            g_free(file_list->path);
	    file_list->path = path;
            gnome_filelist_set_dir(file_list, file_list->path);
         }
	 else
	 {
	    path = build_full_path(file_list->path, file_list->selected);
	    gnome_filelist_set_dir(file_list, path);
	    g_free(path);
	 }
      }
      else if (event)
         gtk_clist_unselect_all(GTK_CLIST(file_list->file_list));
   }
   else
   {
      if (event && event->type == GDK_2BUTTON_PRESS)
      {
         set_file_selection(file_list);
         gtk_signal_emit_by_name(GTK_OBJECT(file_list->ok_button), "clicked", NULL);
      }
      else if (event)
         gtk_clist_unselect_all(GTK_CLIST(file_list->directory_list));
   }
   if (!event || event->type != GDK_2BUTTON_PRESS)
      file_list->history_position = -1;
}


static void file_unselect_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeFileList *file_list)
{
   file_list->selected_row = -1;   
   
   g_free(file_list->selected);
   file_list->selected = NULL;
   
   set_file_selection(file_list);
}

static void gnome_filelist_get_dirs(GnomeFileList *file_list)
{
   DIR *dir_file;
   struct dirent *dir;
   struct stat st;
   gchar filename[384];
   GtkCTreeNode *node;
   gchar *str;
   GList *dirs_list = NULL;
   GList *files_list = NULL;
   GList *temp;
      
   gtk_clist_clear(GTK_CLIST(file_list->directory_list));
   gtk_clist_clear(GTK_CLIST(file_list->file_list));
   gtk_clist_freeze(GTK_CLIST(file_list->directory_list));
   gtk_clist_freeze(GTK_CLIST(file_list->file_list));

   dir_file = opendir(file_list->path);
   if(dir_file != NULL)
   {
      while((dir = readdir(dir_file)) != NULL)
      {
         g_snprintf(filename, sizeof(filename), "%s/%s", file_list->path, dir->d_name);
   	 stat(filename, &st);
         str = g_new(char, strlen(dir->d_name)+1);
         strcpy(str, dir->d_name);
         
	 if(S_ISDIR(st.st_mode))
            dirs_list = g_list_prepend(dirs_list, (gpointer)str);
         else
            files_list = g_list_prepend(files_list, (gpointer)str);	 
      }
      closedir(dir_file);

      dirs_list = g_list_sort(dirs_list, (GCompareFunc)dir_compare_func);
      files_list = g_list_sort(files_list, (GCompareFunc)dir_compare_func);

      temp = dirs_list;
      while(temp != NULL)
      {
         str = (gchar *)temp->data;
         
	 node = gtk_ctree_insert_node(GTK_CTREE(file_list->directory_list), NULL, NULL, &str, 4, file_list->folder->pixmap, file_list->folder->mask, file_list->folder->pixmap, file_list->folder->mask, FALSE, FALSE);
         
	 g_free(str);
         temp = g_list_next(temp);
      }
      g_list_free(dirs_list);

      temp = files_list;
      while(temp != NULL)
      {
         str = (gchar *)temp->data;
         
	 node = gtk_ctree_insert_node(GTK_CTREE(file_list->file_list), NULL, NULL, &str, 4, file_list->file->pixmap, file_list->file->mask, file_list->file->pixmap, file_list->file->mask, FALSE, FALSE);
         
	 g_free(str);
         temp = g_list_next(temp);
      }
      g_list_free(files_list);
   }
   else
      g_print("The directory was not found!\n");
   
   gtk_clist_thaw(GTK_CLIST(file_list->directory_list));
   gtk_clist_thaw(GTK_CLIST(file_list->file_list));
}


static gint dir_compare_func(gchar *txt1, gchar *txt2)
{
   return(strcmp(txt1, txt2));
}

static void set_file_selection(GnomeFileList *file_list)
{
   gchar *temp;

/*
	Should not call getcwd(), because meantime
	the dir might have been changed by the user
	and it causes probem when cwd != file_list->path
	
	Using the already existing file_list->path is fine.
*/
   if(file_list->path)
   {
      gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry), file_list->path);
      
      temp = file_list->path + strlen(file_list->path);
      
      if(temp > file_list->path) temp--;
      if(strcmp(file_list->path, "/") && *temp != '/')
         gtk_entry_append_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry), "/");
   }
   
   if(file_list->selected)
      gtk_entry_set_text(GTK_ENTRY(file_list->selection_entry), file_list->selected);
   else
      gtk_entry_set_text(GTK_ENTRY(file_list->selection_entry), "");
}

gchar *gnome_filelist_get_filename(GnomeFileList *file_list)
{
   gchar *path = NULL;
   gchar *filename = NULL;
   gchar *full = NULL;
   
   g_return_val_if_fail(file_list != NULL, NULL);
   g_return_val_if_fail(GNOME_IS_FILELIST(file_list), NULL);
   
   path = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry));
   filename = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
   full = g_strconcat(path, filename, NULL);
   
   return(full);
}

gchar *gnome_filelist_get_path(GnomeFileList *file_list)
{
   g_return_val_if_fail(file_list != NULL, NULL);
   g_return_val_if_fail(GNOME_IS_FILELIST(file_list), NULL);
   
   return(file_list->path);   
}

gboolean gnome_filelist_set_filename (GnomeFileList *file_list, gchar *fname)
{
   gchar *dir;

   g_return_val_if_fail(fname != NULL, FALSE);
   g_return_val_if_fail(file_list != NULL, FALSE);
   g_return_val_if_fail(GNOME_IS_FILELIST(file_list), FALSE);
      
   dir = g_dirname (fname);
   if (dir)
   {
	   if(gnome_filelist_set_dir (file_list, dir))
	   {
		   g_free(file_list->selected);
		   file_list->selected = g_basename (fname);
		   set_file_selection(file_list);
		   g_free (dir);
		   
		   return TRUE;
	   }
	   else
	   {
		   g_free(file_list->selected);
		   file_list->selected = NULL;
		   g_free (dir);
		   
		   return FALSE;
	   }
   }
   else
   {
	   g_free(file_list->selected);
	   file_list->selected = NULL;
	   
	   return FALSE;
   }
}

gboolean gnome_filelist_set_dir(GnomeFileList *file_list, gchar path[])
{
   gchar *hist_path;
   gchar *path_bak;
   gint return_value;

   g_return_val_if_fail(path != NULL, FALSE);
   g_return_val_if_fail(file_list != NULL, FALSE);
   g_return_val_if_fail(GNOME_IS_FILELIST(file_list), FALSE);

   path_bak = g_get_current_dir ();
	  
   if((return_value = chdir(path))) {
	   g_free (path_bak);
	   
	   return(FALSE);
   }
   
   chdir (path_bak);
   g_free (path_bak);
   
   file_list->path = build_full_path(path, "");
   strcpy(file_list->path, path);
   gnome_filelist_get_dirs(file_list);
   
   g_free(file_list->selected);
   file_list->selected = NULL;

   if(!check_list_for_entry(file_list->history, file_list->path))
   {
      hist_path = g_new(char, strlen(file_list->path)+1);
      strcpy(hist_path, file_list->path);
      
      file_list->history = g_list_prepend(file_list->history, (gpointer)hist_path);
      gtk_combo_set_popdown_strings(GTK_COMBO(file_list->history_combo), file_list->history);
  }
   
   set_file_selection(file_list);
   gtk_widget_grab_focus(file_list->directory_list);
   file_list->selected_row = -1;

   return(TRUE);
}

void gnome_filelist_set_title(GnomeFileList *file_list, gchar *title)
{
   g_return_if_fail(file_list != NULL);
   g_return_if_fail(GNOME_IS_FILELIST(file_list));
   
   gtk_window_set_title(GTK_WINDOW(file_list), title);
}

static gboolean gnome_filelist_check_dir_exists(gchar *path)
{
   DIR *dir_file;
   
   if(!path) return(FALSE);
   
   dir_file = opendir(path);
   
   if(dir_file == NULL)
      return(FALSE);
   
   closedir(dir_file);
   
   return(TRUE);
}

static gboolean check_list_for_entry(GList *list, gchar *new_entry)
{
   GList *temp;
   gchar *cmp_text;
   
   if(list == NULL)
      return(FALSE);
   
   temp = list;
   while(temp != NULL)
   {
      cmp_text = temp->data;
      
      if(!strcmp(cmp_text, new_entry))
         return(TRUE);
      
      temp = g_list_next(temp);   
   }
   
   return(FALSE);
}

static gint gnome_filelist_key_press(GtkWidget *widget, GdkEventKey *event)
{
   GnomeFileList *file_list;
   GtkCList *clist;
   gint row;
   gint skip_rows = 0;
   gfloat left = 0.0;
   GtkCTreeNode *node;
   GdkPixmap *pixmap;
   gchar path[384]= "";
   gchar *util;
   gboolean del = FALSE;

   clist = GTK_CLIST(widget);
   file_list = (GnomeFileList *)widget->parent->parent->parent->parent;
   
   row = file_list->selected_row > -1 ? file_list->selected_row : 0;
   skip_rows = clist->clist_window_height / (clist->row_height + 1);
   
   left = (float)clist->clist_window_height / (float)(clist->row_height + 1);
   left -= skip_rows;
   
   if(left < .5) skip_rows--;
   if(event->keyval == GDK_Return && file_list->selected_row >= 0)
   {
      node = gtk_ctree_node_nth(GTK_CTREE(widget), file_list->selected_row);
      gtk_ctree_node_get_pixtext(GTK_CTREE(widget), node, 0, NULL, NULL, &pixmap, NULL);
      if(pixmap == file_list->folder->pixmap) //its a folder      
      {
         if(!strcmp(file_list->selected, "."))
         {
            gnome_filelist_set_dir(file_list, file_list->path);
            file_list->history_position = -1;
         }
         else if(!strcmp(file_list->selected, ".."))
         {
            if((util = strrchr(file_list->path, '/')))
            {
               if(util == file_list->path)
                  file_list->path[1] = '\0';
               else
                  *util = '\0';
  
	       gnome_filelist_set_dir(file_list, file_list->path);
               file_list->history_position = -1;
            }
         }
         else
         {
            strcat(path, file_list->path);
	    if(strcmp(file_list->path, "/"))
               strcat(path, "/");
            strcat(path, file_list->selected);
        
	    gnome_filelist_set_dir(file_list, path);
            file_list->history_position = -1;
         }
      }
      else
        gtk_signal_emit_by_name(GTK_OBJECT(file_list->ok_button), "clicked", NULL);
      
      return(TRUE);
   }
   else if(event->keyval == GDK_Up)
     row--;
   else if(event->keyval == GDK_Down)
     row++;
   else if(event->keyval == GDK_Page_Up)
     row -= skip_rows;
   else if(event->keyval == GDK_Page_Down)
     row += skip_rows;
   else if(event->keyval == GDK_Delete)
   {
      gtk_signal_emit_by_name(GTK_OBJECT(file_list->delete_button), "clicked", file_list);
      gtk_widget_grab_focus(GTK_WIDGET(clist));
      del = TRUE;
   }
   else if(event->keyval == 'R' || event->keyval == 'r')
   {
      gtk_signal_emit_by_name(GTK_OBJECT(file_list->rename_button), "clicked", file_list);
      gtk_widget_grab_focus(GTK_WIDGET(clist));
      del = TRUE;
   }
   if(row >= 0 && row < clist->rows)
   {
      gtk_clist_select_row(clist, row, 0);
      if(del) gtk_clist_moveto(clist, row, 0, 0.0, 0.0);
   }
   else if(row < 0 && clist->rows)
   {
      gtk_clist_select_row(clist, 0, 0);
      gtk_clist_moveto(clist, 0, 0, 0.0, 0.0);
   }
   else if(row > clist->rows)
   {
      gtk_clist_select_row(clist, clist->rows-1, 0);      
      gtk_clist_moveto(clist, clist->rows-1, 0, 1.0, 1.0);
   }
   return(TRUE);
}

static void refresh_listing(GtkWidget *widget, GnomeFileList *file_list)
{
   gchar *string;
   
   string = build_full_path(file_list->path, "");
   gnome_filelist_set_dir(file_list, string);
   
   g_free(string);
}

static void goto_directory(GtkWidget *widget, GnomeFileList *file_list)
{
   gchar *string, *util, *path, *selected, *new_string;
   
   path = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry));
   selected = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
   string = build_full_path(path, selected);
   util = strrchr(string, '/');
   
   if(util && !strcmp(util, "/."))
   {
      new_string = g_new(char, (util - string) + 1);
      strncpy(new_string, string, util - string);
      g_free(string);
      string = new_string;
   }
   else if(util && !strcmp(util, "/.."))
   {
      util = get_parent_dir(string);
      if(util)
      {
         g_free(string);
         string = util;
      }
   }
   
   gnome_filelist_set_dir(file_list, string);
   file_list->history_position = -1;
   g_free(string);
}

static void goto_parent(GtkWidget *widget, GnomeFileList *file_list)
{
   gchar *string, *parent;
   
   string = build_full_path(file_list->path, "");
   parent = get_parent_dir(string);
   
   gnome_filelist_set_dir(file_list, parent);
   file_list->history_position = -1;
   
   g_free(parent);
   g_free(string);
}

static void goto_last(GtkWidget *widget, GnomeFileList *file_list)
{
   gpointer temp = NULL;
   
   g_free(file_list->selected);
   file_list->selected = NULL;
   file_list->history_position++;
   
   if((temp = g_list_nth_data(file_list->history, file_list->history_position+1)))
      gnome_filelist_set_dir(file_list, (gchar *)temp);
}

static void check_ok_button_cb(GtkWidget *widget, GnomeFileList *file_list)
{
   gchar *path, *selected, *string;
   
   path = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry));
   selected = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
   string = build_full_path(path, selected);
   
   if(gnome_filelist_check_dir_exists(string))
   {
      gtk_widget_set_sensitive(file_list->ok_button, FALSE);
      gtk_widget_set_sensitive(file_list->forward_button, TRUE);
      gtk_widget_set_sensitive(file_list->delete_button, FALSE);
      gtk_widget_set_sensitive(file_list->rename_button, FALSE);
   }
   else
   {
      gtk_widget_set_sensitive(file_list->ok_button, TRUE);
      gtk_widget_set_sensitive(file_list->forward_button, FALSE);
      if(selected && strlen(selected) && check_if_file_exists(string) && check_can_modify(string))
      {
         gtk_widget_set_sensitive(file_list->delete_button, TRUE);
         gtk_widget_set_sensitive(file_list->rename_button, TRUE);
      }
      else
      {
         gtk_widget_set_sensitive(file_list->delete_button, FALSE);
         gtk_widget_set_sensitive(file_list->rename_button, FALSE);
      }
   }
   if(g_list_nth_data(file_list->history, file_list->history_position+2))
      gtk_widget_set_sensitive(file_list->back_button, TRUE);
   else
      gtk_widget_set_sensitive(file_list->back_button, FALSE);
   
   g_free(string);
}

gchar *get_parent_dir(gchar *path)
{
   gchar *tmp, *bak;
   gchar *return_str = NULL;
 
   bak = g_get_current_dir ();

   tmp = g_strdup_printf ("%s/..", path);
   g_free (tmp);
   
   return_str = g_get_current_dir ();

   chdir (bak);
   g_free (bak);
   
   return return_str;
}

gchar *build_full_path(const gchar *path, const gchar *selection)
{
   gchar *ptr = NULL;
   gchar chr;
   gint malloc_size = 0;
   gint offset = 0;
   
   offset =  strlen(path) - 1;
   chr = path[offset];
   
   malloc_size = strlen(path) + 1;
   malloc_size += chr == '/' && offset ? 0 : 1;
   malloc_size += selection ? strlen(selection) : 0;
   
   ptr = g_new(char, malloc_size);
   strcpy(ptr, path);
   
   if(chr != '/' && offset)
      strcat(ptr, "/");
   
   if(selection)
      strcat(ptr, selection);
   
   return(ptr);
}

static void check_goto(GtkWidget *widget, GnomeFileList *file_list)
{
   gchar *string, *path, *selected;
  
   path = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry));
   selected = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
   string = build_full_path(path, selected);

   if(!gnome_filelist_set_dir(file_list, string) && GTK_WIDGET_IS_SENSITIVE(file_list->ok_button))
      gtk_signal_emit_by_name(GTK_OBJECT(file_list->ok_button), "clicked", NULL);   
   else
      file_list->history_position = -1;
   
   g_free(string);
}

static void delete_file(GtkWidget *widget, GnomeFileList *file_list)
{
   GnomeDialog *dialog;
   GtkWidget *box;
   GtkWidget *pixmap;
   GtkWidget *label;
   gchar *path;
   gchar *selected;
   gchar *full;
   gchar *string;
   gchar *name;
   gint result;
   
   path = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry));
   selected = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
   full = build_full_path(path, selected);

   dialog = (GnomeDialog *)gnome_dialog_new("Delete File...", GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO, NULL);
   
   box = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(dialog->vbox), box, FALSE, FALSE, 0);
   gtk_widget_show(box);
   
   name = gnome_pixmap_file("gnome-question.png");
   if(name)
   {
      pixmap = gnome_pixmap_new_from_file(name);
      gtk_box_pack_start(GTK_BOX(box), pixmap, FALSE, FALSE, 10);
      gtk_widget_show(pixmap);
   }
   
   string = g_strconcat("Really delete \"", selected, "\" ?", NULL);
   label = gtk_label_new(string);
   gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 5);
   gtk_widget_show(label);
   
   gnome_dialog_set_default(dialog, 1);
   gnome_dialog_set_parent(dialog, GTK_WINDOW(file_list));
   
   result = gnome_dialog_run_and_close(dialog);
   if(!result)
   {
      remove(full);
      refresh_listing(NULL, file_list);
   }
   
   g_free(string);
   g_free(full);
}

static void rename_file_cb(GtkWidget *entry, GnomeFileList *file_list)
{
   gchar *text;
   
   text = gtk_entry_get_text(GTK_ENTRY(entry));
   file_list->entry_text = g_new0(char, strlen(text)+1);
   strcpy(file_list->entry_text, text);
}

static void rename_file(GtkWidget *widget, GnomeFileList *file_list)
{
   GnomeDialog *dialog;
   GtkWidget *hbox;
   GtkWidget *vbox;
   GtkWidget *pixmap;
   GtkWidget *label;
   GtkWidget *entry;
   gchar *path;
   gchar *selected;
   gchar *full;
   gchar *new_name;
   gchar *string;
   gchar *name;
   gint result;

   path = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(file_list->history_combo)->entry));
   selected = gtk_entry_get_text(GTK_ENTRY(file_list->selection_entry));
   full = build_full_path(path, selected);

   dialog = (GnomeDialog *)gnome_dialog_new("Delete File...", GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
   
   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(dialog->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);
   
   name = gnome_pixmap_file("gnome-question.png");
   if(name)
   {
      pixmap = gnome_pixmap_new_from_file(name);
      gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 10);
      gtk_widget_show(pixmap);
   }
   
   vbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
   gtk_widget_show(vbox);
   
   string = g_strconcat("Rename \"", selected, "\" to:", NULL);
   label = gtk_label_new(string);
   gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 5);
   gtk_widget_show(label);
   
   entry = gtk_entry_new();
   gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 5);
   gtk_entry_set_text(GTK_ENTRY(entry), selected);
   gtk_signal_connect(GTK_OBJECT(entry), "destroy", GTK_SIGNAL_FUNC(rename_file_cb), file_list);
   gtk_widget_show(entry);
   
   gnome_dialog_set_default(dialog, 1);
   gnome_dialog_set_parent(dialog, GTK_WINDOW(file_list));
   result = gnome_dialog_run_and_close(dialog);
   
   if(!result)
   {
      new_name = build_full_path(path, file_list->entry_text);
      
      g_print("%s - %s.\n", full, new_name);
      rename(full, new_name);
      refresh_listing(NULL, file_list);
      
      g_free(new_name);
   }
   
   g_free(file_list->entry_text);
   file_list->entry_text = NULL;
   g_free(string);
   g_free(full);
}

static void home_directory_cb (GtkButton * button, GnomeFileList *file_list)
{
	gchar *home;
	home = gnome_util_user_home ();
	gnome_filelist_set_dir (GNOME_FILELIST (file_list), home);
}

static gboolean check_if_file_exists(gchar *filename)
{
   struct stat st;
   gboolean file = FALSE;
   gboolean stated = FALSE;
   FILE *opened;
   
   if((opened = fopen(filename, "r")))
   {
      file = TRUE;
      fclose(opened);
   }
   
   stat(filename, &st);
   
   if(S_ISREG(st.st_mode))
      stated = TRUE;
   
   return(file && stated ? TRUE : FALSE);
}

static gboolean check_can_modify(gchar *filename)
{
   FILE *file;
   
   if(!(file = fopen(filename, "a")))
      return(FALSE);
   
   fclose(file);
   
   return(TRUE);
}

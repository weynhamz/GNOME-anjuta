#include "gnomedirlist.h"

#include "../pixmaps/bfoldc.xpm"
#include "../pixmaps/bfoldo.xpm"

/* function declarations */
static void gnome_dirlist_class_init(GnomeDirListClass *_class);
static void gnome_dirlist_init(GnomeDirList *dir_list);
GtkWidget *gnome_dirlist_new(void);
static void gnome_dirlist_destroy(GtkObject *object);
static void ctree_select_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeDirList *dir_list);
static void ctree_unselect_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeDirList *dir_list);
static void gnome_dirlist_fill_dir(GnomeDirList *dir_list, GtkCTreeNode *parent);
static void gnome_dirlist_set_dir(GnomeDirList *dir_list);
static gint gnome_dirlist_key_press(GtkWidget *widget, GdkEventKey *event);
/* end function declarations */

static GtkWindowClass *parent_class = NULL;

GtkType gnome_dirlist_get_type(void)
{
   static GtkType dirlist_type=0;
   if(!dirlist_type)
   {
      static const GtkTypeInfo dirlist_info = 
      {	
         "GnomeDirList",
         sizeof(GnomeDirList),
         sizeof(GnomeDirListClass),
         (GtkClassInitFunc) gnome_dirlist_class_init,
         (GtkObjectInitFunc) gnome_dirlist_init,
         NULL,
         NULL,
         (GtkClassInitFunc)NULL,
      };
      dirlist_type = gtk_type_unique(GTK_TYPE_WINDOW, &dirlist_info);
   }
   return(dirlist_type);
}

static void gnome_dirlist_class_init(GnomeDirListClass *_class)
{
   GtkObjectClass *object_class;
   object_class = (GtkObjectClass*)_class;
   parent_class = gtk_type_class(GTK_TYPE_WINDOW);
   object_class->destroy = gnome_dirlist_destroy;
}

static void gnome_dirlist_init(GnomeDirList *dir_list)
{
   dir_list->dirs = NULL;
   dir_list->entry = NULL; 
   dir_list->path = NULL;
}

GtkWidget* gnome_dirlist_new()
{
   GnomeDirList *dir_list;
   GtkWidget *main_box;
   GtkWidget *util_box;
   GtkWidget *scrolled_window;
   GtkWidget *label;
   GtkWidget *hsep;

   dir_list = gtk_type_new(GNOME_TYPE_DIRLIST);

   gtk_container_set_border_width(GTK_CONTAINER(dir_list), 5);
   main_box = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(dir_list), main_box);
   gtk_widget_show(main_box);

   util_box = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(main_box), util_box, TRUE, TRUE, 5);
   gtk_widget_show(util_box);

   scrolled_window = gtk_scrolled_window_new(0, 0);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_set_usize(scrolled_window, 200, 250);
   gtk_box_pack_start(GTK_BOX(util_box), scrolled_window, TRUE, TRUE, 0);
   gtk_widget_show(scrolled_window);

   dir_list->dirs = gtk_ctree_new(1, 0);
   gtk_clist_set_column_title(GTK_CLIST(dir_list->dirs), 0, "Directories");
   gtk_clist_set_selection_mode(GTK_CLIST(dir_list->dirs), GTK_SELECTION_SINGLE);
   gtk_clist_set_shadow_type(GTK_CLIST(dir_list->dirs), GTK_SHADOW_ETCHED_IN);
   gtk_clist_set_column_justification(GTK_CLIST(dir_list->dirs), 0, GTK_JUSTIFY_LEFT);
   gtk_clist_set_row_height(GTK_CLIST(dir_list->dirs), 16);
   gtk_clist_column_titles_show(GTK_CLIST(dir_list->dirs));
   gtk_ctree_set_expander_style(GTK_CTREE(dir_list->dirs), GTK_CTREE_EXPANDER_NONE);
   gtk_container_add(GTK_CONTAINER(scrolled_window), dir_list->dirs);
   gtk_signal_connect(GTK_OBJECT(dir_list->dirs), "key_press_event", GTK_SIGNAL_FUNC(gnome_dirlist_key_press), 0);
   gtk_signal_connect(GTK_OBJECT(dir_list->dirs), "select_row", GTK_SIGNAL_FUNC(ctree_select_event), dir_list);
   gtk_signal_connect(GTK_OBJECT(dir_list->dirs), "unselect_row", GTK_SIGNAL_FUNC(ctree_unselect_event), dir_list);
   gtk_widget_show(dir_list->dirs);

   util_box = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(main_box), util_box, TRUE, TRUE, 0);
   gtk_widget_show(util_box);

   label = gtk_label_new("Path:");
   gtk_box_pack_start(GTK_BOX(util_box), label, FALSE, FALSE, 5);   
   gtk_widget_show(label);
   dir_list->entry = gtk_entry_new();
   gtk_box_pack_start(GTK_BOX(util_box), dir_list->entry, FALSE, TRUE, 5);   
   gtk_widget_show(dir_list->entry);
   
   hsep = gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(main_box), hsep, TRUE, TRUE, 10);
   gtk_widget_show(hsep);

   util_box = gtk_hbutton_box_new();
   gtk_box_pack_start(GTK_BOX(main_box), util_box, TRUE, TRUE, 0);
   gtk_button_box_set_layout(GTK_BUTTON_BOX(util_box), gnome_preferences_get_button_layout());
   gtk_button_box_set_spacing(GTK_BUTTON_BOX(util_box), GNOME_PAD);
   gtk_widget_show(util_box);

   dir_list->ok_button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
   gtk_box_pack_start(GTK_BOX(util_box), dir_list->ok_button, FALSE, FALSE, 5);
   GTK_WIDGET_SET_FLAGS(dir_list->ok_button, GTK_CAN_DEFAULT);
   gtk_widget_show(dir_list->ok_button);   

   dir_list->cancel_button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
   gtk_box_pack_start(GTK_BOX(util_box), dir_list->cancel_button, FALSE, FALSE, 5);
   GTK_WIDGET_SET_FLAGS(dir_list->cancel_button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default(dir_list->cancel_button);
   gtk_widget_show(dir_list->cancel_button);   

   dir_list->folder_closed = (GnomePixmap *)gnome_pixmap_new_from_xpm_d(bfoldc_xpm);
   dir_list->folder_open = (GnomePixmap *)gnome_pixmap_new_from_xpm_d(bfoldo_xpm);
   gnome_dirlist_set_dir(dir_list);
   gnome_dirlist_fill_dir(dir_list, NULL);
   return GTK_WIDGET(dir_list);
}

static void gnome_dirlist_destroy(GtkObject *object)
{
   GtkWidget *dir_list;
   GtkCListRow *row;
   GList *row_list;
   g_return_if_fail(object != NULL);
   g_return_if_fail(GNOME_IS_DIRLIST(object));
   dir_list = GTK_WIDGET(object);
   row_list = GTK_CLIST(GNOME_DIRLIST(dir_list)->dirs)->row_list;
   while(row_list)
   {
      row = (GtkCListRow *)row_list->data;
      if(row->data) g_free(row->data);
      row_list = row_list->next;
   }
   gtk_widget_destroy(dir_list);
}

static void gnome_dirlist_fill_dir(GnomeDirList *dir_list, GtkCTreeNode *parent)
{
   DIR *dir_file;
   struct dirent *dir;
   struct stat st;
   gchar filename[384];
   gchar up_all[] = ".";
   gchar up_one[] = "..";
   GtkCTreeNode *node;
   gchar *directory;
   gchar *str;

   gtk_clist_freeze(GTK_CLIST(dir_list->dirs));
   directory = gtk_entry_get_text(GTK_ENTRY(dir_list->entry));
   dir_file = opendir(directory);
   if(dir_file != NULL)
   {
      while((dir = readdir(dir_file)) != NULL)
      {
         if(!strcmp(dir->d_name, up_one) || !strcmp(dir->d_name, up_all))
            continue;
         if(strcmp(directory, "/"))
            g_snprintf(filename, sizeof(filename), "%s/%s", directory, dir->d_name);
         else
            g_snprintf(filename, sizeof(filename), "/%s", dir->d_name);
         stat(filename, &st);
         if(S_ISDIR(st.st_mode))
         {
            str = dir->d_name;
            node = gtk_ctree_insert_node(GTK_CTREE(dir_list->dirs), parent, NULL, &str, 4, dir_list->folder_closed->pixmap, dir_list->folder_closed->mask, dir_list->folder_open->pixmap, dir_list->folder_open->mask, FALSE, FALSE);
            gtk_ctree_node_set_row_data(GTK_CTREE(dir_list->dirs), node, g_strdup(filename));
         }	    
      }
   }
   else
   {
      g_print("The directory was not found!\n");
   }
   gtk_clist_thaw(GTK_CLIST(dir_list->dirs));
}


static void ctree_select_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeDirList *dir_list)
{
   GtkCTreeNode *node;
   gchar *directory;
   dir_list->selected_row = row;
   node = gtk_ctree_node_nth(tree, row);
   directory = gtk_ctree_node_get_row_data(GTK_CTREE(tree), node);
   dir_list->path = directory;
   gnome_dirlist_set_dir(dir_list);
   if(event && event->type == GDK_2BUTTON_PRESS)
   {
      gnome_dirlist_fill_dir(dir_list, node);
      gtk_ctree_expand(GTK_CTREE(dir_list->dirs), node);
   }
}

static void ctree_unselect_event(GtkCTree *tree, gint row, gint col, GdkEvent *event, GnomeDirList *dir_list)
{
   dir_list->selected_row = -1;
   dir_list->path = NULL;
   gnome_dirlist_set_dir(dir_list);
}

static void gnome_dirlist_set_dir(GnomeDirList *dir_list)
{
   g_return_if_fail(dir_list != NULL);
   g_return_if_fail(GNOME_IS_DIRLIST(dir_list));
   if(dir_list->path)
      gtk_entry_set_text(GTK_ENTRY(dir_list->entry), dir_list->path);
   else
      gtk_entry_set_text(GTK_ENTRY(dir_list->entry), "/");
}

gchar *gnome_dirlist_get_dir(GnomeDirList *dir_list)
{
   g_return_val_if_fail(dir_list != NULL, NULL);
   g_return_val_if_fail(GNOME_IS_DIRLIST(dir_list), NULL);
   return(gtk_entry_get_text(GTK_ENTRY(dir_list->entry)));
}

static gint gnome_dirlist_key_press(GtkWidget *widget, GdkEventKey *event)
{
   GnomeDirList *dir_list;
   GtkCList *clist;
   GtkCTree *ctree;
   gint row = 0;
   gint skip_rows = 0;
   gfloat left = 0.0;
   GtkCTreeNode *node = NULL;
   gchar *directory;

   clist = GTK_CLIST(widget);
   ctree = GTK_CTREE(widget);
   dir_list = (GnomeDirList *)widget->parent->parent->parent->parent;
   row = dir_list->selected_row;
   node = gtk_ctree_node_nth(ctree, row);
   skip_rows = clist->clist_window_height / (clist->row_height + 1);
   left = (float)clist->clist_window_height / (float)(clist->row_height + 1);
   left -= skip_rows;
   if(left < .5) skip_rows--;
   if(event->keyval == GDK_Return && node)
   {
      directory = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), node);
      gtk_entry_set_text(GTK_ENTRY(dir_list->entry), directory);
      gnome_dirlist_fill_dir(dir_list, node);
      gtk_ctree_expand(ctree, node);
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
   if(row >= 0 && row < clist->rows)     
      gtk_clist_select_row(clist, row, 0);
   else if(row < 0 && clist->rows)
      gtk_clist_select_row(clist, 0, 0);   
   else if(row > clist->rows)
      gtk_clist_select_row(clist, clist->rows-1, 0);      
   return(TRUE);
}
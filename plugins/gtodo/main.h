#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

// #include <libgtodo/libgtodo.h>
#include <libgtodo.h>

#define ICON_FILE "anjuta-gtodo-plugin-48.png"

int  get_all_due_dates();
xmlNodePtr  get_id_node(gchar *category, gint id);
extern GtkWidget *window;
extern GtkListStore *list;
extern GtkTreeModelSort *sortmodel;

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else 
#define _(String) String
#endif

enum
{
	ID,
	PRIORITY,
	PRIOSTR,
	DONE,
	SUMMARY,
	COMMENT,
	END_DATE,
	EDITABLE,
	COLOR,
	CATEGORY,
	F_DATE, /* a formated date string */
	START_DATE,
	COMPLETED_DATE,
	DUE,
	N_COL
};

typedef struct{
	GtkWidget *item;
	gchar *date;
} catitems;

typedef struct{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *treeview;
	GtkWidget *statusbar;
	GtkWidget *toolbar;
	GtkListStore *list;
	GtkTreeModel *sortmodel;
	GtkWidget *tbdelbut, *tbaddbut, *tbeditbut;
	GtkWidget *tbeditlb;
	GtkWidget *option, *menu;
	catitems **mitems;
	GtkAccelGroup *accel_group;
	GtkItemFactory *item_factory;

} mwindow;

typedef struct
{
	gboolean size, place;
	gboolean ask_delete_category;
	gboolean auto_purge;
	gint purge_days;
	gboolean hl_due, hl_today, hl_indays;
	gchar *due_color;
	gchar *due_today_color;
	gchar *due_in_days_color;
	gint due_days;
	gboolean hide_done;
	gboolean hide_due;
	gboolean hide_nodate;
	gint sorttype;	
	gint sortorder;
}sets;


/*interface.c*/
extern mwindow mw;
extern sets settings;
extern GConfClient *client;
extern GTodoClient *cl;

void about_window(void);
GtkWidget *gui_create_todo_widget(void);
GtkWidget *gui_create_main_window(void);

void gui_add_todo_item(GtkWidget *useless, gpointer data, guint32 openid);

void gtodo_load_settings (void);
void gtodo_update_settings (void);
void gtodo_set_hide_done (gboolean hide_it);
void gtodo_set_hide_due (gboolean hide_it);
void gtodo_set_hide_nodate (gboolean hide_it);
gboolean gtodo_get_hide_done (void);
gboolean gtodo_get_hide_due (void);
gboolean gtodo_get_hide_nodate (void);
void gtodo_set_sorting_order (gboolean ascending_sort);
void gtodo_set_sorting_type (int sorting_type);

/* callback.c*/
void remove_todo_item(GtkWidget *fake, gboolean internall);
int tray_clicked(GtkWidget *image, GdkEventButton *event);
void read_categorys(void);
void load_category(void);
/* void save_category(); */
void category_changed(void);
extern int categorys;
/* void save_categorys(); */
/* void change_category(); */
/* void delete_category(); */
extern gulong shand;
/* void add_category(); */
int tree_clicked(GtkWidget *tree, GdkEventButton *event);
void list_toggled_done(GtkCellRendererToggle *cell, gchar *path_str);
extern int hidden;
void tray_destroyed(GtkObject *object, gpointer data);
void gui_preferences(void);
void purge_category(void);
int message_box(gchar *text,gchar *buttext, GtkMessageType type);
void category_manager(void);
extern gboolean tray;
gboolean mw_motion_cb (GtkWidget *tv, GdkEventMotion *event, gpointer null);
void mw_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n);
int  get_all_past_purge(void);
/* void update_settings(); */

/* preferences */
GtkWidget *preferences_widget(void);
void preferences_remove_signals(void);

// void preferences_cb_show_date(GtkWidget *chbox);
gint sort_function_test(GtkTreeModel *model,GtkTreeIter *a,GtkTreeIter *b,gpointer user_data);

void set_sorting_menu_item();
void windows_moved(GtkWidget *window);

int check_for_notification_event(void);
void pref_gconf_set_notifications(GConfClient *client);

void export_gui(void);
void open_playlist(void);
void create_playlist(void);
void export_backup_xml(void);

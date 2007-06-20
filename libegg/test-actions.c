#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <libegg/menu/egg-combo-action.h>
#include <libegg/menu/egg-recent-action.h>
//#include <libegg/menu/egg-markup.h>

#ifndef _
#  define _(String) (String)
#  define N_(String) (String)
#endif

static GtkActionGroup *action_group = NULL;
static GtkToolbar *toolbar = NULL;
static GtkUIManager *menu_merge = NULL;

static void
activate_action (GtkAction *action)
{
  const gchar *name;

  g_object_get (G_OBJECT (action), "name", &name, NULL);
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_print ("Action %s (type=%s) activated", name, typename);
}

static void
changed_action (GtkAction *action)
{
  const gchar *name;

  g_object_get (G_OBJECT (action), "name", &name, NULL);
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_print ("Action %s (type=%s) changed activated", name, typename);
}

/* convenience functions for declaring actions */
static GtkActionEntry entries[] = {
	{"ActionList", GTK_STOCK_JUMP_TO, "List", NULL, "List box",
	 G_CALLBACK (activate_action)}
};

static guint n_entries = G_N_ELEMENTS (entries);

/* XML description of the menus for the test app.  The parser understands
 * a subset of the Bonobo UI XML format, and uses GMarkup for parsing */
static const gchar *ui_info =
"<ui>\n"
"  <toolbar name=\"toolbar\">\n"
"    <toolitem name=\"recent\" action=\"ActionRecent\" />\n"
"    <toolitem name=\"button\" action=\"ActionList\" />\n"
"    <toolitem name=\"combo\" action=\"ActionCombo\" />\n"
"    <toolitem name=\"combo2\" action=\"ActionCombo\" />\n"
"    <toolitem name=\"recent2\" action=\"ActionRecent\" />\n"
"  </toolbar>\n"
"</ui>\n";

static void
on_add_merge_widget (GtkWidget *merge, GtkWidget *widget,
					 GtkWidget *ui_container)
{
  // g_message("got widget %s of type %s", name ? name : "(null)", type);
  gtk_container_add (GTK_CONTAINER (ui_container), widget);
  gtk_widget_show(widget);

  if (GTK_IS_TOOLBAR (widget)) {
    toolbar = GTK_TOOLBAR (widget);
    gtk_toolbar_set_show_arrow (toolbar, TRUE);
  }
}

enum {
	COL_PIX, COL_NAME, COL_LINE, N_COLS
};

static GtkTreeModel *
create_model ()
{
	GtkTreeIter iter, *parent;
	gchar *strs[] = {"string 1", "string 2", "string 3", NULL};
	GtkTreeStore *store;
	gchar **idx;

	parent = NULL;
	store = gtk_tree_store_new (N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
	idx = strs;
	while (*idx)
	{
		GdkPixbuf *pixbuf;
		pixbuf = gdk_pixbuf_new_from_file_at_size (PACKAGE_PIXMAPS_DIR"/anjuta.png",
												   16, 16, NULL);
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,
							COL_PIX, pixbuf,
							COL_NAME, *idx,
							COL_LINE, 23, -1);
		g_object_unref (pixbuf);
		//if (parent)
		//gtk_tree_iter_free (parent);
		//parent = gtk_tree_iter_copy (&iter);
		idx++;
	}
	return GTK_TREE_MODEL (store);
}


static EggRecentModel *
create_recent_model ()
{
	EggRecentModel *model;
	model =
		egg_recent_model_new (EGG_RECENT_MODEL_SORT_MRU);
	egg_recent_model_set_limit (model, 15);
	egg_recent_model_set_filter_groups (model,
										"anjuta", NULL);
	return EGG_RECENT_MODEL (model);
}

int
main (int argc, char **argv)
{
  GList *strs;
  GtkAction *action;
  GtkWidget *window;
  GtkWidget *box;
  GError *error = NULL;
  GtkTreeModel *model;
  EggRecentModel *rmodel;
	
  gtk_init (&argc, &argv);

  if (g_file_test("accels", G_FILE_TEST_IS_REGULAR))
    gtk_accel_map_load("accels");

  action_group = gtk_action_group_new ("TestActions");
  gtk_action_group_add_actions (action_group, entries, n_entries, NULL);
	action = g_object_new (EGG_TYPE_COMBO_ACTION,
						   "name", "ActionCombo",
						   "label", _("Search"),
						   "tooltip", _("Incremental search"),
						   "stock_id", GTK_STOCK_JUMP_TO,
							NULL);
	g_assert (EGG_IS_COMBO_ACTION (action));
	g_signal_connect (action, "activate",
					  G_CALLBACK (activate_action), NULL);
	gtk_action_group_add_action (action_group, action);

  model = create_model ();
  egg_combo_action_set_model (EGG_COMBO_ACTION (action), model);
  g_object_unref (model);
  /* recent action */
	action = g_object_new (EGG_TYPE_RECENT_ACTION,
						   "name", "ActionRecent",
						   "label", _("Open _Recent"),
						   "tooltip", _("Open recent files"),
						   "stock_id", NULL,
							NULL);
	g_assert (EGG_IS_RECENT_ACTION (action));
	//g_signal_connect (action, "activate",
	//				  G_CALLBACK (activate_action), NULL);
	gtk_action_group_add_action (action_group, action);
  rmodel = create_recent_model ();
  egg_recent_action_add_model (EGG_RECENT_ACTION (action), rmodel);
  egg_recent_action_add_model (EGG_RECENT_ACTION (action), rmodel);
  egg_recent_action_add_model (EGG_RECENT_ACTION (action), rmodel);
  g_object_unref (rmodel);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_show (window);
  gtk_window_set_title (GTK_WINDOW (window), "Action Test");
  //gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  // gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);

  menu_merge = gtk_ui_manager_new ();
  g_signal_connect (G_OBJECT (menu_merge), "add_widget",
					G_CALLBACK (on_add_merge_widget), box);
  gtk_ui_manager_insert_action_group (menu_merge, action_group, 0);
  gtk_ui_manager_add_ui_from_string (menu_merge, ui_info, strlen(ui_info), &error);
  if (error != NULL)
  {
      g_warning ("building menus failed: %s", error->message);
      g_error_free (error);
  }

  // g_object_unref(accel_group); /* window holds ref to accel group */

  gtk_main ();

  // g_object_unref (action_group);

  gtk_accel_map_save("accels");

  return 0;
}

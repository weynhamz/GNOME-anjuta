#include <gtk/gtk.h>
#include "../toolbar/eggtoolbar.h"
#include <libegg/menu/egg-action.h>
#include <libegg/menu/egg-action-group.h>
#include <libegg/menu/egg-toggle-action.h>
#include <libegg/menu/egg-markup.h>
#include <libegg/menu/egg-accel-dialog.h>

#ifndef _
#  define _(String) (String)
#  define N_(String) (String)
#endif

static EggActionGroup *action_group = NULL;
static EggToolbar *toolbar = NULL;
static EggMenuMerge *menu_merge = NULL;

static void
activate_action (EggAction *action)
{
  const gchar *name = action->name;
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_message ("Action %s (type=%s) activated", name, typename);
}

static void
toggle_action (EggAction *action)
{
  const gchar *name = action->name;
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_message ("Action %s (type=%s) activated (active=%d)", name, typename,
	     (gint) EGG_TOGGLE_ACTION (action)->active);
}

static void
toggle_cnp_actions (EggAction *action)
{
  gboolean sensitive;

  sensitive = EGG_TOGGLE_ACTION (action)->active;
  action = egg_action_group_get_action (action_group, "cut");
  g_object_set (action, "sensitive", sensitive, NULL);
  action = egg_action_group_get_action (action_group, "copy");
  g_object_set (action, "sensitive", sensitive, NULL);
  action = egg_action_group_get_action (action_group, "paste");
  g_object_set (action, "sensitive", sensitive, NULL);

  action = egg_action_group_get_action (action_group, "toggle-cnp");
  if (sensitive)
    g_object_set (action, "label", _("Disable Cut and paste ops"), NULL);
  else
    g_object_set (action, "label", _("Enable Cut and paste ops"), NULL);
}

static void
show_accel_dialog (EggAction *action)
{
  GtkWidget *dialog;

  dialog = egg_accel_dialog_new(menu_merge);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_object_destroy (GTK_OBJECT (dialog));
}

static void
toolbar_style (EggAction *action, gpointer user_data)
{
  GtkToolbarStyle style;

  g_return_if_fail (toolbar != NULL);
  style = GPOINTER_TO_INT (user_data);

  egg_toolbar_set_style (toolbar, style);
}

static void
toolbar_size (EggAction *action, gpointer user_data)
{
  GtkIconSize size;

  g_return_if_fail (toolbar != NULL);
  size = GPOINTER_TO_INT (user_data);

  //egg_toolbar_set_icon_size (toolbar, size);
}

/* convenience functions for declaring actions */
static EggActionGroupEntry entries[] = {
  { "menu1", N_("Menu _1"), NULL, NULL, NULL, NULL, NULL},
  { "menu2", N_("Menu _2"), NULL, NULL, NULL, NULL, NULL},
  { "cut", N_("C_ut"), GTK_STOCK_CUT, "<control>X",
    N_("Cut the selected text to the clipboard"),
    G_CALLBACK (activate_action), NULL },
  { "copy", N_("_Copy"), GTK_STOCK_COPY, "<control>C",
    N_("Copy the selected text to the clipboard"),
    G_CALLBACK (activate_action), NULL },
  { "paste", N_("_Paste"), GTK_STOCK_PASTE, "<control>V",
    N_("Paste the text from the clipboard"),
    G_CALLBACK (activate_action), NULL },
  { "bold", N_("_Bold"), GTK_STOCK_BOLD, "<control>B",
    N_("Change to bold face"),
    G_CALLBACK (toggle_action), NULL, TOGGLE_ACTION },

  { "justify-left", N_("_Left"), GTK_STOCK_JUSTIFY_LEFT, "<control>L",
    N_("Left justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION },
  { "justify-center", N_("C_enter"), GTK_STOCK_JUSTIFY_CENTER, "<control>E",
    N_("Center justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION, "justify-left" },
  { "justify-right", N_("_Right"), GTK_STOCK_JUSTIFY_RIGHT, "<control>R",
    N_("Right justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION, "justify-left" },
  { "justify-fill", N_("_Fill"), GTK_STOCK_JUSTIFY_FILL, "<control>J",
    N_("Fill justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION, "justify-left" },
  { "quit", NULL, GTK_STOCK_QUIT, "<control>Q",
    N_("Quit the application"),
    G_CALLBACK (gtk_main_quit), NULL },
  { "toggle-cnp", N_("Enable Cut/Copy/Paste"), NULL, NULL,
    N_("Change the sensitivity of the cut, copy and paste actions"),
    G_CALLBACK (toggle_cnp_actions), NULL, TOGGLE_ACTION },
  { "customise-accels", N_("Customise _Accels"), GTK_STOCK_PREFERENCES, NULL,
    N_("Customise keyboard shortcuts"),
    G_CALLBACK (show_accel_dialog), NULL },
  { "toolbar-icons", N_("Icons"), NULL, NULL,
    NULL, G_CALLBACK (toolbar_style), GINT_TO_POINTER (GTK_TOOLBAR_ICONS),
    RADIO_ACTION, NULL },
  { "toolbar-text", N_("Text"), NULL, NULL,
    NULL, G_CALLBACK (toolbar_style), GINT_TO_POINTER (GTK_TOOLBAR_TEXT),
    RADIO_ACTION, "toolbar-icons" },
  { "toolbar-both", N_("Both"), NULL, NULL,
    NULL, G_CALLBACK (toolbar_style), GINT_TO_POINTER (GTK_TOOLBAR_BOTH),
    RADIO_ACTION, "toolbar-icons" },
  { "toolbar-both-horiz", N_("Both Horizontal"), NULL, NULL,
    NULL, G_CALLBACK (toolbar_style), GINT_TO_POINTER(GTK_TOOLBAR_BOTH_HORIZ),
    RADIO_ACTION, "toolbar-icons" },
  { "toolbar-small-icons", N_("Small Icons"), NULL, NULL,
    NULL,
    G_CALLBACK (toolbar_size), GINT_TO_POINTER (GTK_ICON_SIZE_SMALL_TOOLBAR) },
  { "toolbar-large-icons", N_("Large Icons"), NULL, NULL,
    NULL,
    G_CALLBACK (toolbar_size), GINT_TO_POINTER (GTK_ICON_SIZE_LARGE_TOOLBAR) },
};
static guint n_entries = G_N_ELEMENTS (entries);

/* XML description of the menus for the test app.  The parser understands
 * a subset of the Bonobo UI XML format, and uses GMarkup for parsing */
static const gchar *ui_info =
"<Root>\n"
"  <menu>\n"
"    <submenu name=\"menu1\" verb=\"menu1\">\n"
"      <menuitem name=\"cut\" verb=\"cut\" />\n"
"      <menuitem name=\"copy\" verb=\"copy\" />\n"
"      <menuitem name=\"paste\" verb=\"paste\" />\n"
"      <separator />\n"
"      <menuitem name=\"bold1\" verb=\"bold\" />\n"
"      <menuitem name=\"bold2\" verb=\"bold\" />\n"
"      <separator />\n"
"      <menuitem name=\"toggle-cnp\" verb=\"toggle-cnp\" />\n"
"      <separator />\n"
"      <menuitem name=\"quit\" verb=\"quit\" />\n"
"    </submenu>\n"
"    <submenu name=\"menu2\" verb=\"menu2\">\n"
"      <menuitem name=\"cut\" verb=\"cut\" />\n"
"      <menuitem name=\"copy\" verb=\"copy\" />\n"
"      <menuitem name=\"paste\" verb=\"paste\" />\n"
"      <separator />\n"
"      <menuitem name=\"bold\" verb=\"bold\" />\n"
"      <separator />\n"
"      <menuitem name=\"left\" verb=\"justify-left\" />\n"
"      <menuitem name=\"center\" verb=\"justify-center\" />\n"
"      <menuitem name=\"right\" verb=\"justify-right\" />\n"
"      <menuitem name=\"fill\" verb=\"justify-fill\" />\n"
"      <separator />\n"
"      <menuitem name=\"customise\" verb=\"customise-accels\" />\n"
"      <separator />\n"
"      <menuitem name=\"icons\" verb=\"toolbar-icons\" />\n"
"      <menuitem name=\"icons\" verb=\"toolbar-text\" />\n"
"      <menuitem name=\"both\" verb=\"toolbar-both\" />\n"
"      <menuitem name=\"horiz\" verb=\"toolbar-both-horiz\" />\n"
"      <separator />\n"
"      <menuitem name=\"small\" verb=\"toolbar-small-icons\" />\n"
"      <menuitem name=\"large\" verb=\"toolbar-large-icons\" />\n"
"    </submenu>\n"
"  </menu>\n"
"  <dockitem name=\"toolbar\">\n"
"    <toolitem name=\"cut\" verb=\"cut\" />\n"
"    <toolitem name=\"copy\" verb=\"copy\" />\n"
"    <toolitem name=\"paste\" verb=\"paste\" />\n"
"    <separator />\n"
"    <toolitem name=\"bold\" verb=\"bold\" />\n"
"    <separator />\n"
"    <toolitem name=\"justify-left\" verb=\"justify-left\" />\n"
"    <toolitem name=\"justify-center\" verb=\"justify-center\" />\n"
"    <toolitem name=\"justify-right\" verb=\"justify-right\" />\n"
"    <toolitem name=\"justify-fill\" verb=\"justify-fill\" />\n"
"    <separator />\n"
"    <toolitem name=\"quit\" verb=\"quit\" />\n"
"    <toolitem name=\"customise\" verb=\"customise-accels\" />\n"
"  </dockitem>\n"
"</Root>\n";

static void
on_add_merge_widget (GtkWidget *merge, GtkWidget *widget,
					 GtkWidget *ui_container)
{
  // g_message("got widget %s of type %s", name ? name : "(null)", type);
  gtk_container_add (GTK_CONTAINER (ui_container), widget);
  gtk_widget_show(widget);

  if (EGG_IS_TOOLBAR (widget)) {
    toolbar = EGG_TOOLBAR (widget);
    egg_toolbar_set_show_arrow (toolbar, TRUE);
  }
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *box;
  // GtkAccelGroup *accel_group;
  GError *error = NULL;
	
  gtk_init (&argc, &argv);

  if (g_file_test("accels", G_FILE_TEST_IS_REGULAR))
    gtk_accel_map_load("accels");

  action_group = egg_action_group_new ("TestActions");
  egg_action_group_add_actions (action_group, entries, n_entries);
  egg_toggle_action_set_active (EGG_TOGGLE_ACTION (egg_action_group_get_action (action_group, "toggle-cnp")), TRUE);
  // accel_group = gtk_accel_group_new();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_show (window);
  gtk_window_set_title (GTK_WINDOW (window), "Action Test");
  //gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  // gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);

  menu_merge = egg_menu_merge_new ();
  g_signal_connect (G_OBJECT (menu_merge), "add_widget",
					G_CALLBACK (on_add_merge_widget), box);
  egg_menu_merge_insert_action_group (menu_merge, action_group, 0);
  egg_menu_merge_add_ui_from_string (menu_merge, ui_info, strlen(ui_info), &error);
  if (error != NULL)
  {
      g_message ("building menus failed: %s", error->message);
      g_error_free (error);
  }

  // g_object_unref(accel_group); /* window holds ref to accel group */

  gtk_main ();

  // g_object_unref (action_group);

  gtk_accel_map_save("accels");

  return 0;
}

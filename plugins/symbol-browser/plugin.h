
#ifndef _FILE_MANAGER_PLUGIN_H_
#define _FILE_MANAGER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _SymbolBrowserPlugin SymbolBrowserPlugin;
typedef struct _SymbolBrowserPluginClass SymbolBrowserPluginClass;

struct _SymbolBrowserPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	
	GtkWidget *sw;             /* symbol main window [gtk_notebook] */
	GtkWidget *sv;             /* symbol view scrolledwindow */
	GtkWidget *sv_tree;        /* anjuta_symbol_view */
	GtkWidget *sv_tab_label;
	GtkWidget *ss;             /* symbol search */
	GtkWidget *ss_tab_label;
	GtkWidget *pref_tree_view; /* Preferences treeview */
	
	GtkActionGroup *action_group;
	GtkActionGroup *action_group_nav;
	gint merge_id;
	gchar *project_root_uri;
	GObject *current_editor;
	guint root_watch_id;
	guint editor_watch_id;
	GHashTable *editor_connected;
	GList *gconf_notify_ids;
};

struct _SymbolBrowserPluginClass{
	AnjutaPluginClass parent_class;
};

#endif

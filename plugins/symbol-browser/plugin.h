
#ifndef _FILE_MANAGER_PLUGIN_H_
#define _FILE_MANAGER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _SymbolBrowserPlugin SymbolBrowserPlugin;
typedef struct _SymbolBrowserPluginClass SymbolBrowserPluginClass;

struct _SymbolBrowserPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	
	GtkWidget *sw;
	GtkWidget *sv;
	GtkActionGroup *action_group;
	GtkActionGroup *action_group_nav;
	gint merge_id;
	gchar *top_dir;
	GObject *current_editor;
	guint root_watch_id;
	guint editor_watch_id;
};

struct _SymbolBrowserPluginClass{
	AnjutaPluginClass parent_class;
};

#endif

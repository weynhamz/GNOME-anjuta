
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
	gint merge_id;
	gchar *top_dir;
	guint root_watch_id;
};

struct _SymbolBrowserPluginClass{
	AnjutaPluginClass parent_class;
};

#endif


#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

#include "style-editor.h"

typedef struct _EditorPlugin EditorPlugin;
typedef struct _EditorPluginClass EditorPluginClass;

struct _EditorPlugin{
	AnjutaPlugin parent;
	GtkWidget *docman;
	AnjutaPreferences *prefs;
	AnjutaUI *ui;
	gint uiid;
	GList *action_groups;
	
	/*! state flag for Ctrl-TAB */
	gboolean g_tabbing;
	
	StyleEditor *style_editor;
};

struct _EditorPluginClass{
	AnjutaPluginClass parent_class;
};


#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

#include "indent-util.h"
#include "indent-dialog.h"

typedef struct _DocmanPlugin DocmanPlugin;
typedef struct _DocmanPluginClass DocmanPluginClass;

struct _DocmanPlugin{
	AnjutaPlugin parent;
	GtkWidget *docman;
	AnjutaPreferences *prefs;
	AnjutaUI *ui;
	gint uiid;
	GList *action_groups;
	
	/*! state flag for Ctrl-TAB */
	gboolean g_tabbing;
	
	IndentData *idt;
	
	/* Autosave timer ID */
	gint autosave_id;
	gboolean autosave_on;

	/* Timer interval in mins */
	gint autosave_it;
	
	GList *gconf_notify_ids;
	
	/* Support plugins */
	GList *support_plugins;
};

struct _DocmanPluginClass{
	AnjutaPluginClass parent_class;
};

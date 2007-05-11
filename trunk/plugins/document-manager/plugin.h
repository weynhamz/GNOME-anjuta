#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

#include "indent-util.h"
#include "indent-dialog.h"

extern GType docman_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_DOCMAN         (docman_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_DOCMAN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_DOCMAN, DocmanPlugin))
#define ANJUTA_PLUGIN_DOCMAN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_DOCMAN, DocmanPluginClass))
#define ANJUTA_IS_PLUGIN_DOCMAN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_DOCMAN))
#define ANJUTA_IS_PLUGIN_DOCMAN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_DOCMAN))
#define ANJUTA_PLUGIN_DOCMAN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_DOCMAN, DocmanPluginClass))

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

#endif

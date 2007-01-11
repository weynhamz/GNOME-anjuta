
#ifndef _FILE_MANAGER_PLUGIN_H_
#define _FILE_MANAGER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>

extern GType file_manager_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_FILE_MANAGER         (file_manager_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_FILE_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_FILE_MANAGER, FileManagerPlugin))
#define ANJUTA_PLUGIN_FILE_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_FILE_MANAGER, FileManagerPluginClass))
#define ANJUTA_IS_PLUGIN_FILE_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_FILE_MANAGER))
#define ANJUTA_IS_PLUGIN_FILE_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_FILE_MANAGER))
#define ANJUTA_PLUGIN_FILE_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_FILE_MANAGER, FileManagerPluginClass))

typedef struct _FileManagerPlugin FileManagerPlugin;
typedef struct _FileManagerPluginClass FileManagerPluginClass;

struct _FileManagerPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	GtkWidget *scrolledwindow;
	GtkWidget *tree;
	GtkActionGroup *action_group;
	gint merge_id;
	gchar *top_dir;
	guint root_watch_id;
	
	GList *gconf_notify_ids;
	gboolean project_is_loaded;
	
	GdkRectangle tooltip_rect;
	GtkWidget *tooltip_window;
	gulong tooltip_timeout;
	PangoLayout *tooltip_layout;
	
	/* Idle node expansion context */
	GList *nodes_to_expand;
	gint idle_id;
};

struct _FileManagerPluginClass{
	AnjutaPluginClass parent_class;
};

void fv_init (FileManagerPlugin *fv);
void fv_finalize (FileManagerPlugin *fv);

void   fv_set_root (FileManagerPlugin *fv, const gchar *root_dir);
void   fv_clear    (FileManagerPlugin *fv);
GList* fv_get_node_expansion_states (FileManagerPlugin *fv);
void   fv_set_node_expansion_states (FileManagerPlugin *fv,
									 GList *expansion_states);
gchar* fv_get_selected_file_path (FileManagerPlugin *fv);

void   fv_refresh (FileManagerPlugin *fv, gboolean save_states);
void   fv_cancel_node_expansion (FileManagerPlugin *fv);

#endif

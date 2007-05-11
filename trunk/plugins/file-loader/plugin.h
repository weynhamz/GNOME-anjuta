
#include <libanjuta/anjuta-plugin.h>
#include <libegg/recent-files/egg-recent-model.h>

extern GType anjuta_file_loader_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_FILE_LOADER         (anjuta_file_loader_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_FILE_LOADER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_FILE_LOADER, AnjutaFileLoaderPlugin))
#define ANJUTA_PLUGIN_FILE_LOADER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_FILE_LOADER, AnjutaFileLoaderPluginClass))
#define ANJUTA_IS_PLUGIN_FILE_LOADER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_FILE_LOADER))
#define ANJUTA_IS_PLUGIN_FILE_LOADER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_FILE_LOADER))
#define ANJUTA_PLUGIN_FILE_LOADER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_FILE_LOADER, AnjutaFileLoaderPluginClass))

typedef struct _AnjutaFileLoaderPlugin AnjutaFileLoaderPlugin;
typedef struct _AnjutaFileLoaderPluginClass AnjutaFileLoaderPluginClass;

struct _AnjutaFileLoaderPlugin{
	AnjutaPlugin parent;
	EggRecentModel *recent_files_model_top;
	EggRecentModel *recent_files_model_bottom;
	GtkActionGroup *action_group;
	GtkActionGroup *popup_action_group;
	GtkActionGroup *recent_group;
	
	gchar *fm_current_uri;
	gchar *pm_current_uri;
	
	gint fm_watch_id;
	gint pm_watch_id;
	
	gint uiid;
};

struct _AnjutaFileLoaderPluginClass{
	AnjutaPluginClass parent_class;
};

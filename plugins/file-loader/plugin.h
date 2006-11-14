
#include <libanjuta/anjuta-plugin.h>
#include <libegg/recent-files/egg-recent-model.h>

extern GType anjuta_file_loader_plugin_type;
#define ANJUTA_FILE_LOADER_PLUGIN_TYPE (anjuta_file_loader_plugin_type)
#define ANJUTA_FILE_LOADER_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_FILE_LOADER_PLUGIN_TYPE, AnjutaFileLoaderPlugin))

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

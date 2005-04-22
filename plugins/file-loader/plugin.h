
#include <libanjuta/anjuta-plugin.h>
#include <libegg/recent-files/egg-recent-model.h>

typedef struct _AnjutaFileLoaderPlugin AnjutaFileLoaderPlugin;
typedef struct _AnjutaFileLoaderPluginClass AnjutaFileLoaderPluginClass;

struct _AnjutaFileLoaderPlugin{
	AnjutaPlugin parent;
	EggRecentModel *recent_files_model_top;
	EggRecentModel *recent_files_model_bottom;
	GtkActionGroup *action_group;
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

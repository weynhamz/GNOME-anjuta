
#include <libanjuta/anjuta-plugin.h>
#include <libegg/recent-files/egg-recent-model.h>

typedef struct _AnjutaFileLoaderPlugin AnjutaFileLoaderPlugin;
typedef struct _AnjutaFileLoaderPluginClass AnjutaFileLoaderPluginClass;

struct _AnjutaFileLoaderPlugin{
	AnjutaPlugin parent;
	EggRecentModel *recent_files_model;
	GtkActionGroup *action_group;
	gchar *fm_current_uri;
	gint fm_watch_id;
	gint uiid;
};

struct _AnjutaFileLoaderPluginClass{
	AnjutaPluginClass parent_class;
};

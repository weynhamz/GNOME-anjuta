
#include <libanjuta/anjuta-plugin.h>

typedef struct _DefaultProfilePlugin DefaultProfilePlugin;
typedef struct _DefaultProfilePluginClass DefaultProfilePluginClass;

struct _DefaultProfilePlugin{
	AnjutaPlugin parent;
	gchar *project_uri;
	gint merge_id;
	GtkActionGroup *action_group;
	
	GHashTable *default_plugins;
	GQueue *loaded_plugins;
	
	/* A flag to indicate that session ops is by this plugin */
	gboolean session_by_me;
};

struct _DefaultProfilePluginClass{
	AnjutaPluginClass parent_class;
};


#include <libanjuta/anjuta-plugin.h>

extern GType gtodo_plugin_type;
#define GTODO_PLUGIN_TYPE (gtodo_plugin_type)
#define GTODO_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GTODO_PLUGIN_TYPE, GTodoPlugin))

typedef struct _GTodoPlugin GTodoPlugin;
typedef struct _GTodoPluginClass GTodoPluginClass;

struct _GTodoPlugin{
	AnjutaPlugin parent;
	GtkWidget *widget;
	GtkWidget *prefs;
	gint uiid;
	gint root_watch_id;
	GtkActionGroup *action_group;
	GtkActionGroup *action_group2;
};

struct _GTodoPluginClass{
	AnjutaPluginClass parent_class;
};

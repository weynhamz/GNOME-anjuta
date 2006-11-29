
#include <libanjuta/anjuta-plugin.h>

extern GType gtodo_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_GTODO         (gtodo_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_GTODO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_GTODO, GTodoPlugin))
#define ANJUTA_PLUGIN_GTODO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_GTODO, GTodoPluginClass))
#define ANJUTA_IS_PLUGIN_GTODO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_GTODO))
#define ANJUTA_IS_PLUGIN_GTODO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_GTODO))
#define ANJUTA_PLUGIN_GTODO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_GTODO, GTodoPluginClass))

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

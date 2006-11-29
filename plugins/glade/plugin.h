
#include <libanjuta/anjuta-plugin.h>

extern GType glade_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_GLADE         (glade_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_GLADE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_GLADE, GladePlugin))
#define ANJUTA_PLUGIN_GLADE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_GLADE, GladePluginClass))
#define ANJUTA_IS_PLUGIN_GLADE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_GLADE))
#define ANJUTA_IS_PLUGIN_GLADE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_GLADE))
#define ANJUTA_PLUGIN_GLADE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_GLADE, GladePluginClass))

typedef struct _GladePlugin GladePlugin;
typedef struct _GladePluginPriv GladePluginPriv;
typedef struct _GladePluginClass GladePluginClass;

struct _GladePlugin{
	AnjutaPlugin parent;
	GladePluginPriv *priv;
};

struct _GladePluginClass{
	AnjutaPluginClass parent_class;
};

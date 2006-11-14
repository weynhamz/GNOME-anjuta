
#include <libanjuta/anjuta-plugin.h>

extern GType glade_plugin_type;
#define GLADE_PLUGIN_TYPE (glade_plugin_type)
#define GLADE_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GLADE_PLUGIN_TYPE, GladePlugin))

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


#include <libanjuta/anjuta-plugin.h>

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

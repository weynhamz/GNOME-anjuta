
#include <libanjuta/anjuta-plugin.h>

typedef struct _DevhelpPlugin DevhelpPlugin;
typedef struct _DevhelpPluginPriv DevhelpPluginPriv;
typedef struct _DevhelpPluginClass DevhelpPluginClass;

struct _DevhelpPlugin{
	AnjutaPlugin parent;
	DevhelpPluginPriv *priv;
	gint uiid;
};

struct _DevhelpPluginClass{
	AnjutaPluginClass parent_class;
};

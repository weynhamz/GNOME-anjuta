
#include <libanjuta/anjuta-plugin.h>

typedef struct _GTodoPlugin GTodoPlugin;
typedef struct _GTodoPluginClass GTodoPluginClass;

struct _GTodoPlugin{
	AnjutaPlugin parent;
	GtkWidget *widget;
	GtkWidget *prefs;
	gint uiid;
};

struct _GTodoPluginClass{
	AnjutaPluginClass parent_class;
};


#include <libanjuta/anjuta-plugin.h>

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

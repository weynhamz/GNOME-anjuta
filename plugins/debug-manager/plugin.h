#include <libanjuta/anjuta-plugin.h>

#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-debug-manager.glade"

typedef struct _DebugManagerPlugin DebugManagerPlugin;
typedef struct _DebugManagerPluginClass DebugManagerPluginClass;

struct _DebugManagerPlugin
{
	AnjutaPlugin parent;
	gint uiid;

	gchar *uri;
	gboolean is_executable;
};

struct _DebugManagerPluginClass
{
	AnjutaPluginClass parent_class;
};

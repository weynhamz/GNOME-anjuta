#include <libanjuta/anjuta-plugin.h>

#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-debugger.glade"

typedef struct _DebuggerPlugin DebuggerPlugin;
typedef struct _DebuggerPluginClass DebuggerPluginClass;

struct _DebuggerPlugin
{
	AnjutaPlugin parent;
	gint uiid;
	gchar *uri;

};

struct _DebuggerPluginClass
{
	AnjutaPluginClass parent_class;
};

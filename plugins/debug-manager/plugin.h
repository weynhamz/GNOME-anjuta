#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-debug-manager.glade"

typedef struct _DebugManagerPlugin DebugManagerPlugin;
typedef struct _DebugManagerPluginClass DebugManagerPluginClass;

struct _DebugManagerPlugin
{
	AnjutaPlugin parent;
	IAnjutaDebugger *debugger;
	gint uiid;
	GtkActionGroup *action_group;
	
	gchar *project_root_uri;
	GObject *current_editor;
	
	guint editor_watch_id;
	guint project_watch_id;
};

struct _DebugManagerPluginClass
{
	AnjutaPluginClass parent_class;
};

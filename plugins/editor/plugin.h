
#include <libanjuta/anjuta-plugin.h>

typedef struct _EditorPlugin EditorPlugin;
typedef struct _EditorPluginClass EditorPluginClass;

struct _EditorPlugin{
	AnjutaPlugin parent;
	GtkWidget *docman;
	gint uiid;
};

struct _EditorPluginClass{
	AnjutaPluginClass parent_class;
};


#include <libanjuta/anjuta-plugin.h>

typedef struct _SamplePlugin SamplePlugin;
typedef struct _SamplePluginClass SamplePluginClass;

struct _SamplePlugin{
	AnjutaPlugin parent;
	GtkWidget *widget;
	gint uiid;
};

struct _SamplePluginClass{
	AnjutaPluginClass parent_class;
};

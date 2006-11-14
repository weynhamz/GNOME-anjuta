
#include <libanjuta/anjuta-plugin.h>

extern GType anjuta_sample_plugin_type;
#define ANJUTA_SAMPLE_PLUGIN_TYPE (anjuta_sample_plugin_type)
#define ANJUTA_SAMPLE_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_SAMPLE_PLUGIN_TYPE, SamplePlugin))

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


#include <libanjuta/anjuta-plugin.h>

extern GType anjuta_sample_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_SAMPLE         (anjuta_sample_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_SAMPLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_SAMPLE, SamplePlugin))
#define ANJUTA_PLUGIN_SAMPLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_SAMPLE, SamplePluginClass))
#define ANJUTA_IS_PLUGIN_SAMPLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_SAMPLE))
#define ANJUTA_IS_PLUGIN_SAMPLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_SAMPLE))
#define ANJUTA_PLUGIN_SAMPLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_SAMPLE, SamplePluginClass))

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

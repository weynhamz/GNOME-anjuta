#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <libanjuta/anjuta-plugin.h>

extern GType starter_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_STARTER         (starter_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_STARTER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_STARTER, StarterPlugin))
#define ANJUTA_PLUGIN_STARTER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_STARTER, StarterPluginClass))
#define ANJUTA_IS_PLUGIN_STARTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_STARTER))
#define ANJUTA_IS_PLUGIN_STARTER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_STARTER))
#define ANJUTA_PLUGIN_STARTER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_STARTER, StarterPluginClass))

typedef struct _StarterPlugin StarterPlugin;
typedef struct _StarterPluginClass StarterPluginClass;

struct _StarterPlugin{
	AnjutaPlugin parent;
	GtkWidget *starter;
};

struct _StarterPluginClass{
	AnjutaPluginClass parent_class;
};

#endif

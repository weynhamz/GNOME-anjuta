
#include <libanjuta/anjuta-plugin.h>

typedef struct _DefaultProfilePlugin DefaultProfilePlugin;
typedef struct _DefaultProfilePluginClass DefaultProfilePluginClass;

struct _DefaultProfilePlugin{
	AnjutaPlugin parent;
};

struct _DefaultProfilePluginClass{
	AnjutaPluginClass parent_class;
};


#include <libanjuta/anjuta-plugin.h>

typedef struct _BasicAutotoolsPlugin BasicAutotoolsPlugin;
typedef struct _BasicAutotoolsPluginClass BasicAutotoolsPluginClass;

struct _BasicAutotoolsPlugin{
	AnjutaPlugin parent;
	
	gint fm_watch_id;
	gint project_watch_id;
	
	gchar *fm_current_filename;

	gint build_merge_id;
	GtkActionGroup *build_action_group;
};

struct _BasicAutotoolsPluginClass{
	AnjutaPluginClass parent_class;
};

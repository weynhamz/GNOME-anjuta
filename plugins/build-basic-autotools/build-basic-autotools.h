
#include <libanjuta/anjuta-plugin.h>

typedef struct _BasicAutotoolsPlugin BasicAutotoolsPlugin;
typedef struct _BasicAutotoolsPluginClass BasicAutotoolsPluginClass;

struct _BasicAutotoolsPlugin{
	AnjutaPlugin parent;
	
	gint fm_merge_id;
	gint fm_watch_id;
	GtkActionGroup *fm_popup_action_group;
	
	gint build_merge_id;
	GtkActionGroup *build_action_group;
	gint project_watch_id;
};

struct _BasicAutotoolsPluginClass{
	AnjutaPluginClass parent_class;
};


#include <libanjuta/anjuta-plugin.h>

typedef struct _BasicAutotoolsPlugin BasicAutotoolsPlugin;
typedef struct _BasicAutotoolsPluginClass BasicAutotoolsPluginClass;

struct _BasicAutotoolsPlugin{
	AnjutaPlugin parent;
	
	/* Watch IDs */
	gint fm_watch_id;
	gint project_watch_id;
	gint editor_watch_id;
	
	/* Watched values */
	gchar *fm_current_filename;
	gchar *project_root_dir;
	gchar *current_editor_filename;

	/* UI */
	gint build_merge_id;
	GtkActionGroup *build_action_group;
};

struct _BasicAutotoolsPluginClass{
	AnjutaPluginClass parent_class;
};

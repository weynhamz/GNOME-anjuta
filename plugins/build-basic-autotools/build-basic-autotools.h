
#ifndef __BUILD_BASIC_AUTOTOOLS_H__
#define __BUILD_BASIC_AUTOTOOLS_H__

#include <libanjuta/anjuta-plugin.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-build-basic-autotools-plugin.glade"

typedef struct _BasicAutotoolsPlugin BasicAutotoolsPlugin;
typedef struct _BasicAutotoolsPluginClass BasicAutotoolsPluginClass;

struct _BasicAutotoolsPlugin{
	AnjutaPlugin parent;
	
	/* Build contexts pool */
	GList *contexts_pool;

	/* Editors in which indicators have been updated */
	GHashTable *indicators_updated_editors;
	
	/* Watch IDs */
	gint fm_watch_id;
	gint pm_watch_id;
	gint project_watch_id;
	gint editor_watch_id;
	
	/* Watched values */
	gchar *fm_current_filename;
	gchar *pm_current_filename;
	gchar *project_root_dir;
	gchar *current_editor_filename;
	IAnjutaEditor *current_editor;
	
	/* UI */
	gint build_merge_id;
	GtkActionGroup *build_action_group;
	
	/* Build parameters */
	gchar *configure_args;
	
	/* Execution parameters */
	gchar *program_args;
	gboolean run_in_terminal;
};

struct _BasicAutotoolsPluginClass{
	AnjutaPluginClass parent_class;
};

#endif


#ifndef _PROJECT_MANAGER_PLUGIN_H_
#define _PROJECT_MANAGER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include <gbf/gbf-project.h>
#include <gbf/gbf-project-model.h>
#include <gbf/gbf-project-view.h>

typedef struct _ProjectManagerPlugin ProjectManagerPlugin;
typedef struct _ProjectManagerPluginClass ProjectManagerPluginClass;

struct _ProjectManagerPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	GbfProject *project;
	GtkWidget *view;
	GbfProjectModel *model;
	GtkWidget *scrolledwindow;
	
	GtkActionGroup *pm_action_group;
	GtkActionGroup *popup_action_group;
	gint merge_id;
	
	gint fm_watch_id;
	gint editor_watch_id;
	
	gchar *fm_current_uri;
	gchar *current_editor_uri;
};

struct _ProjectManagerPluginClass{
	AnjutaPluginClass parent_class;
};

void pm_init (ProjectManagerPlugin *fv);
void pm_finalize (ProjectManagerPlugin *fv);

void       pm_set_root (ProjectManagerPlugin *fv, const gchar *root_dir);
void       pm_clear    (ProjectManagerPlugin *fv);
GList*     pm_get_node_expansion_states (ProjectManagerPlugin *fv);
void       pm_set_node_expansion_states (ProjectManagerPlugin *fv,
									  GList *expansion_states);
gchar*     pm_get_selected_file_path (ProjectManagerPlugin *fv);

void       pm_refresh (ProjectManagerPlugin *fv);

// void        pm_customize(gboolean really_show);
// gboolean   pm_open_file (const char *path, gboolean use_anjuta);
// void        pm_session_save (ProjectDBase *p);
// void        pm_session_load (ProjectDBase *p);

#endif


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
	gint project_watch_id;
	
	gchar *fm_current_uri;
	gchar *current_editor_uri;
	
	/* Update state recording */
	GList *pre_update_sources;
	GList *pre_update_targets;
	GList *pre_update_groups;
};

struct _ProjectManagerPluginClass{
	AnjutaPluginClass parent_class;
};

#endif

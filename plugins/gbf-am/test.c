#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-init.h>
#include "gbf-am-project.h"

static void
source_print (GbfProject *project, GbfProjectTargetSource *source, gint level)
{
	gchar *indent = g_strnfill (level * 3, ' ');

	g_assert (source != NULL);
	
	g_print ("%s%s\n", indent, source->source_uri);
	
	g_free (indent);
}

static void
target_print (GbfProject *project, GbfProjectTarget *target, gint level)
{
	GList *iter;
	gchar *indent = g_strnfill (level * 3, ' ');

	g_assert (target != NULL);
	
	g_print ("%s%s [%s]\n", indent, target->name, target->type);
	for (iter = target->sources; iter; iter = g_list_next (iter)) {
		GbfProjectTargetSource *source;
		source = gbf_project_get_source (project, iter->data, NULL);
		source_print (project, source, level + 1);
		gbf_project_target_source_free (source);
	}
	
	g_free (indent);
}

static void
group_print (GbfProject *project, GbfProjectGroup *group, gint level)
{
	GList *iter;
	gchar *indent = g_strnfill (level * 3, ' ');

	g_assert (group != NULL);
	
	g_print ("%s%s\n", indent, group->name);
	for (iter = group->groups; iter; iter = g_list_next (iter)) {
		GbfProjectGroup *subgroup;
		subgroup = gbf_project_get_group (project, iter->data, NULL);
		group_print (project, subgroup, level + 1);
		gbf_project_group_free (subgroup);
	}
	
	for (iter = group->targets; iter; iter = g_list_next (iter)) {
		GbfProjectTarget *target;
		target = gbf_project_get_target (project, iter->data, NULL);
		target_print (project, target, level + 1);
		gbf_project_target_free (target);
	}
	
	g_free (indent);
}

static void
project_print (GbfProject *project)
{
	GbfProjectGroup *group;

	group = gbf_project_get_group (project, "/", NULL);
	group_print (project, group, 0);
	gbf_project_group_free (group);
}

static void
project_updated_cb (GbfProject *project, gpointer user_data)
{
	g_print ("----------========== PROJECT UPDATED ==========----------\n");
	project_print (project);
	g_print ("----------=====================================----------\n");
}

#define ERROR_CHECK(err)  G_STMT_START {					\
	if ((err) != NULL) {							\
		g_print ("! Last operation failed: %s\n", (err)->message);	\
		g_error_free (err);						\
		(err) = NULL;							\
		goto out;							\
	}									\
} G_STMT_END;

int
main (int argc, char *argv[])
{
	GbfProject *project;
	gboolean valid;
	GList *list;
	gchar *dir;
	gchar *new_group_id, *new_target_id, *new_source_id;
	GError *error = NULL;
	
	/* Bootstrap */
	gnome_program_init ("libgbf-am-test", VERSION, LIBGNOME_MODULE, 
			    argc, argv, NULL,
			    NULL);  /* Avoid GCC sentinel warning */

	if (argc < 2) {
		g_print ("! You need to specify a project path\n");
		return 0;
	}
	dir = argv [1];
	
	g_print ("+ Creating new gbf-am project\n");
	project = gbf_am_project_new ();
	if (!project) {
		g_print ("! Project creation failed\n");
		return 0;
	}
	g_signal_connect (project, "project-updated",
			  G_CALLBACK (project_updated_cb), NULL);
	
	g_print ("+ Probing location: ");
	valid = gbf_project_probe (project, dir, &error);
	g_print ("%s\n", valid ? "ok" : "not an automake project");
	ERROR_CHECK (error);
	
	if (!valid)
		goto out;
	
	g_print ("+ Loading project %s\n\n", dir);
	gbf_project_load (project, dir, &error);
	ERROR_CHECK (error);
	
	/* Show the initial project structure */
	g_print ("+ Project structure:\n\n");
	project_print (project);
	g_print ("\n");

	/* Test remaining elements accessors */
	g_print ("+ Testing getters:\n\n");

	g_print ("- All targets:\n");
	list = gbf_project_get_all_targets (project, NULL);
	while (list) {
		g_print ("%s\n", (gchar *) list->data);
		g_free (list->data);
		list = g_list_delete_link (list, list);
	}
	g_print ("\n");
	
	g_print ("- All sources:\n");
	list = gbf_project_get_all_sources (project, NULL);
	while (list) {
		g_print ("%s\n", (gchar *) list->data);
		g_free (list->data);
		list = g_list_delete_link (list, list);
	}
	g_print ("\n");

	/* Test adders and removers */
	g_print ("+ Adding a group \"test\" in \"/\"\n");
	new_group_id = gbf_project_add_group (project, "/", "test", &error);
	ERROR_CHECK (error);
	g_print ("+ Got group id \"%s\"\n\n", new_group_id);

	g_print ("+ Adding a \"program\" target \"test\" in \"%s\"\n", new_group_id);
	new_target_id = gbf_project_add_target (project, new_group_id,
						"test", "program", &error);
	ERROR_CHECK (error);
	g_print ("+ Got target id \"%s\"\n\n", new_target_id);

	g_print ("+ Adding a source \"test.c\" to target \"%s\"\n", new_target_id);
	new_source_id = gbf_project_add_source (project, new_target_id, "test.c", &error);
	ERROR_CHECK (error);
	g_print ("+ Got source id \"%s\"\n\n", new_source_id);

	g_print ("+ Removing the source \"%s\"\n\n", new_source_id);
	gbf_project_remove_source (project, new_source_id, &error);
	ERROR_CHECK (error);

#if 0
	g_print ("+ Removing the target \"%s\"\n\n", new_target_id);
	gbf_project_remove_target (project, new_target_id, &error);
	ERROR_CHECK (error);
#endif
	
#if 0
	g_print ("+ Removing the group \"%s\"\n\n", new_group_id);
	gbf_project_remove_group (project, new_group_id, &error);
	ERROR_CHECK (error);
#endif
	g_free (new_group_id);
	g_free (new_target_id);
	g_free (new_source_id);
	
	/* Test file monitoring: we need to enter the main loop for
	 * the signal to be emitted */
	sleep (1);
	g_print ("+ Testing file monitoring:\n\n");
	{
		gchar *file, *cmd;
		file = g_build_filename (dir, "configure.in", NULL);
		cmd = g_strdup_printf ("touch %s", file);
		g_spawn_command_line_sync (cmd, NULL, NULL, NULL, NULL);
		g_free (cmd);
		g_free (file);
	}

	g_timeout_add (3000, (GSourceFunc) gtk_main_quit, NULL);
	gtk_main ();
	
  out:
	g_print ("+ Unreffing project\n\n");
	g_object_unref (project);

	return 0;
}


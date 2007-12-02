#include <gtk/gtk.h>
#include "main.h"


void open_playlist(void)
{
	GtkWidget *selection = NULL;
	gchar *path = NULL;
	selection = gtk_file_selection_new(_("Open a Task List"));

	switch(gtk_dialog_run(GTK_DIALOG(selection)))
	{
		case GTK_RESPONSE_OK:
			path = g_strdup_printf("gtodo \"%s\"",gtk_file_selection_get_filename(GTK_FILE_SELECTION(selection)));
			g_print("%s\n", path);
			g_spawn_command_line_async(path, NULL);	
			g_free(path);
		default:
			gtk_widget_destroy(selection);
			return;
	}
}



void create_playlist(void)
{
	GtkWidget *selection = NULL;
	gchar *path = NULL;
	selection = gtk_file_selection_new(_("Create a Task List"));

	switch(gtk_dialog_run(GTK_DIALOG(selection)))
	{
		case GTK_RESPONSE_OK:
			path = g_strdup_printf("gtodo %s",gtk_file_selection_get_filename(GTK_FILE_SELECTION(selection)));
			g_print("%s\n", path);
			g_spawn_command_line_async(path, NULL);	
			g_free(path);
		default:
			gtk_widget_destroy(selection);
			return;
	}
}

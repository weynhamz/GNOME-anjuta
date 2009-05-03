#include <gtk/gtk.h>
#include <libanjuta/anjuta-debug.h>
#include "main.h"


void open_playlist(void)
{
	GtkWidget *selection = NULL;
	gchar *path = NULL;
	selection = gtk_file_chooser_dialog_new (_("Open a Task List"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_OK,
				      NULL);


	switch(gtk_dialog_run(GTK_DIALOG(selection)))
	{
		case GTK_RESPONSE_OK:
			path = g_strdup_printf("gtodo \"%s\"",gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (selection)));
			DEBUG_PRINT ("%s\n", path);
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
	selection = gtk_file_chooser_dialog_new (_("Create a Task List"), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_SAVE, GTK_RESPONSE_OK,
				      NULL);

	switch(gtk_dialog_run(GTK_DIALOG(selection)))
	{
		case GTK_RESPONSE_OK:
			path = g_strdup_printf("gtodo %s",gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (selection)));
			DEBUG_PRINT ("%s\n", path);
			g_spawn_command_line_async(path, NULL);	
			g_free(path);
		default:
			gtk_widget_destroy(selection);
			return;
	}
}

#include <config.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>
#include "main.h"
#include "tray-icon.h"
#include "debug_printf.h"

static char *parse_command (int argc, char **argv);
gint xsize=-1, ysize=-1, xpos=-1, ypos=-1;

// void set_read_only();
// void configuration_restore_size(GConfClient *client);
// void configuration_restore_place(GConfClient *client);
// GConfEngine* gconf_engine_get_default (void); 

static load_settings ()
{
	gtodo_load_settings();
}

static update_settings (GtkWidget *win)
{
	// gtodo_update_settings();
	if(gconf_client_get_int(client, "/apps/gtodo/prefs/size-x",NULL) != -1 &&  
		gconf_client_get_int(client, "/apps/gtodo/prefs/size-x",NULL) != -1 && 
		settings.size)
	{
		gtk_window_resize(GTK_WINDOW(win),  gconf_client_get_int(client, "/apps/gtodo/prefs/size-x",NULL),  gconf_client_get_int(client, "/apps/gtodo/prefs/size-y",NULL));
	}
	
	if(gconf_client_get_int(client, "/apps/gtodo/prefs/pos-x",NULL) != -1 && 
		gconf_client_get_int(client, "/apps/gtodo/prefs/pos-y",NULL) != -1 && 
		settings.place)
	{
		gtk_window_move(GTK_WINDOW(win), gconf_client_get_int(client, "/apps/gtodo/prefs/pos-x",NULL), gconf_client_get_int(client, "/apps/gtodo/prefs/pos-y",NULL));
	}
}

int main(int argc, char **argv){
	char *new_path = NULL;
	GError *error = NULL;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	setlocale(LC_ALL,"");

	debug_printf(DEBUG_INFO, "Parsing commandline options");
	new_path = parse_command (argc, argv);

	debug_printf(DEBUG_INFO, "Initializing gtk and gconf");
	gtk_init(&argc, &argv);
	gconf_init(argc, argv, NULL);
	gnome_vfs_init();

	if(new_path == NULL)
	{
		char *path = g_strdup_printf("%s/.gtodo/", g_getenv("HOME"));
		debug_printf(DEBUG_INFO, "Trying to open default location.");
		if(!g_file_test(path, G_FILE_TEST_EXISTS))
		{
			gnome_vfs_make_directory(path, 0750);
			if(!g_file_test(path, G_FILE_TEST_EXISTS))
			{
				debug_printf(DEBUG_ERROR, "Failed to create directory: %s", path);
				exit(1);
			}
		}
		g_free(path);
		path = g_strdup_printf("%s/.gtodo/todos", g_getenv("HOME"));
		cl = gtodo_client_new_from_file(path, &error);
		g_free(path);
	}
	else
	{
		GnomeVFSURI *uri = gnome_vfs_uri_new(new_path);
		debug_printf(DEBUG_INFO, "Trying to open given uri.");
		gchar *title = NULL, *tmp = NULL;
		if(uri == NULL)
		{
			debug_printf(DEBUG_ERROR, "Invalid uri %s.", new_path);
			exit(1);
		}
		/* test if its a local uri.. they can be releative.. and we need to resolve that */
		if(gnome_vfs_uri_is_local(uri))
		{
			/* if the uri exists we can open it.. no need for more tests */
			if(gnome_vfs_uri_exists(uri))
			{
				cl = gtodo_client_new_from_file(new_path, &error);

			}
			else
			{
				/* we still need to find out if its a valid path */
				char *dirname =  gnome_vfs_uri_extract_dirname(uri);
				if(!strncmp(dirname, ".", 1) || (strlen(dirname) == 1 && !strncmp(dirname, "/", 1)))
				{
					/* there should be default function to get the full path from a relative one  */
					char *cur_dir_tmp = g_get_current_dir();
					char *path = NULL;
					/* grrrrrrrr stupid gnome_vfs needs a ending / in the base path */
					char *cur_dir = g_strdup_printf("%s/", cur_dir_tmp);
					g_free(cur_dir_tmp);
					path =  gnome_vfs_uri_make_full_from_relative(cur_dir,new_path);
					cl = gtodo_client_new_from_file(path, &error);
					g_free(path);		
					g_free(cur_dir);
				}
				else
				{
					cl = gtodo_client_new_from_file(new_path, &error);
				}
				g_free(dirname);
			}
		}
		else
		{
			cl = gtodo_client_new_from_file(new_path, &error);
		}
		/* start checking the GError */
		if(error != NULL)
		{
			GtkWidget *dialog = NULL;
			dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(mw.window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"The following error occured while opening:\n<i>%s</i>",
					error->message);
			gtk_widget_show_all(dialog);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			debug_printf(DEBUG_ERROR,"Error while opening: %s\n", error->message);

			exit(1);
		}

		tmp = gnome_vfs_uri_extract_short_name(uri);
		title = g_strdup_printf("Todo List - %s %s", tmp, (gtodo_client_get_read_only(cl) == TRUE)? "(RO)": "" );
		gtk_window_set_title(GTK_WINDOW(mw.window), title);
		g_free(tmp);
		g_free(title);

		gnome_vfs_uri_unref(uri);
	}

	if(cl == NULL)
	{
		debug_printf(DEBUG_ERROR,"Failed to open, or create the default gtodo client.\n");
		exit(1);
	}


	load_settings();

	/* create the main window */
	debug_printf(DEBUG_INFO, "Creating main interface");
	mw.window = gui_create_main_window();
	update_settings(mw.window);
	gtk_widget_show_all(mw.window);
	
	tray_init(mw.window);
	gtk_main();
	return 0;
}

static void startup_new_item()
{
	g_print("add\n");
	gui_add_todo_item(NULL,GINT_TO_POINTER(0), 0);
}

static void startup_view_item(guint32 buf)
{
	gui_add_todo_item(NULL, GINT_TO_POINTER(-1), buf);
}

static char *parse_command (int argc, char **argv) 
{
	int i;
	char *output = NULL;
	if(argc < 2) return NULL;
	for(i=1; i< argc; i++) 
	{
		if(!strncmp(argv[i], "-?", 2) || !strncmp(argv[i], "--help", 6) || !strncmp(argv[i], _("--help"), strlen(_("--help"))))
		{
			printf(_("gtodo v%s\ngtodo has the following command-line options:\n-s	--show	Shows a todo item by its ID.\n only useful for programs calling gtodo\n-?	--help	This Message.\n"), VERSION);
			exit(1);
		}
		else if (!strncmp(argv[i], "-s", 2) || !strncmp(argv[i], "--show", 6) || !strncmp(argv[i], _("--show"), strlen(_("--show"))))
		{
			if(argc < 1 || argv[i+1]  == NULL)
			{
				g_print(_("The --show option take a to do item ID as argument\n"));
				exit(1);
			}
			i++;
			gtk_init_add((GtkFunction)startup_view_item,(gpointer)(gint)g_ascii_strtoull(argv[i], NULL, 0));
		}
		else if (!strncmp(argv[i], "-n", 2) || !strncmp(argv[i], "--new", 6) || !strncmp(argv[i], _("--new"), strlen(_("--new"))))
		{
			i++;
			gtk_init_add((GtkFunction)startup_new_item,NULL);
		}

		else{
			output = argv[i];
		}
	}
	return output;
}

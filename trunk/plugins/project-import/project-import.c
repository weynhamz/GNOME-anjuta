/*
 *  project-import.c (c) 2005 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "project-import.h"
#include <gnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include <config.h>

#include <gbf/gbf-backend.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-project-import.glade"
#define AM_PROJECT_FILE PACKAGE_DATA_DIR"/project/terminal/project.anjuta"
#define MKFILE_PROJECT_FILE PACKAGE_DATA_DIR"/project/mkfile/project.anjuta"

static GObjectClass *parent_class = NULL;

static void
on_import_cancel (GnomeDruid* druid, ProjectImport* pi)
{
	g_object_unref (G_OBJECT(pi));
}

static gboolean
on_import_key_press_event(GtkWidget *widget, GdkEventKey *event,
                          ProjectImport* pi)
{
	if (event->keyval == GDK_Escape)
	{
		g_object_unref(G_OBJECT(pi));
		return TRUE;
	}
	return FALSE;
}

static gboolean
on_import_next(GnomeDruidPage* page, GtkWidget* druid, ProjectImport* pi)
{
	GSList* l;
	GbfBackend* backend = NULL;
	GbfProject* proj;
	
	const gchar* name = gtk_entry_get_text(GTK_ENTRY(pi->import_name));
	const gchar* path = gtk_entry_get_text(GTK_ENTRY(pi->import_path));
	
	if (!strlen(name) || !strlen(path))
		return TRUE;
	
	gbf_backend_init();
	
	for (l = gbf_backend_get_backends (); l; l = l->next) {
		backend = l->data;
		if (!backend)
		{
			g_warning("Backend appears empty!");
			continue;
		}
		
		/* Probe the backend to find out if the project directory is OK */
		/* If probe() returns TRUE then we have a valid backend */
		
		proj = gbf_backend_new_project(backend->id);
		if (proj)
		{
			if (gbf_project_probe(proj, path, NULL))
			{
				/* This is a valid backend for this root directory */
				/* FIXME: Possibility of more than one valid backend? */
				g_object_unref(proj);
				break;
			}
			g_object_unref(proj);
		}
		backend = NULL;
	}	

	if (!backend)
	{
		gchar* message_text =
		g_strdup_printf(_("Could not find a valid project backend for the "
						  "directory given (%s). Please select a different "
						  "directory, or try upgrading to a newer version of "
						  "the Gnome Build Framework."), path);
		
		GtkDialog* message = 
		GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(pi->window),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_ERROR,
										  GTK_BUTTONS_CLOSE,
										  message_text));
		
		g_free(message_text);
	
		gtk_dialog_run(message);
		gtk_widget_destroy(GTK_WIDGET(message));
		return TRUE;
	}
	
	gchar* summary;
	
	summary = g_strdup_printf(_("Project name: %s\n"
								"Project type: %s\n"
								"Project path: %s\n"),
								name, backend->name, path);
	gnome_druid_page_edge_set_text(GNOME_DRUID_PAGE_EDGE(pi->import_finish),
								   summary);

	g_free(summary);
	
	if (pi->backend_id)
		g_free(pi->backend_id);
	pi->backend_id = g_strdup(backend->id);
	
	return FALSE;							   
}

static gboolean
on_import_finish (GnomeDruidPage* page, GtkWidget* druid, ProjectImport* pi)
{
	const gchar* name = gtk_entry_get_text (GTK_ENTRY(pi->import_name));
	const gchar* path = gtk_entry_get_text (GTK_ENTRY(pi->import_path));
	
	gchar* project_file = g_strconcat (path, "/", name, ".", "anjuta", NULL);
	
	IAnjutaFileLoader* loader;
	
	if (!project_import_generate_file (pi, project_file))
	{
		g_free (project_file);
		return TRUE;
	}
	
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (pi->plugin)->shell,
	                                     IAnjutaFileLoader, NULL);
	if (!loader)
	{
		g_warning("No IAnjutaFileLoader interface! Cannot open project file!");
		g_object_unref (G_OBJECT (pi));
		g_free (project_file);
		return FALSE;
	}
	ianjuta_file_loader_load (loader, project_file, FALSE, NULL);
	g_object_unref (G_OBJECT (pi));
	g_free (project_file);
	return FALSE;
}

static void
project_import_init(ProjectImport *pi)
{
	GladeXML* gxml = glade_xml_new(GLADE_FILE, "import_window", NULL);
	GtkWidget* import_page;
	
	pi->window = glade_xml_get_widget(gxml, "import_window");
	pi->druid = glade_xml_get_widget(gxml, "import_druid");
	pi->import_name = glade_xml_get_widget(gxml, "import_name");
	pi->import_path = glade_xml_get_widget(gxml, "import_path");
	pi->import_finish = glade_xml_get_widget(gxml, "import_finish");
	
	pi->backend_id = NULL;
	
	import_page = glade_xml_get_widget(gxml, "import_page");
	g_signal_connect(G_OBJECT(import_page), "next", 
					 G_CALLBACK(on_import_next), pi);
	g_signal_connect(G_OBJECT(pi->import_finish), "finish",
					 G_CALLBACK(on_import_finish), pi);
	g_signal_connect(G_OBJECT(pi->druid), "cancel",
					 G_CALLBACK(on_import_cancel), pi);
	g_signal_connect(G_OBJECT(pi->druid), "key-press-event",
			G_CALLBACK(on_import_key_press_event), pi);

	g_object_unref(G_OBJECT(gxml));
	gtk_widget_show_all(pi->window);
}

static void
project_import_finalize(GObject *object)
{
	ProjectImport *cobj;
	cobj = PROJECT_IMPORT(object);
	
	if (cobj->backend_id)
		g_free(cobj->backend_id);
	
	gtk_widget_destroy(cobj->window);
	
	/* Deactivate plugin once wizard is finished */
	if (anjuta_plugin_is_active(cobj->plugin))
		anjuta_plugin_deactivate (cobj->plugin);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
project_import_class_init(ProjectImportClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = project_import_finalize;
}

GType
project_import_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (ProjectImportClass),
			NULL,
			NULL,
			(GClassInitFunc)project_import_class_init,
			NULL,
			NULL,
			sizeof (ProjectImport),
			0,
			(GInstanceInitFunc)project_import_init,
		};

		type = g_type_register_static(G_TYPE_OBJECT, 
			"ProjectImport", &our_info, 0);
	}

	return type;
}

ProjectImport *
project_import_new(AnjutaPlugin* plugin)
{
	ProjectImport *obj;
	
	obj = PROJECT_IMPORT(g_object_new(PROJECT_IMPORT_TYPE, NULL));
	
	obj->plugin = plugin;
	gtk_window_set_transient_for (GTK_WINDOW (obj->window),
								  GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell));
	
	return obj;
}

void
project_import_set_name (ProjectImport *pi, const gchar *name)
{
	g_return_if_fail (IS_PROJECT_IMPORT (pi));
	g_return_if_fail (name != NULL);
	
	gtk_entry_set_text (GTK_ENTRY (pi->import_name), name);
}

void
project_import_set_directory (ProjectImport *pi, const gchar *directory)
{
	g_return_if_fail (IS_PROJECT_IMPORT (pi));
	g_return_if_fail (directory != NULL);
	
	gtk_entry_set_text (GTK_ENTRY (pi->import_path), directory);
}

gboolean
project_import_generate_file(ProjectImport* pi, const gchar* prjfile)
{
	/* Of course we could do some more intelligent stuff here
	and check which plugins are really needed but for now we just
	take a default project file. */
	
	GnomeVFSURI* source_uri;
	if (!strcmp (pi->backend_id, "gbf-am:GbfAmProject"))
		source_uri = gnome_vfs_uri_new(AM_PROJECT_FILE);
	else if (!strcmp (pi->backend_id, "gbf-mkfile:GbfMkfileProject"))
		source_uri = gnome_vfs_uri_new(MKFILE_PROJECT_FILE);
	else
	{
		/* We shouldn't get here, unless someone has upgraded their GBF */
		/* but not Anjuta.                                              */
		
		GtkWidget *dlg;
		
		dlg = gtk_message_dialog_new(GTK_WINDOW(pi->window), 
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR, 
									 GTK_BUTTONS_CLOSE,
									 _("Generation of project file failed. Cannot "
									   "find an appropriate project template to "
									   "use. Please make sure your version of "
									   "Anjuta is up to date."));
		
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy (dlg);
		return FALSE;
	}
	
	GnomeVFSURI* dest_uri = gnome_vfs_uri_new(prjfile);
	
	GnomeVFSResult error = gnome_vfs_xfer_uri (source_uri,
						dest_uri,
						GNOME_VFS_XFER_DEFAULT,
						GNOME_VFS_XFER_ERROR_MODE_ABORT,
						GNOME_VFS_XFER_OVERWRITE_MODE_ABORT,
						NULL,
						NULL);
	gnome_vfs_uri_unref (source_uri);
	gnome_vfs_uri_unref (dest_uri);
	
	if (error != GNOME_VFS_OK)
	{
		GtkWidget *dlg;
		
		dlg = gtk_message_dialog_new(GTK_WINDOW(pi->window), 
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR, 
									 GTK_BUTTONS_OK,
									 _("Generation of project file failed. Please "
									   "check if you have write access to the project "
									   "directory: %s"),
									 gnome_vfs_result_to_string (error));
		
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy (dlg);
		return FALSE;
	}
	return TRUE;
}

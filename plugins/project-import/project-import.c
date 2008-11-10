/*
 *  project-import.c (c) 2005 Johannes Schmid
 *			 2008 Ignacio Casal Quinteiro
 * 
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "project-import.h"
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/anjuta-debug.h>

#include <config.h>

#include <gbf/gbf-backend.h>

#define AM_PROJECT_FILE PACKAGE_DATA_DIR"/project/terminal/project.anjuta"
#define MKFILE_PROJECT_FILE PACKAGE_DATA_DIR"/project/mkfile/project.anjuta"

static GObjectClass *parent_class = NULL;

static void
on_import_cancel (GtkAssistant* assistant, ProjectImport* pi)
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

static void
on_import_next(GtkAssistant *assistant, GtkWidget *page, ProjectImport *pi)
{
	GSList* l;
	GbfBackend* backend = NULL;
	GbfProject* proj;
	
	if (page != pi->import_finish)
		return;
	
	const gchar* name = gtk_entry_get_text (GTK_ENTRY (pi->import_name));
	gchar* path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (pi->import_path));
	
	if (!name || !path || !strlen(name) || !strlen(path))
	{
		g_free (path);
		return;
	}
	
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
		GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(pi->assistant),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_ERROR,
										  GTK_BUTTONS_CLOSE,
										  message_text));
		
		g_free(message_text);
	
		gtk_dialog_run(message);
		gtk_widget_destroy(GTK_WIDGET(message));
		g_free (path);
		
		/*
		 * Now we can't apply
		 */
		gtk_label_set_text (GTK_LABEL (pi->import_finish), _("Please, fix the configuration"));
		gtk_assistant_set_page_complete (GTK_ASSISTANT (pi->assistant), pi->import_finish, FALSE);
		return;
	}
	
	gchar* summary;
	
	summary = g_strdup_printf(_("Project name: %s\n"
								"Project type: %s\n"
								"Project path: %s\n"),
								name, backend->name, path);
	gtk_label_set_text (GTK_LABEL (pi->import_finish),
			    summary);

	g_free(summary);
	
	/*
	 * If we are here, everything is ok
	 */
	gtk_assistant_set_page_complete (GTK_ASSISTANT (pi->assistant), pi->import_finish, TRUE);
	
	if (pi->backend_id)
		g_free(pi->backend_id);
	pi->backend_id = g_strdup(backend->id);
	g_free (path);
}

static void
on_import_apply (GtkAssistant *assistant, ProjectImport* pi)
{
	const gchar* name = gtk_entry_get_text (GTK_ENTRY(pi->import_name));
	gchar* path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(pi->import_path));
	gchar* project_file = g_strconcat (path, "/", name, ".", "anjuta", NULL);
	GFile* file = g_file_new_for_path (project_file);
	
	IAnjutaFileLoader* loader;
	
	if (!project_import_generate_file (pi, project_file))
	{
		g_free (project_file);
		g_free (path);
		return;
	}
	
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (pi->plugin)->shell,
	                                     IAnjutaFileLoader, NULL);
	if (!loader)
	{
		g_warning("No IAnjutaFileLoader interface! Cannot open project file!");
		g_free (project_file);
		return;
	}
	ianjuta_file_loader_load (loader, file, FALSE, NULL);
	g_free (project_file);
	g_free (path);
}

static void
create_start_page (ProjectImport *pi)
{
	GtkWidget *box, *label;

	box = gtk_hbox_new (FALSE, 12);
	gtk_widget_show (box);
	gtk_container_set_border_width (GTK_CONTAINER (box), 12);

	label = gtk_label_new (_("This assistant will import an existing project into Anjuta."));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	gtk_assistant_append_page (GTK_ASSISTANT (pi->assistant), box);
	gtk_assistant_set_page_title (GTK_ASSISTANT (pi->assistant), box, _("Import Project"));
	gtk_assistant_set_page_complete (GTK_ASSISTANT (pi->assistant), box, TRUE);
	gtk_assistant_set_page_type (GTK_ASSISTANT (pi->assistant), box, GTK_ASSISTANT_PAGE_INTRO);
}

static void
on_entry_changed (GtkWidget *widget, gpointer data)
{
	GtkAssistant *assistant = GTK_ASSISTANT (data);
	GtkWidget *current_page;
	gint page_number;
	const gchar *text;

	page_number = gtk_assistant_get_current_page (assistant);
	current_page = gtk_assistant_get_nth_page (assistant, page_number);
	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (text && *text)
		gtk_assistant_set_page_complete (assistant, current_page, TRUE);
	else
		gtk_assistant_set_page_complete (assistant, current_page, FALSE);
}

static void
create_import_page (ProjectImport *pi)
{
	GtkWidget *box, *vbox1, *vbox2;
	GtkWidget *label;

	box = gtk_vbox_new (FALSE, 12);
	gtk_widget_show (box);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);

	/*
	 * Project name:
	 */
	vbox1 = gtk_vbox_new (FALSE, 12);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (box), vbox1, FALSE, FALSE, 0);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label),
			      _("<b>Enter the project name:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);
	
	pi->import_name = gtk_entry_new ();
	gtk_widget_show (pi->import_name);
	gtk_box_pack_start (GTK_BOX (vbox1), pi->import_name, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (pi->import_name), "changed",
			  G_CALLBACK (on_entry_changed), pi->assistant);
	
	/*
	 * Base path:
	 */
	vbox2 = gtk_vbox_new (FALSE, 12);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (box), vbox2, FALSE, FALSE, 0);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label),
			      _("<b>Enter the base path of your project:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	
	pi->import_path = gtk_file_chooser_button_new (_("Select project directory"),
						       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_widget_show (pi->import_path);
	gtk_box_pack_start (GTK_BOX (vbox2), pi->import_path, FALSE, FALSE, 0);

	gtk_assistant_append_page (GTK_ASSISTANT (pi->assistant), box);
	gtk_assistant_set_page_title (GTK_ASSISTANT (pi->assistant), box, _("Project to Import"));
}

static void
create_finish_page (ProjectImport *pi)
{
	pi->import_finish = gtk_label_new (NULL);
	gtk_widget_show (pi->import_finish);
	
	gtk_assistant_append_page (GTK_ASSISTANT (pi->assistant), pi->import_finish);
	gtk_assistant_set_page_type (GTK_ASSISTANT (pi->assistant), pi->import_finish,
				     GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_complete (GTK_ASSISTANT (pi->assistant), pi->import_finish, TRUE);
	gtk_assistant_set_page_title (GTK_ASSISTANT (pi->assistant), pi->import_finish, _("Confirmation"));
}

static void
project_import_init (ProjectImport *pi)
{
	pi->assistant = gtk_assistant_new ();
	create_start_page (pi);
	create_import_page (pi);
	create_finish_page (pi);
	
	pi->backend_id = NULL;
	
	g_signal_connect(G_OBJECT (pi->assistant), "prepare", 
					 G_CALLBACK(on_import_next), pi);
	g_signal_connect(G_OBJECT (pi->assistant), "apply",
					 G_CALLBACK (on_import_apply), pi);
	g_signal_connect(G_OBJECT (pi->assistant), "cancel",
					 G_CALLBACK (on_import_cancel), pi);
	g_signal_connect(G_OBJECT (pi->assistant), "close",
					 G_CALLBACK (on_import_cancel), pi);
	g_signal_connect(G_OBJECT(pi->assistant), "key-press-event",
					 G_CALLBACK(on_import_key_press_event), pi);

	gtk_widget_show (pi->assistant);
}

static void
project_import_finalize(GObject *object)
{
	ProjectImport *cobj;
	cobj = PROJECT_IMPORT(object);
	
	DEBUG_PRINT ("Finalizing ProjectImport object");
	
	if (cobj->backend_id)
		g_free(cobj->backend_id);
	
	gtk_widget_destroy(cobj->assistant);
	
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
	gtk_window_set_transient_for (GTK_WINDOW (obj->assistant),
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
	GFile* file;

	g_return_if_fail (IS_PROJECT_IMPORT (pi));
	g_return_if_fail (directory != NULL);
	
	file = g_file_parse_name (directory);
	gchar* uri = g_file_get_uri (file);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (pi->import_path), uri);
	g_object_unref (G_OBJECT (file));
	g_free (uri);
}

gboolean
project_import_generate_file(ProjectImport* pi, const gchar* prjfile)
{
	/* Of course we could do some more intelligent stuff here
	and check which plugins are really needed but for now we just
	take a default project file. */
	
	GFile* source_file;
	if (!strcmp (pi->backend_id, "gbf-am:GbfAmProject"))
		source_file = g_file_new_for_path (AM_PROJECT_FILE);
	else if (!strcmp (pi->backend_id, "gbf-mkfile:GbfMkfileProject"))
		source_file = g_file_new_for_path (MKFILE_PROJECT_FILE);
	else
	{
		/* We shouldn't get here, unless someone has upgraded their GBF */
		/* but not Anjuta.                                              */
		
		GtkWidget *dlg;
		
		dlg = gtk_message_dialog_new(GTK_WINDOW(pi->assistant), 
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
	
	GFile* dest_file = g_file_parse_name (prjfile);
	GError* error = NULL;

	if (!g_file_copy (source_file, dest_file, 
			G_FILE_COPY_NONE,
			NULL,
			NULL,
			NULL,
			&error))
	{
		if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_EXISTS)
		{
			if (anjuta_util_dialog_boolean_question (GTK_WINDOW (pi->assistant),
					_("A file named \"%s\" already exists. "
					  "Do you want to replace it?"), prjfile))
			{
				g_error_free (error);
				error = NULL;
				g_file_copy (source_file, dest_file,
						G_FILE_COPY_OVERWRITE,
						NULL,
						NULL,
						NULL,
						&error);
			}
		}
	}

	g_object_unref (source_file);

	if (!error)
	{
		time_t ctime = time(NULL);
		GFileInfo* file_info = g_file_info_new();
		g_file_info_set_attribute_uint64(file_info, 
				"time::modified",
				ctime);
		g_file_info_set_attribute_uint64(file_info, 
				"time::created",
				ctime);
		g_file_info_set_attribute_uint64(file_info, 
				"time::access",
				ctime);
		g_file_set_attributes_from_info (dest_file, 
				file_info,
				G_FILE_QUERY_INFO_NONE,
				NULL, NULL);

		g_object_unref (G_OBJECT(file_info));
		g_object_unref (dest_file);
	}
	else
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN(pi->plugin)->shell),
				_("A file named \"%s\" cannot be written: %s.  "
				  "Check if you have write access to the project directory."),
				  prjfile, error->message);
		g_object_unref (dest_file);
		return FALSE;

	}

	return TRUE;
}

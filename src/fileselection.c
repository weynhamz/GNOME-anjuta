/*
    fileselection.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "anjuta.h"
#include "resources.h"
#include "fileselection.h"
#include "utilities.h"
#include "gnomefilelist.h"

static gboolean
on_fileselection_delete_event (GtkWidget * w, GdkEvent * event, gpointer data)
{
	gtk_widget_hide (w);
	return TRUE;
}

static void
on_file_selection_ok_clicked (GtkButton * button, gpointer data)
{
	gchar *filename;
	FileSelData *fd = data;

	filename = fileselection_get_filename (fd->filesel);
	if (!filename)
		return;
	if (file_is_directory (filename))
	{
		fileselection_set_dir (fd->filesel, filename);
		g_free (filename);
		return;
	}
	gtk_widget_hide (fd->filesel);
	if (fd->click_ok_callback)
		fd->click_ok_callback (button, fd->data);
	
	g_free (filename);
	return;
}

GtkWidget *
create_fileselection_gui (FileSelData * fsd)
{
	GtkWidget *fileselection_gui;
	GtkWidget *fileselection_ok;
	GtkWidget *fileselection_cancel;

	fileselection_gui = gnome_filelist_new ();
	gnome_filelist_set_title (GNOME_FILELIST(fileselection_gui), _(fsd->title));
	gtk_container_set_border_width (GTK_CONTAINER (fileselection_gui), 10);
	gtk_window_set_position (GTK_WINDOW (fileselection_gui), GTK_WIN_POS_CENTER);
	gtk_window_set_wmclass (GTK_WINDOW (fileselection_gui), "filesel", "Anjuta");
	
	fileselection_ok = GNOME_FILELIST (fileselection_gui)->ok_button;
	gtk_widget_show (fileselection_ok);
	GTK_WIDGET_SET_FLAGS (fileselection_ok, GTK_CAN_DEFAULT);

	fileselection_cancel =GNOME_FILELIST (fileselection_gui)->cancel_button;
	gtk_widget_show (fileselection_cancel);
	GTK_WIDGET_SET_FLAGS (fileselection_cancel, GTK_CAN_DEFAULT);

	gtk_accel_group_attach (app->accel_group,
				GTK_OBJECT (fileselection_gui));

	gtk_signal_connect (GTK_OBJECT (fileselection_gui), "delete_event",
			    GTK_SIGNAL_FUNC (on_fileselection_delete_event),
			    fsd);
	gtk_signal_connect (GTK_OBJECT (fileselection_ok), "clicked",
			    GTK_SIGNAL_FUNC (on_file_selection_ok_clicked),
			    fsd);
	gtk_signal_connect (GTK_OBJECT (fileselection_cancel), "clicked",
			    GTK_SIGNAL_FUNC (fsd->click_cancel_callback),
			    fsd->data);
	gtk_signal_connect (GTK_OBJECT (fileselection_gui), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_unref), NULL);

	fsd->filesel = fileselection_gui;
	gtk_widget_ref (fileselection_gui);
	return fileselection_gui;
}


/* Free the return */
gchar*
fileselection_get_filename (GtkWidget* filesel)
{
	return gnome_filelist_get_filename (GNOME_FILELIST(filesel));
}

GList *
fileselection_get_filelist(GtkWidget * filesel)
{
	 return gnome_filelist_get_filelist (GNOME_FILELIST(filesel));
}

/* Free the return */
gchar*
fileselection_get_path (GtkWidget* filesel)
{
	return gnome_filelist_get_path (GNOME_FILELIST(filesel));
}

void
fileselection_set_title (GtkWidget* filesel, gchar* title)
{
	return gnome_filelist_set_title (GNOME_FILELIST(filesel), title);
}

gboolean
fileselection_set_dir (GtkWidget* filesel, gchar* dir)
{
	return gnome_filelist_set_dir (GNOME_FILELIST(filesel), dir);
}

gboolean
fileselection_set_filename (GtkWidget* filesel, gchar* fname)
{
	return gnome_filelist_set_filename (GNOME_FILELIST(filesel), fname);
}

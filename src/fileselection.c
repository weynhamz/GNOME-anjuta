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

static char *last_dir = NULL;

static gboolean on_file_selection_delete_event (GtkWidget *w, GdkEvent *event, gpointer data);
static void on_file_selection_ok_clicked (GtkButton *button, gpointer data);
static void on_file_selection_cancel_clicked (GtkButton *button, gpointer data);

void fileselection_hide_widget(GtkWidget *widget)
{
	GnomeFileList *file_list;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_FILELIST (widget));

	file_list = GNOME_FILELIST (widget);
	gtk_clist_unselect_all (GTK_CLIST (file_list->file_list));

	gnome_filelist_set_multiple_selection (file_list, FALSE);

	gtk_widget_hide (widget);
}

static gboolean
on_file_selection_delete_event (GtkWidget * w, GdkEvent * event, gpointer data)
{
	//gtk_widget_hide (w);
	fileselection_hide_widget(w);
	return TRUE;
}

static void
on_file_selection_ok_clicked (GtkButton * button, gpointer data)
{
	gchar *filename;
	gchar *file_dir;
	FileSelData *fd = data;

	filename = fileselection_get_filename (fd->filesel);
	if (!filename)
		return;
	file_dir = strrchr(filename, '/');
	if (file_dir)
	{
		*file_dir = '\0';
		if (!last_dir)
			last_dir = g_new(char, PATH_MAX);
		g_snprintf(last_dir, PATH_MAX, "%s", filename);
		*file_dir = '/';
	}
	if (file_is_directory (filename))
	{
		fileselection_set_dir (fd->filesel, filename);
		g_free (filename);
		return;
	}

	if (fd->click_ok_callback)
		fd->click_ok_callback (button, fd->data);

	fileselection_hide_widget(fd->filesel);
	
	g_free (filename);

	return;
}

static void
on_file_selection_cancel_clicked (GtkButton *button,
				  gpointer   data)
{
	FileSelData *fd = data;

	g_return_if_fail (data != NULL);

	if (fd->click_cancel_callback)
		fd->click_cancel_callback (button, fd->data);

	fileselection_hide_widget (fd->filesel);

	return;
}

GtkWidget *
create_fileselection_gui (FileSelData * fsd)
{
	GtkWidget *fileselection_gui;
	GtkWidget *fileselection_ok;
	GtkWidget *fileselection_cancel;

	if (!last_dir)
	{
		last_dir = g_new(char, PATH_MAX);
		getcwd(last_dir, PATH_MAX);
	}

	fileselection_gui = gnome_filelist_new_with_path(last_dir);
	gnome_filelist_set_title (GNOME_FILELIST(fileselection_gui), _(fsd->title));

	/* FIXME: we really need a boolean here */
	/*
	if (strcmp (fsd->title, _("Save")) && strcmp (fsd->title, _("Save As")))
		gtk_widget_hide(GNOME_FILELIST (fileselection_gui)->createdir_button);
	else
		gtk_widget_show(GNOME_FILELIST (fileselection_gui)->createdir_button);
	*/

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
			    GTK_SIGNAL_FUNC (on_file_selection_delete_event),
			    fsd);
	gtk_signal_connect (GTK_OBJECT (fileselection_ok), "clicked",
			    GTK_SIGNAL_FUNC (on_file_selection_ok_clicked),
			    fsd);
	gtk_signal_connect (GTK_OBJECT (fileselection_cancel), "clicked",
			    GTK_SIGNAL_FUNC (on_file_selection_cancel_clicked),
			    fsd);
	gtk_signal_connect (GTK_OBJECT (fileselection_gui), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_unref), NULL);

	fsd->filesel = fileselection_gui;
	gtk_widget_ref (fileselection_gui);
	gtk_window_set_transient_for(GTK_WINDOW(fileselection_gui), GTK_WINDOW(app->widgets.window));
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

GList * fileselection_get_nodelist(GtkWidget * filesel)
{
	return gnome_filelist_get_nodelist (GNOME_FILELIST(filesel));
}

/* Free the return */
gchar*
fileselection_get_path (GtkWidget* filesel)
{
	return gnome_filelist_get_path (GNOME_FILELIST(filesel));
}

gchar *
fileselection_get_lastfilename(GtkWidget * filesel, GList * list)
{
	 return gnome_filelist_get_lastfilename (GNOME_FILELIST(filesel), list);
}

void
fileselection_set_title (GtkWidget* filesel, gchar* title)
{
	return gnome_filelist_set_title (GNOME_FILELIST(filesel), title);
}

gboolean
fileselection_set_dir (GtkWidget* filesel, gchar* dir)
{
	if (!last_dir)
		last_dir = g_new(char, PATH_MAX);
	if (dir)
		g_snprintf(last_dir, PATH_MAX, dir);

	return gnome_filelist_set_dir (GNOME_FILELIST(filesel), dir);
}

gboolean
fileselection_set_filename (GtkWidget* filesel, gchar* fname)
{
	if (!last_dir)
		last_dir = g_new(char, PATH_MAX);
	if (fname)
	{
		char *slash_pos;
		g_snprintf(last_dir, PATH_MAX, fname);
		slash_pos = strrchr(last_dir, '/');
		if (slash_pos)
			*slash_pos = '\0';
	}

	return gnome_filelist_set_filename (GNOME_FILELIST(filesel), fname);
}

GtkWidget * 
fileselection_storetypes(GtkWidget* filesel, GList *filetypes)
{
	GnomeFileList *file_list=GNOME_FILELIST(filesel);
	file_list->filetypes = filetypes;
	return GTK_WIDGET(file_list);
}

GList * 
fileselection_addtype(GList *filetypes, gchar *description, GList *extentions)
{
	GList *ftypes = filetypes;	
	ftypes = gnome_filelisttype_addtype(ftypes, description, extentions);	
	return ftypes;
}

GList * 
fileselection_addtype_f(GList *filetypes, gchar *description, gint amount, ...)
{	
   GList *exts=NULL;
   gchar *ext;
   gint i=0;
   GList *ftypes = filetypes;
   va_list ap;
	  
   va_start (ap, amount);
   while ((ext = g_strdup(va_arg(ap, gchar *)))&&(i<amount)) {
	    i++;
	    exts = g_list_append(exts, ext);
   }

   va_end(ap);
	
   ftypes = fileselection_addtype(ftypes, description, exts);
   
   return ftypes;

}

GList * 
fileselection_getcombolist(GtkWidget * filesel, GList *filetypes)
{
	GList *combolist;
	combolist = gnome_filelisttype_getcombolist(filetypes);
	return combolist;
}

void 
fileselection_set_combolist(GtkWidget* filesel, GList *combolist)
{
	gnome_filelist_set_combolist(GNOME_FILELIST(filesel), combolist);
}

GtkWidget*
fileselection_clearfiletypes(GtkWidget* filesel)
{
	GnomeFileList *file_list=GNOME_FILELIST(filesel);	
	file_list->filetypes = gnome_filelisttype_clearfiletypes(file_list);	
	return GTK_WIDGET(file_list);
}

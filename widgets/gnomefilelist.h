/*  GtkFileList
*   This is my second GtkWidget
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
*  You should have received a copy of the GNU Library General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <gnome.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef GTK_FILELIST_H__
#define GTK_FILELIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#define GNOME_TYPE_FILELIST (gnome_filelist_get_type())
#define GNOME_FILELIST(obj) (GTK_CHECK_CAST((obj), GNOME_TYPE_FILELIST, GnomeFileList))
#define GNOME_FILELIST_CLASS(klass) (GTK_CHECK_CLASS_CAST((klass), GNOME_TYPE_FILELIST, GnomeFileListClass))
#define GNOME_IS_FILELIST(obj) (GTK_CHECK_TYPE((obj), GNOME_TYPE_FILELIST))
#define GNOME_IS_FILELIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), GNOME_TYPE_FILELIST))

typedef struct _GnomeFileList GnomeFileList;
typedef struct _GnomeFileListClass GnomeFileListClass;

struct _GnomeFileList {
   GtkWindow window;
   GtkWidget *back_button;
   GtkWidget *forward_button;
   GtkWidget *home_button;
   GtkWidget *delete_button;
   GtkWidget *rename_button;
   GtkWidget *history_combo;
   GtkWidget *directory_list;
   GtkWidget *file_list;
   GtkWidget *filter_combo;
   GtkWidget *selection_entry;
   GtkWidget *ok_button;
   GtkWidget *cancel_button;
   gchar *path;
   gchar *selected;
   gint history_position;
   GList *history;
   gint selected_row;
   GnomePixmap *folder;
   GnomePixmap *file;

   gchar *entry_text;
};

struct _GnomeFileListClass {
   GtkWindowClass parent_class;
};

GtkType gnome_filelist_get_type(void);

/* make a new widget */
GtkWidget* gnome_filelist_new(void);

/* start it off with a particular path */
GtkWidget *gnome_filelist_new_with_path(gchar *path);

/* get the current selection...make sure to free it with g_free() */
gchar *gnome_filelist_get_filename(GnomeFileList *file_list);
gchar *gnome_filelist_get_path(GnomeFileList *file_list);

void gnome_filelist_set_title(GnomeFileList *file_list, gchar *title);
gboolean gnome_filelist_set_dir(GnomeFileList *file_list, gchar *path);
gboolean gnome_filelist_set_filename (GnomeFileList *file_list, gchar *fname);

#ifdef __cplusplus /* cpp compatibility */
}
#endif
#endif

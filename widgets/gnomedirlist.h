/*  GtkDirList
* This is my first GtkWidget
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

#ifndef GNOME_DIRLIST_H__
#define GNOME_DIRLIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#define GNOME_TYPE_DIRLIST (gnome_dirlist_get_type())
#define GNOME_DIRLIST(obj) (GTK_CHECK_CAST ((obj), GNOME_TYPE_DIRLIST, GnomeDirList))
#define GNOME_DIRLIST_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DIRLIST, GnomeDirListClass))
#define GNOME_IS_DIRLIST(obj) (GTK_CHECK_TYPE((obj), GNOME_TYPE_DIRLIST))
#define GNOME_IS_DIRLIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DIRLIST))

typedef struct _GnomeDirList GnomeDirList;
typedef struct _GnomeDirListClass GnomeDirListClass;

struct _GnomeDirList {
   GtkWindow window;
   GtkWidget *dirs;
   GtkWidget *entry;
   GtkWidget *ok_button;
   GtkWidget *cancel_button;
   gchar *path;
   gint selected_row;
   GnomePixmap *folder_closed;
   GnomePixmap *folder_open;
};

struct _GnomeDirListClass {
   GtkWindowClass parent_class;
};

GtkType gnome_dirlist_get_type(void);
GtkWidget *gnome_dirlist_new(void);
gchar *gnome_dirlist_get_dir(GnomeDirList *dir_list);

#ifdef __cplusplus /* cpp compatibility */
}
#endif
#endif
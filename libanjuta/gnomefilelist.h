/*
 * gnomefilelist.h Copyright (C) 
 *   Chris Phelps <chicane@reninet.com>,
 *   Naba kumar <kh_naba@users.sourceforge.net>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Original author: Chris Phelps <chicane@reninet.com>
 * Widget rewrote: Naba Kumar <kh_naba@users.sourceforge.net>
 * Contributions:
 *     Philip Van Hoof <freax@pandora.be>
 *     Dan Elphick <dre00r@ecs.soton.ac.uk>
 *     Stephane Demurget  <demurgets@free.fr>
 */

#include <stdio.h>
#include <gnome.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>
#include <stdarg.h>

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

typedef struct _GnomeFileListType GnomeFileListType;
typedef struct _GnomeFileList GnomeFileList;
typedef struct _GnomeFileListClass GnomeFileListClass;

struct _GnomeFileListType {	
	gchar *description;
	GList *extentions;
	gchar *pattern;
};


struct _GnomeFileList {
   GtkDialog dialog;
   GtkWidget *back_button;
   GtkWidget *forward_button;
   GtkWidget *home_button;
   GtkWidget *delete_button;
   GtkWidget *rename_button;
   GtkWidget *show_hidden_button;
   GtkWidget *history_combo;
   GtkWidget *directory_list;
   GtkWidget *file_list;
   GtkWidget *filter_combo;
   GtkWidget *selection_label;
   GtkWidget *selection_entry;
   // GtkWidget *ok_button;
   // GtkWidget *cancel_button;
   GtkWidget *filetype_combo;
   GtkWidget *createdir_button;
   GtkWidget *scrolled_window_dir;
   GtkWidget *scrolled_window_file;

   gfloat file_scrollbar_value;
   gfloat dir_scrollbar_value;
   
   gchar *dirpattern;
   gchar *filepattern;
   gint files_matched;
   gint dirs_matched;

   gboolean show_hidden;
   gchar *path;
   gchar *selected;
   gint history_position;
   GList *history;
   GList *filetypes;
   GdkPixbuf *folder_pixbuf;
   gboolean changedironcombo;
   gchar *entry_text;
};

struct _GnomeFileListClass {
   GtkDialogClass parent_class;
};

GtkType gnome_filelist_get_type(void);

/* make a new widget */
GtkWidget* gnome_filelist_new(void);

/* start it off with a particular path */
GtkWidget *gnome_filelist_new_with_path(const gchar *path);

/* get the current selection...make sure to free it with g_free() */
gchar *gnome_filelist_get_filename(GnomeFileList *file_list);
gchar *gnome_filelist_get_path(GnomeFileList *file_list);
GList * gnome_filelist_get_filelist(GnomeFileList * file_list);
void gnome_filelist_set_title(GnomeFileList *file_list, const gchar *title);
gboolean gnome_filelist_set_dir(GnomeFileList *file_list, const gchar *path);
gboolean gnome_filelist_set_filename (GnomeFileList *file_list, const gchar *fname);
void gnome_filelist_set_show_hidden (GnomeFileList *file_list, gboolean show_hidden);
void gnome_filelist_set_selection_mode (GnomeFileList *file_list, GtkSelectionMode mode);
GList * gnome_filelisttype_makedefaultlist(GList *filetypes);
GList * gnome_filelisttype_addtype(GList *filetypes, const gchar *description, GList *extentions);
GList * gnome_filelisttype_getextentions(GList *filetypes, const gchar *description);
GList * gnome_filelisttype_getdescriptions(GList *filetypes);
GList * gnome_filelisttype_getcombolist(GList *filetypes);
GList * gnome_filelisttype_clearfiletypes(GnomeFileList *file_list);
GList * gnome_filelisttype_addtype_f(GList *filetypes, const gchar *description, ...);
GnomeFileListType * gnome_filelisttype_getfiletype(GnomeFileList *file_list, const gchar *description);
void gnome_filelist_set_combolist(GnomeFileList *file_list, GList *combolist);
#ifdef __cplusplus /* cpp compatibility */
}
#endif
#endif

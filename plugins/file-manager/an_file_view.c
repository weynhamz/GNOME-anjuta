/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_file_view.c
 * Copyright (C) 2004 Naba Kumar
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <string.h>
#include <dirent.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <gdl/gdl-icons.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include "plugin.h"

#define TOOL_TIP_TIMEOUT 1000 /* milliseconds */

enum {
	PIXBUF_COLUMN,
	FILENAME_COLUMN,
	REV_COLUMN,
	IS_DIR_COLUMN,
	COLUMNS_NB
};

typedef struct _FileFilter
{
	GList *file_match;
	GList *file_unmatch;
	GList *dir_match;
	GList *dir_unmatch;
	GList *ignore_pattern;
	gboolean ignore_hidden_files;
	gboolean ignore_hidden_dirs;
	gboolean ignore_nonrepo_files;
} FileFilter;

static GdlIcons *icon_set = NULL;
static FileFilter *ff = NULL;

/* Tooltip operations -- taken from gtodo/message_view */

static gchar *
tooltip_get_display_text (FileManagerPlugin *fv)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
	
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(fv->tree),
		fv->tooltip_rect.x, fv->tooltip_rect.y,
		&path, NULL, NULL, NULL))
	{
		gchar *text;
		
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, FILENAME_COLUMN, &text, -1); 
		gtk_tree_path_free(path);
				
		return text;
	}
	return NULL;
}

static void
tooltip_paint (GtkWidget *widget, GdkEventExpose *event, FileManagerPlugin *fv)
{
	GtkStyle *style;
	gchar *tooltiptext;

	tooltiptext = tooltip_get_display_text (fv);
	
	if (!tooltiptext)
		tooltiptext = g_strdup (_("No message details"));

	pango_layout_set_markup (fv->tooltip_layout,
							 tooltiptext,
							 strlen (tooltiptext));
	pango_layout_set_wrap(fv->tooltip_layout, PANGO_WRAP_CHAR);
	pango_layout_set_width(fv->tooltip_layout, 600000);
	style = fv->tooltip_window->style;

	gtk_paint_flat_box (style, fv->tooltip_window->window,
						GTK_STATE_NORMAL, GTK_SHADOW_OUT,
						NULL, fv->tooltip_window,
						"tooltip", 0, 0, -1, -1);

	gtk_paint_layout (style, fv->tooltip_window->window,
					  GTK_STATE_NORMAL, TRUE,
					  NULL, fv->tooltip_window,
					  "tooltip", 4, 4, fv->tooltip_layout);
	/*
	   g_object_unref(layout);
	   */
	g_free(tooltiptext);
	return;
}

static gboolean
tooltip_timeout (FileManagerPlugin *fv)
{
	gint scr_w,scr_h, w, h, x, y;
	gchar *tooltiptext;

	tooltiptext = tooltip_get_display_text (fv);
	
	if (!tooltiptext)
		tooltiptext = g_strdup (_("No file details"));
	
	fv->tooltip_window = gtk_window_new (GTK_WINDOW_POPUP);
	fv->tooltip_window->parent = fv->tree;
	gtk_widget_set_app_paintable (fv->tooltip_window, TRUE);
	gtk_window_set_resizable (GTK_WINDOW(fv->tooltip_window), FALSE);
	gtk_widget_set_name (fv->tooltip_window, "gtk-tooltips");
	g_signal_connect (G_OBJECT(fv->tooltip_window), "expose_event",
					  G_CALLBACK(tooltip_paint), fv);
	gtk_widget_ensure_style (fv->tooltip_window);

	fv->tooltip_layout =
		gtk_widget_create_pango_layout (fv->tooltip_window, NULL);
	pango_layout_set_wrap (fv->tooltip_layout, PANGO_WRAP_CHAR);
	pango_layout_set_width (fv->tooltip_layout, 600000);
	pango_layout_set_markup (fv->tooltip_layout, tooltiptext,
							 strlen (tooltiptext));
	scr_w = gdk_screen_width();
	scr_h = gdk_screen_height();
	pango_layout_get_size (fv->tooltip_layout, &w, &h);
	w = PANGO_PIXELS(w) + 8;
	h = PANGO_PIXELS(h) + 8;

	gdk_window_get_pointer (NULL, &x, &y, NULL);
	if (GTK_WIDGET_NO_WINDOW (fv->tree))
		y += fv->tree->allocation.y;

	x -= ((w >> 1) + 4);

	if ((x + w) > scr_w)
		x -= (x + w) - scr_w;
	else if (x < 0)
		x = 0;

	if ((y + h + 4) > scr_h)
		y = y - h;
	else
		y = y + 6;
	/*
	   g_object_unref(layout);
	   */
	gtk_widget_set_size_request (fv->tooltip_window, w, h);
	gtk_window_move (GTK_WINDOW (fv->tooltip_window), x, y);
	gtk_widget_show (fv->tooltip_window);
	g_free (tooltiptext);
	
	return FALSE;
}

static gboolean
tooltip_motion_cb (GtkWidget *tv, GdkEventMotion *event, FileManagerPlugin *fv)
{
	GtkTreePath *path;
	
	if (fv->tooltip_rect.y == 0 &&
		fv->tooltip_rect.height == 0 &&
		fv->tooltip_timeout)
	{
		g_source_remove (fv->tooltip_timeout);
		fv->tooltip_timeout = 0;
		if (fv->tooltip_window) {
			gtk_widget_destroy (fv->tooltip_window);
			fv->tooltip_window = NULL;
		}
		return FALSE;
	}
	if (fv->tooltip_timeout) {
		if (((int)event->y > fv->tooltip_rect.y) &&
			(((int)event->y - fv->tooltip_rect.height)
				< fv->tooltip_rect.y))
			return FALSE;

		if(event->y == 0)
		{
			g_source_remove (fv->tooltip_timeout);
			fv->tooltip_timeout = 0;
			return FALSE;
		}
		/* We've left the cell.  Remove the timeout and create a new one below */
		if (fv->tooltip_window) {
			gtk_widget_destroy (fv->tooltip_window);
			fv->tooltip_window = NULL;
		}
		g_source_remove (fv->tooltip_timeout);
		fv->tooltip_timeout = 0;
	}

	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(fv->tree),
									   event->x, event->y, &path,
									   NULL, NULL, NULL))
	{
		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (fv->tree),
									 path, NULL, &fv->tooltip_rect);
		
		if (fv->tooltip_rect.y != 0 &&
			fv->tooltip_rect.height != 0)
		{
			gchar *tooltiptext;
			
			tooltiptext = tooltip_get_display_text (fv);
			if (tooltiptext == NULL)
				return FALSE;
			g_free (tooltiptext);
			
			fv->tooltip_timeout =
				g_timeout_add (TOOL_TIP_TIMEOUT,
							   (GSourceFunc) tooltip_timeout, fv);
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}

static void
tooltip_leave_cb (GtkWidget *w, GdkEventCrossing *e, FileManagerPlugin *fv)
{
	if (fv->tooltip_timeout) {
		g_source_remove (fv->tooltip_timeout);
		fv->tooltip_timeout = 0;
	}
	if (fv->tooltip_window) {
		gtk_widget_destroy (fv->tooltip_window);
		g_object_unref (fv->tooltip_layout);
		fv->tooltip_window = NULL;
	}
}

static gboolean
anjuta_fv_open_file (FileManagerPlugin * fv, const char *path)
{
	gchar *uri;
	GObject *obj;
	
	IAnjutaFileLoader *loader;
	g_return_val_if_fail (path != NULL, FALSE);
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (fv)->shell,
										 IAnjutaFileLoader, NULL);
	uri = gnome_vfs_get_uri_from_local_path (path);
	obj = ianjuta_file_loader_load (loader, uri, FALSE, NULL);
	g_free (uri);
	return (obj == NULL)? FALSE:TRUE;
}

/* File filters prefs */
#define FILE_FILTER_MATCH "filter.file.match"
#define FILE_FILTER_MATCH_COMBO "filter.file.match.combo"
#define FILE_FILTER_UNMATCH "filter.file.unmatch"
#define FILE_FILTER_UNMATCH_COMBO "filter.file.unmatch.combo"
#define FILE_FILTER_IGNORE_HIDDEN "filter.file.ignore.hidden"
#define DIR_FILTER_MATCH "filter.dir.match"
#define DIR_FILTER_MATCH_COMBO "filter.dir.match.combo"
#define DIR_FILTER_UNMATCH "filter.dir.unmatch"
#define DIR_FILTER_UNMATCH_COMBO "filter.dir.unmatch.combo"
#define DIR_FILTER_IGNORE_HIDDEN "filter.dir.ignore.hidden"
#define FILE_FILTER_IGNORE_NONREPO "filter.file.ignore.nonrepo"

#define DIR_FILTER_IGNORE_PATTERN "filter.file.ignore.pattern"

#define GET_PREF(var, P) \
	if (ff->var) \
		anjuta_util_glist_strings_free(ff->var); \
	ff->var = NULL; \
	s = anjuta_preferences_get (fv->prefs, P); \
	if (s) { \
		ff->var = anjuta_util_glist_from_string(s); \
		g_free(s); \
	}

#define GET_PREF_BOOL(var, P) \
	ff->var = FALSE; \
	ff->var = anjuta_preferences_get_int (fv->prefs, P);
	
static FileFilter*
fv_prefs_new (FileManagerPlugin *fv)
{
	gchar *s;
	FileFilter *ff = g_new0(FileFilter, 1);
	GET_PREF(file_match, FILE_FILTER_MATCH);
	GET_PREF(file_unmatch, FILE_FILTER_UNMATCH);
	GET_PREF_BOOL(ignore_hidden_files, FILE_FILTER_IGNORE_HIDDEN);
	GET_PREF(dir_match, DIR_FILTER_MATCH);
	GET_PREF(dir_unmatch, DIR_FILTER_UNMATCH);
	GET_PREF_BOOL(ignore_hidden_dirs, DIR_FILTER_IGNORE_HIDDEN);
	GET_PREF_BOOL(ignore_nonrepo_files, FILE_FILTER_IGNORE_NONREPO);

	GET_PREF(ignore_pattern, DIR_FILTER_IGNORE_PATTERN);

	return ff;
}

static void
fv_prefs_free (FileFilter *ff)
{
	g_return_if_fail (ff != NULL);
	if (ff->file_match)
		anjuta_util_glist_strings_free (ff->file_match);
	if (ff->file_unmatch)
		anjuta_util_glist_strings_free (ff->file_unmatch);
	if (ff->dir_match)
		anjuta_util_glist_strings_free (ff->dir_match);
	if (ff->dir_unmatch)
		anjuta_util_glist_strings_free (ff->dir_unmatch);
	if (ff->ignore_pattern)
		anjuta_util_glist_strings_free (ff->ignore_pattern);
	g_free (ff);
	ff = NULL;
}

static gchar *
fv_construct_full_path (FileManagerPlugin *fv, GtkTreeIter *selected_iter)
{
	gchar *path, *dir;
	GtkTreeModel *model;
	GtkTreeIter iter, parent;
	gchar *full_path = NULL;
	GtkTreeView *view = GTK_TREE_VIEW (fv->tree);

	parent = *selected_iter;
	model = gtk_tree_view_get_model (view);
	do
	{
		const gchar *filename;
		iter = parent;
		gtk_tree_model_get (model, &iter, FILENAME_COLUMN, &filename, -1);
		path = g_build_filename (filename, full_path, NULL);
		g_free (full_path);
		full_path = path;
	}
	while (gtk_tree_model_iter_parent (model, &parent, &iter));
	dir = g_path_get_dirname (fv->top_dir);
	path = g_build_filename (dir, full_path, NULL);
	g_free (full_path);
	g_free (dir);
	return path;
}

gchar *
fv_get_selected_file_path (FileManagerPlugin *fv)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeView *view;
	
	view = GTK_TREE_VIEW (fv->tree);
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return NULL;
	return fv_construct_full_path (fv, &iter);
}

static void
on_treeview_row_activated (GtkTreeView *view,
						   GtkTreePath *arg1,
						   GtkTreeViewColumn *arg2,
                           FileManagerPlugin *fv)
{
	gchar *path;

	path = fv_get_selected_file_path (fv);
	if (path)
		anjuta_fv_open_file (fv, path);
	g_free (path);
}

static gboolean
on_tree_view_event  (GtkWidget *widget,
					 GdkEvent  *event,
					 FileManagerPlugin *fv)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	const gchar *version;

	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	if (!event)
		return FALSE;

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, REV_COLUMN, &version, -1);

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;
		if (e->button == 3) {
			GtkWidget *popup;
			popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (fv->ui),
											   "/PopupFileManager");
			g_return_val_if_fail (GTK_IS_WIDGET (popup), TRUE);
			gtk_menu_popup (GTK_MENU (popup),
					NULL, NULL, NULL, NULL,
					((GdkEventButton *) event)->button,
					((GdkEventButton *) event)->time);
		}
	} else if (event->type == GDK_KEY_PRESS) {
		GdkEventKey *e = (GdkEventKey *) event;

		switch (e->keyval) {
			case GDK_Return:
				if (!gtk_tree_model_iter_has_child (model, &iter))
				{
					gchar *path = fv_get_selected_file_path(fv);
					if (path && !g_file_test (path, G_FILE_TEST_IS_DIR))
						anjuta_fv_open_file (fv, path);
					g_free (path);
					return TRUE;
				}
			default:
				return FALSE;
		}
	}

	return FALSE;
}

static void
fv_disconnect (FileManagerPlugin *fv)
{
	g_return_if_fail (fv != NULL);
	g_signal_handlers_block_by_func (fv->tree,
									 G_CALLBACK (on_tree_view_event),
									 NULL);
}

static void
fv_connect (FileManagerPlugin *fv)
{
	g_return_if_fail (fv != NULL && fv->tree);
	g_signal_handlers_unblock_by_func (fv->tree,
									   G_CALLBACK (on_tree_view_event),
									   NULL);
}

static gboolean
file_entry_apply_filter (const char *name, GList *match, GList *unmatch,
						 gboolean ignore_hidden)
{
	GList *tmp;
	gboolean matched = (match == NULL);
	g_return_val_if_fail(name, FALSE);
	if (ignore_hidden && ('.' == name[0]))
		return FALSE;
	for (tmp = match; tmp; tmp = g_list_next(tmp))
	{
		if (0 == fnmatch((char *) tmp->data, name, 0))
		{
			matched = TRUE;
			break;
		}
	}
	if (!matched)
		return FALSE;
	for (tmp = unmatch; tmp; tmp = g_list_next(tmp))
	{
		if (0 == fnmatch((char *) tmp->data, name, 0))
		{
			return FALSE;
		}
	}
	return matched;	
}

static void
fv_add_tree_entry (FileManagerPlugin *fv, const gchar *path, GtkTreeIter *root)
{
	GtkTreeStore *store;
	gchar file_name[PATH_MAX];
	struct stat s;
	DIR *dir;
	struct dirent *dir_entry;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	GList *file_node;
	GList *files = NULL;
	GList *directories = NULL;
	GList *ignore_files = NULL;
	gchar *entries = NULL;
	GtkTreeViewColumn* column;

	g_return_if_fail (path != NULL);

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree)));

	g_snprintf(file_name, PATH_MAX, "%s/CVS/Entries", path);
	if (0 == stat(file_name, &s))
	{
		if (S_ISREG(s.st_mode))
		{
			int fd;
			entries = g_new(char, s.st_size + 2);
			if (0 > (fd = open(file_name, O_RDONLY)))
			{
				g_free(entries);
				entries = NULL;
				
			}
			else
			{
				off_t n =0;
				off_t total_read = 1;
				while (0 < (n = read(fd, entries + total_read, s.st_size - total_read)))
					total_read += n;
				entries[s.st_size] = '\0';
				entries[0] = '\n';
				close(fd);
			}
		}
	}
	column = gtk_tree_view_get_column(GTK_TREE_VIEW(fv->tree), 1);
	gtk_tree_view_column_set_visible(column, entries != NULL);
															 
	
	g_snprintf(file_name, PATH_MAX, "%s/.cvsignore", path);
	if (ff->ignore_nonrepo_files && 0 == stat(file_name, &s))
	{
		if (S_ISREG(s.st_mode))
		{
			int fd;
			if ((fd = open(file_name, O_RDONLY)) >= 0)
			{
				off_t n = 0;
				off_t total_read = 0;
				gchar **strv = NULL;
				gchar **ptrv = NULL;
				gchar *content = g_new (char, s.st_size + 2);
				
				while (0 < (n = read(fd, content + total_read,
									 s.st_size - total_read)))
					total_read += n;
				
				content[s.st_size] = '\0';
				close(fd);
				
				strv = g_strsplit (content, "\n", -1);
				ptrv = strv;
				while (strv && *ptrv)
				{
					ignore_files = g_list_prepend (ignore_files,
												   g_strdup (*ptrv));
					/* DEBUG_PRINT ("Ignoring: %s", *ptrv); */
					ptrv++;
				}
				g_strfreev (strv);
				g_free (content);
			}
		}
	}

	/* add ignore pattern */
	{
		GList *li;

		li = ff->ignore_pattern;
		while (li != NULL)
		{
			ignore_files = g_list_prepend (ignore_files, g_strdup (li->data));
			DEBUG_PRINT ("Ignoring: %s", (gchar*)li->data);
			li = g_list_next (li);
		}
	}

	if (NULL != (dir = opendir(path)))
	{
		while (NULL != (dir_entry = readdir(dir)))
		{
			const gchar *file;
			
			file = dir_entry->d_name;
			
			if ((ff->ignore_hidden_files && *file == '.') ||
				(0 == strcmp(file, ".")) ||
				(0 == strcmp(file, "..")))
				continue;
			if (ignore_files &&
				!file_entry_apply_filter (file, NULL, ignore_files,
										  FALSE))
				continue;
			
			g_snprintf(file_name, PATH_MAX, "%s/%s", path, file);
			
			if (g_file_test (file_name, G_FILE_TEST_IS_SYMLINK))
				continue;
			
			if (g_file_test (file_name, G_FILE_TEST_IS_DIR))
			{
				directories = g_list_prepend (directories, g_strdup (file));
			} else {
				files = g_list_prepend (files, g_strdup (file));
			}
		}
		closedir(dir);
		
		/* Enter directories */
		directories = anjuta_util_glist_strings_sort (directories);
		file_node = directories;
		while (file_node)
		{
			GtkTreeIter sub_iter;
			gchar *dname = file_node->data;
			/*if (!file_entry_apply_filter (file_name, ff->dir_match,
			ff->dir_unmatch,
			ff->ignore_hidden_dirs))
			continue;
			*/
			pixbuf = gdl_icons_get_mime_icon (icon_set,
											  "application/directory-normal");
			gtk_tree_store_append (store, &iter, root);
			gtk_tree_store_set (store, &iter,
								PIXBUF_COLUMN, pixbuf,
								FILENAME_COLUMN, dname,
								REV_COLUMN, "",
								IS_DIR_COLUMN, 1,
								-1);
			g_object_unref (pixbuf);
			gtk_tree_store_append (store, &sub_iter, &iter);
			gtk_tree_store_set (store, &sub_iter,
								FILENAME_COLUMN, _("Loading..."),
								REV_COLUMN, "",
								-1);
			g_free (dname);
			file_node = g_list_next (file_node);
		}
		
		/* Enter files */
		files = anjuta_util_glist_strings_sort (files);
		file_node = files;
		while (file_node)
		{
			gchar *version = NULL;
			gchar *fname = file_node->data;
			g_snprintf(file_name, PATH_MAX, "%s/%s", path, fname);
			/* if (!file_entry_apply_filter (fname, ff->file_match,
										  ff->file_unmatch,
										  ff->ignore_hidden_files))
				continue;
			*/
			if (entries)
			{
				char *str = g_strconcat("\n/", fname, "/", NULL);
				char *name_pos = strstr(entries, str);
				if (NULL != name_pos)
				{
					int len = strlen(str);
					char *version_pos = strchr(name_pos + len, '/');
					if (NULL != version_pos)
					{
						*version_pos = '\0';
						version = g_strdup(name_pos + len);
						*version_pos = '/';
					}
				}
				g_free(str);
			}
			pixbuf = gdl_icons_get_uri_icon(icon_set, file_name);
			gtk_tree_store_append (store, &iter, root);
			gtk_tree_store_set (store, &iter,
						PIXBUF_COLUMN, pixbuf,
						FILENAME_COLUMN, fname,
						REV_COLUMN, version ? version : "",
						-1);
			gdk_pixbuf_unref (pixbuf);
			g_free (version);
			g_free (fname);
			file_node = g_list_next (file_node);
		}
		g_list_free (files);
		g_list_free (directories);
	}
	if (entries)
		g_free (entries);
	if (ignore_files)
	{
		g_list_foreach (ignore_files, (GFunc)g_free, NULL);
		g_list_free (ignore_files);
	}
}

static void
on_file_view_row_expanded (GtkTreeView *view,
						   GtkTreeIter *iter,
						   GtkTreePath *iter_path,
						   FileManagerPlugin *fv)
{
	GdkPixbuf *pix;
	gchar *full_path;
	GtkTreeIter child;
	GList *row_refs, *row_ref_node;
	GtkTreeRowReference *row_ref;
	GtkTreePath *path;
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (fv)->shell, NULL);
	anjuta_status_busy_push (status);
	
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));

	/* Deleting multiple rows at one go is little tricky. We need to
	   take row references before they are deleted
	*/
	row_refs = NULL;
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
	{
		/* Get row references */
		do
		{
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
			row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
			row_refs = g_list_prepend (row_refs, row_ref);
			gtk_tree_path_free (path);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &child));
	}

	/* Update with new info */
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &child, iter_path);
	full_path = fv_construct_full_path (fv, &child);
	fv_add_tree_entry (fv, full_path, &child);
	g_free (full_path);
	
	/* Update folder icon */
	pix = gdl_icons_get_mime_icon (icon_set, "application/directory-normal");
	gtk_tree_store_set (store, &child, PIXBUF_COLUMN, pix, -1);
	g_object_unref (pix);
	
	/* Delete the referenced rows */
	row_ref_node = row_refs;
	while (row_ref_node)
	{
		row_ref = row_ref_node->data;
		path = gtk_tree_row_reference_get_path (row_ref);
		g_assert (path != NULL);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &child, path);
		gtk_tree_store_remove (store, &child);
		gtk_tree_path_free (path);
		gtk_tree_row_reference_free (row_ref);
		row_ref_node = g_list_next (row_ref_node);
	}
	if (row_refs)
	{
		g_list_free (row_refs);
	}
	anjuta_status_busy_pop (status);
}

static void
on_file_view_row_collapsed (GtkTreeView *view,
						   GtkTreeIter *iter,
						   GtkTreePath *iter_path,
						   FileManagerPlugin *fv)
{
	GdkPixbuf *pix;
	GtkTreeIter child, child2;
	GtkTreeStore *store;
	GList *row_refs, *row_ref_node;
	GtkTreeRowReference *row_ref;
	GtkTreePath *path;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	
	/* Deleting multiple rows at one go is little tricky. We need to
	   take row references before they are deleted
	*/
	row_refs = NULL;
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
	{
		/* Get row references */
		do {
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
			row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
			row_refs = g_list_prepend (row_refs, row_ref);
			gtk_tree_path_free (path);
		}
		while (gtk_tree_model_iter_next  (GTK_TREE_MODEL (store), &child));
	}
	
	/* Update folder icon */
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &child, iter_path);
	pix = gdl_icons_get_mime_icon (icon_set, "application/directory-normal");
	gtk_tree_store_set (store, &child, PIXBUF_COLUMN, pix, -1);
	g_object_unref (pix);

	/* Add dummy child */
	gtk_tree_store_append (store, &child2, &child);
	gtk_tree_store_set (store, &child2,
						PIXBUF_COLUMN, NULL,
						FILENAME_COLUMN, _("Loading..."),
						REV_COLUMN, "", -1);

	/* Delete the referenced rows */
	row_ref_node = row_refs;
	while (row_ref_node)
	{
		row_ref = row_ref_node->data;
		path = gtk_tree_row_reference_get_path (row_ref);
		g_assert (path != NULL);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &child, path);
		gtk_tree_store_remove (store, &child);
		gtk_tree_path_free (path);
		gtk_tree_row_reference_free (row_ref);
		row_ref_node = g_list_next (row_ref_node);
	}
	if (row_refs)
	{
		g_list_free (row_refs);
	}
}

static void
on_tree_view_selection_changed (GtkTreeSelection *sel, FileManagerPlugin *fv)
{
	gchar *filename, *uri;
	GValue *value;
	
	filename = fv_get_selected_file_path (fv);
	if (filename)
	{
		uri = gnome_vfs_get_uri_from_local_path (filename);
		g_free (filename);
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);
		g_value_take_string (value, uri);
		anjuta_shell_add_value (ANJUTA_PLUGIN(fv)->shell,
								"file_manager_current_uri",
								value, NULL);
	} else {
		anjuta_shell_remove_value (ANJUTA_PLUGIN(fv)->shell,
								   "file_manager_current_uri", NULL);
	}
}

void
fv_init (FileManagerPlugin *fv)
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	/* Scrolled window */
	fv->scrolledwindow = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (fv->scrolledwindow),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (fv->scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (fv->scrolledwindow),
										 GTK_SHADOW_IN);
	/* Tree and his model */
	store = gtk_tree_store_new (COLUMNS_NB,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_BOOLEAN);
	fv->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (fv->tree), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (fv->scrolledwindow), fv->tree);
	g_signal_connect (fv->tree, "row_expanded",
					  G_CALLBACK (on_file_view_row_expanded), fv);
	g_signal_connect (fv->tree, "row_collapsed",
					  G_CALLBACK (on_file_view_row_collapsed), fv);
	g_signal_connect (fv->tree, "event-after",
					  G_CALLBACK (on_tree_view_event), fv);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_tree_view_selection_changed), fv);
	g_signal_connect (fv->tree, "row_activated",
					  G_CALLBACK (on_treeview_row_activated), fv);
	/* Tooltip signals */
	g_signal_connect (G_OBJECT (fv->tree), "motion-notify-event",
					  G_CALLBACK (tooltip_motion_cb), fv);
	g_signal_connect (G_OBJECT (fv->tree), "leave-notify-event",
					  G_CALLBACK (tooltip_leave_cb), fv);
	gtk_widget_show (fv->tree);

	g_object_unref (G_OBJECT (store));

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("File"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", FILENAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (fv->tree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (fv->tree), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Rev"), renderer,
							   "text", REV_COLUMN,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (fv->tree), column);

	g_object_ref (G_OBJECT (fv->tree));
	g_object_ref (G_OBJECT (fv->scrolledwindow));
}

void
fv_finalize (FileManagerPlugin *fv)
{
	if (fv->tooltip_timeout)
		g_source_remove (fv->tooltip_timeout);
	fv->tooltip_timeout = 0;
	
	if (fv->top_dir)
		g_free (fv->top_dir);
	g_object_unref (G_OBJECT (fv->tree));
	g_object_unref (G_OBJECT (fv->scrolledwindow));
	
	/* Object will be destroyed when removed from container */
	/* gtk_widget_destroy (fv->scrolledwindow); */
	fv_cancel_node_expansion (fv);
	fv->top_dir = NULL;
	fv->tree = NULL;
	fv->scrolledwindow = NULL;
	if (ff != NULL)
		fv_prefs_free (ff);
	ff = NULL;
}

void
fv_clear (FileManagerPlugin *fv)
{
	GtkTreeModel *model;

	g_return_if_fail (fv != NULL && fv->tree);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

static void
mapping_function (GtkTreeView *treeview, GtkTreePath *path, gpointer data)
{
	gchar *str;
	GList *map = * ((GList **) data);
	
	str = gtk_tree_path_to_string (path);
	map = g_list_append (map, str);
	* ((GList **) data) = map;
};

GList *
fv_get_node_expansion_states (FileManagerPlugin *fv)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (fv->tree),
									 mapping_function, &map);
	return map;
}

static gboolean
on_fv_node_expansion_on_idle (gpointer user_data)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	FileManagerPlugin *fv = ANJUTA_PLUGIN_FILE_MANAGER (user_data);
	gchar *node_path = (gchar*)fv->nodes_to_expand->data;
	
	fv->nodes_to_expand = g_list_remove (fv->nodes_to_expand,
											   node_path);
	/* Expand node_path */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
	path = gtk_tree_path_new_from_string (node_path);
	DEBUG_PRINT ("Expanding node: %s", node_path);
	gtk_tree_view_expand_row (GTK_TREE_VIEW (fv->tree), path, FALSE);
	
	/* Clean up */
	gtk_tree_path_free (path);
	g_free (node_path);
	
	/* End of this cycle */
	if (fv->nodes_to_expand == NULL)
	{
		fv->idle_id = 0;
		return FALSE; /* End */
	}
	else
		return TRUE; /* Continue */
}

static void
fv_queue_node_expansion (FileManagerPlugin *fv, const gchar *node_path)
{
	fv->nodes_to_expand = g_list_append (fv->nodes_to_expand,
										 g_strdup (node_path));
	if (fv->idle_id <= 0)
		fv->idle_id = g_idle_add_full (G_PRIORITY_LOW,
									   on_fv_node_expansion_on_idle,
									   fv, NULL);
}

void
fv_set_node_expansion_states (FileManagerPlugin *fv, GList *expansion_states)
{
	/* Queue expanded nodes */	
	if (expansion_states)
	{
		GList *node;
		node = expansion_states;
		
		while (node)
		{
			fv_queue_node_expansion (fv, (gchar *)node->data);
			node = g_list_next (node);
		}
	}
}

void
fv_cancel_node_expansion (FileManagerPlugin *fv)
{
	if (fv->idle_id)
		g_source_remove (fv->idle_id);
	fv->idle_id = 0;
	if (fv->nodes_to_expand)
	{
		g_list_foreach (fv->nodes_to_expand, (GFunc)g_free, NULL);
		g_list_free (fv->nodes_to_expand);
		fv->nodes_to_expand = NULL;
	}
}

void
fv_set_root (FileManagerPlugin *fv, const gchar *root_dir)
{
	if (!fv->top_dir || 0 != strcmp(fv->top_dir, root_dir))
	{
		/* Different directory*/
		if (fv->top_dir)
			g_free(fv->top_dir);
		fv->top_dir = g_strdup(root_dir);
		fv_refresh(fv, FALSE);
	}
}

void
fv_refresh (FileManagerPlugin *fv, gboolean save_states)
{
	static gboolean busy = FALSE;
	GList *selected_items = NULL;
	GtkTreeIter sub_iter;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeStore *store;
	GdkPixbuf *pixbuf;
	gchar *project_dir;
	gchar *root_node_path;
	
	if (busy)
		return;
	else
		busy = TRUE;
	
	/* Make sure node expansion is stoped */
	fv_cancel_node_expansion (fv);
	
	if (icon_set == NULL)
		icon_set = gdl_icons_new (16);
	if (ff != NULL)
		fv_prefs_free (ff);
	ff = fv_prefs_new (fv);
	
	fv_disconnect (fv);

	if (save_states)
		selected_items = fv_get_node_expansion_states (fv);

	fv_clear (fv);

	project_dir = g_path_get_basename (fv->top_dir);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (fv->tree));
	store = GTK_TREE_STORE (model);

	pixbuf = gdl_icons_get_mime_icon (icon_set, "application/directory-normal");
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				PIXBUF_COLUMN, pixbuf,
				FILENAME_COLUMN, project_dir,
				REV_COLUMN, "",
				-1);
	g_object_unref (pixbuf);
	g_free (project_dir);
	
	gtk_tree_store_append (store, &sub_iter, &iter);
	gtk_tree_store_set (store, &sub_iter,
				PIXBUF_COLUMN, NULL,
				FILENAME_COLUMN, _("Loading..."),
				REV_COLUMN, "",
				-1);
	if (save_states)
	{
		fv_set_node_expansion_states (fv, selected_items);
	}
	else
	{
		/* Expand first node */
		gtk_tree_model_get_iter_first (model, &iter);
		path = gtk_tree_model_get_path (model, &iter);
		root_node_path = gtk_tree_path_to_string (path);
		fv_queue_node_expansion (fv, root_node_path);
		gtk_tree_path_free (path);
		g_free (root_node_path);
	}
	if (selected_items)
		anjuta_util_glist_strings_free (selected_items);
	fv_connect (fv);
	busy = FALSE;
	return;
}

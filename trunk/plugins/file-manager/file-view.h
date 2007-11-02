/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * file-manager
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * file-manager is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * file-manager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with file-manager.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _FILE_VIEW_H_
#define _FILE_VIEW_H_

#include <gtk/gtktreeview.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_FILE_VIEW             (file_view_get_type ())
#define ANJUTA_FILE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_FILE_VIEW, AnjutaFileView))
#define ANJUTA_FILE_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_FILE_VIEW, AnjutaFileViewClass))
#define ANJUTA_IS_FILE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_FILE_VIEW))
#define ANJUTA_IS_FILE_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_FILE_VIEW))
#define ANJUTA_FILE_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_FILE_VIEW, AnjutaFileViewClass))

typedef struct _AnjutaFileViewClass AnjutaFileViewClass;
typedef struct _AnjutaFileView AnjutaFileView;

struct _AnjutaFileViewClass
{
	GtkTreeViewClass parent_class;
	
	/* Signals */
	void (*file_open) (AnjutaFileView* view,
					   const gchar* uri);
	void (*show_popup_menu) (AnjutaFileView* view,
							 const gchar* uri,
							 gboolean is_dir,
							 guint button,
							 guint32 time);
	void (*current_uri_changed) (AnjutaFileView* view,
								 const gchar* uri);
};

struct _AnjutaFileView
{
	GtkTreeView parent_instance;
};

GType file_view_get_type (void) G_GNUC_CONST;
GtkWidget* file_view_new (void);

void
file_view_refresh(AnjutaFileView* view,
				  gboolean remember_open);

gchar*
file_view_get_selected (AnjutaFileView* view);

G_END_DECLS

#endif /* _FILE_VIEW_H_ */

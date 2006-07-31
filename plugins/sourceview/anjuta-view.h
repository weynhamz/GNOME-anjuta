/*
 * anjuta-view.h
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2006 Johannes Schmid
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the anjuta Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __ANJUTA_VIEW_H__
#define __ANJUTA_VIEW_H__

#include <gtk/gtk.h>

#include "anjuta-document.h"
#include "tag-window.h"
#include <gtksourceview/gtksourceview.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ANJUTA_TYPE_VIEW            (anjuta_view_get_type ())
#define ANJUTA_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_VIEW, AnjutaView))
#define ANJUTA_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), ANJUTA_TYPE_VIEW, AnjutaViewClass))
#define ANJUTA_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), ANJUTA_TYPE_VIEW))
#define ANJUTA_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_VIEW))
#define ANJUTA_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_VIEW, AnjutaViewClass))

/* Private structure type */
typedef struct _AnjutaViewPrivate	AnjutaViewPrivate;

/*
 * Main object structure
 */
typedef struct _AnjutaView		AnjutaView;
 
struct _AnjutaView
{
	GtkSourceView view;
	
	/*< private > */
	AnjutaViewPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _AnjutaViewClass		AnjutaViewClass;
 
struct _AnjutaViewClass
{
	GtkSourceViewClass parent_class;
	
	void (* char_added)  		(AnjutaDocument    *document,
								 gint position,
								 gchar character);
};

/*
 * Public methods
 */
GtkType		 anjuta_view_get_type     	(void) G_GNUC_CONST;

GtkWidget	*anjuta_view_new			(AnjutaDocument   *doc);

void		 anjuta_view_cut_clipboard 	(AnjutaView       *view);
void		 anjuta_view_copy_clipboard 	(AnjutaView       *view);
void		 anjuta_view_paste_clipboard	(AnjutaView       *view);
void		 anjuta_view_delete_selection	(AnjutaView       *view);
void		 anjuta_view_select_all		(AnjutaView       *view);

void		 anjuta_view_scroll_to_cursor 	(AnjutaView       *view);

void 		 anjuta_view_set_colors 		(AnjutaView       *view, 
						 gboolean         def,
						 GdkColor        *backgroud, 
						 GdkColor        *text,
						 GdkColor        *selection, 
						 GdkColor        *sel_text);

void 		 anjuta_view_set_font		(AnjutaView       *view,
						 gboolean         def,
						 const gchar     *font_name);

void 		 anjuta_view_register_completion(AnjutaView* view, TagWindow* tagwin);

G_END_DECLS

#endif /* __ANJUTA_VIEW_H__ */

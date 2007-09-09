/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta-trunk.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _ASSIST_TIP_H_
#define _ASSIST_TIP_H_

#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktextview.h>

G_BEGIN_DECLS

#define ASSIST_TYPE_TIP             (assist_tip_get_type ())
#define ASSIST_TIP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASSIST_TYPE_TIP, AssistTip))
#define ASSIST_TIP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ASSIST_TYPE_TIP, AssistTipClass))
#define ASSIST_IS_TIP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASSIST_TYPE_TIP))
#define ASSIST_IS_TIP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ASSIST_TYPE_TIP))
#define ASSIST_TIP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ASSIST_TYPE_TIP, AssistTipClass))

typedef struct _AssistTipClass AssistTipClass;
typedef struct _AssistTip AssistTip;

struct _AssistTipClass
{
	GtkWindowClass parent_class;
};

struct _AssistTip
{
	GtkWindow parent_instance;
	
	GtkWidget* label;
	gint position;
};

GType assist_tip_get_type (void) G_GNUC_CONST;
GtkWidget* assist_tip_new (GtkTextView* view, GList* tips);

gint assist_tip_get_position (AssistTip* tip);

G_END_DECLS

#endif /* _ASSIST_TIP_H_ */

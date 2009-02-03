/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef GLADE_PLUGIN_H
#define GLADE_PLUGIN_H

#include <libanjuta/anjuta-plugin.h>

#include "config.h"

#include <gladeui/glade.h>
#include <gladeui/glade-design-view.h>

extern GType glade_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_GLADE         (glade_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_GLADE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_GLADE, GladePlugin))
#define ANJUTA_PLUGIN_GLADE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_GLADE, GladePluginClass))
#define ANJUTA_IS_PLUGIN_GLADE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_GLADE))
#define ANJUTA_IS_PLUGIN_GLADE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_GLADE))
#define ANJUTA_PLUGIN_GLADE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_GLADE, GladePluginClass))

typedef struct _GladePlugin GladePlugin;
typedef struct _GladePluginPriv GladePluginPriv;
typedef struct _GladePluginClass GladePluginClass;

struct _GladePlugin{
	AnjutaPlugin parent;
	GladePluginPriv *priv;
};

struct _GladePluginClass{
	AnjutaPluginClass parent_class;
};

void on_copy_activated (GtkAction *action, GladePlugin *plugin);
void on_cut_activated (GtkAction *action, GladePlugin *plugin);
void on_paste_activated (GtkAction *action, GladePlugin *plugin);
void on_undo_activated (GtkAction *action, GladePlugin *plugin);
void on_redo_activated (GtkAction *action, GladePlugin *plugin);
void on_delete_activated (GtkAction *action, GladePlugin *plugin);

gboolean glade_can_undo(GladePlugin *plugin);
gboolean glade_can_redo(GladePlugin *plugin);

gchar* glade_get_filename(GladePlugin *plugin);

/* GLADE_SIGNAL_EDITOR_EXT means extensible glade signal editor
 * GLADE_LAYOUT_WIDGET_EVENTS means availability of GladeDesignLayout signal for widget events
 */

#define GLADE_LAYOUT_WIDGET_EVENTS 1
#define GLADE_SIGNAL_EDITOR_EXT 1

#ifndef GLADE_SIGNAL_EDITOR_EXT
enum
{
	GSE_COLUMN_SIGNAL,
	GSE_COLUMN_HANDLER,
	GSE_COLUMN_AFTER,
	GSE_COLUMN_USERDATA,
	GSE_COLUMN_LOOKUP,

	GSE_COLUMN_USERDATA_SLOT,
	GSE_COLUMN_LOOKUP_VISIBLE,
	GSE_COLUMN_AFTER_VISIBLE,
	GSE_COLUMN_HANDLER_EDITABLE,
	GSE_COLUMN_USERDATA_EDITABLE,
	GSE_COLUMN_SLOT,
	GSE_COLUMN_BOLD,
	GSE_NUM_COLUMNS
};
#endif


#endif

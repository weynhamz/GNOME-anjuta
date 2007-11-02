/*	 plugin.h	(C) 2005 Massimo Cora'
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#ifndef _PATCH_PLUGIN_H_
#define _PATCH_PLUGIN_H_

#include <config.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

#define ICON_FILE "anjuta-patch-plugin.png"

extern GType patch_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_PATCH         (patch_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_PATCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_PATCH, PatchPlugin))
#define ANJUTA_PLUGIN_PATCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_PATCH, PatchPluginClass))
#define ANJUTA_IS_PLUGIN_PATCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_PATCH))
#define ANJUTA_IS_PLUGIN_PATCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_PATCH))
#define ANJUTA_PLUGIN_PATCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_PATCH, PatchPluginClass))

typedef struct _PatchPlugin PatchPlugin;
typedef struct _PatchPluginClass PatchPluginClass;


struct _PatchPlugin {
	AnjutaPlugin parent;
	AnjutaLauncher *launcher;
	
	IAnjutaMessageView* mesg_view;
	GtkWidget* file_chooser;
	GtkWidget* patch_chooser;
	GtkWidget* dialog;
	GtkWidget* output_label;
	GtkWidget* patch_button;
	GtkWidget* cancel_button;
	GtkWidget* dry_run_check;
	GladeXML* gxml;
	

	gboolean executing;
	gint uiid;
};

struct _PatchPluginClass {
	AnjutaPluginClass parent_class;
};


#endif /* _PATCH_PLUGIN_H_ */

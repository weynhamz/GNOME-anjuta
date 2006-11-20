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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _PATCH_PLUGIN_H_
#define _PATCH_PLUGIN_H_

#include <config.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

#define ICON_FILE "anjuta-patch-plugin.png"

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

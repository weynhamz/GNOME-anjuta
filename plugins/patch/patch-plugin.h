/*
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
 
#ifndef _PATCH_PLUGIN_H
#define _PATCH_PLUGIN_H

typedef struct _PatchPluginGUI PatchPluginGUI;

struct _PatchPluginGUI
{
	GtkWidget* dialog;
	
	GtkWidget* ok_button;
	GtkWidget* cancel_button;
	
	GtkWidget* entry_patch_dir;
	GtkWidget* entry_patch_file;
	
	GtkWidget* hscale_patch_level;
	
	PatchPlugin *plugin;
	IAnjutaMessageView* mesg_view;
};


void patch_show_gui (PatchPlugin *plugin);

#endif /* _PATCH_PLUGIN_H */

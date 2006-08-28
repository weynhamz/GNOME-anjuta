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
 
#ifndef _ANJUTA_VIM_PLUGIN_H_
#define _ANJUTA_VIM_PLUGIN_H_

#include <config.h>
#include <libanjuta/anjuta-plugin.h>

#define ICON_FILE "anjuta-vim-plugin.png"

typedef struct _AnjutaVimPlugin AnjutaVimPlugin;
typedef struct _AnjutaVimPluginClass AnjutaVimPluginClass;

struct _AnjutaVimPlugin {
	AnjutaPlugin parent;
	GtkWidget *vim_widget;
};

struct _AnjutaVimPluginClass {
	AnjutaPluginClass parent_class;
};


#endif /* _PATCH_PLUGIN_H_ */

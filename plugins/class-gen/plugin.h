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
 
#ifndef __CLASS_GEN_PLUGIN_H__
#define __CLASS_GEN_PLUGIN_H__

#include <libanjuta/anjuta-plugin.h>
//#include <libanjuta/interfaces/ianjuta-message-manager.h>
//#include <libanjuta/anjuta-preferences.h>


typedef struct _AnjutaClassGenPlugin AnjutaClassGenPlugin;
typedef struct _AnjutaClassGenPluginClass AnjutaClassGenPluginClass;
	

struct _AnjutaClassGenPlugin {
	AnjutaPlugin parent;

	AnjutaPreferences *prefs;	
	gchar *top_dir;
	guint root_watch_id;
	
	GtkWidget *widget;	
	
	// Member variables
	gboolean		m_bOK;
	gboolean		m_bUserEdited;
	gboolean		m_bUserSelectedHeader;
	gboolean		m_bUserSelectedSource;
	gboolean		m_bVirtualDestructor;
	gboolean		m_bInline;
	gchar*			m_szClassName;
	gchar*			m_szDeclFile;
	gchar*			m_szImplFile;
	gchar*			m_szBaseClassName;
	gchar*			m_szAccess;
	gchar*			m_szClassType;

	// GTK+ interface
	GtkWidget*		dlgClass;
	GtkWidget*		fixed;
	
	// logo
	GtkWidget*		pixmap_logo;
	GdkColormap*	colormap;
	GdkPixmap*		gdkpixmap;
	GdkBitmap*		mask;
	
	// buttons
	GtkWidget*		button_help;
	GtkWidget*		button_cancel;
	GtkWidget*		button_finish;
	GtkWidget*		button_browse_header_file;
	GtkWidget*		button_browse_source_file;
	
	// entries
	GtkWidget*		entry_class_name;
	GtkWidget*		entry_header_file;
	GtkWidget*		entry_source_file;
	GtkWidget*		entry_base_class;
	
	// labels
	GtkWidget*		label_welcome;
	GtkWidget*		label_description;
	GtkWidget*		label_class_type;
	GtkWidget*		label_class_name;
	GtkWidget*		label_header_file;
	GtkWidget*		label_source_file;
	GtkWidget*		label_base_class;
	GtkWidget*		label_access;
	GtkWidget*		label_author;
	GtkWidget*		label_version;
	GtkWidget*		label_todo;
	
	// separators
	GtkWidget*		hseparator1;
	GtkWidget*		hseparator3;
	GtkWidget*		hseparator2;
	GtkWidget*		hseparator4;
	
	// checkbuttons
	GtkWidget*		checkbutton_virtual_destructor;
	GtkWidget*		checkbutton_inline;
	
	// combo boxes
	GtkWidget*		combo_access;
	GList*			combo_access_items;
	GtkWidget*		combo_access_entry;
	GtkWidget*		combo_class_type;
	GList*			combo_class_type_items;
	GtkWidget*		combo_class_type_entry;
	
	// file selections
	GtkWidget*		header_file_selection;
	GtkWidget*		source_file_selection;
	
	// tooltips
	GtkTooltips*	tooltips;

};

struct _AnjutaClassGenPluginClass {
	AnjutaPluginClass parent_class;
};


#endif /* __CLASS_GEN_PLUGIN_H__ */

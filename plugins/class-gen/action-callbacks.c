/*
 *  Copyright (C) 2002  Dave Huseby
 *  Copyright (C) 2005 Massimo Cora' [porting to Anjuta 2.x plugin style]
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

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>

#include "action-callbacks.h"
#include "class_gen.h"
#include "plugin.h"


gint
on_delete_event (GtkWidget* widget, GdkEvent* event, AnjutaClassGenPlugin *plugin)
{	
	/* free up the strings and destroy the window */
	class_gen_del (plugin);
	gtk_widget_destroy (GTK_WIDGET (plugin->dlgClass));
	return FALSE;	
}

void
on_header_browse_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin)
{
	plugin->header_file_selection = gtk_file_selection_new _("Select header file.");
	gtk_window_set_modal (GTK_WINDOW (plugin->header_file_selection), FALSE);
	g_signal_connect(G_OBJECT (GTK_FILE_SELECTION (plugin->header_file_selection)->ok_button),
						"clicked", GTK_SIGNAL_FUNC (on_header_file_selection), plugin);
	
	g_signal_connect(G_OBJECT (GTK_FILE_SELECTION (plugin->header_file_selection)->cancel_button),
						"clicked", GTK_SIGNAL_FUNC (on_header_file_selection_cancel), plugin);

	gtk_file_selection_complete (GTK_FILE_SELECTION (plugin->header_file_selection), "*.h");
	gtk_widget_show (plugin->header_file_selection);
}

void
on_source_browse_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin)
{
	plugin->source_file_selection = gtk_file_selection_new _("Select source file.");
	gtk_window_set_modal(GTK_WINDOW(plugin->source_file_selection), FALSE);
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(plugin->source_file_selection)->ok_button),
						"clicked", GTK_SIGNAL_FUNC(on_source_file_selection), plugin);
	
	g_signal_connect(G_OBJECT (GTK_FILE_SELECTION(plugin->source_file_selection)->cancel_button),
						"clicked", GTK_SIGNAL_FUNC(on_source_file_selection_cancel), plugin);
	
	SAFE_FREE (plugin->m_szClassType);
	plugin->m_szClassType = gtk_editable_get_chars (GTK_EDITABLE (plugin->combo_class_type_entry),0, -1);
	
	if(strcmp (plugin->m_szClassType, "Generic C++ Class") == 0)
	{
		gtk_file_selection_complete(GTK_FILE_SELECTION(plugin->source_file_selection), "*.cc");
	}
	else if (strcmp (plugin->m_szClassType, "GTK+ Class") == 0)
	{
		gtk_file_selection_complete (GTK_FILE_SELECTION (plugin->source_file_selection), "*.c");
	}

	gtk_widget_show (plugin->source_file_selection);
}


void
on_class_name_changed (GtkEditable* editable, AnjutaClassGenPlugin *plugin)
{	
	gchar buf[1024];

	SAFE_FREE (plugin->m_szClassName);
	SAFE_FREE (plugin->m_szDeclFile);
	SAFE_FREE (plugin->m_szImplFile);

	/* get the new class name */
	plugin->m_szClassName = gtk_editable_get_chars (GTK_EDITABLE (plugin->entry_class_name),0, -1);
	
	if (strlen (plugin->m_szClassName) > 0)
	{
		if (!plugin->m_bUserSelectedHeader)
		{
			/* set the header file name */
			memset(buf, 0, 1024 * sizeof(gchar));
			sprintf (buf, "%s.h", plugin->m_szClassName);
			gtk_entry_set_text (GTK_ENTRY (plugin->entry_header_file), buf);
		}
		
		if(!plugin->m_bUserSelectedSource)
		{
			SAFE_FREE (plugin->m_szClassType);
			plugin->m_szClassType = gtk_editable_get_chars (GTK_EDITABLE (plugin->combo_class_type_entry),0, -1);
			
			if (strcmp (plugin->m_szClassType, "Generic C++ Class") == 0)
			{
				/* set the cc file name */
				memset (buf, 0, 1024 * sizeof(gchar));
				sprintf (buf, "%s.cc", plugin->m_szClassName);
				gtk_entry_set_text (GTK_ENTRY (plugin->entry_source_file), buf);
			}
			else if (strcmp (plugin->m_szClassType, "GTK+ Class") == 0)
			{
				/* set the cc file name */
				memset (buf, 0, 1024 * sizeof(gchar));
				sprintf (buf, "%s.c", plugin->m_szClassName);
				gtk_entry_set_text (GTK_ENTRY (plugin->entry_source_file), buf);
			}
		}
		
		/* set the browse buttons to sensitive */
		gtk_widget_set_sensitive (plugin->button_browse_header_file, TRUE);
		if(!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->checkbutton_inline)))
			gtk_widget_set_sensitive (plugin->button_browse_source_file, TRUE);
		
		plugin->m_bUserEdited = TRUE;
		gtk_widget_set_sensitive (GTK_WIDGET (plugin->button_finish), TRUE);
	}
	else
	{
		/* set the header and cc file names to null and set the buttons to insensitive */
		if(!plugin->m_bUserSelectedHeader)
		{
			gtk_entry_set_text (GTK_ENTRY (plugin->entry_header_file), "");
			gtk_widget_set_sensitive (plugin->button_browse_header_file, FALSE);
		}
		
		if(!plugin->m_bUserSelectedSource)
		{
			gtk_entry_set_text (GTK_ENTRY (plugin->entry_source_file), "");
			gtk_widget_set_sensitive (plugin->button_browse_source_file, FALSE);
		}
		
		plugin->m_bUserEdited = FALSE;
		gtk_widget_set_sensitive (GTK_WIDGET (plugin->button_finish), FALSE);
	}
		
	/* get the chars from the entries */
	plugin->m_szDeclFile = gtk_editable_get_chars (GTK_EDITABLE (plugin->entry_header_file), 0, -1);
	plugin->m_szImplFile = gtk_editable_get_chars (GTK_EDITABLE (plugin->entry_source_file), 0, -1);
}


void
on_class_type_changed (GtkEditable* editable, AnjutaClassGenPlugin *plugin)
{	
	gchar buf = '\0';
	SAFE_FREE(plugin->m_szClassType);
	
	/* get the new class name */
	plugin->m_szClassType = gtk_editable_get_chars (GTK_EDITABLE (plugin->combo_class_type_entry),0, -1);
	
	if (strlen (plugin->m_szClassType) > 0)
	{
		if (strcmp (plugin->m_szClassType, "Generic C++ Class") == 0)
		{
			/* tailor the interface to c++ */
			gtk_widget_set_sensitive (plugin->combo_access, TRUE);
			gtk_widget_set_sensitive (plugin->checkbutton_virtual_destructor, TRUE);
			gtk_widget_set_sensitive (plugin->entry_base_class, TRUE);
			gtk_widget_set_sensitive (plugin->label_base_class, TRUE);
			gtk_widget_set_sensitive (plugin->label_access, TRUE);
		}
		else if (strcmp (plugin->m_szClassType, "GTK+ Class") == 0)
		{
			/* tailor the interface to GTK+ c */
			gtk_widget_set_sensitive (plugin->combo_access, FALSE);
			gtk_widget_set_sensitive (plugin->checkbutton_virtual_destructor, FALSE);
			gtk_widget_set_sensitive (plugin->entry_base_class, FALSE);
			gtk_widget_set_sensitive (plugin->label_base_class, FALSE);
			gtk_widget_set_sensitive (plugin->label_access, FALSE);
			gtk_entry_set_text (GTK_ENTRY (plugin->entry_base_class), &buf);
			
			SAFE_FREE (plugin->m_szBaseClassName);
			plugin->m_szBaseClassName = gtk_editable_get_chars (GTK_EDITABLE (plugin->entry_base_class),0, -1);
		}
	}
}


void
on_finish_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin)
{
	DEBUG_PRINT	("on_finish_clicked");
	class_gen_get_strings (plugin);
	if (!is_legal_class_name (plugin->m_szClassName))
	{
		class_gen_message_box(_("Class name not valid"));
		return;
	}
	if (strlen (plugin->m_szBaseClassName) > 0)
	{
		if (!is_legal_class_name (plugin->m_szBaseClassName))
		{
			class_gen_message_box (_("Base class name not valid"));
			return;
		}
	}
	if (!is_legal_file_name (plugin->m_szDeclFile))
	{
		class_gen_message_box (_("Declaration file name not valid"));
		return;
	}
	if (!is_legal_file_name (plugin->m_szImplFile))
	{
		class_gen_message_box (_("Implementation file name not valid"));
		return;
	}
	class_gen_generate (plugin);
	class_gen_del (plugin);
	gtk_widget_destroy (GTK_WIDGET (plugin->dlgClass));
}


void
on_cancel_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin)
{		
	/* free up the strings and destroy the window */
	class_gen_del (plugin);
	gtk_widget_destroy (GTK_WIDGET (plugin->dlgClass));
}


void
on_help_clicked (GtkButton* button, AnjutaClassGenPlugin *plugin)
{
}


void
on_inline_toggled (GtkToggleButton* button, AnjutaClassGenPlugin *plugin)
{
	plugin->m_bInline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(plugin->checkbutton_inline));
	
	if(!plugin->m_bInline)
	{
		/* set the source file entry and browse buttons sensitive */
		gtk_widget_set_sensitive (plugin->entry_source_file, TRUE);
		if (plugin->m_bUserEdited)
			gtk_widget_set_sensitive (plugin->button_browse_source_file, TRUE);
	}
	else
	{
		/* set the source file entry and browse buttons insensitive */
		gtk_widget_set_sensitive (plugin->entry_source_file, FALSE);
		gtk_widget_set_sensitive (plugin->button_browse_source_file, FALSE);
	}
}


void
on_header_file_selection_cancel (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin)
{
	DEBUG_PRINT ("header cancel");
	gtk_widget_destroy (plugin->header_file_selection);
	plugin->header_file_selection = NULL;
}


void
on_source_file_selection_cancel (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin)
{
	DEBUG_PRINT ("source cancel");
	gtk_widget_destroy (plugin->source_file_selection);
	plugin->source_file_selection = NULL;
}


void 
on_header_file_selection (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin)
{
	SAFE_FREE (plugin->m_szDeclFile);
	plugin->m_szDeclFile = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (plugin->header_file_selection)));
	gtk_entry_set_text (GTK_ENTRY (plugin->entry_header_file), plugin->m_szDeclFile);
	
	if (strlen (plugin->m_szDeclFile) > 0)
		plugin->m_bUserSelectedHeader = TRUE;
	else
		plugin->m_bUserSelectedHeader = FALSE;
	
	gtk_widget_destroy (plugin->header_file_selection);
	plugin->header_file_selection = NULL;
}


void
on_source_file_selection (GtkFileSelection* selection, AnjutaClassGenPlugin *plugin)
{
	SAFE_FREE (plugin->m_szImplFile);
	plugin->m_szImplFile = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (plugin->source_file_selection)));
	gtk_entry_set_text (GTK_ENTRY (plugin->entry_source_file), plugin->m_szImplFile);

	if (strlen (plugin->m_szImplFile) > 0)
		plugin->m_bUserSelectedSource = TRUE;
	else
		plugin->m_bUserSelectedSource = FALSE;
	
	gtk_widget_destroy (plugin->source_file_selection);
	plugin->source_file_selection = NULL;
}

/*
 * action-callbacks.c
 * Copyright (C) 2003 Naba Kumar
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * on_enter_sel
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include <libanjuta/interfaces/ianjuta-macro.h>

#include "file.h"
#include "action-callbacks.h"

static IAnjutaEditor*
get_current_editor (AnjutaPlugin *plugin)
{
	IAnjutaEditor *editor;
	IAnjutaDocumentManager* docman;
	docman = anjuta_shell_get_interface (plugin->shell,
										 IAnjutaDocumentManager,
										 NULL);
	editor = ianjuta_document_manager_get_current_editor (docman, NULL);
	return editor;
}

static IAnjutaMacro*
get_macro(gpointer user_data)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (user_data);
	return anjuta_shell_get_interface (plugin->shell, IAnjutaMacro, NULL);
}

void
on_insert_c_gpl_notice(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "/* GPL */", NULL);
}

void
on_insert_cpp_gpl_notice(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "// GPL", NULL);
}

void
on_insert_py_gpl_notice(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "# GPL", NULL);
}

void
on_insert_username(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "UserName", NULL);
}

void
on_insert_changelog_entry(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "Changelog", NULL);
}

void
on_insert_date_time(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "Date_Time", NULL);	
}

void
on_insert_header_template(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	const gchar *filename;

	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	filename = ianjuta_editor_get_filename (editor , NULL);	
	if ( g_strcasecmp((filename) + strlen(filename) - 2, ".h") == 0)
	{
		IAnjutaMacro *macro = get_macro(user_data);
		if (macro)
			ianjuta_macro_insert (macro, "Header_h", NULL);	
	}
}

void
on_insert_header(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "Header_c", NULL);
}

void
on_insert_switch_template(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "switch", NULL);
}

void
on_insert_for_template(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "for", NULL);
}

void
on_insert_while_template(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "while", NULL);
}

void
on_insert_ifelse_template(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "if...else", NULL);
}

void
on_insert_cvs_author(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_author", NULL);
}

void
on_insert_cvs_date(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_date", NULL);
}

void
on_insert_cvs_header(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_header", NULL);
}

void
on_insert_cvs_id(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_id", NULL);
}

void
on_insert_cvs_log(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_log", NULL);
}

void
on_insert_cvs_name(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_name", NULL);
}

void
on_insert_cvs_revision(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_revision", NULL);
}

void
on_insert_cvs_source(GtkAction * action, gpointer user_data)
{
	IAnjutaMacro *macro = get_macro(user_data);
	if (macro)
		ianjuta_macro_insert (macro, "cvs_source", NULL);
}

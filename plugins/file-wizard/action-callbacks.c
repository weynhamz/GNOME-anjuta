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

static AnjutaPreferences *
get_preferences (AnjutaPlugin *plugin)
{
	return anjuta_shell_get_preferences (plugin->shell, NULL);
}

void
on_insert_c_gpl_notice(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_c_gpl_notice(editor);
}

void
on_insert_cpp_gpl_notice(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cpp_gpl_notice(editor);
}

void
on_insert_py_gpl_notice(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_py_gpl_notice(editor);
}

void
on_insert_username(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	AnjutaPreferences *prefs;
	prefs = get_preferences (ANJUTA_PLUGIN (user_data));
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_username(editor, prefs);
}

void
on_insert_changelog_entry(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	AnjutaPreferences *prefs;
	prefs = get_preferences (ANJUTA_PLUGIN (user_data));
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_changelog_entry(editor, prefs);
}

void
on_insert_date_time(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_date_time(editor);
}

void
on_insert_header_template(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_header_template(editor);
}

void
on_insert_header(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	AnjutaPreferences *prefs;
	prefs = get_preferences (ANJUTA_PLUGIN (user_data));
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_header(editor, prefs);
}

void
on_insert_switch_template(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_switch_template(editor);
}

void
on_insert_for_template(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_for_template(editor);
}

void
on_insert_while_template(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_while_template(editor);
}

void
on_insert_ifelse_template(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_ifelse_template(editor);
}

void
on_insert_cvs_author(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	AnjutaPreferences *prefs;
	prefs = get_preferences (ANJUTA_PLUGIN (user_data));
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_author(editor);
}

void
on_insert_cvs_date(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	AnjutaPreferences *prefs;
	prefs = get_preferences (ANJUTA_PLUGIN (user_data));
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_date(editor);
}

void
on_insert_cvs_header(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_header(editor);
}

void
on_insert_cvs_id(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_id(editor);
}

void
on_insert_cvs_log(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_log(editor);
}

void
on_insert_cvs_name(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_name(editor);
}

void
on_insert_cvs_revision(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_revision(editor);
}

void
on_insert_cvs_source(GtkAction * action, gpointer user_data)
{
	IAnjutaEditor *editor;
	editor = get_current_editor (ANJUTA_PLUGIN (user_data));
	if(editor)
		insert_cvs_source(editor);
}

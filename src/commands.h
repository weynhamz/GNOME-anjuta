/*
 * command_editor.h
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include <gnome.h>
#include <glade/glade.h>
#include "properties.h"

typedef struct _CommandEditorWidgets CommandEditorWidgets;
typedef struct _CommandEditor CommandEditor;
typedef struct _CommandData CommandData;

struct _CommandData
{
	gchar* key;

	gchar* compile;
	gchar* make;
	gchar* build;
	gchar* execute;
};

struct _CommandEditorWidgets
{
	GtkWidget *window;

	GtkWidget *pix_editor_entry;
	GtkWidget *image_editor_entry;
	GtkWidget *html_editor_entry;
	GtkWidget *terminal_entry;
	GtkWidget *language_om;
	GtkWidget *compile_entry;
	GtkWidget *build_entry;
	GtkWidget *execute_entry;
	GtkWidget *make_entry;
};

struct _CommandEditor
{
	GladeXML *gxml;
	CommandEditorWidgets widgets;
	
	PropsID props;
	PropsID props_user;
	PropsID props_global;
	
	/*
	 * Private 
	 */
	CommandData *current_command_data;
	
	gboolean is_showing;
	gint win_pos_x, win_pos_y;
	gint win_width, win_height;
};

/* Command data to be used in command editor */
CommandData *command_data_new(void);
void command_data_destroy (CommandData *cdata);

/* Get the string version of the command */
gchar* command_data_get (CommandData *cdata, gchar *cmd);

/* String data should use these functions */
void command_data_set (CommandData *cdata, gchar* cmd, gchar *cmd_str);

/* CommandEditor */
CommandEditor *command_editor_new (PropsID p_global, PropsID p_user, PropsID p);

/* Syncs the key values and the widgets */
void command_editor_sync (CommandEditor *p);

/* Resets the default values into the keys */
void command_editor_reset_defaults (CommandEditor *);

/* ----- */
void command_editor_hide (CommandEditor *);
void command_editor_show (CommandEditor *);
void command_editor_destroy (CommandEditor *);

/* Get commands */

/* Return must be freed */
gchar*
command_editor_get_command_file (CommandEditor* ce, gchar* key, gchar* fname);

/* Return must be freed */
gchar*
command_editor_get_command (CommandEditor* ce, gchar* key);

/* Save and Load */
gboolean command_editor_save_yourself (CommandEditor *p, FILE* stream);
gboolean command_editor_load_yourself (CommandEditor *p, PropsID pr);
gboolean command_editor_save (CommandEditor *p, FILE* stream);
gboolean command_editor_load (CommandEditor *p, PropsID pr);

/*
 * CommandEditor KEY definitions.
 *
 * Use the keys instead of using the strings directly.
 *
 * Call these as the second arg of the
 * functions command_editor_get_command() and command_editor_get_command_file().
 */

#define COMMAND_LANGUAGES "commands.languages"
#define COMMAND_OPEN_FILE "command.open.file."
#define COMMAND_VIEW_FILE "command.view.file."

#define COMMAND_COMPILE_FILE "command.compile.file."
#define COMMAND_MAKE_FILE "command.make.file."
#define COMMAND_BUILD_FILE "command.build.file."
#define COMMAND_EXECUTE_FILE "command.execute.file."

#define COMMAND_BUILD_MODULE "command.build.module"
#define COMMAND_BUILD_PROJECT "command.build.project"
#define COMMAND_BUILD_TARBALL "command.build.tarball"
#define COMMAND_BUILD_INSTALL "command.build.install"
#define COMMAND_BUILD_AUTOGEN "command.build.autogen"
#define COMMAND_BUILD_CLEAN "command.build.clean"
#define COMMAND_BUILD_CLEAN_ALL "command.build.clean.all"
#define COMMAND_EXECUTE_PROJECT "command.execute.project"

#define COMMAND_PIXMAP_EDITOR_OPEN "command.open.file.$(file.patterns.icon)"
#define COMMAND_IMAGE_EDITOR_OPEN "command.open.file.$(file.patterns.image)"
#define COMMAND_HTML_EDITOR_OPEN "command.open.file.$(file.patterns.html)"

#define COMMAND_PIXMAP_EDITOR_VIEW "command.view.file.$(file.patterns.icon)"
#define COMMAND_IMAGE_EDITOR_VIEW "command.view.file.$(file.patterns.image)"
#define COMMAND_HTML_EDITOR_VIEW "command.view.file.$(file.patterns.html)"
#define COMMAND_TERMINAL "command.terminal"

#endif

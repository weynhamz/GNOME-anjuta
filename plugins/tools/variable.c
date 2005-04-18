/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    variable.c
    Copyright (C) 2005 Sebastien Granjoux

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Get Anjuta variables
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "variable.h"

#include <libgnomevfs/gnome-vfs.h>

#include <glib/gi18n.h>

#include <string.h>

/*---------------------------------------------------------------------------*/

/* List of variables understood by the tools editor and their values.
 *---------------------------------------------------------------------------*/

/* enum and following variable_list table must be in the same order */

enum {
	ATP_PROJECT_ROOT_URI = 0,
	ATP_PROJECT_ROOT_DIRECTORY,
	ATP_FILE_MANAGER_CURRENT_URI,
	ATP_FILE_MANAGER_CURRENT_DIRECTORY,
	ATP_FILE_MANAGER_CURRENT_PATH,
	ATP_FILE_MANAGER_CURRENT_FILENAME,
	ATP_FILE_MANAGER_CURRENT_BASENAME,
	ATP_FILE_MANAGER_CURRENT_EXTENSION,
	ATP_VARIABLE_COUNT
		
};

static const struct
{
	char *name;
	char *help;
	ATPFlags flag;
} variable_list[] = {
  {"project_root_uri", "project root URI", ATP_DEFAULT}
, {"project_root_directory", "project root path", ATP_DIRECTORY }
, {"file_manager_current_uri", "selected file manager URI", ATP_DEFAULT }
, {"file_manager_current_directory", "selected file manager directory", ATP_DIRECTORY }
, {"file_manager_current_path", "selected file manager path", ATP_DEFAULT }
, {"file_manager_current_filename", "selected file manager file name", ATP_DEFAULT }
, {"file_manager_current_basename", "selected file manager file name without extension", ATP_DEFAULT }
, {"file_manager_current_extension", "selected file manager file extension", ATP_DEFAULT }
#if 0
  {CURRENT_FULL_FILENAME_WITH_EXT, "Current full filename with extension" }
, {CURRENT_FULL_FILENAME, "Current full filename without extension" }
, {CURRENT_FILENAME_WITH_EXT, "Current filename with extension" }
, {CURRENT_FILENAME, "Current filename without extension" }
, {CURRENT_FILE_DIRECTORY, "Directory of the current file" }
, {CURRENT_FILE_EXTENSION, "Extension of the current file" }
, {"current.file.selection", "Selected/focussed word in the current buffer"}
, {"top.proj.dir", "Top project directory" }
, {"project.name", "Name of the project" }
, {"project.type", "Type of project, e.g. GNOME"}
, {"project.target.type", "Type of target of the project"}
, {"project.version", "Project version"}
, {"project.author", "Author of the project"}
, {"project.source.target", "Main target executable/library for the project"}
, {"project.source.paths", "Directories where project sources are present"}
, {"project.has.gettext", "Whether gettext (INTL) support is enabled for the project"}
, {"project.programming.language", "Programming languages used in the project"}
, {"project.excluded.modules", "Modules (directories) excluded from the project"}
, {"project.config.extra.modules.before", "Modules to configure before source module"}
, {"project.config.extra.modules.after", "Modules to configure after source module"}
, {"project.menu.entry", "Project menu entry (i.e. project name)"}
, {"project.menu.group", "Project menu group (e.g. Applications)"}
, {"project.menu.comment", "Desktop entry descriptive comment"}
, {"project.menu.icon", "Icon file for the project"}
, {"project.menu.need.terminal", "Whether the program should run in a terminal"}
, {"project.configure.options", "Options to pass to project configure script"}
, {"anjuta.program.arguments", "Arguments anjuta passes while executing the program"}
, {"module.*.name", "Name of project module ('*' can be include, source, pixmap, data, help, doc and po"}
, {"module.*.files", "Files under the given project module (source, include, etc.)"}
, {"compiler.options.supports", "Compiler support options (needs better explaination"}
, {"compiler.options.include.paths", "Include paths to pass to the compiler"}
, {"compiler.options.library.paths", "Library paths to pass to the linker"}
, {"compiler.options.libraries", "Libraries to link against"}
, {"compiler.options.defines", "Defines to pass to the compiler"}
, {"compiler.options.other.c.flags", "Other options to pass to the compiler"}
, {"compiler.options.other.l.flags", "Other library flags to pass to the linker"}
, {"compiler.options.opther.l.libs", "Other libraries to link with the application"}
, {"anjuta.last.open.project", "Last open project"}
, {"Generic user preferences", ""}
, {PROJECTS_DIRECTORY, "Directory where projects are created by default"}
, {TARBALLS_DIRECTORY, "Default tarballs creation directory"}
, {RPMS_DIRECTORY, "RPMs directory"}
, {SRPMS_DIRECTORY, "SRPMs directory"}
, {MAXIMUM_RECENT_PROJECTS, "Maximum recent projects to show" }
, {MAXIMUM_RECENT_FILES, "Maximum recent files to show" }
, {MAXIMUM_COMBO_HISTORY, "Maximum combo history size" }
, {DIALOG_ON_BUILD_COMPLETE, "Whether to show dialog on completion of build" }
, {BEEP_ON_BUILD_COMPLETE, "Whether to beep on completion of build" }
, {RELOAD_LAST_PROJECT, "If the last project is to be reloaded at startup" }
, {BUILD_OPTION_KEEP_GOING, "Whether to continue building on errors" }
, {BUILD_OPTION_DEBUG, "Enable debugging of build" }
, {BUILD_OPTION_SILENT, "Build silently" }
, {BUILD_OPTION_WARN_UNDEF, "Warn on undefined variables during build" }
, {BUILD_OPTION_JOBS, "Maximum number of build jobs" }
, {BUILD_OPTION_AUTOSAVE, "Autosave before build"}
, {DISABLE_SYNTAX_HILIGHTING, "Disable syntax highlighting for source files"}
, {SAVE_AUTOMATIC, "Save automatically on a periodic basis"}
, {INDENT_AUTOMATIC, "Auto-indent"}
, {USE_TABS, "Use tabs for indentation"}
, {BRACES_CHECK, "Check brace matching"}
, {"braces.sloppy", "Sloppy braces"}
, {DOS_EOL_CHECK, "Use DOS style end-of-line characters"}
, {WRAP_BOOKMARKS, "Wrap around bookmarks Previous/Next"}
, {TAB_SIZE, "Tab size"}
, {INDENT_SIZE, "Indentation size"}
, {INDENT_OPENING, "Whether to indent opening brace"}
, {INDENT_CLOSING, "Whether to indent closing brace"}
, {AUTOSAVE_TIMER, "Time in minutes after which autosave occurs"}
, {SAVE_SESSION_TIMER, "use save session timer"}
, {AUTOFORMAT_DISABLE, "Disable autoformatting"}
, {AUTOFORMAT_CUSTOM_STYLE, "Custom autoformat style (indent parameters)"}
, {AUTOFORMAT_STYLE, "Predefined autoformat style"}
, {EDITOR_TABS_POS, "Editor tab position"}
, {EDITOR_TABS_HIDE, "Whether to hide editor tabs"}
, {EDITOR_TABS_ORDERING, "Whether to order editor tabs by file name"}
, {EDITOR_TABS_RECENT_FIRST, "Bring recent tabs first on paeg switch"}
, {STRIP_TRAILING_SPACES, "Whether to strip trailing spaces"}
, {"edge.columns", "Maximum length of line suggested by the editor"}
, {"edge.mode", "How the editor marks lines exceeding recommended length"}
, {"edge.color", "Color of edge line"}
, {"margin.linenumber.visible", "Whether linenumber margin is visible"}
, {"margin.marker.visible", "Whether marker margin is visible"}
, {"margin.fold.visible", "Whether the fold margin is visible"}
, {"margin.linenumber.width", "Width of the linenumber margin"}
, {"margin.marker.width", "Width of the marker margin"}
, {"margin.fold.width", "Width of the fold margin"}
, {"buffered.draw", "Use double-buffering for editor draw"}
, {"default.file.ext", "Default extension for new files"}
, {"vc.home.key", "Should the editor use the VC_HOME key"}
, {"highlight.indentation.guides", "Highlight indentation guides"}
, {"dwell.period", "Dwell period (used for showing editor tips"}
, {"fold", "Enable folding"}
, {"fold.flags", "Fold flags"}
, {"fold.compact", "Compact folding"}
, {"fold.use.plus", "User +/- signs for folding"}
, {"fold.comment", "Fold comments"}
, {"fold.comment.python", "Fold python comments"}
, {"fold.quotes.python", "Fold python quotes"}
, {"fold.html", "Fold HTML"}
, {"fold.symbols", "Fold symbols"}
, {"styling.within.preporcessor", "Enable styling within preprocessor"}
, {"xml.auto.close.tags", "Automatically close XML tags"}
, {"asp.default.language", "Default language for ASP pages"}
, {"calltip.*.ignorecase", "Whether calltip should be case-insensitive"}
, {"autocomplete.*.ignorecase", "Whether autocomplete should be case-insensitive"}
, {"autocomplete.choose.single", "Whether single autocomplete option is chosen automatically"}
, {"autocompleteword.automatic", "Try to autocomplete after these many characters"}
, {FOLD_ON_OPEN, "Whether to close folds on opening a new file"}
, {CARET_FORE_COLOR, "Caret foreground color"}
, {CALLTIP_BACK_COLOR, "Background color of calltip"}
, {SELECTION_FORE_COLOR, "Foreground color of selection"}
, {SELECTION_BACK_COLOR, "Background color of selection"}
, {TEXT_ZOOM_FACTOR, "Current text zoom factor"}
, {TRUNCAT_MESSAGES, "Whether to truncate long messages"}
, {TRUNCAT_MESG_FIRST, "Number of characters to truncate after"}
, {TRUNCAT_MESG_LAST, "No of trailing characters to show"}
, {MESSAGES_TAG_POS, "Position of messages window tabs"}
, {MESSAGES_COLOR_ERROR, "Color of error message lines"}
, {MESSAGES_COLOR_WARNING, "Color of warning message lines"}
, {MESSAGES_COLOR_MESSAGES1, "Color of program messages"}
, {MESSAGES_COLOR_MESSAGES2, "Color of other messages"}
, {MESSAGES_INDICATORS_AUTOMATIC, "Enable automatic message indicators"}
, {"indicator.0.style", "Style for general indicators"}
, {"indicator.0.color", "Color for general indicators"}
, {"indicator.1.style", "Style for warning indicators"}
, {"indicator.1.color", "Color for warning indicators"}
, {"indicator.2.style", "Style for error indicators"}
, {"indicator.2.color", "Color for error indicators"}
, {"blank.margin.left", "Width of left editor margin"}
, {"blank.margin.right", "Width of right editor margin"}
, {"horizontal.scrollbar", "Enable horizontal scrollbar"}
, {"chars.alpha", "Alphabetic characters"}
, {"chars.numeric", "Numeric characters"}
, {"chars.accented", "Accented characters"}
, {"source.files", "These extensions will be treated as source files"}
, {"view.eol", "Should the EOL character be visible"}
, {"view.whitespace", "Should the whitespace be visible"}
, {"view.indentation.whitespace", "Should indentation whitespace be visible"}
, {"view.indentation.guides", "Should indentation guides be visible"}
, {"view.line.wrap", "Should line wrap be on by default"}
, {"use.monospaced", "Use monospaced fonts by default"}
, {"statusbar.visible", "Should the statusbar be visible"}
, {"main.toolbar.visible", "Should the main toolbar be visible"}
, {"browser.toolbar.visible", "Should the browser toolbar be visible"}
, {"extended.toolbar.visible", "Should the extended toolbar be visible"}
, {"format.toolbar.visible", "Should the format toolbar be visible"}
, {"debug.toolbar.visible", "Should the debug toolbar be visible"}
, {"font.base", "Default base font"}
, {"font.monospace", "Default monospace font"}
, {"font.big", "Default big font"}
, {"font.medium", "Default medium font"}
, {"font.small", "Default small font"}
, {"font.comment", "Default comment font"}
, {"style.default.whitespace", "Default style for whitespaces"}
, {"style.default.comment", "Default style for comments"}
, {"style.default.number", "Default style for number"}
, {"style.default.keyword", "Default style for keywords"}
, {"style.default.syskeyword", "Default style for system keywords"}
, {"style.default.localkeyword", "Default style for local keywords"}
, {"style.default.doublequote", "Default style for doubel quoted strings"}
, {"style.default.singlequote", "Default style for single quote strings"}
, {"style.default.preprocessor", "Default style for preprocessor"}
, {"style.default.operator", "Default style for operators"}
, {"style.default.unclosedstring", "Default style for unclosed strings"}
, {"style.default.identifier", "Default style for identifiers"}
, {"style.default.definition", "Default style for definition name"}
, {"style.default.function", "Default style for function name"}
, {"messages.is.docked", "Should be messages window be docked"}
, {"project.is.docked", "Should be project window be docked"}
, {AUTOMATIC_TAGS_UPDATE, "Update tag image automatically"}
, {BUILD_SYMBOL_BROWSER, "Build symbol browsxer automatically"}
, {BUILD_FILE_BROWSER, "Build file browser automatically"}
, {SHOW_TOOLTIPS, "Show tooltips"}
, {PRINT_PAPER_SIZE, "Paper size"}
, {PRINT_HEADER, "Print header"}
, {PRINT_WRAP, "Enable wrapping while printing"}
, {PRINT_LINENUM_COUNT, "Print line number after these many lines"}
, {PRINT_LANDSCAPE, "Use landscape printing mode"}
, {PRINT_MARGIN_LEFT, "Left margin for printing"}
, {PRINT_MARGIN_RIGHT, "Right margin for printing"}
, {PRINT_MARGIN_TOP, "Top margin for printing"}
, {PRINT_MARGIN_BOTTOM, "Bottom margin for printing"}
, {PRINT_COLOR, "Enable color printing"}
, {USE_COMPONENTS, "Use components"}
, {IDENT_NAME, "User name"}
, {IDENT_EMAIL, "User e-mail address"}
, {"anjuta.home.directory", "Home directory for Anjuta"}
, {"anjuta.data.directory", "Data directory for anjuta"}
, {"anjuta.pixmap.directory", "Pixmap directory for anjuta"}
, {"anjuta.version", "Anjuta version"}
, {"make", "Command for executing make"}
, {"anjuta.make.options", "Options to pass to the make command"}
, {"command.build.module", "Command to build a module"}
, {"command.build.project", "Command to build the full project"}
, {"command.build.tarball", "Command to build the project tarball"}
, {"command.build.install", "Command to install a project"}
, {"command.build.clean", "Command to clean up temporary project files"}
, {"command.build.clean.all", "Command to clean all generated files"}
, {"command.execute.project", "Command to run the project executable"}
, {"command.terminal", "Command to execute a program in a terminal"}
, {"anjuta.terminal", "Terminal program to be used (deprecated)"}
, {"anjuta.compiler.flags", "Default compiler flags"}
, {"anjuta.linker.flags", "Default linler flags"}
#endif
};


/* variable functions
 *---------------------------------------------------------------------------*/

guint
atp_variable_get_count (const ATPVariable* this)
{
	return ATP_VARIABLE_COUNT;
}

const gchar*
atp_variable_get_name (const ATPVariable* this, guint id)
{
	return id >= ATP_VARIABLE_COUNT ? NULL : variable_list[id].name;
}

const gchar*
atp_variable_get_help (const ATPVariable* this, guint id)
{
	return id >= ATP_VARIABLE_COUNT ? NULL : _(variable_list[id].help);
}

ATPFlags
atp_variable_get_flag (const ATPVariable* this, guint id)
{
	return id >= ATP_VARIABLE_COUNT ? ATP_NONE : variable_list[id].flag;
}

static guint
atp_variable_get_id (const ATPVariable* this, const gchar* name)
{
	guint i;

	for (i = 0; i < ATP_VARIABLE_COUNT; ++i)
	{
		if (strcmp (variable_list[i].name, name) == 0) break;
	}

	return i;
}

static guint
atp_variable_get_id_from_name_part (const ATPVariable* this, const gchar* name, gsize length)
{
	guint i;

	for (i = 0; i < ATP_VARIABLE_COUNT; ++i)
	{
		if ((strncmp (variable_list[i].name, name, length) == 0)
			      	&& (variable_list[i].name[length] == '\0')) break;
	}

	return i;
}

gchar*
atp_variable_get_value (const ATPVariable* this, const gchar* name)
{
	guint id;

	id = atp_variable_get_id (this, name);

	return atp_variable_get_value_from_id (this, id);
}

gchar*
atp_variable_get_value_from_name_part (const ATPVariable* this, const gchar* name, gsize length)
{
	guint id;

	id = atp_variable_get_id_from_name_part (this, name, length);

	return atp_variable_get_value_from_id (this, id);
}

static gchar*
atp_variable_get_anjuta_variable (const ATPVariable *this, guint id)
{
	gchar* string;
	GValue value = {0,};
	GError* err = NULL;

	anjuta_shell_get_value (*this, variable_list[id].name, &value, &err);
	if (err != NULL)
	{
		/* Value probably does not exist */
		g_error_free (err);
		return NULL;
	}
	string = G_VALUE_HOLDS (&value, G_TYPE_STRING) ? g_value_dup_string (&value) : NULL;
	g_value_unset (&value);

	return string;
}

static gchar*
atp_variable_get_path_from_anjuta_variable (const ATPVariable *this, guint id)
{
	gchar *var;
	gchar *dir;

	var = atp_variable_get_anjuta_variable (this, id);
	if (var == NULL) return NULL;
	dir = gnome_vfs_get_local_path_from_uri (var);
	g_free (var);

	return dir;
}

gchar*
atp_variable_get_value_from_id (const ATPVariable* this, guint id)
{
	char *path;
	char *val;
	char *ext;

	switch (id)
	{
	case ATP_PROJECT_ROOT_URI:
	case ATP_FILE_MANAGER_CURRENT_URI:
		return atp_variable_get_anjuta_variable (this, id);
	case ATP_PROJECT_ROOT_DIRECTORY:
		return atp_variable_get_path_from_anjuta_variable (this, ATP_PROJECT_ROOT_URI);
	case ATP_FILE_MANAGER_CURRENT_DIRECTORY:
		path = atp_variable_get_path_from_anjuta_variable (this, ATP_FILE_MANAGER_CURRENT_URI);
		if (path != NULL)
		{
			val = g_path_get_dirname (path);
			g_free (path);
			return val;
		}
		return path;
	case ATP_FILE_MANAGER_CURRENT_PATH:
		return atp_variable_get_path_from_anjuta_variable (this, ATP_FILE_MANAGER_CURRENT_URI);
	case ATP_FILE_MANAGER_CURRENT_FILENAME:
		path = atp_variable_get_path_from_anjuta_variable (this, ATP_FILE_MANAGER_CURRENT_URI);
		if (path != NULL)
		{
			val = g_path_get_basename (path);
			g_free (path);
			return val;
		}
		return path;
	case ATP_FILE_MANAGER_CURRENT_BASENAME:
		path = atp_variable_get_path_from_anjuta_variable (this, ATP_FILE_MANAGER_CURRENT_URI);
		if (path != NULL)
		{
			val = g_path_get_basename (path);
			g_free (path);
			ext = strrchr (val, '.');
			if (ext != NULL) *ext = '\0';	
			return val;
		}
		return path;
	case ATP_FILE_MANAGER_CURRENT_EXTENSION:
		path = atp_variable_get_path_from_anjuta_variable (this, ATP_FILE_MANAGER_CURRENT_URI);
		if (path != NULL)
		{
			val = g_path_get_basename (path);
			g_free (path);
			ext = strrchr (val, '.');
			if (ext != NULL) strcpy(val, ext);
			return val;
		}
		return path;
	default:
		return NULL;
	}
}

ATPVariable*
atp_variable_construct (ATPVariable* this, AnjutaShell* shell)
{
	*this = shell;

	return this;
}

void
atp_variable_destroy (ATPVariable* this)
{
}

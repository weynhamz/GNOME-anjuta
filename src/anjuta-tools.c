/*
** User customizable tools implementation
** Author: Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
*/

/**
Anjuta user-defined tools requirements statement:
Convention:
(M) = Must have
(R) = Recommended
(N) = Nice to have
(D) = Done (Implemented)
(P) = Partly implemented

User-defined tools should provide the user with a powerful mechanism to
do various activities customized to his needs from with the Anjuta GUI
framework. The tool system should also be flexible enough so that developers
of Anjuta can use it to easily integrate external programs in Anjuta and
enhance functionality of the IDE through the use of tools.

The following is a list of requirements and their relative pririties.
Progress should be tracked by marking each of the items as and when they are
implemented. Feel free to add/remove/reprioritize these items but please
discuss in the devel list before making any major changes.

R1: Modify GUI at program startup
	1) (P) Add new menu items associated with external commands.
	2) (N) Add drop-down toolbar item for easy access to all tools.
	3) (D) Should be appendable under any of the top/sub level menus.
	4) (N) Should be able to associate icons.
	5) (P) Should be able to associate shortcuts.

R2: Command line parameters
	1) (D) Pass variable command-line parameters to the tool.
	2) (D) Use existing properties system to pass parameters.
	3) (N) Ask user at run-time for parameters.

R3: Working directory
	1) (D) Specify working directory for the tool.
	2) (D) Ability to specify property variables for working dir.

R4: Standard input to the tool
	1) (R) Specify current buffer as standard input.
	2) (R) Specify property variables as standard input.
	3) (N) Specify list of open files as standard input.

R5: Output and error redirection
	1) (D) Output to any of the message pane windows.
	2) (M) Run in terminal.
	3) (R) Output to current/new buffer.
	4) (N) Show output in a popup window.

R6: Tool manipulation GUI
	1) (M) Add/remove tools with all options.
	2) (R) Enable/disable tool loading.

R7: Tool Storage
	1) (D) Gloabal and local tool storage.
	2) (R) Override global tools with local settings.
	3) (N) Project specific tools (load/unload with project)
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <gnome.h>
#include <glade/glade.h>

#include "scroll-menu.h"
#include "main_menubar.h"
#include "message-manager.h"
#include "widget-registry.h"
#include "anjuta.h"
#include "anjuta-tools.h"

#define SEPERATOR '\001'
#define LINESEP '\002'
#define TOOLS_FILE "tools.properties"

/** Defines how output should be added to buffer */
typedef enum _AnToolBufferAction
{
	AN_TBUF_NEW = MESSAGE_MAX + 1, /* Create a new buffer and put output into it */
	AN_TBUF_REPLACE, /* Replace exisitng buffer content with output */
	AN_TBUF_INSERT, /* Insert at cursor position in the current buffer */
	AN_TBUF_APPEND /* Append output to the end of the buffer */
} AnToolBufferAction;

/** Defines how and where tool information will be stored. */
typedef enum _AnToolStore
{
	AN_TSTORE_LOCAL = 0,
	AN_TSTORE_GLOBAL,
	AN_TSTORE_PROJECT
} AnToolStore;

typedef struct _AnUserTool
{
	gchar *name;
	gchar *command;
	gboolean enabled;
	gboolean file_level;
	gboolean project_level;
	gboolean detached;
	gboolean run_in_terminal;
	gchar *location;
	gchar *icon;
	gchar *shortcut;
	gchar *working_dir;
	gchar *input;
	int output; /* MESSAGE_* or AN_TBUF_* */
	int error; /* MESSAGE_* or AN_TBUF_* */
	AnToolStore storage_type;
	GtkWidget *menu_item;
} AnUserTool;

/* Maintains a list of all tools loaded */
static GSList *tool_list = NULL;

/* Maintains a hash table of tool name vs. tool object pointer to resolve
** name conflicts Multiple tools with the same name will result in error.
*/
static GHashTable *tool_hash = NULL;

/* Tool currently being executed. This is a bit problematic since multiple
** tools might be execured at the same time. So, this only keeps track of
** the tool being executed under 'launcher' control (The launcher can only
** execute one tool at at time.
** TODO: This needs to be reworked.
*/
static AnUserTool *current_tool = NULL;

/* Destroys memory allocated to an user tool */
#define FREE(x) if (x) g_free(x)
static void an_user_tool_free(AnUserTool *tool, gboolean remove_from_list)
{
	if (tool)
	{
		if (remove_from_list)
		{
			if (tool_list)
				tool_list = g_slist_remove(tool_list, tool);
			if (tool_hash && tool->name)
				g_hash_table_remove(tool_hash, tool->name);
		}
		FREE(tool->name);
		FREE(tool->command);
		FREE(tool->location);
		FREE(tool->icon);
		FREE(tool->shortcut);
		FREE(tool->working_dir);
		FREE(tool->input);
		if (tool->menu_item)
		{
			gtk_widget_hide(tool->menu_item);
			gtk_widget_unref(tool->menu_item);
		}
		g_free(tool);
	}
}

/* Creates a new tool item from the given buffer. Returns NULL
** on error. Note that the buffer passed is modified. On return,
** it will contain the position upto which read happened.
*/
#define STRMATCH(p) if (0 == strcmp(#p, name)) tool->p = g_strdup(value);
#define NUMMATCH(p) if (0 == strcmp(#p, name)) tool->p = atoi(value);
static AnUserTool *an_user_tool_new(char **buf)
{
	AnUserTool *tool, *tool1;
	char *s = *buf;
	char *name;
	char *value;
	char *seppos;
	char *colpos;

#ifdef TOOL_DEBUG
	fprintf(stderr, "Got line: '%s'\n", buf);
#endif

	tool = g_new0(AnUserTool, 1);
	/* Set default values */
	tool->enabled = TRUE;
	tool->output = MESSAGE_STDOUT;
	tool->error = MESSAGE_STDERR;
	while (*s != LINESEP && *s != '\0')
	{
		if (NULL == (colpos = strchr(s, ':')))
			break;
		if (NULL == (seppos = strchr(colpos + 1, SEPERATOR)))
			break;
		*colpos = '\0';
		*seppos = '\0';
		name = s;
		value = colpos + 1;
		STRMATCH(name)
		else STRMATCH(command)
		else NUMMATCH(enabled)
		else NUMMATCH(file_level)
		else NUMMATCH(project_level)
		else NUMMATCH(detached)
		else NUMMATCH(run_in_terminal)
		else STRMATCH(location)
		else STRMATCH(icon)
		else STRMATCH(shortcut)
		else STRMATCH(working_dir)
		else STRMATCH(input)
		else NUMMATCH(output)
		else NUMMATCH(error)
		s = seppos + 1;
	}
	*buf = s;
	/* Check if the minimum required fields were populated */
	if (NULL == tool->name || NULL == tool->command)
	{
#ifdef TOOL_DEBUG
		fprintf(stderr, "Got a tool with no name or command!\n");
#endif
		an_user_tool_free(tool, FALSE);
		return NULL;
	}
	/* Check for name clash */
	if (NULL == tool_hash)
	{
		tool_hash = g_hash_table_new(g_str_hash, g_str_equal);
	}
	else
	{
		tool1 = g_hash_table_lookup(tool_hash, tool->name);
		if (NULL != tool1)
		{
			g_warning("Tool '%s (%s)' has a name clash with tool '%s (%s)'"
			  , tool->name, tool->command, tool1->name, tool1->command);
			an_user_tool_free(tool, FALSE);
			return NULL;
		}
	}
	tool_list = g_slist_append(tool_list, tool);
	g_hash_table_insert(tool_hash, tool->name, tool);
	return tool;
}

/* Writes tool information to the given file. Properties are written as
** <name>:<value> pairs and seperated by the SEPERATOR character. End
** of line is indicated by the LINESEP character followed by a newline.
** A simple newline won't work because the plan is to allow the user
** to write simple scripts directly inside the tool. Note that the
** SEPERATOR and LINESEP characters are ASCII values 1 and 2, which
** should not occur in normal text - that's why they were chosen.
**
** FIXME: We really should be using XML here but I'm not familiar with
** LibXML2 API. If some kind folk could convert this mess into XML storage,
** I'll be really grateful.
*/
#define STRWRITE(p) if (tool->p && '\0' != tool->p[0]) \
	{\
		if (1 > fprintf(f, "%s:%s%c", #p, tool->p, SEPERATOR))\
			return FALSE;\
	}
#define NUMWRITE(p) if (1 > fprintf(f, "%s:%d%c", #p, tool->p, SEPERATOR))\
	{\
		return FALSE;\
	}
static gboolean an_user_tool_save(AnUserTool *tool, FILE *f)
{
	STRWRITE(name)
	STRWRITE(command)
	NUMWRITE(enabled)
	NUMWRITE(file_level)
	NUMWRITE(project_level)
	NUMWRITE(detached)
	NUMWRITE(run_in_terminal)
	STRWRITE(location)
	STRWRITE(icon)
	STRWRITE(shortcut)
	STRWRITE(working_dir)
	STRWRITE(input)
	NUMWRITE(output)
	NUMWRITE(error)
	if (1 > fprintf(f, "%c\n", LINESEP))
		return FALSE;
	else
		return TRUE;
}

/* Simplistic output handler - needs to be enhanced */
static void tool_stdout_handler(gchar *line)
{
	if (line)
	{
		if (current_tool && (current_tool->output <= MESSAGE_MAX))
		{
			anjuta_message_manager_append (app->messages, line
			  ,current_tool->output);
		}
	}
}

/* Simplistic error handler - needs to be enhanced */
static void tool_stderr_handler(gchar *line)
{
	if (line)
	{
		if (current_tool && (current_tool->error <= MESSAGE_MAX))
		{
			anjuta_message_manager_append (app->messages, line
			  , current_tool->error);
		}
	}
}

/* Simplistic termination handler - needs to be enhanced */
static void tool_terminate_handler(gint status, time_t time)
{
	char line[BUFSIZ];
	snprintf(line, BUFSIZ, "Tool terminated with status %d\n", status);
	if (current_tool && (current_tool->output <= MESSAGE_MAX))
	{
		anjuta_message_manager_append(app->messages, line
		  , current_tool->output);
	}
	current_tool = NULL;
}

/* Menu activate handler which executes the tool. It should do command
** substitution, input, output and error redirection, setting the
** working directory, etc. Currently, itjust executes the tool and
** appends output and error to output and error panes of the message
** manager.
*/
static void execute_tool(GtkMenuItem *item, gpointer data)
{
	AnUserTool *tool = (AnUserTool *) data;
	gchar *command;

#ifdef TOOL_DEBUG
	g_message("Tool: %s (%s)\n", tool->name, tool->command);
#endif
	/* Expand variables to get the full command */
	if (app->current_text_editor && app->current_text_editor->full_filename)
		anjuta_set_file_properties(app->current_text_editor->full_filename);
	command = prop_expand(app->project_dbase->props, tool->command);
	if (NULL == command)
		return;
#ifdef TOOL_DEBUG
	g_message("Command is '%s'", command);
#endif

	/* Set the current working directory */
	if (tool->working_dir)
	{
		gchar *working_dir = prop_expand(app->project_dbase->props
		  , tool->working_dir);
		if (working_dir)
		{
#ifdef TOOL_DEBUG
			g_message("Working dir is %s", working_dir);
#endif
			anjuta_set_execution_dir(working_dir);
			chdir(working_dir);
			g_free(working_dir);
		}
	}
	if (tool->detached)
	{
		/* Detached mode - execute and forget about it */
		if(tool->run_in_terminal)
		{
			gchar* escaped_cmd = anjuta_util_escape_quotes(command);
			prop_set_with_key (app->project_dbase->props
			  , "anjuta.current.command", escaped_cmd);
			g_free(escaped_cmd);
#ifdef TOOL_DEBUG
			g_message("Escaped Command is: %s", escaped_cmd);
#endif
			g_free(command);
			command = command_editor_get_command (app->command_editor
			  , COMMAND_TERMINAL);
		}
		else
		{
			prop_set_with_key (app->project_dbase->props
			  , "anjuta.current.command", command);
		}
		gnome_execute_shell(tool->working_dir, command);
	}
	else
	{
		/* Attached mode - run through launcher */
		current_tool = tool;
		if (tool->error <= MESSAGE_MAX)
			anjuta_message_manager_clear(app->messages, tool->error);
		if (tool->output <= MESSAGE_MAX)
		{
			anjuta_message_manager_clear(app->messages, tool->output);
			anjuta_message_manager_show(app->messages, tool->output);
		}
		if (FALSE == launcher_execute(command, tool_stdout_handler
	  	  , tool_stderr_handler, tool_terminate_handler))
		{
			anjuta_error("%s: Unable to launch!", tool->command);
		}
		g_free(command);
	}
}

/* Activates a tool */
static void an_user_tool_activate(AnUserTool *tool)
{
	GtkWidget *submenu;

	if (NULL == (submenu = an_get_submenu(tool->location)))
		an_user_tool_free(tool, TRUE);
	else
	{
		tool->menu_item = gtk_menu_item_new_with_label(tool->name);
		gtk_widget_ref(tool->menu_item);
		gtk_signal_connect(GTK_OBJECT(tool->menu_item), "activate"
		  , execute_tool, tool);
		gtk_menu_append(GTK_MENU(submenu), tool->menu_item);
		gtk_widget_show(tool->menu_item);
	}
}

/* Loads toolset from a configuration file. Tools properties are saved
** as <property>:<value> pairs seperated by the SEPERATOR character.
** Lines starting with '#' are treated as comments.
*/
static gboolean anjuta_tools_load_from_file(const gchar *file_name)
{
	int fd;
	char *buf;
	struct stat st;
	char menu[256];
	char name[256];
	char command[BUFSIZ];
	int params;
	int n_bytes = 0;
	int bytes_read = 0;
	char *s;
	AnUserTool *tool;

#ifdef TOOLS_DEBUG
	g_message("Loading tools from %s\n", file_name);
#endif
	if (0 != stat(file_name, &st))
	{
		/* This is not an error condition since the user might not have
		** defined any tools, or there might not be any global tools */
		return TRUE;
	}

	if (0 > (fd = open(file_name, O_RDONLY)))
	{
		anjuta_system_error(errno, "Unable to open tools file %s for reading"
		  , file_name);
		return FALSE;
	}

	/* Read the file contents into buffer at one go for speed */
	buf = g_malloc(st.st_size + 1);
	for(n_bytes = 0, bytes_read = 0;st.st_size > n_bytes; n_bytes += bytes_read)
	{
		if (0 > (bytes_read = read(fd, buf + n_bytes, st.st_size - n_bytes)))
		{
			anjuta_system_error(errno, "Unable to read from tools file %s"
			  , file_name);
			g_free(buf);
			close(fd);
			return FALSE;
		}
	}
	buf[st.st_size] = '\0';
	for (s = buf; (NULL != (tool = an_user_tool_new(&s))); s += 2)
	{
		an_user_tool_activate(tool);
	}
	close(fd);
	return TRUE;
}

gboolean anjuta_tools_load(void)
{
	char file_name[PATH_MAX];

	/* First, load global tools */
	snprintf(file_name, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, TOOLS_FILE);
	anjuta_tools_load_from_file(file_name);

	/* Now, user tools */
	snprintf(file_name, PATH_MAX, "%s/%s", app->dirs->settings, TOOLS_FILE);
	anjuta_tools_load_from_file(file_name);
}

/* While saving, save only the user tools since it is unlikely that the
** user will have permission to write to the system-wide tools file
*/
gboolean anjuta_tools_save(void)
{
	char file[PATH_MAX];
	FILE *f;
	AnUserTool *tool;
	GSList *tmp;

	snprintf(file, PATH_MAX, "%s/%s", app->dirs->settings, TOOLS_FILE);
	if (NULL == (f = fopen(file, "w")))
	{
		anjuta_error("Unable to open %s for writing", file);
		return FALSE;
	}
	for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
	{
		tool = (AnUserTool *) tmp->data;
		if (AN_TSTORE_LOCAL == tool->storage_type)
		{
			if (FALSE == an_user_tool_save(tool, f))
			{
				fclose(f);
				return FALSE;
			}
		}
	}
	fclose(f);
	return TRUE;
}

/* Structure containing the required properties of the tool list GUI */
typedef struct _AnToolList
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean busy;
	GtkCList *clist;
	GtkButton *new_btn;
	GtkButton *edit_btn;
	GtkButton *delete_btn;
	GtkButton *ok_btn;
	int row; /* Current selected row */
	AnUserTool *tool; /* Current selected tool */
} AnToolList;

/* Structure containing the required properties of the tool editor GUI */
typedef struct _AnToolEditor
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean busy;
	GtkEditable *name_en;
	GtkEditable *location_en;
	GtkEditable *command_en;
	GtkEditable *dir_en;
	GtkToggleButton *enabled_tb;
	GtkToggleButton *detached_tb;
	GtkToggleButton *terminal_tb;
	GtkToggleButton *file_tb;
	GtkToggleButton *project_tb;
	GtkEditable *input_en;
	GtkEditable *output_en;
	GtkCombo *output_com;
	GtkEditable *error_en;
	GtkCombo *error_com;
	GtkEditable *shortcut_en;
	GtkEditable *icon_en;
	GtkButton *ok_btn;
	GtkButton *cancel_btn;
	AnUserTool *tool;
	gboolean editing;
} AnToolEditor;

static AnToolList *tl = NULL;
static AnToolEditor *ted = NULL;

#define GLADE_FILE "anjuta.glade"
#define TOOL_LIST "dialog.tool.list"
#define TOOL_EDITOR "dialog.tool.edit"
#define TOOL_CLIST "tools.clist"
#define OK_BUTTON "button.ok"
#define CANCEL_BUTTON "button.cancel"
#define NEW_BUTTON "button.new"
#define EDIT_BUTTON "button.edit"
#define DELETE_BUTTON "button.delete"
#define TOOL_NAME "tool.name"
#define TOOL_LOCATION "tool.location"
#define TOOL_COMMAND "tool.command"
#define TOOL_WORKING_DIR "tool.working_dir"
#define TOOL_ENABLED "tool.enabled"
#define TOOL_DETACHED "tool.detached"
#define TOOL_TERMINAL "tool.run_in_terminal"
#define TOOL_FILE_LEVEL "tool.file_level"
#define TOOL_PROJECT_LEVEL "tool.project_level"
#define TOOL_INPUT "tool.input"
#define TOOL_OUTPUT "tool.output"
#define TOOL_OUTPUT_COMBO "tool.output.combo"
#define TOOL_ERROR "tool.error"
#define TOOL_ERROR_COMBO "tool.error.combo"
#define TOOL_SHORTCUT "tool.shortcut"
#define TOOL_ICON "tool.icon"

/* Start the tool lister and editor */
gboolean anjuta_tools_edit(void)
{
	GSList *tmp;
	char *s[2];
	int row;

	if (NULL == tl)
	{
		char glade_file[PATH_MAX];

		tl = g_new0(AnToolList, 1);
		snprintf(glade_file, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, GLADE_FILE);
		if (NULL == (tl->xml = glade_xml_new(glade_file, TOOL_LIST)))
		{
			anjuta_error("Unable to build user interface for tool list");
			g_free(tl);
			tl = NULL;
			return FALSE;
		}
		tl->dialog = glade_xml_get_widget(tl->xml, TOOL_LIST);
		gtk_window_set_transient_for (GTK_WINDOW(tl->dialog)
		  , GTK_WINDOW(app->widgets.window));
		gtk_widget_ref(tl->dialog);
		tl->clist = (GtkCList *) glade_xml_get_widget(tl->xml, TOOL_CLIST);
		gtk_widget_ref((GtkWidget *) tl->clist);
		tl->new_btn = (GtkButton *) glade_xml_get_widget(tl->xml, NEW_BUTTON);
		gtk_widget_ref((GtkWidget *) tl->new_btn);
		tl->edit_btn = (GtkButton *) glade_xml_get_widget(tl->xml, EDIT_BUTTON);
		gtk_widget_ref((GtkWidget *) tl->edit_btn);
		tl->delete_btn = (GtkButton *) glade_xml_get_widget(tl->xml, DELETE_BUTTON);
		gtk_widget_ref((GtkWidget *) tl->delete_btn);
		tl->ok_btn = (GtkButton *) glade_xml_get_widget(tl->xml, OK_BUTTON);
		gtk_widget_ref((GtkWidget *) tl->ok_btn);
		tl->row = -1;
		gtk_widget_set_sensitive((GtkWidget *) tl->edit_btn, FALSE);
		gtk_widget_set_sensitive((GtkWidget *) tl->delete_btn, FALSE);
		glade_xml_signal_autoconnect(tl->xml);
	}
	if (tl->busy)
		return FALSE;
	gtk_clist_clear(tl->clist);
	s[1] = "";
	for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
	{
		s[0] = ((AnUserTool *) tmp->data)->name;
		row = gtk_clist_append(tl->clist, s);
		gtk_clist_set_row_data(tl->clist, row, tmp->data);
	}
	tl->busy = TRUE;
	gtk_widget_show(tl->dialog);
	return TRUE;
}

struct {
	int output;
	char *name;
} output_strings[] = {
  { MESSAGE_BUILD, "Build Pane" }
, { MESSAGE_DEBUG, "Debug Pane" }
, { MESSAGE_FIND, "Find Pane" }
, { MESSAGE_CVS, "CVS Pane" }
, { MESSAGE_TERMINAL, "Terminal Pane" }
, { MESSAGE_STDOUT, "Stdout Pane" }
, { MESSAGE_STDERR, "Stderr Pane" }
, { AN_TBUF_NEW, "New Buffer" }
, { AN_TBUF_REPLACE, "Replace Buffer" }
, { AN_TBUF_INSERT, "Insert in Buffer" }
, { AN_TBUF_APPEND, "Append to Buffer" }
, { -1, "" }
};

static int output_from_string(const char *str)
{
	int i = 0;

	while (-1 != output_strings[i].output)
	{
		if (0 == strcmp(output_strings[i].name, str))
			return output_strings[i].output;
		++ i;
	}
	return -1;
}

static const char *string_from_output(int output)
{
	int i = 0;
	while (-1 != output_strings[i].output)
	{
		if (output_strings[i].output == output)
			return output_strings[i].name;
		++ i;
	}
	return "";
}

static GList *glist_from_output(void)
{
	GList *out_list = NULL;
	int i = 0;
	while (-1 != output_strings[i].output)
	{
		out_list = g_list_append(out_list, output_strings[i].name);
		++ i;
	}
	return out_list;
}

static void clear_tool_editor()
{
	if (ted)
	{
		gtk_editable_delete_text(ted->name_en, 0, -1);
		gtk_editable_delete_text(ted->location_en, 0, -1);
		gtk_editable_delete_text(ted->command_en, 0, -1);
		gtk_editable_delete_text(ted->dir_en, 0, -1);
		gtk_toggle_button_set_active(ted->enabled_tb, TRUE);
		gtk_toggle_button_set_active(ted->detached_tb, FALSE);
		gtk_toggle_button_set_active(ted->terminal_tb, FALSE);
		gtk_toggle_button_set_active(ted->file_tb, FALSE);
		gtk_toggle_button_set_active(ted->project_tb, FALSE);
		gtk_editable_delete_text(ted->input_en, 0, -1);
		gtk_editable_delete_text(ted->output_en, 0, -1);
		gtk_editable_delete_text(ted->error_en, 0, -1);
		gtk_editable_delete_text(ted->shortcut_en, 0, -1);
		gtk_editable_delete_text(ted->icon_en, 0, -1);
	}
}

static void populate_tool_editor(void)
{
	int pos;
	if (ted && ted->tool)
	{
		if (ted->tool->name)
			gtk_editable_insert_text(ted->name_en, ted->tool->name
			  , strlen(ted->tool->name), &pos);
		if (ted->tool->location)
		{
			gtk_editable_insert_text(ted->location_en, ted->tool->location
			  , strlen(ted->tool->location), &pos);
		}
		if (ted->tool->command)
		{
			gtk_editable_insert_text(ted->command_en, ted->tool->command
			  , strlen(ted->tool->command), &pos);
		}
		if (ted->tool->working_dir)
		{
			gtk_editable_insert_text(ted->dir_en, ted->tool->working_dir
			  , strlen(ted->tool->working_dir), &pos);
		}
		gtk_toggle_button_set_active(ted->enabled_tb, ted->tool->enabled);
		gtk_toggle_button_set_active(ted->detached_tb, ted->tool->detached);
		gtk_toggle_button_set_active(ted->terminal_tb, ted->tool->run_in_terminal);
		gtk_toggle_button_set_active(ted->file_tb, ted->tool->file_level);
		gtk_toggle_button_set_active(ted->project_tb, ted->tool->project_level);
		if (ted->tool->input)
		{
			gtk_editable_insert_text(ted->input_en, ted->tool->input
			  , strlen(ted->tool->input), &pos);
		}
		if (ted->tool->output >= 0)
		{
			const char *s = string_from_output(ted->tool->output);
			if (s)
				gtk_editable_insert_text(ted->output_en, s, strlen(s), &pos);
		}
		if (ted->tool->error >= 0)
		{
			const char *s = string_from_output(ted->tool->error);
			if (s)
				gtk_editable_insert_text(ted->error_en, s, strlen(s), &pos);
		}
		if (ted->tool->shortcut)
		{
			gtk_editable_insert_text(ted->shortcut_en, ted->tool->shortcut
			  , strlen(ted->tool->shortcut), &pos);
		}
		if (ted->tool->icon)
		{
			gtk_editable_insert_text(ted->icon_en, ted->tool->icon
			  , strlen(ted->tool->icon), &pos);
		}
	}
}

static void set_tool_editor_widget_sensitivity(gboolean state)
{
	gtk_widget_set_sensitive((GtkWidget *) ted->input_en, !state);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_en, !state);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_com, !state);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_en, !state);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_com, !state);
	gtk_widget_set_sensitive((GtkWidget *) ted->terminal_tb, state);	
}

static gboolean show_tool_editor(AnUserTool *tool, gboolean editing)
{
	if (NULL == ted)
	{
		char glade_file[PATH_MAX];
		GList *strlist;

		ted = g_new0(AnToolEditor, 1);
		snprintf(glade_file, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, GLADE_FILE);
		if (NULL == (ted->xml = glade_xml_new(glade_file, TOOL_EDITOR)))
		{
			anjuta_error(_("Unable to build user interface for tool editor"));
			g_free(ted);
			ted = NULL;
			return FALSE;
		}
		ted->dialog = glade_xml_get_widget(ted->xml, TOOL_EDITOR);
		gtk_window_set_transient_for (GTK_WINDOW(ted->dialog)
		  , GTK_WINDOW(app->widgets.window));
		gtk_widget_ref(ted->dialog);
		ted->name_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_NAME);
		gtk_widget_ref((GtkWidget *) ted->name_en);
		ted->location_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_LOCATION);
		gtk_widget_ref((GtkWidget *) ted->location_en);
		ted->command_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_COMMAND);
		gtk_widget_ref((GtkWidget *) ted->command_en);
		ted->dir_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_WORKING_DIR);
		gtk_widget_ref((GtkWidget *) ted->dir_en);
		ted->enabled_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_ENABLED);
		gtk_widget_ref((GtkWidget *) ted->enabled_tb);
		ted->detached_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_DETACHED);
		gtk_widget_ref((GtkWidget *) ted->detached_tb);
		ted->terminal_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_TERMINAL);
		gtk_widget_ref((GtkWidget *) ted->terminal_tb);
		ted->file_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_FILE_LEVEL);
		gtk_widget_ref((GtkWidget *) ted->file_tb);
		ted->project_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_PROJECT_LEVEL);
		gtk_widget_ref((GtkWidget *) ted->project_tb);
		ted->input_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_INPUT);
		gtk_widget_ref((GtkWidget *) ted->input_en);
		ted->output_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_OUTPUT);
		gtk_widget_ref((GtkWidget *) ted->output_en);
		ted->output_com = (GtkCombo *) glade_xml_get_widget(ted->xml, TOOL_OUTPUT_COMBO);
		gtk_widget_ref((GtkWidget *) ted->output_com);
		ted->error_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_ERROR);
		gtk_widget_ref((GtkWidget *) ted->error_en);
		ted->error_com = (GtkCombo *) glade_xml_get_widget(ted->xml, TOOL_ERROR_COMBO);
		gtk_widget_ref((GtkWidget *) ted->error_com);
		ted->shortcut_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_SHORTCUT);
		gtk_widget_ref((GtkWidget *) ted->shortcut_en);
		ted->icon_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_ICON);
		gtk_widget_ref((GtkWidget *) ted->icon_en);
		ted->ok_btn = (GtkButton *) glade_xml_get_widget(ted->xml, OK_BUTTON);
		gtk_widget_ref((GtkWidget *) ted->ok_btn);
		ted->cancel_btn = (GtkButton *) glade_xml_get_widget(ted->xml, CANCEL_BUTTON);
		gtk_widget_ref((GtkWidget *) ted->cancel_btn);
		strlist = glist_from_output();
		gtk_combo_set_popdown_strings(ted->output_com, strlist);
		gtk_combo_set_popdown_strings(ted->error_com, strlist);
		g_list_free(strlist);
		glade_xml_signal_autoconnect(ted->xml);
	}
	if (ted->busy)
		return FALSE;
	clear_tool_editor();
	ted->editing = editing;
	if (tool)
	{
		ted->tool = tool;
		populate_tool_editor();
		set_tool_editor_widget_sensitivity(ted->tool->detached);
	}
	else
		ted->tool = NULL;
	ted->busy = TRUE;
	gtk_widget_show(ted->dialog);
	return TRUE;
}

/* Callbacks for tool list GUI */
void on_user_tool_select(GtkCList *clist, gint row, gint column,
  GdkEventButton *event, gpointer user_data)
{
	tl->row = row;
	tl->tool = (AnUserTool *) gtk_clist_get_row_data(clist, row);
	gtk_widget_set_sensitive((GtkWidget *) tl->edit_btn, TRUE);
	gtk_widget_set_sensitive((GtkWidget *) tl->delete_btn, TRUE);
	if (event && GDK_2BUTTON_PRESS == event->type && 1 == event->button)
	{
		gtk_button_clicked(tl->edit_btn);
	}
}

void on_user_tool_unselect(GtkCList *clist, gint row, gint column,
  GdkEventButton *event, gpointer user_data)
{
	tl->row = -1;
	tl->tool = NULL;
	gtk_widget_set_sensitive((GtkWidget *) tl->edit_btn, FALSE);
	gtk_widget_set_sensitive((GtkWidget *) tl->delete_btn, FALSE);
}

void on_user_tool_ok_clicked(GtkButton *button, gpointer user_data)
{
	/* Don't hide the tool list if a tool is being edited */
	if ((NULL == ted) || (TRUE != ted->busy))
	{
		GSList *tmp;
		AnUserTool *tool;
		anjuta_tools_save();
		for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
		{
			tool = (AnUserTool *) tmp->data;
			if (NULL == tool->menu_item)
				an_user_tool_activate(tool);
		}
		gtk_widget_hide(tl->dialog);
		tl->busy = FALSE;
	}
}

void on_user_tool_edit_clicked(GtkButton *button, gpointer user_data)
{
	if (0 <= tl->row)
		show_tool_editor(tl->tool, TRUE);
}

void on_user_tool_new_clicked(GtkButton *button, gpointer user_data)
{
	show_tool_editor(tl->tool, FALSE);
}

static void really_delete_tool(GtkButton * button, gpointer user_data)
{
	if (tl->tool)
	{
		an_user_tool_free(tl->tool, TRUE);
		tl->tool = NULL;
		gtk_clist_remove(tl->clist, tl->row);
		tl->row = -1;
	}
}

void on_user_tool_delete_clicked(GtkButton *button, gpointer user_data)
{
	/* Don't allow deletes if a tool is being edited */
	if (ted && ted->busy)
		return;
	if (tl->tool)
	{
		char question[1000];
		snprintf(question, 10000, N_("Are you sure you want to delete tool '%s'")
		  , tl->tool->name);
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		  _(question), GNOME_STOCK_BUTTON_NO, GNOME_STOCK_BUTTON_YES, NULL
		  , GTK_SIGNAL_FUNC(really_delete_tool), NULL);
	}
}

static AnUserTool *an_user_tool_from_gui(void)
{
	char *s;
	AnUserTool *tool = g_new0(AnUserTool, 1);
	tool->name = gtk_editable_get_chars(ted->name_en, 0, -1);
	tool->location = gtk_editable_get_chars(ted->location_en, 0, -1);
	tool->command = gtk_editable_get_chars(ted->command_en, 0, -1);
	tool->working_dir = gtk_editable_get_chars(ted->dir_en, 0, -1);
	tool->enabled = gtk_toggle_button_get_active(ted->enabled_tb);
	tool->detached = gtk_toggle_button_get_active(ted->detached_tb);
	tool->run_in_terminal = gtk_toggle_button_get_active(ted->terminal_tb);
	tool->file_level = gtk_toggle_button_get_active(ted->file_tb);
	tool->project_level = gtk_toggle_button_get_active(ted->project_tb);
	tool->input = gtk_editable_get_chars(ted->input_en, 0, -1);
	s = gtk_editable_get_chars(ted->output_en, 0, -1);
	tool->output = output_from_string(s);
	g_free(s);
	s = gtk_editable_get_chars(ted->error_en, 0, -1);
	tool->error = output_from_string(s);
	g_free(s);
	tool->shortcut = gtk_editable_get_chars(ted->shortcut_en, 0, -1);
	tool->icon = gtk_editable_get_chars(ted->icon_en, 0, -1);
	return tool;
}

/* Callbacks for the tool editor GUI */

on_user_tool_edit_detached_toggled(GtkToggleButton *tb, gpointer user_data)
{
	gboolean state = gtk_toggle_button_get_active(tb);
	/* For a tool in detached mode, input, output and error cannot be
	** redirected */
	set_tool_editor_widget_sensitivity(state);
}

void on_user_tool_edit_ok_clicked(GtkButton *button, gpointer user_data)
{
	AnUserTool *tool, *tool1 = NULL;
	GSList *tmp;
	int row;
	char *s[2];

	tool = an_user_tool_from_gui();
	/* Check for all mandatory fields */
	if (!tool->name || '\0' == tool->name[0])
	{
		anjuta_error("You must provide a tool name!");
		an_user_tool_free(tool, FALSE);
		return;
	}
	if (!tool->command || '\0' == tool->command[0])
	{
		anjuta_error("You must provide a tool command!");
		an_user_tool_free(tool, FALSE);
		return;
	}
	if (NULL == tool_hash)
		tool_hash = g_hash_table_new(g_str_hash, g_str_equal);
	else
	{
		tool1 = g_hash_table_lookup(tool_hash, tool->name);
		if (tool1)
		{
			/* A tool with the same name exists */
			if (!ted->editing || tool1 != ted->tool)
			{
				anjuta_error_parented(ted->dialog, "A tool with the name '%s' already exists!"
				  , tool->name);
				return;
			}
		}
	}
	if (ted->editing)
		an_user_tool_free(ted->tool, TRUE);
	g_hash_table_insert(tool_hash, tool->name, tool);
	tool_list = g_slist_append(tool_list, tool);
	ted->tool = NULL;
	gtk_clist_freeze(tl->clist);
	tl->row = -1;
	tl->tool = NULL;
	gtk_widget_set_sensitive((GtkWidget *) tl->edit_btn, FALSE);
	gtk_widget_set_sensitive((GtkWidget *) tl->delete_btn, FALSE);
	gtk_clist_clear(tl->clist);
	s[1] = "";
	for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
	{
		s[0] = ((AnUserTool *) tmp->data)->name;
		row = gtk_clist_append(tl->clist, s);
		gtk_clist_set_row_data(tl->clist, row, tmp->data);
	}
	gtk_clist_thaw(tl->clist);
	gtk_widget_hide(ted->dialog);
	ted->busy = FALSE;
}

void on_user_tool_edit_cancel_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_hide(ted->dialog);
	ted->busy = FALSE;
}

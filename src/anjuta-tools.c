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
	1) (M) Add new menu items associated with external commands.
	2) (N) Add drop-down toolbar item for easy access to all tools.
	3) (R) Should be appendable under any of the top/sub level menus.
	4) (N) Should be able to associate icons.
	5) (R) Should be able to associate shortcuts.

R2: Command line parameters
	1) (M) Pass variable command-line parameters to the tool.
	2) (R) Use existing properties system to pass parameters.
	3) (R) Ask user at run-time for parameters.

R3: Working directory
	1) (M) Specify working directory for the tool.
	2) (R) Ability to specify property variables for working dir.

R4: Standard input to the tool
	1) (R) Specify current buffer as standard input.
	2) (R) Specify property variables as standard input.
	3) (N) Specify list of open files as standard input.

R5: Output and error redirection
	1) (M) Output to any of the message pane windows.
	2) (M) Run in terminal.
	3) (R) Output to current/new buffer.
	4) (N) Show output in a popup window.

R6: Tool manipulation GUI
	1) (M) Add/remove tools with all options.
	2) (R) Enable/disable tool loading.

R7: Tool Storage
	1) (M) Gloabal and local tool storage.
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

#include "scroll-menu.h"
#include "main_menubar.h"
#include "message-manager.h"
#include "widget-registry.h"
#include "anjuta.h"
#include "anjuta-tools.h"

#define SEPERATOR '\001'
#define LINESEP '\002'
#define TOOLS_FILE "tools.properties"

/** Defines what type of data to pass to the standard input of the tool */
typedef enum _AnToolInput
{
	AN_TINP_NONE,
	AN_TIMP_BUFFER,
	AN_TIMP_SELECTION,
	AN_TIMP_VARIABLE
} AnToolInput;

/** Defines what to do with the output of the tool */
typedef enum _AnToolOutputType
{
	AN_TOUT_NONE,
	AN_TOUT_MESSAGE,
	AN_TOUT_BUFFER,
} AnToolOutputType;

/** Defines how output should be added to buffer */
typedef enum _AnToolBufferAction
{
	AN_TBUF_INSERT, /* Insert at cursor position in the current buffer */
	AN_TBUF_REPLACE, /* Replace exisitng buffer content with output */
	AN_TBUF_APPEND, /* Append output to the end of the buffer */
	AN_TBUF_NEW /* Create a new buffer and put output into it */
} AnToolBufferAction;

/** Defines how and where tool information will be stored. */
typedef enum _AnToolStore
{
	AN_TSTORE_GLOBAL,
	AN_TSTORE_LOCAL,
	AN_TSTORE_PROJECT
} AnToolStore;

/** Defines how stdout and stderr will be handled */
typedef struct _AnToolOutput
{
	AnToolOutputType type;
	int target; /* AnToolBufferAction or AnMessageType */
} AnToolOutput;

typedef struct _AnUserTool
{
	gchar *name;
	gchar *command;
	gboolean enabled;
	gboolean file_level;
	gboolean project_level;
	gboolean run_in_terminal;
	gchar *location;
	gchar *pixmap;
	gchar *shortcut;
	gchar *working_dir;
	AnToolInput input_type;
	gchar *input_var;
	AnToolOutput output;
	AnToolOutput error;
	AnToolStore storage_type;
	GtkWidget *menu_item;
} AnUserTool;

/* Maintains a list of all tools loaded */
static GSList *tool_list = NULL;

/* Maintains a hash table of tool name vs. tool object pointer to resolve
** name conflicts (local version will override global version. Multiple
** tools with the same name will be flagged but this is not an error
** condition
*/
static GHashTable *tool_hash = NULL;

/* Destroys memory allocated to an user tool */
#define FREE(x) if (x) g_free(x)
static void an_user_tool_free(AnUserTool *tool)
{
	if (tool)
	{
		FREE(tool->name);
		FREE(tool->command);
		FREE(tool->location);
		FREE(tool->pixmap);
		FREE(tool->shortcut);
		FREE(tool->working_dir);
		FREE(tool->input_var);
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
	AnUserTool *tool;
	char *s = *buf;
	char *name;
	char *value;
	char *seppos;
	char *colpos;

#ifdef TOOL_DEBUG
	fprintf(stderr, "Got line: '%s'\n", buf);
#endif

	tool = g_new0(AnUserTool, 1);
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
		else NUMMATCH(run_in_terminal)
		else STRMATCH(location)
		else STRMATCH(pixmap)
		else STRMATCH(shortcut)
		else STRMATCH(working_dir)
		else NUMMATCH(input_type)
		else STRMATCH(input_var)
		else NUMMATCH(output.type)
		else NUMMATCH(output.target)
		else NUMMATCH(error.type)
		else NUMMATCH(error.target)
		s = seppos + 1;
	}
	*buf = s;
	/* Check if the minimum required fields were populated */
	if (NULL == tool->name || NULL == tool->command)
	{
#ifdef TOOL_DEBUG
		fprintf(stderr, "Got a tool with no name or command!\n");
#endif
		an_user_tool_free(tool);
		return NULL;
	}
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
#define STRWRITE(p) if (1 > fprintf(f, "%s:%s%c", #p, tool->p, SEPERATOR)) return FALSE
#define NUMWRITE(p) if (1 > fprintf(f, "%s:%d%c", #p, tool->p, SEPERATOR)) return FALSE
static gboolean an_user_tool_save(AnUserTool *tool, FILE *f)
{
	STRWRITE(name);
	STRWRITE(command);
	NUMWRITE(enabled);
	NUMWRITE(file_level);
	NUMWRITE(project_level);
	NUMWRITE(run_in_terminal);
	STRWRITE(location);
	STRWRITE(pixmap);
	STRWRITE(shortcut);
	STRWRITE(working_dir);
	NUMWRITE(input_type);
	STRWRITE(input_var);
	NUMWRITE(output.type);
	NUMWRITE(output.target);
	NUMWRITE(error.type);
	NUMWRITE(output.target);
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
		anjuta_message_manager_append (app->messages, line, MESSAGE_STDOUT);
	}
}

/* Simplistic error handler - needs to be enhanced */
static void tool_stderr_handler(gchar *line)
{
	if (line)
	{
		anjuta_message_manager_append (app->messages, line, MESSAGE_STDERR);
	}
}

/* Simplistic termination handler - needs to be enhanced */
static void tool_terminate_handler(gint status, time_t time)
{
	char line[BUFSIZ];
	snprintf(line, BUFSIZ, "Tool terminated with status %d\n", status);
	anjuta_message_manager_append(app->messages, line, MESSAGE_STDOUT);
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
			chdir(working_dir);
			g_free(working_dir);
		}
	}
	anjuta_message_manager_clear(app->messages, MESSAGE_STDOUT);
	anjuta_message_manager_clear(app->messages, MESSAGE_STDERR);
	anjuta_message_manager_show(app->messages, MESSAGE_STDOUT);
	if (FALSE == launcher_execute(command, tool_stdout_handler
	  , tool_stderr_handler, tool_terminate_handler))
	{
		anjuta_error("%s: Unable to launch!", tool->command);
	}
	g_free(command);
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
	GtkWidget *submenu;
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
		if (NULL == (submenu = an_get_submenu(tool->location)))
			an_user_tool_free(tool);
		else
		{
			tool->menu_item = gtk_menu_item_new_with_label(tool->name);
			gtk_widget_ref(tool->menu_item);
			gtk_signal_connect(GTK_OBJECT(tool->menu_item), "activate"
			  , execute_tool, tool);
			gtk_menu_append(GTK_MENU(submenu), tool->menu_item);
			gtk_widget_show(tool->menu_item);
			tool_list = g_slist_prepend(tool_list, tool);
		}
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

/* Tool editor - GUI for defining user tools */
gboolean anjuta_tools_edit(void)
{
	anjuta_error("Tool editor not yet implemented!");
	return FALSE;
}

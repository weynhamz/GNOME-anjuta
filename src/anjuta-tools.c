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
	3) (P) Should be appendable under any of the top/sub level menus.
	4) (N) Should be able to associate icons.
	5) (R) Should be able to associate shortcuts.

R2: Command line parameters
	1) (D) Pass variable command-line parameters to the tool.
	2) (D) Use existing properties system to pass parameters.
	3) (N) Ask user at run-time for parameters.

R3: Working directory
	1) (D) Specify working directory for the tool.
	2) (D) Ability to specify property variables for working dir.

R4: Standard input to the tool
	1) (D) Specify current buffer as standard input.
	2) (D) Specify property variables as standard input.
	3) (N) Specify list of open files as standard input.

R5: Output and error redirection
	1) (D) Output to any of the message pane windows.
	2) (D) Run in terminal (detached mode).
	3) (D) Output to current/new buffer.
	4) (D) Show output in a popup window.

R6: Tool manipulation GUI
	1) (D) Add/remove tools with all options.
	2) (D) Enable/disable tool loading.

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
#include <libxml/parser.h>

#include "scroll-menu.h"
#include "main_menubar.h"
#include "message-manager.h"
#include "widget-registry.h"
#include "anjuta.h"
#include "launcher.h"
#include "anjuta-tools.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#define TOOLS_FILE "tools.xml"

/** Defines how output should be handled */
typedef enum _AnToolOutputAction
{
	AN_TBUF_NEW = MESSAGE_MAX + 1, /* Create a new buffer and put output into it */
	AN_TBUF_REPLACE, /* Replace exisitng buffer content with output */
	AN_TBUF_INSERT, /* Insert at cursor position in the current buffer */
	AN_TBUF_APPEND, /* Append output to the end of the buffer */
	AN_TBUF_REPLACESEL, /* Replace the current selection with the output */
	AN_TBUF_POPUP /* Show result in a popup window */
} AnToolOutputAction;

/** Defines what to supply to the standard input of the tool  */
typedef enum _AnToolInputType
{
	AN_TINP_NONE = 0, /* No input */
	AN_TINP_BUFFER, /* Contents of current buffer */
	AN_TINP_STRING /* User defined string (variables will be expanded) */
} AnToolInputType;

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
	gboolean user_params;
	gboolean autosave;
	gchar *location;
	gchar *icon;
	gchar *shortcut;
	gchar *working_dir;
	int input_type; /* AnToolInputType */
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

/* The temporary file to use to store standard input */
static char tmp_file[PATH_MAX] = "";

/* Buffers the output and error lines of the current tool under execution */
GString *current_tool_output = NULL;
GString *current_tool_error = NULL;

/* Destroys memory allocated to an user tool */

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

/* Creates a new tool item from an xml node */
#define STRMATCH(node, key) \
	if (strcmp(node->name, #key) == 0) \
	{ \
		char* text = xmlNodeGetContent(node); \
		tool->key = NULL; \
		if (text) \
		{ \
			tool->key = g_strdup(text); \
			g_free (text); \
		} \
		else \
			g_warning ("Anjuta tools xml parse error: Invalid node"); \
	}

#define NUMMATCH(node, key) \
	if (strcmp(node->name, #key) == 0) \
	{ \
		char* text = xmlNodeGetContent(node); \
		tool->key = 0; \
		if (text) \
		{ \
			tool->key = atoi(text); \
			g_free (text); \
		} \
		else \
			g_warning ("Anjuta tools xml parse error: Invalid node"); \
	}

static AnUserTool *an_user_tool_new(xmlNodePtr tool_node)
{
	AnUserTool *tool, *tool1;
	xmlChar *name;
	xmlNodePtr node;
	
	g_return_val_if_fail (tool_node, NULL);
	if (xmlIsBlankNode(tool_node) || !tool_node->name ||
		strcmp(tool_node->name, "tool") != 0)
	{
		g_warning ("Anjuta tools xml parse error: Invalide Node");
		return NULL;
	}

	tool = g_new0(AnUserTool, 1);
	/* Set default values */
	tool->enabled = TRUE;
	tool->output = MESSAGE_STDOUT;
	tool->error = MESSAGE_STDERR;
	name = xmlGetProp (tool_node, "name");
	if (name)
	{
		tool->name = g_strdup (name);
		g_free (name);
	}
	else
	{
		g_warning ("Anjuta tools xml parse error: Invalide Node");
		an_user_tool_free(tool, FALSE);
		return NULL;
	}
		
	node = tool_node->childs;
	while (node)
	{
		if (!node->name) {
			g_warning ("Anjuta tools xml parse error: Invalid node");
			node = node->next;
			continue;
		}
		/*if*/ STRMATCH(node, command)
		else NUMMATCH(node, enabled)
		else NUMMATCH(node, file_level)
		else NUMMATCH(node, project_level)
		else NUMMATCH(node, detached)
		else NUMMATCH(node, run_in_terminal)
		else NUMMATCH(node, user_params)
		else NUMMATCH(node, autosave)
		else STRMATCH(node, location)
		else STRMATCH(node, icon)
		else STRMATCH(node, shortcut)
		else STRMATCH(node, working_dir)
		else NUMMATCH(node, input_type)
		else STRMATCH(node, input)
		else NUMMATCH(node, output)
		else NUMMATCH(node, error)
		node = node->next;
	}

	/* Check if the minimum required fields were populated */
	if (NULL == tool->name || NULL == tool->command)
	{
		g_warning ("Got a tool with no name or command!\n");
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

/* Writes tool information to the given file in xml format */
#define STRWRITE(p) if (tool->p && '\0' != tool->p[0]) \
	{\
		if (1 > fprintf(f, "\t\t<%s><![CDATA[%s]]></%s>\n", #p, tool->p, #p))\
			return FALSE;\
	}
#define NUMWRITE(p) if (1 > fprintf(f, "\t\t<%s>%d</%s>\n", #p, tool->p, #p))\
	{\
		return FALSE;\
	}
static gboolean an_user_tool_save(AnUserTool *tool, FILE *f)
{
	fprintf (f, "\t<tool name=\"%s\">\n", tool->name);
	STRWRITE(command)
	NUMWRITE(enabled)
	NUMWRITE(file_level)
	NUMWRITE(project_level)
	NUMWRITE(detached)
	NUMWRITE(run_in_terminal)
	NUMWRITE(user_params)
	NUMWRITE(autosave)
	STRWRITE(location)
	STRWRITE(icon)
	STRWRITE(shortcut)
	STRWRITE(working_dir)
	NUMWRITE(input_type)
	STRWRITE(input)
	NUMWRITE(output)
	NUMWRITE(error)
	fprintf (f, "\t</tool>\n");
	return TRUE;
}

/* Simplistic output handler - needs to be enhanced */
static void tool_stdout_handler(gchar *line)
{
	if (line && current_tool)
	{
		if (current_tool->output <= MESSAGE_MAX && current_tool->output >= 0)
		{
			/* Send the message to the proper message pane */
			anjuta_message_manager_append (app->messages, line
			  ,current_tool->output);
		}
		else
		{
			/* Store the line so that proper action can be taken once
			the tool has finished running */
			if (NULL == current_tool_output)
				current_tool_output = g_string_new(line);
			else
				g_string_append(current_tool_output, line);
		}
	}
}

/* Simplistic error handler - needs to be enhanced */
static void tool_stderr_handler(gchar *line)
{
	if (line && current_tool)
	{
		if (current_tool->error <= MESSAGE_MAX && current_tool->error >= 0)
		{
			/* Simply send the message to the proper message pane */
			anjuta_message_manager_append (app->messages, line
			  , current_tool->error);
		}
		else
		{
			/* Store the line so that proper action can be taken once
			the tool has finished running */
			if (NULL == current_tool_error)
				current_tool_error = g_string_new(line);
			else
				g_string_append(current_tool_error, line);
		}
	}
}

/* Handle non-message output and error lines from current tool */
static void handle_tool_output(int type, GString *s, gboolean is_error)
{
	TextEditor *te;
	int sci_message = -1;

	if (AN_TBUF_POPUP == type)
	{
		messagebox(is_error ? GNOME_MESSAGE_BOX_ERROR : GNOME_MESSAGE_BOX_INFO
		  , s->str);
		return;
	}
	if (AN_TBUF_NEW == type || (NULL == app->current_text_editor))
		te = anjuta_append_text_editor(NULL);
	else
		te = app->current_text_editor;
	if (NULL == te)
	{
		anjuta_error("Unable to create new text buffer");
		return;
	}

	switch(type)
	{
		case AN_TBUF_NEW:
		case AN_TBUF_INSERT:
			sci_message = SCI_ADDTEXT;
			break;
		case AN_TBUF_REPLACE:
			sci_message = SCI_SETTEXT;
			break;
		case AN_TBUF_APPEND:
			sci_message = SCI_APPENDTEXT;
			break;
		case AN_TBUF_REPLACESEL:
			sci_message = SCI_REPLACESEL;
			break;
		default:
			anjuta_error("Got invalid tool output/error redirection message");
			return;
	}
	scintilla_send_message(SCINTILLA(te->widgets.editor), sci_message
	  , s->len, (long) s->str);
}

/* Termination handler
** Nothing much to do if output and error messages have been sent to the
** message panes. Otherwise, we need to decide what to do with the output
** and error and do it.
*/
static void tool_terminate_handler(gint status, time_t time)
{
	if (current_tool)
	{
		if (current_tool->error <= MESSAGE_MAX && current_tool->error >= 0)
		{
			char line[BUFSIZ];
			snprintf(line, BUFSIZ, "Tool terminated with status %d\n", status);
			anjuta_message_manager_append(app->messages, line
			  , current_tool->error);
		}
		else if (current_tool_error && (0 < current_tool_error->len))
		{
			handle_tool_output(current_tool->error, current_tool_error, TRUE);
		}
		if (current_tool->output <= MESSAGE_MAX && current_tool->output >= 0)
		{
			/* Nothing to do here */
		}
		else if (current_tool_output && (0 < current_tool_output->len))
		{
			handle_tool_output(current_tool->output, current_tool_output, FALSE);
		}
		if ('\0' != tmp_file[0])
		{
			unlink(tmp_file);
			tmp_file[0] = '\0';
		}
		current_tool = NULL;
	}
}

/* Popup a dialog to ask for user parameters */
static char *get_user_params(AnUserTool *tool, gboolean *button);

/* Menu activate handler which executes the tool. It should do command
** substitution, input, output and error redirection, setting the
** working directory, etc. Currently, it just executes the tool and
** appends output and error to output and error panes of the message
** manager.
*/
static void execute_tool(GtkMenuItem *item, gpointer data)
{
	AnUserTool *tool = (AnUserTool *) data;
	char *params = NULL;
	gchar *command;

#ifdef TOOL_DEBUG
	g_message("Tool: %s (%s)\n", tool->name, tool->command);
#endif	
	/* Ask for user parameters if required */
	if (tool->user_params)
	{
		gboolean button;
		params = get_user_params(tool, &button);
		if (button != 0) /* No OK button clicked */
			return;
	}
	if (tool->autosave)
	{
		anjuta_save_all_files();
	}
	/* Expand variables to get the full command */
	if (app->current_text_editor)
	{
		/* Update file level properties */
		gchar *word;
		anjuta_set_file_properties(app->current_text_editor->full_filename);
		word = text_editor_get_current_word(app->current_text_editor);
		prop_set_with_key(app->preferences->props, "current.file.selection"
		  , word?word:"");
		if (word)
			g_free(word);
	}
	if (params)
	{
		gchar *cmd = g_strconcat(tool->command, " ", params, NULL);
		command = prop_expand(app->project_dbase->props, cmd);
		g_free(cmd);
	}
	else
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
			g_free(command);
			command = command_editor_get_command (app->command_editor
			  , COMMAND_TERMINAL);
		}
		else
		{
			prop_set_with_key (app->project_dbase->props
			  , "anjuta.current.command", command);
		}
#ifdef TOOL_DEBUG
		g_message("Final command: '%s'\n", command);
#endif
		gnome_execute_shell(tool->working_dir, command);
	}
	else
	{
		/* Attached mode - run through launcher */
		char *buf = NULL;
		current_tool = tool;
		if (current_tool_output)
			g_string_truncate(current_tool_output, 0);
		if (current_tool_error)
			g_string_truncate(current_tool_error, 0);
		if (tool->error <= MESSAGE_MAX && tool->error >= 0)
			anjuta_message_manager_clear(app->messages, tool->error);
		if (tool->input_type == AN_TINP_BUFFER && app->current_text_editor)
		{
			long len = scintilla_send_message(
			  SCINTILLA(app->current_text_editor->widgets.editor)
			, SCI_GETLENGTH, 0, 0);
			buf = g_new(char, len+1);
			scintilla_send_message(
			  SCINTILLA(app->current_text_editor->widgets.editor)
			, SCI_GETTEXT, len+1, (long) buf);
		}
		else if ((tool->input_type == AN_TINP_STRING) && (tool->input)
		  && '\0' != tool->input[0])
		{
			buf = prop_expand(app->project_dbase->props, tool->input);
			if (NULL == buf)
				buf = g_strdup("");
		}
		if (buf)
		{
			int fd;
			int buflen = strlen(buf);
			snprintf(tmp_file, PATH_MAX, "/tmp/anjuta.%d.XXXXXX", getpid());
			if (0 > (fd = mkstemp(tmp_file)))
			{
				anjuta_system_error(errno, "Unable to create temporary file %s!"
				  , tmp_file);
				return;
			}
			if (buflen != write(fd, buf, buflen))
			{
				anjuta_system_error(errno, "Unable to write to temporary file %s."
				  , tmp_file);
				return;
			}
			close(fd);
			gchar* escaped_cmd = anjuta_util_escape_quotes(command);
			g_free(command);
			command = g_strconcat("sh -c \"", escaped_cmd, "<", tmp_file, "\"", NULL);
			g_free(buf);
		}
		if (tool->output <= MESSAGE_MAX && tool->output >= 0)
		{
			anjuta_message_manager_clear(app->messages, tool->output);
			anjuta_message_manager_show(app->messages, tool->output);
		}
#ifdef TOOL_DEBUG
	g_message("Final command: '%s'\n", command);
#endif
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

	if (FALSE == tool->enabled)
		return;
	else if (NULL == (submenu = an_get_submenu(tool->location)))
	{
		tool->enabled = FALSE;
		tool->menu_item = NULL;
		return;
	}
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

/* Loads toolset from a xml configuration file.
 * Tools properties are saved xml format */
static gboolean anjuta_tools_load_from_file(const gchar *file_name)
{
	xmlDocPtr xml_doc;
	struct stat st;
	AnUserTool *tool;
	gboolean status = TRUE;

#ifdef TOOL_DEBUG
	g_message("Loading tools from %s\n", file_name);
#endif
	if (0 != stat(file_name, &st))
	{
		/* This is not an error condition since the user might not have
		** defined any tools, or there might not be any global tools */
		return TRUE;
	}
	xml_doc = xmlParseFile (file_name);
	if (xml_doc)
	{
		xmlNodePtr root, node;
		root = xmlDocGetRootElement(xml_doc);
		if (root && root->name && strcmp (root->name, "anjuta-tools") == 0)
		{
			node = root->childs;
			while (node)
			{
				tool = an_user_tool_new(node);
				if (tool) an_user_tool_activate(tool);
				node = node->next;
			}
		}
		else
		{
			g_warning ("Anjuta tools xml parse error: Invalid xml document");
			status = FALSE;
		}
		xmlFreeDoc(xml_doc);
	}
	else
	{
		g_warning ("Anjuta tools xml parse error: Invalid xml document");
		status = FALSE;
	}
	return status;
}

gboolean anjuta_tools_load(void)
{
	char file_name[PATH_MAX];

	/* First, load global tools */
	snprintf(file_name, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, TOOLS_FILE);
	anjuta_tools_load_from_file(file_name);

	/* Now, user tools */
	snprintf(file_name, PATH_MAX, "%s/%s", app->dirs->settings, TOOLS_FILE);
	return anjuta_tools_load_from_file(file_name);
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
	fprintf (f, "<?xml version=\"1.0\"?>\n");
	fprintf (f, "<anjuta-tools>\n");
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
	fprintf (f, "</anjuta-tools>\n");
	fclose(f);
	return TRUE;
}

void anjuta_tools_sensitize(void)
{
	GSList *tmp;
	AnUserTool *tool;

	for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
	{
		tool = (AnUserTool *) tmp->data;
		if (tool->menu_item)
		{
			if ((!app->project_dbase->project_is_open && tool->project_level)
			|| (!app->current_text_editor && tool->file_level))
			{
				gtk_widget_set_sensitive(tool->menu_item, FALSE);
			}
			else
				gtk_widget_set_sensitive(tool->menu_item, TRUE);
		}
	}
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
	GtkToggleButton *autosave_tb;
	GtkToggleButton *file_tb;
	GtkToggleButton *project_tb;
	GtkToggleButton *params_tb;
	GtkEditable *input_type_en;
	GtkCombo *input_type_com;
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

typedef struct _AnToolHelp
{
	GladeXML *xml;
	GtkWidget *dialog;
	GtkCList *clist;
	GtkButton *ok_btn;
	gboolean busy;
} AnToolHelp;

typedef struct _AnToolParams
{
	GladeXML *xml;
	GtkWidget *dialog;
	GtkEditable *params_en;
	GtkCombo *params_com;
} AnToolParams;

static AnToolList *tl = NULL;
static AnToolEditor *ted = NULL;
static AnToolHelp *th = NULL;
static AnToolParams *tp = NULL;

#define GLADE_FILE "anjuta.glade"
#define TOOL_LIST "dialog.tool.list"
#define TOOL_EDITOR "dialog.tool.edit"
#define TOOL_HELP "dialog.tool.help"
#define TOOL_PARAMS "dialog.tool.params"
#define TOOL_CLIST "tools.clist"
#define TOOL_HELP_CLIST "tools.help.clist"
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
#define TOOL_USER_PARAMS "tool.user_params"
#define TOOL_AUTOSAVE "tool.autosave"
#define TOOL_FILE_LEVEL "tool.file_level"
#define TOOL_PROJECT_LEVEL "tool.project_level"
#define TOOL_INPUT_TYPE "tool.input.type"
#define TOOL_INPUT_TYPE_COMBO "tool.input.type.combo"
#define TOOL_INPUT "tool.input"
#define TOOL_OUTPUT "tool.output"
#define TOOL_OUTPUT_COMBO "tool.output.combo"
#define TOOL_ERROR "tool.error"
#define TOOL_ERROR_COMBO "tool.error.combo"
#define TOOL_SHORTCUT "tool.shortcut"
#define TOOL_ICON "tool.icon"
#define TOOL_PARAMS_EN "tool.params"
#define TOOL_PARAMS_EN_COMBO "tool.params.combo"

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
		if (((AnUserTool *) tmp->data)->enabled)
			s[1] = "Enabled";
		else
			s[1] = "";
		row = gtk_clist_append(tl->clist, s);
		gtk_clist_set_row_data(tl->clist, row, tmp->data);
	}
	tl->busy = TRUE;
	gtk_widget_show(tl->dialog);
	return TRUE;
}

StringMap input_type_strings[] = {
  { AN_TINP_NONE, "None" }
, { AN_TINP_BUFFER, "Current buffer" }
, { AN_TINP_STRING, "String" }
, { -1, NULL }
};

StringMap output_strings[] = {
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
, { AN_TBUF_REPLACESEL, "Replace current selection" }
, { AN_TBUF_POPUP, "Show as a popup message" }
, { -1, NULL }
};

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
		gtk_toggle_button_set_active(ted->params_tb, FALSE);
		gtk_toggle_button_set_active(ted->file_tb, FALSE);
		gtk_toggle_button_set_active(ted->project_tb, FALSE);
		gtk_editable_delete_text(ted->input_type_en, 0, -1);
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
		gtk_toggle_button_set_active(ted->params_tb, ted->tool->user_params);
		gtk_toggle_button_set_active(ted->autosave_tb, ted->tool->autosave);
		gtk_toggle_button_set_active(ted->file_tb, ted->tool->file_level);
		gtk_toggle_button_set_active(ted->project_tb, ted->tool->project_level);
		if (ted->tool->input_type)
		{
			const char *s = string_from_type(input_type_strings
			  , ted->tool->input_type);
			if (s)
				gtk_editable_insert_text(ted->input_type_en, s, strlen(s), &pos);
		}
		if (ted->tool->input)
		{
			gtk_editable_insert_text(ted->input_en, ted->tool->input
			  , strlen(ted->tool->input), &pos);
		}
		if (ted->tool->output >= 0)
		{
			const char *s = string_from_type(output_strings
			  , ted->tool->output);
			if (s)
				gtk_editable_insert_text(ted->output_en, s, strlen(s), &pos);
		}
		if (ted->tool->error >= 0)
		{
			const char *s = string_from_type(output_strings
			  , ted->tool->error);
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

static void set_tool_editor_widget_sensitivity(gboolean s1, gboolean s2)
{
	gtk_widget_set_sensitive((GtkWidget *) ted->input_type_en, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->input_type_com, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->input_en, !s1 && s2);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_en, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->output_com, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_en, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->error_com, !s1);
	gtk_widget_set_sensitive((GtkWidget *) ted->terminal_tb, s1);	
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
		ted->params_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_USER_PARAMS);
		gtk_widget_ref((GtkWidget *) ted->params_tb);
		ted->autosave_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_AUTOSAVE);
		gtk_widget_ref((GtkWidget *) ted->autosave_tb);
		ted->file_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_FILE_LEVEL);
		gtk_widget_ref((GtkWidget *) ted->file_tb);
		ted->project_tb = (GtkToggleButton *) glade_xml_get_widget(ted->xml, TOOL_PROJECT_LEVEL);
		gtk_widget_ref((GtkWidget *) ted->project_tb);
		ted->input_type_en = (GtkEditable *) glade_xml_get_widget(ted->xml, TOOL_INPUT_TYPE);
		gtk_widget_ref((GtkWidget *) ted->input_type_en);
		ted->input_type_com = (GtkCombo *) glade_xml_get_widget(ted->xml, TOOL_INPUT_TYPE_COMBO);
		gtk_widget_ref((GtkWidget *) ted->input_type_com);
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
		strlist = glist_from_map(input_type_strings);
		gtk_combo_set_popdown_strings(ted->input_type_com, strlist);
		g_list_free(strlist);
		strlist = glist_from_map(output_strings);
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
		set_tool_editor_widget_sensitivity(ted->tool->detached
		  , (ted->tool->input_type == AN_TINP_STRING));
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
		anjuta_tools_sensitize();
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
	/* Make sure no tool is selected, so that the edit form is cleared
	when a new tool is created */
	if (tl)
		gtk_clist_unselect_all (GTK_CLIST(tl->clist));
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
		  _(question), GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		  GTK_SIGNAL_FUNC(really_delete_tool), NULL, NULL);
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
	tool->user_params = gtk_toggle_button_get_active(ted->params_tb);
	tool->autosave = gtk_toggle_button_get_active(ted->autosave_tb);
	tool->file_level = gtk_toggle_button_get_active(ted->file_tb);
	tool->project_level = gtk_toggle_button_get_active(ted->project_tb);
	s = gtk_editable_get_chars(ted->input_type_en, 0, -1);
	tool->input_type = type_from_string(input_type_strings, s);
	g_free(s);
	tool->input = gtk_editable_get_chars(ted->input_en, 0, -1);
	s = gtk_editable_get_chars(ted->output_en, 0, -1);
	tool->output = type_from_string(output_strings, s);
	g_free(s);
	s = gtk_editable_get_chars(ted->error_en, 0, -1);
	tool->error = type_from_string(output_strings, s);
	g_free(s);
	tool->shortcut = gtk_editable_get_chars(ted->shortcut_en, 0, -1);
	tool->icon = gtk_editable_get_chars(ted->icon_en, 0, -1);
	return tool;
}

/* Callbacks for the tool editor GUI */

void on_user_tool_edit_detached_toggled(GtkToggleButton *tb, gpointer user_data)
{
	int input_type = AN_TINP_NONE;
	char *s = gtk_editable_get_chars(ted->input_type_en, 0, -1);
	gboolean state = gtk_toggle_button_get_active(tb);
	/* For a tool in detached mode, input, output and error cannot be
	** redirected. Also, unless the input type is AN_TINP_STRING,
	** input string should be disabled.	*/
	if (s)
	{
		input_type = type_from_string(input_type_strings, s);
		g_free(s);
	}
	set_tool_editor_widget_sensitivity(state, (AN_TINP_STRING == input_type));
}

void on_user_tool_edit_input_type_changed(GtkEditable *editable, gboolean user_data)
{
	int input_type = AN_TINP_NONE;
	char *s = gtk_editable_get_chars(editable, 0, -1);
	if (s)
	{
		input_type = type_from_string(input_type_strings, s);
		g_free(s);
	}
	gtk_widget_set_sensitive((GtkWidget *) ted->input_en
	  , (AN_TINP_STRING == input_type));
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
		if (((AnUserTool *) tmp->data)->enabled)
			s[1] = "Enabled";
		else
			s[1] = "";
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

/* List of variables understood by the tools editor and their values.
** It is grossly incomplete and I need help here ! - Biswa
*/
static struct
{
	char *name;
	char *value;
} variable_list[] = {
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
, {EDITOR_TAG_POS, "Editor tab position"}
, {EDITOR_TAG_HIDE, "Whether to hide editor tabs"}
, {EDITOR_TABS_ORDERING, "Whether to order editor tabs by file name"}
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
, {CHARACTER_SET, "Current character set"}
, {TERMINAL_FONT, "Font of the embedded terminal"}
, {TERMINAL_SCROLLSIZE, "Scroll buffer of the embedded terminal"}
, {TERMINAL_TERM, "Type (TERM) of the embedded terminal"}
, {TERMINAL_WORDCLASS, "Wordclass (used for selecting words) for terminal"}
, {TERMINAL_BLINK, "Enable blinking cursor for terminal"}
, {TERMINAL_BELL, "Enable terminal bell"}
, {TERMINAL_SCROLL_KEY, "Enable terminal scroll on keystroke"}
, {TERMINAL_SCROLL_OUTPUT, "Enable terminal scroll on output"}
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
, {NULL, NULL}
};

gboolean on_user_tool_edit_help_clicked(GtkButton *button, gpointer user_data)
{
	int len;
	int i = 0;
	char *s[4];

	if (NULL == th)
	{
		char glade_file[PATH_MAX];

		th = g_new0(AnToolHelp, 1);
		snprintf(glade_file, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, GLADE_FILE);
		if (NULL == (th->xml = glade_xml_new(glade_file, TOOL_HELP)))
		{
			anjuta_error("Unable to build user interface for tool help");
			g_free(th);
			th = NULL;
			return FALSE;
		}
		th->dialog = glade_xml_get_widget(th->xml, TOOL_HELP);
		gtk_window_set_transient_for (GTK_WINDOW(th->dialog)
		  , GTK_WINDOW(app->widgets.window));
		gtk_widget_ref(th->dialog);
		th->clist = (GtkCList *) glade_xml_get_widget(th->xml, TOOL_HELP_CLIST);
		gtk_widget_ref((GtkWidget *) th->clist);
		th->ok_btn = (GtkButton *) glade_xml_get_widget(th->xml, OK_BUTTON);
		gtk_widget_ref((GtkWidget *) th->ok_btn);
		glade_xml_signal_autoconnect(th->xml);
	}
	if (th->busy)
		return FALSE;
	gtk_clist_clear(th->clist);
	if (app->current_text_editor)
	{
		/* Update file level properties */
		gchar *word;
		anjuta_set_file_properties(app->current_text_editor->full_filename);
		word = text_editor_get_current_word(app->current_text_editor);
		prop_set_with_key(app->preferences->props, "current.file.selection"
		  , word?word:"");
		if (word)
			g_free(word);
	}
	s[3] = "";
	while (NULL != variable_list[i].name)
	{
		s[0] = variable_list[i].name;
		s[1] = variable_list[i].value;
		s[2] = prop_get(app->project_dbase->props, s[0]);
		if (s[2])
		{
			len = strlen(s[2]);
			if (len > 20)
				s[20] = '\0';
		}
		else
			s[2] = g_strdup("Undefined");
		gtk_clist_append(th->clist, s);
		g_free(s[2]);
		++ i;
	}
	th->busy = TRUE;
	gtk_widget_show(th->dialog);
	return TRUE;
}

void on_user_tool_help_ok_clicked(GtkButton *button, gpointer user_data)
{
	th->busy = FALSE;
	gtk_widget_hide(th->dialog);
}

void anjuta_tools_show_variables()
{
	on_user_tool_edit_help_clicked(NULL, NULL);
}

/* Popup a dialog to ask for user parameters */
static char *get_user_params(AnUserTool *tool, gboolean *button)
{
	if (NULL == tp)
	{
		char glade_file[PATH_MAX];
		char title[256];

		tp = g_new0(AnToolParams, 1);
		snprintf(glade_file, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, GLADE_FILE);
		if (NULL == (tp->xml = glade_xml_new(glade_file, TOOL_PARAMS)))
		{
			anjuta_error("Unable to build user interface for tool parameters");
			g_free(tp);
			tp = NULL;
			return FALSE;
		}
		tp->dialog = glade_xml_get_widget(tp->xml, TOOL_PARAMS);
		snprintf(title, 256, _("%s: Command line parameters"), tool->name);
		gtk_window_set_title (GTK_WINDOW(tp->dialog), title);
		gtk_window_set_transient_for (GTK_WINDOW(tp->dialog)
		  , GTK_WINDOW(app->widgets.window));
		gtk_widget_ref(tp->dialog);
		tp->params_en = (GtkEditable *) glade_xml_get_widget(tp->xml, TOOL_PARAMS_EN);
		gtk_widget_ref((GtkWidget *) tp->params_en);
		tp->params_com = (GtkCombo *) glade_xml_get_widget(tp->xml, TOOL_PARAMS_EN_COMBO);
		gtk_widget_ref((GtkWidget *) tp->params_com);
		gtk_combo_disable_activate (GTK_COMBO(tp->params_com));
		gnome_dialog_editable_enters (GNOME_DIALOG (tp->dialog),
			GTK_EDITABLE(GTK_COMBO(tp->params_com)->entry));
		glade_xml_signal_autoconnect(tp->xml);
	}
	*button = gnome_dialog_run_and_close((GnomeDialog *) tp->dialog);
	return gtk_editable_get_chars(tp->params_en, 0, -1);
}

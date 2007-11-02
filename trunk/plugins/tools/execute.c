/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    execute.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Execute an external tool
 *  
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "execute.h"

#include "variable.h"

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include <glib.h>

#include <ctype.h>

/*---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-tools-plugin.png"
#define MAX_TOOL_PANES 4

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define TOOL_PARAMS "param_dialog"
#define TOOL_PARAMS_EN "tool.params"
#define TOOL_PARAMS_EN_COMBO "tool.params.combo"

/*---------------------------------------------------------------------------*/

/* Output information
 * Allow to have common code for handling stderr and stdout but it includes
 * some part checking if this is used to implement stderr or stdout. So, It
 * is strongly linked to ATPExecutionContext. 
 */
typedef struct
{
	ATPOutputType type;
	struct _ATPExecutionContext* execution;
	IAnjutaMessageView* view;
	gboolean created;
	GString* buffer;
	IAnjutaEditor* editor;
	guint position;
} ATPOutputContext;

/* Execute information
 * This is filled at the beginning with all necessary tool information,
 * so it becomes independent from the tool after creation. It includes
 * two OutputContext (one for stderr and one for stdout). The context
 * is not destroyed when the tool execution terminate, so it can be
 * reuse later for another tool. It useful mainly for keeping pointer
 * on created message panes. All the tool context are kept in a list. */
typedef struct _ATPExecutionContext
{
	gchar* name;
	gchar* directory; // used for opening file from message pane
	ATPOutputContext output;
	ATPOutputContext error;
	AnjutaPlugin *plugin;
	AnjutaLauncher *launcher;
	gboolean busy;
} ATPExecutionContext;

/* Helper function
 *---------------------------------------------------------------------------*/

/* Save all current anjuta files */
static void
save_all_files (AnjutaPlugin *plugin)
{
	IAnjutaDocumentManager *docman;
	IAnjutaFileSavable* save;

	docman = anjuta_shell_get_interface (plugin->shell, IAnjutaDocumentManager, NULL);
	/* No document manager, so no file to save */
	if (docman != NULL)
	{
		save = IANJUTA_FILE_SAVABLE (docman);
		if (save) ianjuta_file_savable_save (save, NULL);
	}
}

/* Replace variable in source and add prefix separated with a space */
static gchar*
replace_variable (const gchar* prefix,  const gchar* source,
				  ATPVariable* variable)
{
	guint len;
	gchar *val;
	GString* str;

	/* Create string and add prefix */
	str = g_string_new (prefix);
	if (prefix != NULL)
	{
		g_string_append_c (str, ' ');
	}

	/* Add source and replace variable */	
	if (source != NULL)
	{
		for (; *source != '\0'; source += len)
		{
			if (source[0] != '$')
			{	
				/* Just append anything that doesn't looks like a variable */
				for (len = 0; (source[len] != '\0') &&
					 (source[len] != '$'); len++);
				g_string_append_len (str, source, len);
			}
			else if (source[1] != '(')
			{
				g_string_append_c (str, '$');
				len = 1;
			}
			else
			{
				/* Check if there is a variable */
				for (len = 2; g_ascii_isalnum(source[len])
						|| (source[len] == '_') ; len++);
				if (source[len] == ')')
				{	
					len++;
					val = atp_variable_get_value_from_name_part (variable, source + 2, len - 3);
					if (val)
					{
						/* This is really a variable, replace */
						g_string_append (str, val);
						continue;
					}
				}

				/* It's not a variable */
				g_string_append_len (str, source, len);
			}
		}
	}

	/* Remove leading space, trailing space and empty string */
	val = g_string_free (str, FALSE);
	if (val != NULL)
	{
		g_strstrip (val);
		if ((val != NULL) && (*val == '\0'))
		{
			g_free (val);
			val = NULL;
		}
	}

	return val;
}

static gboolean
parse_error_line (const gchar * line, gchar ** filename, int *lineno)
{
	gint i = 0;
	gint j = 0;
	gint k = 0;
	gchar *dummy;

	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			goto down;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = atoi (dummy);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (line, j - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}

      down:
	i = strlen (line) - 1;
	while (isspace (line[i]) == FALSE)
	{
		i--;
		if (i < 0)
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	k = i++;
	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = atoi (dummy);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (&line[k], j - k - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}
	*lineno = 0;
	*filename = NULL;
	return FALSE;
}

/* Output context functions
 *---------------------------------------------------------------------------*/

static void
on_message_buffer_click (IAnjutaMessageView *view, const gchar *line,
						 ATPOutputContext *this)
{
	gchar *filename;
	gint lineno;

	if (parse_error_line (line, &filename, &lineno))
	{
		gchar *uri;
		IAnjutaFileLoader *loader;
	
		/* Go to file and line number */
		loader = anjuta_shell_get_interface (this->execution->plugin->shell,
											 IAnjutaFileLoader,
											 NULL);
		
		/* Append current directory */
		if ((this->execution->directory != NULL) &&
			 (*filename != G_DIR_SEPARATOR))
		{
			if (*filename == '.')
			{
				uri = g_strdup_printf ("file://%s/%s#%d",
									   this->execution->directory,
									   filename + 1, lineno);
			}
			else
			{
				uri = g_strdup_printf ("file://%s/%s#%d",
									   this->execution->directory,
									   filename, lineno);
			}
		}
		else
		{
			uri = g_strdup_printf ("file:///%s#%d", filename, lineno);
		}
		g_free (filename);
		ianjuta_file_loader_load (loader, uri, FALSE, NULL);
		g_free (uri);
	}
}

static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *msg_line,
						 ATPOutputContext *this)
{
	gchar *dummy_fn;
	gint dummy_int;
	gchar *line;
	IAnjutaMessageViewType type;
	
	/* If there is a gdb style annotation to open a file, open it */
	if (strlen(msg_line) > 2 && msg_line[0] == '\032' &&
		msg_line[1] == '\032')
	{
		line = g_strdup_printf (_("Opening %s"), &msg_line[2]);
		on_message_buffer_click (view, &msg_line[2], this);
	}
	else
	{
		line = g_strdup (msg_line);
	}
	
	if (this->view)
	{
		gchar *desc = "";
		
		type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;
		if (parse_error_line(line, &dummy_fn, &dummy_int))
		{
			g_free (dummy_fn);
			if ((strstr (line, _("warning:")) != NULL) ||
				(strstr (line, "warning:") != NULL))
			{
				type = IANJUTA_MESSAGE_VIEW_TYPE_WARNING;
			}
			else if ((strstr (line, _("error:")) != NULL) ||
					 (strstr (line, "error:") != NULL))
			{
				type = IANJUTA_MESSAGE_VIEW_TYPE_ERROR;
			}
			desc = line;
		}
		else if (strstr (line, ":") != NULL)
		{
			type = IANJUTA_MESSAGE_VIEW_TYPE_INFO;
		}
		ianjuta_message_view_append (this->view, type, line, desc, NULL);
	}
	g_free (line);
}

/* Handle output for stdout and stderr */
static gboolean
atp_output_context_print (ATPOutputContext *this, const gchar* text)
{
	const gchar* str;

	if (this->type == ATP_TOUT_SAME)
	{
		/* Valid for error output only, get output type
		 * from standard output */
		this = &this->execution->output;
	}

	switch (this->type)
	{
	case ATP_TOUT_SAME:
		/* output should not use this */
		g_return_val_if_reached (TRUE);
		break;
	case ATP_TOUT_NULL:
		break;
	case ATP_TOUT_COMMON_PANE:
	case ATP_TOUT_NEW_PANE:
		/* Check if the view has already been created */
		if (this->created == FALSE)
		{
			IAnjutaMessageManager *man;
			gchar* title  = this->execution->name;

			man = anjuta_shell_get_interface (this->execution->plugin->shell,
											  IAnjutaMessageManager, NULL);
			if (this->view == NULL)
			{
				this->view = ianjuta_message_manager_add_view (man, title,
															   ICON_FILE,
															   NULL);
				g_signal_connect (G_OBJECT (this->view), "buffer_flushed",
							  G_CALLBACK (on_message_buffer_flush), this);
				g_signal_connect (G_OBJECT (this->view), "message_clicked",
							  G_CALLBACK (on_message_buffer_click), this);
				g_object_add_weak_pointer (G_OBJECT (this->view),
										   (gpointer *)(gpointer)&this->view);
			}
			else
			{
				ianjuta_message_view_clear (this->view, NULL);
			}
			if (this->execution->error.type == ATP_TOUT_SAME)
			{
				/* Same message used for all outputs */
				str = "";
			}
			else if (this == &this->execution->output)
			{
				/* Only for output data */
				str = _("(output)");
			}
			else
			{
				/* Only for error data */
				str = _("(error)");
			}
			title = g_strdup_printf ("%s %s", this->execution->name, str);
			
			ianjuta_message_manager_set_view_title (man, this->view,
													title, NULL);
			g_free (title);
			this->created = TRUE;
		}
		/* Display message */
		if (this->view)
		{
			ianjuta_message_view_buffer_append (this->view, text, NULL);
		}
		break;
	case ATP_TOUT_NEW_BUFFER:
	case ATP_TOUT_REPLACE_BUFFER:
		if (this->editor)
		{
			ianjuta_editor_append (this->editor, text, strlen(text), NULL);
		}
		break;
	case ATP_TOUT_INSERT_BUFFER:
	case ATP_TOUT_APPEND_BUFFER:
	case ATP_TOUT_REPLACE_SELECTION:	
	case ATP_TOUT_POPUP_DIALOG:
		g_string_append (this->buffer, text);	
		break;
	case ATP_TOUT_UNKNOWN:
	case ATP_OUTPUT_TYPE_COUNT:
		g_return_val_if_reached (TRUE);
	}

	return TRUE;
}

/* Write a small message at the beginning use only on stdout */
static gboolean
atp_output_context_print_command (ATPOutputContext *this, const gchar* command)
{
	gboolean ok;

	ok = TRUE;
	switch (this->type)
	{
	case ATP_TOUT_NULL:
	case ATP_TOUT_SAME:
		break;
	case ATP_TOUT_COMMON_PANE:
	case ATP_TOUT_NEW_PANE:
		
		ok = atp_output_context_print (this, _("Running command: "));
		ok &= atp_output_context_print (this, command);
		ok &= atp_output_context_print (this, "...\n");	
		break;
	case ATP_TOUT_NEW_BUFFER:
	case ATP_TOUT_REPLACE_BUFFER:
	case ATP_TOUT_INSERT_BUFFER:
	case ATP_TOUT_APPEND_BUFFER:
	case ATP_TOUT_REPLACE_SELECTION:
	case ATP_TOUT_POPUP_DIALOG:
		/* Do nothing for all these cases */
		break;
	case ATP_TOUT_UNKNOWN:
	case ATP_OUTPUT_TYPE_COUNT:
		g_return_val_if_reached (TRUE);
	}

	return ok;
};

/* Call at the end for stdout and stderr */
static gboolean
atp_output_context_print_result (ATPOutputContext *this, gint error)
{
	gboolean ok;
	char buffer[33];
	IAnjutaMessageManager *man;

	ok = TRUE;
	switch (this->type)
	{
	case ATP_TOUT_NULL:
	case ATP_TOUT_SAME:
		break;
	case ATP_TOUT_COMMON_PANE:
	case ATP_TOUT_NEW_PANE:
		if (this == &this->execution->output)
		{
			if (error)
			{
				ok = atp_output_context_print (this, _("Completed... unsuccessful with "));
				sprintf (buffer, "%d", error);
				ok &= atp_output_context_print (this, buffer);
			}
			else
			{
				ok = atp_output_context_print (this, _("Completed... successful"));
			}
			ok &= atp_output_context_print (this, "\n");
			if (this->view)
			{
				man = anjuta_shell_get_interface (this->execution->plugin->shell,
												  IAnjutaMessageManager, NULL);
				ianjuta_message_manager_set_current_view (man, this->view, NULL);
			}

		}
		break;
	case ATP_TOUT_NEW_BUFFER:
	case ATP_TOUT_REPLACE_BUFFER:
		/* Do nothing  */
		break;
	case ATP_TOUT_INSERT_BUFFER:
		if (this->editor)
		{
			ianjuta_editor_insert (this->editor, this->position, this->buffer->str,
							   this->buffer->len, NULL);
		}
		g_string_free (this->buffer, TRUE);
		this->buffer = NULL;
		break;
	case ATP_TOUT_APPEND_BUFFER:
		if (this->editor)
		{
			ianjuta_editor_append (this->editor, this->buffer->str,
							   this->buffer->len, NULL);
		}
		g_string_free (this->buffer, TRUE);
		this->buffer = NULL;
		break;
	case ATP_TOUT_REPLACE_SELECTION:
		if (this->editor)
		{
			ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (this->editor),
											  this->buffer->str,
											  this->buffer->len, NULL);
		}
		g_string_free (this->buffer, TRUE);
		this->buffer = NULL;
		break;
	case ATP_TOUT_POPUP_DIALOG:
		if (this->buffer->len)
		{
			if (this == &this->execution->output)
			{
				anjuta_util_dialog_info (GTK_WINDOW (this->execution->plugin->shell),
										 this->buffer->str);
			}
			else
			{
				anjuta_util_dialog_error (GTK_WINDOW (this->execution->plugin->shell),
										  this->buffer->str);
			}
			g_string_free (this->buffer, TRUE);
			this->buffer = NULL;
		}
		break;
	case ATP_TOUT_UNKNOWN:
	case ATP_OUTPUT_TYPE_COUNT:
		g_return_val_if_reached (TRUE);
	}

	return ok;
};

static IAnjutaEditor*
get_current_editor(IAnjutaDocumentManager* docman)
{
	if (docman == NULL)
		return NULL;
	IAnjutaDocument* doc = ianjuta_document_manager_get_current_document(docman, NULL);
	if (doc && IANJUTA_IS_EDITOR(doc))
		return IANJUTA_EDITOR(doc);
	else
		return NULL;
}

static ATPOutputContext*
atp_output_context_initialize (ATPOutputContext *this,
							   ATPExecutionContext *execution,
							   ATPOutputType type)
{
	IAnjutaDocumentManager *docman;

	this->type = type;
	switch (this->type)
	{
	case ATP_TOUT_NULL:
	case ATP_TOUT_SAME:
		break;
	case ATP_TOUT_COMMON_PANE:
	case ATP_TOUT_NEW_PANE:
		this->created = FALSE;
		break;
	case ATP_TOUT_REPLACE_BUFFER:
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->execution->plugin)->shell, IAnjutaDocumentManager, NULL);
		this->editor = get_current_editor(docman);
		if (this->editor != NULL)
		{
			g_object_add_weak_pointer (G_OBJECT (this->editor), (gpointer *)(gpointer)&this->editor);
			ianjuta_editor_erase_all (this->editor, NULL);
			break;
		}
		/* Go through, try to create a new buffer */
	case ATP_TOUT_NEW_BUFFER:
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->execution->plugin)->shell, IAnjutaDocumentManager, NULL);
		this->editor = get_current_editor(docman);
		if (this->editor == NULL)
		{
			anjuta_util_dialog_warning (GTK_WINDOW (this->execution->plugin->shell), _("Unable to create a buffer, command aborted"));
			return NULL;
		}
		g_object_add_weak_pointer (G_OBJECT (this->editor), (gpointer *)(gpointer)&this->editor);
		break;
	case ATP_TOUT_INSERT_BUFFER:
	case ATP_TOUT_APPEND_BUFFER:
	case ATP_TOUT_REPLACE_SELECTION:
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (this->execution->plugin)->shell, IAnjutaDocumentManager, NULL);
		this->editor = docman == NULL ? NULL : IANJUTA_EDITOR( ianjuta_document_manager_get_current_document (docman, NULL));
		if (this->editor == NULL)
		{
			anjuta_util_dialog_warning (GTK_WINDOW (this->execution->plugin->shell), _("No document currently open, command aborted"));
			return NULL;
		}
		g_object_add_weak_pointer (G_OBJECT (this->editor), (gpointer *)(gpointer)&this->editor);
		this->position = ianjuta_editor_get_position (this->editor, NULL);
		/* No break, need a buffer too */
	case ATP_TOUT_POPUP_DIALOG:
		if (this->buffer == NULL)
		{
			this->buffer = g_string_new ("");
		}
		else
		{
			g_string_erase (this->buffer, 0, -1);
		}
		break;
	case ATP_TOUT_UNKNOWN:
	case ATP_OUTPUT_TYPE_COUNT:
		g_return_val_if_reached (this);
	}

	return this;
}


static ATPOutputContext*
atp_output_context_construct (ATPOutputContext *this,
							  ATPExecutionContext *execution,
							  ATPOutputType type)
{

	this->execution = execution;
	this->view = NULL;
	this->buffer = NULL;

	return atp_output_context_initialize (this, execution, type);
}

static void
atp_output_context_destroy (ATPOutputContext *this)
{
	if (this->view)
	{
		IAnjutaMessageManager *man;

		man = anjuta_shell_get_interface (this->execution->plugin->shell,
										  IAnjutaMessageManager, NULL);
		ianjuta_message_manager_remove_view (man, this->view, NULL);
		this->view = NULL;
	}
	if (this->buffer)
	{
		g_string_free (this->buffer, TRUE);
	}
}

/* Execution context
 *---------------------------------------------------------------------------*/

static void
on_run_terminated (AnjutaLauncher* launcher, gint pid, gint status,
				   gulong time, gpointer user_data)
{
	ATPExecutionContext *this = (ATPExecutionContext *)user_data;

	atp_output_context_print_result (&this->output, status);
	atp_output_context_print_result (&this->error, status);
	this->busy = FALSE;
}

static void
on_run_output (AnjutaLauncher* launcher, AnjutaLauncherOutputType type,
			   const gchar* output, gpointer user_data)
{
	ATPExecutionContext* this = (ATPExecutionContext*)user_data;

	switch (type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
		atp_output_context_print (&this->output, output);
		break;
	case ANJUTA_LAUNCHER_OUTPUT_STDERR:
		atp_output_context_print (&this->error, output);
		break;
	case ANJUTA_LAUNCHER_OUTPUT_PTY:
		break;
	}
}

static ATPExecutionContext*
atp_execution_context_reuse (ATPExecutionContext* this, const gchar *name,
							 ATPOutputType output, ATPOutputType error)
{
	if (this->name) g_free (this->name);
	this->name = atp_remove_mnemonic (name);

	if (atp_output_context_initialize (&this->output, this, output) == NULL)
	{
		return NULL;
	}
	if (atp_output_context_initialize (&this->error, this, error) == NULL)
	{
		return NULL;
	}
	
	return this;
}

static ATPExecutionContext*
atp_execution_context_new (AnjutaPlugin *plugin, const gchar *name,
						   guint id, ATPOutputType output, ATPOutputType error)
{
	ATPExecutionContext *this;

	this = g_new0 (ATPExecutionContext, 1);

	this->plugin = plugin;
	this->launcher =  anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (this->launcher), "child-exited",
					  G_CALLBACK (on_run_terminated), this);
	this->name = atp_remove_mnemonic (name);

	if (atp_output_context_construct (&this->output, this, output) == NULL)
	{
 		g_free (this);
		return NULL;
	}
	if (atp_output_context_construct (&this->error, this, error) == NULL)
	{
		g_free (this);
		return NULL;
	}
	
	return this;
}

static void
atp_execution_context_free (ATPExecutionContext* this)
{
	atp_output_context_destroy (&this->output);
	atp_output_context_destroy (&this->error);

	if (this->launcher)
	{
		g_object_unref (this->launcher);
	}
	if (this->name) g_free (this->name);
	if (this->directory) g_free (this->directory);

	g_free (this);
}

static void
atp_execution_context_set_directory (ATPExecutionContext* this,
									 const gchar* directory)
{
	if (this->directory != NULL) g_free (this->directory);
	this->directory = directory == NULL ? NULL : g_strdup (directory);
}

static void
atp_execution_context_execute (ATPExecutionContext* this,
							   const gchar* command, const gchar* input)
{
	gchar* prev_dir = NULL;

	atp_output_context_print_command (&this->output, command);

	/* Change working directory */
	if (this->directory != NULL)
	{
		prev_dir = g_get_current_dir();
		chdir (this->directory);
	}
	/* Execute */
	anjuta_launcher_execute (this->launcher, command, on_run_output, this);
	/* Restore previous current directory */
	if (this->directory != NULL)
	{
		chdir (prev_dir);
		g_free (prev_dir);
	}
	anjuta_launcher_set_encoding (this->launcher, NULL);
	this->busy = TRUE;

	/* Send stdin data if needed */
	if (input != NULL)
	{
		anjuta_launcher_send_stdin (this->launcher, input);
		/* Send end marker */
		anjuta_launcher_send_stdin (this->launcher, "\x04");
	}
}

/* Execute context list
 *---------------------------------------------------------------------------*/

ATPContextList *
atp_context_list_construct (ATPContextList *this)
{
	this->list = NULL;
	return this;
}

void
atp_context_list_destroy (ATPContextList *this)
{
	GList *item;

	for (item = this->list; item != NULL; item = this->list)
	{
		this->list = g_list_remove_link (this->list, item);
		atp_execution_context_free ((ATPExecutionContext *)item->data);
		g_list_free (item);
	}
}

static ATPExecutionContext*
atp_context_list_find_context (ATPContextList *this, AnjutaPlugin *plugin,
							   const gchar* name, ATPOutputType output,
							   ATPOutputType error)
{
	ATPExecutionContext* context;
	GList* reuse = NULL;
	guint best;
	guint pane;
	GList* node;
	gboolean new_pane;
	gboolean output_pane;
	gboolean error_pane;

	pane = 0;
	best = 0;
	context = NULL;
	new_pane = (output == ATP_TOUT_NEW_PANE) || (error == ATP_TOUT_NEW_PANE);
	output_pane = (output == ATP_TOUT_NEW_PANE) || (output == ATP_TOUT_COMMON_PANE);
	error_pane = (error == ATP_TOUT_NEW_PANE) || (error == ATP_TOUT_COMMON_PANE);
	for (node = this->list; node != NULL; node = g_list_next (node))
	{
		ATPExecutionContext* current;
		guint score;

		current = (ATPExecutionContext *)node->data;

		/* Count number of used message panes */
		if (current->output.view != NULL) pane++;
		if (current->error.view != NULL) pane++;

		score = 1;
		if ((current->output.view != NULL) == output_pane) score++;
		if ((current->error.view != NULL) == error_pane) score++;

		if (!current->busy)
		{
			if ((score > best) || ((score == best) && (new_pane)))
			{
				/* Reuse this context */
				context = current;
				reuse = node;
				best = score;
			}
		}
	}

	if ((new_pane) && (pane < MAX_TOOL_PANES))
	{
		/* Not too many message pane, we can create a new one */
		context = NULL;
	}

	if (context == NULL)
	{
		/* Create a new node */
		context = atp_execution_context_new (plugin, name, 0, output, error);
		if (context != NULL)
		{
			this->list = g_list_prepend (this->list, context);
		}
	}
	else
	{
		this->list = g_list_remove_link (this->list, reuse);
		context = atp_execution_context_reuse (context, name, output, error);
		if (context != NULL)
		{
			this->list = g_list_concat (reuse, this->list);
		}
	}
	
	return context;
}

/* Execute tools
 *---------------------------------------------------------------------------*/

/* Menu activate handler which executes the tool. It should do command
** substitution, input, output and error redirection, setting the
** working directory, etc.
*/
void
atp_user_tool_execute (GtkMenuItem *item, ATPUserTool* this)
{
	ATPPlugin* plugin;
	ATPVariable* variable;
	ATPContextList* list;
	ATPExecutionContext* context;
	IAnjutaDocumentManager *docman;
	IAnjutaEditor *ed;
	gchar* dir;
	gchar* cmd;
	gchar* input;
	gchar* val = NULL;

	plugin = atp_user_tool_get_plugin (this);
	variable = atp_plugin_get_variable (plugin);

	/* Save files if requested */
	if (atp_user_tool_get_flag (this, ATP_TOOL_AUTOSAVE))
	{
		save_all_files (ANJUTA_PLUGIN (plugin));
	}
	
	/* Make command line */
	cmd = replace_variable (atp_user_tool_get_command (this),
							atp_user_tool_get_param (this), variable);

	/* Get working directory and replace variable */
	dir = replace_variable (NULL, atp_user_tool_get_working_dir (this),
							variable);

	if (atp_user_tool_get_flag (this, ATP_TOOL_TERMINAL))
	{
		/* Run in a terminal */
		/* don't need a execution context, launch and forget */

		gnome_execute_terminal_shell (dir, cmd);
	}
	else
	{
		/* Get stdin if necessary */
		input = NULL;
		switch (atp_user_tool_get_input (this))
		{
		case ATP_TIN_BUFFER:
			docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												 IAnjutaDocumentManager, NULL);
			ed = get_current_editor(docman);
			if (ed != NULL)
			{
				input = ianjuta_editor_get_text (ed, 0, -1, NULL);
			}
			break;
		case ATP_TIN_SELECTION:
			docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												 IAnjutaDocumentManager, NULL);
			ed = get_current_editor(docman);
			if (ed != NULL)
			{
				input = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (ed),
													  NULL);
			}
			break;
		case ATP_TIN_STRING:
			input = replace_variable (NULL,
									  atp_user_tool_get_input_string (this),
									  variable);
			break;
		case ATP_TIN_FILE:
			val = replace_variable (NULL, atp_user_tool_get_input_string (this),
									variable);
			if ((val == NULL) || (!g_file_get_contents (val, &input, NULL, NULL)))
			{
				anjuta_util_dialog_error (atp_plugin_get_app_window (plugin),
										  _("Unable to open input file %s, Command aborted"), val == NULL ? "(null)" : val);		
				if (val != NULL) g_free (val);
				if (dir != NULL) g_free (dir);
				if (cmd != NULL) g_free (cmd);

				return;
			}
			g_free (val);
			break;
		default:
			break;
		}

		list = atp_plugin_get_context_list (plugin);

		context = atp_context_list_find_context (list, ANJUTA_PLUGIN(plugin),
			       atp_user_tool_get_name (this),
			       atp_user_tool_get_output (this),
			       atp_user_tool_get_error (this));

		if (context)
		{
			/* Set working directory */
			atp_execution_context_set_directory (context, dir);
	
			/* Run command */
			atp_execution_context_execute (context, cmd, input);
		}

		if (input != NULL) g_free(input);
	}

	if (dir != NULL) g_free (dir);
	if (cmd != NULL) g_free (cmd);
}

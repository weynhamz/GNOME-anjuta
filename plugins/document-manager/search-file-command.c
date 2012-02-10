/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2012 <jhs@gnome.org>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "search-file-command.h"
#include <string.h>

struct _SearchFileCommandPrivate
{
	GFile* file;
	gchar* pattern;
	gchar* replace;
	gboolean regex;

	gint n_matches;
};

enum
{
	PROP_0,
	PROP_FILE,
	PROP_PATTERN,
	PROP_REPLACE,
	PROP_REGEX
};

#define BUFFER_SIZE 1024

static void
search_file_command_save (SearchFileCommand* cmd, const gchar* new_content, GError **error)
{
	GFileOutputStream* ostream;

	ostream = g_file_replace (cmd->priv->file,
	                          NULL,
	                          TRUE,
	                          G_FILE_CREATE_NONE,
	                          NULL,
	                          error);
	if (*error)
	{
		return;
	}
	/* TODO: Convert to original encoding */
	g_output_stream_write_all (G_OUTPUT_STREAM(ostream),
	                           new_content,
	                           strlen(new_content),
	                           NULL,
	                           NULL,
	                           error);
	g_object_unref (ostream);
}

static gchar*
search_file_command_load (SearchFileCommand* cmd, GError **error)
{
	GString* content;
	GFileInputStream* istream;
	gchar buffer[BUFFER_SIZE];
	gssize bytes_read;
	
	istream = g_file_read (cmd->priv->file, NULL, error);
	if (*error)
	{
		return NULL;
	}
	
	/* Read file into buffer */
	content = g_string_new (NULL);
	while ((bytes_read = g_input_stream_read (G_INPUT_STREAM (istream),
	                                          buffer,
	                                          BUFFER_SIZE,
	                                          NULL,
	                                          error)))
	{
		if (*error)
		{
			g_string_free (content, TRUE);
			g_object_unref (istream);
			return NULL;
		}
		/* TODO: Non-UTF8 files... */
		g_string_append_len (content, buffer, bytes_read);
	}
	g_object_unref (istream);

	return g_string_free (content, FALSE);
}

static guint
search_file_command_run (AnjutaCommand* anjuta_cmd)
{
	SearchFileCommand* cmd = SEARCH_FILE_COMMAND(anjuta_cmd);
	GError* error = NULL;
	gchar* pattern;
	gchar* replace;
	gchar* content;
	
	g_return_val_if_fail (cmd->priv->file != NULL && G_IS_FILE (cmd->priv->file), 1);

	cmd->priv->n_matches = 0;
	
	content = search_file_command_load (cmd, &error);
	if (error)
	{
		int code = error->code;
		g_error_free (error);
		return code;
	}
	
	if (!cmd->priv->regex)
	{
		pattern = g_regex_escape_string (cmd->priv->pattern, -1);
		if (cmd->priv->replace)
			replace = g_regex_escape_string (cmd->priv->replace, -1);
		else
			replace = NULL;
	}
	else
	{
		pattern = g_strdup (cmd->priv->pattern);
		if (cmd->priv->replace)
			replace = g_strdup (cmd->priv->replace);
		else
			replace = NULL;
	}

	if (!replace)
	{
		GRegex *regex;
		GMatchInfo *match_info;

		regex = g_regex_new (pattern, 0, 0, NULL);
		g_regex_match (regex, content, 0, &match_info);
		while (g_match_info_matches (match_info))
		{
			cmd->priv->n_matches++;
			g_match_info_next (match_info, NULL);
		}
		g_match_info_free (match_info);
		g_regex_unref (regex);                      
	}
	else
	{
		gchar* new_content;
		GRegex *regex;
		
		regex = g_regex_new (pattern, 0, 0, NULL);
		new_content = g_regex_replace (regex, content, -1, 0, replace, 0, NULL);
		g_regex_unref (regex);

		search_file_command_save (cmd, new_content, &error);
		g_free (new_content);

		if (error)
		{
			anjuta_async_command_set_error_message (anjuta_cmd, error->message);
			return 1;
		}
	}
	g_free (content);

	return 0;
}

G_DEFINE_TYPE (SearchFileCommand, search_file_command, ANJUTA_TYPE_ASYNC_COMMAND);

static void
search_file_command_init (SearchFileCommand *cmd)
{
	cmd->priv = G_TYPE_INSTANCE_GET_PRIVATE (cmd, SEARCH_TYPE_FILE_COMMAND, SearchFileCommandPrivate);

	cmd->priv->file = NULL;
	cmd->priv->pattern = NULL;
	cmd->priv->replace = NULL;
	cmd->priv->regex = FALSE;
}

static void
search_file_command_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (search_file_command_parent_class)->finalize (object);
}

static void
search_file_command_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SearchFileCommand* cmd;
	
	g_return_if_fail (SEARCH_IS_FILE_COMMAND (object));

	cmd = SEARCH_FILE_COMMAND (object);

	switch (prop_id)
	{
	case PROP_FILE:
		if (cmd->priv->file)
				g_object_unref (cmd->priv->file);
		cmd->priv->file = g_value_dup_object (value);
		break;
	case PROP_PATTERN:
		g_free (cmd->priv->pattern);
		cmd->priv->pattern = g_value_dup_string (value);
		break;
	case PROP_REPLACE:
		g_free (cmd->priv->replace);
		cmd->priv->replace = g_value_dup_string (value);
		break;
	case PROP_REGEX:
		cmd->priv->regex = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
search_file_command_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SearchFileCommand* cmd;
	
	g_return_if_fail (SEARCH_IS_FILE_COMMAND (object));

	cmd = SEARCH_FILE_COMMAND (object);
	
	switch (prop_id)
	{
	case PROP_FILE:
		g_value_set_object (value, cmd->priv->file);
		break;
	case PROP_PATTERN:
		g_value_set_string (value, cmd->priv->pattern);
		break;
	case PROP_REPLACE:
		g_value_set_string (value, cmd->priv->replace);
		break;
	case PROP_REGEX:
		g_value_set_boolean (value, cmd->priv->regex);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
search_file_command_class_init (SearchFileCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS(klass);
	
	object_class->finalize = search_file_command_finalize;
	object_class->set_property = search_file_command_set_property;
	object_class->get_property = search_file_command_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_FILE,
	                                 g_param_spec_object ("file",
	                                                      "filename",
	                                                      "Filename to search in",
	                                                      G_TYPE_FILE,
	                                                      G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_PATTERN,
	                                 g_param_spec_string ("pattern", "", "",
	                                                       "",
	                                                       G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_PATTERN,
	                                 g_param_spec_string ("replace", "", "",
	                                                       "",
	                                                       G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));	

	g_object_class_install_property (object_class,
	                                 PROP_REGEX,
	                                 g_param_spec_boolean ("regex", "", "",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	command_class->run = search_file_command_run;

	g_type_class_add_private (klass, sizeof(SearchFileCommandPrivate));
}


SearchFileCommand*
search_file_command_new (GFile* file, const gchar* pattern, const gchar* replace, gboolean regex)
{
	SearchFileCommand* command;

	command = SEARCH_FILE_COMMAND (g_object_new (SEARCH_TYPE_FILE_COMMAND,
	                                             "file", file,
	                                             "pattern", pattern,
	                                             "replace", replace,
	                                             "regex", regex, NULL));
	return command;
}

gint
search_command_get_n_matches (SearchFileCommand* cmd)
{
	g_return_val_if_fail (cmd != NULL && !SEARCH_IS_FILE_COMMAND (cmd), 0);

	return cmd->priv->n_matches;
}
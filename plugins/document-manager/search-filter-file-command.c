/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2012 <jhs@Obelix>
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

#include "search-filter-file-command.h"
#include <libanjuta/anjuta-debug.h>

struct _SearchFilterFileCommandPrivate
{
	GFile* file;
	gchar* mime_types;
};

enum
{
	PROP_0,

	PROP_FILE,
	PROP_MIME_TYPES
};



static guint
search_filter_file_command_run (AnjutaCommand* anjuta_cmd)
{
	SearchFilterFileCommand* cmd = SEARCH_FILTER_FILE_COMMAND (anjuta_cmd);
	
	gchar** mime_types = g_strsplit (cmd->priv->mime_types, ",", -1);
	gchar** mime_type;
	guint retval = 1;

	GFileInfo* file_info;
	GError* error = NULL;

	file_info = g_file_query_info (cmd->priv->file,
	                               G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE,
	                               NULL, &error);

	if (file_info)
	{
		for (mime_type = mime_types; mime_type && *mime_type; mime_type++)
		{
			gchar* content_type = g_content_type_from_mime_type (*mime_type);
			if (g_content_type_is_a (g_file_info_get_content_type (file_info),
				                     content_type))
			{
				retval = 0;
				g_free (content_type);
				break;
			}
		}
		g_object_unref (file_info);
	}
	else
	{
		int code = error->code;
		DEBUG_PRINT ("Couldn't query mime-type: %s", error->message);
		g_error_free (error);
		return code;
	}
	g_strfreev(mime_types);
	
	return retval;
}

G_DEFINE_TYPE (SearchFilterFileCommand, search_filter_file_command, ANJUTA_TYPE_ASYNC_COMMAND);

static void
search_filter_file_command_init (SearchFilterFileCommand *cmd)
{
	cmd->priv = G_TYPE_INSTANCE_GET_PRIVATE (cmd,
	                                         SEARCH_TYPE_FILTER_FILE_COMMAND,
	                                         SearchFilterFileCommandPrivate);
}

static void
search_filter_file_command_finalize (GObject *object)
{
	SearchFilterFileCommand* cmd = SEARCH_FILTER_FILE_COMMAND(object);

	if (cmd->priv->file)
		g_object_unref (cmd->priv->file);
	g_free (cmd->priv->mime_types);

	G_OBJECT_CLASS (search_filter_file_command_parent_class)->finalize (object);
}

static void
search_filter_file_command_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SearchFilterFileCommand* cmd;
	
	g_return_if_fail (SEARCH_IS_FILTER_FILE_COMMAND (object));

	cmd = SEARCH_FILTER_FILE_COMMAND(object);

	switch (prop_id)
	{
	case PROP_FILE:
		if (cmd->priv->file)
			g_object_unref (cmd->priv->file);
		cmd->priv->file = G_FILE(g_value_dup_object (value));
		break;
	case PROP_MIME_TYPES:
		g_free (cmd->priv->mime_types);
		cmd->priv->mime_types = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
search_filter_file_command_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SearchFilterFileCommand* cmd;
	
	g_return_if_fail (SEARCH_IS_FILTER_FILE_COMMAND (object));

	cmd = SEARCH_FILTER_FILE_COMMAND(object);

	switch (prop_id)
	{
	case PROP_FILE:
		g_value_set_object (value, cmd->priv->file);
		break;
	case PROP_MIME_TYPES:
		g_value_set_string (value, cmd->priv->mime_types);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
search_filter_file_command_class_init (SearchFilterFileCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* cmd_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = search_filter_file_command_finalize;
	object_class->set_property = search_filter_file_command_set_property;
	object_class->get_property = search_filter_file_command_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_FILE,
	                                 g_param_spec_object ("file",
	                                                      "",
	                                                      "",
	                                                      G_TYPE_FILE,
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_MIME_TYPES,
	                                 g_param_spec_string ("mime-types",
	                                                       "",
	                                                       "",
	                                                      NULL,
	                                                       G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	cmd_class->run = search_filter_file_command_run;
	
	g_type_class_add_private (klass, sizeof (SearchFilterFileCommandPrivate));
}


SearchFilterFileCommand*
search_filter_file_command_new (GFile* file, const gchar* mime_types)
{
	SearchFilterFileCommand* cmd;

	cmd = SEARCH_FILTER_FILE_COMMAND (g_object_new (SEARCH_TYPE_FILTER_FILE_COMMAND,
	                                                "file", file,
	                                                "mime-types", mime_types,
	                                                NULL));
	return cmd;
}

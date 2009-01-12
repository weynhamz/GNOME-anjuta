/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "subversion-vcs-interface.h"

void
subversion_ivcs_iface_init (IAnjutaVcsIface *iface)
{
	iface->add = subversion_ivcs_add;
	iface->checkout = subversion_ivcs_checkout;
	iface->diff = subversion_ivcs_diff;
	iface->query_status = subversion_ivcs_query_status;
	iface->remove = subversion_ivcs_remove;
}

void
subversion_ivcs_add (IAnjutaVcs *obj, GList *files, AnjutaAsyncNotify *notify,
					 GError **err)
{
	GList *path_list;
	SvnAddCommand *add_command;
	
	path_list = anjuta_util_convert_gfile_list_to_path_list (files);
	add_command = svn_add_command_new_list (path_list, FALSE, TRUE);
	
	anjuta_util_glist_strings_free (path_list);
	
	g_signal_connect (G_OBJECT (add_command), "command-finished", 
					  G_CALLBACK (g_object_unref), 
					  NULL);
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (add_command), "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (add_command));
	
}

void
subversion_ivcs_checkout (IAnjutaVcs *obj, 
						  const gchar *repository_location, GFile *dest,
						  GCancellable *cancel,
						  AnjutaAsyncNotify *notify, GError **err)
{
	gchar *path;
	SvnCheckoutCommand *checkout_command;
	Subversion *plugin;
	
	path = g_file_get_path (dest);
	checkout_command = svn_checkout_command_new (repository_location, path);
	plugin = ANJUTA_PLUGIN_SUBVERSION (obj);
	
	g_free (path);
	
	create_message_view (plugin);
	
	g_signal_connect (G_OBJECT (checkout_command), "data-arrived",
					  G_CALLBACK (on_command_info_arrived),
					  plugin);
	
	if (cancel)
	{
		g_signal_connect_swapped (G_OBJECT (cancel), "cancelled",
								  G_CALLBACK (anjuta_command_cancel),
								  checkout_command);
	}
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (checkout_command), 
								  "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (checkout_command));
}

static void
on_diff_command_data_arrived (AnjutaCommand *command, 
							  IAnjutaVcsDiffCallback callback)
{
	GQueue *output;
	gchar *line;
	
	output = svn_diff_command_get_output (SVN_DIFF_COMMAND (command));
	
	while (g_queue_peek_head (output))
	{
		line = g_queue_pop_head (output);
		callback (g_object_get_data (G_OBJECT (command), "file"), line,
				  g_object_get_data (G_OBJECT (command), "user-data"));
		g_free (line);
	}
}

void 
subversion_ivcs_diff (IAnjutaVcs *obj, GFile* file,  
					  IAnjutaVcsDiffCallback callback, gpointer user_data,  
					  GCancellable* cancel, AnjutaAsyncNotify *notify,
					  GError **err)
{
	gchar *path;
	SvnDiffCommand *diff_command;
	
	path = g_file_get_path (file);
	diff_command = svn_diff_command_new (path, SVN_DIFF_REVISION_NONE,
										 SVN_DIFF_REVISION_NONE,
										 ANJUTA_PLUGIN_SUBVERSION (obj)->project_root_dir,
										 TRUE);
	
	g_free (path);
	
	g_object_set_data_full (G_OBJECT (diff_command), "file", 
							g_object_ref (file),
							(GDestroyNotify) g_object_unref);
	g_object_set_data (G_OBJECT (diff_command), "user-data", user_data);
	
	g_signal_connect (G_OBJECT (diff_command), "command-finished", 
					  G_CALLBACK (g_object_unref),
					  NULL);
	
	g_signal_connect (G_OBJECT (diff_command), "data-arrived",
					  G_CALLBACK (on_diff_command_data_arrived),
					  callback);
	
	if (cancel)
	{
		g_signal_connect_swapped (G_OBJECT (cancel), "cancelled",
								  G_CALLBACK (anjuta_command_cancel),
								  diff_command);
	}
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (diff_command), "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (diff_command));
}

/* FIXME: The stuff in subversion-ui-utils.c should be namespaced. */
static void
on_ivcs_status_command_data_arrived (AnjutaCommand *command, 
								IAnjutaVcsStatusCallback callback)
{
	GQueue *status_queue;
	SvnStatus *status;
	gchar *path;
	
	status_queue = svn_status_command_get_status_queue (SVN_STATUS_COMMAND (command));
	
	while (g_queue_peek_head (status_queue))
	{
		status = g_queue_pop_head (status_queue);
		path = svn_status_get_path (status);
		
		callback (g_object_get_data (G_OBJECT (command), "file"), 
				  svn_status_get_vcs_status (status),
				  g_object_get_data (G_OBJECT (command), "user-data"));
		
		svn_status_destroy (status);
		g_free (path);
	}
}

void 
subversion_ivcs_query_status (IAnjutaVcs *obj, GFile *file, 
							  IAnjutaVcsStatusCallback callback,
							  gpointer user_data, GCancellable *cancel,
							  AnjutaAsyncNotify *notify, GError **err)
{
	gchar *path;
	SvnStatusCommand *status_command;
	
	path = g_file_get_path (file);
	status_command = svn_status_command_new (path, TRUE, FALSE);
	
	g_free (path);
	
	g_object_set_data_full (G_OBJECT (status_command), "file", 
							g_object_ref (file),
							(GDestroyNotify) g_object_unref);
	g_object_set_data (G_OBJECT (status_command), "user-data", user_data);
	
	g_signal_connect (G_OBJECT (status_command), "data-arrived",
					  G_CALLBACK (on_ivcs_status_command_data_arrived),
					  callback);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (g_object_unref),
					  NULL);
	
	if (cancel)
	{
		g_signal_connect_swapped (G_OBJECT (cancel), "cancelled",
								  G_CALLBACK (anjuta_command_cancel),
								  status_command);
	}
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (status_command), "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (status_command));
}

void 
subversion_ivcs_remove (IAnjutaVcs *obj, GList *files, 
						AnjutaAsyncNotify *notify, GError **err)
{
	GList *path_list;
	SvnRemoveCommand *remove_command;
	
	path_list = anjuta_util_convert_gfile_list_to_path_list (files);
	remove_command = svn_remove_command_new_list (path_list, NULL, FALSE);
	
	anjuta_util_glist_strings_free (path_list);
	
	g_signal_connect (G_OBJECT (remove_command), "command-finished", 
					  G_CALLBACK (g_object_unref), 
					  NULL);
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (remove_command), "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (remove_command));
}
/*
 * svn-thread.c (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "svn-thread.h"
#include "svn-backend.h"
#include "svn-backend-priv.h"
#include "svn-notify.h"

static void
on_mesg_view_destroy(Subversion* plugin, gpointer destroyed_view)
{
	plugin->mesg_view = NULL;
}

static svn_error_t*
on_log_callback (const char **log_msg,
                                const char **tmp_file,
                                apr_array_header_t *commit_items,
                                void *baton,
                                apr_pool_t *pool)
{
	*log_msg = baton;
	*tmp_file = NULL;
	
	return SVN_NO_ERROR;
}

static svn_opt_revision_t*
get_svn_revision(const gchar* revision)
{
	svn_opt_revision_t* svn_revision = g_new0(svn_opt_revision_t, 1);
	
	/* FIXME: Parse the revision string */
	svn_revision->kind = svn_opt_revision_head;
	
	return svn_revision;
}

static void
create_message_view(Subversion* plugin)
{
	IAnjutaMessageManager* mesg_manager = anjuta_shell_get_interface 
		(ANJUTA_PLUGIN (plugin)->shell,	IAnjutaMessageManager, NULL);
	plugin->mesg_view = 
	    ianjuta_message_manager_get_view_by_name(mesg_manager, _("Subversion"), NULL);
	if (!plugin->mesg_view)
	{
		/* FIXME: We need an Icon */
		plugin->mesg_view =
		     ianjuta_message_manager_add_view (mesg_manager, _("Subversion"), 
											   "", NULL);
		g_object_weak_ref (G_OBJECT (plugin->mesg_view), 
						  (GWeakNotify)on_mesg_view_destroy, plugin);
	}
	ianjuta_message_view_clear(plugin->mesg_view, NULL);
}

static void
svn_thread_init(SVNThread* thread)
{
	thread->info_messages = g_queue_new();
	thread->error_messages = g_queue_new();
	thread->complete = FALSE;
	
	thread->mutex = g_mutex_new();
}

static void
svn_thread_clean(SVNThread* thread)
{
	g_queue_free(thread->info_messages);
	g_queue_free(thread->error_messages);
	
	g_mutex_free(thread->mutex);
}

static gboolean
svn_thread_flush(SVNBackend* backend)
{
	SVNThread* thread = &backend->svn->thread;
		
	if (g_mutex_trylock(thread->mutex))
	{
		while (g_queue_peek_head(thread->info_messages))
		{
			if (backend->plugin->mesg_view != NULL)
				ianjuta_message_view_append(backend->plugin->mesg_view, 
										IANJUTA_MESSAGE_VIEW_TYPE_INFO,
										g_queue_peek_head(thread->info_messages), 
										"", NULL);
			g_queue_pop_head(thread->info_messages);
		}
		while (g_queue_peek_head(thread->error_messages))
		{
			if (backend->plugin->mesg_view != NULL)
				ianjuta_message_view_append(backend->plugin->mesg_view, 
										IANJUTA_MESSAGE_VIEW_TYPE_ERROR,
										g_queue_peek_head(thread->error_messages),
										"", NULL);
			g_queue_pop_head(thread->error_messages);
		}
		if (thread->complete)
		{
			g_mutex_unlock(thread->mutex);
			svn_thread_clean(thread);
			backend->svn->busy = FALSE;
			
			if (backend->plugin->mesg_view != NULL)
				ianjuta_message_view_append(backend->plugin->mesg_view, 
											IANJUTA_MESSAGE_VIEW_TYPE_INFO,
											_("Subversion command finished!"),
											"", NULL);
			
			return FALSE; /* Leave g_on_idle() */
		}
		else
			g_mutex_unlock(thread->mutex);
	}	
	return TRUE;
}


void svn_thread_start(SVNBackend* backend, GThreadFunc func, gpointer data)
{
#ifdef G_THREADS_ENABLED
	if (!g_thread_supported()) g_thread_init(NULL);
	
	svn_thread_init(&backend->svn->thread);
	g_idle_add((GSourceFunc)svn_thread_flush, backend);
	
	create_message_view(backend->plugin);
	
	backend->svn->busy = TRUE;
	g_thread_create(func, data, TRUE, NULL);
#else
	#warning Subversion plugin does not work without thread support!
#endif
}

gpointer svn_add_thread(SVNAdd* add)
{
	SVN* svn = add->svn;
	svn_error_t* error;
	
	error = svn_client_add2(add->filename, add->force, add->recurse, 
							svn->ctx, svn->pool);
	show_svn_error(error, add->svn);
	
	svn->thread.complete = TRUE;
	g_free(add->filename);
	g_free(add);
	return NULL;
}

gpointer 
svn_remove_thread(SVNRemove* remove)
{
	SVN* svn = remove->svn;
	svn_error_t* error;
	svn_client_commit_info_t* commit_info;
	
	apr_array_header_t* path = apr_array_make (svn->pool, 1, sizeof (char *));
	/* I just copied this so don't blame me... */
	(*((const char **) apr_array_push (path))) = remove->filename;
	
	error = svn_client_delete(&commit_info, path, remove->force,
							svn->ctx, svn->pool);
	show_svn_error(error, remove->svn);
	
	svn->thread.complete = TRUE;
	g_free(remove->filename);
	g_free(remove);
	return NULL;
}

gpointer
svn_commit_thread(SVNCommit* commit)
{
	gchar* log_message = g_strdup(commit->log);
	SVN* svn = commit->svn;
	svn_error_t* error;
	svn_client_commit_info_t* commit_info;
	
	svn->ctx->log_msg_func = on_log_callback;
	svn->ctx->log_msg_baton = log_message;
	
	apr_array_header_t* svn_path = apr_array_make (svn->pool, 1, sizeof (char *));
	/* I just copied this so don't blame me... */
	(*((const char **) apr_array_push (svn_path))) = commit->path;
	
	/* Note that the third arg is non-recurse... */
	error = svn_client_commit(&commit_info, svn_path, !commit->recurse,
							svn->ctx, svn->pool);
	show_svn_error(error, commit->svn);
	
	svn->thread.complete = TRUE;
	
	g_free(log_message);
	g_free(commit->path);
	g_free(commit->log);
	g_free(commit);
	
	return NULL;
}

gpointer
svn_update_thread(SVNUpdate* update)
{
	svn_opt_revision_t* svn_revision = get_svn_revision(update->revision);
	SVN* svn = update->svn;
	svn_error_t* error;
	
	error = svn_client_update (NULL,
					   update->path,
					   svn_revision,
					   update->recurse,
					   svn->ctx,
					   svn->pool);
	show_svn_error(error, update->svn);
	
	svn->thread.complete = TRUE;
	
	g_free(update->path);
	g_free(update->revision);
	g_free(update);
	
	return NULL;
}

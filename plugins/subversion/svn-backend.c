/* 
 * svn-backend.c (c) 2005 Johannes Schmid
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

#include "svn-backend.h"
#include "svn-backend-priv.h"
#include "svn-notify.h"
#include "svn-thread.h"
#include "plugin.h"

#include <subversion-1/svn_pools.h>
#include <subversion-1/svn_auth.h>
#include <subversion-1/svn_config.h>
/*
#include <apr.h>
#include <apr_date.h>
*/

static gpointer parent_class;

/* We cannot really take care of errors during init, we just print them
for debugging */

static void 
print_svn_error (svn_error_t *error)
{
	svn_error_t *itr = error;

	while (itr)
	{
		char buffer[256] = { 0 };

		g_warning ("SVN: Source error code: %d (%s)\n",
				   error->apr_err,
				   svn_strerror (itr->apr_err, buffer, sizeof (buffer)));
		g_warning ("SVN: Error description: %s\n", error->message);
				
		itr = itr->child;
	}
	svn_error_clear (error);
}

/* This is just a simple example how svn_auth can be used. We need
to investigate how this works in detail */

static void
svn_fill_auth(SVN* svn)
{
	svn_auth_baton_t *auth_baton;

    apr_array_header_t *providers
      = apr_array_make (svn->pool, 1, sizeof (svn_auth_provider_object_t *));

    svn_auth_provider_object_t *username_provider 
      = apr_pcalloc (svn->pool, sizeof(*username_provider));

    svn_client_get_username_provider(&username_provider, svn->pool);

    *(svn_auth_provider_object_t **)apr_array_push (providers) 
      = username_provider;

    svn_auth_open (&auth_baton, providers, svn->pool);

    svn->ctx->auth_baton = auth_baton;
}

static void
svn_backend_dispose (GObject* object)
{
	SVNBackend* backend = SVN_BACKEND(object);
	svn_pool_clear(backend->svn->pool);
	g_free(backend->svn);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (backend)));
}

static void
svn_backend_class_init (SVNBackendClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = svn_backend_dispose;
}

static void
svn_backend_init (SVNBackend* backend)
{
	svn_error_t* error;
	
	backend->svn = g_new0(SVN, 1);
	
	apr_initialize();
	backend->svn->pool = svn_pool_create(NULL);
	
	svn_client_create_context(&backend->svn->ctx,
							  backend->svn->pool);
	
	error = svn_config_get_config(&(backend->svn->ctx)->config,
								  NULL, /* default dir */
								  backend->svn->pool);
	print_svn_error (error);
	backend->svn->ctx->notify_baton = backend;
	backend->svn->ctx->notify_func = on_svn_notify;
	
	svn_fill_auth(backend->svn);
}

GType
svn_backend_get_type (void)
{
	static GType svn_backend_type = 0;
	if (!svn_backend_type)
	{
		static const GTypeInfo svn_backend_info = {
			sizeof (SVNBackendClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) svn_backend_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SVNBackend),
			0,
			(GInstanceInitFunc) svn_backend_init,
		};
		svn_backend_type =
			g_type_register_static (G_TYPE_OBJECT, "SVNBackend",
						&svn_backend_info, 0);
	}
	return svn_backend_type;
}

SVNBackend* 
svn_backend_new(Subversion* plugin)
{
	SVNBackend* backend = SVN_BACKEND(g_object_new (svn_backend_get_type (), 
										NULL));
	backend->plugin = plugin;
	return backend;
}

gboolean
svn_backend_busy(SVNBackend* backend)
{
	g_return_val_if_fail(backend != NULL, FALSE);
	return backend->svn->busy;
}

void
svn_backend_add(SVNBackend* backend, const gchar* filename,
				gboolean force, gboolean recurse)
{
	SVNAdd* add = g_new0(SVNAdd, 1);
	add->filename = g_strdup(filename);
	add->force = force;
	add->recurse = recurse;
	add->svn = backend->svn;
	
	svn_thread_start(backend, (GThreadFunc)svn_add_thread, add);
}

void
svn_backend_remove(SVNBackend* backend, const gchar* filename,
				   gboolean force)
{
	SVNRemove* remove = g_new0(SVNRemove, 1);
	remove->filename = g_strdup(filename);
	remove->force = force;
	remove->svn = backend->svn;
	
	svn_thread_start(backend, (GThreadFunc)svn_remove_thread, remove);
}

void
svn_backend_commit(SVNBackend* backend, const gchar* path, 
				   const gchar* log, gboolean recurse)
{
	SVNCommit* commit = g_new0(SVNCommit, 1);
	commit->path = g_strdup(path);
	commit->log = g_strdup(log);
	commit->recurse = recurse;
	commit->svn = backend->svn;
	
	svn_thread_start(backend, (GThreadFunc) svn_commit_thread, commit);
}

void
svn_backend_update(SVNBackend* backend, const gchar* path,
						const gchar* revision, gboolean recurse)
{
	SVNUpdate* update = g_new0(SVNUpdate, 1);
	update->path = g_strdup(path);
	update->revision = g_strdup(revision);
	update->recurse = recurse;
	update->svn = backend->svn;
	
	svn_thread_start(backend, (GThreadFunc) svn_update_thread, update);
}

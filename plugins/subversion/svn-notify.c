/*
 * svn_notify.c (c) 2005 Johannes Schmid
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

#include "svn-notify.h"
#include "svn-backend-priv.h"

static void
show_info(SVNBackend* backend, const gchar* type, const gchar* file)
{
	gchar* message;
	
	g_return_if_fail(type != NULL);
	g_return_if_fail(file != NULL);
		
	message = g_strdup_printf("%s: %s", type, file);
	
	g_mutex_lock(backend->svn->thread.mutex);
	g_queue_push_tail(backend->svn->thread.info_messages, message);
	g_mutex_unlock(backend->svn->thread.mutex);
}

void
show_svn_error(svn_error_t *error, SVN* svn)
{
	svn_error_t *itr = error;

	while (itr)
	{
		gchar* message = g_strdup_printf("SVN Error: %s!", error->message);
		g_mutex_lock(svn->thread.mutex);
		g_queue_push_tail(svn->thread.error_messages, message);
		g_mutex_unlock(svn->thread.mutex);
		
		itr = itr->child;
	}
	svn_error_clear (error);
}

void
on_svn_notify (gpointer baton,
					  const gchar *path,
					  svn_wc_notify_action_t action,
					  svn_node_kind_t kind,
					  const gchar *mime_type,
					  svn_wc_notify_state_t content_state,
					  svn_wc_notify_state_t prop_state,
					  svn_revnum_t revision)
{
	SVNBackend* backend = SVN_BACKEND(baton);
	switch(action)
	{
		case svn_wc_notify_add:
		{
			show_info(backend, _("Add"), path);
			break;
		}
		case svn_wc_notify_copy:
		{
			show_info(backend, _("Copy"), path);
			break;
		}
		case svn_wc_notify_delete:
		{
			show_info(backend, _("Delete"), path);
			break;
		}
		case svn_wc_notify_restore:
		{
			show_info(backend, _("Restore"), path);
			break;
		}
		case svn_wc_notify_revert:
		{
			show_info(backend, _("Revert"), path);
			break;
		}
		case svn_wc_notify_failed_revert:
		{
			show_info(backend, _("Revert failed"), path);
			break;
		}
		case svn_wc_notify_resolved:
		{
			show_info(backend, _("Resolved"), path);
			break;
		}
		case svn_wc_notify_skip:
		{
			show_info(backend, _("Skip"), path);
			break;
		}
		case svn_wc_notify_update_delete:
		{
			show_info (backend, _("Update delete"), path);
			break;
		}
		case svn_wc_notify_update_add:
		{
			show_info (backend, _("Update add"), path);
			break;
		}
		case svn_wc_notify_update_update:
		{
			show_info (backend, _("Update"), path);
			break;
		}
		case svn_wc_notify_update_completed:
		{
			show_info (backend, _("Update completed"), path);
			break;
		}
		case svn_wc_notify_update_external:
		{
			show_info (backend, _("Update external"), path);
			break;
		}
		case svn_wc_notify_status_completed:
		{
			show_info (backend, _("Status completed"), path);
			break;
		}
		case svn_wc_notify_status_external:
		{
			show_info (backend, _("Status external"), path);
			break;
		}
		case svn_wc_notify_commit_modified:
		{
			show_info (backend, _("Commit modified"), path);
			break;
		}
		case svn_wc_notify_commit_added:
		{
			show_info (backend, _("Commit added"), path);
			break;
		}
		case svn_wc_notify_commit_deleted:
		{
			show_info (backend, _("Commit deleted"), path);
			break;
		}
		case svn_wc_notify_commit_replaced:
		{
			show_info (backend, _("Commit replaced"), path);
			break;
		}
		case svn_wc_notify_commit_postfix_txdelta:
		{
			show_info (backend, _("Commit postfix"), path);
			break;
		}
		case svn_wc_notify_blame_revision:
		{
			show_info (backend, _("Blame revision"), path);
			break;
		}		
		default:
		{
			g_warning("Unknown notification");
			break;
		}
	}
}


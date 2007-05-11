/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * svn_notify.h (c) 2005 Johannes Schmid
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

#ifndef SVN_NOTIFY
#define SVN_NOTIFY

#include <svn_client.h>
#include "svn-backend.h"

void
on_svn_notify (gpointer baton,
					  const gchar *path,
					  svn_wc_notify_action_t action,
					  svn_node_kind_t kind,
					  const gchar *mime_type,
					  svn_wc_notify_state_t content_state,
					  svn_wc_notify_state_t prop_state,
					  svn_revnum_t revision);
					  
void
show_svn_error(svn_error_t *error, SVN* backend);

void
svn_show_info(SVNBackend* backend, const gchar* type, const gchar* file);


#endif

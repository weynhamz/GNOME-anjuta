/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * svn-backend-priv.h (c) 2005 Johannes Schmid
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
 
#ifndef SVN_BACKEND_PRIV_H
#define SVN_BACKEND_PRIV_H

#include "svn_client.h"
#include <glib.h>
#include "svn-backend.h"

typedef struct _SVNThread SVNThread;
	
struct _SVNThread
{
	GMutex* mutex;
	
	GQueue* info_messages;
	GQueue* error_messages;
	GQueue* diff_messages;
	
	gboolean complete;
};

struct _SVN
{
	svn_client_ctx_t* ctx;
	apr_pool_t *pool;
	
	gchar* log_message;
	
	SVNThread thread;
	gboolean busy;
};

#endif

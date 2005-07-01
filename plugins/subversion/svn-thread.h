/*
 * svn-thread.h (c) 2005 Johannes Schmid
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
 
#ifndef SVN_THREAD_H
#define SVN_THREAD_H

#include <glib.h>
#include "svn-backend-priv.h"

typedef struct _SVNAdd SVNAdd;
typedef struct _SVNRemove SVNRemove;
typedef struct _SVNCommit SVNCommit;
typedef struct _SVNUpdate SVNUpdate;
/* These are currently identical */
typedef struct _SVNUpdate SVNDiff;
typedef struct _SVNStatus SVNStatus;
	
struct _SVNAdd
{
	gchar* filename;
	gboolean force;
	gboolean recurse;
	
	SVN* svn;
};

struct _SVNRemove
{
	gchar* filename;
	gboolean force;
	
	SVN* svn;
};

struct _SVNCommit
{
	gchar* path;
	gchar* log;
	gboolean recurse;
	
	SVN* svn;
};

struct _SVNUpdate
{
	gchar* path;
	gchar* revision;
	gboolean recurse;
	
	SVN* svn;
};

void 
svn_thread_start(SVNBackend* backend, GThreadFunc func, gpointer data);

gpointer
svn_add_thread(SVNAdd* add);

gpointer 
svn_remove_thread(SVNRemove* remove);

gpointer
svn_commit_thread(SVNCommit* commit);

gpointer
svn_update_thread(SVNUpdate* update);

gpointer
svn_diff_thread(SVNDiff* diff);

#endif

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

#ifndef _GIT_VCS_INTERFACE_H_
#define _GIT_VCS_INTERFACE_H_

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-vcs.h>

#include "git-add-command.h"
#include "git-diff-command.h"
#include "git-status-command.h"
#include "git-remove-command.h"

#include "git-ui-utils.h"

void git_ivcs_iface_init (IAnjutaVcsIface *iface);
void git_ivcs_add (IAnjutaVcs *obj, GList *files, 
				   AnjutaAsyncNotify *notify, GError **err);
void git_ivcs_checkout (IAnjutaVcs *obj, 
						const gchar *repository_location, GFile *dest,
						GCancellable *cancel,
						AnjutaAsyncNotify *notify, GError **err);
void git_ivcs_diff (IAnjutaVcs *obj, GFile* file,  
					IAnjutaVcsDiffCallback callback, gpointer user_data,  
					GCancellable* cancel, AnjutaAsyncNotify *notify,
					GError **err);
void git_ivcs_query_status (IAnjutaVcs *obj, GFile *file, 
							IAnjutaVcsStatusCallback callback,
							gpointer user_data, GCancellable *cancel,
							AnjutaAsyncNotify *notify, GError **err);
void git_ivcs_remove (IAnjutaVcs *obj, GList *files, 
					  AnjutaAsyncNotify *notify, GError **err);


#endif 
 
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
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

#ifndef GIT_LOG_DIALOG_H
#define GIT_LOG_DIALOG_H

#include "git-ui-utils.h"
#include "git-log-command.h"
#include "git-log-message-command.h"
#include "git-ref-command.h"
#include "giggle-graph-renderer.h"

void on_menu_git_log (GtkAction* action, Git *plugin);
void on_fm_git_log (GtkAction *action, Git *plugin);

GtkWidget *git_log_window_create (Git *plugin);
void git_log_window_clear (Git *plugin);
GitRevision *git_log_get_selected_revision (Git *plugin);
gchar *git_log_get_path (Git *plugin);

#endif

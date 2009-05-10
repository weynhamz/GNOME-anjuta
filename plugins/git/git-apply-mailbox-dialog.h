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

#ifndef _GIT_APPLY_MAILBOX_DIALOG_H
#define _GIT_APPLY_MAILBOX_DIALOG_H

#include "git-apply-mailbox-command.h"
#include "git-apply-mailbox-continue-command.h"
#include "git-ui-utils.h"

void on_menu_git_apply_mailbox_apply (GtkAction *action, Git *plugin);
void on_menu_git_apply_mailbox_resolved (GtkAction *action, Git *plugin);
void on_menu_git_apply_mailbox_skip (GtkAction *action, Git *plugin);
void on_menu_git_apply_mailbox_abort (GtkAction *action, Git *plugin);

#endif

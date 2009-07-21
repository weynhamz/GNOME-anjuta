/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef _GIT_STASH_WIDGET_H
#define _GIT_STASH_WIDGET_H

#include "git-ui-utils.h"
#include "git-stash-save-command.h"
#include "git-stash-apply-command.h"

void git_stash_widget_create (Git *plugin, GtkWidget **stash_widget,
							  GtkWidget **stash_widget_grip);
void git_stash_widget_refresh (Git *plugin);
void git_stash_widget_clear (Git *plugin);
GFileMonitor *git_stash_widget_setup_refresh_monitor (Git *plugin);

#endif
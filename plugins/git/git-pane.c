/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-pane.h"



G_DEFINE_ABSTRACT_TYPE (GitPane, git_pane, ANJUTA_TYPE_DOCK_PANE);

static void
git_pane_init (GitPane *object)
{
	/* TODO: Add initialization code here */
}

static void
git_pane_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (git_pane_parent_class)->finalize (object);
}

static void
git_pane_class_init (GitPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
#if 0
	AnjutaDockPaneClass* parent_class = ANJUTA_DOCK_PANE_CLASS (klass);
#endif

	object_class->finalize = git_pane_finalize;
}


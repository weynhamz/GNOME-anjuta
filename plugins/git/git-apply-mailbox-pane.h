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

#ifndef _GIT_APPLY_MAILBOX_PANE_H_
#define _GIT_APPLY_MAILBOX_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-file-list.h>
#include "git-pane.h"
#include "plugin.h"
#include "git-apply-mailbox-command.h"
#include "git-apply-mailbox-continue-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_APPLY_MAILBOX_PANE             (git_apply_mailbox_pane_get_type ())
#define GIT_APPLY_MAILBOX_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_APPLY_MAILBOX_PANE, GitApplyMailboxPane))
#define GIT_APPLY_MAILBOX_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_APPLY_MAILBOX_PANE, GitApplyMailboxPaneClass))
#define GIT_IS_APPLY_MAILBOX_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_APPLY_MAILBOX_PANE))
#define GIT_IS_APPLY_MAILBOX_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_APPLY_MAILBOX_PANE))
#define GIT_APPLY_MAILBOX_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_APPLY_MAILBOX_PANE, GitApplyMailboxPaneClass))

typedef struct _GitApplyMailboxPaneClass GitApplyMailboxPaneClass;
typedef struct _GitApplyMailboxPane GitApplyMailboxPane;
typedef struct _GitApplyMailboxPanePriv GitApplyMailboxPanePriv;

struct _GitApplyMailboxPaneClass
{
	GitPaneClass parent_class;
};

struct _GitApplyMailboxPane
{
	GitPane parent_instance;

	GitApplyMailboxPanePriv *priv;
};

GType git_apply_mailbox_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_apply_mailbox_pane_new (Git *plugin);
void on_apply_mailbox_button_clicked (GtkAction *action, Git *plugin);
void on_apply_mailbox_continue_button_clicked (GtkAction *action, Git *plugin);
void on_apply_mailbox_skip_button_clicked (GtkAction *action, Git *plugin);
void on_apply_mailbox_abort_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_RESET_PANE_H_ */

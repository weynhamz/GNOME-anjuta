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

#ifndef _GIT_CHECKOUT_PANE_H_
#define _GIT_CHECKOUT_PANE_H_

#include <glib-object.h>
#include "git-pane.h"
#include "git-status-pane.h"
#include "git-checkout-files-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_CHECKOUT_PANE             (git_checkout_pane_get_type ())
#define GIT_CHECKOUT_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_CHECKOUT_PANE, GitCheckoutPane))
#define GIT_CHECKOUT_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_CHECKOUT_PANE, GitCheckoutPaneClass))
#define GIT_IS_CHECKOUT_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_CHECKOUT_PANE))
#define GIT_IS_CHECKOUT_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_CHECKOUT_PANE))
#define GIT_CHECKOUT_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_CHECKOUT_PANE, GitCheckoutPaneClass))

typedef struct _GitCheckoutPaneClass GitCheckoutPaneClass;
typedef struct _GitCheckoutPane GitCheckoutPane;
typedef struct _GitCheckoutPanePriv GitCheckoutPanePriv;

struct _GitCheckoutPaneClass
{
	GitPaneClass parent_class;
};

struct _GitCheckoutPane
{
	GitPane parent_instance;

	GitCheckoutPanePriv *priv;
};

GType git_checkout_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_checkout_pane_new (Git *plugin);
void on_checkout_button_clicked (GtkAction *action, Git *plugin);
void on_git_status_checkout_activated (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_CHECKOUT_PANE_H_ */

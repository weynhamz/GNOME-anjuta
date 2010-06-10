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

#ifndef _GIT_PANE_H_
#define _GIT_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-dock-pane.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include "plugin.h"

G_BEGIN_DECLS

#define GIT_TYPE_PANE             (git_pane_get_type ())
#define GIT_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_PANE, GitPane))
#define GIT_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_PANE, GitPaneClass))
#define GIT_IS_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_PANE))
#define GIT_IS_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_PANE))
#define GIT_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_PANE, GitPaneClass))

typedef struct _GitPaneClass GitPaneClass;
typedef struct _GitPane GitPane;

struct _GitPaneClass
{
	AnjutaDockPaneClass parent_class;
};

struct _GitPane
{
	AnjutaDockPane parent_instance;
};

GType git_pane_get_type (void) G_GNUC_CONST;

/* Static helper methods */
void git_pane_create_message_view (Git *plugin);
void git_pane_on_command_info_arrived (AnjutaCommand *command, Git *plugin);
void git_pane_set_log_view_column_label (GtkTextBuffer *buffer, 
                                         GtkTextIter *location,
                                         GtkTextMark *mark,
                                         GtkLabel *column_label);
gchar *git_pane_get_log_from_text_view (GtkTextView *text_view);
gboolean git_pane_check_input (GtkWidget *parent, GtkWidget *widget, 
                               const gchar *input, const gchar *error_message);
                      

G_END_DECLS

#endif /* _GIT_PANE_H_ */

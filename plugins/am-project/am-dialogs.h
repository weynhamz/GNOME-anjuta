/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-dialogs.h
 *
 * Copyright (C) 2009  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _AM_DIALOGS_H_
#define _AM_DIALOGS_H_

#include <gtk/gtk.h>
#include "am-project.h"

G_BEGIN_DECLS

GtkWidget *amp_configure_project_dialog (AmpProject *project, AnjutaProjectNode *node, GError **error);
GtkWidget *amp_configure_group_dialog (AmpProject *project, AnjutaProjectNode *group, GError **error);
GtkWidget *amp_configure_target_dialog (AmpProject *project, AnjutaProjectNode *target, GError **error);
GtkWidget *amp_configure_source_dialog (AmpProject *project, AnjutaProjectNode *target, GError **error);

G_END_DECLS

#endif /* _AM_DIALOGS_H_ */

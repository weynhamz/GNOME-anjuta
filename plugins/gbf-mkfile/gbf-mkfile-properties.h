/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf-mkfile-properties.h
 *
 * Copyright (C) 2005  Eric Greveson
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
 *
 * Author: Eric Greveson
 * Based on the Autotools GBF backend (libgbf-am) by
 *   JP Rosevear
 *   Dave Camp
 *   Naba Kumar
 *   Gustavo Giráldez
 */

#ifndef _GBF_MKFILE_PROPERTIES_H_
#define _GBF_MKFILE_PROPERTIES_H_

#include "gbf-mkfile-project.h"

GtkWidget *gbf_mkfile_properties_get_widget (GbfMkfileProject *project, GError **error);
GtkWidget *gbf_mkfile_properties_get_group_widget (GbfMkfileProject *project,
					    const gchar *group_id,
					    GError **error);
GtkWidget *gbf_mkfile_properties_get_target_widget (GbfMkfileProject *project,
						const gchar *target_id,
						GError **error);

#endif /* _GBF_MKFILE_PROPERTIES_H_ */

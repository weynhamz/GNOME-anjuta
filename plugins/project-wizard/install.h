/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    install.h
    Copyright (C) 2004 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __INSTALL_H__
#define __INSTALL_H__

#include <glib.h>
#include <libanjuta/anjuta-plugin.h>

#include "file.h"

typedef struct _NPWInstall NPWInstall;

NPWInstall* npw_install_new(struct _NPWPlugin* plugin);
void npw_install_destroy(NPWInstall* this);

gboolean npw_install_set_property(NPWInstall* this, GQueue* page_list, AnjutaPlugin* plugin);
gboolean npw_install_set_wizard_file(NPWInstall* this, const gchar* filename);
gboolean npw_install_launch(NPWInstall* this);

#endif

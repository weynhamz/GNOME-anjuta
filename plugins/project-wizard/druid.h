/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    druid.h
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

#ifndef __DRUID_H__
#define __DRUID_H__

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-project-wizard.glade"
#define ICON_FILE "anjuta-project-wizard-plugin.png"

#include <glib/gtypes.h> 

struct _NPWPlugin;
typedef struct _NPWDruid NPWDruid;

void npw_druid_show (NPWDruid* this);

NPWDruid* npw_druid_new (struct _NPWPlugin* plugin);

void npw_druid_set_busy (NPWDruid *this, gboolean busy_state);

#endif

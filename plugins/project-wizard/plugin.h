/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar

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

#ifndef __PROJECT_WIZARD_PLUGIN__
#define __PROJECT_WIZARD_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

typedef struct _NPWPlugin NPWPlugin;
typedef struct _NPWPluginClass NPWPluginClass;

struct _NPWPlugin {
	AnjutaPlugin parent;

	struct _NPWDruid* druid;
	struct _NPWInstall* install;
	IAnjutaMessageView* view;
};

struct _NPWPluginClass {
	AnjutaPluginClass parent_class;
};

#endif

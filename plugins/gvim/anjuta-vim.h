/***************************************************************************
 *            anjuta-vim.h
 *
 *  Do Dez 29 00:50:15 2005
 *  Copyright  2005  Naba Kumar  <naba@gnome.org>
 *  jhs@gnome.org
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __ANJUTA_VIM_H__
#define __ANJUTA_VIM_H__

#include <glib.h>
#include <glib-object.h>
#include <libanjuta/anjuta-plugin.h>

#include "gtkvim.h"

#define ANJUTA_TYPE_VIM         (anjuta_vim_get_type ())
#define ANJUTA_VIM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_VIM, AnjutaVim))
#define ANJUTA_VIM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_ANJUTA_VIM, AnjutaVimClass))
#define ANJUTA_IS_VIM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_VIM))
#define ANJUTA_IS_VIM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_VIM))
#define ANJUTA_VIM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_VIM, AnjutaVimClass))

typedef struct _AnjutaVimPrivate AnjutaVimPrivate;

typedef struct {
	GtkVim parent;
	AnjutaVimPrivate *priv;
} AnjutaVim;

typedef struct {
	GtkVimClass parent_class;
} AnjutaVimClass;

GType anjuta_vim_get_type(void);
AnjutaVim *anjuta_vim_new(const gchar* uri, const gchar* filename,
						  AnjutaPlugin* plugin);

#endif /* __ANJUTA_VIM_H__ */

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  combo-flags.h
 *  Copyright (C) 2006 Armin Burgmeier
 *
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

#ifndef __CLASSGEN_COMBO_FLAGS_H__
#define __CLASSGEN_COMBO_FLAGS_H__

#include <gtk/gtktreemodel.h>
#include <gtk/gtkhbox.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define CG_TYPE_COMBO_FLAGS             (cg_combo_flags_get_type ())
#define CG_COMBO_FLAGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CG_TYPE_COMBO_FLAGS, CgComboFlags))
#define CG_COMBO_FLAGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CG_TYPE_COMBO_FLAGS, CgComboFlagsClass))
#define CG_IS_COMBO_FLAGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CG_TYPE_COMBO_FLAGS))
#define CG_IS_COMBO_FLAGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CG_TYPE_COMBO_FLAGS))
#define CG_COMBO_FLAGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CG_TYPE_COMBO_FLAGS, CgComboFlagsClass))

typedef struct _CgComboFlagsClass CgComboFlagsClass;
typedef struct _CgComboFlags CgComboFlags;

/* This widget is a simple hbox that is able to launch a popup window
 * (similar to a combobox) that may be used to select flags. For each
 * selected flag, the selected signal is emitted. Flags are selected via
 * clicking on an entry in the popup or move to it using the up and down
 * keys and then press space.
 *
 * The popup may be removed in two different ways. Either by a right mouse
 * click or the enter key which is interpreted as a successful closure or via
 * a click somewhere else on the screen or the escape key which is
 * interpreted as cancellation. cg_combo_flags_editing_canceled may be
 * used to differentiate between the two.
 *
 * The widget implements the cell editable interface and may thus be used
 * from within a cell renderer to edit a cell. Such a cell renderer is
 * cell-renderer-flags. It should also be possible to use the widget as
 * a standalone widget if you put something in it (remember, it is a hbox)
 * that displays the current state, but has not been tested as such. */

struct _CgComboFlagsClass
{
	GtkHBoxClass parent_class;
};

struct _CgComboFlags
{
	GtkHBox parent_instance;
};

GType cg_combo_flags_get_type (void) G_GNUC_CONST;

GtkWidget *cg_combo_flags_new (void);
GtkWidget *cg_combo_flags_new_with_model (GtkTreeModel *model);
void cg_combo_flags_popup (CgComboFlags *combo);
void cg_combo_flags_popdown (CgComboFlags *combo);
gboolean cg_combo_flags_editing_canceled (CgComboFlags *combo);

G_END_DECLS

#endif /* __CLASSGEN_COMBO_FLAGS_H__ */

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _GIT_BRANCH_COMBO_MODEL_H
#define _GIT_BRANCH_COMBO_MODEL_H

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "git-branch.h"
#include "plugin.h"

typedef struct
{
	GtkListStore *model;
	GtkComboBox *combo_box;
	GtkBuilder *bxml;  /* Seems redundant, but we don't know what the combo box
					  * is called in the glade file. */
	Git *plugin;
} GitBranchComboData;

GitBranchComboData *git_branch_combo_data_new (GtkListStore *model, 
											   GtkComboBox *combo_box, 
											   GtkBuilder *bxml, Git *plugin);
void git_branch_combo_data_free (GitBranchComboData *data);
GtkListStore *git_branch_combo_model_new (void);
void git_branch_combo_model_setup_widget (GtkWidget *widget);
void git_branch_combo_model_append (GtkListStore *model, GitBranch *branch);
void git_branch_combo_model_append_remote (GtkListStore *model, 
										   const gchar *name);
gchar *git_branch_combo_model_get_branch (GtkListStore *model, 
										  GtkTreeIter *iter);

#endif

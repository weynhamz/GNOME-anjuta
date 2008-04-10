/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * search-replace.h: Generic Search and Replace header file
 * Copyright (C) 2004 Biswapesh Chattopadhyay
 * Copyright (C) 2004-2007 Naba Kumar  <naba@gnome.org>
 *
 * This file is part of anjuta.
 * Anjuta is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with anjuta. If not, contact the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _SEARCH_REPLACE_H
#define _SEARCH_REPLACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h>
#include <pcre.h>
#include "search-replace_backend.h"
	
typedef enum _GUIElementType
{
	GE_NONE,
	GE_BUTTON,
	GE_COMBO,
	GE_COMBO_ENTRY,
	GE_TEXT,	/* anything else that implements GtkEditable interface */
	GE_BOOLEAN
} GUIElementType;

typedef struct _GUIElement
{
	GUIElementType type;
	gchar *name;
	gpointer extra;
} GUIElement;

#define GLADE_FILE "anjuta.glade"
#define SEARCH_REPLACE_DIALOG "dialog.search.replace"

/* enum for all glade widgets that need specific handling */
typedef enum _GUIElementId
{
	CLOSE_BUTTON,
	STOP_BUTTON,
	REPLACE_BUTTON,
	SEARCH_BUTTON,

	/* Frames */
	FILE_FILTER_FRAME,
	SEARCH_SCOPE_FRAME,

	/* Labels */
	LABEL_REPLACE,

	/* Entries */
	SEARCH_STRING,
	MATCH_FILES,
	UNMATCH_FILES,
	MATCH_DIRS,
	UNMATCH_DIRS,
	REPLACE_STRING,

	/* Spinner */
	ACTIONS_MAX,

	/* Checkboxes */
	SEARCH_REGEX,
	GREEDY,
	IGNORE_CASE,
	WHOLE_WORD,
	WORD_START,
	WHOLE_LINE,
	IGNORE_HIDDEN_FILES,
	IGNORE_HIDDEN_DIRS,
	SEARCH_RECURSIVE,
	REPLACE_REGEX,
//	SEARCH_BASIC,

	/* Radio buttons */
	SEARCH_WHOLE,
	SEARCH_FORWARD,
	SEARCH_BACKWARD,
	ACTIONS_NO_LIMIT,
	ACTIONS_LIMIT,

	/* Combo boxes */
	SEARCH_STRING_COMBO,
	SEARCH_TARGET_COMBO,
	SEARCH_ACTION_COMBO,
	MATCH_FILES_COMBO,
	UNMATCH_FILES_COMBO,
	MATCH_DIRS_COMBO,
	UNMATCH_DIRS_COMBO,
	REPLACE_STRING_COMBO,

	GUI_ELEMENT_COUNT
} GUIElementId;

typedef struct _SearchReplaceGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	GtkWidget *widgets [GUI_ELEMENT_COUNT];	/* array of widgets for each GUIElement */
	SearchReplace *sr;	/* current s/r data */
	gboolean showing;
} SearchReplaceGUI;

void anj_sr_execute (SearchReplace *sr, gboolean dlg);
void anj_sr_select_next (SearchReplaceGUI *sg);
void anj_sr_select_previous (SearchReplaceGUI *sg);
void anj_sr_activate (gboolean replace, gboolean project);
void anj_sr_list_all_uses (const gchar *symbol);
void anj_sr_repeat (SearchReplaceGUI *sg);
//GUIElement *anj_sr_get_ui_element (GUIElementId id);
//GtkWidget *anj_sr_get_ui_widget (GUIElementId id);
#define anj_sr_get_ui_widget(id) sg->widgets[id]
void anj_sr_populate_data (SearchReplaceGUI *sg);
void anj_sr_populate_dialog (SearchReplaceGUI *sg);
void anj_sr_set_dialog_searchdata (SearchReplaceGUI *sg, SearchReplace *sr);
SearchReplaceGUI *anj_sr_get_default_uidata (void);
SearchReplaceGUI *anj_sr_get_current_uidata (GtkWidget *widget);
void anj_sr_get_best_uidata (SearchReplaceGUI **sg, SearchReplace **sr);
void anj_sr_destroy_ui_data (SearchReplaceGUI *sg);
void anj_sr_execute_init (AnjutaPlugin *plugin);

//void search_toolbar_set_text(gchar *search_text);

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */

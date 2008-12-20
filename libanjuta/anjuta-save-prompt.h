/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-save-prompt.h
 * Copyright (C) 2000 Naba Kumar  <naba@gnome.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ANJUTA_SAVE_PROMPT_H
#define ANJUTA_SAVE_PROMPT_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SAVE_PROMPT         (anjuta_save_prompt_get_type ())
#define ANJUTA_SAVE_PROMPT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_SAVE_PROMPT, AnjutaSavePrompt))
#define ANJUTA_SAVE_PROMPT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_SAVE_PROMPT, AnjutaSavePromptClass))
#define ANJUTA_IS_SAVE_PROMPT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_SAVE_PROMPT))
#define ANJUTA_IS_SAVE_PROMPT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_SAVE_PROMPT))
#define ANJUTA_SAVE_PROMPT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_SAVE_PROMPT, AnjutaSavePromptClass))

typedef struct _AnjutaSavePrompt AnjutaSavePrompt;
typedef struct _AnjutaSavePromptPrivate AnjutaSavePromptPrivate;
typedef struct _AnjutaSavePromptClass AnjutaSavePromptClass;

enum {
	ANJUTA_SAVE_PROMPT_RESPONSE_DISCARD,
	ANJUTA_SAVE_PROMPT_RESPONSE_SAVE_CLOSE,
	ANJUTA_SAVE_PROMPT_RESPONSE_CANCEL
};

typedef gboolean (*AnjutaSavePromptSaveFunc) (AnjutaSavePrompt *save_prompt,
											  gpointer item,
											  gpointer user_data);
struct _AnjutaSavePrompt {
	GtkMessageDialog parent;
	AnjutaSavePromptPrivate *priv;
};

struct _AnjutaSavePromptClass {
	GtkMessageDialogClass parent_class;
	/* Add Signal Functions Here */
};

GType anjuta_save_prompt_get_type (void);
AnjutaSavePrompt *anjuta_save_prompt_new (GtkWindow *parent);

gint anjuta_save_prompt_get_items_count (AnjutaSavePrompt *save_prompt);

void anjuta_save_prompt_add_item (AnjutaSavePrompt *save_prompt,
								  const gchar *item_name,
								  const gchar *item_detail,
								  gpointer item,
								  AnjutaSavePromptSaveFunc item_save_func,
								  gpointer user_data);
G_END_DECLS

#endif /* ANJUTA_SAVE_PROMPT_H */

/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * anjuta-completion.h
 * Copyright (C) 2013 Carl-Anton Ingmarsson <carlantoni@gnome.org>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#ifndef _ANJUTA_COMPLETION_H_
#define _ANJUTA_COMPLETION_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_COMPLETION             (anjuta_completion_get_type ())
#define ANJUTA_COMPLETION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_COMPLETION, AnjutaCompletion))
#define ANJUTA_COMPLETION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_COMPLETION, AnjutaCompletionClass))
#define ANJUTA_IS_COMPLETION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_COMPLETION))
#define ANJUTA_IS_COMPLETION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_COMPLETION))
#define ANJUTA_COMPLETION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_COMPLETION, AnjutaCompletionClass))

typedef struct _AnjutaCompletionClass AnjutaCompletionClass;
typedef struct _AnjutaCompletion AnjutaCompletion;
typedef struct _AnjutaCompletionPrivate AnjutaCompletionPrivate;



struct _AnjutaCompletionClass
{
    GObjectClass parent_class;
};

struct _AnjutaCompletion
{
    GObject parent_instance;

    AnjutaCompletionPrivate* priv;
};


typedef const char* (*AnjutaCompletionNameFunc) (const void* item);
typedef gboolean    (*AnjutaCompletionFilterFunc) (const void* item, void* user_data);


GType anjuta_completion_get_type (void) G_GNUC_CONST;

AnjutaCompletion*
anjuta_completion_new (AnjutaCompletionNameFunc name_func);

gboolean
anjuta_completion_get_case_sensitive (AnjutaCompletion* self);

void
anjuta_completion_set_case_sensitive (AnjutaCompletion* self,
                                      gboolean case_sensitive);

void
anjuta_completion_set_item_destroy_func (AnjutaCompletion* self,
                                         GDestroyNotify item_destroy_func);

void
anjuta_completion_set_filter_func        (AnjutaCompletion* self,
                                          AnjutaCompletionFilterFunc filter_func,
                                          void* user_data);

void
anjuta_completion_add_item (AnjutaCompletion* self,
                            void*             item);

void
anjuta_completion_clear    (AnjutaCompletion* self);

GList*
anjuta_completion_complete (AnjutaCompletion* self,
                            const char*       prefix,
                            gint              max_completions);

G_END_DECLS

#endif /* _ANJUTA_COMPLETION_H_ */


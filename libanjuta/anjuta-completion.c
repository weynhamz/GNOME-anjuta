/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * anjuta-completion.c
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

#include <string.h>

#include "anjuta-completion.h"


struct _AnjutaCompletionPrivate
{
    GPtrArray* items;
    gboolean   items_sorted;

    char*      last_complete;
    gint       last_complete_start;
    gint       last_complete_end;

    AnjutaCompletionNameFunc name_func;
    GDestroyNotify           item_destroy_func;

    AnjutaCompletionFilterFunc filter_func;
    void*                      filter_func_user_data;

    /* Properties */
    gboolean   case_sensitive;
};

enum
{
    PROP_0,
    PROP_CASE_SENSITIVE,
    PROP_LAST
};
static GParamSpec* properties[PROP_LAST];

G_DEFINE_TYPE (AnjutaCompletion, anjuta_completion, G_TYPE_OBJECT);

static inline
const char* anjuta_completion_default_name_func (const void* item)
{
    return (const char*)item;
}

static gint
anjuta_completion_item_sort_func (gconstpointer a, gconstpointer b,
                                  void* user_data)
{
    AnjutaCompletion* self = ANJUTA_COMPLETION (user_data);

    const char* name_a;
    const char* name_b;

    name_a = self->priv->name_func (*(const void**)a);
    name_b = self->priv->name_func (*(const void**)b);

    if (self->priv->case_sensitive)
        return strcmp (name_a, name_b);
    else
        return g_ascii_strcasecmp (name_a, name_b);
}

/**
 * anjuta_completion_complete:
 * @self: A #AnjutaCompletion
 * @prefix: The prefix to complete against.
 * 
 * Returns: (transfer container): The list of completions that matched
 * @prefix.
 */
GList*
anjuta_completion_complete (AnjutaCompletion* self,
                            const char*       prefix,
                            gint              max_completions)

{
    gint start, end;
    gint l, r;
    gint (*ncmp_func)(const char* s1, const char* s2, gsize n);
    gint first_match, last_match;
    GList* completions = NULL;

    g_return_val_if_fail (ANJUTA_IS_COMPLETION (self), NULL);
    g_return_val_if_fail (prefix, NULL);

    /* Start from old completion if possible */
    if (self->priv->last_complete &&
        self->priv->items_sorted &&
        g_str_has_prefix (prefix, self->priv->last_complete))
    {
        start = self->priv->last_complete_start;
        end = self->priv->last_complete_end;
    }
    else
    {
        start = 0;
        end = self->priv->items->len - 1;
    }

    /* Free last complete */
    if (self->priv->last_complete)
    {
        g_free (self->priv->last_complete);
        self->priv->last_complete = NULL;
    }

    /* Sort the items if they're not already sorted */
    if (!self->priv->items_sorted)
    {
        g_ptr_array_sort_with_data (self->priv->items, anjuta_completion_item_sort_func,
                                    self);
        self->priv->items_sorted = TRUE;
    }

    ncmp_func = self->priv->case_sensitive ? strncmp : g_ascii_strncasecmp;

    /* Do a binary search to find the first match */
    for (l = start, r = end; l < r;)
    {
        gint mid;
        void* item;
        const char* name;
        gint cmp;

        mid = l + (r - l) / 2;
        item = g_ptr_array_index (self->priv->items, mid);
        name = self->priv->name_func (item);

        cmp = ncmp_func (prefix, name, strlen (prefix));
        if (cmp > 0)
            l = mid + 1;
        else if (cmp < 0)
            r = mid - 1;
        else
            r = mid;
    }
    first_match = l;

    /* Do a binary search to find the last match */
    for (l = first_match, r = end; l < r;)
    {
        gint mid;
        void* item;
        const char* name;
        gint cmp;

        mid = l + ((r - l) / 2) + 1;
        item = g_ptr_array_index (self->priv->items, mid);
        name = self->priv->name_func (item);

        cmp = ncmp_func (prefix, name, strlen (prefix));
        if (cmp == 0)
            l = mid;
        else
            r = mid - 1;
            
    }
    last_match = r;

    if (first_match <= last_match)
    {
        gint i;
        gint n_completions;

        n_completions = 0;
        for (i = first_match; i <= last_match; i++)
        {
            void* item;
            const char* name;

            item = g_ptr_array_index (self->priv->items, i);
            name = self->priv->name_func (item);

            if (ncmp_func (prefix, name, strlen (prefix)) != 0)
                break;

            if (self->priv->filter_func &&
                !self->priv->filter_func (item, self->priv->filter_func_user_data))
                continue;

            completions = g_list_prepend (completions, item);
            n_completions++;
            if (max_completions > 0 && n_completions == max_completions)
                break;
        }
        completions = g_list_reverse (completions);
    }

    self->priv->last_complete = g_strdup (prefix);
    self->priv->last_complete_start = first_match;
    self->priv->last_complete_end = last_match;

    return completions;
}

static void
anjuta_completion_clear_items (AnjutaCompletion* self)
{
    if (self->priv->item_destroy_func)
    {
        gint i;

        for (i = 0; i < self->priv->items->len; i++)
        {
            void* item =  g_ptr_array_index (self->priv->items, i);

            self->priv->item_destroy_func (item);
        }
    }
    g_ptr_array_unref (self->priv->items);

    g_free (self->priv->last_complete);
    self->priv->last_complete = NULL;
}

/**
 * anjuta_completion_clear:
 * @self: a #AnjutaCompletion
 *
 * Clear all items added to the completion.
 */
void
anjuta_completion_clear (AnjutaCompletion* self)
{
    g_return_if_fail (ANJUTA_IS_COMPLETION (self));

    anjuta_completion_clear_items (self);
    self->priv->items = g_ptr_array_new ();
}

/**
 * anjuta_completion_add_item:
 * @self: a #AnjutaCompletion
 * @item: the item to be added.
 * 
 * Add an item to the completion.
 */
void
anjuta_completion_add_item (AnjutaCompletion* self,
                            void*             item)
{
    g_ptr_array_add (self->priv->items, item);

    self->priv->items_sorted = FALSE;
}

void
anjuta_completion_set_filter_func (AnjutaCompletion* self,
                                   AnjutaCompletionFilterFunc filter_func,
                                   void* user_data)
{
    g_return_if_fail (ANJUTA_IS_COMPLETION (self));

    self->priv->filter_func = filter_func;
    self->priv->filter_func_user_data = user_data;
}

/**
 * anjuta_completion_set_item_destroy_func:
 * @self: a #AnjutaCompletion
 * @item_destroy_func: (allow-none): the function to be called on
 * the added items when the #AnjutaCompletion object is destroyed.
 */
void
anjuta_completion_set_item_destroy_func (AnjutaCompletion* self,
                                         GDestroyNotify item_destroy_func)
{
    g_return_if_fail (ANJUTA_IS_COMPLETION (self));

    self->priv->item_destroy_func = item_destroy_func;
}

gboolean
anjuta_completion_get_case_sensitive (AnjutaCompletion* self)
{
    g_return_val_if_fail (ANJUTA_IS_COMPLETION (self), FALSE);

    return self->priv->case_sensitive;
}

void
anjuta_completion_set_case_sensitive (AnjutaCompletion* self,
                                      gboolean case_sensitive)
{
    g_return_if_fail (ANJUTA_IS_COMPLETION (self));

    if (self->priv->case_sensitive == case_sensitive)
        return;

    g_free (self->priv->last_complete);
    self->priv->last_complete = NULL;

    self->priv->items_sorted = FALSE;

    self->priv->case_sensitive = case_sensitive;
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CASE_SENSITIVE]);
}

AnjutaCompletion*
anjuta_completion_new (AnjutaCompletionNameFunc name_func)
{
    AnjutaCompletion* self;
    
    self = g_object_new (ANJUTA_TYPE_COMPLETION, NULL);

    if (name_func)
        self->priv->name_func = name_func;

    return self;
}

static void
anjuta_completion_init (AnjutaCompletion* self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ANJUTA_TYPE_COMPLETION,
                                              AnjutaCompletionPrivate);

    self->priv->name_func = anjuta_completion_default_name_func;
    self->priv->case_sensitive = TRUE;

    self->priv->items = g_ptr_array_new ();
}

static void
anjuta_completion_finalize (GObject *object)
{
    AnjutaCompletion* self  = ANJUTA_COMPLETION (object);

    anjuta_completion_clear_items (self);

    G_OBJECT_CLASS (anjuta_completion_parent_class)->finalize (object);
}

static void
anjuta_completion_get_property (GObject* object, guint prop_id, GValue* value,
                                GParamSpec* pspec)
{
    AnjutaCompletion* self = ANJUTA_COMPLETION (object);

    switch (prop_id)
    {
        case PROP_CASE_SENSITIVE:
            g_value_set_boolean (value,
                                 anjuta_completion_get_case_sensitive (self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
anjuta_completion_set_property (GObject* object, guint prop_id,
                                const GValue* value, GParamSpec* pspec)
{
    AnjutaCompletion* self = ANJUTA_COMPLETION (object);

    switch (prop_id)
    {
        case PROP_CASE_SENSITIVE:
            anjuta_completion_set_case_sensitive (self, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
anjuta_completion_class_init (AnjutaCompletionClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = anjuta_completion_get_property;
    object_class->set_property = anjuta_completion_set_property;
    object_class->finalize = anjuta_completion_finalize;

    properties[PROP_CASE_SENSITIVE] =
        g_param_spec_boolean ("case-sensitive", "Case sensitive", "Case sensitive",
                              TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, PROP_LAST, properties);
    g_type_class_add_private (klass, sizeof (AnjutaCompletionPrivate));
}

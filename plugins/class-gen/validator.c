/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  validator.c
 *	Copyright (C) 2006 Armin Burgmeier
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU Library General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "validator.h"

#include <gtk/gtkentry.h>

typedef struct _CgValidatorPrivate CgValidatorPrivate;
struct _CgValidatorPrivate
{
	GtkWidget *widget;
	GSList *entry_list;
};

#define CG_VALIDATOR_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE( \
		(o), \
		CG_TYPE_VALIDATOR, \
		CgValidatorPrivate \
	))

enum {
	PROP_0,

	/* Construct only */
	PROP_WIDGET
};

static GObjectClass *parent_class = NULL;

static void
cg_validator_entry_changed_cb (GtkEntry *entry,
                               gpointer user_data)
{
	CgValidator *validator;
	validator = CG_VALIDATOR (user_data);

	cg_validator_revalidate (validator);
}

static void
cg_validator_init (CgValidator *validator)
{
	CgValidatorPrivate *priv;
	priv = CG_VALIDATOR_PRIVATE (validator);

	priv->widget = NULL;
	priv->entry_list = NULL;
}

static void 
cg_validator_finalize (GObject *object)
{
	CgValidator *validator;
	CgValidatorPrivate *priv;
	GSList *item;
	
	validator = CG_VALIDATOR (object);
	priv = CG_VALIDATOR_PRIVATE (validator);
	
	for (item = priv->entry_list; item != NULL; item = item->next)
	{
		g_signal_handlers_disconnect_by_func (
			G_OBJECT (item->data), G_CALLBACK (cg_validator_entry_changed_cb),
			validator);
	}
	
	g_slist_free (priv->entry_list);
	priv->entry_list = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cg_validator_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
	CgValidator *validator;
	CgValidatorPrivate *priv;

	g_return_if_fail (CG_IS_VALIDATOR (object));

	validator = CG_VALIDATOR (object);
	priv = CG_VALIDATOR_PRIVATE (validator);

	switch (prop_id)
	{
	case PROP_WIDGET:
		priv->widget = GTK_WIDGET (g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_validator_get_property (GObject *object,
                           guint prop_id,
                           GValue *value, 
                           GParamSpec *pspec)
{
	CgValidator* validator;
	CgValidatorPrivate* priv;

	g_return_if_fail(CG_IS_VALIDATOR(object));

	validator = CG_VALIDATOR(object);
	priv = CG_VALIDATOR_PRIVATE(validator);

	switch(prop_id)
	{
	case PROP_WIDGET:
		g_value_set_object(value, G_OBJECT(priv->widget));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_validator_class_init (CgValidatorClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (CgValidatorPrivate));

	object_class->finalize = cg_validator_finalize;
	object_class->set_property = cg_validator_set_property;
	object_class->get_property = cg_validator_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_WIDGET,
	                                 g_param_spec_object ("widget",
	                                                      "Widget",
	                                                      "Widget to make insensitive if the entries are empty",
	                                                      GTK_TYPE_WIDGET,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

GType
cg_validator_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (CgValidatorClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cg_validator_class_init,
			NULL,
			NULL,
			sizeof (CgValidator),
			0,
			(GInstanceInitFunc) cg_validator_init,
			NULL
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "CgValidator",
		                                   &our_info, 0);
	}

	return our_type;
}

CgValidator *
cg_validator_new (GtkWidget *widget,
                  ...)
{
	CgValidator *validator;
	CgValidatorPrivate *priv;
	GtkEntry *entry;
	va_list arglist;

	validator = CG_VALIDATOR (g_object_new (CG_TYPE_VALIDATOR, "widget",
	                                        widget, NULL));

	priv = CG_VALIDATOR_PRIVATE (validator);

	va_start (arglist, widget);
	entry = va_arg (arglist, GtkEntry *);
	while (entry != NULL)
	{
		g_signal_connect (G_OBJECT (entry), "changed",
		                  G_CALLBACK (cg_validator_entry_changed_cb),
		                  validator);

		priv->entry_list = g_slist_prepend (priv->entry_list, entry);
		entry = va_arg (arglist, GtkEntry *);
	}
	
	cg_validator_revalidate (validator);
	return validator;
}

void
cg_validator_revalidate (CgValidator *validator)
{
	CgValidatorPrivate *priv;
	GtkEntry *entry;
	GSList *item;
	gchar *text;
	
	priv = CG_VALIDATOR_PRIVATE (validator);
	for (item = priv->entry_list; item != NULL; item = item->next)
	{
		entry = GTK_ENTRY (item->data);
		text = g_strdup (gtk_entry_get_text (entry));
		g_strchomp (text);
		if (*text == '\0') break;
	}
	
	if (item == NULL)
		gtk_widget_set_sensitive (priv->widget, TRUE);
	else
		gtk_widget_set_sensitive (priv->widget, FALSE);
}

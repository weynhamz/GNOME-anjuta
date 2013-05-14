/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-entry.h"

/**
 * SECTION:anjuta-entry
 * @short_description: #GtkEntry subclass that displays help text with a button
 *                     to clear the entry's contents.
 * @include: libanjuta/anjuta-entry.h
 *
 * AnjutaEntry is a  version of a #GtkEntry that displays some text, in
 * a lighter color, that describes what is to be entered into it. There is also
 * a button on the left to clear the entry's content quickly. AnjutaEntry is 
 * similar to the serach boxes used in Evolution and Glade, but is more generic
 * can can be used in almost any situation. 
 */

enum
{
	PROP_0,

	PROP_HELP_TEXT
};

typedef enum
{
	ANJUTA_ENTRY_NORMAL,
	ANJUTA_ENTRY_HELP
} AnjutaEntryMode;

struct _AnjutaEntryClassPrivate
{
	GtkCssProvider *css;
};

struct _AnjutaEntryPrivate
{
	gboolean showing_help_text;
	gchar *help_text;
};

G_DEFINE_TYPE_WITH_CODE (AnjutaEntry, anjuta_entry, GTK_TYPE_ENTRY,
                         g_type_add_class_private (g_define_type_id, sizeof (AnjutaEntryClassPrivate)))

static void
anjuta_entry_set_mode (AnjutaEntry *self, AnjutaEntryMode mode)
{
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (GTK_WIDGET (self));

	switch (mode)
	{
		case ANJUTA_ENTRY_NORMAL:
			/* Remove the help text from the widget */
			if (self->priv->showing_help_text)
				gtk_entry_set_text (GTK_ENTRY (self), "");

			gtk_style_context_add_class (context, "anjuta-entry-mode-normal");
			gtk_style_context_remove_class (context, "anjuta-entry-mode-help");

			self->priv->showing_help_text = FALSE;

			break;
		case ANJUTA_ENTRY_HELP:
			if (self->priv->help_text)
				gtk_entry_set_text (GTK_ENTRY (self), self->priv->help_text);
			else
				gtk_entry_set_text (GTK_ENTRY (self), "");

			gtk_style_context_add_class (context, "anjuta-entry-mode-help");
			gtk_style_context_remove_class (context, "anjuta-entry-mode-normal");

			self->priv->showing_help_text = TRUE;

			break;
		default:
			break;
	}
}
		
/* It's probably terrible practice for a subclass to be listening to the 
 * parent' class's signals, but for some reason the icon release signal 
 * doesn't have a virtual method pointer in the GtkEntry class structure */
static void
anjuta_entry_icon_release (GtkEntry *entry, GtkEntryIconPosition icon_pos,
                           GdkEvent *event, gpointer user_data)
{
	if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
		gtk_entry_set_text (entry, "");
}

static void
anjuta_entry_init (AnjutaEntry *self)
{
	GtkStyleContext *context;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ANJUTA_TYPE_ENTRY,
	                                          AnjutaEntryPrivate);

	/* Setup styling */
	context = gtk_widget_get_style_context (GTK_WIDGET (self));
	gtk_style_context_add_provider (context,
	                                GTK_STYLE_PROVIDER (ANJUTA_ENTRY_GET_CLASS (self)->priv->css),
	                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	
	gtk_entry_set_icon_from_stock (GTK_ENTRY (self), GTK_ENTRY_ICON_SECONDARY,
	                               GTK_STOCK_CLEAR);
	gtk_entry_set_icon_activatable (GTK_ENTRY (self), GTK_ENTRY_ICON_SECONDARY,
	                                TRUE);

	g_signal_connect (G_OBJECT (self), "icon-release",
	                  G_CALLBACK (anjuta_entry_icon_release),
	                  NULL);

	anjuta_entry_set_mode (self, ANJUTA_ENTRY_HELP);
}

static void
anjuta_entry_finalize (GObject *object)
{
	AnjutaEntry *self;

	self = ANJUTA_ENTRY (object);

	g_free (self->priv->help_text);

	G_OBJECT_CLASS (anjuta_entry_parent_class)->finalize (object);
}

static void
anjuta_entry_set_property (GObject *object, guint prop_id, const GValue *value, 
                           GParamSpec *pspec)
{
	AnjutaEntry *self;

	g_return_if_fail (ANJUTA_IS_ENTRY (object));

	self = ANJUTA_ENTRY (object);

	switch (prop_id)
	{
		case PROP_HELP_TEXT:
			g_free (self->priv->help_text);

			self->priv->help_text = g_value_dup_string (value);

			/* Update the display */
			if (self->priv->showing_help_text)
			{
				if (self->priv->help_text)
				{
					gtk_entry_set_text (GTK_ENTRY (self), 
					                    self->priv->help_text);
				}
			}
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
anjuta_entry_get_property (GObject *object, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	AnjutaEntry *self;

	g_return_if_fail (ANJUTA_IS_ENTRY (object));

	self = ANJUTA_ENTRY (object);

	switch (prop_id)
	{
		case PROP_HELP_TEXT:
			g_value_set_string (value, self->priv->help_text);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static gboolean
anjuta_entry_focus_in_event (GtkWidget *widget, GdkEventFocus *event)
{
	AnjutaEntry *self;

	self = ANJUTA_ENTRY (widget);

	if (self->priv->showing_help_text)
		anjuta_entry_set_mode (self, ANJUTA_ENTRY_NORMAL);

	return GTK_WIDGET_CLASS (anjuta_entry_parent_class)->focus_in_event (widget, event);
}

static gboolean
anjuta_entry_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	AnjutaEntry *self;
	const gchar *text;

	self = ANJUTA_ENTRY (widget);
	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (text == NULL || text[0] == '\0')
		anjuta_entry_set_mode (self, ANJUTA_ENTRY_HELP);

	return GTK_WIDGET_CLASS (anjuta_entry_parent_class)->focus_out_event (widget, event);
}

static void
anjuta_entry_class_init (AnjutaEntryClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	static const gchar entry_style[] =
		"AnjutaEntry.anjuta-entry-mode-help {\n"
		"color: @insensitive_fg_color;\n"
		"}";

	object_class->finalize = anjuta_entry_finalize;
	object_class->set_property = anjuta_entry_set_property;
	object_class->get_property = anjuta_entry_get_property;
	widget_class->focus_in_event = anjuta_entry_focus_in_event;
	widget_class->focus_out_event = anjuta_entry_focus_out_event;

	g_type_class_add_private (klass, sizeof(AnjutaEntryPrivate));

	/**
	 * AnjutaEntry::help-text:
	 *
	 * Text that should be displayed when the entry is empty. This text should
	 * briefly describe what the user should enter.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_HELP_TEXT,
	                                 g_param_spec_string ("help-text",
	                                                      _("Help text"),
	                                                      _("Text to show the user what to enter into the entry"),
	                                                      "",
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));


	klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, ANJUTA_TYPE_ENTRY, AnjutaEntryClassPrivate);

	klass->priv->css = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (klass->priv->css, entry_style, -1, NULL);
}

/**
 * anjuta_entry_new:
 *
 * Creates a new AnjutaEntry.
 */
GtkWidget *
anjuta_entry_new (void)
{
	return g_object_new (ANJUTA_TYPE_ENTRY, NULL);
}

/**
 * anjuta_entry_get_text:
 * @self: An AnjutaEntry
 *
 * Returns: The contents of the entry. If the entry is empty, the help text will
 * be displayed and an empty string will be returned.
 */
const gchar *
anjuta_entry_get_text (AnjutaEntry *self)
{
	return (self->priv->showing_help_text) ? 
		    "" : gtk_entry_get_text (GTK_ENTRY (self)) ;
}

/**
 * anjuta_entry_dup_text:
 * @self: An AnjutaEntry
 *
 * Returns: (transfer full): A copy of the contents of the entry. If the entry
 * is empty, the returned string will be empty. The returned string must be
 * freed when no longer needed.
 */
gchar *
anjuta_entry_dup_text (AnjutaEntry *self)
{
	return g_strdup (anjuta_entry_get_text (self));
}

/**
 * anjuta_entry_set_text:
 * @self: An AnjutaEntry
 * @text: The new text
 *
 * Sets the text on the entry, showing the help text if the text is empty.
 */
void
anjuta_entry_set_text (AnjutaEntry *self, const gchar *text)
{
	if (text != NULL && text[0] != '\0')
		anjuta_entry_set_mode (self, ANJUTA_ENTRY_NORMAL);
	else
		anjuta_entry_set_mode (self, ANJUTA_ENTRY_HELP);

	gtk_entry_set_text (GTK_ENTRY (self), text);
}

/**
 * anjuta_entry_is_showing_help_text:
 * @self: An AnjutaEntry
 * 
 * Returns: Whether the entry is showing its help text. In practice, if this
 * method returns %TRUE, it means that the user has not entered anything.
 */
gboolean
anjuta_entry_is_showing_help_text (AnjutaEntry *self)
{
	return self->priv->showing_help_text;
}

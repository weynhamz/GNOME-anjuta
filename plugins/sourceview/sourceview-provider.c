/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2009 <jhs@gnome.org>
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

#include "sourceview-provider.h"
#include "sourceview-cell.h"
#include "sourceview-private.h"


static void
sourceview_provider_iface_init (GtkSourceCompletionProviderIface* provider);

G_DEFINE_TYPE_WITH_CODE (SourceviewProvider,
			 sourceview_provider,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_SOURCE_COMPLETION_PROVIDER,
			                        sourceview_provider_iface_init))

static void on_context_cancelled (GtkSourceCompletionContext* context,
                                  SourceviewProvider* prov)
{
	ianjuta_provider_cancelled(prov->iprov, NULL);
}

static void
sourceview_provider_populate (GtkSourceCompletionProvider* provider, GtkSourceCompletionContext* context)
{
	SourceviewProvider* prov = SOURCEVIEW_PROVIDER(provider);
	GtkTextIter iter;
	SourceviewCell* cell;
	gtk_source_completion_context_get_iter(context, &iter);
	cell = sourceview_cell_new (&iter, GTK_TEXT_VIEW(prov->sv->priv->view));
	prov->context = context;
	g_signal_connect (context, "cancelled", G_CALLBACK(on_context_cancelled), prov);
	ianjuta_provider_populate(prov->iprov, IANJUTA_ITERABLE(cell), NULL);
	prov->context = NULL;
	g_object_unref (cell);
}

static const gchar*
sourceview_provider_get_name (GtkSourceCompletionProvider* provider)
{
	return "Internal Sourceview editor provider";
}


static gboolean
sourceview_provider_get_start_iter (GtkSourceCompletionProvider* provider, 
                                    GtkSourceCompletionContext* context,
                                    GtkSourceCompletionProposal* proposal, GtkTextIter* iter)
{
	SourceviewProvider* prov = SOURCEVIEW_PROVIDER(provider);
	IAnjutaIterable* iiter = ianjuta_provider_get_start_iter (prov->iprov, NULL);
	if (iiter)
	{
		SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
		GtkTextIter* source_iter = sourceview_cell_get_iter(cell);
		*iter = *source_iter;
		return TRUE;
	}
	else
		return FALSE;
}

static void
sourceview_provider_activate_proposal (GtkSourceCompletionProvider* provider,
                                       GtkSourceCompletionProposal* proposal,
                                       GtkTextIter* iter)
{
	SourceviewProvider* prov = SOURCEVIEW_PROVIDER (provider);
	SourceviewCell* cell = sourceview_cell_new (iter, GTK_TEXT_VIEW(prov->sv->priv->view));
	gpointer data = g_object_get_data (proposal, "__data");

	ianjuta_provider_activate(prov->iprov, IANJUTA_ITERABLE(cell), data);
	
	g_object_unref (cell);
}

static void
sourceview_provider_iface_init (GtkSourceCompletionProviderIface* provider)
{
	provider->get_name = sourceview_provider_get_name;
	provider->populate = sourceview_provider_populate;
	provider->get_start_iter = sourceview_provider_get_start_iter;
	provider->activate_proposal = sourceview_provider_activate_proposal;
}

static void
sourceview_provider_init (SourceviewProvider *object)
{
  object->context = NULL;
}

static void
sourceview_provider_dispose (GObject* obj)
{

}

static void
sourceview_provider_class_init (SourceviewProviderClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = sourceview_provider_dispose;
}

GtkSourceCompletionProvider* sourceview_provider_new (Sourceview* sv,
                                                      IAnjutaProvider* iprov)
{
	GObject* obj = g_object_new (SOURCEVIEW_TYPE_PROVIDER, NULL);
	SourceviewProvider* prov = SOURCEVIEW_PROVIDER(obj);
	prov->sv = sv;
	prov->iprov = iprov;
	return GTK_SOURCE_COMPLETION_PROVIDER(obj);
}


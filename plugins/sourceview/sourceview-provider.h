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

#ifndef _SOURCEVIEW_PROVIDER_H_
#define _SOURCEVIEW_PROVIDER_H_

#include <libanjuta/interfaces/ianjuta-provider.h>
#include <gtksourceview/gtksourceview.h>
#include "sourceview.h"

G_BEGIN_DECLS

#define SOURCEVIEW_TYPE_PROVIDER             (sourceview_provider_get_type ())
#define SOURCEVIEW_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOURCEVIEW_TYPE_PROVIDER, SourceviewProvider))
#define SOURCEVIEW_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SOURCEVIEW_TYPE_PROVIDER, SourceviewProviderClass))
#define SOURCEVIEW_IS_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOURCEVIEW_TYPE_PROVIDER))
#define SOURCEVIEW_IS_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SOURCEVIEW_TYPE_PROVIDER))
#define SOURCEVIEW_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SOURCEVIEW_TYPE_PROVIDER, SourceviewProviderClass))

typedef struct _SourceviewProviderClass SourceviewProviderClass;
typedef struct _SourceviewProvider SourceviewProvider;

struct _SourceviewProviderClass
{
	GObjectClass parent_class;
};

struct _SourceviewProvider
{
	GObject parent_instance;
	
	Sourceview* sv;
	IAnjutaProvider* iprov;
	GtkSourceCompletionContext* context;
	gboolean cancelled;
};

GType sourceview_provider_get_type (void) G_GNUC_CONST;

GtkSourceCompletionProvider* sourceview_provider_new (Sourceview* sv, IAnjutaProvider* iprov);

G_END_DECLS

#endif /* _SOURCEVIEW_PROVIDER_H_ */

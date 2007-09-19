/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C)  2007 Naba Kumar  <naba@gnome.org>
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

#ifndef _CPP_JAVA_ASSIST_H_
#define _CPP_JAVA_ASSIST_H_

#include <glib-object.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

G_BEGIN_DECLS

#define TYPE_CPP_JAVA_ASSIST             (cpp_java_assist_get_type ())
#define CPP_JAVA_ASSIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CPP_JAVA_ASSIST, CppJavaAssist))
#define CPP_JAVA_ASSIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CPP_JAVA_ASSIST, CppJavaAssistClass))
#define IS_CPP_JAVA_ASSIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CPP_JAVA_ASSIST))
#define IS_CPP_JAVA_ASSIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CPP_JAVA_ASSIST))
#define CPP_JAVA_ASSIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CPP_JAVA_ASSIST, CppJavaAssistClass))

typedef struct _CppJavaAssistClass CppJavaAssistClass;
typedef struct _CppJavaAssist CppJavaAssist;
typedef struct _CppJavaAssistContext CppJavaAssistContext;
typedef struct _CppJavaAssistPriv CppJavaAssistPriv;

struct _CppJavaAssistContext {
	GCompletion* completion;
	GList* tips;
	gint position;
};

struct _CppJavaAssistClass
{
	GObjectClass parent_class;
};

struct _CppJavaAssist
{
	GObject parent_instance;
	CppJavaAssistPriv *priv;
};

GType cpp_java_assist_get_type (void) G_GNUC_CONST;
CppJavaAssist *cpp_java_assist_new (IAnjutaEditorAssist *assist,
									IAnjutaSymbolManager *isymbol_manager,
									AnjutaPreferences *preferences);
gboolean cpp_java_assist_check (CppJavaAssist *assist, gboolean autocomplete,
								gboolean calltips);

G_END_DECLS

#endif /* _CPP_JAVA_ASSIST_H_ */

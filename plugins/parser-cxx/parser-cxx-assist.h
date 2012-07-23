/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * parser-cxx-anjuta
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

#ifndef _PARSER_CXX_ASSIST_H_
#define _PARSER_CXX_ASSIST_H_

#include <glib-object.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

G_BEGIN_DECLS

#define TYPE_PARSER_CXX_ASSIST             (parser_cxx_assist_get_type ())
#define PARSER_CXX_ASSIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PARSER_CXX_ASSIST, ParserCxxAssist))
#define PARSER_CXX_ASSIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PARSER_CXX_ASSIST, ParserCxxAssistClass))
#define IS_PARSER_CXX_ASSIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PARSER_CXX_ASSIST))
#define IS_PARSER_CXX_ASSIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PARSER_CXX_ASSIST))
#define PARSER_CXX_ASSIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PARSER_CXX_ASSIST, ParserCxxAssistClass))

typedef struct _ParserCxxAssistClass ParserCxxAssistClass;
typedef struct _ParserCxxAssist ParserCxxAssist;
typedef struct _ParserCxxAssistContext ParserCxxAssistContext;
typedef struct _ParserCxxAssistPriv ParserCxxAssistPriv;

struct _ParserCxxAssistContext {
	GCompletion* completion;
	GList* tips;
	gint position;
};

struct _ParserCxxAssistClass
{
	GObjectClass parent_class;
};

struct _ParserCxxAssist
{
	GObject parent_instance;
	ParserCxxAssistPriv *priv;
};

GType parser_cxx_assist_get_type (void) G_GNUC_CONST;

ParserCxxAssist*
parser_cxx_assist_new                             (IAnjutaEditor *ieditor,
                                                   IAnjutaSymbolManager *isymbol_manager,
                                                   GSettings* settings);

G_END_DECLS

#endif /* _PARSER_CXX_ASSIST_H_ */

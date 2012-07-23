/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * python-assist.h
 * Copyright (C) Ishan Chattopadhyaya 2009 <ichattopadhyaya@gmail.com>
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

#ifndef _PYTHON_ASSIST_H_
#define _PYTHON_ASSIST_H_

#include <glib-object.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

G_BEGIN_DECLS

#define TYPE_PYTHON_ASSIST             (python_assist_get_type ())
#define PYTHON_ASSIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PYTHON_ASSIST, PythonAssist))
#define PYTHON_ASSIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PYTHON_ASSIST, PythonAssistClass))
#define IS_PYTHON_ASSIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PYTHON_ASSIST))
#define IS_PYTHON_ASSIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PYTHON_ASSIST))
#define PYTHON_ASSIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PYTHON_ASSIST, PythonAssistClass))

typedef struct _PythonAssistClass PythonAssistClass;
typedef struct _PythonAssist PythonAssist;
typedef struct _PythonAssistContext PythonAssistContext;
typedef struct _PythonAssistPriv PythonAssistPriv;

struct _PythonAssistContext {
	GCompletion* completion;
	GList* tips;
	gint position;
};

struct _PythonAssistClass
{
	GObjectClass parent_class;
};

struct _PythonAssist
{
	GObject parent_instance;
	PythonAssistPriv *priv;
};

GType python_assist_get_type (void) G_GNUC_CONST;

PythonAssist*
python_assist_new                             (IAnjutaEditor *ieditor,
                                               IAnjutaSymbolManager *isymbol_manager,
                                               GSettings* settings,
                                               AnjutaPlugin *plugin,
                                               const gchar *project_root);

G_END_DECLS

#endif /* _PYTHON_ASSIST_H_ */

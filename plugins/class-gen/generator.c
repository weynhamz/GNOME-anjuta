/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  generator.c
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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "generator.h"

#include <plugins/project-wizard/autogen.h>

#include <libanjuta/anjuta-marshal.h>

#include <glib/gstdio.h>

typedef struct _CgGeneratorPrivate CgGeneratorPrivate;
struct _CgGeneratorPrivate
{
	NPWAutogen *autogen;
	
	gchar *header_template;
	gchar *source_template;
	gchar *header_destination;
	gchar *source_destination;
};

#define CG_GENERATOR_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE( \
		(o), \
		CG_TYPE_GENERATOR, \
		CgGeneratorPrivate \
	))

enum {
	PROP_0,

	/* Construct only */
	PROP_HEADER_TEMPLATE,
	PROP_SOURCE_TEMPLATE,
	PROP_HEADER_DESTINATION,
	PROP_SOURCE_DESTINATION
};

enum
{
	CREATED,
	ERROR,

	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint generator_signals[LAST_SIGNAL] = { 0 };

static gchar *
cg_generator_make_absolute (const gchar *path)
{
	gchar *current_dir;
	gchar *abs_path;

	/* TODO: Use gio stuff here? */
	if (g_path_is_absolute (path))
	{
		abs_path = g_strdup (path);
	}
	else
	{
		current_dir = g_get_current_dir ();
		abs_path = g_build_filename (current_dir, path, NULL);
		g_free (current_dir);
	}
	
	return abs_path;
}

static void
cg_generator_autogen_source_func (NPWAutogen *autogen,
                                  gpointer user_data)
{
	CgGenerator *generator;
	CgGeneratorPrivate *priv;
	GError *error;
	gboolean success;
	
	generator = CG_GENERATOR (user_data);
	priv = CG_GENERATOR_PRIVATE (generator);

	success = TRUE;
	if (g_file_test (priv->header_destination,
	                 G_FILE_TEST_IS_REGULAR) == FALSE)
	{
		if (g_file_test (priv->source_destination,
		                 G_FILE_TEST_IS_REGULAR) == TRUE)
		{
			g_unlink (priv->source_destination);
		}

		success = FALSE;
	}
	else if (g_file_test (priv->source_destination,
	                      G_FILE_TEST_IS_REGULAR) == FALSE)
	{
		/* It is guaranteed that header file is a regular file, otherwise
		 * the else branch would not have been executed */
		g_unlink (priv->source_destination);
		success = FALSE;
	}

	if (success == TRUE)
	{
		/* TODO: Check contents of destination file for autogen error */
		g_signal_emit(G_OBJECT(generator), generator_signals[CREATED], 0);
	}
	else
	{
		error = NULL;

		g_set_error(&error, g_quark_from_static_string("CG_GENERATOR_ERROR"),
		            CG_GENERATOR_ERROR_NOT_GENERATED,
		            _("Header or source file has not been created"));

		g_signal_emit (G_OBJECT (generator), generator_signals[ERROR],
		               0, error);
		g_error_free (error);
	}
}

static void
cg_generator_autogen_header_func (NPWAutogen *autogen,
                                  gpointer user_data)
{
	CgGenerator *generator;
	CgGeneratorPrivate *priv;
	gboolean result;
	GError *error;

	generator = CG_GENERATOR (user_data);
	priv = CG_GENERATOR_PRIVATE (generator);

	error = NULL;

	npw_autogen_set_input_file (priv->autogen, priv->source_template, NULL, NULL); /*"[+", "+]");*/
	npw_autogen_set_output_file (priv->autogen, priv->source_destination);

	result = npw_autogen_execute (priv->autogen,
	                              cg_generator_autogen_source_func,
	                              generator, &error);

	if (result == FALSE)
	{
		g_signal_emit (G_OBJECT(generator), generator_signals[ERROR],
		               0, error);
		g_error_free (error);
	}
}

static void
cg_generator_init (CgGenerator *generator)
{
	CgGeneratorPrivate *priv;
	priv = CG_GENERATOR_PRIVATE (generator);

	priv->autogen = npw_autogen_new ();
	
	priv->header_template = NULL;
	priv->source_template = NULL;
	priv->header_destination = NULL;
	priv->source_destination = NULL;
}

static void 
cg_generator_finalize (GObject *object)
{
	CgGenerator *generator;
	CgGeneratorPrivate *priv;
	
	generator = CG_GENERATOR (object);
	priv = CG_GENERATOR_PRIVATE (generator);

	npw_autogen_free (priv->autogen);
	
	g_free (priv->header_template);
	g_free (priv->source_template);
	g_free (priv->header_destination);
	g_free (priv->source_destination);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cg_generator_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
	CgGenerator *generator;
	CgGeneratorPrivate *priv;

	g_return_if_fail (CG_IS_GENERATOR (object));

	generator = CG_GENERATOR (object);
	priv = CG_GENERATOR_PRIVATE (generator);

	switch (prop_id)
	{
	case PROP_HEADER_TEMPLATE:
		g_free (priv->header_template);
		priv->header_template =
			cg_generator_make_absolute (g_value_get_string (value));
		break;
	case PROP_SOURCE_TEMPLATE:
		g_free (priv->source_template);
		priv->source_template =
			cg_generator_make_absolute (g_value_get_string (value));
		break;
	case PROP_HEADER_DESTINATION:
		g_free (priv->header_destination);
		priv->header_destination =
			cg_generator_make_absolute (g_value_get_string (value));
		break;
	case PROP_SOURCE_DESTINATION:
		g_free (priv->source_destination);
		priv->source_destination =
			cg_generator_make_absolute (g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_generator_get_property (GObject *object,
                           guint prop_id,
                           GValue *value, 
                           GParamSpec *pspec)
{
	CgGenerator *generator;
	CgGeneratorPrivate *priv;

	g_return_if_fail (CG_IS_GENERATOR (object));

	generator = CG_GENERATOR (object);
	priv = CG_GENERATOR_PRIVATE (generator);

	switch (prop_id)
	{
	case PROP_HEADER_TEMPLATE:
		g_value_set_string (value, priv->header_template);
		break;
	case PROP_SOURCE_TEMPLATE:
		g_value_set_string (value, priv->source_template);
		break;
	case PROP_HEADER_DESTINATION:
		g_value_set_string (value, priv->header_destination);
		break;
	case PROP_SOURCE_DESTINATION:
		g_value_set_string (value, priv->source_destination);
		break;		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cg_generator_class_init (CgGeneratorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private(klass, sizeof(CgGeneratorPrivate));

	object_class->finalize = cg_generator_finalize;
	object_class->set_property = cg_generator_set_property;
	object_class->get_property = cg_generator_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_HEADER_TEMPLATE,
	                                 g_param_spec_string ("header-template",
	                                                      "Header Template File",
	                                                      _("Autogen template used for the header file"),
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_SOURCE_TEMPLATE,
	                                 g_param_spec_string ("source-template",
	                                                      "Source Template File",
	                                                      _("Autogen template used for the implementation file"),
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_HEADER_DESTINATION,
	                                 g_param_spec_string ("header-destination",
	                                                      "Header Target File",
	                                                      _("File to which the processed template will be written"),
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_SOURCE_DESTINATION,
	                                 g_param_spec_string ("source-destination",
	                                                      "Source Target File",
	                                                      _("File to which the processed template will be written"),
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	generator_signals[CREATED] =
		g_signal_new ("created",
		              G_OBJECT_CLASS_TYPE(object_class),
		              G_SIGNAL_RUN_LAST,
		              0, /* no default handler */
		              NULL, NULL,
		              anjuta_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
		              0);

	generator_signals[ERROR] =
		g_signal_new ("error",
		              G_OBJECT_CLASS_TYPE(object_class),
		              G_SIGNAL_RUN_LAST,
		              0, /* no default handler */
		              NULL, NULL,
		              anjuta_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE,
		              1,
		              G_TYPE_POINTER); /* GError seems not to have its own GType */
}

GType
cg_generator_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (CgGeneratorClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cg_generator_class_init,
			NULL,
			NULL,
			sizeof (CgGenerator),
			0,
			(GInstanceInitFunc) cg_generator_init,
			NULL
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "CgGenerator",
		                                   &our_info, 0);
	}

	return our_type;
}

CgGenerator *
cg_generator_new (const gchar *header_template,
                  const gchar *source_template,
                  const gchar *header_destination,
                  const gchar *source_destination)
{
	GObject *object;

	object = g_object_new (CG_TYPE_GENERATOR,
	                       "header-template", header_template,
	                       "source-template", source_template,
	                       "header-destination", header_destination,
	                       "source-destination", source_destination, NULL);

	return CG_GENERATOR (object);
}

gboolean
cg_generator_run (CgGenerator *generator,
                  GHashTable *values,
                  GError **error)
{
	CgGeneratorPrivate *priv;
	priv = CG_GENERATOR_PRIVATE (generator);

	/* TODO: npw_autogen_write_definiton_file should take a GError... */
	if (npw_autogen_write_definition_file (priv->autogen, values) == FALSE)
	{
		g_set_error (error, g_quark_from_static_string("CG_GENERATOR_ERROR"),
		             CG_GENERATOR_ERROR_DEFFILE,
		             _("Failed to write autogen definition file"));

		return FALSE;
	}
	else
	{
		npw_autogen_set_input_file (priv->autogen, priv->header_template,
		                            NULL, NULL); /*"[+", "+]");*/
		npw_autogen_set_output_file (priv->autogen, priv->header_destination);

		return npw_autogen_execute (priv->autogen,
		                            cg_generator_autogen_header_func,
		                            generator, error);
	}
}

const gchar *
cg_generator_get_header_template (CgGenerator *generator)
{
	return CG_GENERATOR_PRIVATE (generator)->header_template;
}

const gchar *
cg_generator_get_source_template (CgGenerator *generator)
{
	return CG_GENERATOR_PRIVATE (generator)->source_template;
}

const gchar *
cg_generator_get_header_destination (CgGenerator *generator)
{
	return CG_GENERATOR_PRIVATE (generator)->header_destination;
}

const gchar *
cg_generator_get_source_destination(CgGenerator *generator)
{
	return CG_GENERATOR_PRIVATE (generator)->source_destination;
}

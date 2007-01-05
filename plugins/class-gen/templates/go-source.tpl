[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * [+ProjectName+][+IF (=(get "Headings") "1")+]
 * Copyright (C) [+AuthorName+] [+(shell "date +%Y")+] <[+AuthorEmail+]>[+ENDIF+]
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "ProjectName") (get "AuthorName") " * ")+]
[+ == "LGPL" +][+(lgpl (get "ProjectName") (get "AuthorName") " * ")+]
[+ == "GPL"  +][+(gpl  (get "ProjectName")                    " * ")+]
[+ESAC+] */

#include "[+HeaderFile+]"[+
IF (not (=(get "PrivateVariableCount") "0"))+]

typedef struct _[+ClassName+]Private [+ClassName+]Private;
struct _[+ClassName+]Private
{[+
	FOR Members +][+
		IF (=(get "Scope") "private")+][+
			IF (=(get "Arguments") "")+]
	[+
				Type+] [+
				Name+];[+
			ENDIF+][+
		ENDIF+][+
	ENDFOR+]
};

#define [+TypePrefix+]_[+TypeSuffix+]_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), [+TypePrefix+]_TYPE_[+TypeSuffix+], [+ClassName+]Private))[+

ENDIF+][+IF (not (=(get "Properties[0].Name") ""))+]

enum
{
	PROP_0,

[+
	FOR Properties ",\n" +]	PROP_[+
		(string-upcase(string->c-name!(get "Name")))+][+
	ENDFOR+]
};[+
ENDIF+][+IF (not (=(get "Signals[0].Name") ""))+]

enum
{[+
	FOR Signals+][+ "\n\t" +][+
		(string-upcase(string->c-name!(get "Name")))+],[+
	ENDFOR+]

	LAST_SIGNAL
};[+
ENDIF+]

static [+BaseClass+]Class* parent_class = NULL;[+
IF (not (=(get "Signals[0].Name") ""))+]
static guint [+ (string-downcase(get "TypeSuffix")) +]_signals[LAST_SIGNAL] = { 0 };[+
ENDIF+][+
FOR Members+][+
	IF (=(get "Scope") "private")+][+
		IF (not(=(get "Arguments") "")) +]

static [+Type+]
[+FuncPrefix+]_[+Name+] [+Arguments+]
{
	/* TODO: Add private function implementation here */
}[+
		ENDIF+][+
	ENDIF+][+
ENDFOR+]

static void
[+FuncPrefix+]_init ([+ClassName+] *object)
{
	/* TODO: Add initialization code here */
}

static void
[+FuncPrefix+]_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (parent_class)->finalize (object);
}[+
IF (not (=(get "Properties[0].Name") ""))+]

static void
[+FuncPrefix+]_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail ([+TypePrefix+]_IS_[+TypeSuffix+] (object));

	switch (prop_id)
	{[+
	FOR Properties+]
	case PROP_[+ (string-upcase(string->c-name!(get "Name")))+]:
		/* TODO: Add setter for [+ (c-string(get "Name")) +] property here */
		break;[+
	ENDFOR+]
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
[+FuncPrefix+]_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail ([+TypePrefix+]_IS_[+TypeSuffix+] (object));

	switch (prop_id)
	{[+
	FOR Properties+]
	case PROP_[+ (string-upcase(string->c-name!(get "Name")))+]:
		/* TODO: Add getter for [+ (c-string(get "Name")) +] property here */
		break;[+
	ENDFOR+]
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}[+
ENDIF+][+
IF (not (=(get "Signals[0].Name") "")) +]

[+
	FOR Signals "\n\n"
+]static [+Type+]
[+FuncPrefix+]_[+ (string-downcase(string->c-name!(get "Name")))+] [+Arguments+]
{
	/* TODO: Add default signal handler implementation here */
}[+
	ENDFOR+][+
ENDIF+]

static void
[+FuncPrefix+]_class_init ([+ClassName+]Class *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = [+BaseTypePrefix+]_[+BaseTypeSuffix+]_CLASS (g_type_class_peek_parent (klass));[+
IF (not (=(get "PrivateVariableCount") "0"))+]

	g_type_class_add_private (klass, sizeof ([+ClassName+]Private));[+
ENDIF+]

	object_class->finalize = [+FuncPrefix+]_finalize;[+
IF (not (=(get "Properties[0].Name") "")) +]
	object_class->set_property = [+FuncPrefix+]_set_property;
	object_class->get_property = [+FuncPrefix+]_get_property;[+
ENDIF+][+
IF (not (=(get "Signals[0].Name") "")) +]

[+
	FOR Signals "\n"+]	klass->[+
		(string-downcase(string->c-name!(get "Name")))+] = [+
		FuncPrefix+]_[+
		(string-downcase(string->c-name!(get "Name")))+];[+
	ENDFOR+][+
ENDIF+][+
IF (not (=(get "Properties[0].Name") "")) +]

[+
	FOR Properties "\n\n"+]	g_object_class_install_property (object_class,
	                                 PROP_[+ (string-upcase(string->c-name!(get "Name")))+],
	                                 [+ ParamSpec +] ([+ (c-string(get "Name"))+],[+
		CASE (get "ParamSpec")+][+
		== "g_param_spec_boolean"+]
	                                                       [+ (c-string(get "Nick"))+],
	                                                       [+ (c-string(get "Blurb"))+],
	                                                       [+Default+],
	                                                       [+Flags+][+
		== "g_param_spec_boxed"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     [+Type+],
	                                                     [+Flags+][+
		== "g_param_spec_char"+]
	                                                    [+ (c-string(get "Nick"))+],
	                                                    [+ (c-string(get "Blurb"))+],
	                                                    G_MININT8, /* TODO: Adjust minimum property value */
	                                                    G_MAXINT8, /* TODO: Adjust maximum property value */
	                                                    [+Default+],
	                                                    [+Flags+][+
		== "g_param_spec_double"+]
	                                                      [+ (c-string(get "Nick"))+],
	                                                      [+ (c-string(get "Blurb"))+],
	                                                      G_MINDOUBLE, /* TODO: Adjust minimum property value */
	                                                      G_MAXDOUBLE, /* TODO: Adjust maximum property value */
	                                                      [+Default+],
	                                                      [+Flags+][+
		== "g_param_spec_enum"+]
	                                                    [+ (c-string(get "Nick"))+],
	                                                    [+ (c-string(get "Blurb"))+],
	                                                    [+Type+],
	                                                    [+Default+],
	                                                    [+Flags+][+
		== "g_param_spec_flags"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     [+Type+],
	                                                     [+Default+],
	                                                     [+Flags+][+
		== "g_param_spec_float"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     G_MINFLOAT, /* TODO: Adjust minimum property value */
	                                                     G_MAXFLOAT, /* TODO: Adjust maximum property value */
	                                                     [+Default+],
	                                                     [+Flags+][+
		== "g_param_spec_int"+]
	                                                   [+ (c-string(get "Nick"))+],
	                                                   [+ (c-string(get "Blurb"))+],
	                                                   G_MININT, /* TODO: Adjust minimum property value */
	                                                   G_MAXINT, /* TODO: Adjust maximum property value */
	                                                   [+Default+],
	                                                   [+Flags+][+
		== "g_param_spec_int64"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     G_MININT64, /* TODO: Adjust minimum property value */
	                                                     G_MAXINT64, /* TODO: Adjust maximum property value */
	                                                     [+Default+],
	                                                     [+Flags+][+
		== "g_param_spec_long"+]
	                                                    [+ (c-string(get "Nick"))+],
	                                                    [+ (c-string(get "Blurb"))+],
	                                                    G_MINLONG, /* TODO: Adjust minimum property value */
	                                                    G_MAXLONG, /* TODO: Adjust maximum property value */
	                                                    [+Default+],
	                                                    [+Flags+][+
		== "g_param_spec_object"+]
	                                                      [+ (c-string(get "Nick"))+],
	                                                      [+ (c-string(get "Blurb"))+],
	                                                      [+Type+],
	                                                      [+Flags+][+
		== "g_param_spec_param"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     [+Type+],
	                                                     [+Flags+][+
		== "g_param_spec_pointer"+]
	                                                       [+ (c-string(get "Nick"))+],
	                                                       [+ (c-string(get "Blurb"))+],
	                                                       [+Flags+][+
		== "g_param_spec_string"+]
	                                                      [+ (c-string(get "Nick"))+],
	                                                      [+ (c-string(get "Blurb"))+],
	                                                      [+ (c-string(get "Default")) +],
	                                                      [+Flags+][+
		== "g_param_spec_uchar"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     0, /* TODO: Adjust minimum property value */
	                                                     G_MAXUINT8, /* TODO: Adjust maximum property value */
	                                                     [+Default+],
	                                                     [+Flags+][+
		== "g_param_spec_uint"+]
	                                                    [+ (c-string(get "Nick"))+],
	                                                    [+ (c-string(get "Blurb"))+],
	                                                    0, /* TODO: Adjust minimum property value */
	                                                    G_MAXUINT, /* TODO: Adjust maximum property value */
	                                                    [+Default+],
	                                                    [+Flags+][+
		== "g_param_spec_uint64"+]
	                                                      [+ (c-string(get "Nick"))+],
	                                                      [+ (c-string(get "Blurb"))+],
	                                                      0, /* TODO: Adjust minimum property value */
	                                                      G_MAXUINT64, /* TODO: Adjust maximum property value */
	                                                      [+Default+],
	                                                      [+Flags+][+
		== "g_param_spec_ulong"+]
	                                                     [+ (c-string(get "Nick"))+],
	                                                     [+ (c-string(get "Blurb"))+],
	                                                     0, /* TODO: Adjust minimum property value */
	                                                     G_MAXULONG, /* TODO: Adjust maximum property value */
	                                                     [+Default+],
	                                                     [+Flags+][+
		== "g_param_spec_unichar"+]
	                                                       [+ (c-string(get "Nick"))+],
	                                                       [+ (c-string(get "Blurb"))+],
	                                                       [+Default+],
	                                                       [+Flags+][+
		ESAC+]));[+
	ENDFOR+][+
ENDIF+][+
IF (not (=(get "Signals[0].Name") "")) +]

[+
	FOR Signals "\n\n" +]	[+
	(string-downcase(get "TypeSuffix")) +]_signals[[+
	(string-upcase(string->c-name!(get "Name")))+]] =
		g_signal_new ([+ (c-string(get "Name")) +],
		              G_OBJECT_CLASS_TYPE (klass),
		              [+ Flags +],
		              G_STRUCT_OFFSET ([+ClassName+]Class, [+ (string->c-name!(get "Name"))+]),
		              NULL, NULL,
		              [+ Marshaller +],
		              [+GTypePrefix+]_TYPE_[+GTypeSuffix+], [+ArgumentCount+],
		              [+ArgumentGTypes+]);[+
	ENDFOR+][+
ENDIF +]
}

GType
[+FuncPrefix+]_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof ([+ClassName+]Class), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) [+FuncPrefix+]_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof ([+ClassName+]), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) [+FuncPrefix+]_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static ([+BaseTypePrefix+]_TYPE_[+BaseTypeSuffix+], [+ (c-string (get "ClassName")) +],
		                                   &our_info, 0);
	}

	return our_type;
}[+
FOR Members+][+
	IF (=(get "Scope") "public")+][+
		IF (not(=(get "Arguments") ""))+]

[+Type+]
[+FuncPrefix+]_[+Name+] [+Arguments+]
{
	/* TODO: Add public function implementation here */
}[+
		ENDIF+][+
	ENDIF+][+
ENDFOR+]

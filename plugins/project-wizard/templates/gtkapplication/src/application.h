[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * [+NameLower+].h
 * Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
 * 
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "Name") OWNER=(get "Author") \+]
 */

#ifndef _[+NameCUpper+]_
#define _[+NameCUpper+]_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define [+NameCUpper+]_TYPE_APPLICATION             ([+NameCLower+]_get_type ())
#define [+NameCUpper+]_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), [+NameCUpper+]_TYPE_APPLICATION, [+NameCClass+]))
#define [+NameCUpper+]_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), [+NameCUpper+]_TYPE_APPLICATION, [+NameCClass+]Class))
#define [+NameCUpper+]_IS_APPLICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), [+NameCUpper+]_TYPE_APPLICATION))
#define [+NameCUpper+]_IS_APPLICATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), [+NameCUpper+]_TYPE_APPLICATION))
#define [+NameCUpper+]_APPLICATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), [+NameCUpper+]_TYPE_APPLICATION, [+NameCClass+]Class))

typedef struct _[+NameCClass+]Class [+NameCClass+]Class;
typedef struct _[+NameCClass+] [+NameCClass+];
[+IF (=(get "HaveBuilderUI") "1")+]typedef struct _[+NameCClass+]Private [+NameCClass+]Private;[+ENDIF+]

struct _[+NameCClass+]Class
{
	GtkApplicationClass parent_class;
};

struct _[+NameCClass+]
{
	GtkApplication parent_instance;
[+IF (=(get "HaveBuilderUI") "1")+]
	[+NameCClass+]Private *priv;
[+ENDIF+]
};

GType [+NameCLower+]_get_type (void) G_GNUC_CONST;
[+NameCClass+] *[+NameCLower+]_new (void);

/* Callbacks */

G_END_DECLS

#endif /* _APPLICATION_H_ */

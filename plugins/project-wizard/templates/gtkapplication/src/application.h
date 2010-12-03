[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * [+NameLower+].h
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "Name") (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl (get "Name") (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  (get "Name")                " * ")+]
[+ESAC+] */

#ifndef _[+NameCUppper+]_
#define _[+NameCUppper+]_

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

struct _[+NameCClass+]Class
{
	GtkApplicationClass parent_class;
};

struct _[+NameCClass+]
{
	GtkApplication parent_instance;
};

GType [+NameCLower+]_get_type (void) G_GNUC_CONST;
[+NameCClass+] *[+NameCLower+]_new (void);

G_END_DECLS

#endif /* _APPLICATION_H_ */

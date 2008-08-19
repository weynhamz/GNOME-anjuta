[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  "callbacks.h" (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl "callbacks.h" (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  "callbacks.h"                " * ")+]
[+ESAC+] */

#include <gtk/gtk.h>

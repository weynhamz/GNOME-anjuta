[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

[+IF (=(get "IncludeGNUHeader") "1") +]/*
	interface.h
	Copyright (C) [+Author+]

[+(gpl "interface.h"  "\t")+]
*/[+ENDIF+]

GtkWidget* create_window1 (void);

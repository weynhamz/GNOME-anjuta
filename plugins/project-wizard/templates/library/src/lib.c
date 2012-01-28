[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * lib.c
 * Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
 * 
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "Name") OWNER=(get "Author") \+]
 */

#include <stdio.h>

int [+NameCLower+]_func(void)
{
	printf("Hello world\n");
	return (0);
}

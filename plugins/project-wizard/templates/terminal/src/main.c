[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

[+IF (=(get "IncludeGNUHeader") "1") +]/*
  main.c
  Copyright (C) [+Author+]

[+CASE (get "License") +]
[+ == "BSD" +][+(bsd "main.c" (get "Author") "  ")+]
[+ == "LGPL" +][+(lgpl "main.c" (get "Author") "  ")+]
[+ == "GPL" +][+(gpl "main.c"  "  ")+]
[+ESAC+]
*/[+ENDIF+]


#include <stdio.h>
int main()
{
	printf("Hello world\n");
	return (0);
}

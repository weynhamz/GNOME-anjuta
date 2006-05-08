[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  "main.cpp" (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl "main.cpp" (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  "main.cpp"                " * ")+]
[+ESAC+] */

#include <iostream>

int main()
{
	std::cout << "Hello world!" << std::endl;
	return 0;
}

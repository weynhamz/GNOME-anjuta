[+ autogen5 template +]
/*
 * main.c
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 *
[+CASE (get "License") +]
[+ == "BSD" +][+(bsd "main.c" (get "Author") "\t")+]
[+ == "LGPL" +][+(lgpl "main.c" (get "Author") "\t")+]
[+ == "GPL" +][+(gpl "main.c"  "\t")+]
[+ESAC+] */

/*Program closes with a mouse click or keypress */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
[+IF (=(get "HaveSDL_net") "1")+]
#include <SDL/SDL_net.h>
[+ENDIF+]

int main (int argc, char *argv[])
{
	int i;
[+IF (=(get "HaveSDL_net") "1")+]
	IPaddress local = {0x0100007F, 0x50};
[+ENDIF+]
	
	printf ("Initializing SDL.\n");
	
	/* Initializes Audio and the CDROM, add SDL_INIT_VIDEO for Video */
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_CDROM)< 0)
	{
    	printf("Could not initialize SDL:%s\n", SDL_GetError());
		SDL_Quit();
		
		return 1;
	}
	printf("Audio & CDROM initialized correctly\n");
	
    /* Trying to read number of CD devices on system */
	printf("Drives available: %d\n", SDL_CDNumDrives());
    	for (i=0; i < SDL_CDNumDrives(); ++i)
	{
		printf("Drive %d\"%s\"\n", i, SDL_CDName(i));
    	}
	
[+IF (=(get "HaveSDL_net") "1")+]
	if (SDLNet_Init() < 0)
	{
		printf("Could not initialize SDL_net:%s\n", SDLNet_GetError());
		SDL_Quit();

		return 1;
	}
	printf("\n\nNetwork initialized correctly\n");
	
	/* Get host name */
	printf("Hostname: %s\n", SDLNet_ResolveIP(&local));

	SDLNet_Quit();
[+ENDIF+]
	
	SDL_Quit();
	
	return(0);
}

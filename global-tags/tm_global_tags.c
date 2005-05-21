#include "stdlib.h"
#include "tm_tagmanager.h"

static char *pre_process = "gcc -E -dD -p";
int main(int argc, char **argv)
{
	if (argc > 2)
	{
		/* Create global taglist */
		int status;
		char *command;
		command = g_strdup_printf("%s %s", pre_process,
								  NVL(getenv("CFLAGS"), ""));
		//printf(">%s<\n", command);
		status = tm_workspace_create_global_tags(command,
												 (const char **) (argv + 2),
												 argc - 2, argv[1]);
		g_free(command);
		if (!status)
			return 1;
	}
	else
	{
		fprintf(stderr, "Usage: %s <Tag File> <File list>\n", argv[0]);
		return 1;
	}
	return 0;
}

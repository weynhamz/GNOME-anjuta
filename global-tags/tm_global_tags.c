#include "stdlib.h"
#include "tm_tagmanager.h"

static char *pre_process = "gcc -E -dD -p";
int main(int argc, char **argv)
{
	if (argc > 2)
	{
		/* Create global taglist */
		int i, status;
		char *command;
		GString *includes = g_string_new(NULL);
		for (i = 2; i < argc; ++i)
		{
			g_string_append(includes, argv[i]);
			g_string_append_c(includes, ' ');
		}
		command = g_strdup_printf("%s %s", pre_process, NVL(getenv("CFLAGS"), ""));
		status = tm_workspace_create_global_tags(command, includes->str, argv[1]);
		g_free(command);
		g_string_free(includes, TRUE);
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

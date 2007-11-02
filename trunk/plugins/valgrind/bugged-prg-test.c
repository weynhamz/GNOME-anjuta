#include <stdlib.h>
#include <stdio.h>


struct _fake {
	int a;
	char *b;
} fake;




int main(int argc, char**argv)
{
	int i;
	char **vec;
	
	vec = (char**)calloc (5, sizeof(char*));
	
	for (i=0; i < 10; i++)
		vec[i] = (char *)'x';
	
	return 0;
}

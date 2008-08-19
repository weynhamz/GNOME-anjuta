# include "stdio.h"

char *buffer; /* Comments to the right of declarations */
char *start, *end, *last;
char *name;
/*         This separates blocks of declarations */
int baz;

struct square {int x;int y;};

#ifdef ENABLE_NLS
bindtextdomain (GETTEXT_PACKAGE,
		PACKAGE_LOCALE_DIR);
#else /* Comments to the right of preproc directives */
textdomain (PACKAGE);
#endif

int foo(int number,int len, char *name)
{
if(number >0) { 
for(int i=0;i<7;i++)
len ++;
number--;}
else
{while(len)
{len--};
number++;}
puts("Hi");
}
   /* The procedure bar is even less interesting. */
char * bar(int nb)
{
long c;
c =(long)foo(2, 5, "end");
puts("Hello"); /* Comments to the right of code */
switch(nb) {
case 0: break;
case 1:{nb++; break;}
default: break;
}
}

int bool_test(char *mask)
{
if(mask 
&& ((mask[0]=='\0') || 
(mask[1]=='\0' && ((mask[0]=='0')||(mask[0]=='*')))))
return 0;
}


 
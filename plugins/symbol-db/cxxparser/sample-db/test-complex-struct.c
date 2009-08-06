int var_alone;

typedef struct _asd {
	char a; 
	int b; 
} asd;

typedef struct _foo {
	char c;
	void *d;

	asd *asd_struct;
} foo;


int main () { 
	asd *var; 
	((foo*)var)->asd_struct->

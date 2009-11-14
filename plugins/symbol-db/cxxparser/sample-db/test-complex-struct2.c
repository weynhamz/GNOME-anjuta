int var_alone;

typedef struct _qwe {
	int e;
	int f;
	
} qwe;

typedef struct _asd {
	char a; 
	int b;

	qwe * qwe_struct;
} asd;

typedef struct _foo {
	char c;
	void *d;

	asd *asd_struct;
} foo;


int main () { 
	foo *var; 

	var->asd_struct->qwe_struct->

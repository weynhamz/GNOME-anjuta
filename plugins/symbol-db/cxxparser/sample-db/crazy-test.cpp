


typedef struct A_t {
	char *a;
} A; 


typedef struct B_t {
	int b;
} B; 

typedef struct C_t {
	void * c;
} C; 


int main ()
{
	C* foo;

	int f = ((B*)(foo))->b;

	char *buf = ((A*)((B*)(foo)))->a;
	
	return 0;
}

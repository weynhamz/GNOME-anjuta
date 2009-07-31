

struct _asd {
	char a; 
	int b; 
};

struct _foo {
	char c;
	void *d;
};

void main () { 
	_asd var; 
	((_foo)var).

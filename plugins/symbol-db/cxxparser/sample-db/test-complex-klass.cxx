// FirstKlass Decl
class FirstKlass 
{
private:
	int foo1_private ();
	void foo2_private () {
	};

	const char * foo3_private () {
		return "hey";
	};
	
public:
	FirstKlass () {};
};

// SecondKlass Decl
class SecondKlass 
{
private:

	int foo4_private ();
	void foo5_private () {
		return;
	};
	
protected:
	char foo6_protected () {
		return 'c';
	};
	
public:
	SecondKlass (int a, char b, char * c);

	// data
	FirstKlass *pm_first_klass;
};


int main ()
{

	SecondKlass * kl = new SecondKlass ();

	kl->pm_first_klass->

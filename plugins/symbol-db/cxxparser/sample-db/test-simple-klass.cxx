class FirstKlass {

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



int main ()
{

	FirstKlass * kl = new FirstKlass ();

	kl->

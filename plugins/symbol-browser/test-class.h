
namespace First {

	

	class First_1_class  {
	
		public:
			First_1_class ();
	
	
	};


	namespace Second {
		class Second_1_class {
			public:
			Second_1_class () {};
			
			static void func_second_1_class_foo() {};
		};
	}


}


namespace Third {

	class Third_1_class {
	
		public:
			Third_1_class ();
			
			
	
	};

	namespace Fourth {
		
		int my_fourth_global_var;
		char *heyyy;
		
		class Fourth_1_class {
			public:
			Fourth_1_class () {};
			
			void func_fourth_1_class_foo (void) {};
		};
		
	}

}


class MyClass {

public:
	MyClass ();


private:

	void mc_first_func( void );


};



class YourClass {


public:
	YourClass ();
	YourClass (int par1, char* par2);

	void yc_first_function( int par1, char** par2);

private:



};

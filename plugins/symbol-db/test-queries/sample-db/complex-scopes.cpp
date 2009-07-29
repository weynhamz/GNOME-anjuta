
namespace NSOne {

	/* this class is a copy of the one in namespace NSFour */
	class OneA {
		OneA () {};

		private:
			int foo (void ) {};
			
	}
	
	namespace NSTwo {
		struct TwoA {
			char a;
			int b;
			void *c;
		};

		typedef struct _TwoB {
			char *hey;
			int dude;
			
		} TwoB;

		void TwoC (int a, char b, void *c);
	}


	namespace NSTree {

		
	}

}



/* the bastard one :) */
namespace NSFour {

	/* simulate some problems with naming... */
	/* the exact class as in NSOne */
	class OneA {
		OneA () {};

		private:
			int foo (void ) {};
			
	}
}

#include <iostream>

#ifdef QEMUCSD_DEBUG
#include <backward.hpp>
using namespace backward;
#endif

int main(int argc, char* argv[]) {
	#ifdef QEMUCSD_DEBUG
	StackTrace st; st.load_here(32);
	Printer p; p.print(st);
	#else
	std::cout << "Target not build as type debug, backward not linked" <<
		std::endl;
	#endif
}
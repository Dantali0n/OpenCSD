#include <boost/locale.hpp>
#include <iostream>

using namespace std;
using namespace boost::locale;

int main()
{
	generator gen;
	// Specify location of dictionaries
	gen.add_messages_path(".");
	gen.add_messages_domain("hello");
	// Generate locales and imbue them to iostream
	locale::global(gen(""));
	cout.imbue(locale());
	// Display a message using current system locale
	cout << translate("Hello World") << endl;
}

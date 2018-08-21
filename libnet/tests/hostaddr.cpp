#include "net.h"
#include "host.h"
#include <iostream>

int
main(int argc, const char **argv)
{
	snf::net::initialize();

	try {
		snf::net::host h { argv[1] };
		std::cout << "Canonical Name: " << h.get_canonical_name() << std::endl;

		const std::vector<std::string> &names = h.get_names();
		std::vector<std::string>::const_iterator I;
		for (I = names.begin(); I != names.end(); ++I) {
			std::cout << "Other names: " << *I << std::endl;
		}

		const std::vector<snf::net::internet_address> &ias = h.get_internet_addresses();
		std::vector<snf::net::internet_address>::const_iterator J;
		for (J = ias.begin(); J != ias.end(); ++J) {
			std::cout << "Internet Address: " << J->str(false) << std::endl;
		}
	} catch (std::system_error e) {
		std::cerr << "Error Code: " << e.code() << std::endl;
		std::cerr << "Error: " << e.what() << std::endl;
	}

}

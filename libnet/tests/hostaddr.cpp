#include "net.h"
#include "host.h"
#include <iostream>

enum operation { none, lookup, compare };

int
main(int argc, const char **argv)
{
	enum operation op = none;

	if (argc < 2) {
		std::cerr << "specify at least one argument" << std::endl;
		return 1;
	}

	if (argc == 2)
		op = lookup;
	else if (argc == 3)
		op = compare;

	try {
		snf::net::initialize(false);

		if (op == lookup) {
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
		} else if (op == compare) {
			std::string h1 { argv[1] };
			std::string h2 { argv[2] };
			bool equal = snf::net::hosteq(h1, h2);
			std::cout << h1 << " and " << h2 << " are";
			if (!equal)
				std::cout << " not";
			std::cout << " same" << std::endl;
		}
	} catch (std::system_error e) {
		std::cerr << "Error Code: " << e.code() << std::endl;
		std::cerr << "Error: " << e.what() << std::endl;
	} catch (std::runtime_error e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}

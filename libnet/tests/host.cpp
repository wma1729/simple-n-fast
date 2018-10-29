#include "net.h"
#include "host.h"
#include <iostream>

enum operation { none, lookup, compare };

static int
usage(const std::string &prog)
{
	std::cerr << prog << " lookup <host>" << std::endl;
	std::cerr << prog << " compare <host1> <host2>" << std::endl;
	return 1;
}

int
main(int argc, const char **argv)
{
	enum operation op = none;
	std::string h1;
	std::string h2;

	for (int i = 1; i < argc; ++i) {
		if (snf::streq("lookup", argv[i])) {
			op = lookup;
			if (argv[i + 1]) {
				h1.assign(argv[++i]);
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("compare", argv[i])) {
			op = compare;
			if (argv[i + 1] && argv[i + 2]) {
				h1.assign(argv[++i]);
				h2.assign(argv[++i]);
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
			}
		} else {
			return usage(argv[0]);
		}
	}

	if (op == none) {
		std::cerr << "specify at least one argument" << std::endl;
		return usage(argv[0]);
	}

	try {
		snf::net::initialize(false);

		if (op == lookup) {
			snf::net::host h { h1 };
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
			bool equal = snf::net::hosteq(h1, h2);
			std::cout << h1 << " and " << h2 << " are";
			if (!equal)
				std::cout << " not";
			std::cout << " equal" << std::endl;
		}
	} catch (std::system_error e) {
		std::cerr << "Error Code: " << e.code() << std::endl;
		std::cerr << "Error: " << e.what() << std::endl;
	} catch (std::runtime_error e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}

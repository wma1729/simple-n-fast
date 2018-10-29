#include "net.h"
#include "cnxn.h"
#include <iostream>

struct arguments
{
	snf::net::connection_mode cnxn_mode;
	std::string host;
	std::string port;
};

static int
usage(const std::string &prog)
{
	std::cerr << prog << " connect -host <host> -port <port>" << std::endl;
	std::cerr << prog << " bind -port <port>" << std::endl;
	return 1;
}

static int
parse_arguments(arguments &args, int argc, const char **argv)
{
	args.cnxn_mode = snf::net::connection_mode::client;

	for (int i = 1; i < argc; ++i) {
		if (snf::streq("connect", argv[i])) {
			args.cnxn_mode = snf::net::connection_mode::client;
		} else if (snf::streq("bind", argv[i])) {
			args.cnxn_mode = snf::net::connection_mode::server;
		} else if (snf::streq("-host", argv[i])) {
			if (argv[i + 1]) {
				args.host = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-port", argv[i])) {
			if (argv[i + 1]) {
				args.port = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else {
			std::cerr << "invalid argument " << argv[i] << std::endl;
			return usage(argv[0]);
		}
	}

	return 0;
}

int
main(int argc, const char **argv)
try {
	arguments args;
	int retval = parse_arguments(args, argc, argv);
	if (retval != 0)
		return retval;

	return 0;
} catch (std::invalid_argument ex) {
	std::cerr << ex.what() << std::endl;
} catch (snf::net::ssl::ssl_exception ex) {
	std::cerr << ex.what() << std::endl;
	std::vector<snf::net::ssl::ssl_error>::const_iterator I;
	for (I = ex.begin(); I != ex.end(); ++I)
		std::cerr << *I << std::endl;
} catch (std::system_error ex) {
	std::cerr << "Error Code: " << ex.code() << std::endl;
	std::cerr << "Error: " << ex.what() << std::endl;
} catch (std::runtime_error ex) {
	std::cerr << ex.what() << std::endl;
}

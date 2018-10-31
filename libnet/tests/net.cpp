#include "net.h"
#include "cnxn.h"
#include <iostream>
#include <thread>

struct arguments
{
	snf::net::connection_mode cnxn_mode;
	std::string host;
	in_port_t port;

	arguments()
		: cnxn_mode(snf::net::connection_mode::client)
		, host("localhost")
		, port(16781)
	{
	}
	
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
				args.port = static_cast<in_port_t>(atoi(argv[++i]));
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

static int
client(arguments &args)
{
	int retval = E_ok;
	snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

	sock.keepalive(true);
	sock.tcpnodelay(true);

	sock.connect(AF_INET, args.host, args.port);

	std::string buf1, buf2;
	while (std::getline(std::cin, buf1)) {
		retval = sock.write_string(buf1);
		if (E_ok == retval) {
			retval = sock.read_string(buf2);
			if (E_ok == retval) {
				std::cout << buf2 << std::endl;
			}
		}

		if (E_ok == retval) {
			buf1.clear();
			buf2.clear();
		} else {
			break;
		}
	}

	sock.close();

	return 0;
}

static void
worker_thread(snf::net::socket &&sock)
{
	int retval = E_ok;
	std::string buf;

	do {
		retval = sock.read_string(buf);
		if (E_ok == retval)
			retval = sock.write_string(buf);
		buf.clear();
	} while (E_ok == retval);

	sock.close();
}

static int
server(arguments &args)
{
	snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

	sock.keepalive(true);
	sock.tcpnodelay(true);
	sock.reuseaddr(true);

	sock.bind(AF_INET, args.port);
	sock.listen(20);

	while (true) {
		snf::net::socket nsock = std::move(sock.accept());
		std::thread wt(worker_thread, std::move(nsock));
		wt.detach();
	}

	sock.close();

	return 0;
}

int
main(int argc, const char **argv)
try {
	arguments args;

	int retval = parse_arguments(args, argc, argv);
	if (retval == 0) {
		if (args.cnxn_mode == snf::net::connection_mode::client)
			retval = client(args);
		else
			retval = server(args);
	}

	return retval;
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

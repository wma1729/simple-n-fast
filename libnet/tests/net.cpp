#include "net.h"
#include "cnxn.h"
#include <iostream>
#include <thread>

struct arguments
{
	snf::net::connection_mode cnxn_mode;
	std::string host;
	in_port_t port;
	bool use_ssl;
	std::string keyfile;
	snf::net::ssl::data_fmt kf_fmt;
	std::string kf_password;
	std::string certfile;
	snf::net::ssl::data_fmt cf_fmt;
	std::string cafile;
	std::string crlfile;
	bool verify_peer;
	bool require_cert;
	int depth;

	arguments()
		: cnxn_mode(snf::net::connection_mode::client)
		, host("localhost")
		, port(16781)
		, use_ssl(false)
		, verify_peer(false)
		, require_cert(false)
		, depth(2)
	{
		kf_fmt = snf::net::ssl::data_fmt::pem;
		cf_fmt = snf::net::ssl::data_fmt::pem;
	}
	
};

static snf::net::ssl::context g_ctx;

static int
usage(const std::string &prog)
{
	std::cerr << prog << " (connect|bind) [-host <host>] [-port <port>] -ssl" << std::endl;
	std::cerr << "    -key <key-file> -keypass <key-password> -keyfmt <der|pem>" << std::endl;
	std::cerr << "    -cert <certificate-chain> -certfmt <der|pem>" << std::endl;
	std::cerr << "    -ca <ca-bundle-file> -crl <crl-file>" << std::endl;
	std::cerr << "    -verify -reqcert -depth <certificate-chain-depth>" << std::endl;
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
		} else if (snf::streq("-ssl", argv[i])) {
			args.use_ssl = true;
		} else if (snf::streq("-key", argv[i])) {
			if (argv[i + 1]) {
				args.keyfile = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-keypass", argv[i])) {
			if (argv[i + 1]) {
				args.kf_password = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-keyfmt", argv[i])) {
			if (argv[i + 1]) {
				std::string type = argv[++i];
				if (snf::streq("der", type))
					args.kf_fmt = snf::net::ssl::data_fmt::der;
				else
					args.kf_fmt = snf::net::ssl::data_fmt::pem;
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-cert", argv[i])) {
			if (argv[i + 1]) {
				args.certfile = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-certfmt", argv[i])) {
			if (argv[i + 1]) {
				std::string type = argv[++i];
				if (snf::streq("der", type))
					args.cf_fmt = snf::net::ssl::data_fmt::der;
				else
					args.cf_fmt = snf::net::ssl::data_fmt::pem;
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-ca", argv[i])) {
			if (argv[i + 1]) {
				args.cafile = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-crl", argv[i])) {
			if (argv[i + 1]) {
				args.crlfile = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-verify", argv[i])) {
			args.verify_peer = true;
		} else if (snf::streq("-reqcert", argv[i])) {
			args.require_cert = true;
		} else if (snf::streq("-depth", argv[i])) {
			if (argv[i + 1]) {
				args.depth = atoi(argv[++i]);
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else {
			std::cerr << "invalid argument " << argv[i] << std::endl;
			return usage(argv[0]);
		}
	}

	if (!args.use_ssl)
		return 0;

	if (args.cafile.empty()) {
		std::cerr << "-ca is required for client mode" << std::endl;
		return usage(argv[0]);
	}

	if (snf::net::connection_mode::server == args.cnxn_mode) {
		if (args.keyfile.empty()) {
			std::cerr << "-key is required for server mode" << std::endl;
			return usage(argv[0]);
		}

		if (args.certfile.empty()) {
			std::cerr << "-cert is required for server mode" << std::endl;
			return usage(argv[0]);
		}
	} else {
		args.verify_peer = true;
		args.require_cert = true;
	}

	return 0;
}

static void
prepare_context(const arguments &args)
{
	if (!args.keyfile.empty()) {
		const char *passwd = args.kf_password.empty() ? nullptr : args.kf_password.c_str();
		snf::net::ssl::pkey key { args.kf_fmt, args.keyfile, passwd };
		g_ctx.use_private_key(key);
	}

	if (!args.certfile.empty()) {
		snf::net::ssl::x509_certificate cert { args.cf_fmt, args.certfile };
		g_ctx.use_certificate(cert);
	}

	if (!args.keyfile.empty() && !args.certfile.empty())
		g_ctx.check_private_key();

	snf::net::ssl::truststore store { args.cafile };
	if (!args.crlfile.empty()) {
		snf::net::ssl::x509_crl crl { args.crlfile };
		store.add_crl(crl);
	}

	g_ctx.use_truststore(store);	

	if (args.verify_peer)
		g_ctx.verify_peer(args.require_cert);

	g_ctx.limit_certificate_chain_depth(args.depth);
}

static int
client(const arguments &args)
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
server(const arguments &args)
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
		snf::net::initialize(args.use_ssl);

		if (args.use_ssl)
			prepare_context(args);

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

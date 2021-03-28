#include "net.h"
#include "cnxn.h"
#include <iostream>
#include <thread>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <csignal>

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
	std::string altcertfile;
	snf::net::ssl::data_fmt cf_fmt;
	std::string cafile;
	std::string crlfile;
	std::string snihost;
	std::string session_file;
	bool verify_peer;
	bool require_cert;
	bool check_hosts;
	bool save_session;
	bool restore_session;
	bool use_ticket;
	int depth;

	arguments()
		: cnxn_mode(snf::net::connection_mode::client)
		, host("localhost")
		, port(16781)
		, use_ssl(false)
		, verify_peer(false)
		, require_cert(false)
		, check_hosts(false)
		, save_session(false)
		, restore_session(false)
		, use_ticket(false)
		, depth(2)
	{
		kf_fmt = snf::net::ssl::data_fmt::pem;
		cf_fmt = snf::net::ssl::data_fmt::pem;
	}
	
};

struct input
{
	std::string cmd;
	std::string arg;
};

static std::exception_ptr g_except_ptr = nullptr;
volatile std::sig_atomic_t g_terminated = 0;

void
signal_handler(int signo)
{
	g_terminated = 1;
}

static int
usage(const std::string &prog)
{
	std::cerr << prog << " (connect|bind) [-host <host>] [-port <port>] -ssl" << std::endl;
	std::cerr << "    -key <key-file> -keypass <key-password> -keyfmt <der|pem>" << std::endl;
	std::cerr << "    -cert <certificate> -certfmt <der|pem>" << std::endl;
	std::cerr << "    -altcert <secondary-certificate-for-SNI>" << std::endl;
	std::cerr << "    -ca <ca-bundle-file> -crl <crl-file>" << std::endl;
	std::cerr << "    -verify -reqcert -depth <certificate-chain-depth>" << std::endl;
	std::cerr << "    -chkhost -sni <sni-host>" << std::endl;
	std::cerr << "    [-save|-restore] -session <session-file> [-ticket]" << std::endl;
	return 1;
}

static bool
get_input(input &in)
{
	in.cmd.clear();
	in.arg.clear();

	std::cin >> in.cmd;

	std::string buf;
	if (std::getline(std::cin, buf)) {
		in.arg = std::move(snf::trim(buf));
		return true;
	}

	return false;
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
		} else if (snf::streq("-altcert", argv[i])) {
			if (argv[i + 1]) {
				args.altcertfile = argv[++i];
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
		} else if (snf::streq("-session", argv[i])) {
			if (argv[i + 1]) {
				args.session_file = argv[++i];
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-verify", argv[i])) {
			args.verify_peer = true;
		} else if (snf::streq("-reqcert", argv[i])) {
			args.require_cert = true;
		} else if (snf::streq("-chkhost", argv[i])) {
			args.check_hosts = true;
		} else if (snf::streq("-save", argv[i])) {
			args.save_session = true;
		} else if (snf::streq("-restore", argv[i])) {
			args.restore_session = true;
		} else if (snf::streq("-ticket", argv[i])) {
			args.use_ticket = true;
		} else if (snf::streq("-depth", argv[i])) {
			if (argv[i + 1]) {
				args.depth = atoi(argv[++i]);
			} else {
				std::cerr << "argument to " << argv[i] << " missing" << std::endl;
				return usage(argv[0]);
			}
		} else if (snf::streq("-sni", argv[i])) {
			if (argv[i + 1]) {
				args.snihost = argv[++i];
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
		if (args.save_session || args.restore_session) {
			if (args.session_file.empty()) {
				std::cerr << "-session is required if -save or -restore specified"
					<< std::endl;
				return usage(argv[0]);
			}
		}
	}

	return 0;
}

static void
prepare_context(const arguments &args, bool usealtcert, snf::net::ssl::context *ctx)
{
	if (!args.keyfile.empty()) {
		const char *passwd = args.kf_password.empty() ? nullptr : args.kf_password.c_str();
		snf::net::ssl::pkey key { args.kf_fmt, args.keyfile, passwd };
		ctx->use_private_key(key);
	}

	if (usealtcert) {
		if (!args.altcertfile.empty()) {
			snf::net::ssl::x509_certificate cert { args.cf_fmt, args.altcertfile };
			ctx->use_certificate(cert);
		}
	} else {
		if (!args.certfile.empty()) {
			snf::net::ssl::x509_certificate cert { args.cf_fmt, args.certfile };
			ctx->use_certificate(cert);
		}
	}

	if (!args.keyfile.empty() && !args.certfile.empty())
		ctx->check_private_key();

	snf::net::ssl::truststore store { args.cafile };
	if (!args.crlfile.empty()) {
		snf::net::ssl::x509_crl crl { args.crlfile };
		store.add_crl(crl);
	}

	ctx->use_truststore(store);	

	ctx->set_ciphers();

	if (args.verify_peer)
		ctx->verify_peer(args.require_cert);

	ctx->limit_certificate_chain_depth(args.depth);

	if (snf::net::connection_mode::server == args.cnxn_mode)
		ctx->set_session_context("server:16781");

	if (args.use_ticket) {
		ctx->session_ticket(args.cnxn_mode, true);
	}
}

static int
client(const arguments &args, snf::net::ssl::context *ctx)
{
	int retval = E_ok;
	snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

	sock.keepalive(true);
	sock.tcpnodelay(true);
	sock.blocking(false);

	sock.connect(AF_INET, args.host, args.port);

	snf::net::nio *io = nullptr;
	snf::net::ssl::connection *cnxn = nullptr;

	if (args.use_ssl) {
		cnxn = DBG_NEW snf::net::ssl::connection { snf::net::connection_mode::client, *ctx };
		if (args.check_hosts)
			cnxn->check_hosts({ args.host });
		if (!args.snihost.empty())
			cnxn->set_sni(args.snihost);

		if (args.restore_session) {
			snf::net::ssl::session sess { args.session_file };
			cnxn->set_session(sess);
		}

		std::chrono::time_point<std::chrono::high_resolution_clock> begin_time;
		std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

		begin_time = std::chrono::high_resolution_clock::now();
		cnxn->handshake(sock);
		end_time = std::chrono::high_resolution_clock::now();

		std::cout << "SSL handshake took "
			<< std::chrono::duration_cast<std::chrono::microseconds>
				(end_time - begin_time).count()
			<< " microseconds" << std::endl;

		if (cnxn->is_session_reused())
			std::cout << "SSL session is reused" << std::endl;
		else
			std::cout << "SSL session is not reused" << std::endl;

		std::unique_ptr<snf::net::ssl::x509_certificate> cert
			(cnxn->get_peer_certificate());
		if (cert)
			std::cout << "Got certificate for " << cert->subject() << std::endl;

		std::string errstr;
		if (cnxn->is_verification_successful(errstr)) {
			std::cerr << "Handshake successfull" << std::endl;

			snf::net::ssl::session sess = std::move(cnxn->get_session());
			std::cout << "session id = " << sess.get_id() << std::endl;
			if (sess.has_ticket())
				std::cout << "session has ticket" << std::endl;

			if (args.save_session)
				sess.to_file(args.session_file);
		} else {
			std::cerr << "Handshake failure: " << errstr << std::endl;
		}

		io = cnxn;
	} else {
		io = &sock;
	}

	std::string buf;
	input in;
	while (!g_terminated && get_input(in)) {
		if (in.cmd == "exit") {
			break;
		} else if (in.cmd == "echo") {
			retval = io->write_string(in.arg);
			if (E_ok == retval) {
				retval = io->read_string(buf);
				if (E_ok == retval) {
					std::cout << buf << std::endl;
					buf.clear();
				}
			}
		}

		if (E_ok != retval)
			break;
	}

	if (cnxn) {
		cnxn->shutdown();
		delete cnxn;
	}

	sock.close();

	return 0;
}

static void
worker_thread(const arguments &args, snf::net::ssl::context &ctx, snf::net::socket &&s)
{
	int retval = E_ok;
	std::string buf;

	snf::net::socket sock = std::move(s);
	snf::net::nio *io = nullptr;
	snf::net::ssl::connection *cnxn = nullptr;

	try {
		if (args.use_ssl) {
			cnxn = DBG_NEW snf::net::ssl::connection
				{ snf::net::connection_mode::server, ctx };

			if (!args.altcertfile.empty()) {
				snf::net::ssl::context ctx2;
				prepare_context(args, true, &ctx2);
				cnxn->add_context(ctx2);
				cnxn->enable_sni();
			}

			cnxn->handshake(sock);

			std::string errstr;
			if (cnxn->is_verification_successful(errstr))
				std::cerr << "Handshake successfull" << std::endl;
			else
				std::cerr << "Handshake failure: " << errstr << std::endl;

			io = cnxn;
		} else {
			io = &sock;
		}

		int oserr;

		do {
			if (g_terminated)
				break;

			oserr = 0;
			retval = io->read_string(buf, snf::net::POLL_WAIT_FOREVER, &oserr);
			if (E_ok == retval)
				retval = io->write_string(buf);
			buf.clear();
		} while (E_ok == retval);

		std::cerr << "worker thread exited with " << retval << ", " << oserr << std::endl;

		if (cnxn) {
			cnxn->shutdown();
			delete cnxn;
		}

		sock.close();
	} catch (...) {
		g_except_ptr = std::current_exception();
	}
}

static int
server(const arguments &args, snf::net::ssl::context *ctx)
{
	int retval, oserr;
	snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

	sock.keepalive(true);
	sock.tcpnodelay(true);
	sock.reuseaddr(true);
	sock.blocking(false);

	sock.bind(AF_INET, args.port);
	sock.listen(20);

	while (!g_terminated) {
		pollfd fdelem = { sock, POLLIN | POLLOUT, 0 };
		std::vector<pollfd> fdvec { 1, fdelem };

		retval = snf::net::poll(fdvec, 1000, &oserr);
		if (0 == retval) {
			if (g_except_ptr)
				std::rethrow_exception(g_except_ptr);
		} else if (SOCKET_ERROR == retval) {
			throw std::system_error(
				oserr,
				std::system_category(),
				"failed to poll socket");
		} else /* if (retval > 0) */ {
			snf::net::socket nsock = std::move(sock.accept());
			nsock.blocking(false);
			std::thread wt(
				worker_thread,
				std::ref(args),
				std::ref(*ctx),
				std::move(nsock));
			wt.detach();
		}
	}

	sock.close();

	return 0;
}

int
main(int argc, const char **argv)
{
try {
	arguments args;

	int retval = parse_arguments(args, argc, argv);
	if (retval == 0) {
		std::signal(SIGINT, signal_handler);
		std::signal(SIGTERM, signal_handler);
#if !defined(_WIN32)
		if (args.use_ssl)
			std::signal(SIGPIPE, SIG_IGN);
#endif

		snf::net::initialize(args.use_ssl);

		snf::net::ssl::context *ctx = nullptr;
		if (args.use_ssl) {
			ctx = DBG_NEW snf::net::ssl::context {};
			prepare_context(args, false, ctx);
		}

		if (args.cnxn_mode == snf::net::connection_mode::client)
			retval = client(args, ctx);
		else
			retval = server(args, ctx);

		if (ctx)
			delete ctx;
	}

	return retval;
} catch (const std::invalid_argument &ex) {
	std::cerr << ex.what() << std::endl;
} catch (const snf::net::ssl::exception &ex) {
	std::cerr << ex.what() << std::endl;
	for (auto I = ex.begin(); I != ex.end(); ++I)
		std::cerr << *I << std::endl;
} catch (const std::system_error &ex) {
	std::cerr << "Error Code: " << ex.code() << std::endl;
	std::cerr << "Error: " << ex.what() << std::endl;
} catch (const std::runtime_error &ex) {
	std::cerr << ex.what() << std::endl;
} catch (...) {
}
}

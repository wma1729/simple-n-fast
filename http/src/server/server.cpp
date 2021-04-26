#include "server.h"
#include "error.h"
#include "handler.h"
#include "logmgr.h"

namespace snf {
namespace http {

int
server::setup_context()
{
	try {
		DEBUG_STRM("server")
			<< "setting SSL context"
			<< snf::log::record::endl;

		if (m_config->keyfile().empty()) {
			ERROR_STRM("server")
				<< "private key file not specified"
				<< snf::log::record::endl;
			return E_not_found;
		} else {
			const char *password = m_config->keyfile_password().empty()
						? nullptr : m_config->keyfile_password().c_str();
			snf::ssl::pkey key(m_config->keyfile_format(), m_config->keyfile(), password);
			m_ctx.use_private_key(key);
		}

		if (m_config->certfile().empty()) {
			ERROR_STRM("server")
				<< "certificate file not specified"
				<< snf::log::record::endl;
			return E_not_found;
		} else {
			snf::ssl::x509_certificate cert(m_config->certfile_format(), m_config->certfile());
			m_ctx.use_certificate(cert);
		}

		m_ctx.check_private_key();

		if (m_config->cafile().empty()) {
			ERROR_STRM("server")
				<< "CA file not specified"
				<< snf::log::record::endl;
			return E_not_found;
		} else {
			snf::ssl::truststore store(m_config->cafile());
			m_ctx.use_truststore(store);
		}

		m_ctx.set_ciphers();
		m_ctx.verify_peer(false);
		m_ctx.limit_certificate_chain_depth(m_config->certificate_chain_depth());

		DEBUG_STRM("server")
			<< "SSL context is set successfully"
			<< snf::log::record::endl;

		return E_ok;
	} catch (snf::ssl::exception &ex) {
		ERROR_STRM("server")
			<< ex.what()
			<< snf::log::record::endl;
		for (auto I = ex.begin(); I != ex.end(); ++I)
			ERROR_STRM("server")
				<< *I
				<< snf::log::record::endl;
		return E_ssl_error;
	}
}

snf::net::socket *
server::setup_socket(in_port_t port)
{
	try {
		std::unique_ptr<snf::net::socket> s(
			DBG_NEW snf::net::socket(AF_INET, snf::net::socket_type::tcp));
		s->keepalive(true);
		s->tcpnodelay(true);
		s->reuseaddr(true);
		s->blocking(false);
		s->bind(AF_INET, port);

		DEBUG_STRM("server")
			<< "socket " << *s
			<< " bound to port "
			<< port
			<< snf::log::record::endl;
		s->listen(20);

		return s.release();
	} catch (std::system_error &ex) {
		ERROR_STRM("server", ex.code().value())
			<< ex.what()
			<< snf::log::record::endl;
		return nullptr;
	}
}

int
server::start(const server_config *cfg)
{
	if (m_started)
		return E_ok;

	if (m_stopped) {
		ERROR_STRM("server")
			<< "server cannot be restarted once it is stopped"
			<< snf::log::record::endl;
		return E_invalid_state;
	}

	if (cfg == nullptr) {
		ERROR_STRM("server")
			<< "server config is nil"
			<< snf::log::record::endl;
		return E_invalid_arg;
	}

	m_config = cfg;

	int r = setup_context();
	if (r != E_ok)
		return r;

	m_thrdpool.reset(DBG_NEW snf::thread_pool(m_config->worker_thread_count()));

	std::unique_ptr<snf::net::socket> sock(setup_socket(m_config->http_port()));
	if (!sock) {
		ERROR_STRM("server")
			<< "failed to get socket bound to http port "
			<< m_config->http_port()
			<< snf::log::record::endl;
		return E_bind_failed;
	} else {
		INFO_STRM("server")
			<< "created http socket "
			<< *sock
			<< sock->dump_options()
			<< snf::log::record::endl;

		sock_t s = *sock;
		m_reactor.add_handler(
				s, 
				snf::net::event::read,
				DBG_NEW accept_handler(sock.release(), snf::net::event::read));
	}

	std::unique_ptr<snf::net::socket> sec_sock(setup_socket(m_config->https_port()));
	if (!sec_sock) {
		ERROR_STRM("server")
			<< "failed to get socket bound to https port "
			<< m_config->https_port()
			<< snf::log::record::endl;
		return E_bind_failed;
	} else {
		INFO_STRM("server")
			<< "created https socket "
			<< *sec_sock
			<< sec_sock->dump_options()
			<< snf::log::record::endl;

		sock_t s = *sec_sock;
		m_reactor.add_handler(
				s,
				snf::net::event::read,
				DBG_NEW accept_handler(sec_sock.release(), snf::net::event::read, true));
	}

	m_started = true;

	return E_ok;
}

int
server::stop()
{
	m_reactor.stop();
	m_thrdpool->stop();
	m_started = false;
	m_stopped = true;
	return 0;
}

void
server::register_path(const std::string &path, request_handler_t h)
{
	router::instance().add(path, h);
}

} // namespace http
} // namespace snf

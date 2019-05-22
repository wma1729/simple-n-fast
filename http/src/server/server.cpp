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
		if (m_config->keyfile().empty()) {
			ERROR_STRM("server")
				<< "private key file not specified"
				<< snf::log::record::endl;
			return E_not_found;
		} else {
			const char *password = m_config->keyfile_password().empty()
						? nullptr : m_config->keyfile_password().c_str();
			snf::net::ssl::pkey key(m_config->keyfile_format(), m_config->keyfile(), password);
			m_ctx.use_private_key(key);
		}

		if (m_config->certfile().empty()) {
			ERROR_STRM("server")
				<< "certificate file not specified"
				<< snf::log::record::endl;
			return E_not_found;
		} else {
			snf::net::ssl::x509_certificate cert(m_config->certfile_format(), m_config->certfile());
			m_ctx.use_certificate(cert);
		}

		m_ctx.check_private_key();

		if (m_config->cafile().empty()) {
			throw std::runtime_error("CA file not specified");
		} else {
			snf::net::ssl::truststore store(m_config->cafile());
			m_ctx.use_truststore(store);
		}

		m_ctx.set_ciphers();
		m_ctx.verify_peer(false);
		m_ctx.limit_certificate_chain_depth(m_config->certificate_chain_depth());
	} catch (snf::net::ssl::exception ex) {
		ERROR_STRM("server")
			<< ex.what()
			<< snf::log::record::endl;
		for (auto I = ex.begin(); I != ex.end(); ++I)
			ERROR_STRM("server")
				<< *I
				<< snf::log::record::endl;
		return E_ssl_error;
	}

	return E_ok;
}

snf::net::socket *
server::setup_socket(in_port_t port)
{
	snf::net::socket *s = DBG_NEW snf::net::socket(AF_INET, snf::net::socket_type::tcp);
	s->keepalive(true);
	s->tcpnodelay(true);
	s->reuseaddr(true);
	s->blocking(false);

	try {
		s->bind(AF_INET, port);
		s->listen(20);
	} catch (std::system_error &ex) {
		ERROR_STRM("server", ex.code().value())
			<< ex.what()
			<< snf::log::record::endl;
		delete s;
		return nullptr;
	}

	return s;
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

	m_http_sock.reset(setup_socket(m_config->http_port()));
	if (!m_http_sock) {
		ERROR_STRM("server")
			<< "failed to get socket bound to http port "
			<< m_config->http_port()
			<< snf::log::record::endl;
		return E_bind_failed;
	} else {
		INFO_STRM("server")
			<< "created http socket "
			<< *m_http_sock
			<< ", " << m_http_sock->dump_options()
			<< snf::log::record::endl;
	}

	m_https_sock.reset(setup_socket(m_config->https_port()));
	if (!m_https_sock) {
		ERROR_STRM("server")
			<< "failed to get socket bound to https port "
			<< m_config->https_port()
			<< snf::log::record::endl;
		return E_bind_failed;
	} else {
		INFO_STRM("server")
			<< "created https socket "
			<< *m_https_sock
			<< ", " << m_https_sock->dump_options()
			<< snf::log::record::endl;
	}

	m_thrdpool.reset(DBG_NEW snf::thread_pool(m_config->worker_thread_count()));

	m_reactor.add_handler(
			*m_http_sock, 
			snf::net::event::read,
			DBG_NEW accept_handler(*m_http_sock, snf::net::event::read));

	m_reactor.add_handler(
			*m_https_sock,
			snf::net::event::read,
			DBG_NEW accept_handler(*m_https_sock, snf::net::event::read, true));

	m_started = true;

	return E_ok;
}

int
server::stop()
{
	m_reactor.stop();
	m_thrdpool->stop();
	m_http_sock->close();
	m_https_sock->close();
	m_started = false;
	m_stopped = true;
	return 0;
}

} // namespace http
} // namespace snf

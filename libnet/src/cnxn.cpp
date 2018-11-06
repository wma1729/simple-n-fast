#include "cnxn.h"
#include "error.h"
#include <algorithm>

extern "C" int
sni_cb(SSL *ssl, int *, void *arg)
{
	snf::net::ssl::connection *ssock =
		reinterpret_cast<snf::net::ssl::connection *>(arg);
	if (ssock == nullptr)
		return 0;
	try {
		std::string servername = std::move(ssock->get_sni());
		ssock->switch_context(servername);
	} catch (snf::net::ssl::ssl_exception &) {
		return 0;
	}

	return 1;
}

namespace snf {
namespace net {
namespace ssl {

int
connection::handle_ssl_error(sock_t sock, int to, int error, const std::string &errstr, int *oserr)
{
	int retval = E_ok;
	std::ostringstream oss;

	if (error > 0)
		return retval;

	oss << errstr << "; error = " << error;

	pollfd fdelem = { sock, 0, 0 };

	int ssl_error = ssl_library::instance().ssl_get_error()(m_ssl, error);

	oss << ", ssl_error = " << ssl_error;

	switch (ssl_error) {
		case SSL_ERROR_NONE:
		case SSL_ERROR_ZERO_RETURN:
			break;

		case SSL_ERROR_WANT_READ:
			fdelem.events |= POLLIN;
			retval = E_try_again;
			break;

		case SSL_ERROR_WANT_WRITE:
			fdelem.events |= POLLOUT;
			retval = E_try_again;
			break;

		case SSL_ERROR_SYSCALL:
			if (ssl_library::instance().err_peek()() != 0) {
				throw ssl_exception(oss.str());
			} else if (error == 0) {
				if (oserr) *oserr = EOF;
				retval = E_syscall_failed;
			} else if (error == -1) {
				if (oserr) *oserr = snf::net::error();
				retval = E_syscall_failed;
			}
			break;

		default:
			throw ssl_exception(oss.str());
			break;
	}

	if ((INVALID_SOCKET != sock) && (E_try_again == retval)) {
		std::vector<pollfd> fdvec { 1, fdelem };

		retval = snf::net::poll(fdvec, to, oserr);
		if (0 == retval) {
			if (oserr) *oserr = ETIMEDOUT;
			retval = E_timed_out;
		} else if (SOCKET_ERROR == retval) {
			retval = E_syscall_failed;
		} else /* if (retval > 0) */ {
			retval = E_try_again;
		}
	}

	return retval;
}

connection::connection(connection_mode m, context &ctx)
	: m_mode(m)
{
	m_ssl = ssl_library::instance().ssl_new()(ctx);
	if (m_ssl == nullptr)
		throw ssl_exception("failed to create SSL object");

	if (connection_mode::client == m_mode)
		ssl_library::instance().ssl_set_connect_state()(m_ssl);
	else
		ssl_library::instance().ssl_set_accept_state()(m_ssl);

	ctxinfo ci;
	ci.cur = true;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

connection::connection(const connection &d)
{
	m_mode = d.m_mode;
	m_contexts = d.m_contexts;

	m_ssl = ssl_library::instance().ssl_dup()(d.m_ssl);
	if (m_ssl == nullptr)
		throw ssl_exception("failed to duplicate SSL object");
}

connection::connection(connection &&d)
{
	m_mode = d.m_mode;
	m_contexts = std::move(d.m_contexts);
	m_ssl = d.m_ssl;
	d.m_ssl = nullptr;
}

connection::~connection()
{
	shutdown();
	if (m_ssl) {
		ssl_library::instance().ssl_free()(m_ssl);
		m_ssl = nullptr;
	}
	m_contexts.clear();
}

const connection &
connection::operator=(const connection &d)
{
	if (this != &d) {
		m_mode = d.m_mode;
		m_contexts = d.m_contexts;

		m_ssl = ssl_library::instance().ssl_dup()(d.m_ssl);
		if (m_ssl == nullptr)
			throw ssl_exception("failed to duplicate SSL object");
	}
	return *this;
}

connection &
connection::operator=(connection &&d)
{
	if (this != &d) {
		m_mode = d.m_mode;
		m_contexts = std::move(d.m_contexts);
		m_ssl = d.m_ssl;
		d.m_ssl = nullptr;
	}
	return *this;
}

void
connection::add_context(context &ctx)
{
	if (connection_mode::client == m_mode)
		throw ssl_exception("only 1 SSL context can be added in client mode");

	std::lock_guard<std::mutex> guard(m_lock);
	ctxinfo ci;
	ci.cur = false;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

void
connection::switch_context(const std::string &servername)
{
	std::lock_guard<std::mutex> guard(m_lock);

	std::vector<ctxinfo>::iterator it;
	for (it = m_contexts.begin(); it != m_contexts.end(); ++it) {
		x509_certificate cert = std::move(it->ctx.get_certificate());
		if (cert.matches(servername)) {
			if (it->cur) {
				// already current
				return;
			} else {
				// make this current
				break;
			}
		}
	}

	if (it == m_contexts.end()) {
		std::ostringstream oss;
		oss << "SSL context for server name " << servername << " not found";
		throw ssl_exception(oss.str());
	} else {
		std::for_each(m_contexts.begin(), m_contexts.end(),
			[](ctxinfo &ci) { ci.cur = false; });
		ssl_library::instance().ssl_set_ssl_ctx()(m_ssl, it->ctx);
		it->cur = true;
	}
}

void
connection::check_hosts(const std::vector<std::string> &hostnames)
{
	int retval;

	X509_VERIFY_PARAM *param = ssl_library::instance().ssl_get0_param()(m_ssl);
	if (param == nullptr)
		throw ssl_exception("failed to get X509 verify parameters");

	std::vector<std::string>::const_iterator it = hostnames.begin();
	if (it != hostnames.end()) {
		retval = ssl_library::instance().x509_verify_param_set1_host()
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to set " << *it << " to X509 verify parameters";
			throw ssl_exception(oss.str());
		}
		it++;
	}

	while (it != hostnames.end()) {
		retval = ssl_library::instance().x509_verify_param_add1_host()
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to add " << *it << " to X509 verify parameters";
			throw ssl_exception(oss.str());
		}
		it++;
	}

	unsigned int flags =
		X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT |
		X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS;
			
	ssl_library::instance().x509_verify_param_set_hostflags()
		(param, flags);
}

void
connection::check_inaddr(const internet_address &ia)
{
	X509_VERIFY_PARAM *param = ssl_library::instance().ssl_get0_param()(m_ssl);
	if (param == nullptr)
		throw ssl_exception("failed to get X509 verify parameters");

	int retval = ssl_library::instance().x509_verify_param_set1_ip_asc()
		(param, ia.str(false).c_str());
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set " << ia << " to X509 verify parameters";
		throw ssl_exception(oss.str());
	}
}

void
connection::set_sni(const std::string &servername)
{
	if (connection_mode::server == m_mode)
		throw ssl_exception("setting server name for SNI only possible in client mode");

	int retval = ssl_library::instance().ssl_ctrl()
			(m_ssl,
			SSL_CTRL_SET_TLSEXT_HOSTNAME,
			TLSEXT_NAMETYPE_host_name,
			const_cast<char *>(servername.c_str()));
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set SNI for " << servername << " in SSL";
		throw ssl_exception(oss.str());
	}
}

std::string
connection::get_sni()
{
	if (connection_mode::client == m_mode)
		throw ssl_exception("getting server name for SNI only possible in server mode");

	const char *name = ssl_library::instance().ssl_get_servername()
				(m_ssl, TLSEXT_NAMETYPE_host_name);
	if ((name == nullptr) || (*name == '\0'))
		throw ssl_exception("failed to get SNI from SSL");

	std::string servername(name);
	return servername;
}

void
connection::enable_sni()
{
	if (connection_mode::client == m_mode)
		throw ssl_exception("handling SNI requests can only be enabled in server mode");

	std::lock_guard<std::mutex> guard(m_lock);

	std::vector<ctxinfo>::iterator it;
	for (it = m_contexts.begin(); it != m_contexts.end(); ++it) {
		if (ssl_library::instance().ssl_ctx_cb_ctrl()
			(it->ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_CB,
			reinterpret_cast<void (*)(void)>(sni_cb)) != 1)
				throw ssl_exception("failed to set SNI callback function");
		if (ssl_library::instance().ssl_ctx_ctrl()
			(it->ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_ARG, 0,
			reinterpret_cast<void *>(this)) != 1)
				throw ssl_exception("failed to set SNI callback argument");
	}
}

void
connection::handshake(const socket &s, int to)
{
	sock_t sock = s;
	int retval = ssl_library::instance().ssl_set_fd()(m_ssl, static_cast<int>(sock));
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set socket "
			<< static_cast<int64_t>(sock)
			<< " for TLS communication";
		throw ssl_exception(oss.str());
	}

	int oserr;

	std::ostringstream oss;
	oss << "failed to complete the handshake during ";
	if (connection_mode::client == m_mode)
		oss << "client connect";
	else
		oss << "server accept";

	do {
		if (connection_mode::client == m_mode)
			retval = ssl_library::instance().ssl_connect()(m_ssl);
		else /* if (connection_mode::server == m_mode) */
			retval = ssl_library::instance().ssl_accept()(m_ssl);

		oserr = 0;
		retval = handle_ssl_error(sock, to, retval, oss.str(), &oserr); 
		if ((E_ok != retval) && (E_try_again != retval)) {
			throw std::system_error(
				oserr,
				std::system_category(),
				oss.str());
		}
	} while (retval != E_ok);
}

int
connection::read(void *buf, int to_read, int *bread, int to, int *oserr)
{
	int     retval = E_ok;
	int     n = 0, nbytes = 0;
	char    *cbuf = static_cast<char *>(buf);
	sock_t  sock;

	if (buf == nullptr)
		return E_invalid_arg;

	if (to_read <= 0)
		return E_invalid_arg;

	if (bread == nullptr)
		return E_invalid_arg;

	sock = ssl_library::instance().ssl_get_fd()(m_ssl);
	if (sock < 1)
		throw ssl_exception("failed to get internal socket");

	do {
		n = ssl_library::instance().ssl_read()(m_ssl, cbuf, to_read);
		if (n <= 0) {
			if (oserr) *oserr = 0;
			retval = handle_ssl_error(sock, to, n,
					"failed to complete the handshake during read",
					oserr);
			if ((E_ok != retval) && (E_try_again != retval)) {
				break;
			}
		} else {
			cbuf += n;
			to_read -= n;
			nbytes += n;
		}
	} while (to_read > 0);

	*bread = nbytes;

	return retval;
}

int
connection::write(const void *buf, int to_write, int *bwritten, int to, int *oserr)
{
	int         retval = E_ok;
	int         n = 0, nbytes = 0;
	const char  *cbuf = static_cast<const char *>(buf);
	sock_t      sock;

	if (buf == nullptr)
		return E_invalid_arg;

	if (to_write <= 0)
		return E_invalid_arg;

	if (bwritten == nullptr)
		return E_invalid_arg;

	sock = ssl_library::instance().ssl_get_fd()(m_ssl);
	if (sock < 1)
		throw ssl_exception("failed to get internal socket");

	do {
		n = ssl_library::instance().ssl_write()(m_ssl, cbuf, to_write);
		if (n <= 0) {
			if (oserr) *oserr = 0;
			retval = handle_ssl_error(sock, to, n,
					"failed to complete the handshake during write",
					oserr);
			if ((E_ok != retval) && (E_try_again != retval)) {
				break;
			}
		} else {
			cbuf += n;
			to_write -= n;
			nbytes += n;
		}
	} while (to_write > 0);

	*bwritten = nbytes;

	return retval;
}

void
connection::shutdown()
{
	int retval = ssl_library::instance().ssl_shutdown()(m_ssl);
	if (retval == 0) {
		retval = ssl_library::instance().ssl_shutdown()(m_ssl);
	}

	if (retval != 1) {
		int oserr = 0;
		retval = handle_ssl_error(INVALID_SOCKET, POLL_WAIT_NONE, retval,
				"failed to shutdown TLS connection", &oserr);
		if (oserr && retval != E_ok) {
			throw std::system_error(
				oserr,
				std::system_category(),
				"failed to shutdown TLS connection");
		}
	}
}

void
connection::reset()
{
	if (ssl_library::instance().ssl_clear()(m_ssl) != 1)
		throw ssl_exception("failed to prepare TLS object for new connection");
}

void
connection::renegotiate(int to)
{
	int retval;
	int oserr;

	if (ssl_library::instance().ssl_renegotiate()(m_ssl) != 1)
		throw ssl_exception("failed to schedule TLS renegotiation");

	sock_t sock = ssl_library::instance().ssl_get_fd()(m_ssl);
	if (sock < 1)
		throw ssl_exception("failed to get internal socket");

	do {
		retval = ssl_library::instance().ssl_do_handshake()(m_ssl);
		if (retval <= 0) {
			oserr = 0;
			retval = handle_ssl_error(sock, to, retval,
					"failed to complete the handshake during renegotiation",
					&oserr);
			if ((E_ok != retval) && (E_try_again != retval)) {
				throw std::system_error(
					oserr,
					std::system_category(),
					"failed to complete the handshake during renegotiation");
			}
		}
	} while (ssl_library::instance().ssl_renegotiate_pending()(m_ssl) == 1);
}

x509_certificate *
connection::get_peer_certificate()
{
	X509 *c = ssl_library::instance().ssl_get_peer_cert()(m_ssl);
	if (c) {
		x509_certificate *crt = DBG_NEW x509_certificate(c);
		ssl_library::instance().x509_free()(c);
		return crt;
	}
	return nullptr;
}

bool
connection::is_verification_successful(std::string &errstr)
{
	errstr.clear();
	long result = ssl_library::instance().ssl_get_verify_result()(m_ssl);
	if (result == X509_V_OK)
		return true;

	errstr.assign(ssl_library::instance().x509_verify_cert_error_string()(result));
	return false;
}

} // namespace ssl
} // namespace net
} // namespace snf

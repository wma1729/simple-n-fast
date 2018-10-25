#include "drvr.h"
#include "error.h"
#include <algorithm>

extern "C" int
sni_cb(SSL *ssl, int *, void *arg)
{
	snf::net::ssl::driver *ssock =
		reinterpret_cast<snf::net::ssl::driver *>(arg);
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

bool
driver::decode_ssl_error(int status, ssl_error &err)
{
	err.want_rd = false;
	err.want_wr = false;
	err.chk_err_stk = false;
	err.syserr = 0;

	if (status > 0)
		return true;

	int ssl_err = ssl_library::instance().ssl_get_error()(m_ssl, status);
	bool success = true;

	switch (ssl_err) {
		case SSL_ERROR_NONE:
		case SSL_ERROR_ZERO_RETURN:
			break;

		case SSL_ERROR_WANT_READ:
			err.want_rd = true;
			break;

		case SSL_ERROR_WANT_WRITE:
			err.want_wr = true;
			break;

		case SSL_ERROR_SYSCALL:
			if (ssl_library::instance().err_peek()() != 0) {
				err.chk_err_stk = true;
			} else if (status == 0) {
				err.syserr = EOF;
			} else if (status == -1) {
				err.syserr = snf::net::error();
			}
			success = false;
			break;

		default:
			err.chk_err_stk = true;
			success = false;
			break;
	}

	return success;
}

driver::driver(driver_mode m, context &ctx)
	: m_mode(m)
{
	m_ssl = ssl_library::instance().ssl_new()(ctx);
	if (m_ssl == nullptr)
		throw ssl_exception("failed to create SSL object");

	if (driver_mode::client == m_mode)
		ssl_library::instance().ssl_set_connect_state()(m_ssl);
	else
		ssl_library::instance().ssl_set_accept_state()(m_ssl);

	ctxinfo ci;
	ci.cur = true;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

driver::driver(const driver &d)
{
	m_mode = d.m_mode;
	m_contexts = d.m_contexts;

	m_ssl = ssl_library::instance().ssl_dup()(d.m_ssl);
	if (m_ssl == nullptr)
		throw ssl_exception("failed to duplicate SSL object");
}

driver::driver(driver &&d)
{
	m_mode = d.m_mode;
	m_contexts = std::move(d.m_contexts);
	m_ssl = d.m_ssl;
	d.m_ssl = nullptr;
}

driver::~driver()
{
	shutdown();
	if (m_ssl) {
		ssl_library::instance().ssl_free()(m_ssl);
		m_ssl = nullptr;
	}
	m_contexts.clear();
}

const driver &
driver::operator=(const driver &d)
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
driver &
driver::operator=(driver &&d)
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
driver::add_context(context &ctx)
{
	if (driver_mode::client == m_mode)
		throw ssl_exception("only 1 SSL context can be added in client mode");

	std::lock_guard<std::mutex> guard(m_lock);
	ctxinfo ci;
	ci.cur = false;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

void
driver::switch_context(const std::string &servername)
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
driver::check_hosts(const std::vector<std::string> &hostnames)
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
driver::check_inaddr(const internet_address &ia)
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
driver::set_sni(const std::string &servername)
{
	if (driver_mode::server == m_mode)
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
driver::get_sni()
{
	if (driver_mode::client == m_mode)
		throw ssl_exception("getting server name for SNI only possible in server mode");

	const char *name = ssl_library::instance().ssl_get_servername()
				(m_ssl, TLSEXT_NAMETYPE_host_name);
	if ((name == nullptr) || (*name == '\0'))
		throw ssl_exception("failed to get SNI from SSL");

	std::string servername(name);
	return servername;
}

void
driver::enable_sni()
{
	if (driver_mode::client == m_mode)
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
driver::handshake(const socket &s, int to)
{
	sock_t sock = s.handle();
	int retval = ssl_library::instance().ssl_set_fd()(m_ssl, static_cast<int>(sock));
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set socket "
			<< static_cast<int64_t>(sock)
			<< " for TLS communication";
		throw ssl_exception(oss.str());
	}

	ssl_error err;

	while (true) {
		if (driver_mode::client == m_mode)
			retval = ssl_library::instance().ssl_connect()(m_ssl);
		else /* if (driver_mode::server == m_mode) */
			retval = ssl_library::instance().ssl_accept()(m_ssl);

		if (decode_ssl_error(retval, err)) {
			if (err.want_rd || err.want_wr) {
				pollfd fdelem = { sock, 0, 0 };
				if (err.want_rd) fdelem.events |= POLLIN;
				if (err.want_wr) fdelem.events |= POLLOUT;
				std::vector<pollfd> fdvec { 1, fdelem };

				retval = snf::net::poll(fdvec, to, &err.syserr);
				if (0 == retval) {
					err.syserr = ETIMEDOUT;
					retval = SOCKET_ERROR;
				}

				if (SOCKET_ERROR == retval) {
					throw std::system_error(
						err.syserr,
						std::system_category(),
						"failed to complete the handshake");
				}

				// try again
			} else {
				// handshake complete
				break;
			}
		} else {
			if (err.syserr != 0) {
				throw std::system_error(
					err.syserr,
					std::system_category(),
					"failed to complete the handshake");
			} else {
				throw ssl_exception("failed to complete the handshake");
			}
		}
	}
}

int
driver::read(void *buf, int to_read, int *bread, int to, int *oserr)
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

	ssl_error err;

	do {
		n = ssl_library::instance().ssl_read()(m_ssl, cbuf, to_read);
		if (n <= 0) {
			if (decode_ssl_error(retval, err)) {
				if (err.want_rd || err.want_wr) {
					pollfd fdelem = { sock, 0, 0 };
					if (err.want_rd) fdelem.events |= POLLIN;
					if (err.want_wr) fdelem.events |= POLLOUT;
					std::vector<pollfd> fdvec { 1, fdelem };

					retval = snf::net::poll(fdvec, to, &err.syserr);
					if (0 == retval) {
						err.syserr = ETIMEDOUT;
					}

					if (SOCKET_ERROR == retval) {
						retval = map_system_error(
								err.syserr, E_write_failed);
						break;
					}

					// try again
				}
			} else {
				if (err.syserr != 0) {
					retval = map_system_error(
							err.syserr, E_write_failed);
					break;
				} else {
					throw ssl_exception("failed to complete the handshake");
				}
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
driver::write(const void *buf, int to_write, int *bwritten, int to, int *oserr)
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

	ssl_error err;

	do {
		n = ssl_library::instance().ssl_write()(m_ssl, cbuf, to_write);
		if (n <= 0) {
			if (decode_ssl_error(retval, err)) {
				if (err.want_rd || err.want_wr) {
					pollfd fdelem = { sock, 0, 0 };
					if (err.want_rd) fdelem.events |= POLLIN;
					if (err.want_wr) fdelem.events |= POLLOUT;
					std::vector<pollfd> fdvec { 1, fdelem };

					retval = snf::net::poll(fdvec, to, &err.syserr);
					if (0 == retval) {
						err.syserr = ETIMEDOUT;
					}

					if (SOCKET_ERROR == retval) {
						retval = map_system_error(
								err.syserr, E_write_failed);
						break;
					}

					// try again
				}
			} else {
				if (err.syserr != 0) {
					retval = map_system_error(
							err.syserr, E_write_failed);
					break;
				} else {
					throw ssl_exception("failed to complete the handshake");
				}
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
driver::shutdown()
{
	int retval = ssl_library::instance().ssl_shutdown()(m_ssl);
	if (retval == 0) {
		retval = ssl_library::instance().ssl_shutdown()(m_ssl);
	}

	if (retval != 1) {
		ssl_error err;
		if (!decode_ssl_error(retval, err)) {
			if (err.syserr != 0) {
				throw std::system_error(
					err.syserr,
					std::system_category(),
					"failed to shutdown TLS connection");
			} else {
				throw ssl_exception("failed to shutdown TLS connection");
			}
		}
	}
}

void
driver::reset()
{
	if (ssl_library::instance().ssl_clear()(m_ssl) != 1)
		throw ssl_exception("failed to prepare TLS object for new connection");
}

} // namespace ssl
} // namespace net
} // namespace snf

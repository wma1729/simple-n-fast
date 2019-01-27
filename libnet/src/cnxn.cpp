#include "cnxn.h"
#include "error.h"
#include <algorithm>

/*
 * Server name callback for SNI.
 * This is set using SSL_CTX_set_tlsext_servername_callback() or equivalent.
 * Finds the server name specfied in the TLS extension and switch to the
 * matching context.
 *
 * @param [in] ssl - raw SSL object.
 * @param [in] arg - argument to function set using SSL_CTX_set_tlsext_servername_arg()
 *                   or equivalent.
 *
 * @return 1 on success, 0 on failure.
 */
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
	} catch (snf::net::ssl::exception &) {
		return 0;
	}

	return 1;
}

namespace snf {
namespace net {
namespace ssl {

/* Operation codes for better exception messages. */
enum class operation
{
	unknown,
	connect,
	accept,
	read,
	write,
	shutdown
};

/* Tracks error(s) and the associated operation. */
struct error_info
{
	int         error;
	int         ssl_error;
	int         os_error;
	operation   op;

	error_info() : error(0), ssl_error(0), os_error(0), op(operation::unknown) { }
};

/*
 * Stream operator to generate the detailed error message.
 */
static std::ostream &
operator<< (std::ostream &os, const error_info &ei)
{
	os << "failed to complete the handshake";
	switch (ei.op) {
		case operation::connect:
			os << " during client connect";
			break;

		case operation::accept:
			os << " during server accept";
			break;

		case operation::read:
			os << " during read";
			break;

		case operation::write:
			os << " during write";
			break;

		case operation::shutdown:
			os << " during shutdown";
			break;

		default:
			break;
	}

	os << "; error = " << ei.error
		<< ", ssl_error = " << ei.ssl_error
		<< ", os_error = " << ei.os_error;

	return os;
}

/*
 * Switches SSL context based on the given server name.
 * The X509 certificate associated with each added SSL context is
 * retrieved. The server name is then compared to the server
 * name in the certificate. If the name matches, it switches to
 * the matching SSL context.
 *
 * @param [in] servername - server name to use to find a match.
 *
 * @throws snf::net::ssl::exception if the context for the given
 *         server name is not found.
 */
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
		throw exception(oss.str());
	} else {
		std::for_each(m_contexts.begin(), m_contexts.end(),
			[](ctxinfo &ci) { ci.cur = false; });
		ssl_library::instance().ssl_set_ssl_ctx()(m_ssl, it->ctx);
		it->cur = true;
	}
}

/*
 * Gets the server name for SNI specified in the TLS extension. Only TLS server
 * calls this private function.
 *
 * @return server name.
 *
 * @throws snf::net::ssl::exception if the server name could not be retrieved.
 */
std::string
connection::get_sni()
{
	if (connection_mode::client == m_mode)
		throw exception("getting server name for SNI only possible in server mode");

	const char *name = ssl_library::instance().ssl_get_servername()
				(m_ssl, TLSEXT_NAMETYPE_host_name);
	if ((name == nullptr) || (*name == '\0'))
		throw exception("failed to get SNI from TLS extension");

	return std::string(name);
}

/*
 * Handles SSL error.
 * - Populates error_info nased on the SSL error code.
 * - Poll for the socket to get ready if appropriate when the SSL
 *   error code is SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE.
 *
 * @return E_ok on success, -ve error code on failure.
 *
 * @throws snf::net::ssl::exception if a non-retryable error is seen.
 */
int
connection::handle_ssl_error(sock_t sock, int to, error_info &ei)
{
	int retval = E_ok;

	if (ei.error > 0)
		return retval;

	pollfd fdelem = { sock, 0, 0 };

	ei.ssl_error = ssl_library::instance().ssl_get_error()(m_ssl, ei.error);

	switch (ei.ssl_error) {
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
				retval = E_ssl_error;
			} else if (ei.error == 0) {
				ei.os_error = EOF;
				retval = E_syscall_failed;
			} else if (ei.error == -1) {
				ei.os_error = snf::net::error();
				if (ei.op == operation::connect) {
					retval = E_connect_failed;
				} else if (ei.op == operation::accept) {
					retval = E_accept_failed;
				} else if (ei.op == operation::read) {
					retval = E_read_failed;
				} else if (ei.op == operation::write) {
					retval = E_write_failed;
				} else if (ei.op == operation::shutdown) {
					if (ei.os_error == 0)
						retval = E_ok;
					else
						retval = E_close_failed;
				} else {
					retval = E_syscall_failed;
				}
			}
			break;

		default:
			retval = E_ssl_error;
			break;
	}

	if ((INVALID_SOCKET != sock) && (E_try_again == retval)) {
		std::vector<pollfd> fdvec { 1, fdelem };

		retval = snf::net::poll(fdvec, to, &ei.os_error);
		if (0 == retval) {
			ei.os_error = ETIMEDOUT;
			retval = E_timed_out;
		} else if (SOCKET_ERROR == retval) {
			retval = E_syscall_failed;
		} else /* if (retval > 0) */ {
			retval = E_try_again;
		}
	}

	if (E_ssl_error == retval) {
		std::ostringstream oss;
		oss << ei;
		throw exception(oss.str());
	}

	return retval;
}

/*
 * Constructs the TLS connection.
 *
 * @param [in] m   - Connection mode: client or server.
 *                   Depending on the value, the connection is
 *                   set in either connect (client) or
 *                   accept (server) state.
 * @param [in] ctx - SSL context. The default (current) SSL context.
 *
 * @throws snf::net::ssl::exception if the connection could not be created.
 */
connection::connection(connection_mode m, context &ctx)
	: m_mode(m)
{
	m_ssl = ssl_library::instance().ssl_new()(ctx);
	if (m_ssl == nullptr)
		throw exception("failed to create SSL object");

	if (connection_mode::client == m_mode)
		ssl_library::instance().ssl_set_connect_state()(m_ssl);
	else
		ssl_library::instance().ssl_set_accept_state()(m_ssl);

	ctxinfo ci;
	ci.cur = true;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

/*
 * Copy constructor. Unlike other SSL objects, there is a real (deep)
 * copy done. The internal SSL object is not reference counted.
 * Use this with caution.
 *
 * @param [in] c - TLS connection.
 *
 * @throws snf::net::ssl::exception if copy fails.
 */
connection::connection(const connection &c)
{
	m_mode = c.m_mode;
	m_contexts = c.m_contexts;

	m_ssl = ssl_library::instance().ssl_dup()(c.m_ssl);
	if (m_ssl == nullptr)
		throw exception("failed to duplicate SSL object");
}

/*
 * Move constructor.
 */
connection::connection(connection &&c)
{
	m_mode = c.m_mode;
	m_contexts = std::move(c.m_contexts);
	m_ssl = c.m_ssl;
	c.m_ssl = nullptr;
}

/*
 * Destructor. This instance of the TLS connection is freed.
 * Remember: the internal SSL object is not reference counted.
 */
connection::~connection()
{
	if (m_ssl) {
		ssl_library::instance().ssl_free()(m_ssl);
		m_ssl = nullptr;
	}
	m_contexts.clear();
}

/*
 * Copy operator. Unlike other SSL objects, there is a real (deep)
 * copy done. The internal SSL object is not reference counted.
 * Use this with caution.
 *
 * @throws snf::net::ssl::exception if copy fails.
 */
const connection &
connection::operator=(const connection &c)
{
	if (this != &c) {
		m_mode = c.m_mode;
		m_contexts = c.m_contexts;

		m_ssl = ssl_library::instance().ssl_dup()(c.m_ssl);
		if (m_ssl == nullptr)
			throw exception("failed to duplicate SSL object");
	}
	return *this;
}

/*
 * Move operator.
 */
connection &
connection::operator=(connection &&c)
{
	if (this != &c) {
		m_mode = c.m_mode;
		m_contexts = std::move(c.m_contexts);
		m_ssl = c.m_ssl;
		c.m_ssl = nullptr;
	}
	return *this;
}

/*
 * Used by TLS server to add context in case the server has multiple names
 * and supports unique certificate for each server name.
 *
 * @param [in] ctx - SSL context.
 */
void
connection::add_context(context &ctx)
{
	if (connection_mode::client == m_mode)
		throw exception("only 1 SSL context can be added in client mode");

	std::lock_guard<std::mutex> guard(m_lock);
	ctxinfo ci;
	ci.cur = false;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

/*
 * Enables host name validation as part of the verification process. The
 * caller can specify a vector of valid host names. If the peer name matches
 * any of the host name in the certificate, the verification succeeds,
 * else the verification fails.
 *
 * @param [in] hostnames - vector of valid host names.
 *
 * @throws snf::net::ssl::exception if the host names or verification
 *         flags could not be set.
 */
void
connection::check_hosts(const std::vector<std::string> &hostnames)
{
	int retval;

	X509_VERIFY_PARAM *param = ssl_library::instance().ssl_get0_param()(m_ssl);
	if (param == nullptr)
		throw exception("failed to get X509 verify parameters");

	std::vector<std::string>::const_iterator it = hostnames.begin();
	if (it != hostnames.end()) {
		retval = ssl_library::instance().x509_verify_param_set1_host()
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to set " << *it << " to X509 verify parameters";
			throw exception(oss.str());
		}
		it++;
	}

	while (it != hostnames.end()) {
		retval = ssl_library::instance().x509_verify_param_add1_host()
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to add " << *it << " to X509 verify parameters";
			throw exception(oss.str());
		}
		it++;
	}

	unsigned int flags =
		X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT |
		X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS;
			
	ssl_library::instance().x509_verify_param_set_hostflags()
		(param, flags);
}

/*
 * Enables IP address validation as part of the verification process. The
 * caller can specify an IP address. If the peer IP address matches
 * any of the IP address in the certificate, the verification succeeds,
 * else the verification fails.
 *
 * @param [in] ia - IP address.
 *
 * @throws snf::net::ssl::exception if the IP address could not be set.
 */
void
connection::check_inaddr(const internet_address &ia)
{
	X509_VERIFY_PARAM *param = ssl_library::instance().ssl_get0_param()(m_ssl);
	if (param == nullptr)
		throw exception("failed to get X509 verify parameters");

	std::string ip = std::move(ia.str(true));
	int retval = ssl_library::instance().x509_verify_param_set1_ip_asc()
		(param, ip.c_str());
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set " << ia << " to X509 verify parameters";
		throw exception(oss.str());
	}
}

/*
 * Used by the TLS client to set the server name to use in the TLS extension.
 *
 * @param [in] servername - server name to use.
 *
 * @throws snf::net::ssl::exception if the server name could not be set.
 */
void
connection::set_sni(const std::string &servername)
{
	if (connection_mode::server == m_mode)
		throw exception("setting server name for SNI only possible in client mode");

	int retval = ssl_library::instance().ssl_ctrl()
			(m_ssl,
			SSL_CTRL_SET_TLSEXT_HOSTNAME,
			TLSEXT_NAMETYPE_host_name,
			const_cast<char *>(servername.c_str()));
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set " << servername << " for SNI in TLS extension";
		throw exception(oss.str());
	}
}

/*
 * Used by the TLS server to enable SNI handling. The server name callback and arguments
 * are registered for all contexts. All known contexts must be added before calling this
 * function using add_context().
 *
 * @throws snf::net::ssl::exception if the callback or argument could not be set.
 */
void
connection::enable_sni()
{
	if (connection_mode::client == m_mode)
		throw exception("handling SNI requests can only be enabled in server mode");

	std::lock_guard<std::mutex> guard(m_lock);

	std::vector<ctxinfo>::iterator it;
	for (it = m_contexts.begin(); it != m_contexts.end(); ++it) {
		if (ssl_library::instance().ssl_ctx_cb_ctrl()
			(it->ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_CB,
			reinterpret_cast<void (*)(void)>(sni_cb)) != 1)
				throw exception("failed to set SNI callback function");
		if (ssl_library::instance().ssl_ctx_ctrl()
			(it->ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_ARG, 0,
			reinterpret_cast<void *>(this)) != 1)
				throw exception("failed to set SNI callback argument");
	}
}

/*
 * Gets the current SSL session.
 *
 * @throws snf::net::ssl::exception if the session cannot be retrieved or
 *         no session is active.
 */
session
connection::get_session()
{
	SSL_SESSION *sess = ssl_library::instance().ssl_get_session()(m_ssl);
	if (sess == nullptr)
		throw exception("no SSL session avaliable");
	return session { sess };
}

/*
 * Sets the SSL session to use for resumption.
 *
 * @param [in] sess - SSL session to resume.
 *
 * @throws snf::net::ssl::exception if the session could not be set for
 *         resumption.
 */
void
connection::set_session(session &sess)
{
	if (ssl_library::instance().ssl_set_session()(m_ssl, sess) != 1)
		throw exception("failed to set SSL session");
}

/*
 * Determines if the session set, using set_session(), is resumed/reused.
 */
bool
connection::is_session_reused()
{
	p_ssl_session_reused session_reused =
		ssl_library::instance().ssl_session_reused();
	if (session_reused)
		return (session_reused(m_ssl) == 1);
	else
		return (ssl_library::instance().ssl_ctrl()
			(m_ssl, SSL_CTRL_GET_SESSION_REUSED, 0, NULL) == 1);
}

/*
 * Associate the raw socket with the TLS connection and
 * perform secured connect (client) or accept (server) to
 * perform the TLS handshake. This is the initial handshake.
 * Both the TLS client and the TLS server must do this.
 *
 * @param [in] s  - socket to associate with TLS connection.
 * @param [in] to - timeout in milliseconds.
 *
 * @throws snf::net::ssl::exception if the socket could not be associate with
 *         the TLS connection or the TLS handshake fails.
 */
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
		throw exception(oss.str());
	}

	do {
		error_info ei;

		if (connection_mode::client == m_mode) {
			ei.op = operation::connect;
			ei.error = ssl_library::instance().ssl_connect()(m_ssl);
		} else /* if (connection_mode::server == m_mode) */ {
			ei.op = operation::accept;
			ei.error = ssl_library::instance().ssl_accept()(m_ssl);
		}

		retval = handle_ssl_error(sock, to, ei);
		if (E_ok == retval)
			break;
	} while (retval == E_try_again);
}

/**
 * Reads from the TLS connection.
 *
 * @param [out] buf     - buffer to read the data into.
 * @param [in]  to_read - number of bytes to read.
 * @param [out] bread   - number of bytes read. This
 *                        can be less than to_read.
 * @param [in]  to      - timeout in milliseconds.
 *                        POLL_WAIT_FOREVER for inifinite wait.
 *                        POLL_WAIT_NONE for no wait.
 * @param [out] oserr   - system error code.
 *
 * @return E_ok on success, -ve error code on success.
 *
 * @throws snf::net::ssl::exception if the internal socket could not be
 *         retrieved or a SSL occurs while reading.
 */
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
		throw exception("failed to get internal socket");

	do {
		error_info ei;

		n = ssl_library::instance().ssl_read()(m_ssl, cbuf, to_read);
		if (n <= 0) {
			ei.op = operation::read;
			ei.error = n;

			retval = handle_ssl_error(sock, to, ei);
			if (E_try_again != retval) {
				if (oserr) *oserr = ei.os_error;
				break;
			}
		} else {
			retval = E_ok;
			cbuf += n;
			to_read -= n;
			nbytes += n;
		}
	} while (to_read > 0);

	*bread = nbytes;

	return retval;
}

/**
 * Writes to the TLS connection. SIGPIPE must be handled
 * explicitly while using this.
 *
 * @param [in]  buf      - buffer to write the data from.
 * @param [in]  to_write - number of bytes to write.
 * @param [out] bwritten - number of bytes written.
 * @param [in]  to       - timeout in milliseconds.
 *                         POLL_WAIT_FOREVER for inifinite wait.
 *                         POLL_WAIT_NONE for no wait.
 * @param [out] oserr    - system error code.
 *
 * @return E_ok on success, -ve error code on success.
 *
 * @throws snf::net::ssl::exception if the internal socket could not be
 *         retrieved or a SSL occurs while writing.
 */
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
		throw exception("failed to get internal socket");

	do {
		error_info ei;

		n = ssl_library::instance().ssl_write()(m_ssl, cbuf, to_write);
		if (n <= 0) {
			ei.op = operation::write;
			ei.error = n;

			retval = handle_ssl_error(sock, to, ei);
			if (E_try_again != retval) {
				if (oserr) *oserr = ei.os_error;
				break;
			}
		} else {
			retval = E_ok;
			cbuf += n;
			to_write -= n;
			nbytes += n;
		}
	} while (to_write > 0);

	*bwritten = nbytes;

	return retval;
}

/*
 * Shuts down the TLS connection.
 */
void
connection::shutdown()
{
	int retval = ssl_library::instance().ssl_shutdown()(m_ssl);
	if (retval == 0) {
		retval = ssl_library::instance().ssl_shutdown()(m_ssl);
	}

	if (retval != 1) {
		error_info ei;
		ei.op = operation::shutdown;
		ei.error = retval;
		handle_ssl_error(INVALID_SOCKET, POLL_WAIT_NONE, ei);
	}
}

/*
 * Resets the TLS connection. Look at SSL_clear() for more details.
 *
 * @throws snf::net::ssl::exception if the connection could not be reset.
 */
void
connection::reset()
{
	if (ssl_library::instance().ssl_clear()(m_ssl) != 1)
		throw exception("failed to prepare SSL object for new connection");
}

/*
 * Gets the peer certificate.
 *
 * @return pointer to the peer certificate or nullptr if the peer certificate
 *         could not be obtained or is not available.
 */
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

/*
 * Determines if the certificate verification completed successfully after
 * the handshake operation. If there is a failure, error message describing
 * the failure reason is obtained.
 *
 * @param [out] errstr - reason for certificate verification failure.
 *
 * @return true if the verification passes, false otherwise.
 */
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

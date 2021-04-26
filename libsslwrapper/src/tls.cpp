#include "tls.h"

namespace snf {
namespace ssl {

/*
 * Constructs the TLS connection.
 *
 * @param [in] server - true if server, false for client.
 *                      Depending on the value, the connection is
 *                      set in either connect (client) or
 *                      accept (server) state.
 * @param [in] ctx    - SSL context. The default (current) SSL context.
 *
 * @throws snf::ssl::exception if the connection could not be created.
 */
tls::tls(bool server, context &ctx)
	: m_server(server)
{
	m_ssl = SSL_FCN<p_ssl_new>("SSL_new")(ctx);
	if (m_ssl == nullptr)
		throw exception("failed to create SSL object");

	if (m_server)
		SSL_FCN<p_ssl_set_accept_state>("SSL_set_accept_state")(m_ssl);
	else
		SSL_FCN<p_ssl_set_connect_state>("SSL_set_connect_state")(m_ssl);
}

/*
 * Copy constructor. Unlike other SSL objects, there is a real (deep)
 * copy done. The internal SSL object is not reference counted.
 * Use this with caution.
 *
 * @param [in] c - TLS object.
 *
 * @throws snf::ssl::exception if copy fails.
 */
tls::tls(const tls &c)
{
	m_server = c.m_server;
	m_ssl = SSL_FCN<p_ssl_dup>("SSL_dup")(c.m_ssl);
	if (m_ssl == nullptr)
		throw exception("failed to duplicate SSL object");
}

/*
 * Move constructor.
 */
tls::tls(tls &&c)
{
	m_server = c.m_server;
	m_ssl = c.m_ssl;
	c.m_ssl = nullptr;
}

/*
 * Destructor. This instance of the TLS object is freed.
 * Remember: the internal SSL object is not reference counted.
 */
tls::~tls()
{
	if (m_ssl) {
		SSL_FCN<p_ssl_free>("SSL_free")(m_ssl);
		m_ssl = nullptr;
	}
}

/*
 * Copy operator. Unlike other SSL objects, there is a real (deep)
 * copy done. The internal SSL object is not reference counted.
 * Use this with caution.
 *
 * @throws snf::ssl::exception if copy fails.
 */
const tls &
tls::operator=(const tls &c)
{
	if (this != &c) {
		m_server = c.m_server;
		m_ssl = SSL_FCN<p_ssl_dup>("SSL_dup")(c.m_ssl);
		if (m_ssl == nullptr)
			throw exception("failed to duplicate SSL object");
	}
	return *this;
}

/*
 * Move operator.
 */
tls &
tls::operator=(tls &&c)
{
	if (this != &c) {
		m_server = c.m_server;
		m_ssl = c.m_ssl;
		c.m_ssl = nullptr;
	}
	return *this;
}

/**
 * Sets SSL context.
 *
 * @param [in] ctx    - SSL context. The default (current) SSL context.
 *
 * @throws snf::ssl::exception if the connection could not be created.
 */
void
tls::set_context(context &ctx)
{
	if (0 == SSL_FCN<p_ssl_set_ssl_ctx>("SSL_set_SSL_CTX")(m_ssl, ctx))
		throw exception("unable to set the SSL context");
}
 

/*
 * Enables host name validation as part of the verification process. The
 * caller can specify a vector of valid host names. If the peer name matches
 * any of the host name in the certificate, the verification succeeds,
 * else the verification fails.
 *
 * @param [in] hostnames - vector of valid host names.
 *
 * @throws snf::ssl::exception if the host names or verification
 *         flags could not be set.
 */
void
tls::check_hosts(const std::vector<std::string> &hostnames)
{
	int retval;

	X509_VERIFY_PARAM *param = SSL_FCN<p_ssl_get0_param>("SSL_get0_param")(m_ssl);
	if (param == nullptr)
		throw exception("failed to get X509 verify parameters");

	std::vector<std::string>::const_iterator it = hostnames.begin();
	if (it != hostnames.end()) {
		retval = SSL_FCN<p_x509_verify_param_set1_host>("X509_VERIFY_PARAM_set1_host")
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to set " << *it << " to X509 verify parameters";
			throw exception(oss.str());
		}
		it++;
	}

	while (it != hostnames.end()) {
		retval = SSL_FCN<p_x509_verify_param_add1_host>("X509_VERIFY_PARAM_add1_host")
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

	SSL_FCN<p_x509_verify_param_set_hostflags>("X509_VERIFY_PARAM_set_hostflags")
		(param, flags);
}

/*
 * Enables IP address validation as part of the verification process. The
 * caller can specify an IP address. If the peer IP address matches
 * any of the IP address in the certificate, the verification succeeds,
 * else the verification fails.
 *
 * @param [in] ia - IP address string.
 *
 * @throws snf::ssl::exception if the IP address could not be set.
 */
void
tls::check_inaddr(const char *ia)
{
	X509_VERIFY_PARAM *param = SSL_FCN<p_ssl_get0_param>("SSL_get0_param")(m_ssl);
	if (param == nullptr)
		throw exception("failed to get X509 verify parameters");

	int retval = SSL_FCN<p_x509_verify_param_set1_ip_asc>("X509_VERIFY_PARAM_set1_ip_asc")(param, ia);
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set " << ia << " to X509 verify parameters";
		throw exception(oss.str());
	}
}

/*
 * Gets the server name for SNI specified in the TLS extension. Only TLS server
 * calls this private function.
 *
 * @return server name.
 *
 * @throws snf::ssl::exception if the server name could not be retrieved.
 */
std::string
tls::get_sni()
{
	if (!m_server)
		throw exception("getting server name for SNI only possible in server mode");

	const char *name = SSL_FCN<p_ssl_get_servername>("SSL_get_servername")(m_ssl, TLSEXT_NAMETYPE_host_name);
	if ((name == nullptr) || (*name == '\0'))
		throw exception("failed to get SNI from TLS extension");

	return std::string(name);
}

/*
 * Used by the TLS client to set the server name to use in the TLS extension.
 *
 * @param [in] servername - server name to use.
 *
 * @throws snf::ssl::exception if the server name could not be set.
 */
void
tls::set_sni(const std::string &servername)
{
	if (m_server)
		throw exception("setting server name for SNI only possible in client mode");

	int retval = SSL_FCN<p_ssl_ctrl>("SSL_ctrl")
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
 * Gets the current SSL session.
 *
 * @throws snf::ssl::exception if the session cannot be retrieved or
 *         no session is active.
 */
session
tls::get_session()
{
	SSL_SESSION *sess = SSL_FCN<p_ssl_get_session>("SSL_get_session")(m_ssl);
	if (sess == nullptr)
		throw exception("no SSL session avaliable");
	return session { sess };
}

/*
 * Sets the SSL session to use for resumption.
 *
 * @param [in] sess - SSL session to resume.
 *
 * @throws snf::ssl::exception if the session could not be set for
 *         resumption.
 */
void
tls::set_session(session &sess)
{
	if (SSL_FCN<p_ssl_set_session>("SSL_set_session")(m_ssl, sess) != 1)
		throw exception("failed to set SSL session");
}

/*
 * Determines if the session set, using set_session(), is resumed/reused.
 */
bool
tls::is_session_reused()
{
	p_ssl_session_reused session_reused =
		SSL_FCN<p_ssl_session_reused>("SSL_session_reused");
	if (session_reused)
		return (session_reused(m_ssl) == 1);
	else
#if defined(SSL_CTRL_GET_SESSION_REUSED)
		return (SSL_FCN<p_ssl_ctrl>("SSL_ctrl")(m_ssl, SSL_CTRL_GET_SESSION_REUSED, 0, NULL) == 1);
#else
		return false;
#endif
}

/**
 * Get the socket associated with the secure connection.
 */
int
tls::get_socket()
{
	int sock = SSL_FCN<p_ssl_get_fd>("SSL_get_fd")(m_ssl);
	if (sock < 1)
		throw exception("failed to get internal socket");
	return sock;
}

/**
 * Set the socket for the secure connection.
 *
 * @param [in] s - the socket.
 */
void
tls::set_socket(int s)
{
	if (SSL_FCN<p_ssl_set_fd>("SSL_set_fd")(m_ssl, s) != 1) {
		std::ostringstream oss;
		oss << "filed to set socket " << s << " for TLS communication";
		throw exception(oss.str());
	}
}

/**
 * Used by client to perform secure connect as part of initial handshake.
 */
int
tls::connect()
{
	if (m_server)
		throw exception("connect is only possible in client mode");
	return SSL_FCN<p_ssl_connect>("SSL_connect")(m_ssl);
}

/**
 * Used by server to perform secure accept as part of initial handshake.
 */
int
tls::accept()
{
	if (!m_server)
		throw exception("accept is only possible in server mode");
	return SSL_FCN<p_ssl_accept>("SSL_accept")(m_ssl);
}

/**
 * Reads from the TLS connection.
 *
 * @param [out] buf     - buffer to read the data into.
 * @param [in]  to_read - number of bytes to read.
 * @param [out] bread   - number of bytes read. This
 *                        can be less than to_read.
 *
 * @return -ve or 0 on suspicious error conditions, 1 on success.
 */
int
tls::read(char *buf, int to_read, int *bread)
{
	int n = SSL_FCN<p_ssl_read>("SSL_read")(m_ssl, buf, to_read);
	if (n > 0) {
		*bread = n;
		n = 1;
	} else {
		*bread = 0;
	}

	return n;
}

/**
 * Writes to the TLS connection. SIGPIPE must be handled
 * explicitly while using this.
 *
 * @param [in]  buf      - buffer to write the data from.
 * @param [in]  to_write - number of bytes to write.
 * @param [out] bwritten - number of bytes written.
 *
 * @return -ve or 0 on suspicious error conditions, 1 on success.
 */
int
tls::write(const char *buf, int to_write, int *bwritten)
{
	int n = SSL_FCN<p_ssl_write>("SSL_write")(m_ssl, buf, to_write);
	if (n > 0) {
		*bwritten = n;
		n = 1;
	} else {
		*bwritten = 0;
	}

	return n;
}

/*
 * Shuts down the TLS connection.
 */
int
tls::shutdown()
{
	int retval = SSL_FCN<p_ssl_shutdown>("SSL_shutdown")(m_ssl);
	if (retval == 0)
		retval = SSL_FCN<p_ssl_shutdown>("SSL_shutdown")(m_ssl);
	return retval;
}

/*
 * Resets the TLS connection. Look at SSL_clear() for more details.
 *
 * @throws snf::ssl::exception if the TLS connection could not be reset.
 */
void
tls::reset()
{
	if (SSL_FCN<p_ssl_clear>("SSL_clear")(m_ssl) != 1)
		throw exception("failed to prepare SSL object for new connection");
}

/*
 * Gets the peer certificate.
 *
 * @return pointer to the peer certificate or nullptr if the peer certificate
 *         could not be obtained or is not available.
 */
x509_certificate *
tls::get_peer_certificate()
{
	X509 *c = SSL_FCN<p_ssl_get_peer_cert>("SSL_get_peer_certificate")(m_ssl);
	if (c) {
		x509_certificate *crt = DBG_NEW x509_certificate(c);
		SSL_FCN<p_x509_free>("X509_free")(c);
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
tls::is_verification_successful(std::string &errstr)
{
	errstr.clear();
	long result = SSL_FCN<p_ssl_get_verify_result>("SSL_get_verify_result")(m_ssl);
	if (result == X509_V_OK)
		return true;

	errstr.assign(CRYPTO_FCN<p_x509_verify_cert_error_string>("X509_verify_cert_error_string")(result));
	return false;
}

/**
 * Get SSL error.
 *
 * @param [in] rc - the return code of previous I/O call.
 *
 * @return SSL error code for the I/O operation.
 */
int
tls::get_error(int rc)
{
	return SSL_FCN<p_ssl_get_error>("SSL_get_error")(m_ssl, rc);
}

} // namespace ssl
} // namespace snf

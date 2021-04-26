#include "ctx.h"
#include "keymgr.h"
#include <time.h>
#include <sstream>

namespace snf {
namespace ssl {

static const int NEW_SESSION_KEY = 1;
static const int RETRIEVE_SESSION_KEY = !NEW_SESSION_KEY;

/*
 * A callback function for handling session tickets.
 * The callback function is called for every client instigated TLS
 * session when session ticket extension is presented in the TLS
 * hello message. It is the responsibility of this function to create
 * or retrieve the cryptographic parameters and to maintain their state.
 *
 * @param [in]    ssl        - current SSL connection.
 * @param [inout] key_name   - key name presented by the TLS client.
 * @param [inout] iv         - initialization vector of the cipher.
 * @param [inout] cipher_ctx - cipher context initialized with EVP_CIPHER_CTX_init().
 * @param [inout] hmac_ctx   - HMAC context initialized with HMAC_CTX_init().
 * @param [in]    mode       - For new sessions tickets, when the client doesn't
 *                             present a session ticket, or an attempted retrieval
 *                             of the ticket failed, or a renew option was indicated,
 *                             the callback function is called with enc equal to 1.
 *                             The callback function sets an arbitrary key_name,
 *                             initialize iv, the cipher_ctx, and the hmac_ctx.
 *
 * @return 0 - If it was not possible to set/retrieve a session ticket and the SSL/TLS
 *             session will continue by negotiating a set of cryptographic parameters.
 *             If mode is 0, the callback will be called again with mode set to 1.
 *         1 - If cipher_ctx and hmac_ctx have been set and the session can continue
 *             using those parameters.
 *         2 - If cipher_ctx and hmac_ctx have been set and the session can continue
 *             using those parameters. It also indicates that the key has expired. The
 *             next time, this callback function is called with mode set to 1.
 */
extern "C" int
ssl_session_ticket_key_cb(
	SSL *ssl,
	unsigned char *key_name,
	unsigned char *iv,
	EVP_CIPHER_CTX *cipher_ctx,
	HMAC_CTX *hmac_ctx,
	int mode)
{
	const keyrec *krec = nullptr;
	keymgr *km = context::get_keymgr();
	if (km == nullptr)
		return -1;

	if (mode == NEW_SESSION_KEY) {
		krec = km->get();
		if (krec == nullptr)
			return -1;

		memcpy(key_name, krec->key_name, KEY_SIZE);
		if (CRYPTO_FCN<p_rand_bytes>("RAND_bytes")(iv, EVP_MAX_IV_LENGTH) != 1)
			return -1;

		CRYPTO_FCN<p_evp_encrypt_init_ex>("EVP_EncryptInit_ex")(
			cipher_ctx,
			CRYPTO_FCN<p_evp_aes_256_cbc>("EVP_aes_256_cbc")(),
			nullptr,
			krec->aes_key,
			iv);

		CRYPTO_FCN<p_hmac_init_ex>("HMAC_Init_ex")(
			hmac_ctx,
			krec->hmac_key,
			HMAC_SIZE,
			CRYPTO_FCN<p_evp_sha256>("EVP_sha256")(),
			nullptr);

		return 1;
	} else {
		krec = km->find(key_name, KEY_SIZE);
		if (krec == nullptr)
			return 0;

		CRYPTO_FCN<p_evp_decrypt_init_ex>("EVP_DecryptInit_ex")(
			cipher_ctx,
			CRYPTO_FCN<p_evp_aes_256_cbc>("EVP_aes_256_cbc")(),
			nullptr,
			krec->aes_key,
			iv);

		CRYPTO_FCN<p_hmac_init_ex>("HMAC_Init_ex")(
			hmac_ctx,
			krec->hmac_key,
			HMAC_SIZE,
			CRYPTO_FCN<p_evp_sha256>("EVP_sha256")(),
			nullptr);

		time_t now = time(0);
		if (krec->expire < now)
			return 2;

		return 1;
	}
}

keymgr *context::s_km = nullptr;

/*
 * Get SSL context options.
 *
 * @return the current SSL context options.
 */
long
context::get_options()
{
	p_ssl_ctx_get_options pgetopt = SSL_FCN<p_ssl_ctx_get_options>("SSL_CTX_get_options", false);
	if (pgetopt != nullptr) {
		return pgetopt(m_ctx);
	} else {
#if defined(SSL_CTRL_OPTIONS)
		return SSL_FCN<p_ssl_ctx_ctrl>("SSL_CTX_ctrl")(m_ctx, SSL_CTRL_OPTIONS, 0, nullptr);
#else
		return -1;
#endif
	}
}

/*
 * Clear SSL context options.
 *
 * @param [in] opt - options to clear.
 *
 * @return the current SSL context options after clearing the given options.
 */
long
context::clr_options(unsigned long opt)
{
	p_ssl_ctx_clr_options pclropt = SSL_FCN<p_ssl_ctx_clr_options>("SSL_CTX_clear_options", false);
	if (pclropt != nullptr) {
		return pclropt(m_ctx, opt);
	} else {
#if defined(SSL_CTRL_CLEAR_OPTIONS)
		return SSL_FCN<p_ssl_ctx_ctrl>("SSL_CTX_ctrl")(m_ctx, SSL_CTRL_CLEAR_OPTIONS, opt, nullptr);
#else
		return -1;
#endif
	}
}

/*
 * Set SSL context options.
 *
 * @param [in] opt - options to set.
 *
 * @return the current SSL context options after setting the given options.
 */
long
context::set_options(unsigned long opt)
{
	p_ssl_ctx_set_options psetopt = SSL_FCN<p_ssl_ctx_set_options>("SSL_CTX_set_options", false);
	if (psetopt != nullptr) {
		return psetopt(m_ctx, opt);
	} else {
#if defined(SSL_CTRL_OPTIONS)
		return SSL_FCN<p_ssl_ctx_ctrl>("SSL_CTX_ctrl")(m_ctx, SSL_CTRL_OPTIONS, opt, nullptr);
#else
		return -1;
#endif
	}
}

/*
 * Disables SSL session caching.
 */
void
context::disable_session_caching()
{
	SSL_FCN<p_ssl_ctx_ctrl>("SSL_CTX_ctrl")
		(m_ctx, SSL_CTRL_SET_SESS_CACHE_MODE, SSL_SESS_CACHE_OFF, nullptr); 
}

/*
 * Gets the X509 certificate stored in the SSL context.
 *
 * @throws snf::ssl::exception if failed to fetch the certificate.
 */
x509_certificate
context::get_certificate()
{
	X509 *c = SSL_FCN<p_ssl_ctx_get0_cert>("SSL_CTX_get0_certificate")(m_ctx);
	if (c == nullptr)
		throw exception("failed to get certificate from SSL context");

	x509_certificate cert(c);
	return cert;
}

/*
 * Constructs the SSL context.
 * - Minimum TLS version is set to TLS1_VERSION.
 * - SSL context mode is set to:
 *       SSL_MODE_ENABLE_PARTIAL_WRITE |
 *       SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
 *       SSL_MODE_AUTO_RETRY
 * - SSL context option is set to:
 *       SSL_OP_NO_TICKET
 *
 * @throws snf::ssl::exception if the SSL context could not
 *         be created or the mode/options could not be applied.
 */
context::context()
{
	m_ctx = SSL_FCN<p_ssl_ctx_new>("SSL_CTX_new")
			(SSL_FCN<p_tls_method>("TLS_method")());
	if (m_ctx == nullptr)
		throw exception("failed to create ssl context");

	if (SSL_FCN<p_ssl_ctx_set_min_ver>("SSL_CTX_set_min_proto_version")(m_ctx, TLS1_VERSION) != 1) {
		SSL_FCN<p_ssl_ctx_free>("SSL_CTX_free")(m_ctx);
		throw exception("failed to set minimum protocol version");
	}

	if (SSL_FCN<p_ssl_ctx_set_max_ver>("SSL_CTX_set_max_proto_version")(m_ctx, 0) != 1) {
		SSL_FCN<p_ssl_ctx_free>("SSL_CTX_free")(m_ctx);
		throw exception("failed to set maximum protocol version");
	}

	// mimics SSL_CTX_set_mode
	long mode = SSL_MODE_ENABLE_PARTIAL_WRITE |
			SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
			SSL_MODE_AUTO_RETRY;
	SSL_FCN<p_ssl_ctx_ctrl>("SSL_CTX_ctrl")(m_ctx, SSL_CTRL_MODE, mode, nullptr);

	set_options(SSL_OP_NO_TICKET);
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw SSL context and the reference count in bumped up.
 *
 * @param [in] ctx - SSL context.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
context::context(const context &ctx)
{
	if (SSL_FCN<p_ssl_ctx_up_ref>("SSL_CTX_up_ref")(ctx.m_ctx) != 1)
		throw exception("failed to increment the ssl context reference count");
	m_ctx = ctx.m_ctx;
}

/*
 * Move constructor.
 */
context::context(context &&ctx)
{
	m_ctx = ctx.m_ctx;
	ctx.m_ctx = nullptr;		
}

/*
 * Destructor. The reference count to the SSL context is decremented. If it is the
 * last reference, the SSL context is deleted.
 */
context::~context()
{
	if (m_ctx) {
		SSL_FCN<p_ssl_ctx_free>("SSL_CTX_free")(m_ctx);
		m_ctx = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * raw SSL context and the reference count in bumped up.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
const context &
context::operator=(const context &ctx)
{
	if (this != &ctx) {
		if (SSL_FCN<p_ssl_ctx_up_ref>("SSL_CTX_up_ref")(ctx.m_ctx) != 1)
			throw exception("failed to increment the ssl context reference count");
		if (m_ctx)
			SSL_FCN<p_ssl_ctx_free>("SSL_CTX_free")(m_ctx);
		m_ctx = ctx.m_ctx;
	}
	return *this;
}

/*
 * Move operator.
 */
context &
context::operator=(context &&ctx)
{
	if (this != &ctx) {
		if (m_ctx)
			SSL_FCN<p_ssl_ctx_free>("SSL_CTX_free")(m_ctx);
		m_ctx = ctx.m_ctx;
		ctx.m_ctx = nullptr;		
	}
	return *this;
}

/*
 * Call on TLS server.
 * When choosing a cipher, use the TLS server's preferences instead of the
 * client preferences. When not set, the TLS server will always follow the
 * clients preferences.
 */
void
context::prefer_server_cipher()
{
	set_options(SSL_OP_CIPHER_SERVER_PREFERENCE);
}

/*
 * Call on TLS server.
 * This is the default. The TLS server prefers client preferenced in
 * choosing the cipher.
 */
void
context::prefer_client_cipher()
{
	clr_options(SSL_OP_CIPHER_SERVER_PREFERENCE);
}

/*
 * Gets the current SSL session timeout in seconds.
 */
time_t
context::session_timeout()
{
	return static_cast<time_t>(SSL_FCN<p_ssl_ctx_get_timeout>("SSL_CTX_get_timeout")(m_ctx));
}

/*
 * Sets SSL session timeout in seconds.
 *
 * @param [in] to - timeout in seconds.
 *
 * @return previously set SSL session timeout in seconds.
 */
time_t
context::session_timeout(time_t to)
{
	long lto = static_cast<long>(to);
	return static_cast<time_t>(SSL_FCN<p_ssl_ctx_set_timeout>("SSL_CTX_set_timeout")(m_ctx, lto));
}

/*
 * If using Session ID context based SSL resumption, the TLS server
 * should call this function to set the session ID context.
 *
 * @param [in] ctx - session ID context.
 *
 * @throws snf::ssl::exception if the session ID context is too
 *         large or if the session ID context could not be set.
 */
void
context::set_session_context(const std::string &ctx)
{
	if (ctx.size() > SSL_MAX_SSL_SESSION_ID_LENGTH)
		throw exception("session ID context is too large");

	unsigned int ctxlen = static_cast<unsigned int>(ctx.size());
	const unsigned char *pctx = reinterpret_cast<const unsigned char *>(ctx.c_str());

	if (SSL_FCN<p_ssl_ctx_set_sid_ctx>("SSL_CTX_set_session_id_context")(m_ctx, pctx, ctxlen) != 1)
		throw exception("failed to set session ID context");
}

/*
 * Enables/disables SSL session resumption using session ticket.
 *
 * @param [in] server - true if server, false for client.
 * @param [in] enable - If true, SSL resumption using session ticket
 *                      is enabled. A callback function is registered
 *                      for handling session tickets. If false, SSL
 *                      resumption using session ticket is disabled.
 *                      The callback function is un-registered.
 *
 * @throws snf::ssl::exception if the callback handler could
 *         not be registered or un-registered.
 */
void
context::session_ticket(bool server, bool enable)
{
	if (enable) {
		clr_options(SSL_OP_NO_TICKET);
	} else {
		set_options(SSL_OP_NO_TICKET);
	}

	if (server) {
		p_ssl_ctx_tlsext_ticket_key_cb cb = nullptr;
		if (enable)
			cb = ssl_session_ticket_key_cb;

		if (SSL_FCN<p_ssl_ctx_cb_ctrl>("SSL_CTX_callback_ctrl")
			(m_ctx,
			SSL_CTRL_SET_TLSEXT_TICKET_KEY_CB,
			reinterpret_cast<void (*)(void)>(cb)) != 1)
			throw exception("failed to set ticket key callback for session resumption");
		disable_session_caching();
	}
}

/*
 * Sets ciphers to use. See SSL_CTX_set_cipher_list() for the format.
 * The default is TLSv1.2:SSLv3:!aNULL:!eNULL:!aGOST:!MD5:!MEDIUM:!CAMELLIA:!PSK:!RC4::@STRENGTH.
 *
 * @param [in] ciphers - cipher list to use.
 *
 * @throws snf::ssl::exception if the cipher list could not be set.
 */
void
context::set_ciphers(const std::string &ciphers)
{
	if (SSL_FCN<p_ssl_ctx_set_ciphers>("SSL_CTX_set_cipher_list")(m_ctx, ciphers.c_str()) != 1) {
		std::ostringstream oss;
		oss << "failed to set cipher list using " << ciphers;
		throw exception(oss.str());
	}
}

/*
 * Specifies the private key to use.
 *
 * @param [in] key - private key.
 *
 * @throws snf::ssl::exception if the private key could not be used.
 */
void
context::use_private_key(pkey &key)
{
	if (SSL_FCN<p_ssl_ctx_use_private_key>("SSL_CTX_use_PrivateKey")(m_ctx, key) != 1)
		throw exception("failed to use private key");
}

/*
 * Specifies the X509 certificate to use.
 *
 * @param [in] crt - X509 certificate.
 *
 * @throws snf::ssl::exception if the X509 certificate could not be used.
 */
void
context::use_certificate(x509_certificate &crt)
{
	if (SSL_FCN<p_ssl_ctx_use_certificate>("SSL_CTX_use_certificate")(m_ctx, crt) != 1)
		throw exception("failed to use X509 certificate");
}

/*
 * Specifies the trust store to use.
 *
 * @param [in] store - trust store.
 *
 * @throws snf::ssl::exception if the trust store could not be used.
 */
void
context::use_truststore(truststore &store)
{
	X509_STORE *s = store;
	if (CRYPTO_FCN<p_x509_store_up_ref>("X509_STORE_up_ref")(s) != 1)
		throw exception("failed to increment the X509 trust store reference count");

	SSL_FCN<p_ssl_ctx_use_truststore>("SSL_CTX_set_cert_store")(m_ctx, s);
}

/*
 * Specifies the CRL to use.
 *
 * @param [in] crl - CRL.
 *
 * @throws snf::ssl::exception if the CRL could not be used.
 */
void
context::use_crl(x509_crl &crl)
{
	X509_STORE *store = SSL_FCN<p_ssl_ctx_get_cert_store>("SSL_CTX_get_cert_store")(m_ctx);
	if (store == nullptr)
		throw exception("failed to get X509 trust store for the current context");

	if (CRYPTO_FCN<p_x509_store_add_crl>("X509_STORE_add_crl")(store, crl) != 1)
		throw exception("failed to add CRL to the X509 trust store");

	unsigned long flags = X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL;
	if (CRYPTO_FCN<p_x509_store_set_flags>("X509_STORE_set_flags")(store, flags) != 1)
		throw exception("failed to set flags for the X509 trust store");
}

/*
 * Checks if the private key and the certificate matches.
 *
 * @throws snf::ssl::exception if the private key and the certificate do not match.
 */
void
context::check_private_key()
{
	if (SSL_FCN<p_ssl_ctx_check_private_key>("SSL_CTX_check_private_key")(m_ctx) != 1)
		throw exception("private key and X509 certificate do not match");
}

/*
 * Specifies if the peer certificate, if provided, should be verified.
 *
 * @param [in] require_certificate - If true and no certificate is provided, the
 *                                   verification fails.
 * @param [in] do_it_once          - Verify only for initial handshake and not for
 *                                   renegotiation.
 */
void
context::verify_peer(bool require_certificate, bool do_it_once)
{
	int mode = SSL_VERIFY_PEER;
	if (require_certificate)
		mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	if (do_it_once)
		mode |= SSL_VERIFY_CLIENT_ONCE;

	SSL_FCN<p_ssl_ctx_set_verify>("SSL_CTX_set_verify")(m_ctx, mode, nullptr);
}

/*
 * Specifies the certificate chain depth for verification.
 *
 * @param [in] depth - certificate chain depth.
 */
void
context::limit_certificate_chain_depth(int depth)
{
	SSL_FCN<p_ssl_ctx_set_verify_depth>("SSL_CTX_set_verify_depth")(m_ctx, depth);
}

void
context::set_sni_callback(void (*cb)(void), void *arg)
{
	if (SSL_FCN<p_ssl_ctx_cb_ctrl>("SSL_CTX_callback_ctrl")(m_ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_CB, cb) != 1)
		throw exception("failed to set SNI callback function");

	if (SSL_FCN<p_ssl_ctx_ctrl>("SSL_CTX_ctrl")(m_ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_ARG, 0, arg) != 1) {
		SSL_FCN<p_ssl_ctx_cb_ctrl>("SSL_CTX_callback_ctrl")(m_ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_CB, 0);
		throw exception("failed to set SNI callback argument");
	}
}

} // namespace ssl
} // namespace snf

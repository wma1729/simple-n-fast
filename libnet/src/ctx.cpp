#include "ctx.h"
#include <time.h>
#include <sstream>

static const int NEW_SESSION_KEY = 1;
static const int RETRIEVE_SESSION_KEY = !NEW_SESSION_KEY;

static snf::net::ssl::keymgr *g_km = nullptr;

extern "C" int
ssl_session_tickey_key_cb(
	SSL *ssl,
	unsigned char *key_name,
	unsigned char *iv,
	EVP_CIPHER_CTX *cipher_ctx,
	HMAC_CTX *hmac_ctx,
	int mode)
{
	const snf::net::ssl::keyrec *krec = nullptr;

	if (mode == NEW_SESSION_KEY) {
		krec = g_km->get();
		if (krec == nullptr)
			return -1;

		memcpy(key_name, krec->key_name, snf::net::ssl::KEY_SIZE);
		if (snf::net::ssl::ssl_library::instance().rand_bytes()
			(iv, EVP_MAX_IV_LENGTH) != 1)
			return -1;

		snf::net::ssl::ssl_library::instance().evp_encrypt_init_ex()(
			cipher_ctx,
			snf::net::ssl::ssl_library::instance().evp_aes_256_cbc()(),
			nullptr,
			krec->aes_key,
			iv);

		snf::net::ssl::ssl_library::instance().hmac_init_ex()(
			hmac_ctx,
			krec->hmac_key,
			snf::net::ssl::HMAC_SIZE,
			snf::net::ssl::ssl_library::instance().evp_sha256()(),
			nullptr);

		return 1;
	} else {
		krec = g_km->find(key_name, snf::net::ssl::KEY_SIZE);
		if (krec == nullptr)
			return 0;

		snf::net::ssl::ssl_library::instance().evp_decrypt_init_ex()(
			cipher_ctx,
			snf::net::ssl::ssl_library::instance().evp_aes_256_cbc()(),
			nullptr,
			krec->aes_key,
			iv);

		snf::net::ssl::ssl_library::instance().hmac_init_ex()(
			hmac_ctx,
			krec->hmac_key,
			snf::net::ssl::HMAC_SIZE,
			snf::net::ssl::ssl_library::instance().evp_sha256()(),
			nullptr);

		time_t now = time(0);
		if (krec->expire < now)
			return 2;

		return 1;
	}
}

namespace snf {
namespace net {
namespace ssl {

long
context::get_options()
{
	p_ssl_ctx_get_options pgetopt = ssl_library::instance().ssl_ctx_get_options();
	if (pgetopt != nullptr) {
		return pgetopt(m_ctx);
	} else {
		return ssl_library::instance().ssl_ctx_ctrl()
			(m_ctx, SSL_CTRL_OPTIONS, 0, nullptr);
	}
}

long
context::clr_options(unsigned long opt)
{
	p_ssl_ctx_clr_options pclropt = ssl_library::instance().ssl_ctx_clr_options();
	if (pclropt != nullptr) {
		return pclropt(m_ctx, opt);
	} else {
		return ssl_library::instance().ssl_ctx_ctrl()
			(m_ctx, SSL_CTRL_CLEAR_OPTIONS, opt, nullptr);
	}
}

long
context::set_options(unsigned long opt)
{
	p_ssl_ctx_set_options psetopt = ssl_library::instance().ssl_ctx_set_options();
	if (psetopt != nullptr) {
		return psetopt(m_ctx, opt);
	} else {
		return ssl_library::instance().ssl_ctx_ctrl()
			(m_ctx, SSL_CTRL_OPTIONS, opt, nullptr);
	}
}

context::context()
{
	m_ctx = ssl_library::instance().ssl_ctx_new()
			(ssl_library::instance().tls_method()());
	if (m_ctx == nullptr)
		throw ssl_exception("failed to create ssl context");

	if (ssl_library::instance().ssl_ctx_set_min_ver()(m_ctx, TLS1_VERSION) != 1) {
		ssl_library::instance().ssl_ctx_free()(m_ctx);
		throw ssl_exception("failed to set minimum protocol version");
	}

	if (ssl_library::instance().ssl_ctx_set_max_ver()(m_ctx, 0) != 1) {
		ssl_library::instance().ssl_ctx_free()(m_ctx);
		throw ssl_exception("failed to set maximum protocol version");
	}

	// mimics SSL_CTX_set_mode
	long mode = SSL_MODE_ENABLE_PARTIAL_WRITE |
			SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
			SSL_MODE_AUTO_RETRY;
	ssl_library::instance().ssl_ctx_ctrl()
		(m_ctx, SSL_CTRL_MODE, mode, nullptr);

	set_options(SSL_OP_NO_TICKET);
	set_options(SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
}

context::context(const context &ctx)
{
	if (ssl_library::instance().ssl_ctx_up_ref()(ctx.m_ctx) != 1)
		throw ssl_exception("failed to increment the ssl context reference count");
	m_ctx = ctx.m_ctx;
}

context::context(context &&ctx)
{
	m_ctx = ctx.m_ctx;
	ctx.m_ctx = nullptr;		
}

const context &
context::operator=(const context &ctx)
{
	if (this != &ctx) {
		if (ssl_library::instance().ssl_ctx_up_ref()(ctx.m_ctx) != 1)
			throw ssl_exception("failed to increment the ssl context reference count");
		if (m_ctx)
			ssl_library::instance().ssl_ctx_free()(m_ctx);
		m_ctx = ctx.m_ctx;
	}
	return *this;
}

context &
context::operator=(context &&ctx)
{
	if (this != &ctx) {
		if (m_ctx)
			ssl_library::instance().ssl_ctx_free()(m_ctx);
		m_ctx = ctx.m_ctx;
		ctx.m_ctx = nullptr;		
	}
	return *this;
}

context::~context()
{
	if (m_ctx) {
		ssl_library::instance().ssl_ctx_free()(m_ctx);
		m_ctx = nullptr;
	}
}

void
context::prefer_server_cipher()
{
	set_options(SSL_OP_CIPHER_SERVER_PREFERENCE);
}

void
context::prefer_client_cipher()
{
	clr_options(SSL_OP_CIPHER_SERVER_PREFERENCE);
}

void
context::disable_session_caching()
{
	ssl_library::instance().ssl_ctx_ctrl() 
		(m_ctx, SSL_CTRL_SET_SESS_CACHE_MODE, SSL_SESS_CACHE_OFF, nullptr); 
}

void
context::tickets_for_session_resumption(bool enable)
{
	if (enable) {
		clr_options(SSL_OP_NO_TICKET);
	} else {
		set_options(SSL_OP_NO_TICKET);
	}
}

void
context::set_session_context(const std::string &ctx)
{
	if (ctx.size() > SSL_MAX_SSL_SESSION_ID_LENGTH)
		throw ssl_exception("session ID context is too large");

	unsigned int ctxlen = static_cast<unsigned int>(ctx.size());
	const unsigned char *pctx = reinterpret_cast<const unsigned char *>(ctx.c_str());

	if (ssl_library::instance().ssl_ctx_set_sid_ctx()(m_ctx, pctx, ctxlen) != 1)
		throw ssl_exception("failed to set session ID context");
}

void
context::set_ciphers(const std::string &ciphers)
{
	if (ssl_library::instance().ssl_ctx_set_ciphers()(m_ctx, ciphers.c_str()) != 1) {
		std::ostringstream oss;
		oss << "failed to set cipher list using " << ciphers;
		throw ssl_exception(oss.str());
	}
}

void
context::use_private_key(pkey &key)
{
	if (ssl_library::instance().ssl_ctx_use_private_key()(m_ctx, key) != 1)
		throw ssl_exception("failed to use private key");
}

void
context::use_certificate(x509_certificate &crt)
{
	if (ssl_library::instance().ssl_ctx_use_certificate()(m_ctx, crt) != 1)
		throw ssl_exception("failed to use X509 certificate");
}

void
context::use_truststore(truststore &store)
{
	X509_STORE *s = store;
	if (ssl_library::instance().x509_store_up_ref()(s) != 1)
		throw ssl_exception("failed to increment the X509 trust store reference count");

	ssl_library::instance().ssl_ctx_use_truststore()(m_ctx, s);
}

void
context::use_crl(x509_crl &crl)
{
	X509_STORE *store = ssl_library::instance().ssl_ctx_get_cert_store()(m_ctx);
	if (store == nullptr)
		throw ssl_exception("failed to get X509 trust store for the current context");

	if (ssl_library::instance().x509_store_add_crl()(store, crl) != 1)
		throw ssl_exception("failed to add CRL to the X509 trust store");

	unsigned long flags = X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL;
	if (ssl_library::instance().x509_store_set_flags()(store, flags) != 1)
		throw ssl_exception("failed to set flags for the X509 trust store");
}

void
context::check_private_key()
{
	if (ssl_library::instance().ssl_ctx_check_private_key()(m_ctx) != 1)
		throw ssl_exception("private key and X509 certificate do not match");
}

void
context::verify_peer(bool require_certificate, bool do_it_once)
{
	int mode = SSL_VERIFY_PEER;
	if (require_certificate)
		mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	if (do_it_once)
		mode |= SSL_VERIFY_CLIENT_ONCE;

	ssl_library::instance().ssl_ctx_set_verify()(m_ctx, mode, nullptr);
}

void
context::limit_certificate_chain_depth(int depth)
{
	ssl_library::instance().ssl_ctx_set_verify_depth()(m_ctx, depth);
}

x509_certificate
context::get_certificate()
{
	X509 *c = ssl_library::instance().ssl_ctx_get0_cert()(m_ctx);
	if (c == nullptr)
		throw ssl_exception("failed to get certificate from SSL context");

	x509_certificate cert(c);
	return cert;
}

void
context::register_keymgr_for_session_tickets(keymgr *km)
{
	if (g_km) delete g_km;
	g_km = km;

	if (ssl_library::instance().ssl_ctx_set_tlsext_ticket_key_cb()
		(m_ctx, ssl_session_tickey_key_cb) != 0)
		throw ssl_exception("failed to set tickey key callback for session resumption");
}

} // namespace ssl
} // namespace net
} // namespace snf

#include "ctx.h"
#include <sstream>

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

} // namespace ssl
} // namespace net
} // namespace snf
